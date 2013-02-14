/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2011, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/

#include "solver.h"
#include "varreplacer.h"
#include "time_mem.h"
#include "searcher.h"
#include "sccfinder.h"
#include "simplifier.h"
#include "prober.h"
#include "clausevivifier.h"
#include "clausecleaner.h"
#include "solutionextender.h"
#include "varupdatehelper.h"
#include "gatefinder.h"
#include <fstream>
#include <cmath>
#include "xorfinder.h"
#include <fcntl.h>
using std::cout;
using std::endl;

#ifdef USE_OMP
#include <omp.h>
#endif

#ifdef USE_MYSQL
#include "mysqlstats.h"
#endif

//#define DEBUG_TRI_SORTED_SANITY

Solver::Solver(const SolverConf& _conf) :
    Searcher(_conf, this)
    , backupActivityInc(_conf.var_inc_start)
    , mtrand(_conf.origSeed)
    , conf(_conf)
    , needToInterrupt(false)

    //Stuff
    , nextCleanLimit(0)
    , numDecisionVars(0)
    , zeroLevAssignsByCNF(0)
    , zeroLevAssignsByThreads(0)
{
    if (conf.doSQL) {
        #ifdef USE_MYSQL
        sqlStats = new MySQLStats();

        #else

        cout<< "ERROR: "
        << "Cannot use SQL: no SQL library was found during compilation."
        << endl;

        exit(-1);
        #endif
    } else {
        sqlStats = NULL;
    }

    prober = new Prober(this);
    simplifier = new Simplifier(this);
    sCCFinder = new SCCFinder(this);
    clauseVivifier = new ClauseVivifier(this);
    clauseCleaner = new ClauseCleaner(this);
    clAllocator = new ClauseAllocator;
    varReplacer = new VarReplacer(this);
}

Solver::~Solver()
{
    delete sqlStats;
    delete prober;
    delete simplifier;
    delete sCCFinder;
    delete clauseVivifier;
    delete clauseCleaner;
    delete clAllocator;
    delete varReplacer;
}

bool Solver::addXorClause(const vector<Var>& vars, bool rhs)
{
    vector<Lit> ps(vars.size());
    for(size_t i = 0; i < vars.size(); i++) {
        ps[i] = Lit(vars[i], false);
    }

    if (!addClauseHelper(ps))
        return false;

    if (!addXorClauseInt(ps, rhs, true))
        return false;

    return okay();
}

bool Solver::addXorClauseInt(
    const vector< Lit >& lits
    , bool rhs
    , const bool attach
) {
    assert(ok);
    assert(!attach || qhead == trail.size());
    assert(decisionLevel() == 0);

    vector<Lit> ps(lits);
    std::sort(ps.begin(), ps.end());
    Lit p;
    uint32_t i, j;
    for (i = j = 0, p = lit_Undef; i != ps.size(); i++) {
        assert(!ps[i].sign()); //Every literal has to be unsigned

        if (ps[i].var() == p.var()) {
            //added, but easily removed
            j--;
            p = lit_Undef;

            //Flip rhs if neccessary
            if (value(ps[i]) != l_Undef) {
                rhs ^= value(ps[i]).getBool();
            }

        } else if (value(ps[i]) == l_Undef) {
            //If signed, unsign it and flip rhs
            if (ps[i].sign()) {
                rhs ^= true;
                ps[i] = ps[i].unsign();
            }

            //Add and remember as last one to have been added
            ps[j++] = p = ps[i];

            assert(!simplifier->getVarElimed()[p.var()]);
        } else {
            //modify rhs instead of adding
            assert(value(ps[i]) != l_Undef);
            rhs ^= value(ps[i]).getBool();
        }
    }
    ps.resize(ps.size() - (i - j));

    if (ps.size() > (0x01UL << 18)) {
        cout << "Too long clause!" << endl;
        exit(-1);
    }


    switch(ps.size()) {
        case 0:
            if (rhs)
                ok = false;
            return ok;

        case 1:
            enqueue(Lit(ps[0].var(), !rhs));
            #ifdef STATS_NEEDED
            propStats.propsUnit++;
            #endif
            if (attach)
                ok = propagate().isNULL();
            return ok;

        case 2:
            ps[0] ^= !rhs;
            addClauseInt(ps, false, ClauseStats(), attach);
            if (!ok)
                return false;

            ps[0] ^= true;
            ps[1] ^= true;
            addClauseInt(ps, false, ClauseStats(), attach);
            break;

        default:
            assert(false && "larger than 2-long XORs are not supported yet");
            break;
    }

    return ok;
}

/**
@brief Adds a clause to the problem. Should ONLY be called internally

This code is very specific in that it must NOT be called with varibles in
"ps" that have been replaced, eliminated, etc. Also, it must not be called
when the wer are in an UNSAT (!ok) state, for example. Use it carefully,
and only internally
*/
Clause* Solver::addClauseInt(
    const vector<Lit>& lits
    , const bool learnt
    , const ClauseStats stats
    , const bool attach
    , vector<Lit>* finalLits
) {
    assert(ok);
    assert(decisionLevel() == 0);
    assert(!attach || qhead == trail.size());
    #ifdef VERBOSE_DEBUG
    cout << "addClauseInt clause " << lits << endl;
    #endif //VERBOSE_DEBUG

    vector<Lit> ps(lits.size());
    std::copy(lits.begin(), lits.end(), ps.begin());

    std::sort(ps.begin(), ps.end());
    Lit p = lit_Undef;
    uint32_t i, j;
    for (i = j = 0; i != ps.size(); i++) {
        if (value(ps[i]).getBool() || ps[i] == ~p)
            return NULL;
        else if (value(ps[i]) != l_False && ps[i] != p) {
            ps[j++] = p = ps[i];

            if (varData[p.var()].elimed != ELIMED_NONE
                && varData[p.var()].elimed != ELIMED_QUEUED_VARREPLACER
            ) {
                cout << "ERROR: clause " << lits << " contains literal "
                << p << " whose variable has been eliminated (elim number "
                << (int) (varData[p.var()].elimed) << " )"
                << endl;
            }

            //Variables that have been eliminated cannot be added internally
            //as part of a clause. That's a bug
            assert(varData[p.var()].elimed == ELIMED_NONE
                    || varData[p.var()].elimed == ELIMED_QUEUED_VARREPLACER);
        }
    }
    ps.resize(ps.size() - (i - j));

    //If caller required final set of lits, return it.
    if (finalLits)
        *finalLits = ps;

    //Handle special cases
    switch (ps.size()) {
        case 0:
            ok = false;
            return NULL;
        case 1:
            enqueue(ps[0]);
            #ifdef STATS_NEEDED
            propStats.propsUnit++;
            #endif
            if (attach)
                ok = (propagate().isNULL());

            return NULL;
        case 2:
            attachBinClause(ps[0], ps[1], learnt);
            return NULL;

        case 3:
            attachTriClause(ps[0], ps[1], ps[2], learnt);
            return NULL;

        default:
            Clause* c = clAllocator->Clause_new(ps, sumStats.conflStats.numConflicts);
            if (learnt)
                c->makeLearnt(stats.glue);
            c->stats = stats;

            //In class 'Simplifier' we don't need to attach normall
            if (attach)
                attachClause(*c);
            else {
                if (learnt)
                    binTri.redLits += ps.size();
                else
                    binTri.irredLits += ps.size();
            }

            return c;
    }
}

void Solver::attachClause(
    const Clause& cl
    , const bool checkAttach
) {
    //Update stats
    if (cl.learnt())
        binTri.redLits += cl.size();
    else
        binTri.irredLits += cl.size();

    //Call Solver's function for heavy-lifting
    PropEngine::attachClause(cl, checkAttach);
}

void Solver::attachTriClause(
    const Lit lit1
    , const Lit lit2
    , const Lit lit3
    , const bool learnt
) {
    //Update stats
    if (learnt) {
        binTri.redLits += 3;
        binTri.redTris++;
    } else {
        binTri.irredLits += 3;
        binTri.irredTris++;
    }

    //Call Solver's function for heavy-lifting
    PropEngine::attachTriClause(lit1, lit2, lit3, learnt);
}

void Solver::attachBinClause(
    const Lit lit1
    , const Lit lit2
    , const bool learnt
    , const bool checkUnassignedFirst
) {
    //Update stats
    if (learnt) {
        binTri.redLits += 2;
        binTri.redBins++;
    } else {
        binTri.irredLits += 2;
        binTri.irredBins++;
    }
    binTri.numNewBinsSinceSCC++;

    //Call Solver's function for heavy-lifting
    PropEngine::attachBinClause(lit1, lit2, learnt, checkUnassignedFirst);
}

void Solver::detachTriClause(
    const Lit lit1
    , const Lit lit2
    , const Lit lit3
    , const bool learnt
) {
    if (learnt) {
        binTri.redLits -= 3;
        binTri.redTris--;
    } else {
        binTri.irredLits -= 3;
        binTri.irredTris--;
    }

    PropEngine::detachTriClause(lit1, lit2, lit3, learnt);
}

void Solver::detachBinClause(
    const Lit lit1
    , const Lit lit2
    , const bool learnt
) {
    if (learnt) {
        binTri.redLits -= 2;
        binTri.redBins--;
    } else {
        binTri.irredLits -= 2;
        binTri.irredBins--;
    }

    PropEngine::detachBinClause(lit1, lit2, learnt);
}

void Solver::detachClause(const Clause& c)
{
    assert(c.size() > 3);
    detachModifiedClause(c[0], c[1], c.size(), &c);
}

