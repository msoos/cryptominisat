/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos and collaborators. All rights reserved.
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
#include "sqlstats.h"
#include <fstream>
#include <cmath>
#include <fcntl.h>
#include "completedetachreattacher.h"
#include "compfinder.h"
#include "comphandler.h"
#include "subsumestrengthen.h"
#include "varupdatehelper.h"

using namespace CMSat;
using std::cout;
using std::endl;

#ifdef USE_OMP
#include <omp.h>
#endif

#ifdef USE_MYSQL
#include "mysqlstats.h"
#endif

//#define DRUP_DEBUG

//#define DEBUG_RENUMBER

//#define DEBUG_TRI_SORTED_SANITY

Solver::Solver(const SolverConf& _conf) :
    Searcher(_conf, this)
    , backupActivityInc(_conf.var_inc_start)
    , prober(NULL)
    , simplifier(NULL)
    , sCCFinder(NULL)
    , clauseVivifier(NULL)
    , clauseCleaner(NULL)
    , varReplacer(NULL)
    , compHandler(NULL)
    , mtrand(_conf.origSeed)
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

    if (conf.doProbe) {
        prober = new Prober(this);
    }
    if (conf.doSimplify) {
        simplifier = new Simplifier(this);
    }
    sCCFinder = new SCCFinder(this);
    clauseVivifier = new ClauseVivifier(this);
    clauseCleaner = new ClauseCleaner(this);
    clAllocator = new ClauseAllocator;
    varReplacer = new VarReplacer(this);
    if (conf.doCompHandler) {
        compHandler = new CompHandler(this);
    }
    Searcher::solver = this;
}

