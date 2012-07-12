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

#include "Solver.h"
#include "VarReplacer.h"
#include "time_mem.h"
#include "Searcher.h"
#include "SCCFinder.h"
#include "Simplifier.h"
#include "Prober.h"
#include "ClauseVivifier.h"
#include "ClauseCleaner.h"
#include "SolutionExtender.h"
#include "VarUpdateHelper.h"
#include "GateFinder.h"
#include <omp.h>
#include <fstream>
#include <cmath>
#include "XorFinder.h"
#include <fcntl.h>
#include <unistd.h>
using std::cout;
using std::endl;

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
    , numCallReachCalc(0)
    , clausesLits(0)
    , learntsLits(0)
    , numBinsNonLearnt(0)
    , numBinsLearnt(0)
    , numTrisNonLearnt(0)
    , numTrisLearnt(0)
{
    prober = new Prober(this);
    subsumer = new Simplifier(this);
    sCCFinder = new SCCFinder(this);
    clauseVivifier = new ClauseVivifier(this);
    clauseCleaner = new ClauseCleaner(this);
    clAllocator = new ClauseAllocator;
    varReplacer = new VarReplacer(this);

    //Open SQL file for writing
    PropEngine::clAllocator = clAllocator;
    std::string sqlFilename = "data.sql";
    sqlFile.open(sqlFilename.c_str());
    if (!sqlFile) {
        cout
        << "ERROR: Couldn't open file '" << sqlFilename << "' for writing!"
        << endl;

        exit(-1);
    }

    //Generate random ID for SQL
    int randomData = open("/dev/random", O_RDONLY);
    if (randomData == -1) {
        cout << "Error reading from /dev/random !" << endl;
        exit(-1);
    }
    ssize_t ret = read(randomData, &solveStats.runID, sizeof(solveStats.runID));
    if (ret != sizeof(solveStats.runID)) {
        cout << "Couldn't read from /dev/random!" << endl;
        exit(-1);
    }
    close(randomData);
    cout << "c runID: " << solveStats.runID << endl;
}

Solver::~Solver()
{
    delete prober;
    delete subsumer;
    delete sCCFinder;
    delete clauseVivifier;
    delete clauseCleaner;
    delete clAllocator;
    delete varReplacer;
}

bool Solver::addXorClauseInt(const vector< Lit >& lits, bool rhs)
{
    assert(ok);
    assert(qhead == trail.size());
    assert(decisionLevel() == 0);

    if (lits.size() > (0x01UL << 18)) {
        cout << "Too long clause!" << endl;
        exit(-1);
    }

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
            if (!assigns[ps[i].var()].isUndef())
                rhs ^= assigns[ps[i].var()].getBool();
        } else if (assigns[ps[i].var()].isUndef()) { //just add
            ps[j++] = p = ps[i];
            assert(!subsumer->getVarElimed()[p.var()]);
        } else //modify rhs instead of adding
            rhs ^= (assigns[ps[i].var()].getBool());
    }
    ps.resize(ps.size() - (i - j));

    switch(ps.size()) {
        case 0:
            if (rhs)
                ok = false;
            return ok;

        case 1:
            enqueue(Lit(ps[0].var(), !rhs));
            propStats.propsUnit++;
            ok = propagate().isNULL();
            return ok;

        case 2:
            ps[0] ^= !rhs;
            addClauseInt(ps, false);
            if (!ok)
                return false;

            ps[0] ^= true;
            ps[1] ^= true;
            addClauseInt(ps, false);
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
template <class T>
Clause* Solver::addClauseInt(
    const T& lits
    , const bool learnt
    , const ClauseStats& stats
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
            propStats.propsUnit++;
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
                    learntsLits += ps.size();
                else
                    clausesLits += ps.size();
            }

            return c;
    }
}

template Clause* Solver::addClauseInt(
    const Clause& ps
    , const bool learnt
    , const ClauseStats& stats
    , const bool attach
    , vector<Lit>* finalLits
);

template Clause* Solver::addClauseInt(
    const vector<Lit>& ps
    , const bool learnt
    , const ClauseStats& stats
    , const bool attach
    , vector<Lit>* finalLits
);

void Solver::attachClause(const Clause& cl)
{
    //Update stats
    if (cl.learnt())
        learntsLits += cl.size();
    else
        clausesLits += cl.size();

    //Call Solver's function for heavy-lifting
    PropEngine::attachClause(cl);
}