void Solver::detachModifiedClause(
    const Lit lit1
    , const Lit lit2
    , const uint32_t origSize
    , const Clause* address
) {
    //Update stats
    if (address->learnt())
        binTri.redLits -= origSize;
    else
        binTri.irredLits -= origSize;

    //Call heavy-lifter
    PropEngine::detachModifiedClause(lit1, lit2, origSize, address);
}

bool Solver::addClauseHelper(vector<Lit>& ps)
{
    //If already UNSAT, just return
    if (!ok)
        return false;

    //Sanity checks
    assert(decisionLevel() == 0);
    assert(qhead == trail.size());
    if (ps.size() > (0x01UL << 18)) {
        cout << "Too long clause!" << endl;
        exit(-1);
    }
    for (vector<Lit>::const_iterator it = ps.begin(), end = ps.end(); it != end; it++) {
        assert(it->var() < nVars()
        && "Clause inserted, but variable inside has not been declared with PropEngine::newVar() !");
    }

    for (uint32_t i = 0; i != ps.size(); i++) {
        //Update to correct var
        ps[i] = varReplacer->getReplaceTable()[ps[i].var()] ^ ps[i].sign();

        //Uneliminate var if need be
        if (simplifier->getVarElimed()[ps[i].var()]) {
            if (!simplifier->unEliminate(ps[i].var()))
                return false;
        }
    }

    //Randomise
    for (uint32_t i = 0; i < ps.size(); i++) {
        std::swap(ps[i], ps[(mtrand.randInt() % (ps.size()-i)) + i]);
    }

    return true;
}

/**
@brief Adds a clause to the problem. Calls addClauseInt() for heavy-lifting

Checks whether the
variables of the literals in "ps" have been eliminated/replaced etc. If so,
it acts on them such that they are correct, and calls addClauseInt() to do
the heavy-lifting
*/
bool Solver::addClause(const vector<Lit>& lits)
{
    if (simplifier->getAnythingHasBeenBlocked()) {
        cout
        << "ERROR: Cannot add new clauses to the system if blocking was"
        << " enabled. Turn it off from conf.doBlockClauses"
        << endl;
        exit(-1);
    }

    #ifdef VERBOSE_DEBUG
    cout << "Adding clause " << lits << endl;
    #endif //VERBOSE_DEBUG
    const size_t origTrailSize = trail.size();

    vector<Lit> ps = lits;
    if (!addClauseHelper(ps))
        return false;

    Clause* cl = addClauseInt(ps);

    if (cl != NULL) {
        ClOffset offset = clAllocator->getOffset(cl);
        longIrredCls.push_back(offset);
    }

    zeroLevAssignsByCNF += trail.size() - origTrailSize;
    return ok;
}

bool Solver::addLearntClause(
    const vector<Lit>& lits
    , const ClauseStats& stats
) {
    vector<Lit> ps(lits.size());
    std::copy(lits.begin(), lits.end(), ps.begin());

    if (!addClauseHelper(ps))
        return false;

    Clause* cl = addClauseInt(ps, true, stats);
    if (cl != NULL) {
        ClOffset offset = clAllocator->getOffset(cl);
        longRedCls.push_back(offset);
    }

    return ok;
}

void Solver::reArrangeClause(ClOffset offset)
{
    Clause& cl = *clAllocator->getPointer(offset);
    assert(cl.size() > 3);
    if (cl.size() == 3) return;

    //Change anything, but find the first two and assign them
    //accordingly at the ClauseData
    const Lit lit1 = cl[0];
    const Lit lit2 = cl[1];
    assert(lit1 != lit2);

    std::sort(cl.begin(), cl.end(), PolaritySorter(varData));

    uint8_t foundDatas = 0;
    for (uint16_t i = 0; i < cl.size(); i++) {
        if (cl[i] == lit1) {
            std::swap(cl[i], cl[0]);
            foundDatas++;
        }
    }

    for (uint16_t i = 0; i < cl.size(); i++) {
        if (cl[i] == lit2) {
            std::swap(cl[i], cl[1]);
            foundDatas++;
        }
    }
    assert(foundDatas == 2);
}

void Solver::reArrangeClauses()
{
    assert(decisionLevel() == 0);
    assert(ok);
    assert(qhead == trail.size());

    double myTime = cpuTime();
    for (uint32_t i = 0; i < longIrredCls.size(); i++) {
        reArrangeClause(longIrredCls[i]);
    }
    for (uint32_t i = 0; i < longRedCls.size(); i++) {
        reArrangeClause(longRedCls[i]);
    }

    if (conf.verbosity >= 3) {
        cout
        << "c Rearrange lits in clauses "
        << std::setprecision(2) << (cpuTime() - myTime)  << " s"
        << endl;
    }
}

static void printArray(const vector<Var>& array, const std::string& str)
{
    cout << str << " : " << endl;
    for(size_t i = 0; i < array.size(); i++) {
        cout << str << "[" << i << "] : " << array[i] << endl;
    }
    cout << endl;
}

//Beware. Cannot be called while Searcher is running.
void Solver::renumberVariables()
{
    double myTime = cpuTime();
    clauseCleaner->removeAndCleanAll();

    /*vector<uint32_t> myOuterToInter;
    myOuterToInter.push_back(2);
    myOuterToInter.push_back(3);
    myOuterToInter.push_back(1);
    myOuterToInter.push_back(0);
    myOuterToInter.push_back(4);
    myOuterToInter.push_back(5);

    vector<uint32_t> myInterToOUter;
    myInterToOUter.push_back(3);
    myInterToOUter.push_back(2);
    myInterToOUter.push_back(0);
    myInterToOUter.push_back(1);
    myInterToOUter.push_back(4);
    myInterToOUter.push_back(5);

    vector<uint32_t> toreorder;
    for(size_t i = 0; i < 6; i++)
        toreorder.push_back(i);

    //updateBySwap(toreorder, seen, myOuterToInter);
    updateVarsArray(toreorder, myInterToOUter);
    for(size_t i = 0; i < 6; i++) {
        cout << toreorder[i] << " , ";
    }

    cout << endl;
    exit(-1);*/

    //outerToInter[10] = 0 ---> what was 10 is now 0.

    //Fill the first part of interToOuter with vars that are used
    vector<Var> outerToInter(nVars());
    vector<Var> interToOuter(nVars());
    size_t at = 0;
    vector<Var> useless;
    for(size_t i = 0; i < nVars(); i++) {
        if (value(i) != l_Undef
            || varData[i].elimed == ELIMED_VARELIM
            || varData[i].elimed == ELIMED_VARREPLACER
        ) {
            useless.push_back(i);
            continue;
        }

        outerToInter[i] = at;
        interToOuter[at] = i;
        at++;
    }

    //Fill the rest with variables that have been removed/eliminated/set
    for(vector<Var>::const_iterator
        it = useless.begin(), end = useless.end()
        ; it != end
        ; it++
    ) {
        outerToInter[*it] = at;
        interToOuter[at] = *it;
        at++;
    }
    assert(at == nVars());

    //Create temporary outerToInter2
    vector<uint32_t> interToOuter2(interToOuter.size()*2);
    for(size_t i = 0; i < interToOuter.size(); i++) {
        interToOuter2[i*2] = interToOuter[i]*2;
        interToOuter2[i*2+1] = interToOuter[i]*2+1;
    }

    //Update updater data
    updateArray(interToOuterMain, interToOuter);
    updateArray(outerToInterMain, outerToInter);

    //For debug
    /*printArray(outerToInter, "outerToInter");
    printArray(outerToInterMain, "outerToInterMain");
    printArray(interToOuter, "interToOuter");
    printArray(interToOuterMain, "interToOuterMain");*/


    //Update local data
    updateArray(backupActivity, interToOuter);
    updateArray(backupPolarity, interToOuter);
    updateArray(decisionVar, interToOuter);
    PropEngine::updateVars(outerToInter, interToOuter, interToOuter2);
    updateLitsMap(assumptions, outerToInter);

    //Update stamps
    for(size_t i = 0; i < timestamp.size(); i++) {
        for(size_t i2 = 0; i2 < 2; i2++) {
        if (timestamp[i].dominator[i2] != lit_Undef)
            timestamp[i].dominator[i2] = getUpdatedLit(timestamp[i].dominator[i2], outerToInter);
        }
    }

    //Update clauses
    //Clauses' abstractions have to be re-calculated
    for(size_t i = 0; i < longIrredCls.size(); i++) {
        Clause* cl = clAllocator->getPointer(longIrredCls[i]);
        updateLitsMap(*cl, outerToInter);
        cl->reCalcAbstraction();
    }

    for(size_t i = 0; i < longRedCls.size(); i++) {
        Clause* cl = clAllocator->getPointer(longRedCls[i]);
        updateLitsMap(*cl, outerToInter);
        cl->reCalcAbstraction();
    }

    //Update sub-elements' vars
    updateArray(timestamp, interToOuter2);
    simplifier->updateVars(outerToInter, interToOuter);
    varReplacer->updateVars(outerToInter, interToOuter);
    implCache.updateVars(seen, outerToInter, interToOuter2);

    //Check if we renumbered the varibles in the order such as to make
    //the unknown ones first and the known/eliminated ones second
    bool uninteresting = false;
    bool problem = false;
    for(size_t i = 0; i < nVars(); i++) {
        //cout << "val[" << i << "]: " << value(i);

        if (value(i)  != l_Undef)
            uninteresting = true;

        if (varData[i].elimed == ELIMED_VARELIM
            || varData[i].elimed == ELIMED_VARREPLACER
        ) {
            uninteresting = true;
            //cout << " elimed" << endl;
        } else {
            //cout << " non-elimed" << endl;
        }

        if (value(i) == l_Undef
            && varData[i].elimed != ELIMED_VARELIM
            && varData[i].elimed != ELIMED_VARREPLACER
            && uninteresting
        ) {
            problem = true;
        }
    }
    assert(!problem && "We renumbered the variables in the wrong order!");

    //Print results
    if (conf.verbosity >= 3) {
        cout
        << "c Reordered variables T: "
        << std::fixed << std::setw(5) << std::setprecision(2)
        << (cpuTime() - myTime)
        << endl;
    }
}