Solver::~Solver()
{
    delete compHandler;
    delete sqlStats;
    delete prober;
    delete simplifier;
    delete sCCFinder;
    delete clauseVivifier;
    delete clauseCleaner;
    delete varReplacer;
    delete clAllocator;
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

            assert(!conf.doSimplify || !simplifier->getVarElimed(p.var()));
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
            if (rhs) {
                #ifdef DRUP
                if (drup) {
                    *drup
                    << "0\n";
                }
                #endif

                ok = false;
            }
            return ok;

        case 1: {
            Lit lit = Lit(ps[0].var(), !rhs);
            enqueue(lit);
            #ifdef DRUP
            if (drup) {
                *drup
                << lit << " 0\n";
            }
            #endif

            #ifdef STATS_NEEDED
            propStats.propsUnit++;
            #endif
            if (attach)
                ok = propagate().isNULL();
            return ok;
        }

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
    , const bool red
    , ClauseStats stats
    , const bool attach
    , vector<Lit>* finalLits
    , bool addDrup
) {
    assert(ok);
    assert(decisionLevel() == 0);
    assert(!attach || qhead == trail.size());
    #ifdef VERBOSE_DEBUG
    cout << "addClauseInt clause " << lits << endl;
    #endif //VERBOSE_DEBUG

    //Make stats sane
    stats.conflictNumIntroduced = std::min<uint64_t>(Searcher::sumConflicts(), stats.conflictNumIntroduced);

    vector<Lit> ps = lits;

    std::sort(ps.begin(), ps.end());
    Lit p = lit_Undef;
    uint32_t i, j;
    for (i = j = 0; i != ps.size(); i++) {
        if (value(ps[i]).getBool() || ps[i] == ~p)
            return NULL;
        else if (value(ps[i]) != l_False && ps[i] != p) {
            ps[j++] = p = ps[i];

            if (varData[p.var()].removed != Removed::none
                && varData[p.var()].removed != Removed::queued_replacer
            ) {
                cout << "ERROR: clause " << lits << " contains literal "
                << p << " whose variable has been eliminated (elim number "
                << (int) (varData[p.var()].removed) << " )"
                << endl;
            }

            //Variables that have been eliminated cannot be added internally
            //as part of a clause. That's a bug
            assert(varData[p.var()].removed == Removed::none
                    || varData[p.var()].removed == Removed::queued_replacer);
        }
    }
    ps.resize(ps.size() - (i - j));

    #ifdef VERBOSE_DEBUG
    cout << "addClauseInt final clause " << ps << endl;
    #endif

    //If caller required final set of lits, return it.
    if (finalLits) {
        *finalLits = ps;
    }

    #ifdef DRUP
    if (drup && addDrup) {
        (*drup) << ps << " 0\n";
    }
    #endif

    //Handle special cases
    switch (ps.size()) {
        case 0:
            ok = false;
            if (conf.verbosity >= 6) {
                cout
                << "c solver received clause through addClause(): "
                << lits
                << " that became an empty clause at toplevel --> UNSAT"
                << endl;
            }
            return NULL;
        case 1:
            enqueue(ps[0]);
            #ifdef STATS_NEEDED
            propStats.propsUnit++;
            #endif
            if (attach) {
                ok = (solver->propagate().isNULL());
            }

            return NULL;
        case 2:
            attachBinClause(ps[0], ps[1], red);
            return NULL;

        case 3:
            /*cout << "Attached tri clause: "
            << ps[0] << ", "
            << ps[1] << ", "
            << ps[2]
            << endl;*/

            attachTriClause(ps[0], ps[1], ps[2], red);
            return NULL;

        default:
            Clause* c = clAllocator->Clause_new(ps, sumStats.conflStats.numConflicts);
            if (red)
                c->makeRed(stats.glue);
            c->stats = stats;

            //In class 'Simplifier' we don't need to attach normall
            if (attach)
                attachClause(*c);
            else {
                if (red)
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
    #if defined(DRUP_DEBUG) && defined(DRUP)
    if (drup) {
        for(size_t i = 0; i < cl.size(); i++) {
            *drup
            << cl[i] << " ";
        }
        *drup << "0\n";
    }
    #endif

    //Update stats
    if (cl.red())
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
    , const bool red
) {
    #if defined(DRUP_DEBUG) && defined(DRUP)
    if (drup) {
        *drup
        << lit1 << " "
        << lit2 << " "
        << lit3 << " 0\n";
    }
    #endif

    //Update stats
    if (red) {
        binTri.redTris++;
    } else {
        binTri.irredTris++;
    }

    //Call Solver's function for heavy-lifting
    PropEngine::attachTriClause(lit1, lit2, lit3, red);
}

void Solver::attachBinClause(
    const Lit lit1
    , const Lit lit2
    , const bool red
    , const bool checkUnassignedFirst
) {
    #if defined(DRUP_DEBUG) && defined(DRUP)
    if (drup) {
        *drup
        << lit1 << " "
        << lit2 << " 0\n";
    }
    #endif

    //Update stats
    if (red) {
        binTri.redBins++;
    } else {
        binTri.irredBins++;
    }
    binTri.numNewBinsSinceSCC++;

    //Call Solver's function for heavy-lifting
    PropEngine::attachBinClause(lit1, lit2, red, checkUnassignedFirst);
}

void Solver::detachTriClause(
    const Lit lit1
    , const Lit lit2
    , const Lit lit3
    , const bool red
) {
    if (red) {
        binTri.redTris--;
    } else {
        binTri.irredTris--;
    }

    PropEngine::detachTriClause(lit1, lit2, lit3, red);
}

void Solver::detachBinClause(
    const Lit lit1
    , const Lit lit2
    , const bool red
) {
    if (red) {
        binTri.redBins--;
    } else {
        binTri.irredBins--;
    }

    PropEngine::detachBinClause(lit1, lit2, red);
}

void Solver::detachClause(const Clause& cl, const bool removeDrup)
{
    #ifdef DRUP
    if (drup && doDRUP) {
        (*drup) << "d " << cl << " 0\n";
    }
    #endif

    assert(cl.size() > 3);
    detachModifiedClause(cl[0], cl[1], cl.size(), &cl);
}

void Solver::detachClause(const ClOffset offset, const bool removeDrup)
{
    Clause* cl = clAllocator->getPointer(offset);
    detachClause(*cl, removeDrup);
}

void Solver::detachModifiedClause(
    const Lit lit1
    , const Lit lit2
    , const uint32_t origSize
    , const Clause* address
) {
    //Update stats
    if (address->red())
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

    //Check for too long clauses
    if (ps.size() > (0x01UL << 18)) {
        cout << "Too long clause!" << endl;
        exit(-1);
    }

    //Check for too large variable number
    for (Lit lit: ps) {
        if (lit.var() >= nVarsReal()) {
            cout
            << "ERROR: Variable " << lit.var() + 1
            << " inserted, but max var is "
            << nVarsReal() +1
            << endl;
            exit(-1);
        }
        assert(lit.var() < nVarsReal()
        && "Clause inserted, but variable inside has not been declared with PropEngine::newVar() !");
    }

    //External var number -> Internal var number
    for (Lit& lit: ps) {
        Lit origLit = lit;

        //Update variable numbering
        lit = getUpdatedLit(lit, outerToInterMain);

        if (conf.verbosity >= 12) {
            cout
            << "var-renumber updating lit "
            << origLit
            << " to lit "
            << lit
            << endl;
        }
    }

    //Undo var replacement
    for (Lit& lit: ps) {
        Lit origLit = lit;
        lit = varReplacer->getLitReplacedWith(lit);
        #ifdef VERBOSE_DEBUG
        cout
        << "EqLit updating lit " << origLit
        << " to lit " << lit
        << endl;
        #endif
    }

    for (const Lit lit:ps) {
        //It's not too large, but larger than the saved memory size
        //then expand everything to normal size. This will take memory
        //but it's the easiest thing to do
        if (lit.var() >= nVars()) {
            unSaveVarMem();
            assert(lit.var() < nVars()
            && "After memory expansion, the variable must fit");
        }
    }

    //Uneliminate vars
    for (const Lit lit: ps) {
        if (conf.doSimplify
            && simplifier->getVarElimed(lit.var())
        ) {
            #ifdef VERBOSE_DEBUG_RECONSTRUCT
            cout << "Uneliminating var " << lit.var() + 1 << endl;
            #endif
            if (!simplifier->unEliminate(lit.var()))
                return false;
        }
    }

    //Undo comp handler
    if (conf.doCompHandler) {
        for (const Lit lit: ps) {
            if (varData[lit.var()].removed == Removed::decomposed) {
                compHandler->readdRemovedClauses();
            }
        }
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
    if (conf.doSimplify && simplifier->getAnythingHasBeenBlocked()) {
        cout
        << "ERROR: Cannot add new clauses to the system if blocking was"
        << " enabled. Turn it off from conf.doBlockClauses"
        << " (note: current state of blocking enabled: "
        << conf.doBlockClauses << " )"
        << endl;
        exit(-1);
    }

    #ifdef VERBOSE_DEBUG
    cout << "Adding clause " << lits << endl;
    #endif //VERBOSE_DEBUG
    const size_t origTrailSize = trail.size();

    vector<Lit> ps = lits;
    #ifdef DRUP
    vector<Lit> origCl = ps;
    vector<Lit> finalCl;
    #endif

    if (!addClauseHelper(ps)) {
        return false;
    }

    Clause* cl = addClauseInt(
        ps
        , false //irred
        , ClauseStats() //default stats
        , true //yes, attach
        #ifdef DRUP
        , &finalCl
        , false
        #endif
    );

    #ifdef DRUP
    //We manipulated the clause, delete
    std::sort(origCl.begin(), origCl.end());
    if (drup
        && origCl != finalCl
    ) {
        //Dump only if non-empty (UNSAT handled later)
        if (!finalCl.empty()) {
            (*drup)
            << finalCl << " 0\n";
        }

        //Empty clause, it's UNSAT
        if (!solver->okay()) {
            (*drup)
            << "0\n";
        }
        (*drup)
        << "d " << origCl << " 0\n";
    }
    #endif

    if (cl != NULL) {
        ClOffset offset = clAllocator->getOffset(cl);
        longIrredCls.push_back(offset);
    }

    zeroLevAssignsByCNF += trail.size() - origTrailSize;

    return ok;
}

bool Solver::addRedClause(
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

#ifdef DEBUG_RENUMBER
static void printArray(const vector<Var>& array, const std::string& str)
{
    cout << str << " : " << endl;
    for(size_t i = 0; i < array.size(); i++) {
        cout << str << "[" << i << "] : " << array[i] << endl;
    }
    cout << endl;
}
#endif

//Beware. Cannot be called while Searcher is running.
void Solver::renumberVariables()
{
    double myTime = cpuTime();
    clauseCleaner->removeAndCleanAll();

    //outerToInter[10] = 0 ---> what was 10 is now 0.

    #ifdef DEBUG_RENUMBER
    printArray(interToOuterMain, "interToOuterMain");
    printArray(outerToInterMain, "outerToInterMain");
    #endif

    //Fill the first part of interToOuter with vars that are used
    interToOuter.clear();
    interToOuter.resize(nVarsReal());
    outerToInter.clear();
    outerToInter.resize(nVarsReal());
    size_t at = 0;
    vector<Var> useless;
    size_t numEffectiveVars = 0;
    for(size_t i = 0; i < nVars(); i++) {
        if (value(i) != l_Undef
            || varData[i].removed == Removed::elimed
            || varData[i].removed == Removed::replaced
            || varData[i].removed == Removed::decomposed
        ) {
            useless.push_back(i);
            continue;
        }

        outerToInter[i] = at;
        interToOuter[at] = i;
        at++;
        numEffectiveVars++;
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

    //Extend to nVarsReal() --> these are just the identity transformation
    for(size_t i = nVars(); i < nVarsReal(); i++) {
        outerToInter[i] = i;
        interToOuter[i] = i;
    }

    //Create temporary outerToInter2
    interToOuter2.clear();
    interToOuter2.resize(interToOuter.size()*2);
    for(size_t i = 0; i < interToOuter.size(); i++) {
        interToOuter2[i*2] = interToOuter[i]*2;
        interToOuter2[i*2+1] = interToOuter[i]*2+1;
    }

    //Update updater data
    updateArray(interToOuterMain, interToOuter);
    updateArrayMapCopy(outerToInterMain, outerToInter);

    #ifdef DEBUG_RENUMBER
    printArray(interToOuter, "interToOuter");
    printArray(outerToInter, "outerToInter");
    printArray(interToOuterMain, "interToOuterMain");
    printArray(outerToInterMain, "outerToInterMain");
    #endif


    //Update local data
    updateArray(backupActivity, interToOuter);
    updateArray(backupPolarity, interToOuter);
    updateArray(decisionVar, interToOuter);
    PropEngine::updateVars(outerToInter, interToOuter, interToOuter2);
    updateLitsMap(assumptions, outerToInter);

    //Update stamps
    if (conf.doStamp) {

        //Update both dominators
        for(size_t i = 0; i < stamp.tstamp.size(); i++) {
            for(size_t i2 = 0; i2 < 2; i2++) {
                if (stamp.tstamp[i].dominator[i2] != lit_Undef)
                    stamp.tstamp[i].dominator[i2]
                        = getUpdatedLit(stamp.tstamp[i].dominator[i2], outerToInter);
            }
        }

        //Update the stamp. Stamp can be very large, so update by swapping
        updateBySwap(stamp.tstamp, seen, interToOuter2);
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
    if (conf.doSimplify) {
        simplifier->updateVars(outerToInter, interToOuter);
    }
    varReplacer->updateVars(outerToInter, interToOuter);
    if (conf.doCache) {
        implCache.updateVars(seen, outerToInter, interToOuter2, numEffectiveVars);
    }

    //Check if we renumbered the varibles in the order such as to make
    //the unknown ones first and the known/eliminated ones second
    bool uninteresting = false;
    bool problem = false;
    for(size_t i = 0; i < nVars(); i++) {
        //cout << "val[" << i << "]: " << value(i);

        if (value(i)  != l_Undef)
            uninteresting = true;

        if (varData[i].removed == Removed::elimed
            || varData[i].removed == Removed::replaced
            || varData[i].removed == Removed::decomposed
        ) {
            uninteresting = true;
            //cout << " removed" << endl;
        } else {
            //cout << " non-removed" << endl;
        }

        if (value(i) == l_Undef
            && varData[i].removed != Removed::elimed
            && varData[i].removed != Removed::replaced
            && varData[i].removed != Removed::decomposed
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

    //Test for reflectivity of interToOuterMain & outerToInterMain
    #ifndef NDEBUG
    vector<Var> test(nVarsReal());
    for(size_t i = 0; i  < nVarsReal(); i++) {
        test[i] = i;
    }
    updateArrayRev(test, interToOuterMain);
    #ifdef DEBUG_RENUMBER
    for(size_t i = 0; i < nVarsReal(); i++) {
        cout << i << ": "
        << std::setw(2) << test[i] << ", "
        << std::setw(2) << outerToInterMain[i]
        << endl;
    }
    #endif
    for(size_t i = 0; i < nVarsReal(); i++) {
        assert(test[i] == outerToInterMain[i]);
    }
    #ifdef DEBUG_RENUMBER
    cout << "Passed test" << endl;
    #endif
    #endif

    if (conf.doSaveMem) {
        saveVarMem(numEffectiveVars);
    }

    //Update order heap -- needed due to decisionVar update
    redoOrderHeap();
}

void Solver::saveVarMem(const uint32_t newNumVars)
{
    //never resize varData --> contains info about what is replaced/etc.
    //never resize assigns --> contains 0-level assigns
    //never resize interToOuterMain, outerToInterMain

    //printMemStats();

    watches.resize(newNumVars*2);
    watches.shrink_to_fit();
    implCache.newNumVars(newNumVars);
    stamp.newNumVars(newNumVars);

    //Resize 'seen'
    seen.resize(newNumVars*2);
    seen.shrink_to_fit();
    seen2.resize(newNumVars*2);
    seen2.shrink_to_fit();

    activities.resize(newNumVars);
    activities.shrink_to_fit();
    minNumVars = newNumVars;

    //printMemStats();
}

void Solver::unSaveVarMem()
{
    //printMemStats();

    watches.resize(nVarsReal()*2);
    implCache.newNumVars(nVarsReal());
    stamp.newNumVars(nVarsReal());

    //Resize 'seen'
    seen.resize(nVarsReal()*2);
    seen2.resize(nVarsReal()*2);

    activities.resize(nVarsReal());
    minNumVars = nVarsReal();

    //printMemStats();
}

Var Solver::newVar(const bool dvar)
{
    //For adding the variable to all sorts of places, we need to increment
    //the sizes of the vectors/heaps
    if (nVars() != nVarsReal())
        unSaveVarMem();

    const Var var = decisionVar.size();

    if (conf.doStamp
        && nVars() > 15ULL*1000ULL*1000ULL
    ) {
        conf.doStamp = false;
        stamp.freeMem();
        if (conf.verbosity >= 2) {
            cout
            << "c Switching off stamping due to excessive number of variables"
            << " (it would take too much memory)"
            << endl;
        }
    }

    if (conf.doCache && nVars() > 8ULL*1000ULL*1000ULL) {
        conf.doCache = false;
        implCache.free();

        if (conf.verbosity >= 2) {
            cout
            << "c Switching off caching due to excessive number of variables"
            << " (it would take too much memory)"
            << endl;
        }
    }

    if (conf.doSimplify
        && conf.doFindXors
        && nVars() > 1ULL*1000ULL*1000ULL
    ) {
        conf.doFindXors = false;
        simplifier->freeXorMem();

        if (conf.verbosity >= 2) {
            cout
            << "c Switching off XOR finding due to excessive number of variables"
            << " (it would take too much memory)"
            << endl;
        }
    }

    if (conf.doStamp) {
        stamp.newVar();
    }

    outerToInterMain.push_back(var);
    interToOuterMain.push_back(var);
    decisionVar.push_back(dvar);
    numDecisionVars += dvar;

    if (conf.doCache) {
        implCache.addNew();
        litReachable.push_back(LitReachData());
        litReachable.push_back(LitReachData());
    }

    backupActivity.push_back(0);
    backupPolarity.push_back(false);

    Searcher::newVar(dvar);

    varReplacer->newVar();
    if (conf.doSimplify) {
        simplifier->newVar();
    }

    if (conf.doCompHandler) {
        compHandler->newVar();
    }

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
@brief Removes redundant clauses that have been found not to be too good
*/
CleaningStats Solver::reduceDB()
{
    //Clean the clause database before doing cleaning
    //varReplacer->performReplace();
    clauseCleaner->removeAndCleanAll();

    const double myTime = cpuTime();
    solveStats.nbReduceDB++;
    CleaningStats tmpStats;
    tmpStats.origNumClauses = longRedCls.size();
    tmpStats.origNumLits = binTri.redLits;

    //Calculate how many to remove
    uint64_t origRemoveNum = (double)longRedCls.size() *conf.ratioRemoveClauses;

    //If there is a ratio limit, and we are over it
    //then increase the removeNum accordingly
    uint64_t maxToHave = (double)(longIrredCls.size() + binTri.irredTris + nVars() + 300ULL)
        * (double)solveStats.nbReduceDB
        * conf.maxNumRedsRatio;
    uint64_t removeNum = std::max<long long>(origRemoveNum, (long)longRedCls.size()-(long)maxToHave);

    if (removeNum != origRemoveNum) {
        if (conf.verbosity >= 2) {
            cout
            << "c [DBclean] Hard upper limit reached, removing more than normal: "
            << origRemoveNum << " --> " << removeNum
            << endl;
        }
    } else {
        if (conf.verbosity >= 2) {
        cout
        << "c [DBclean] Hard limit would be: " << maxToHave
        << endl;
        }
    }

    //Subsume
    uint64_t sumConfl = sumConflicts();
    //simplifier->subsumeReds();
    if (conf.verbosity >= 3) {
        cout
        << "c Time wasted on clean&replace&sub: "
        << std::setprecision(3) << cpuTime()-myTime
        << endl;
    }

    //Complete detach&reattach of OK clauses will be *much* faster
    CompleteDetachReatacher detachReattach(this);
    detachReattach.detachNonBinsNonTris();

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
                #ifdef DRUP
                if (drup) {
                    (*drup)
                    << "d "
                    << *cl
                    << " 0\n";
                }
                #endif
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
    cout << "Cleaning redundant clauses. Red clauses after sort: " << endl;
    for (uint32_t i = 0; i != longRedCls.size(); i++) {
        const Clause* cl = clAllocator->getPointer(longRedCls[i]);
        cout << *cl << endl;
    }
    #endif

    //Remove clauses
    size_t i, j;
    for (i = j = 0
        ; i < longRedCls.size() && tmpStats.removed.num < removeNum
        ; i++
    ) {
        ClOffset offset = longRedCls[i];
        Clause* cl = clAllocator->getPointer(offset);
        assert(cl->size() > 3);

        //Don't delete if not aged long enough
        if (cl->stats.conflictNumIntroduced + 1000
             >= Searcher::sumConflicts()
        ) {
            longRedCls[j++] = offset;
            tmpStats.remain.incorporate(cl);
            tmpStats.remain.age += sumConfl - cl->stats.conflictNumIntroduced;
            continue;
        }

        //Stats Update
        tmpStats.removed.incorporate(cl);
        tmpStats.removed.age += sumConfl - cl->stats.conflictNumIntroduced;

        //free clause
        #ifdef DRUP
        if (drup) {
            (*drup)
            << "d "
            << *cl
            << " 0\n";
        }
        #endif
        clAllocator->clauseFree(offset);
    }

    //Count what is left
    for (; i < longRedCls.size(); i++) {
        ClOffset offset = longRedCls[i];
        Clause* cl = clAllocator->getPointer(offset);

        /*
        //No use at all? Remove!
        if (cl->stats.numPropAndConfl() == 0
            && cl->stats.conflictNumIntroduced + 20000
                < sumStats.conflStats.numConflicts
        ) {
            //Stats Update
            tmpStats.removed.incorporate(cl);
            tmpStats.removed.age += sumConfl - cl->stats.conflictNumIntroduced;

            //free clause
            clAllocator->clauseFree(offset);
            continue;
        }*/

        //Stats Update
        tmpStats.remain.incorporate(cl);
        tmpStats.remain.age += sumConfl - cl->stats.conflictNumIntroduced;

        if (cl->stats.conflictNumIntroduced > sumConfl) {
            cout
            << "c DEBUG: conflict introduction numbers are wrong."
            << " according to CL, introduction: " << cl->stats.conflictNumIntroduced
            << " but we think max confl: "  << sumConfl
            << endl;
        }
        assert(cl->stats.conflictNumIntroduced <= sumConfl);

        longRedCls[j++] = offset;
    }

    //Resize long redundant clause array
    longRedCls.resize(longRedCls.size() - (i - j));

    //Reattach what's left
    detachReattach.reattachLongs();

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
    release_assert(!(conf.doLHBR && !conf.propBinFirst)
        && "You must NOT set both LHBR and any-order propagation. LHBR needs binary clauses propagated first."
    );

    release_assert(conf.shortTermHistorySize > 0
        && "You MUST give a short term history size (\"--gluehist\")  greater than 0!"
    );

    if (conf.verbosity >= 6) {
        cout
        << "c Solver::solve() called"
        << endl;
    }

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
    lbool status = l_Undef;
    if (!ok) {
        status = l_False;
        if (conf.verbosity >= 6) {
            cout
            << "c Solver status l_Fase on startup of solve()"
            << endl;
        }
    }

    //If still unknown, simplify
    if (status == l_Undef
        && nVars() > 0
        && conf.doPreSchedSimpProblem
        && conf.doSchedSimpProblem
    ) {
        status = simplifyProblem();
    }

    //Iterate until solved
    while (status == l_Undef
        && !needToInterrupt
        && cpuTime() < conf.maxTime
        && sumStats.conflStats.numConflicts < conf.maxConfl
    ) {
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

        //Abide by maxConfl limit
        numConfls = std::min<uint32_t>(numConfls, conf.maxConfl - sumStats.conflStats.numConflicts);

        //Solve and update stats
        status = Searcher::solve(assumptions, numConfls);

        //If stats indicate that recursive minimization is not helping
        //turn it off
        if (status == l_Undef
            && conf.doRecursiveMinim
        ) {
            const Searcher::Stats& stats = Searcher::getStats();
            double remPercent =
                (double)stats.recMinLitRem/(double)stats.litsRedNonMin*100.0;

            double costPerGained = (double)stats.recMinimCost/remPercent;
            if (costPerGained > 200ULL*1000ULL*1000ULL) {
                conf.doRecursiveMinim = false;
                if (conf.verbosity >= 2) {
                    cout
                    << "c recursive minimization too costly: "
                    << std::fixed << std::setprecision(0) << (costPerGained/1000.0)
                    << "Kcost/(% lits removed) --> disabling"
                    << endl;
                }
            } else {
                if (conf.verbosity >= 2) {
                    cout
                    << "c recursive minimization cost OK: "
                    << std::fixed << std::setprecision(0) << (costPerGained/1000.0)
                    << "Kcost/(% lits removed)"
                    << endl;
                }
            }
        }

        //If more minimization isn't helping much, disable
        if (status == l_Undef
            && conf.doMinimRedMore
        ) {
            const Searcher::Stats& stats = Searcher::getStats();
            double remPercent =
                (double)(stats.moreMinimLitsStart-stats.moreMinimLitsEnd)/
                    (double)(stats.moreMinimLitsStart)*100.0;

            if (remPercent < 1.0) {
                conf.doMinimRedMore = false;
                if (conf.verbosity >= 2) {
                    cout
                    << "c more minimization effectiveness low: "
                    << std::fixed << std::setprecision(2) << remPercent
                    << " % lits removed --> disabling"
                    << endl;
                }
            } else if (remPercent > 7.0) {
                conf.moreMinimLimit = 800;
                if (conf.verbosity >= 2) {
                    cout
                    << "c more minimization effectiveness good: "
                    << std::fixed << std::setprecision(2) << remPercent
                    << " % --> increasing limit to " << conf.moreMinimLimit
                    << endl;
                }
            } else {
                conf.moreMinimLimit = 300;
                if (conf.verbosity >= 2) {
                    cout
                    << "c more minimization effectiveness OK: "
                    << std::fixed << std::setprecision(2) << remPercent
                    << " % --> setting limit to norm " << conf.moreMinimLimit
                    << endl;
                }
                conf.moreMinimLimit = 300;
            }
        }

        sumStats += Searcher::getStats();
        sumPropStats += propStats;
        propStats.clear();

        //Solution has been found
        if (status != l_Undef) {
            break;
        }

        //If we are over the limit, exit
        if (sumStats.conflStats.numConflicts >= conf.maxConfl
            || cpuTime() > conf.maxTime
        ) {
            status = l_Undef;
            break;
        }

        //Back up activities, polairties and var_inc
        backupActivity.clear();
        backupPolarity.clear();
        backupActivity.resize(nVarsReal(), 0);
        backupPolarity.resize(nVarsReal(), false);
        for (size_t i = 0; i < nVars(); i++) {
            backupPolarity[i] = varData[i].polarity;
            backupActivity[i] = Searcher::getSavedActivity(i);
        }
        backupActivityInc = Searcher::getVarInc();

        if (status != l_False) {
            Searcher::resetStats();
            fullReduce();
        }

        zeroLevAssignsByThreads += trail.size() - origTrailSize;

        //Simplify
        if (conf.doSchedSimpProblem) {
            status = simplifyProblem();
        }
    }

    //Handle found solution
    if (status == l_True) {
        //If literal stats are wrong, the solution is probably wrong
        checkStats();

        //Extend solution to stored solution in component handler
        if (conf.doCompHandler) {
            compHandler->addSavedState(solution);
        }

        //Extend solution
        if (conf.doSimplify
            || conf.doFindAndReplaceEqLits
        ) {
            SolutionExtender extender(this, solution);
            extender.extend();
        } else {
            model = solution;
        }
        cancelUntil(0);

        //Renumber model back to original variable numbering
        updateArrayRev(model, interToOuterMain);
    } else {
        //Back-number the conflict
        updateLitsMap(conflict, interToOuterMain);
    }
    checkDecisionVarCorrectness();
    checkImplicitStats();

    return status;
}

void Solver::checkDecisionVarCorrectness() const
{
    //Check for var deicisonness
    for(size_t var = 0; var < nVarsReal(); var++) {
        if (varData[var].removed != Removed::none
            && varData[var].removed != Removed::queued_replacer
        ) {
            assert(!decisionVar[var]);
        }
    }
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
    #ifdef DEBUG_IMPLICIT_STATS
    checkStats();
    #endif
    reArrangeClauses();

    if (conf.verbosity >= 6) {
        cout
        << "c Solver::simplifyProblem() called"
        << endl;
    }

    if (conf.doFindComps
        && getNumFreeVars() < conf.compVarLimit
    ) {
        CompFinder findParts(this);
        if (!findParts.findComps()) {
            goto end;
        }
    }

    if (conf.doCompHandler
        && getNumFreeVars() < conf.compVarLimit
        && solveStats.numSimplify >= conf.handlerFromSimpNum
        //Only every 2nd, since it can be costly to find parts
        && solveStats.numSimplify % 2 == 0
    ) {
        if (!compHandler->handle())
            goto end;
    }

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
        if (!implCache.clean(this))
            goto end;

        if (!implCache.tryBoth(this))
            goto end;
    }

    //Treat implicits
    if (conf.doStrSubImplicit) {
        clauseVivifier->subsumeImplicit();
    }

    //PROBE
    updateDominators();
    if (conf.doProbe && !prober->probe()) {
        goto end;
    }

    //If we are over the limit, exit
    if (sumStats.conflStats.numConflicts >= conf.maxConfl
        || cpuTime() > conf.maxTime
    ) {
        return l_Undef;
    }

    //Don't replace first -- the stamps won't work so well
    if (conf.doClausVivif && !clauseVivifier->vivify(true)) {
        goto end;
    }

    //Treat implicits
    if (conf.doStrSubImplicit) {
        clauseVivifier->subsumeImplicit();
    }

    //SCC&VAR-REPL
    if (conf.doFindAndReplaceEqLits) {
        if (!sCCFinder->find2LongXors())
            goto end;

        if (!varReplacer->performReplace())
            goto end;
    }

    //Check if time is up
    if (needToInterrupt)
        return l_Undef;

    //Var-elim, gates, subsumption, strengthening
    if (conf.doSimplify && !simplifier->simplify())
        goto end;

    //Treat implicits
    if (conf.doStrSubImplicit) {
        if (!clauseVivifier->strengthenImplicit()) {
            goto end;
        }

        clauseVivifier->subsumeImplicit();
    }

    //Clean cache before vivif
    if (conf.doCache && !implCache.clean(this))
        goto end;

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

    //Re-calculate reachability after re-numbering and new cache data
    if (conf.doCache) {
        calcReachability();
    }

    //Delete and disable cache if too large
    if (conf.doCache) {
        const size_t memUsedMB = implCache.memUsed()/(1024UL*1024UL);
        if (memUsedMB > conf.maxCacheSizeMB) {
            if (conf.verbosity >= 2) {
                cout
                << "c Turning off cache, memory used, "
                << memUsedMB/(1024UL*1024UL) << " MB"
                << " is over limit of " << conf.maxCacheSizeMB  << " MB"
                << endl;
            }
            implCache.free();
            vector<LitReachData> tmp;
            litReachable.swap(tmp);
            conf.doCache = false;
        }
    }

    if (conf.doRenumberVars) {
        //Clean cache before renumber -- very important, otherwise
        //we will be left with lits inside the cache that are out-of-bounds
        if (conf.doCache) {
            bool setSomething = true;
            while(setSomething) {
                if (!implCache.clean(this, &setSomething))
                    goto end;
            }
        }

        renumberVariables();
    }

    //Free unused watch memory
    freeUnusedWatches();

    reArrangeClauses();

    //addSymmBreakClauses();

end:
    if (conf.verbosity >= 3)
        cout << "c Searcher::simplifyProblem() finished" << endl;

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

ClauseUsageStats Solver::sumClauseData(
    const vector<ClOffset>& toprint
    , const bool red
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

        //If redundant, sum up GLUE-based stats
        if (red) {
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
            if (cl.red()) {
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
        if (red) {
            cout << "c red  ";
        } else {
            cout << "c irred";
        }
        cout
        #ifdef STATS_NEEDED
        << " lits visit: "
        << std::setw(8) << stats.sumLitVisited/1000UL
        << "K"

        << " cls visit: "
        << std::setw(7) << stats.sumLookedAt/1000UL
        << "K"
        #endif

        << " prop: "
        << std::setw(5) << stats.sumProp/1000UL
        << "K"

        << " conf: "
        << std::setw(5) << stats.sumConfl/1000UL
        << "K"

        << " UIP used: "
        << std::setw(5) << stats.sumUsedUIP/1000UL
        << "K"
        << endl;
    }

    //Print more stats
    if (conf.verbosity >= 4) {
        printPropConflStats("clause-len", perSizeStats);

        if (red) {
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

void Solver::printStats() const
{
    const double cpu_time = cpuTime();
    cout << "c ------- FINAL TOTAL SOLVING STATS ---------" << endl;
    printStatsLine("c UIP search time"
        , sumStats.cpu_time
        , sumStats.cpu_time/cpu_time*100.0
        , "% time"
    );

    if (conf.verbStats >= 1) {
        printFullStats();
    } else {
        printMinStats();
    }
}

void Solver::printMinStats() const
{
    const double cpu_time = cpuTime();
    sumStats.printShort();
    printStatsLine("c props/decision"
        , (double)propStats.propagations/(double)sumStats.decisions
    );
    printStatsLine("c props/conflict"
        , (double)propStats.propagations/(double)sumStats.conflStats.numConflicts
    );

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
    if (conf.doProbe) {
        printStatsLine("c probing time"
            , prober->getStats().cpu_time
            , prober->getStats().cpu_time/cpu_time*100.0
            , "% time"
        );

        prober->getStats().printShort();
    }
    //Simplifier stats
    if (conf.doSimplify) {
        printStatsLine("c Simplifier time"
            , simplifier->getStats().totalTime()
            , simplifier->getStats().totalTime()/cpu_time*100.0
            , "% time"
        );
        simplifier->getStats().printShort(nVars());
    }
    printStatsLine("c SCC time"
        , sCCFinder->getStats().cpu_time
        , sCCFinder->getStats().cpu_time/cpu_time*100.0
        , "% time"
    );
    sCCFinder->getStats().printShort();
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
    //varReplacer->getStats().printShort(nVars());
    printStatsLine("c asymm time"
                    , clauseVivifier->getStats().timeNorm
                    , clauseVivifier->getStats().timeNorm/cpu_time*100.0
                    , "% time"
    );
    printStatsLine("c vivif cache-irred time"
                    , clauseVivifier->getStats().irredCacheBased.cpu_time
                    , clauseVivifier->getStats().irredCacheBased.cpu_time/cpu_time*100.0
                    , "% time"
    );
    printStatsLine("c vivif cache-red time"
                    , clauseVivifier->getStats().redCacheBased.cpu_time
                    , clauseVivifier->getStats().redCacheBased.cpu_time/cpu_time*100.0
                    , "% time"
    );
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
    if (conf.doCache) {
        implCache.printStatsSort(this);
    }
}

void Solver::printFullStats() const
{
    const double cpu_time = cpuTime();

    sumStats.print();
    sumPropStats.print(sumStats.cpu_time);
    printStatsLine("c props/decision"
        , (double)propStats.propagations/(double)sumStats.decisions
    );
    printStatsLine("c props/conflict"
        , (double)propStats.propagations/(double)sumStats.conflStats.numConflicts
    );
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
    if (conf.doProbe) {
        printStatsLine("c probing time"
            , prober->getStats().cpu_time
            , prober->getStats().cpu_time/cpu_time*100.0
            , "% time"
        );

        prober->getStats().print(nVars());
    }

    //Simplifier stats
    if (conf.doSimplify) {
        printStatsLine("c Simplifier time"
            , simplifier->getStats().totalTime()
            , simplifier->getStats().totalTime()/cpu_time*100.0
            , "% time"
        );

        simplifier->getStats().print(nVars());
    }

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

    if (conf.doCache) {
        implCache.printStats(this);
    }

    //Other stats
    printStatsLine("c Conflicts in UIP"
        , sumStats.conflStats.numConflicts
        , (double)sumStats.conflStats.numConflicts/cpu_time
        , "confl/TOTAL_TIME_SEC"
    );
    printStatsLine("c Total time", cpu_time);
    printMemStats();
}

uint64_t Solver::printWatchMemUsed(const uint64_t totalMem) const
{
    size_t mem = 0;
    mem += watches.capacity()*sizeof(vec<Watched>);
    for(size_t i = 0; i < watches.size(); i++) {
        mem += watches[i].capacity()*sizeof(Watched);
    }
    printStatsLine("c Mem for watches"
        , mem/(1024UL*1024UL)
        , "MB"
        , (double)mem/(double)totalMem*100.0
        , "%"
    );

    return mem;
}

void Solver::printMemStats() const
{
    const uint64_t totalMem = memUsed();
    printStatsLine("c Mem used"
        , totalMem/(1024UL*1024UL)
        , "MB"
    );
    uint64_t account = 0;

    size_t mem = 0;
    mem += clAllocator->getMemUsed();
    mem += longIrredCls.capacity()*sizeof(ClOffset);
    mem += longRedCls.capacity()*sizeof(ClOffset);
    printStatsLine("c Mem for longclauses"
        , mem/(1024UL*1024UL)
        , "MB"
        , (double)mem/(double)totalMem*100.0
        , "%"
    );
    account += mem;

    account += printWatchMemUsed(totalMem);

    mem = 0;
    mem += assigns.capacity()*sizeof(lbool);
    mem += varData.capacity()*sizeof(VarData);
    #ifdef STATS_NEEDED
    mem += varDataLT.capacity()*sizeof(VarData::Stats);
    #endif
    mem += backupActivity.capacity()*sizeof(uint32_t);
    mem += backupPolarity.capacity()*sizeof(bool);
    mem += decisionVar.capacity()*sizeof(char);
    mem += assumptions.capacity()*sizeof(Lit);
    printStatsLine("c Mem for vars"
        , mem/(1024UL*1024UL)
        , "MB"
        , (double)mem/(double)totalMem*100.0
        , "%"

    );
    account += mem;

    mem = 0;
    mem += toPropNorm.capacity()*sizeof(Lit);
    mem += toPropBin.capacity()*sizeof(Lit);
    mem += toPropRedBin.capacity()*sizeof(Lit);
    mem += stamp.getMemUsed();
    printStatsLine("c Mem for stamps"
        , mem/(1024UL*1024UL)
        , "MB"
        , (double)mem/(double)totalMem*100.0
        , "%"
    );
    account += mem;

    mem = implCache.memUsed();
    mem += litReachable.capacity()*sizeof(LitReachData);
    printStatsLine("c Mem for impl cache"
        , mem/(1024UL*1024UL)
        , "MB"
        , (double)mem/(double)totalMem*100.0
        , "%"
    );
    account += mem;

    mem = hist.getMemUsed();
    printStatsLine("c Mem for history stats"
        , mem/(1024UL*1024UL)
        , "MB"
        , (double)mem/(double)totalMem*100.0
        , "%"
    );
    account += mem;

    mem = memUsedSearch();
    mem += model.capacity()*sizeof(lbool);
    if (conf.verbosity >= 3) {
        cout << "model bytes: "
        << model.capacity()*sizeof(lbool)
        << endl;
    }
    printStatsLine("c Mem for search"
        , mem/(1024UL*1024UL)
        , "MB"
        , (double)mem/(double)totalMem*100.0
        , "%"
    );
    account += mem;

    mem = 0;
    mem += seen.capacity()*sizeof(uint16_t);
    mem += seen2.capacity()*sizeof(uint16_t);
    mem += toClear.capacity()*sizeof(Lit);
    mem += analyze_stack.capacity()*sizeof(Lit);
    printStatsLine("c Mem for temporaries"
        , mem/(1024UL*1024UL)
        , "MB"
        , (double)mem/(double)totalMem*100.0
        , "%"
    );
    account += mem;

    mem = 0;
    mem += interToOuterMain.capacity()*sizeof(Var);
    mem += interToOuter.capacity()*sizeof(Var);
    mem += interToOuter2.capacity()*sizeof(Var);
    mem += outerToInter.capacity()*sizeof(Var);
    mem += outerToInterMain.capacity()*sizeof(Var);
    printStatsLine("c Mem for renumberer"
        , mem/(1024UL*1024UL)
        , "MB"
        , (double)mem/(double)totalMem*100.0
        , "%"
    );
    account += mem;

    if (conf.doSimplify) {
        mem = simplifier->memUsed();
        printStatsLine("c Mem for simplifier"
            , mem/(1024UL*1024UL)
            , "MB"
            , (double)mem/(double)totalMem*100.0
            , "%"
        );
        account += mem;

        mem = simplifier->memUsedXor();
        printStatsLine("c Mem for xor-finder"
            , mem/(1024UL*1024UL)
            , "MB"
            , (double)mem/(double)totalMem*100.0
            , "%"
        );
        account += mem;
    }

    mem = varReplacer->bytesMemUsed();
    printStatsLine("c Mem for varReplacer"
        , mem/(1024UL*1024UL)
        , "MB"
        , (double)mem/(double)totalMem*100.0
        , "%"
    );
    account += mem;

    mem = sCCFinder->memUsed();
    printStatsLine("c Mem for SCC"
        , mem/(1024UL*1024UL)
        , "MB"
        , (double)mem/(double)totalMem*100.0
        , "%"
    );
    account += mem;

    if (conf.doProbe) {
        mem = prober->memUsed();
        printStatsLine("c Mem for prober"
            , mem/(1024UL*1024UL)
            , "MB"
            , (double)mem/(double)totalMem*100.0
            , "%"
        );
        account += mem;
    }

    printStatsLine("c Accounted for mem"
        , (double)account/(double)totalMem*100.0
        , "%"
    );
}

void Solver::dumpBinClauses(
    const bool dumpRed
    , const bool dumpNonRed
    , std::ostream* outfile
) const {
    //Go trough each watchlist
    size_t wsLit = 0;
    for (vector<vec<Watched> >::const_iterator
        it = watches.begin(), end = watches.end()
        ; it != end
        ; it++, wsLit++
    ) {
        Lit lit = Lit::toLit(wsLit);
        const vec<Watched>& ws = *it;

        //Each element in the watchlist
        for (vec<Watched>::const_iterator
            it2 = ws.begin(), end2 = ws.end()
            ; it2 != end2
            ; it2++
        ) {
            //Only dump binaries
            if (it2->isBinary() && lit < it2->lit2()) {
                bool toDump = false;
                if (it2->red() && dumpRed) toDump = true;
                if (!it2->red() && dumpNonRed) toDump = true;

                if (toDump) {
                    tmpCl.clear();
                    tmpCl.push_back(getUpdatedLit(it2->lit2(), interToOuterMain));
                    tmpCl.push_back(getUpdatedLit(lit, interToOuterMain));
                    std::sort(tmpCl.begin(), tmpCl.end());

                    *outfile
                    << tmpCl[0] << " "
                    << tmpCl[1]
                    << " 0\n";
                }
            }
        }
    }
}

void Solver::dumpTriClauses(
    const bool alsoRed
    , const bool alsoNonRed
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
            if (it2->isTri() && lit < it2->lit2()) {
                bool toDump = false;
                if (it2->red() && alsoRed) toDump = true;
                if (!it2->red() && alsoNonRed) toDump = true;

                if (toDump) {
                    tmpCl.clear();
                    tmpCl.push_back(getUpdatedLit(it2->lit2(), interToOuterMain));
                    tmpCl.push_back(getUpdatedLit(it2->lit3(), interToOuterMain));
                    tmpCl.push_back(getUpdatedLit(lit, interToOuterMain));
                    std::sort(tmpCl.begin(), tmpCl.end());

                    *outfile
                    << tmpCl[0] << " "
                    << tmpCl[1] << " "
                    << tmpCl[2]
                    << " 0\n";
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

    cout
    << "c"
    << " size4: " << size4
    << " size5: " << size5
    << " larger: " << sizeLarge << endl;
}

void Solver::dumpEquivalentLits(std::ostream* os) const
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
        << tmpCl[0] << " "
        << tmpCl[1]
        << " 0\n";

        tmpCl[0] ^= true;
        tmpCl[1] ^= true;

        *os
        << tmpCl[0] << " "
        << tmpCl[1]
        << " 0\n";
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
        << " 0\n";
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
    << "c redundant binary clauses (extracted from watchlists)" << endl
    << "c ---------------------------------" << endl;
    if (maxSize >= 2) {
        dumpBinClauses(true, false, os);
    }

    *os
    << "c " << endl
    << "c ---------------------------------" << endl
    << "c redundant tertiary clauses (extracted from watchlists)" << endl
    << "c ---------------------------------" << endl;
    if (maxSize >= 2) {
        dumpTriClauses(true, false, os);
    }

    if (maxSize >= 2) {
        dumpEquivalentLits(os);
    }

    *os
    << "c " << endl
    << "c --------------------" << endl
    << "c redundant long clauses" << endl
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
            << "c clause redundant "
            << (cl->red() ? "yes" : "no")
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
    if (varReplacer) {
        const vector<Lit>& table = varReplacer->getReplaceTable();
        for (Var var = 0; var != table.size(); var++) {
            Lit lit = table[var];
            if (lit.var() == var)
                continue;
            numClauses += 2;
        }
    }

    //Normal clauses
    numClauses += binTri.irredBins;
    numClauses += binTri.irredTris;
    numClauses += longIrredCls.size();
    if (conf.doCompHandler) {
        compHandler->getRemovedClauses().sizes.size();
    }

    //previously eliminated clauses
    if (conf.doSimplify) {
        const vector<BlockedClause>& blockedClauses = simplifier->getBlockedClauses();
        numClauses += blockedClauses.size();
    }

    *os << "p cnf " << nVars() << " " << numClauses << endl;

    ////////////////////////////////////////////////////////////////////

    dumpUnitaryClauses(os);

    dumpEquivalentLits(os);

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
        assert(!cl->red());
        *os << clauseBackNumbered(*cl) << " 0" << endl;
    }

    if (conf.doSimplify) {
        const vector<BlockedClause>& blockedClauses
            = simplifier->getBlockedClauses();

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

    if (conf.doCompHandler) {
        *os
        << "c " << endl
        << "c ---------------" << endl
        << "c clauses in components" << endl
        << "c ---------------" << endl;

        const CompHandler::RemovedClauses& removedClauses = compHandler->getRemovedClauses();

        vector<Lit> tmp;
        size_t at = 0;
        for (uint32_t size :removedClauses.sizes) {
            tmp.clear();
            for(size_t i = at; i < at + size; i++) {
                tmp.push_back(removedClauses.lits[i]);
            }
            std::sort(tmp.begin(), tmp.end());
            *os << tmp << " 0" << endl;

            //Move 'at' along
            at += size;
        }
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
                cout << "Binary clause part: " << lit << " , " << it2->lit2() << endl;
            } else if (it2->isClause()) {
                cout << "Normal clause offs " << it2->getOffset() << endl;
            } else if (it2->isTri()) {
                cout << "Tri clause:"
                << lit << " , "
                << it2->lit2() << " , "
                << it2->lit3() << endl;
            }
        }
    }
}

bool Solver::verifyImplicitClauses() const
{
    uint32_t wsLit = 0;
    for (vector<vec<Watched> >::const_iterator
        it = watches.begin(), end = watches.end()
        ; it != end
        ; it++, wsLit++
    ) {
        Lit lit = Lit::toLit(wsLit);
        const vec<Watched>& ws = *it;

        for (Watched w: ws) {
            if (w.isBinary()
                && modelValue(lit) != l_True
                && modelValue(w.lit2()) != l_True
            ) {
                cout
                << "bin clause: "
                << lit << " , " << w.lit2()
                << " not satisfied!"
                << endl;

                cout
                << "value of unsat bin clause: "
                << value(lit) << " , " << value(w.lit2())
                << endl;

                return false;
            }

             if (w.isTri()
                && modelValue(lit) != l_True
                && modelValue(w.lit2()) != l_True
                && modelValue(w.lit3()) != l_True
            ) {
                cout
                << "tri clause: "
                << lit
                << " , " << w.lit2()
                << " , " << w.lit3()
                << " not satisfied!"
                << endl;

                cout
                << "value of unsat tri clause: "
                << value(lit)
                << " , " << value(w.lit2())
                << " , " << value(w.lit3())
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
    verificationOK &= verifyImplicitClauses();

    if (conf.verbosity >= 1 && verificationOK) {
        cout
        << "c Verified "
        << longIrredCls.size() + longRedCls.size()
            + binTri.irredBins + binTri.redBins
            + binTri.irredTris + binTri.redTris
        << " clause(s)."
        << endl;
    }

    return verificationOK;
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

    #ifndef MORE_DEBUG
    return;
    #endif

    for (size_t i = 0; i < watches.size(); i++) {
        const Lit lit = Lit::toLit(i);
        for (uint32_t i2 = 0; i2 < watches[i].size(); i2++) {
            const Watched& w = watches[i][i2];
            if (!w.isClause())
                continue;

            //Get clause
            Clause* cl = clAllocator->getPointer(w.getOffset());
            assert(!cl->getFreed());

            //Assert watch correctness
            if ((*cl)[0] != lit
                && (*cl)[1] != lit
            ) {
                cout
                << "ERROR! Clause " << (*cl)
                << " not attached?"
                << endl;

                assert(false);
                exit(-1);
            }

            //Clause in one of the lists
            if (!findClause(w.getOffset())) {
                cout
                << "ERROR! did not find clause " << *cl
                << endl;

                assert(false);
                exit(1);
            }
        }
    }

    findAllAttach(longIrredCls);
    findAllAttach(longRedCls);
}

void Solver::findAllAttach(const vector<ClOffset>& cs) const
{
    for(vector<ClOffset>::const_iterator
        it = cs.begin(), end = cs.end()
        ; it != end
        ; it++
    ) {
        Clause& cl = *clAllocator->getPointer(*it);
        bool ret = findWCl(watches[cl[0].toInt()], *it);
        if (!ret) {
            cout
            << "Clause " << cl
            << " (red: " << cl.red() << ")"
            << " doesn't have its 1st watch attached!"
            << endl;

            assert(false);
            exit(-1);
        }

        ret = findWCl(watches[cl[1].toInt()], *it);
        if (!ret) {
            cout
            << "Clause " << cl
            << " (red: " << cl.red() << ")"
            << " doesn't have its 2nd watch attached!"
            << endl;

            assert(false);
            exit(-1);
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
            if (i > 0)
                assert(cl[i-1].var() != cl[i].var());
        }
    }
}

size_t Solver::getNumFreeVars() const
{
    assert(decisionLevel() == 0);
    uint32_t freeVars = nVarsReal();
    freeVars -= trail.size();
    if (conf.doSimplify) {
        freeVars -= simplifier->getStats().numVarsElimed;
    }
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
    //Don't check if in crazy mode
    #ifdef NDEBUG
    return;
    #endif

    //Check number of red & irred binary clauses
    uint64_t thisNumRedBins = 0;
    uint64_t thisNumNonRedBins = 0;
    uint64_t thisNumRedTris = 0;
    uint64_t thisNumNonRedTris = 0;

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
                if (it2->red())
                    thisNumRedBins++;
                else
                    thisNumNonRedBins++;

                continue;
            }

            if (it2->isTri()) {
                assert(it2->lit2() < it2->lit3());
                assert(it2->lit2().var() != it2->lit3().var());

                #ifdef DEBUG_TRI_SORTED_SANITY
                Lit lits[3];
                lits[0] = lit;
                lits[1] = it2->lit2();
                lits[2] = it2->lit3();
                std::sort(lits, lits + 3);
                findWatchedOfTri(watches, lits[0], lits[1], lits[2], it2->red());
                findWatchedOfTri(watches, lits[1], lits[0], lits[2], it2->red());
                findWatchedOfTri(watches, lits[2], lits[0], lits[1], it2->red());
                #endif //DEBUG_TRI_SORTED_SANITY

                if (it2->red())
                    thisNumRedTris++;
                else
                    thisNumNonRedTris++;

                continue;
            }
        }
    }

    if (thisNumNonRedBins/2 != binTri.irredBins) {
        cout
        << "ERROR:"
        << " thisNumNonRedBins/2: " << thisNumNonRedBins/2
        << " binTri.irredBins: " << binTri.irredBins
        << "thisNumNonRedBins: " << thisNumNonRedBins
        << "thisNumRedBins: " << thisNumRedBins << endl;
    }
    assert(thisNumNonRedBins % 2 == 0);
    assert(thisNumNonRedBins/2 == binTri.irredBins);

    if (thisNumRedBins/2 != binTri.redBins) {
        cout
        << "ERROR:"
        << " thisNumRedBins/2: " << thisNumRedBins/2
        << " binTri.redBins: " << binTri.redBins
        << endl;
    }
    assert(thisNumRedBins % 2 == 0);
    assert(thisNumRedBins/2 == binTri.redBins);

    if (thisNumNonRedTris/3 != binTri.irredTris) {
        cout
        << "ERROR:"
        << " thisNumNonRedTris/3: " << thisNumNonRedTris/3
        << " binTri.irredTris: " << binTri.irredTris
        << endl;
    }
    assert(thisNumNonRedTris % 3 == 0);
    assert(thisNumNonRedTris/3 == binTri.irredTris);

    if (thisNumRedTris/3 != binTri.redTris) {
        cout
        << "ERROR:"
        << " thisNumRedTris/3: " << thisNumRedTris/3
        << " binTri.redTris: " << binTri.redTris
        << endl;
    }
    assert(thisNumRedTris % 3 == 0);
    assert(thisNumRedTris/3 == binTri.redTris);
}

void Solver::checkStats(const bool allowFreed) const
{
    //If in crazy mode, don't check
    #ifdef NDEBUG
    return;
    #endif

    checkImplicitStats();

    //Count number of irred clauses' literals
    uint64_t numLitsNonRed = 0;
    for(vector<ClOffset>::const_iterator
        it = longIrredCls.begin(), end = longIrredCls.end()
        ; it != end
        ; it++
    ) {
        const Clause& cl = *clAllocator->getPointer(*it);
        if (cl.freed()) {
            assert(allowFreed);
        } else {
            numLitsNonRed += cl.size();
        }
    }

    //Count number of redundant clauses' literals
    uint64_t numLitsRed = 0;
    for(vector<ClOffset>::const_iterator
        it = longRedCls.begin(), end = longRedCls.end()
        ; it != end
        ; it++
    ) {
        const Clause& cl = *clAllocator->getPointer(*it);
        if (cl.freed()) {
            assert(allowFreed);
        } else {
            numLitsRed += cl.size();
        }
    }

    //Check counts
    if (numLitsNonRed != binTri.irredLits) {
        cout << "ERROR: " << endl;
        cout << "->numLitsNonRed: " << numLitsNonRed << endl;
        cout << "->binTri.irredLits: " << binTri.irredLits << endl;
    }
    if (numLitsRed != binTri.redLits) {
        cout << "ERROR: " << endl;
        cout << "->numLitsRed: " << numLitsRed << endl;
        cout << "->binTri.redLits: " << binTri.redLits << endl;
    }
    assert(numLitsNonRed == binTri.irredLits);
    assert(numLitsRed == binTri.redLits);
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
            << "BIN: " << lit << ", " << it->lit2()
            << " (l: " << it->red() << ")";
        }

        if (it->isTri()) {
            cout
            << "TRI: " << lit << ", " << it->lit2() << ", " << it->lit3()
            << " (l: " << it->red() << ")";
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
            const lbool val2 = value(it2->lit2());

            //Handle binary
            if (it2->isBinary()) {
                if (val1 == l_False) {
                    if (val2 != l_True) {
                        cout << "not prop BIN: "
                        << lit << ", " << it2->lit2()
                        << " (red: " << it2->red()
                        << endl;
                    }
                    assert(val2 == l_True);
                }

                if (val2 == l_False)
                    assert(val1 == l_True);
            }

            //Handle 3-long clause
            if (it2->isTri()) {
                const lbool val3 = value(it2->lit3());

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
    if (conf.doSimplify) {
        return simplifier->getStats().numVarsElimed;
    } else {
        return 0;
    }
}

size_t Solver::getNumVarsReplaced() const
{
    return varReplacer->getNumReplacedVars();
}

void Solver::dumpIfNeeded() const
{
    if (conf.redDumpFname.empty()
        && !conf.irredDumpFname.empty()
    ) {
        //Nothing to do, return
        return;
    }

    //Handle UNSAT
    if (!solver->okay()) {
        cout
        << "c Problem is UNSAT, so irred and/or redundant clauses cannot be dumped"
        << endl;

        return;
    }

    //Don't dump implicit clauses multiple times
    if (conf.doStrSubImplicit) {
        solver->clauseVivifier->subsumeImplicit();
    }

    if (!conf.redDumpFname.empty()) {
        std::ofstream outfile;
        outfile.open(conf.redDumpFname.c_str());
        if (!outfile) {
            cout
            << "ERROR: Couldn't open file '"
            << conf.redDumpFname
            << "' for writing, cannot dump redundant clauses!"
            << endl;
        } else {
            solver->dumpRedClauses(&outfile, conf.maxDumpRedsSize);
        }

        cout << "Dumped redundant clauses" << endl;
    }

    if (!conf.irredDumpFname.empty()) {
        if (conf.verbosity >= 1) {
            cout
            << "c Dumping irred irredundant clauses to file '"
            << conf.irredDumpFname << "'"
            << endl;
        }

        std::ofstream outfile;
        outfile.open(conf.irredDumpFname.c_str());
        if (!outfile) {
            cout
            << "Cannot open file '"
            << conf.irredDumpFname
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

        cout << "Dumped irredundant (~irred) clauses" << endl;
    }
}

Lit Solver::updateLitForDomin(Lit lit) const
{
    //Nothing to update
    if (lit == lit_Undef)
        return lit;

    lit = varReplacer->getLitReplacedWith(lit);

    if (varData[lit.var()].removed != Removed::none)
        return lit_Undef;

    return lit;
}

void Solver::updateDominators()
{
    for(vector<Timestamp>::iterator
        it = stamp.tstamp.begin(), end = stamp.tstamp.end()
        ; it != end
        ; it++
    ) {
        for(size_t i = 0; i < 2; i++) {
            Lit newLit = updateLitForDomin(it->dominator[i]);
            it->dominator[i] = newLit;
            if (newLit == lit_Undef)
                it->numDom[i] = 0;
        }
    }
}

void Solver::print_elimed_vars() const
{
    if (conf.doSimplify) {
        simplifier->print_elimed_vars();
    }
}

void Solver::calcReachability()
{
    double myTime = cpuTime();

    //Clear out
    for (size_t i = 0; i < nVars()*2; i++) {
        litReachable[i] = LitReachData();
    }

    //Go through each that is decision variable
    for (Var var = 0; var < nVars(); var++) {

        //Check if it's a good idea to look at the variable as a dominator
        if (value(var) != l_Undef
            || varData[var].removed != Removed::none
            || !decisionVar[var]
        ) {
            continue;
        }

        //Both poliarities
        for (uint32_t sig1 = 0; sig1 < 2; sig1++)  {
            const Lit lit = Lit(var, sig1);

            const vector<LitExtra>& cache = implCache[lit.toInt()].lits;
            uint32_t cacheSize = cache.size();
            for (vector<LitExtra>::const_iterator
                it = cache.begin(), end = cache.end()
                ; it != end
                ; it++
            ) {
                /*if (solver.value(it->var()) != l_Undef
                || solver.subsumer->getVarElimed()[it->var()]
                || solver.xorSubsumer->getVarElimed()[it->var()])
                continue;*/

                assert(it->getLit() != lit);
                assert(it->getLit() != ~lit);

                if (litReachable[it->getLit().toInt()].lit == lit_Undef
                    || litReachable[it->getLit().toInt()].numInCache < cacheSize
                ) {
                    litReachable[it->getLit().toInt()].lit = ~lit;
                    litReachable[it->getLit().toInt()].numInCache = cacheSize;
                }
            }
        }
    }

    if (conf.verbosity >= 1) {
        cout
        << "c calculated reachability. T: "
        << std::setprecision(3) << (cpuTime() - myTime)
        << endl;
    }
}

void Solver::freeUnusedWatches()
{
    size_t wsLit = 0;
    for (vector<vec<Watched> >::iterator
        it = solver->watches.begin(), end = solver->watches.end()
        ; it != end
        ; it++, wsLit++
    ) {
        Lit lit = Lit::toLit(wsLit);
        if (varData[lit.var()].removed == Removed::elimed
            || varData[lit.var()].removed == Removed::replaced
            || varData[lit.var()].removed == Removed::decomposed
        ) {
            assert(it->empty());
            it->clear(true);
        }

        it->fitToSize();
    }
}

bool Solver::enqueueThese(const vector<Lit>& toEnqueue)
{
    assert(ok);
    assert(decisionLevel() == 0);
    for(size_t i = 0; i < toEnqueue.size(); i++) {
        const lbool val = value(toEnqueue[i]);
        if (val == l_Undef) {
            enqueue(toEnqueue[i]);
            ok = propagate().isNULL();
            if (!ok) {
                return false;
            }
        } else if (val == l_False) {
            ok = false;
            return false;
        }
    }

    return true;
}