void Solver::attachTriClause(
    const Lit lit1
    , const Lit lit2
    , const Lit lit3
    , const bool learnt
) {
    //Update stats
    if (learnt) {
        learntsLits += 3;
        numTrisLearnt++;
    } else {
        clausesLits += 3;
        numTrisNonLearnt++;
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
        learntsLits += 2;
        numBinsLearnt++;
    } else {
        clausesLits += 2;
        numBinsNonLearnt++;
    }
    numNewBinsSinceSCC++;

    //Call Solver's function for heavy-lifting
    PropEngine::attachBinClause(lit1, lit2, learnt, checkUnassignedFirst);
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
        learntsLits -= origSize;
    else
        clausesLits -= origSize;

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
        if (subsumer->getVarElimed()[ps[i].var()]) {
            //if (!subsumer->unEliminate(ps[i].var(), this) return false
            assert(false);
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
    #ifdef VERBOSE_DEBUG
    cout << "Adding clause " << lits << endl;
    #endif //VERBOSE_DEBUG
    const size_t origTrailSize = trail.size();

    vector<Lit> ps = lits;
    if (!addClauseHelper(ps))
        return false;

    Clause* c = addClauseInt(ps);
    if (c != NULL)
        clauses.push_back(c);

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

    Clause* c = addClauseInt(ps, true, stats);
    if (c != NULL)
        learnts.push_back(c);

    return ok;
}

void Solver::reArrangeClause(Clause* clause)
{
    Clause& c = *clause;
    assert(c.size() > 3);
    if (c.size() == 3) return;

    //Change anything, but find the first two and assign them
    //accordingly at the ClauseData
    const Lit lit1 = c[0];
    const Lit lit2 = c[1];
    assert(lit1 != lit2);

    std::sort(c.begin(), c.end(), PolaritySorter(varData));

    uint8_t foundDatas = 0;
    for (uint16_t i = 0; i < c.size(); i++) {
        if (c[i] == lit1) {
            std::swap(c[i], c[0]);
            foundDatas++;
        }
    }

    for (uint16_t i = 0; i < c.size(); i++) {
        if (c[i] == lit2) {
            std::swap(c[i], c[1]);
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
    for (uint32_t i = 0; i < clauses.size(); i++) {
        reArrangeClause(clauses[i]);
    }
    for (uint32_t i = 0; i < learnts.size(); i++) {
        reArrangeClause(learnts[i]);
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
    updateArray(candidateForBothProp, interToOuter);
    updateArray(backupPolarity, interToOuter);
    updateArray(decision_var, interToOuter);
    PropEngine::updateVars(outerToInter, interToOuter, interToOuter2);
    updateLitsMap(assumptions, outerToInter);

    //Update reachability
    updateArray(litReachable, interToOuter2);
    for(size_t i = 0; i < litReachable.size(); i++) {
        if (litReachable[i].lit != lit_Undef)
            litReachable[i].lit = getUpdatedLit(litReachable[i].lit, outerToInter);
    }

    //Update clauses
    //Clauses' abstractions have to be re-calculated
    for(size_t i = 0; i < clauses.size(); i++) {
        updateLitsMap(*clauses[i], outerToInter);
        (*clauses[i]).reCalcAbstraction();
    }

    for(size_t i = 0; i < learnts.size(); i++) {
        updateLitsMap(*learnts[i], outerToInter);
        (*learnts[i]).reCalcAbstraction();
    }

    //Update sub-elements' vars
    subsumer->updateVars(outerToInter, interToOuter);
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
    const Var var = decision_var.size();

    outerToInterMain.push_back(var);
    interToOuterMain.push_back(var);
    decision_var.push_back(dvar);
    numDecisionVars += dvar;

    implCache.addNew();
    litReachable.push_back(LitReachData());
    litReachable.push_back(LitReachData());
    backupActivity.push_back(0);
    backupPolarity.push_back(false);
    candidateForBothProp.push_back(TwoSignAppearances());

    Searcher::newVar();

    varReplacer->newVar();
    subsumer->newVar();

    return decision_var.size()-1;
}

/// @brief Sort clauses according to glues: large glues first
bool Solver::reduceDBStructGlue::operator () (
    const Clause* x
    , const Clause* y
) {
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

/// @brief Sort clauses according to size: large sizes first
bool Solver::reduceDBStructSize::operator () (
    const Clause* x
    , const Clause* y
) {
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
    const Clause* x
    , const Clause* y
) {
    const uint32_t xsize = x->size();
    const uint32_t ysize = y->size();

    //No clause should be less than 3-long: 2&3-long are not removed
    assert(xsize > 2 && ysize > 2);

    //First tie: numPropConfl -- notice the reversal of 1/0
    //Larger is better --> should be last in the sorted list
    if (x->stats.numPropAndConfl > y->stats.numPropAndConfl) return 0;
    if (x->stats.numPropAndConfl < y->stats.numPropAndConfl) return 1;

    //Second tie: size
    return xsize > ysize;
}

/**
@brief Removes learnt clauses that have been found not to be too good

Either based on glue or MiniSat-style learnt clause activities, the clauses are
sorted and then removed
*/
void Solver::reduceDB()
{
    //Clean the clause database before doing cleaning
    clauseCleaner->removeAndCleanAll();

    const double myTime = cpuTime();
    solveStats.nbReduceDB++;
    CleaningStats tmpStats;
    tmpStats.origNumClauses = learnts.size();
    tmpStats.origNumLits = learntsLits - numBinsLearnt*2;

    //Calculate how much to remove
    uint32_t removeNum = (double)learnts.size() * conf.ratioRemoveClauses;

    if (conf.doPreClauseCleanPropAndConfl) {
        //Reduce based on props&confls
        size_t i, j;
        for (i = j = 0; i < learnts.size(); i++) {
            Clause* cl = learnts[i];
            assert(learnts[i]->size() > 3);
            if (cl->stats.numPropAndConfl < conf.preClauseCleanLimit
                && cl->stats.conflictNumIntroduced + conf.preCleanMinConflTime
                    < sumStats.conflStats.numConflicts
            ) {
                //Stat update
                tmpStats.preRemovedClauses++;
                tmpStats.preRemovedClausesLits += cl->size();
                tmpStats.preRemovedClausesGlue += cl->stats.glue;
                if (cl->stats.glue > cl->size() + 1000) {
                    cout
                    << "c DEBUG strangely large glue: " << *cl
                    << " glue: " << cl->stats.glue
                    << " size: " << cl->size()
                    << endl;
                }

                //detach&free
                detachClause(*cl);
                clAllocator->clauseFree(cl);

            } else {
                learnts[j++] = learnts[i];
            }
        }
        learnts.resize(learnts.size() -(i-j));
    }

    //Clean according to type
    tmpStats.clauseCleaningType = conf.clauseCleaningType;
    switch (conf.clauseCleaningType) {
        case CLEAN_CLAUSES_GLUE_BASED :
            //Sort for glue-based removal
            std::sort(learnts.begin(), learnts.end(), reduceDBStructGlue());
            tmpStats.glueBasedClean = 1;
            break;

        case CLEAN_CLAUSES_SIZE_BASED :
            //Sort for glue-based removal
            std::sort(learnts.begin(), learnts.end(), reduceDBStructSize());
            tmpStats.sizeBasedClean = 1;
            break;

        case CLEAN_CLAUSES_PROPCONFL_BASED :
            //Sort for glue-based removal
            std::sort(learnts.begin(), learnts.end(), reduceDBStructPropConfl());
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

    /*if (conf.verbosity >= 2) {
        cout << "c To remove (according to remove ratio): " << removeNum;
        if (removeNum <= tmpStats.preRemovedClauses)
            removeNum = 0;
        else
            removeNum -= tmpStats.preRemovedClauses;
        cout << " -- still to be removed: " << removeNum << endl;
    }*/

    //Remove normally
    size_t i, j;
    for (i = j = 0
        ; i < learnts.size() && tmpStats.removedClauses < removeNum
        ; i++
    ) {
        //Prefetch next clause
        if (i+1 < learnts.size())
            __builtin_prefetch(learnts[i+1], 0);

        Clause *cl = learnts[i];
        assert(cl->size() > 3);
        if (learnts[i]->stats.glue > 2
            && cl->stats.numPropAndConfl < conf.clauseCleanNeverCleanAtOrAboveThisPropConfl
        ) {
            //Stats
            tmpStats.removedClauses++;
            tmpStats.removedClausesLits+= cl->size();
            tmpStats.removedClausesGlue += cl->stats.glue;

            //detach & free
            detachClause(*cl);
            clAllocator->clauseFree(cl);
        } else {
            //Stats
            tmpStats.remainClauses++;
            tmpStats.remainClausesLits+= cl->size();
            tmpStats.remainClausesGlue += cl->stats.glue;

            learnts[j++] = cl;
        }
    }

    //Count what is left
    for (; i < learnts.size(); i++) {
        const Clause* cl = learnts[i];
        tmpStats.remainClauses++;
        tmpStats.remainClausesLits+= cl->size();
        tmpStats.remainClausesGlue += cl->stats.glue;

        learnts[j++] = learnts[i];
    }

    //Resize learnt datastruct
    learnts.resize(learnts.size() - (i - j));

    //Print results
    tmpStats.cpu_time = cpuTime() - myTime;
    if (conf.verbosity >= 1) {
        if (conf.verbosity >= 3)
            tmpStats.print(1);
        else
            tmpStats.printShort();
    }
    cleaningStats += tmpStats;
}

lbool Solver::solve(const vector<Lit>* _assumptions)
{
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
    while (status == l_Undef) {
        if (conf.verbosity >= 2)
            printClauseSizeDistrib();

        //This is crucial, since we need to attach() clauses to threads
        clauseCleaner->removeAndCleanAll();

        //Solve using threads
        const size_t origTrailSize = trail.size();
        vector<lbool> statuses;
        uint32_t numConfls = nextCleanLimit - sumStats.conflStats.numConflicts;
        for (size_t i = 0; i < conf.numCleanBetweenSimplify; i++) {
            numConfls+= (double)nextCleanLimitInc * std::pow(conf.increaseClean, i);
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

    //Cache clean before failed lit (for speed)
    if (conf.doCache)
        implCache.clean(this);

    if (!implCache.tryBoth(this))
        goto end;

    /*if (conf.doBothProp && !bothProp->tryBothProp())
        goto end;*/

    //Treat implicits
    if (!subsumeAndStrengthenImplicit())
        goto end;

    //PROBE
    if (conf.doProbe && !prober->probe())
        goto end;

    //SCC&VAR-REPL
    if (solveStats.numSimplify > 0
        && conf.doFindAndReplaceEqLits
    ) {
        if (!sCCFinder->find2LongXors())
            goto end;

        if (!varReplacer->performReplace())
            goto end;
    }

    if (needToInterrupt) return l_Undef;

    //Treat implicits
    if (!subsumeAndStrengthenImplicit())
        goto end;

    //Subsume only
    if (conf.doClausVivif && !clauseVivifier->vivify(false)) {
        goto end;
    }

    //Var-elim, gates, subsumption, strengthening
    if (conf.doSatELite && !subsumer->simplifyBySubsumption())
        goto end;

    //Vivify clauses
    if (solveStats.numSimplify > 1) {
        if (conf.doClausVivif && !clauseVivifier->vivify(true)) {
            goto end;
        }
    } else {
        //Subsume only
        if (conf.doClausVivif && !clauseVivifier->vivify(false)) {
            goto end;
        }
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

    //Cleaning, stat counting, etc.
    if (conf.doCache)
        implCache.clean(this);

    if (!implCache.tryBoth(this))
        goto end;

    if (conf.doCache && conf.doCalcReach)
        calcReachability();

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
    if (conf.doClearPropConfEveryClauseCleaning) {
        clearPropConfl(clauses);
        clearPropConfl(learnts);
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

void Solver::calcReachability()
{
    numCallReachCalc++;
    ReachabilityStats tmpStats;
    const double myTime = cpuTime();

    for (uint32_t i = 0; i < nVars()*2; i++) {
        litReachable[i] = LitReachData();
    }

    for (size_t litnum = 0; litnum < nVars()*2; litnum++) {
        const Lit lit = Lit::toLit(litnum);
        const Var var = lit.var();
        if (value(var) != l_Undef
            || varData[var].elimed != ELIMED_NONE
            || !decision_var[var]
        ) continue;

        //See where setting this lit leads to
        const vector<LitExtra>& cache = implCache[(~lit).toInt()].lits;
        const size_t cacheSize = cache.size();
        for (vector<LitExtra>::const_iterator it = cache.begin(), end = cache.end(); it != end; it++) {
            //Cannot lead to itself
            assert(it->getLit().var() != lit.var());

            //If learnt, skip
            if (!it->getOnlyNLBin())
                continue;

            //If reachability is nonexistent or low, set it
            if (litReachable[it->getLit().toInt()].lit == lit_Undef
                || litReachable[it->getLit().toInt()].numInCache < cacheSize
            ) {
                litReachable[it->getLit().toInt()].lit = lit;
                //NOTE: we actually MISREPRESENT this, as only non-learnt should be presented here
                litReachable[it->getLit().toInt()].numInCache = cacheSize;
            }
        }
    }

    vector<size_t> forEachSize(nVars()*2, 0);
    for(vector<LitReachData>::const_iterator
        it = litReachable.begin(), end = litReachable.end()
        ; it != end
        ; it++
    ) {
        if (it->lit != lit_Undef)
            forEachSize[it->lit.toInt()]++;
    }

    size_t lit = 0;
    for(vector<LitReachData>::const_iterator
        it = litReachable.begin(), end = litReachable.end()
        ; it != end
        ; it++, lit++
    ) {
        if (forEachSize[lit])
            tmpStats.dominators++;

        const size_t var = lit/2;

        //Variable is not used
        if (varData[var].elimed != ELIMED_NONE
            || value(var) != l_Undef
            || !decision_var[var]
        )
            continue;

        tmpStats.numLits++;
        tmpStats.numLitsDependent += (it->lit == lit_Undef) ? 0 : 1;
    }

    tmpStats.cpu_time = cpuTime() - myTime;
    if (conf.verbosity >= 1) {
        if (conf.verbosity >= 3)
            tmpStats.print();
        else
            tmpStats.printShort();
    }
    reachStats += tmpStats;
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
            learnts.push_back(cl);
            break;
    }

    return cl;
}

Solver::UsageStats Solver::sumClauseData(
    const vector<Clause*>& toprint
    , const bool learnt
) const {
    vector<UsageStats> usageStats;
    vector<UsageStats> usageStatsGlue;

    //Reset stats
    UsageStats stats;

    for(vector<Clause*>::const_iterator
        it = toprint.begin()
        , end = toprint.end()
        ; it != end
        ; it++
    ) {
        //Clause data
        const Clause& cl = **it;
        const uint32_t clause_size = cl.size();

        //We have stats on this clause
        if (cl.size() == 3)
            continue;

        //Sum stats
        stats.num++;
        stats.sumPropConfl += cl.stats.numPropAndConfl;
        stats.sumLitVisited += cl.stats.numLitVisited;
        stats.sumLookedAt += cl.stats.numLookedAt;

        //Update size statistics
        if (usageStats.size() < cl.size() + 1U)
            usageStats.resize(cl.size()+1);

        usageStats[clause_size].num++;
        usageStats[clause_size].sumPropConfl += cl.stats.numPropAndConfl;
        usageStats[clause_size].sumLitVisited += cl.stats.numLitVisited;
        usageStats[clause_size].sumLookedAt += cl.stats.numLookedAt;

        //If learnt, sum up GLUE-based stats
        if (learnt) {
            const size_t glue = cl.stats.glue;
            assert(glue != std::numeric_limits<uint32_t>::max());
            if (usageStatsGlue.size() < glue + 1) {
                usageStatsGlue.resize(glue + 1);
            }

            usageStatsGlue[glue].num++;
            usageStatsGlue[glue].sumPropConfl += cl.stats.numPropAndConfl;
            usageStatsGlue[glue].sumLitVisited += cl.stats.numLitVisited;
            usageStatsGlue[glue].sumLookedAt += cl.stats.numLookedAt;
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
            << " Props&confls: " << std::setw(10) << cl.stats.numPropAndConfl
            << " Lit visited: " << std::setw(10)<< cl.stats.numLitVisited
            << " Looked at: " << std::setw(10)<< cl.stats.numLookedAt
            << " Props&confls/Litsvisited*10: ";
            if (cl.stats.numLitVisited > 0) {
                cout
                << std::setw(6) << std::fixed << std::setprecision(4)
                << (10.0*(double)cl.stats.numPropAndConfl/(double)cl.stats.numLitVisited);
            }
            cout << endl;
        }
    }

    if (conf.verbosity >= 3) {
        //Print SUM stats
        if (learnt) {
            cout << "c Learnt    ";
        } else {
            cout << "c Non-learnt";
        }
        cout
        << " sum lits visited: "
        << std::setw(8) << stats.sumLitVisited/1000UL
        << " K";

        cout
        << " sum cls visited: "
        << std::setw(7) << stats.sumLookedAt/1000UL
        << " K";

        cout
        << " sum prop&conf: "
        << std::setw(6) << stats.sumPropConfl/1000UL
        << " K"
        << endl;
    }

    //Print more stats
    if (conf.verbosity >= 4) {
        printPropConflStats("clause-len", usageStats);

        if (learnt) {
            printPropConflStats("clause-glue", usageStatsGlue);
        }
    }

    return stats;
}

void Solver::printPropConflStats(
    std::string name
    , const vector<UsageStats>& stats
) const {
    for(size_t i = 0; i < stats.size(); i++) {
        //Nothing to do here, no stats really
        if (stats[i].num == 0)
            continue;

        cout
        << name << " : " << std::setw(4) << i
        << " Avg. props&confls: " << std::setw(6) << std::fixed << std::setprecision(2)
        << ((double)stats[i].sumPropConfl/(double)stats[i].num);

        if (stats[i].sumLookedAt > 0) {
            cout
            << " Props&confls/looked at: " << std::setw(6) << std::fixed << std::setprecision(2)
            << ((double)stats[i].sumPropConfl/(double)stats[i].sumLookedAt);
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
            << (10.0*(double)stats[i].sumPropConfl/(double)stats[i].sumLitVisited);
        }

        cout << endl;
    }
}

void Solver::dumpIndividualPropConflStats(
    std::string name
    , const vector<UsageStats>& stats
    , const bool learnt
) const {
    //Generate file name
    std::stringstream ss;
    ss << "stats/propconflPERlitsvisited-stat-" << solveStats.nbReduceDB << "-"
    << name << "-learnt-" << (int)learnt << ".txt";

    //Open file
    std::ofstream file(ss.str().c_str());
    if (!file) {
        cout << "Couldn't open file " << ss.str() << " for writing!" << endl;
        exit(-1);
    }

    //Dump stats per-glue/size line-by-line
    for(size_t i = 0; i < stats.size(); i++) {
        if (stats[i].sumLitVisited == 0)
            continue;

        file
        << i << "  "
        << (double)stats[i].sumPropConfl/(double)stats[i].sumLitVisited
        << endl;
    }

    //Generate file name
    std::stringstream ss2;
    ss2 << "stats/AVGpropconfl-stat-" << solveStats.nbReduceDB << "-"
    << name << "-learnt-" << (int)learnt << ".txt";

    //Open file
    std::ofstream file2(ss2.str().c_str());
    if (!file2) {
        cout << "Couldn't open file " << ss2.str() << " for writing!" << endl;
        exit(-1);
    }

    //Dump stats per glue/size line-by-line
    for(size_t i = 0; i < stats.size(); i++) {
        if (stats[i].num == 0)
            continue;

        file2
        << i << "  "
        << (double)stats[i].sumPropConfl/(double)stats[i].num
        << endl;
    }
}

void Solver::clearPropConfl(vector<Clause*>& clauseset)
{
    //Clear prop&confl for normal clauses
    for(vector<Clause*>::iterator
        it = clauseset.begin(), end = clauseset.end()
        ; it != end
        ; it++
    ) {
        (*it)->stats.numLitVisited = 0;
        (*it)->stats.numPropAndConfl = 0;
        (*it)->stats.numLookedAt = 0;
    }
}

void Solver::printClauseStatsSQL(vector<Clause*> cls)
{
    for(vector<Clause*>::const_iterator
        it = cls.begin(), end = cls.end()
        ; it != end
        ; it++
    ) {
        sqlFile
        << "insert into `clauseStats`"
        << "("
        << " `runID`, `simplifications`, `reduceDB`"
        << " , `learnt`, `size`, `glue`, `numPropAndConfl`, `numLitVisited`, `numLookedAt`"
        << ")"
        << " values ("
        //Position
        << "  " << getSolveStats().runID
        << ", " << getSolveStats().numSimplify
        << ", " << solveStats.nbReduceDB

        //data
        << ", " << (int)(*it)->learnt()
        << ", " << (*it)->size()
        << ", " << (*it)->stats.glue
        << ", " << (*it)->stats.numPropAndConfl
        << ", " << (*it)->stats.numLitVisited
        << ", " << (*it)->stats.numLookedAt

        //finish
        << " );" << endl;
    }
}

void Solver::fullReduce()
{
    if (conf.verbosity >= 1) {
        UsageStats stats;
        stats += sumClauseData(clauses, false);
        stats += sumClauseData(learnts, true);

        cout
        << "c sum            lits visited: "
        << std::setw(8) << stats.sumLitVisited/1000UL
        << " K";

        cout
        << " sum cls visited: "
        << std::setw(7) << stats.sumLookedAt/1000UL
        << " K";

        cout
        << " sum prop&conf: "
        << std::setw(6) << stats.sumPropConfl/1000UL
        << " K"
        << endl;
    }

    if (conf.doSQL) {
        //printClauseStatsSQL(clauses);
        //printClauseStatsSQL(learnts);
    }
    reduceDB();
    solver->consolidateMem();

    if (conf.doClearPropConfEveryClauseCleaning) {
        clearPropConfl(clauses);
        clearPropConfl(learnts);
    }

    nextCleanLimit += nextCleanLimitInc;
    nextCleanLimitInc *= conf.increaseClean;
}

void Solver::consolidateMem()
{
    clAllocator->consolidate(this, true);
}

void Solver::printFullStats()
{
    const double cpu_time = cpuTime();
    printStatsLine("c UIP search time"
        , sumStats.cpu_time
        , sumStats.cpu_time/cpu_time*100.0
        , "% time"
    );

    cout << "c ------- FINAL TOTAL SOLVING STATS ---------" << endl;
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
    printStatsLine("c probing time"
        , prober->getStats().cpu_time
        , prober->getStats().cpu_time/cpu_time*100.0
        , "% time"
    );

    prober->getStats().print(nVars());

    //Simplifier stats
    printStatsLine("c SatELite time"
        , subsumer->getStats().totalTime()
        , subsumer->getStats().totalTime()/cpu_time*100.0
        , "% time"
    );

    subsumer->getStats().print(solver->nVars());

    //GateFinder stats
    /*printStatsLine("c gatefinder time"
                    , subsumer->getGateFinder()->getStats().totalTime()
                    , subsumer->getGateFinder()->getStats().totalTime()/cpu_time*100.0
                    , "% time");
    subsumer->getGateFinder()->getStats().print(solver->nVars());

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

    printStatsLine("c cache bprop time"
        , implCache.getStats().cpu_time
        , implCache.getStats().cpu_time/cpu_time*100.0
        , "% time"
    );
    printStatsLine("c cache bprop calls"
        , implCache.getStats().numCalls
        , implCache.getStats().cpu_time/implCache.getStats().numCalls
        , "s/call"
    );
    printStatsLine("c cache bprop 0-depth ass"
        , implCache.getStats().zeroDepthAssigns
        , implCache.getStats().zeroDepthAssigns/implCache.getStats().numCalls
        , "assign/call"
    );


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
    clauseVivifier->getStats().print(solver->nVars());

    //Other stats
    printStatsLine("c Conflicts in UIP"
        , sumStats.conflStats.numConflicts
        , (double)sumStats.conflStats.numConflicts/cpu_time
        , "confl/TOTAL_TIME_SEC"
    );
    printStatsLine("c Total time", cpu_time);
}

void Solver::dumpBinClauses(const bool alsoLearnt, const bool alsoNonLearnt, std::ostream& outfile) const
{
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

                if (toDump)
                    outfile << it2->lit1() << " " << lit << " 0" << endl;
            }
        }
    }
}

void Solver::printClauseSizeDistrib()
{
    size_t size3 = 0;
    size_t size4 = 0;
    size_t size5 = 0;
    size_t sizeLarge = 0;
    for(vector<Clause*>::const_iterator it = clauses.begin(), end = clauses.end(); it != end; it++) {
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

    cout << "c size3: " << size3
    << " size4: " << size4
    << " size5: " << size5
    << " larger: " << sizeLarge << endl;
}

void Solver::dumpLearnts(std::ostream& os, const uint32_t maxSize)
{
    os
    << "c " << endl
    << "c ---------" << endl
    << "c unitaries" << endl
    << "c ---------" << endl;
    for (uint32_t i = 0, end = (trail_lim.size() > 0) ? trail_lim[0] : trail.size() ; i < end; i++) {
        os << trail[i] << " 0" << endl;    }


    os
    << "c " << endl
    << "c ---------------------------------" << endl
    << "c learnt binary clauses (extracted from watchlists)" << endl
    << "c ---------------------------------" << endl;
    if (maxSize >= 2)
        dumpBinClauses(true, false, os);

    os
    << "c " << endl
    << "c ---------------------------------------" << endl
    << "c clauses representing 2-long XOR clauses" << endl
    << "c ---------------------------------------" << endl;
    if (maxSize >= 2) {
        const vector<Lit>& table = varReplacer->getReplaceTable();
        for (Var var = 0; var != table.size(); var++) {
            Lit lit = table[var];
            if (lit.var() == var)
                continue;

            os << (~lit) << " " << Lit(var, false) << " 0" << endl;
            os << lit << " " << Lit(var, true) << " 0" << endl;
        }
    }

    os
    << "c " << endl
    << "c --------------------" << endl
    << "c clauses from learnts" << endl
    << "c --------------------" << endl;
    for (int i = learnts.size()-1; i >= 0 ; i--) {
        Clause& cl = *learnts[i];
        if (cl.size() <= maxSize) {
            os << cl << " 0" << endl;
            os
            << "c clause learnt "
            << (cl.learnt() ? "yes" : "no")
            << " stats "  << cl.stats << endl;
        }
    }
}

void Solver::dumpIrredClauses(std::ostream& os) const
{
    uint32_t numClauses = 0;
    //unitary clauses
    for (uint32_t i = 0, end = (trail_lim.size() > 0) ? trail_lim[0] : trail.size() ; i < end; i++)
        numClauses++;

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
    numClauses += clauses.size();

    //previously eliminated clauses
    const vector<BlockedClause>& blockedClauses = subsumer->getBlockedClauses();
    numClauses += blockedClauses.size();

    os << "p cnf " << nVars() << " " << numClauses << endl;

    ////////////////////////////////////////////////////////////////////

    os
    << "c " << endl
    << "c ---------" << endl
    << "c unitaries" << endl
    << "c ---------" << endl;
    for (uint32_t i = 0, end = (trail_lim.size() > 0) ? trail_lim[0] : trail.size() ; i < end; i++) {
        os << trail[i] << " 0" << endl;
    }

    os
    << "c " << endl
    << "c ---------------------------------------" << endl
    << "c clauses representing 2-long XOR clauses" << endl
    << "c ---------------------------------------" << endl;
    for (Var var = 0; var != table.size(); var++) {
        Lit lit = table[var];
        if (lit.var() == var)
            continue;

        Lit litP1 = ~lit;
        Lit litP2 = Lit(var, false);
        os << litP1 << " " << litP2 << endl;
        os << ~litP1 << " " << ~litP2 << endl;
    }

    os
    << "c " << endl
    << "c ---------------" << endl
    << "c binary clauses" << endl
    << "c ---------------" << endl;
    dumpBinClauses(false, true, os);

    os
    << "c " << endl
    << "c ---------------" << endl
    << "c normal clauses" << endl
    << "c ---------------" << endl;
    for (vector<Clause*>::const_iterator i = clauses.begin(); i != clauses.end(); i++) {
        assert(!(*i)->learnt());
        os << (**i) << " 0" << endl;
    }

    os
    << "c " << endl
    << "c -------------------------------" << endl
    << "c previously eliminated variables" << endl
    << "c -------------------------------" << endl;
    for (vector<BlockedClause>::const_iterator it = blockedClauses.begin(); it != blockedClauses.end(); it++) {
        os << "c next clause is eliminated/blocked on lit " << it->blockedOn << endl;
        os << it->lits << " 0" << endl;
    }
}

void Solver::printAllClauses() const
{
    for (uint32_t i = 0; i < clauses.size(); i++) {
        cout
                << "Normal clause num " << clAllocator->getOffset(clauses[i])
                << " cl: " << *clauses[i] << endl;
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
                cout << "Normal clause num " << it2->getOffset() << endl;
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
                cout << "bin clause: " << lit << " , " << i->lit1() << " not satisfied!" << endl;
                cout << "value of unsat bin clause: " << value(lit) << " , " << value(i->lit1()) << endl;
                return false;
            }
        }
    }

    return true;
}

bool Solver::verifyClauses(const vector<Clause*>& cs) const
{
    #ifdef VERBOSE_DEBUG
    cout << "Checking clauses whether they have been properly satisfied." << endl;;
    #endif

    bool verificationOK = true;

    for (uint32_t i = 0; i != cs.size(); i++) {
        Clause& c = *cs[i];
        for (uint32_t j = 0; j < c.size(); j++)
            if (modelValue(c[j]) == l_True)
                goto next;

            cout << "unsatisfied clause: " << *cs[i] << endl;
        verificationOK = false;
        next:
                ;
    }

    return verificationOK;
}

bool Solver::verifyModel() const
{
    bool verificationOK = true;
    verificationOK &= verifyClauses(clauses);
    verificationOK &= verifyClauses(learnts);
    verificationOK &= verifyBinClauses();

    if (conf.verbosity >= 1 && verificationOK) {
        cout
        << "c Verified " <<  clauses.size() << " clauses."
        << endl;
    }

    return verificationOK;
}


void Solver::checkLiteralCount() const
{
    // Check that sizes are calculated correctly:
    uint64_t cnt = 0;
    for (uint32_t i = 0; i != clauses.size(); i++)
        cnt += clauses[i]->size();

    if (clausesLits != cnt) {
        cout << "c ERROR! literal count: " << clausesLits << " , real value = " <<  cnt << endl;
        assert(clausesLits == cnt);
    }
}

uint32_t Solver::getNumDecisionVars() const
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

    for (vector<Clause*>::const_iterator it = clauses.begin(), end = clauses.end(); it != end; it++) {
        assert(normClauseIsAttached(**it));
    }
}

bool Solver::normClauseIsAttached(const Clause& c) const
{
    bool attached = true;
    assert(c.size() > 3);

    ClOffset offset = clAllocator->getOffset(&c);
    attached &= findWCl(watches[c[0].toInt()], offset);
    attached &= findWCl(watches[c[1].toInt()], offset);

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
            if (!findClause(cl)) {
                cout << "ERROR! did not find clause!" << endl;
            }
        }
    }
}


bool Solver::findClause(const Clause* c) const
{
    for (uint32_t i = 0; i < clauses.size(); i++) {
        if (clauses[i] == c) return true;
    }
    for (uint32_t i = 0; i < learnts.size(); i++) {
        if (learnts[i] == c) return true;
    }

    return false;
}

void Solver::checkNoWrongAttach() const
{
    #ifndef VERBOSE_DEBUG
    return;
    #endif //VERBOSE_DEBUG

    for (vector<Clause*>::const_iterator
        i = learnts.begin(), end = learnts.end()
        ; i != end; i++
    ) {
        const Clause& cl = **i;
        for (uint32_t i = 0; i < cl.size(); i++) {
            if (i > 0) assert(cl[i-1].var() != cl[i].var());
        }
    }
}

uint32_t Solver::getNumFreeVars() const
{
    assert(decisionLevel() == 0);
    uint32_t freeVars = nVars();
    freeVars -= trail.size();
    freeVars -= subsumer->getStats().numVarsElimed;
    freeVars -= varReplacer->getNumReplacedVars();

    return freeVars;
}

uint32_t Solver::getNumFreeVarsAdv(size_t trail_size_of_thread) const
{
    assert(decisionLevel() == 0);
    uint32_t freeVars = nVars();
    freeVars -= trail_size_of_thread;
    freeVars -= subsumer->getStats().numVarsElimed;
    freeVars -= varReplacer->getNumReplacedVars();

    return freeVars;
}

void Solver::printClauseStats()
{
    if (clauses.size() > 20000) {
        cout
        << " " << std::setw(4) << clauses.size()/1000 << "K";
    } else {
        cout
        << " " << std::setw(7) << clauses.size();
    }

    if (numTrisNonLearnt > 20000) {
        cout
        << " " << std::setw(4) << numTrisNonLearnt/1000 << "K";
    } else {
        cout
        << " " << std::setw(7) << numTrisNonLearnt;
    }

    if (numBinsNonLearnt > 20000) {
        cout
        << " " << std::setw(4) << numBinsNonLearnt/1000 << "K";
    } else {
        cout
        << " " << std::setw(6) << numBinsNonLearnt;
    }

    cout
    << " " << std::setw(4) << std::fixed << std::setprecision(1);

    cout
    << (double)(clausesLits - numBinsNonLearnt*2)/(double)(clauses.size() + numTrisNonLearnt)

    << " " << std::setw(6) << learnts.size()
    << " " << std::setw(6) << numTrisLearnt;

    if (numBinsLearnt > 20000) {
        cout
        << " " << std::setw(4) << numBinsLearnt/1000 << "K";
    } else {
        cout
        << " " << std::setw(6) << numBinsLearnt;
    }

    cout
    << " " << std::setw(4) << std::fixed << std::setprecision(1)
    << (double)(learntsLits - numBinsLearnt*2)/(double)(learnts.size() + numTrisLearnt)
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

    if (thisNumNonLearntBins/2 != numBinsNonLearnt) {
        cout
        << "ERROR:"
        << " thisNumNonLearntBins/2: " << thisNumNonLearntBins/2
        << " numBinsNonLearnt: " << numBinsNonLearnt
        << "thisNumNonLearntBins: " << thisNumNonLearntBins
        << "thisNumLearntBins: " << thisNumLearntBins << endl;
    }
    assert(thisNumNonLearntBins % 2 == 0);
    assert(thisNumNonLearntBins/2 == numBinsNonLearnt);

    if (thisNumLearntBins/2 != numBinsLearnt) {
        cout
        << "ERROR:"
        << " thisNumLearntBins/2: " << thisNumLearntBins/2
        << " numBinsLearnt: " << numBinsLearnt
        << endl;
    }
    assert(thisNumLearntBins % 2 == 0);
    assert(thisNumLearntBins/2 == numBinsLearnt);

    if (thisNumNonLearntTris/3 != numTrisNonLearnt) {
        cout
        << "ERROR:"
        << " thisNumNonLearntTris/3: " << thisNumNonLearntTris/3
        << " numTrisNonLearnt: " << numTrisNonLearnt
        << endl;
    }
    assert(thisNumNonLearntTris % 3 == 0);
    assert(thisNumNonLearntTris/3 == numTrisNonLearnt);

    if (thisNumLearntTris/3 != numTrisLearnt) {
        cout
        << "ERROR:"
        << " thisNumLearntTris/3: " << thisNumLearntTris/3
        << " numTrisLearnt: " << numTrisLearnt
        << endl;
    }
    assert(thisNumLearntTris % 3 == 0);
    assert(thisNumLearntTris/3 == numTrisLearnt);
}

void Solver::checkStats(const bool allowFreed) const
{
    //If in crazy mode, don't check
    #ifdef NDEBUG
    return;
    #endif

    checkImplicitStats();

    //Check number of non-learnt literals
    uint64_t numLitsNonLearnt = numBinsNonLearnt*2 + numTrisNonLearnt*3;
    for(vector<Clause*>::const_iterator
        it = clauses.begin(), end = clauses.end()
        ; it != end
        ; it++
    ) {
        const Clause& cl = **it;
        if (cl.freed()) {
            assert(allowFreed);
        } else {
            numLitsNonLearnt += cl.size();
        }
    }
    if (numLitsNonLearnt != clausesLits) {
        cout << "ERROR: " << endl;
        cout << "->numLitsNonLearnt: " << numLitsNonLearnt << endl;
        cout << "->clausesLits: " << clausesLits << endl;
    }
    assert(numLitsNonLearnt == clausesLits);

    //Check number of learnt literals
    uint64_t numLitsLearnt = numBinsLearnt*2 + numTrisLearnt*3;
    for(vector<Clause*>::const_iterator
        it = learnts.begin(), end = learnts.end()
        ; it != end
        ; it++
    ) {
        const Clause& cl = **it;
        if (cl.freed()) {
            assert(allowFreed);
        } else {
            numLitsLearnt += cl.size();
        }
    }
    if (numLitsLearnt != learntsLits) {
        cout << "ERROR: " << endl;
        cout << "->numLitsLearnt: " << numLitsLearnt << endl;
        cout << "->learntsLits: " << learntsLits << endl;
    }
    assert(numLitsLearnt == learntsLits);

}

uint32_t Solver::getNewToReplaceVars() const
{
    return varReplacer->getNewToReplaceVars();
}

const char* Solver::getVersion()
{
    return get_git_version();
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

bool Solver::subsumeAndStrengthenImplicit()
{
    assert(ok);
    const double myTime = cpuTime();
    uint64_t remBins = 0;
    uint64_t remTris = 0;
    uint64_t remLitFromBin = 0;
    uint64_t remLitFromTri = 0;
    const size_t origTrailSize = trail.size();

    //For delayed enqueue and binary adding
    //Used for strengthening
    vector<Lit> bin(2);
    vector<BinaryClause> binsToAdd;
    vector<Lit> toEnqueue;

    size_t wsLit = 0;
    for (vector<vec<Watched> >::iterator
        it = watches.begin(), end = watches.end()
        ; it != end
        ; it++, wsLit++
    ) {
        vec<Watched>& ws = *it;
        Lit lit = Lit::toLit(wsLit);

        //We can't do much when there is nothing, or only one
        if (ws.size() < 2)
            continue;

        std::sort(ws.begin(), ws.end(), WatchSorter());
        /*cout << "---> Before" << endl;
        printWatchlist(ws, lit);*/

        Watched* i = ws.begin();
        Watched* j = i;
        Watched* lastBin = NULL;

        Lit lastLit = lit_Undef;
        Lit lastLit2 = lit_Undef;
        bool lastLearnt = false;
        for (vec<Watched>::iterator end = ws.end(); i != end; i++) {

            //Don't care about long clauses
            if (i->isClause()) {
                *j++ = *i;
                continue;
            }

            if (i->isTri()) {

                //Only treat one of the TRI's instances
                if (lit > i->lit1()) {
                    *j++ = *i;
                    continue;
                }

                //Brand new TRI
                if (lastLit != i->lit1()) {
                    lastLit2 = i->lit2();
                    lastLearnt = i->learnt();
                    *j++ = *i;
                    continue;
                }

                bool remove = false;

                //Subsumed by bin
                if (lastLit2 == lit_Undef
                    && lastLit == i->lit1()
                ) {
                    if (lastLearnt && !i->learnt()) {
                        assert(lastBin->isBinary());
                        assert(lastBin->learnt());
                        assert(lastBin->lit1() == lastLit);

                        lastBin->setLearnt(false);
                        findWatchedOfBin(watches, lastLit, lit, true).setLearnt(false);
                        learntsLits -= 2;
                        clausesLits += 2;
                        numBinsLearnt--;
                        numBinsNonLearnt++;
                        lastLearnt = false;
                    }

                    remove = true;
                }

                //Subsumed by Tri
                if (lastLit == i->lit1() && lastLit2 == i->lit2()) {
                    //The sorting algorithm prefers non-learnt to learnt, so it is
                    //impossible to have non-learnt before learnt
                    assert(!(i->learnt() == false && lastLearnt == true));

                    remove = true;
                }

                if (remove) {
                    //Remove Tri
                    remTris++;
                    removeWTri(watches, i->lit1(), lit, i->lit2(), i->learnt());
                    removeWTri(watches, i->lit2(), lit, i->lit1(), i->learnt());

                    if (i->learnt()) {
                        learntsLits -= 3;
                        numTrisLearnt--;
                    } else {
                        clausesLits -= 3;
                        numTrisNonLearnt--;
                    }
                    continue;
                }

                //Don't remove
                lastLit = i->lit1();
                lastLit2 = i->lit2();
                lastLearnt = i->learnt();
                *j++ = *i;
                continue;
            }

            //Binary from here on
            assert(i->isBinary());

            //Subsume bin with bin
            if (i->lit1() == lastLit && lastLit2 == lit_Undef) {
                //The sorting algorithm prefers non-learnt to learnt, so it is
                //impossible to have non-learnt before learnt
                assert(!(i->learnt() == false && lastLearnt == true));

                remBins++;
                assert(i->lit1().var() != lit.var());
                removeWBin(watches, i->lit1(), lit, i->learnt());
                if (i->learnt()) {
                    learntsLits -= 2;
                    numBinsLearnt--;
                } else {
                    clausesLits -= 2;
                    numBinsNonLearnt--;
                }
                continue;
            } else {
                lastBin = j;
                lastLit = i->lit1();
                lastLit2 = lit_Undef;
                lastLearnt = i->learnt();
                *j++ = *i;
            }
        }
        ws.shrink(i-j);

        /*cout << "---> After" << endl;
        printWatchlist(ws, lit);*/


        //Now strengthen
        i = ws.begin();
        j = i;
        for (vec<Watched>::iterator end = ws.end(); i != end; i++) {
            //Can't do much with clause, will treat them during vivification
            if (i->isClause()) {
                *j++ = *i;
                continue;
            }

            //Strengthen bin with bin -- effectively setting literal
            if (i->isBinary()) {
                /*if (findWBin(watches, lit, ~i->lit1())) {
                    remLitFromBin++;
                    toEnqueue.push_back(lit);
                }*/
                *j++ = *i;
                continue;
            }

            //Strengthen tri with bin
            if (i->isTri()) {
                bool firstRem = findWBin(watches, lit, ~i->lit1());
                bool secondRem = findWBin(watches, lit, ~i->lit2());

                //Nothing to do
                if (!firstRem && !secondRem) {
                    *j++ = *i;
                    continue;
                }

                //Remove tri
                Lit lits[3];
                lits[0] = lit;
                lits[1] = i->lit1();
                lits[2] = i->lit2();
                std::sort(lits, lits+3);
                removeTriAllButOne(watches, lit, lits, i->learnt());

                //Update stats for tri
                if (i->learnt()) {
                    learntsLits -= 3;
                    numTrisLearnt--;
                } else {
                    clausesLits -= 3;
                    numTrisNonLearnt--;
                }

                //If both are to be removed, then only one lit left -> enqueue
                if (firstRem && secondRem) {
                    remLitFromTri += 2;
                    toEnqueue.push_back(lit);
                    continue;
                }

                //Exaclty one will be removed
                remLitFromTri++;

                if (firstRem) {
                    binsToAdd.push_back(BinaryClause(lit, i->lit2(), i->learnt()));
                    continue;
                }

                if (secondRem) {
                    binsToAdd.push_back(BinaryClause(lit, i->lit1(), i->learnt()));
                    continue;
                }
            }

            //Only bin, tri and clause in watchlist
            assert(false);
        }
        ws.shrink(i-j);
    }

    //Enqueue delayed values
    for(vector<Lit>::const_iterator
        it = toEnqueue.begin(), end = toEnqueue.end()
        ; it != end
        ; it++
    ) {
        if (value(*it) == l_False) {
            ok = false;
            break;
        }

        if (value(*it) == l_Undef)
            enqueue(*it);
    }
    ok = propagate().isNULL();
    if (!ok)
        goto end;

    //Add delayed binary clauses
    for(vector<BinaryClause>::const_iterator
        it = binsToAdd.begin(), end = binsToAdd.end()
        ; it != end
        ; it++
    ) {
        bin[0] = it->getLit1();
        bin[1] = it->getLit2();
        addClauseInt(bin, it->getLearnt());
        if (!ok)
            goto end;
    }

end:
    if (conf.verbosity  >= 1) {
        cout
        << "c [implicit]"
        << " rem-bin " << remBins
        << " rem-tri " << remTris
        << " rem-litBin: " << remLitFromBin
        << " rem-litTri: " << remLitFromTri
        << " set-var: " << trail.size() - origTrailSize

        << " time: " << std::fixed << std::setprecision(2) << std::setw(5)
        << (cpuTime() - myTime)
        << " s" << endl;
    }
    solver->checkStats();

    //Update stats
    solveStats.subsBinWithBinTime += cpuTime() - myTime;
    solveStats.subsBinWithBin += remBins;

    return solver->ok;
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