Var Solver::newVar(const bool dvar)
{
    const Var var = decisionVar.size();

    outerToInterMain.push_back(var);
    interToOuterMain.push_back(var);
    decisionVar.push_back(dvar);
    numDecisionVars += dvar;

    if (conf.doCache)
        implCache.addNew();

    backupActivity.push_back(0);
    backupPolarity.push_back(false);

    Searcher::newVar();

    varReplacer->newVar();
    simplifier->newVar();

    return decisionVar.size()-1;
}

/// @brief Sort clauses according to glues: large glues first
bool Solver::reduceDBStructGlue::operator () (
    const ClOffset xOff
    , const ClOffset yOff
) {
    //Get their pointers
    const Clause* x = clAllocator->getPointer(xOff);
    const Clause* y = clAllocator->getPointer(yOff);

    const uint32_t xsize = x->size();
    const uint32_t ysize = y->size();

    //No clause should be less than 3-long: 2&3-long are not removed
    assert(xsize > 2 && ysize > 2);

    //First tie: glue
    if (x->stats.glue > y->stats.glue) return 1;
    if (x->stats.glue < y->stats.glue) return 0;

    //Second tie: size
    return xsize > ysize;
}

bool Solver::reduceDBStructActivity::operator () (
    const ClOffset xOff
    , const ClOffset yOff
) {
    //Get their pointers
    const Clause* x = clAllocator->getPointer(xOff);
    const Clause* y = clAllocator->getPointer(yOff);

    const uint32_t xsize = x->size();
    const uint32_t ysize = y->size();

    //No clause should be less than 3-long: 2&3-long are not removed
    assert(xsize > 2 && ysize > 2);

    //First tie: activity
    if (x->stats.activity < y->stats.activity) return 1;
    if (x->stats.activity > y->stats.activity) return 0;

    //Second tie: size
    return xsize > ysize;
}


/// @brief Sort clauses according to size: large sizes first
bool Solver::reduceDBStructSize::operator () (
    const ClOffset xOff
    , const ClOffset yOff
) {
    //Get their pointers
    const Clause* x = clAllocator->getPointer(xOff);
    const Clause* y = clAllocator->getPointer(yOff);

    const uint32_t xsize = x->size();
    const uint32_t ysize = y->size();

    //No clause should be less than 3-long: 2&3-long are not removed
    assert(xsize > 2 && ysize > 2);

    //First tie: size
    if (xsize > ysize) return 1;
    if (xsize < ysize) return 0;

    //Second tie: glue
    return x->stats.glue > y->stats.glue;
}

/// @brief Sort clauses according to size: small prop+confl first
bool Solver::reduceDBStructPropConfl::operator() (
    const ClOffset xOff
    , const ClOffset yOff
) {
    //Get their pointers
    const Clause* x = clAllocator->getPointer(xOff);
    const Clause* y = clAllocator->getPointer(yOff);

    const uint32_t xsize = x->size();
    const uint32_t ysize = y->size();

    //No clause should be less than 3-long: 2&3-long are not removed
    assert(xsize > 2 && ysize > 2);

    //First tie: numPropAndConfl -- notice the reversal of 1/0
    //Larger is better --> should be last in the sorted list
    if (x->stats.numPropAndConfl() != y->stats.numPropAndConfl())
        return (x->stats.numPropAndConfl() < y->stats.numPropAndConfl());

    //Second tie: size
    if (x->stats.numUsedUIP != y->stats.numUsedUIP)
        return x->stats.numUsedUIP < y->stats.numUsedUIP;

    return x->size() > y->size();
}

/**
@brief Removes learnt clauses that have been found not to be too good

Either based on glue or MiniSat-style learnt clause activities, the clauses are
sorted and then removed
*/
CleaningStats Solver::reduceDB()
{
    //Clean the clause database before doing cleaning
    varReplacer->performReplace();
    clauseCleaner->removeAndCleanAll();

    const double myTime = cpuTime();
    solveStats.nbReduceDB++;
    CleaningStats tmpStats;
    tmpStats.origNumClauses = longRedCls.size();
    tmpStats.origNumLits = binTri.redLits - binTri.redBins*2;

    //Calculate how many to remove
    size_t origRemoveNum = (double)longRedCls.size() *conf.ratioRemoveClauses;

    //If there is a ratio limit, and we are over it
    //then increase the removeNum accordingly
    size_t maxToHave = (double)(longIrredCls.size() + binTri.irredTris) * conf.maxNumLearntsRatio;
    size_t removeNum = std::max<long>(origRemoveNum, (long)longRedCls.size()-(long)maxToHave);

    if (removeNum != origRemoveNum && conf.verbosity >= 2) {
        cout
        << "c Hard upper limit reached, removing more than normal: "
        << origRemoveNum << " --> " << removeNum
        << endl;
    }

    //Subsume
    uint64_t sumConfl = solver->sumConflicts();
    simplifier->subsumeLearnts();
    if (conf.verbosity >= 3) {
        cout
        << "c Time wasted on clean&replace&sub: "
        << std::setprecision(3) << cpuTime()-myTime
        << endl;
    }

    //pre-remove
    if (conf.doPreClauseCleanPropAndConfl) {
        //Reduce based on props&confls
        size_t i, j;
        for (i = j = 0; i < longRedCls.size(); i++) {
            ClOffset offset = longRedCls[i];
            Clause* cl = clAllocator->getPointer(offset);
            assert(cl->size() > 3);
            if (cl->stats.numPropAndConfl() < conf.preClauseCleanLimit
                && cl->stats.conflictNumIntroduced + conf.preCleanMinConflTime
                    < sumStats.conflStats.numConflicts
            ) {
                //Stat update
                tmpStats.preRemove.incorporate(cl);
                tmpStats.preRemove.age += sumConfl - cl->stats.conflictNumIntroduced;

                //Check
                assert(cl->stats.conflictNumIntroduced <= sumConfl);

                if (cl->stats.glue > cl->size() + 1000) {
                    cout
                    << "c DEBUG strangely large glue: " << *cl
                    << " glue: " << cl->stats.glue
                    << " size: " << cl->size()
                    << endl;
                }

                //detach&free
                detachClause(*cl);
                clAllocator->clauseFree(offset);

            } else {
                longRedCls[j++] = offset;
            }
        }
        longRedCls.resize(longRedCls.size() -(i-j));
    }

    //Clean according to type
    tmpStats.clauseCleaningType = conf.clauseCleaningType;
    switch (conf.clauseCleaningType) {
        case CLEAN_CLAUSES_GLUE_BASED :
            //Sort for glue-based removal
            std::sort(longRedCls.begin(), longRedCls.end()
                , reduceDBStructGlue(clAllocator));
            tmpStats.glueBasedClean = 1;
            break;

        case CLEAN_CLAUSES_SIZE_BASED :
            //Sort for glue-based removal
            std::sort(longRedCls.begin(), longRedCls.end()
                , reduceDBStructSize(clAllocator));
            tmpStats.sizeBasedClean = 1;
            break;

        case CLEAN_CLAUSES_ACTIVITY_BASED :
            //Sort for glue-based removal
            std::sort(longRedCls.begin(), longRedCls.end()
                , reduceDBStructActivity(clAllocator));
            tmpStats.actBasedClean = 1;
            break;

        case CLEAN_CLAUSES_PROPCONFL_BASED :
            //Sort for glue-based removal
            std::sort(longRedCls.begin(), longRedCls.end()
                , reduceDBStructPropConfl(clAllocator));
            tmpStats.propConflBasedClean = 1;
            break;
    }

    #ifdef VERBOSE_DEBUG
    cout << "Cleaning learnt clauses. Learnt clauses after sort: " << endl;
    for (uint32_t i = 0; i != learnts.size(); i++) {
        cout << "activity:" << learnts[i]->getGlue()
        << " \tsize:" << learnts[i]->size() << endl;
    }
    #endif

    //Remove normally
    size_t i, j;
    for (i = j = 0
        ; i < longRedCls.size() && tmpStats.removed.num < removeNum
        ; i++
    ) {
        ClOffset offset = longRedCls[i];
        Clause* cl = clAllocator->getPointer(offset);
        assert(cl->size() > 3);

        //Stats Update
        tmpStats.removed.incorporate(cl);
        tmpStats.removed.age += sumConfl - cl->stats.conflictNumIntroduced;

        //Check

        //detach & free
        detachClause(*cl);
        clAllocator->clauseFree(offset);
    }

    //Count what is left
    for (; i < longRedCls.size(); i++) {
        ClOffset offset = longRedCls[i];
        Clause* cl = clAllocator->getPointer(offset);

        //Stats Update
        tmpStats.remain.incorporate(cl);
        tmpStats.remain.age += sumConfl - cl->stats.conflictNumIntroduced;

        assert(cl->stats.conflictNumIntroduced <= sumConfl);

        longRedCls[j++] = offset;
    }

    //Resize learnt datastruct
    longRedCls.resize(longRedCls.size() - (i - j));

    //Print results
    tmpStats.cpu_time = cpuTime() - myTime;
    if (conf.verbosity >= 1) {
        if (conf.verbosity >= 3)
            tmpStats.print(1);
        else
            tmpStats.printShort();
    }
    cleaningStats += tmpStats;

    return tmpStats;
}

lbool Solver::solve(const vector<Lit>* _assumptions)
{
    //Set up SQL writer
    if (conf.doSQL) {
        sqlStats->setup(this);
    }

    //Initialise stuff
    nextCleanLimitInc = conf.startClean;
    nextCleanLimit += nextCleanLimitInc;
    if (_assumptions != NULL) {
        assumptions = *_assumptions;
    }

    //Check if adding the clauses caused UNSAT
    lbool status = ok ? l_Undef : l_False;

    //If still unknown, simplify
    if (status == l_Undef && nVars() > 0)
        status = simplifyProblem();

    //Iterate until solved
    while (status == l_Undef  && !needToInterrupt) {
        if (conf.verbosity >= 2)
            printClauseSizeDistrib();

        //This is crucial, since we need to attach() clauses to threads
        clauseCleaner->removeAndCleanAll();

        //Solve using threads
        const size_t origTrailSize = trail.size();
        vector<lbool> statuses;
        uint32_t numConfls = nextCleanLimit - sumStats.conflStats.numConflicts;
        assert(conf.increaseClean >= 1 && "Clean increment factor between cleaning must be >=1");
        for (size_t i = 0; i < conf.numCleanBetweenSimplify; i++) {
            numConfls+= (double)nextCleanLimitInc * std::pow(conf.increaseClean, (int)i);
        }

        status = Searcher::solve(assumptions, numConfls);
        sumStats += Searcher::getStats();
        sumPropStats += propStats;
        propStats.clear();

        //Back up activities, polairties and var_inc
        backupActivity.clear();
        backupPolarity.clear();
        backupActivity.resize(varData.size());
        backupPolarity.resize(varData.size());
        for (size_t i = 0; i < varData.size(); i++) {
            backupPolarity[i] = varData[i].polarity;
            backupActivity[i] = Searcher::getSavedActivity(i);
        }
        backupActivityInc = Searcher::getVarInc();

        if (status != l_False) {
            Searcher::resetStats();
            fullReduce();
        }

        zeroLevAssignsByThreads += trail.size() - origTrailSize;
        if (status != l_Undef)
            break;

        //Simplify
        status = simplifyProblem();
    }

    //Handle found solution
    if (status == l_False) {
        //Not much to do, just return l_False
        return l_False;
    } else if (status == l_True) {
        //Extend solution
        SolutionExtender extender(this, solution);
        extender.extend();

        //Renumber model back to original variable numbering
        updateArrayRev(model, interToOuterMain);
    }

    return status;
}

/**
@brief The function that brings together almost all CNF-simplifications

It burst-searches for given number of conflicts, then it tries all sorts of
things like variable elimination, subsumption, failed literal probing, etc.
to try to simplifcy the problem at hand.
*/
lbool Solver::simplifyProblem()
{
    assert(ok);
    testAllClauseAttach();
    checkStats();
    reArrangeClauses();

    //SCC&VAR-REPL
    if (solveStats.numSimplify > 0
        && conf.doFindAndReplaceEqLits
    ) {
        if (!sCCFinder->find2LongXors())
            goto end;

        if (varReplacer->getNewToReplaceVars() > ((double)getNumFreeVars()*0.001)) {
            if (!varReplacer->performReplace())
                goto end;
        }
    }

    //Cache clean before probing (for speed)
    if (conf.doCache) {
        implCache.clean(this);
        if (!implCache.tryBoth(this))
            goto end;
    }

    //Treat implicits
    clauseVivifier->subsumeImplicit();

    //PROBE
    updateDominators();
    if (conf.doProbe && !prober->probe())
        goto end;

    //Don't replace first -- the stamps won't work so well
    if (conf.doClausVivif && !clauseVivifier->vivify(true)) {
        goto end;
    }

    //Treat implicits
    clauseVivifier->subsumeImplicit();

    //SCC&VAR-REPL
    if (conf.doFindAndReplaceEqLits) {
        if (!sCCFinder->find2LongXors())
            goto end;

        if (!varReplacer->performReplace())
            goto end;
    }

    if (needToInterrupt) return l_Undef;

    //Var-elim, gates, subsumption, strengthening
    if (conf.doSimplify && !simplifier->simplify())
        goto end;

    //Treat implicits
    if (!clauseVivifier->strengthenImplicit())
        goto end;
    clauseVivifier->subsumeImplicit();


    //Clean cache before vivif
    if (conf.doCache) {
        implCache.clean(this);
    }

    //Vivify clauses
    if (conf.doClausVivif && !clauseVivifier->vivify(true)) {
        goto end;
    }

    //Search & replace 2-long XORs
    if (conf.doFindAndReplaceEqLits) {
        if (!sCCFinder->find2LongXors())
            goto end;

        if (varReplacer->getNewToReplaceVars() > ((double)getNumFreeVars()*0.001)) {
            if (!varReplacer->performReplace())
                goto end;
        }
    }

    if (conf.doSortWatched)
        sortWatched();

    if (conf.doRenumberVars)
        renumberVariables();

    reArrangeClauses();

    //addSymmBreakClauses();

end:
    if (conf.verbosity >= 3)
        cout << "c Simplifying finished" << endl;

    testAllClauseAttach();
    checkNoWrongAttach();

    //The algorithms above probably have changed the propagation&usage data
    //so let's clear it
    if (conf.doClearStatEveryClauseCleaning) {
        clearClauseStats(longIrredCls);
        clearClauseStats(longRedCls);
    }

    solveStats.numSimplify++;

    if (!ok) {
        return l_False;
    } else {
        checkStats();
        checkImplicitPropagated();
        return l_Undef;
    }
}

Clause* Solver::newClauseByThread(const vector<Lit>& lits, const uint32_t glue)
{
    assert(glue < 60000);
    Clause* cl = NULL;
    switch (lits.size()) {
        case 1:
        case 2:
        case 3:
            break;
        default:
            cl = clAllocator->Clause_new(lits, Searcher::sumConflicts());
            cl->makeLearnt(glue);
            ClOffset offset = clAllocator->getOffset(cl);
            longRedCls.push_back(offset);
            break;
    }

    return cl;
}

ClauseUsageStats Solver::sumClauseData(
    const vector<ClOffset>& toprint
    , const bool learnt
) const {
    vector<ClauseUsageStats> perSizeStats;
    vector<ClauseUsageStats> perGlueStats;

    //Reset stats
    ClauseUsageStats stats;

    for(vector<ClOffset>::const_iterator
        it = toprint.begin()
        , end = toprint.end()
        ; it != end
        ; it++
    ) {
        //Clause data
        ClOffset offset = *it;
        Clause& cl = *clAllocator->getPointer(offset);
        const uint32_t clause_size = cl.size();

        //We have stats on this clause
        if (cl.size() == 3)
            continue;

        //Sum stats
        stats.addStat(cl);

        //Update size statistics
        if (perSizeStats.size() < cl.size() + 1U)
            perSizeStats.resize(cl.size()+1);

        perSizeStats[clause_size].addStat(cl);

        //If learnt, sum up GLUE-based stats
        if (learnt) {
            const size_t glue = cl.stats.glue;
            assert(glue != std::numeric_limits<uint32_t>::max());
            if (perSizeStats.size() < glue + 1) {
                perSizeStats.resize(glue + 1);
            }

            perSizeStats[glue].addStat(cl);
        }

        //If lots of verbosity, print clause's individual stat
        if (conf.verbosity >= 4) {
            //Print clause data
            cout
            << "Clause size " << std::setw(4) << cl.size();
            if (cl.learnt()) {
                cout << " glue : " << std::setw(4) << cl.stats.glue;
            }
            cout
            << " Props: " << std::setw(10) << cl.stats.numProp
            << " Confls: " << std::setw(10) << cl.stats.numConfl
            #ifdef STATS_NEEDED
            << " Lit visited: " << std::setw(10)<< cl.stats.numLitVisited
            << " Looked at: " << std::setw(10)<< cl.stats.numLookedAt
            << " Props&confls/Litsvisited*10: ";
            if (cl.stats.numLitVisited > 0) {
                cout
                << std::setw(6) << std::fixed << std::setprecision(4)
                << (10.0*(double)cl.stats.numPropAndConfl()/(double)cl.stats.numLitVisited);
            }
            #endif
            ;
            cout << " UIP used: " << std::setw(10)<< cl.stats.numUsedUIP;
            cout << endl;
        }
    }

    if (conf.verbosity >= 1) {
        //Print SUM stats
        if (learnt) {
            cout << "c red  ";
        } else {
            cout << "c irred";
        }
        cout
        << " lits visit: "
        << std::setw(8) << stats.sumLitVisited/1000UL
        << "K";

        cout
        << " cls visit: "
        << std::setw(7) << stats.sumLookedAt/1000UL
        << "K";

        cout
        << " prop: "
        << std::setw(5) << stats.sumProp/1000UL
        << "K";

        cout
        << " conf: "
        << std::setw(5) << stats.sumConfl/1000UL
        << "K";

        cout
        << " UIP used: "
        << std::setw(5) << stats.sumUsedUIP/1000UL
        << "K"
        << endl;
    }

    //Print more stats
    if (conf.verbosity >= 4) {
        printPropConflStats("clause-len", perSizeStats);

        if (learnt) {
            printPropConflStats("clause-glue", perGlueStats);
        }
    }

    return stats;
}

void Solver::printPropConflStats(
    std::string name
    , const vector<ClauseUsageStats>& stats
) const {
    for(size_t i = 0; i < stats.size(); i++) {
        //Nothing to do here, no stats really
        if (stats[i].num == 0)
            continue;

        cout
        << name << " : " << std::setw(4) << i
        << " Avg. props: " << std::setw(6) << std::fixed << std::setprecision(2)
        << ((double)stats[i].sumProp/(double)stats[i].num);

        cout
        << name << " : " << std::setw(4) << i
        << " Avg. confls: " << std::setw(6) << std::fixed << std::setprecision(2)
        << ((double)stats[i].sumConfl/(double)stats[i].num);

        if (stats[i].sumLookedAt > 0) {
            cout
            << " Props&confls/looked at: " << std::setw(6) << std::fixed << std::setprecision(2)
            << ((double)stats[i].sumPropAndConfl()/(double)stats[i].sumLookedAt);
        }

        cout
        << " Avg. lits visited: " << std::setw(6) << std::fixed << std::setprecision(2)
        << ((double)stats[i].sumLitVisited/(double)stats[i].num);

        if (stats[i].sumLookedAt > 0) {
            cout
            << " Lits visited/looked at: " << std::setw(6) << std::fixed << std::setprecision(2)
            << ((double)stats[i].sumLitVisited/(double)stats[i].sumLookedAt);
        }

        if (stats[i].sumLitVisited > 0) {
            cout
            << " Props&confls/Litsvisited*10: "
            << std::setw(6) << std::fixed << std::setprecision(4)
            << (10.0*(double)stats[i].sumPropAndConfl()/(double)stats[i].sumLitVisited);
        }

        cout << endl;
    }
}

void Solver::clearClauseStats(vector<ClOffset>& clauseset)
{
    //Clear prop&confl for normal clauses
    for(vector<ClOffset>::iterator
        it = clauseset.begin(), end = clauseset.end()
        ; it != end
        ; it++
    ) {
        Clause* cl = clAllocator->getPointer(*it);
        cl->stats.clearAfterReduceDB();
    }
}

void Solver::fullReduce()
{
    ClauseUsageStats irredStats = sumClauseData(longIrredCls, false);
    ClauseUsageStats redStats   = sumClauseData(longRedCls, true);

    //Calculating summary
    ClauseUsageStats stats;
    stats += irredStats;
    stats += redStats;

    if (conf.verbosity >= 1) {
        cout
        << "c sum   lits visit: "
        << std::setw(8) << stats.sumLitVisited/1000UL
        << "K";

        cout
        << " cls visit: "
        << std::setw(7) << stats.sumLookedAt/1000UL
        << "K";

        cout
        << " prop: "
        << std::setw(5) << stats.sumProp/1000UL
        << "K";

        cout
        << " conf: "
        << std::setw(5) << stats.sumConfl/1000UL
        << "K";

        cout
        << " UIP used: "
        << std::setw(5) << stats.sumUsedUIP/1000UL
        << "K"
        << endl;
    }

    if (conf.doSQL) {
        //printClauseStatsSQL(clauses);
        //printClauseStatsSQL(learnts);
    }
    CleaningStats iterCleanStat = reduceDB();
    consolidateMem();

    if (conf.doSQL) {
        sqlStats->reduceDB(irredStats, redStats, iterCleanStat, solver);
    }

    if (conf.doClearStatEveryClauseCleaning) {
        clearClauseStats(longIrredCls);
        clearClauseStats(longRedCls);
    }

    nextCleanLimit += nextCleanLimitInc;
    nextCleanLimitInc *= conf.increaseClean;
}

void Solver::consolidateMem()
{
    clAllocator->consolidate(this, true);
}

void Solver::printFullStats() const
{
    const double cpu_time = cpuTime();
    printStatsLine("c UIP search time"
        , sumStats.cpu_time
        , sumStats.cpu_time/cpu_time*100.0
        , "% time"
    );

    cout << "c ------- FINAL TOTAL SOLVING STATS ---------" << endl;
    sumStats.print();
    #ifdef STATS_NEEDED
    sumPropStats.print(sumStats.cpu_time);
    printStatsLine("c props/decision"
        , (double)propStats.propagations/(double)sumStats.decisions
    );
    printStatsLine("c props/conflict"
        , (double)propStats.propagations/(double)sumStats.conflStats.numConflicts
    );
    #endif
    cout << "c ------- FINAL TOTAL SOLVING STATS END ---------" << endl;

    printStatsLine("c clause clean time"
        , cleaningStats.cpu_time
        , (double)cleaningStats.cpu_time/cpu_time*100.0
        , "% time"
    );
    cleaningStats.print(solveStats.nbReduceDB);

    printStatsLine("c reachability time"
        , reachStats.cpu_time
        , (double)reachStats.cpu_time/cpu_time*100.0
        , "% time"
    );
    reachStats.print();

    printStatsLine("c 0-depth assigns", trail.size()
        , (double)trail.size()/(double)nVars()*100.0
        , "% vars"
    );
    printStatsLine("c 0-depth assigns by thrds"
        , zeroLevAssignsByThreads
        , (double)zeroLevAssignsByThreads/(double)nVars()*100.0
        , "% vars"
    );
    printStatsLine("c 0-depth assigns by CNF"
        , zeroLevAssignsByCNF
        , (double)zeroLevAssignsByCNF/(double)nVars()*100.0
        , "% vars"
    );

    //Failed lit stats
    printStatsLine("c probing time"
        , prober->getStats().cpu_time
        , prober->getStats().cpu_time/cpu_time*100.0
        , "% time"
    );

    prober->getStats().print(nVars());

    //Simplifier stats
    printStatsLine("c Simplifier time"
        , simplifier->getStats().totalTime()
        , simplifier->getStats().totalTime()/cpu_time*100.0
        , "% time"
    );

    simplifier->getStats().print(nVars());

    //GateFinder stats
    /*printStatsLine("c gatefinder time"
                    , subsumer->getGateFinder()->getStats().totalTime()
                    , subsumer->getGateFinder()->getStats().totalTime()/cpu_time*100.0
                    , "% time");
    subsumer->getGateFinder()->getStats().print(nVars());

    //XOR stats
    printStatsLine("c XOR time"
        , subsumer->getXorFinder()->getStats().totalTime()
        , subsumer->getXorFinder()->getStats().totalTime()/cpu_time*100.0
        , "% time"
    );
    subsumer->getXorFinder()->getStats().print(
        subsumer->getXorFinder()->getNumCalls()
    );*/

    //VarReplacer stats
    printStatsLine("c SCC time"
        , sCCFinder->getStats().cpu_time
        , sCCFinder->getStats().cpu_time/cpu_time*100.0
        , "% time"
    );
    sCCFinder->getStats().print();


    printStatsLine("c vrep replace time"
        , varReplacer->getStats().cpu_time
        , varReplacer->getStats().cpu_time/cpu_time*100.0
        , "% time"
    );

    printStatsLine("c vrep tree roots"
        , varReplacer->getNumTrees()
    );

    printStatsLine("c vrep trees' crown"
        , varReplacer->getNumReplacedVars()
        , (double)varReplacer->getNumReplacedVars()/(double)varReplacer->getNumTrees()
        , "leafs/tree"
    );
    varReplacer->getStats().print(nVars());

    //Vivifier-ASYMM stats
    printStatsLine("c vivif time"
                    , clauseVivifier->getStats().timeNorm
                    , clauseVivifier->getStats().timeNorm/cpu_time*100.0
                    , "% time");
    printStatsLine("c vivif cache-irred time"
                    , clauseVivifier->getStats().irredCacheBased.cpu_time
                    , clauseVivifier->getStats().irredCacheBased.cpu_time/cpu_time*100.0
                    , "% time");
    printStatsLine("c vivif cache-red time"
                    , clauseVivifier->getStats().redCacheBased.cpu_time
                    , clauseVivifier->getStats().redCacheBased.cpu_time/cpu_time*100.0
                    , "% time");
    clauseVivifier->getStats().print(nVars());

    //Other stats
    printStatsLine("c Conflicts in UIP"
        , sumStats.conflStats.numConflicts
        , (double)sumStats.conflStats.numConflicts/cpu_time
        , "confl/TOTAL_TIME_SEC"
    );
    printStatsLine("c Total time", cpu_time);
    printStatsLine("c Mem used"
        , memUsed()/(1024UL*1024UL)
        , "MB"
    );

    printStatsLine("c Mem for longclauses"
        , clAllocator->getMemUsed()/(1024UL*1024UL)
        , "MB"
    );

    size_t numBytesForWatch = 0;
    numBytesForWatch += watches.capacity()*sizeof(vec<Watched>);
    for(size_t i = 0; i < watches.size(); i++) {
        numBytesForWatch += watches[i].capacity()*sizeof(Watched);
    }
    printStatsLine("c Mem for watches"
        , numBytesForWatch/(1024UL*1024UL)
        , "MB"
    );

    size_t numBytesForVars = 0;
    numBytesForVars += assigns.capacity()*sizeof(lbool);
    numBytesForVars += varData.capacity()*sizeof(VarData);
    numBytesForVars += varDataLT.capacity()*sizeof(VarData::Stats);
    numBytesForVars += backupActivity.capacity()*sizeof(uint32_t);
    numBytesForVars += backupPolarity.capacity()*sizeof(bool);
    numBytesForVars += decisionVar.capacity()*sizeof(char);
    numBytesForVars += assumptions.capacity()*sizeof(Lit);

    printStatsLine("c Mem for vars"
        , numBytesForVars/(1024UL*1024UL)
        , "MB"
    );

    printStatsLine("c Mem for stamps"
        , timestamp.capacity()*sizeof(Timestamp)/(1024UL*1024UL)
        , "MB"
    );

    printStatsLine("c Mem for search stats"
        , hist.getMemUsed()/(1024UL*1024UL)
        , "MB"
    );


    size_t searchMem = 0;
    searchMem += toPropNorm.capacity()*sizeof(Lit);
    searchMem += toPropBin.capacity()*sizeof(Lit);
    searchMem += toPropRedBin.capacity()*sizeof(Lit);
    searchMem += trail.capacity()*sizeof(Lit);
    searchMem += trail_lim.capacity()*sizeof(Lit);
    searchMem += activities.capacity()*sizeof(uint32_t);
    //searchMem += order_heap.memUsed();
    printStatsLine("c Mem for search"
        , searchMem/(1024UL*1024UL)
        , "MB"
    );

    size_t tempsSize = 0;
    tempsSize += seen.capacity()*sizeof(uint16_t);
    tempsSize += seen2.capacity()*sizeof(uint16_t);
    tempsSize += toClear.capacity()*sizeof(Lit);
    tempsSize += analyze_stack.capacity()*sizeof(Lit);
    tempsSize += dummy.capacity()*sizeof(Lit);
    printStatsLine("c Mem for temporaries"
        , tempsSize/(1024UL*1024UL)
        , "MB"
    );
}

void Solver::dumpBinClauses(
    const bool alsoLearnt
    , const bool alsoNonLearnt
    , std::ostream* outfile
) const {
    uint32_t wsLit = 0;
    for (vector<vec<Watched> >::const_iterator
        it = watches.begin(), end = watches.end()
        ; it != end
        ; it++, wsLit++
    ) {
        Lit lit = Lit::toLit(wsLit);
        const vec<Watched>& ws = *it;
        for (vec<Watched>::const_iterator it2 = ws.begin(), end2 = ws.end(); it2 != end2; it2++) {
            if (it2->isBinary() && lit < it2->lit1()) {
                bool toDump = false;
                if (it2->learnt() && alsoLearnt) toDump = true;
                if (!it2->learnt() && alsoNonLearnt) toDump = true;

                if (toDump) {
                    tmpCl.clear();
                    tmpCl.push_back(getUpdatedLit(it2->lit1(), interToOuterMain));
                    tmpCl.push_back(getUpdatedLit(lit, interToOuterMain));
                    std::sort(tmpCl.begin(), tmpCl.end());

                    *outfile
                    << tmpCl[0]
                    << " "
                    << tmpCl[1]
                    << " 0"
                    << endl;
                }
            }
        }
    }
}

void Solver::dumpTriClauses(
    const bool alsoLearnt
    , const bool alsoNonLearnt
    , std::ostream* outfile
) const {
    uint32_t wsLit = 0;
    for (vector<vec<Watched> >::const_iterator
        it = watches.begin(), end = watches.end()
        ; it != end
        ; it++, wsLit++
    ) {
        Lit lit = Lit::toLit(wsLit);
        const vec<Watched>& ws = *it;
        for (vec<Watched>::const_iterator it2 = ws.begin(), end2 = ws.end(); it2 != end2; it2++) {
            //Only one instance of tri clause
            if (it2->isTri() && lit < it2->lit1()) {
                bool toDump = false;
                if (it2->learnt() && alsoLearnt) toDump = true;
                if (!it2->learnt() && alsoNonLearnt) toDump = true;

                if (toDump) {
                    tmpCl.clear();
                    tmpCl.push_back(getUpdatedLit(it2->lit1(), interToOuterMain));
                    tmpCl.push_back(getUpdatedLit(it2->lit2(), interToOuterMain));
                    tmpCl.push_back(getUpdatedLit(lit, interToOuterMain));
                    std::sort(tmpCl.begin(), tmpCl.end());

                    *outfile
                    << tmpCl[0]
                    << " "
                    << tmpCl[1]
                    << " "
                    << tmpCl[2]
                    << " 0" << endl;
                }
            }
        }
    }
}

void Solver::printClauseSizeDistrib()
{
    size_t size4 = 0;
    size_t size5 = 0;
    size_t sizeLarge = 0;
    for(vector<ClOffset>::const_iterator
        it = longIrredCls.begin(), end = longIrredCls.end()
        ; it != end
        ; it++
    ) {
        Clause* cl = clAllocator->getPointer(*it);
        switch(cl->size()) {
            case 0:
            case 1:
            case 2:
                assert(false);
                break;
            case 3:
                assert(false);
                break;
            case 4:
                size4++;
                break;
            case 5:
                size5++;
                break;
            default:
                sizeLarge++;
                break;
        }
    }

    /*for(vector<Clause*>::const_iterator it = learnts.begin(), end = learnts.end(); it != end; it++) {
        switch((*it)->size()) {
            case 0:
            case 1:
            case 2:
                assert(false);
                break;
            case 3:
                size3++;
                break;
            case 4:
                size4++;
                break;
            case 5:
                size5++;
                break;
            default:
                sizeLarge++;
                break;
        }
    }*/

    cout
    << "c"
    << " size4: " << size4
    << " size5: " << size5
    << " larger: " << sizeLarge << endl;
}

void Solver::dump2LongXorClauses(std::ostream* os) const
{
    *os
    << "c " << endl
    << "c ---------------------------------------" << endl
    << "c clauses representing 2-long XOR clauses" << endl
    << "c ---------------------------------------" << endl;
    const vector<Lit>& table = varReplacer->getReplaceTable();
    for (Var var = 0; var != table.size(); var++) {
        Lit lit = table[var];
        if (lit.var() == var)
            continue;

        tmpCl.clear();
        tmpCl.push_back(getUpdatedLit((~lit), interToOuterMain));
        tmpCl.push_back(getUpdatedLit(Lit(var, false), interToOuterMain));
        std::sort(tmpCl.begin(), tmpCl.end());

        *os
        << tmpCl[0]
        << " "
        << tmpCl[1]
        << " 0"
        << endl;

        tmpCl[0] ^= true;
        tmpCl[1] ^= true;

        *os
        << tmpCl[0]
        << " "
        << tmpCl[1]
        << " 0"
        << endl;
    }
}

void Solver::dumpUnitaryClauses(std::ostream* os) const
{
    *os
    << "c " << endl
    << "c ---------" << endl
    << "c unitaries" << endl
    << "c ---------" << endl;
    for (uint32_t i = 0, end = (trail_lim.size() > 0) ? trail_lim[0] : trail.size() ; i < end; i++) {
        *os
        << getUpdatedLit(trail[i], interToOuterMain)
        << " 0"
        << endl;
    }
}

void Solver::dumpRedClauses(
    std::ostream* os
    , const uint32_t maxSize
) const {

    dumpUnitaryClauses(os);

    *os
    << "c " << endl
    << "c ---------------------------------" << endl
    << "c learnt binary clauses (extracted from watchlists)" << endl
    << "c ---------------------------------" << endl;
    if (maxSize >= 2) {
        dumpBinClauses(true, false, os);
    }

    *os
    << "c " << endl
    << "c ---------------------------------" << endl
    << "c learnt tertiary clauses (extracted from watchlists)" << endl
    << "c ---------------------------------" << endl;
    if (maxSize >= 2) {
        dumpTriClauses(true, false, os);
    }

    if (maxSize >= 2) {
        dump2LongXorClauses(os);
    }

    *os
    << "c " << endl
    << "c --------------------" << endl
    << "c clauses from learnts" << endl
    << "c --------------------" << endl;
    for (vector<ClOffset>::const_iterator
        it = longRedCls.begin(), end = longRedCls.end()
        ; it != end
        ; it++
    ) {
        const Clause* cl = clAllocator->getPointer(*it);

        if (cl->size() <= maxSize) {
            *os << clauseBackNumbered(*cl) << " 0" << endl;

            //Dump the information about the clause
            *os
            << "c clause learnt "
            << (cl->learnt() ? "yes" : "no")
            << " stats "  << cl->stats << endl;
        }
    }
}

void Solver::dumpIrredClauses(std::ostream* os) const
{
    size_t numClauses = 0;

    //unitary clauses
    for (size_t
        i = 0, end = (trail_lim.size() > 0) ? trail_lim[0] : trail.size()
        ; i < end; i++
    ) {
        numClauses++;
    }

    //binary XOR clauses
    const vector<Lit>& table = varReplacer->getReplaceTable();
    for (Var var = 0; var != table.size(); var++) {
        Lit lit = table[var];
        if (lit.var() == var)
            continue;
        numClauses += 2;
    }

    //binary normal clauses
    numClauses += countNumBinClauses(false, true);

    //normal clauses
    numClauses += longIrredCls.size();

    //previously eliminated clauses
    const vector<BlockedClause>& blockedClauses = simplifier->getBlockedClauses();
    numClauses += blockedClauses.size();

    *os << "p cnf " << nVars() << " " << numClauses << endl;

    ////////////////////////////////////////////////////////////////////

    dumpUnitaryClauses(os);

    dump2LongXorClauses(os);

    *os
    << "c " << endl
    << "c ---------------" << endl
    << "c binary clauses" << endl
    << "c ---------------" << endl;
    dumpBinClauses(false, true, os);

    *os
    << "c " << endl
    << "c ---------------" << endl
    << "c tertiary clauses" << endl
    << "c ---------------" << endl;
    dumpTriClauses(false, true, os);

    *os
    << "c " << endl
    << "c ---------------" << endl
    << "c normal clauses" << endl
    << "c ---------------" << endl;
    for(vector<ClOffset>::const_iterator
        it = longIrredCls.begin(), end = longIrredCls.end()
        ; it != end
        ; it++
    ) {
        Clause* cl = clAllocator->getPointer(*it);
        assert(!cl->learnt());
        *os << clauseBackNumbered(*cl) << " 0" << endl;
    }

    *os
    << "c " << endl
    << "c -------------------------------" << endl
    << "c previously eliminated variables" << endl
    << "c -------------------------------" << endl;
    for (vector<BlockedClause>::const_iterator
        it = blockedClauses.begin(); it != blockedClauses.end()
        ; it++
    ) {

        //Print info about clause
        *os
        << "c next clause is eliminated/blocked on lit "
        << getUpdatedLit(it->blockedOn, interToOuterMain)
        << endl;

        //Print clause
        *os
        << clauseBackNumbered(it->lits)
        << " 0"
        << endl;
    }
}

void Solver::printAllClauses() const
{
    for(vector<ClOffset>::const_iterator
        it = longIrredCls.begin(), end = longIrredCls.end()
        ; it != end
        ; it++
    ) {
        Clause* cl = clAllocator->getPointer(*it);
        cout
        << "Normal clause offs " << *it
        << " cl: " << *cl
        << endl;
    }


    uint32_t wsLit = 0;
    for (vector<vec<Watched> >::const_iterator
        it = watches.begin(), end = watches.end()
        ; it != end
        ; it++, wsLit++
    ) {
        Lit lit = Lit::toLit(wsLit);
        const vec<Watched>& ws = *it;
        cout << "watches[" << lit << "]" << endl;
        for (vec<Watched>::const_iterator it2 = ws.begin(), end2 = ws.end(); it2 != end2; it2++) {
            if (it2->isBinary()) {
                cout << "Binary clause part: " << lit << " , " << it2->lit1() << endl;
            } else if (it2->isClause()) {
                cout << "Normal clause offs " << it2->getOffset() << endl;
            } else if (it2->isTri()) {
                cout << "Tri clause:"
                << lit << " , "
                << it2->lit1() << " , "
                << it2->lit2() << endl;
            }
        }
    }
}

bool Solver::verifyBinClauses() const
{
    uint32_t wsLit = 0;
    for (vector<vec<Watched> >::const_iterator
        it = watches.begin(), end = watches.end()
        ; it != end
        ; it++, wsLit++
    ) {
        Lit lit = Lit::toLit(wsLit);
        const vec<Watched>& ws = *it;

        for (vec<Watched>::const_iterator i = ws.begin(), end = ws.end() ; i != end; i++) {
            if (i->isBinary()
                && modelValue(lit) != l_True
                && modelValue(i->lit1()) != l_True
            ) {
                cout
                << "bin clause: "
                << lit << " , " << i->lit1()
                << " not satisfied!"
                << endl;

                cout
                << "value of unsat bin clause: "
                << value(lit) << " , " << value(i->lit1())
                << endl;

                return false;
            }
        }
    }

    return true;
}

bool Solver::verifyClauses(const vector<ClOffset>& cs) const
{
    #ifdef VERBOSE_DEBUG
    cout << "Checking clauses whether they have been properly satisfied." << endl;;
    #endif

    bool verificationOK = true;

    for (vector<ClOffset>::const_iterator
        it = cs.begin(), end = cs.end()
        ; it != end
        ; it++
    ) {
        Clause& cl = *clAllocator->getPointer(*it);
        for (uint32_t j = 0; j < cl.size(); j++)
            if (modelValue(cl[j]) == l_True)
                goto next;

        cout << "unsatisfied clause: " << cl << endl;
        verificationOK = false;
        next:
        ;
    }

    return verificationOK;
}

bool Solver::verifyModel() const
{
    bool verificationOK = true;
    verificationOK &= verifyClauses(longIrredCls);
    verificationOK &= verifyClauses(longRedCls);
    verificationOK &= verifyBinClauses();

    if (conf.verbosity >= 1 && verificationOK) {
        cout
        << "c Verified " << longIrredCls.size() << " clauses."
        << endl;
    }

    return verificationOK;
}


void Solver::checkLiteralCount() const
{
    // Check that sizes are calculated correctly:
    uint64_t cnt = 0;
    for(vector<ClOffset>::const_iterator
        it = longIrredCls.begin(), end = longIrredCls.end()
        ; it != end
        ; it++
    ) {
        const Clause* cl = clAllocator->getPointer(*it);
        cnt += cl->size();
    }

    if (binTri.irredLits != cnt) {
        cout
        << "c ERROR! literal count: "
        << binTri.irredLits
        << " , real value = "
        << cnt
        << endl;

        assert(binTri.irredLits == cnt);
    }
}

size_t Solver::getNumDecisionVars() const
{
    return numDecisionVars;
}

void Solver::setNeedToInterrupt()
{
    Searcher::setNeedToInterrupt();

    needToInterrupt = true;
}

lbool Solver::modelValue (const Lit p) const
{
    return model[p.var()] ^ p.sign();
}

void Solver::testAllClauseAttach() const
{
#ifndef DEBUG_ATTACH_MORE
    return;
#endif

    for (vector<ClOffset>::const_iterator
        it = longIrredCls.begin(), end = longIrredCls.end()
        ; it != end
        ; it++
    ) {
        assert(normClauseIsAttached(*it));
    }
}

bool Solver::normClauseIsAttached(const ClOffset offset) const
{
    bool attached = true;
    const Clause& cl = *clAllocator->getPointer(offset);
    assert(cl.size() > 3);

    attached &= findWCl(watches[cl[0].toInt()], offset);
    attached &= findWCl(watches[cl[1].toInt()], offset);

    return attached;
}

void Solver::findAllAttach() const
{
    for (uint32_t i = 0; i < watches.size(); i++) {
        const Lit lit = Lit::toLit(i);
        for (uint32_t i2 = 0; i2 < watches[i].size(); i2++) {
            const Watched& w = watches[i][i2];
            if (!w.isClause())
                continue;

            //Get clause
            Clause* cl = clAllocator->getPointer(w.getOffset());
            assert(!cl->getFreed());
            cout << (*cl) << endl;

            //Assert watch correctness
            if ((*cl)[0] != lit
                && (*cl)[1] != lit
            ) {
                cout
                << "ERROR! Clause " << (*cl)
                << " not attached?"
                << endl;
            }

            //Clause in one of the lists
            if (!findClause(w.getOffset())) {
                cout << "ERROR! did not find clause!" << endl;
            }
        }
    }
}


bool Solver::findClause(const ClOffset offset) const
{
    for (uint32_t i = 0; i < longIrredCls.size(); i++) {
        if (longIrredCls[i] == offset)
            return true;
    }
    for (uint32_t i = 0; i < longRedCls.size(); i++) {
        if (longRedCls[i] == offset)
            return true;
    }

    return false;
}

void Solver::checkNoWrongAttach() const
{
    #ifndef MORE_DEBUG
    return;
    #endif

    for (vector<ClOffset>::const_iterator
        it = longRedCls.begin(), end = longRedCls.end()
        ; it != end
        ; it++
    ) {
        const Clause& cl = *clAllocator->getPointer(*it);
        for (uint32_t i = 0; i < cl.size(); i++) {
            if (i > 0) assert(cl[i-1].var() != cl[i].var());
        }
    }
}

size_t Solver::getNumFreeVars() const
{
    assert(decisionLevel() == 0);
    uint32_t freeVars = nVars();
    freeVars -= trail.size();
    freeVars -= simplifier->getStats().numVarsElimed;
    freeVars -= varReplacer->getNumReplacedVars();

    return freeVars;
}

void Solver::printClauseStats() const
{
    //LONG irred
    if (longIrredCls.size() > 20000) {
        cout
        << " " << std::setw(4) << longIrredCls.size()/1000 << "K";
    } else {
        cout
        << " " << std::setw(5) << longIrredCls.size();
    }

    //TRI irred
    if (binTri.irredTris > 20000) {
        cout
        << " " << std::setw(4) << binTri.irredTris/1000 << "K";
    } else {
        cout
        << " " << std::setw(5) << binTri.irredTris;
    }

    //BIN irred
    if (binTri.irredBins > 20000) {
        cout
        << " " << std::setw(4) << binTri.irredBins/1000 << "K";
    } else {
        cout
        << " " << std::setw(5) << binTri.irredBins;
    }

    //LITERALS irred
    cout
    << " " << std::setw(5) << std::fixed << std::setprecision(1)
    << (double)(binTri.irredLits - binTri.irredBins*2 - binTri.irredTris*3)
    /(double)(longIrredCls.size());

    //LONG red
    if (longRedCls.size() > 20000) {
        cout
        << " " << std::setw(4) << longRedCls.size()/1000 << "K";
    } else {
        cout
        << " " << std::setw(5) << longRedCls.size();
    }

    //TRI red
    if (binTri.redTris > 20000) {
        cout
        << " " << std::setw(4) << binTri.redTris/1000 << "K";
    } else {
        cout
        << " " << std::setw(5) << binTri.redTris;
    }

    //BIN red
    if (binTri.redBins > 20000) {
        cout
        << " " << std::setw(4) << binTri.redBins/1000 << "K";
    } else {
        cout
        << " " << std::setw(5) << binTri.redBins;
    }

    //LITERALS red
    cout
    << " " << std::setw(5) << std::fixed << std::setprecision(1)
    << (double)(binTri.redLits - binTri.redBins*2 - binTri.redTris*3)
        /(double)(longRedCls.size())
    ;
}

void Solver::checkImplicitStats() const
{
    //Check number of learnt & non-learnt binary clauses
    uint64_t thisNumLearntBins = 0;
    uint64_t thisNumNonLearntBins = 0;
    uint64_t thisNumLearntTris = 0;
    uint64_t thisNumNonLearntTris = 0;

    size_t wsLit = 0;
    for(vector<vec<Watched> >::const_iterator
        it = watches.begin(), end = watches.end()
        ; it != end
        ; it++, wsLit++
    ) {
        #ifdef DEBUG_TRI_SORTED_SANITY
        const Lit lit = Lit::toLit(wsLit);
        #endif //DEBUG_TRI_SORTED_SANITY

        const vec<Watched>& ws = *it;
        for(vec<Watched>::const_iterator
            it2 = ws.begin(), end2 = ws.end()
            ; it2 != end2
            ; it2++
        ) {
            if (it2->isBinary()) {
                if (it2->learnt())
                    thisNumLearntBins++;
                else
                    thisNumNonLearntBins++;

                continue;
            }

            if (it2->isTri()) {
                assert(it2->lit1() < it2->lit2());
                assert(it2->lit1().var() != it2->lit2().var());

                #ifdef DEBUG_TRI_SORTED_SANITY
                Lit lits[3];
                lits[0] = lit;
                lits[1] = it2->lit1();
                lits[2] = it2->lit2();
                std::sort(lits, lits + 3);
                findWatchedOfTri(watches, lits[0], lits[1], lits[2], it2->learnt());
                findWatchedOfTri(watches, lits[1], lits[0], lits[2], it2->learnt());
                findWatchedOfTri(watches, lits[2], lits[0], lits[1], it2->learnt());
                #endif //DEBUG_TRI_SORTED_SANITY

                if (it2->learnt())
                    thisNumLearntTris++;
                else
                    thisNumNonLearntTris++;

                continue;
            }
        }
    }

    if (thisNumNonLearntBins/2 != binTri.irredBins) {
        cout
        << "ERROR:"
        << " thisNumNonLearntBins/2: " << thisNumNonLearntBins/2
        << " binTri.irredBins: " << binTri.irredBins
        << "thisNumNonLearntBins: " << thisNumNonLearntBins
        << "thisNumLearntBins: " << thisNumLearntBins << endl;
    }
    assert(thisNumNonLearntBins % 2 == 0);
    assert(thisNumNonLearntBins/2 == binTri.irredBins);

    if (thisNumLearntBins/2 != binTri.redBins) {
        cout
        << "ERROR:"
        << " thisNumLearntBins/2: " << thisNumLearntBins/2
        << " binTri.redBins: " << binTri.redBins
        << endl;
    }
    assert(thisNumLearntBins % 2 == 0);
    assert(thisNumLearntBins/2 == binTri.redBins);

    if (thisNumNonLearntTris/3 != binTri.irredTris) {
        cout
        << "ERROR:"
        << " thisNumNonLearntTris/3: " << thisNumNonLearntTris/3
        << " binTri.irredTris: " << binTri.irredTris
        << endl;
    }
    assert(thisNumNonLearntTris % 3 == 0);
    assert(thisNumNonLearntTris/3 == binTri.irredTris);

    if (thisNumLearntTris/3 != binTri.redTris) {
        cout
        << "ERROR:"
        << " thisNumLearntTris/3: " << thisNumLearntTris/3
        << " binTri.redTris: " << binTri.redTris
        << endl;
    }
    assert(thisNumLearntTris % 3 == 0);
    assert(thisNumLearntTris/3 == binTri.redTris);
}

void Solver::checkStats(const bool allowFreed) const
{
    //If in crazy mode, don't check
    #ifdef NDEBUG
    return;
    #endif

    checkImplicitStats();

    //Count number of non-learnt literals
    uint64_t numLitsNonLearnt = binTri.irredBins*2 + binTri.irredTris*3;
    for(vector<ClOffset>::const_iterator
        it = longIrredCls.begin(), end = longIrredCls.end()
        ; it != end
        ; it++
    ) {
        const Clause& cl = *clAllocator->getPointer(*it);
        if (cl.freed()) {
            assert(allowFreed);
        } else {
            numLitsNonLearnt += cl.size();
        }
    }

    //Count number of learnt literals
    uint64_t numLitsLearnt = binTri.redBins*2 + binTri.redTris*3;
    for(vector<ClOffset>::const_iterator
        it = longRedCls.begin(), end = longRedCls.end()
        ; it != end
        ; it++
    ) {
        const Clause& cl = *clAllocator->getPointer(*it);
        if (cl.freed()) {
            assert(allowFreed);
        } else {
            numLitsLearnt += cl.size();
        }
    }

    //Check counts
    if (numLitsNonLearnt != binTri.irredLits) {
        cout << "ERROR: " << endl;
        cout << "->numLitsNonLearnt: " << numLitsNonLearnt << endl;
        cout << "->binTri.irredLits: " << binTri.irredLits << endl;
    }
    if (numLitsLearnt != binTri.redLits) {
        cout << "ERROR: " << endl;
        cout << "->numLitsLearnt: " << numLitsLearnt << endl;
        cout << "->binTri.redLits: " << binTri.redLits << endl;
    }
    assert(numLitsNonLearnt == binTri.irredLits);
    assert(numLitsLearnt == binTri.redLits);
}

size_t Solver::getNewToReplaceVars() const
{
    return varReplacer->getNewToReplaceVars();
}

const char* Solver::getVersion()
{
    #ifdef _MSC_VER
    return "MSVC-compiled, without GIT";
    #else
    return get_git_version();
    #endif //_MSC_VER
}


void Solver::printWatchlist(const vec<Watched>& ws, const Lit lit) const
{
    for (vec<Watched>::const_iterator
        it = ws.begin(), end = ws.end()
        ; it != end
        ; it++
    ) {
        if (it->isClause()) {
            cout
            << "Clause: " << *clAllocator->getPointer(it->getOffset());
        }

        if (it->isBinary()) {
            cout
            << "BIN: " << lit << ", " << it->lit1()
            << " (l: " << it->learnt() << ")";
        }

        if (it->isTri()) {
            cout
            << "TRI: " << lit << ", " << it->lit1() << ", " << it->lit2()
            << " (l: " << it->learnt() << ")";
        }

        cout << endl;
    }
    cout << endl;
}

void Solver::checkImplicitPropagated() const
{
    size_t wsLit = 0;
    for(vector<vec<Watched> >::const_iterator
        it = watches.begin(), end = watches.end()
        ; it != end
        ; it++, wsLit++
    ) {
        const Lit lit = Lit::toLit(wsLit);
        for(vec<Watched>::const_iterator
            it2 = it->begin(), end2 = it->end()
            ; it2 != end2
            ; it2++
        ) {
            //Satisfied, or not implicit, skip
            if (value(lit) == l_True
                || it2->isClause()
            ) {
                continue;
            }

            const lbool val1 = value(lit);
            const lbool val2 = value(it2->lit1());

            //Handle binary
            if (it2->isBinary()) {
                if (val1 == l_False) {
                    if (val2 != l_True) {
                        cout << "not prop BIN: "
                        << lit << ", " << it2->lit1()
                        << " (learnt: " << it2->learnt()
                        << endl;
                    }
                    assert(val2 == l_True);
                }

                if (val2 == l_False)
                    assert(val1 == l_True);
            }

            //Handle 3-long clause
            if (it2->isTri()) {
                const lbool val3 = value(it2->lit2());

                if (val1 == l_False
                    && val2 == l_False
                ) {
                    assert(val3 == l_True);
                }

                if (val2 == l_False
                    && val3 == l_False
                ) {
                    assert(val1 == l_True);
                }

                if (val1 == l_False
                    && val3 == l_False
                ) {
                    assert(val2 == l_True);
                }
            }
        }
    }
}

size_t Solver::getNumVarsElimed() const
{
    return simplifier->getStats().numVarsElimed;
}

size_t Solver::getNumVarsReplaced() const
{
    return varReplacer->getNumReplacedVars();
}

void Solver::dumpIfNeeded() const
{
    if (conf.needToDumpLearnts) {
        std::ofstream outfile;
        outfile.open(conf.learntsDumpFilename.c_str());
        if (!outfile) {
            cout
            << "ERROR: Couldn't open file '"
            << conf.learntsDumpFilename
            << "' for writing, cannot dump learnt clauses!"
            << endl;
        } else {
            solver->dumpRedClauses(&outfile, conf.maxDumpLearntsSize);
        }

        cout << "Dumped redundant (~learnt) clauses" << endl;
    }

    if (conf.needToDumpSimplified) {
        if (conf.verbosity >= 1) {
            cout
            << "c Dumping simplified original clauses to file '"
            << conf.simplifiedDumpFilename << "'"
            << endl;
        }

        std::ofstream outfile;
        outfile.open(conf.simplifiedDumpFilename.c_str());
        if (!outfile) {
            cout
            << "Cannot open file '"
            << conf.simplifiedDumpFilename
            << "' for writing. exiting"
            << endl;
            exit(-1);
        }

        if (okay()) {
            solver->dumpIrredClauses(&outfile);
        } else {
            outfile << "p cnf 0 1" << endl;
            outfile << "0";
        }

        cout << "Dumped irredundant (~non-learnt) clauses" << endl;
    }
}

Lit Solver::updateLit(Lit lit) const
{
    //Nothing to update
    if (lit == lit_Undef)
        return lit;

    lit = varReplacer->getLitReplacedWith(lit);

    if (varData[lit.var()].elimed)
        return lit_Undef;

    return lit;
}

void Solver::updateDominators()
{
    for(vector<Timestamp>::iterator
        it = timestamp.begin(), end = timestamp.end()
        ; it != end
        ; it++
    ) {
        for(size_t i = 0; i < 2; i++) {
            Lit newLit = updateLit(it->dominator[i]);
            it->dominator[i] = newLit;
            if (newLit == lit_Undef)
                it->numDom[i] = 0;
        }
    }
}
