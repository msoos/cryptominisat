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
#include "Subsumer.h"
#include "ClauseCleaner.h"
#include "time_mem.h"
#include "assert.h"
#include <iomanip>
#include <cmath>
#include <algorithm>
#include "VarReplacer.h"
#include "CompleteDetachReattacher.h"
#include "VarUpdateHelper.h"
#include <set>
#include <algorithm>
#include <fstream>
#include <set>
using std::cout;
using std::endl;

#include "constants.h"
#include "SolutionExtender.h"
#include "XorFinder.h"
#include "GateFinder.h"

//#define VERBOSE_DEBUG
#ifdef VERBOSE_DEBUG
#define BIT_MORE_VERBOSITY
#define VERBOSE_ORGATE_REPLACE
#define VERBOSE_ASYMTE
#define VERBOSE_GATE_REMOVAL
#define VERBOSE_XORGATE_MIX
#define VERBOSE_DEBUG_XOR_FINDER
#define VERBOSE_DEBUG_VARELIM
#endif

//#define VERBOSE_DEBUG_VARELIM
//#define VERBOSE_DEBUG_XOR_FINDER
//#define BIT_MORE_VERBOSITY
//#define TOUCH_LESS
//#define VERBOSE_ORGATE_REPLACE
//#define VERBOSE_DEBUG_ASYMTE
//#define VERBOSE_GATE_REMOVAL
//#define VERBOSE_XORGATE_MIX

Subsumer::Subsumer(Solver* _solver):
    solver(_solver)
    , varElimOrder(VarOrderLt(varElimComplexity))
    , numCalls(0)
{
    xorFinder = new XorFinder(this, solver);
    gateFinder = new GateFinder(this, solver);
}

Subsumer::~Subsumer()
{
    delete xorFinder;
    delete gateFinder;
}

/**
@brief New var has been added to the solver

@note: MUST be called if a new var has been added to the solver

Adds occurrence list places, increments seen, etc.
*/
void Subsumer::newVar()
{
    occur   .push_back(Occur());
    occur   .push_back(Occur());
    seen    .push_back(0);       // (one for each polarity)
    seen    .push_back(0);
    seen2   .push_back(0);       // (one for each polarity)
    seen2   .push_back(0);
    touchedVars .addOne(solver->nVars()-1);
    ol_seenNeg.push_back(1);
    ol_seenPos.push_back(1);
    gateFinder->newVar();

    //variable status
    var_elimed .push_back(0);
}

void Subsumer::updateVars(
    const vector<uint32_t>& outerToInter
    , const vector<uint32_t>& interToOuter
) {
    updateArray(var_elimed, interToOuter);

    for(vector<BlockedClause>::iterator
        it = blockedClauses.begin(), end = blockedClauses.end()
        ; it != end
        ; it++
    ) {
        it->blockedOn = getUpdatedLit(it->blockedOn, outerToInter);
        for(size_t i = 0; i < it->lits.size(); i++) {
            it->lits[i] = getUpdatedLit(it->lits[i], outerToInter);
        }
    }
}

void Subsumer::extendModel(SolutionExtender* extender) const
{
    //go through in reverse order
    for (vector<BlockedClause>::const_reverse_iterator
        it = blockedClauses.rbegin(), end = blockedClauses.rend()
        ; it != end
        ; it++
    ) {
        extender->addBlockedClause(*it);
    }
}

bool Subsumer::unEliminate(const Var, Solver*)
{
    //TODO
    assert(false);
    return solver->ok;
}

/**
@brief Backward-subsumption using given clause: helper function

Checks all clauses in the occurrence lists if they are subsumed by ps or not.

The input clause can be learnt. In that case, if it subsumes non-learnt clauses,
it will become non-learnt.

Handles it well if the subsumed clause has a higher activity than the subsuming
clause (will take the max() of the two)

@p c The clause to use
@p cl The clause to use

*/
void Subsumer::subsume0(ClauseIndex c, Clause& cl)
{
    #ifdef VERBOSE_DEBUG
    cout << "subsume0-ing with clause: " << cl << endl;
    #endif
    Sub0Ret ret = subsume0(c.index, cl, clauseData[c.index].abst);

    //If non-learnt is subsumed by learnt, make the learnt into non-learnt
    if (cl.learnt()
        && ret.subsumedNonLearnt
    ) {
        cl.makeNonLearnt();
    }

    //Combine stats
    cl.combineStats(ret.stats);
}

/**
@brief Backward-subsumption using given clause

@note Use helper function

@param ps The clause to use to backward-subsume
@param[in] abs The abstraction of the clause
@return Subsumed anything? If so, what was the max activity? Was it non-learnt?
*/
template<class T>
Subsumer::Sub0Ret Subsumer::subsume0(
    const uint32_t index
    , const T& ps
    , CL_ABST_TYPE abs
) {
    Sub0Ret ret;

    vector<ClauseIndex> subs;
    findSubsumed0(index, ps, abs, subs);

    //Go through each clause that can be subsumed
    for (vector<ClauseIndex>::const_iterator
        it = subs.begin(), end = subs.end()
        ; it != end
        ; it++
    ) {
        #ifdef VERBOSE_DEBUG
        cout << "-> subsume0 removing:" << *clauses[it->index] << endl;
        #endif

        Clause *tmp = clauses[it->index];

        //Combine stats
        ret.stats = ClauseStats::combineStats(tmp->stats, ret.stats);

        //At least one is non-learnt. Indicate this to caller.
        if (!tmp->learnt())
            ret.subsumedNonLearnt = true;

        runStats.clauses_subsumed++;
        unlinkClause(*it);
        ret.numSubsumed++;

        //If we are waaay over time, just exit
        if (*toDecrease < -20L*1000L*1000L)
            break;
    }

    return ret;
}

/**
@brief Backward subsumption and self-subsuming resolution

Performs backward subsumption AND
self-subsuming resolution using backward-subsumption

@param[in] ps The clause to use for backw-subsumption and self-subs. resolution
*/
void Subsumer::subsume1(ClauseIndex c, Clause& ps)
{
    vector<ClauseIndex>    subs;
    vector<Lit>           subsLits;
    #ifdef VERBOSE_DEBUG
    cout << "subsume1-ing with clause:" << ps << endl;
    #endif

    findSubsumed1(c.index, ps, clauseData[c.index].abst, subs, subsLits);
    for (uint32_t j = 0; j < subs.size(); j++) {
        ClauseIndex c = subs[j];
        Clause& cl = *clauses[c.index];
        if (subsLits[j] == lit_Undef) {
            runStats.clauses_subsumed++;

            //If subsumes a non-learnt, and is learnt, make it non-learnt
            if (ps.learnt()
                && !cl.learnt()
            ) {
                ps.makeNonLearnt();
            }

            //Update stats
            ps.combineStats(cl.stats);

            unlinkClause(c);
        } else {
            strengthen(c, subsLits[j]);
            if (!solver->ok)
                return;

            //If we are waaay over time, just exit
            if (*toDecrease < -20L*1000L*1000L)
                break;
        }
    }
}

/**
@brief Removes&free-s a clause from everywhere

Removes clause from occurence lists, from Subsumer::clauses

If clause is to be removed because the variable in it is eliminated, the clause
is saved in elimedOutVar[] before it is fully removed.

@param[in] c The clause to remove
@param[in] elim If the clause is removed because of variable elmination, this
parameter is different from var_Undef.
*/
void Subsumer::unlinkClause(ClauseIndex c, const Lit elim)
{
    Clause& cl = *clauses[c.index];
    for (uint32_t i = 0; i < cl.size(); i++) {
        *toDecrease -= 2*occur[cl[i].toInt()].size();

        occur[cl[i].toInt()].remove(c);
        touchedVars.touch(cl[i], cl.learnt());
    }

    // Remove from sets:
    strengthenWith.exclude(c);
    subsumeWith.exclude(c);

    //If elimed and non-learnt, we need to save it to stack
    if (elim != lit_Undef
        && !cl.learnt()
    ) {
        #ifdef VERBOSE_DEBUG
        cout << "Eliminating non-bin clause: " << *clauses[c.index] << endl;
        cout << "On variable: " << elim.unsign() << endl;
        #endif //VERBOSE_DEBUG

        vector<Lit> lits(cl.size());
        std::copy(cl.begin(), cl.end(), lits.begin());
        blockedClauses.push_back(BlockedClause(elim, lits));
    } else {
        solver->clAllocator->clauseFree(&cl);
        clauses[c.index] = NULL;
    }
}

lbool Subsumer::cleanClause(ClauseIndex c, Clause& cl)
{
    assert(solver->ok);
    #ifdef VERBOSE_DEBUG
    cout << "Clause to clean: " << cl << endl;
    for(size_t i = 0; i < cl.size(); i++) {
        cout << cl[i] << " : "  << solver->value(cl[i]) << " , ";
    }
    cout << endl;
    #endif

    bool satisfied = false;
    Lit* i = cl.begin();
    Lit* j = cl.begin();
    const Lit* end = cl.end();
    *toDecrease -= cl.size();
    for(; i != end; i++) {
        if (solver->value(*i) == l_Undef) {
            *j++ = *i;
            continue;
        }

        if (solver->value(*i) == l_True) {
            occur[i->toInt()].remove(c);
            touchedVars.touch(*i, cl.learnt());
            satisfied = true;
            continue;
        }

        if (solver->value(*i) == l_False) {
            occur[i->toInt()].remove(c);
            touchedVars.touch(*i, cl.learnt());
            continue;
        }
    }
    cl.shrink(i-j);

    if (satisfied) {
        #ifdef VERBOSE_DEBUG
        cout << "Clause cleaning -- satisfied, removing" << endl;
        #endif
        unlinkClause(c);
        return l_True;
    }

    //Update alreadyAdded and ol_seenNeg & ol_seenPos
    alreadyAdded[c.index] = 0;
    for(size_t i = 0; i < cl.size(); i++) {
        ol_seenNeg[cl[i].var()] = 0;
        ol_seenPos[cl[i].var()] = 0;
        *toDecrease -= 2;

        //Pos
        const Occur& occ = occur[cl[i].toInt()];
        for(Occur::const_iterator
            it = occ.begin(), end = occ.end()
            ; it != end; it++
        ) {
            alreadyAdded[it->index] = 0;
        }
        *toDecrease -= occ.size();

        //Neg
        const Occur& occ2 = occur[(~cl[i]).toInt()];
        for(Occur::const_iterator
            it = occ2.begin(), end = occ2.end()
            ; it != end; it++
        ) {
            alreadyAdded[it->index] = 0;
        }
        *toDecrease -= occ2.size();
    }

    #ifdef VERBOSE_DEBUG
    cout << "-> Clause became after cleaning:" << *clauses[c.index] << endl;
    #endif

    switch(cl.size()) {
        case 0:
            solver->ok = false;
            return l_False;

        case 1:
            solver->enqueue(cl[0]);
            solver->propStats.propsUnit++;
            unlinkClause(c);
            solver->ok = solver->propagate().isNULL();
            return (solver->ok ? l_True : l_False);

        case 2:
            solver->attachBinClause(cl[0], cl[1], cl.learnt());
            unlinkClause(c);
            return l_True;

        default:
            clauseData[c.index].abst = calcAbstraction(cl);
            clauseData[c.index].size = cl.size();
            strengthenWith.add(c);
            subsumeWith.add(c);
            return l_Undef;
    }
}

/**
@brief Removes a literal from a clause

May return with solver->ok being FALSE, and may set&propagate variable values.

@param c Clause to be cleaned of the literal
@param[in] toRemoveLit The literal to be removed from the clause
*/
void Subsumer::strengthen(ClauseIndex& c, const Lit toRemoveLit)
{
    #ifdef VERBOSE_DEBUG
    cout << "-> Strenghtening clause :" << *clauses[c.index];
    cout << " with lit: " << toRemoveLit << endl;
    #endif

    Clause& cl = *clauses[c.index];
    *toDecrease -= 5;
    cl.strengthen(toRemoveLit);
    runStats.litsRemStrengthen++;
    touchedVars.touch(toRemoveLit, cl.learnt());
    occur[toRemoveLit.toInt()].remove(c);

    cleanClause(c, cl);
}

void Subsumer::printLimits()
{
#ifdef BIT_MORE_VERBOSITY
    cout << "c  subsumed:" << clauses_subsumed << endl;
    cout << "c  strengthenWith.nElems():" << strengthenWith.nElems() << endl;
    cout << "c  subsumeWith.nElems():" << subsumeWith.nElems() << endl;
    cout << "c  clauses.size():" << clauses.size() << endl;
    cout << "c  numMaxSubsume0:" << numMaxSubsume0 << endl;
    cout << "c  numMaxSubsume1:" << numMaxSubsume1 << endl;
    cout << "c  numMaxElim:" << numMaxElim << endl;
#endif //BIT_MORE_VERBOSITY
}

void Subsumer::performSubsume0()
{
    vector<ClauseIndex> remClTouched; //These clauses will be untouched
    vector<ClauseIndex> s0;
    alreadyAdded.clear();
    alreadyAdded.resize(clauses.size(), 0);

    double myTime = cpuTime();
    toDecrease = &numMaxSubsume0;
    while (numMaxSubsume0 > 0 && subsumeWith.nElems() > 0)  {
        remClTouched.clear();
        s0.clear();
        printLimits();

        //Go through subsumeWith, break when clTouchedTodo is reached
        for (CSet::iterator
            it = subsumeWith.begin(), end = subsumeWith.end()
            ; it != end
            ; ++it
        ) {
            //Clause already removed
            if (it->index == std::numeric_limits< uint32_t >::max())
                continue;
            if (clauses[it->index] == NULL) {
                remClTouched.push_back(*it);
                continue;
            }

            //Too many
            if (s0.size() >= clTouchedTodo)
                break;
            remClTouched.push_back(it->index);

            Clause& cl = *clauses[it->index];
            //Nothing to do
            if (!cl.getStrenghtened() && !cl.getChanged()) {
                continue;
            }

            //Strengthened or changed, so it can subsume0 others
            if (!alreadyAdded[it->index]) {
                s0.push_back(*it);
                alreadyAdded[it->index] = 1;
            }

            //If not changed, then it cannot be subsumed
            if (!cl.getChanged())
                continue;

            //Look at POS occurs
            for (uint32_t j = 0; j < cl.size(); j++) {
                if (ol_seenPos[cl[j].var()])
                    continue;

                const Occur& occs = occur[cl[j].toInt()];
                *toDecrease -= occs.size();
                for (Occur::const_iterator it = occs.begin(), end = occs.end(); it != end; it++) {
                    //Too many
                    if (s0.size() >= clTouchedTodo)
                        break;

                    //Already added
                    if (!alreadyAdded[it->index]) {
                        s0.push_back(*it);
                        alreadyAdded[it->index] = 1;
                    }
                }
                ol_seenPos[cl[j].var()] = 1;
            }
        }
        //Remove clauses to be used from touched
        for (uint32_t i = 0; i < remClTouched.size(); i++) {
            subsumeWith.exclude(remClTouched[i]);
        }

        //Subsume 0
        for (vector<ClauseIndex>::const_iterator
            it = s0.begin(), end = s0.end()
            ; it != end
            ; ++it
        ) {
            //Has already been removed
            if (clauses[it->index] == NULL)
                continue;

            subsume0(*it, *clauses[it->index]);

            //Waaaay over time? Exit, don't bother about the consequences
            if (*toDecrease < -20L*1000L*1000L)
                break;
        }
    }
    //cout << "subsume0 done: " << numDone << endl;
    runStats.subsumeTime += cpuTime() - myTime;
}

/**
@brief Executes subsume1() recursively on all clauses
*/
bool Subsumer::performSubsume1()
{
    assert(solver->ok);

    vector<ClauseIndex> remClTouched; //These clauses will be untouched
    vector<ClauseIndex> s1;
    alreadyAdded.clear();
    alreadyAdded.resize(clauses.size(), 0);

    double myTime = cpuTime();
    toDecrease = &numMaxSubsume1;
    while(strengthenWith.nElems() > 0 && numMaxSubsume1 > 0) {
        s1.clear();
        remClTouched.clear();
        printLimits();

        for (CSet::iterator it = strengthenWith.begin(), end = strengthenWith.end(); it != end; ++it) {
            //Clause already removed
            if (it->index == std::numeric_limits< uint32_t >::max())
                continue;
            if (clauses[it->index] == NULL) {
                remClTouched.push_back(*it);
                continue;
            }

            //Too many
            if (s1.size() >= clTouchedTodo)
                break;
            remClTouched.push_back(*it);

            Clause& cl = *clauses[it->index];
            //Nothing to do
            if (!cl.getStrenghtened() && !cl.getChanged())
                continue;

            //Strengthened or changed, so it can subsume0&1 others
            if (!alreadyAdded[it->index]) {
                s1.push_back(*it);
                alreadyAdded[it->index] = 1;
            }
            cl.unsetStrenghtened();

            //If not changed, then it cannot be subsume1-ed
            if (!cl.getChanged())
                continue;
            cl.unsetChanged();

            for (uint32_t j = 0; j < cl.size(); j++) {
                //All clauses related to this have already been added
                if (ol_seenNeg[cl[j].var()])
                    continue;

                //Too many
                if (s1.size() >= clTouchedTodo)
                    break;

                //Look at POS occurs
                const Occur& occs2 = occur[cl[j].toInt()];
                *toDecrease -= occs2.size();
                for (Occur::const_iterator it = occs2.begin(), end = occs2.end(); it != end; it++) {
                    //Too many
                    if (s1.size() >= clTouchedTodo)
                        break;

                    /*//Cannot subsume/strenghten with larger clause
                    if (clauses[it->index].size > cl.size())
                        continue;*/

                    //Already added
                    if (!alreadyAdded[it->index]) {
                        s1.push_back(*it);
                        alreadyAdded[it->index] = 1;
                    }
                }

                const Occur& occs1 = occur[(~cl[j]).toInt()];
                *toDecrease -= occs1.size();
                for (Occur::const_iterator it = occs1.begin(), end = occs1.end(); it != end; it++) {
                    //Too many
                    if (s1.size() >= clTouchedTodo)
                        break;

                    /*//Cannot subsume/strenghten with larger clause
                    if (clauses[it->index].size > cl.size())
                        continue;*/

                    if (!alreadyAdded[it->index]) {
                        s1.push_back(*it);
                        alreadyAdded[it->index] = 1;
                    }
                }
                ol_seenNeg[cl[j].var()] = 1;
            }
        }

        //Remove clauses to be used from touched
        for (uint32_t i = 0; i < remClTouched.size(); i++) {
            strengthenWith.exclude(remClTouched[i]);
        }

        //Subsume 1
        for (vector<ClauseIndex>::const_iterator
            it = s1.begin(), end = s1.end()
            ; it != end
            ; ++it
        ) {
            //Has already been removed
            if (clauses[it->index] == NULL)
                continue;

            subsume1(*it, *clauses[it->index]);
            if (!solver->ok)
                return false;

            //Waaaay over time? Exit, don't bother about the consequences
            if (*toDecrease < -20L*1000L*1000L)
                break;
        }

        /*cout
        << " cl_touched: " << strengthenWith.nElems()
        << " numMaxSubume1: " << numMaxSubsume1 << endl;*/

    };
    runStats.strengthenTime += cpuTime() - myTime;

    return solver->ok;
}

/**
@brief Links in a clause into the occurrence lists and the clauses[]

@param[in] cl The clause to link in
*/
ClauseIndex Subsumer::linkInClause(Clause& cl)
{
    ClauseIndex c(clauses.size());
    clauses.push_back(&cl);
    clauseData.push_back(AbstData(cl, false));
    assert(clauseData.size() == clauses.size());

    std::sort(cl.begin(), cl.end());
    for (uint32_t i = 0; i < cl.size(); i++) {
        *toDecrease -= occur[cl[i].toInt()].size();
        occur[cl[i].toInt()].add(c);
        touchedVars.touch(cl[i], cl.learnt());

        if (cl.getChanged() || cl.getStrenghtened()) {
            ol_seenNeg[cl[i].var()] = 0;
            ol_seenPos[cl[i].var()] = 0;
        }
    }
    if (cl.getStrenghtened() || cl.getChanged()) {
        strengthenWith.add(c);
        subsumeWith.add(c);
    }

    return c;
}

/**
@brief Adds clauses from the solver to here, and removes them from the solver

Which clauses are needed can be controlled by the parameters

@param[in] cs The clause-set to use, e.g. solver->binaryClauses, solver->learnts
@param[in] alsoLearnt Also add learnt clauses?
@param[in] addBinAndAddToCL If set to FALSE, binary clauses are not added, and
clauses are never added to the cl_touched set.
*/
uint64_t Subsumer::addFromSolver(vector<Clause*>& cs)
{
    uint64_t numLitsAdded = 0;
    vector<Clause*>::iterator i = cs.begin();
    vector<Clause*>::iterator j = i;
    for (vector<Clause*>::iterator end = i + cs.size(); i !=  end; i++) {
        if (i+1 != end)
            __builtin_prefetch(*(i+1));

        linkInClause(**i);
        numLitsAdded += (*i)->size();
    }
    cs.resize(cs.size() - (i-j));

    return numLitsAdded;
}

/**
@brief Frees memory occupied by occurrence lists
*/
void Subsumer::freeMemory()
{
    for (uint32_t i = 0; i < occur.size(); i++) {
        occur[i].freeMem();
    }
}

/**
@brief Adds clauses from here, back to the solver
*/
void Subsumer::addBackToSolver()
{
    assert(solver->clauses.size() == 0);
    assert(solver->learnts.size() == 0);
    for (uint32_t i = 0; i < clauses.size(); i++) {
        //Clause has been removed
        if (clauses[i] == NULL)
            continue;

        //All clauses are larger than 2-long
        assert(clauses[i]->size() > 2);

        if (solver->ok) {
            //Go through each literal
            for (Clause::const_iterator
                it = clauses[i]->begin(), end = clauses[i]->end()
                ; it != end
                ; it++
            ) {
                assert(solver->varData[it->var()].elimed == ELIMED_NONE
                    || solver->varData[it->var()].elimed == ELIMED_QUEUED_VARREPLACER);
            }
        }

        //Add clause according to whether it's learnt or not
        if (clauses[i]->learnt())
            solver->learnts.push_back(clauses[i]);
        else
            solver->clauses.push_back(clauses[i]);
    }
}

void Subsumer::removeBinsAndTris(const Var var)
{
    uint32_t numRemovedLearnt = 0;

    Lit lit = Lit(var, false);

    numRemovedLearnt += removeBinAndTrisHelper(lit, solver->watches[(~lit).toInt()]);
    numRemovedLearnt += removeBinAndTrisHelper(~lit, solver->watches[lit.toInt()]);

    solver->learntsLits -= numRemovedLearnt*2;
    solver->numBinsLearnt -= numRemovedLearnt;
}

uint32_t Subsumer::removeBinAndTrisHelper(const Lit lit, vec<Watched>& ws)
{
    uint32_t numRemovedLearnt = 0;

    Watched* i = ws.begin();
    Watched* j = i;
    for (Watched *end = ws.end(); i != end; i++) {
        if (i->isTriClause()) continue;

        if (i->isBinary()) {
            assert(i->getLearnt());
            removeWBin(solver->watches, i->getOtherLit(), lit, i->getLearnt());
            numRemovedLearnt++;
            continue;
        }

        assert(false);
    }
    ws.shrink_(i - j);

    return numRemovedLearnt;
}

void Subsumer::removeWrongBins()
{
    uint32_t numRemovedHalfLearnt = 0;
    uint32_t wsLit = 0;
    for (vector<vec<Watched> >::iterator
        it = solver->watches.begin(), end = solver->watches.end()
        ; it != end
        ; it++, wsLit++
    ) {
        Lit lit = ~Lit::toLit(wsLit);
        vec<Watched>& ws = *it;

        vec<Watched>::iterator i = ws.begin();
        vec<Watched>::iterator j = i;
        for (vec<Watched>::iterator end2 = ws.end(); i != end2; i++) {
            if (i->isBinary()
                && (var_elimed[lit.var()] || var_elimed[i->getOtherLit().var()])
            ) {
                #ifdef VERBOSE_DEBUG_VARELIM
                if (i->getLearnt()) {
                    cout
                    << "c ERROR! Binary " << lit << " , " << i->getOtherLit()
                    << " is still in, but one of its vars has been eliminated"
                    << endl;
                }
                #endif
                assert(i->getLearnt());
                numRemovedHalfLearnt++;
            } else {
                *j++ = *i;
            }
        }
        ws.shrink_(i - j);
    }

    assert(numRemovedHalfLearnt % 2 == 0);
    solver->learntsLits -= numRemovedHalfLearnt;
    solver->numBinsLearnt -= numRemovedHalfLearnt/2;
    runStats.binLearntClRemThroughElim += numRemovedHalfLearnt/2;
}

void Subsumer::removeAllTris()
{
    for (vector<vec<Watched> >::iterator
        it = solver->watches.begin(), end = solver->watches.end()
        ; it != end
        ; it++
    ) {
        vec<Watched>& ws = *it;

        vec<Watched>::iterator i = ws.begin();
        vec<Watched>::iterator j = i;
        for (vec<Watched>::iterator end2 = ws.end(); i != end2; i++) {
            if (i->isTriClause()) {
                continue;
            } else {
                *j++ = *i;
            }
        }
        ws.shrink_(i - j);
    }
}

/**
@brief Clears and deletes (almost) everything in this class

Clears touchlists, occurrance lists, clauses, and variable touched lists
*/
void Subsumer::clearAll()
{
    touchedVars.clear();
    clauses.clear();
    addedClauseLits = 0;
    for (Var var = 0; var < solver->nVars(); var++) {
        occur[2*var].clear();
        occur[2*var+1].clear();
        ol_seenNeg[var] = 1;
        ol_seenPos[var] = 1;
    }
    clauseData.clear();
    strengthenWith.clear();
    subsumeWith.clear();
    runStats.clear();
}

bool Subsumer::eliminateVars()
{
    double myTime = cpuTime();
    uint32_t vars_elimed = 0;
    uint32_t numtry = 0;
    toDecrease = &numMaxElim;
    orderVarsForElimInit();

    #ifdef BIT_MORE_VERBOSITY
    cout << "c #order size:" << order.size() << endl;
    #endif

    //Go through the ordered list of variables to eliminate
    while(!varElimOrder.empty()
        && numMaxElim > 0
        && numMaxElimVars > 0
    ) {
        Var var = varElimOrder.removeMin();

        //Can this variable be eliminated at all?
        if (solver->value(var) != l_Undef
            || solver->varData[var].elimed != ELIMED_NONE
            || !gateFinder->canElim(var)
        ) {
            continue;
        }

        //Try to eliminate
        numtry++;
        if (maybeEliminate(var)) {
            vars_elimed++;
            numMaxElimVars--;
        }

        //During elimination, we reached UNSAT, finish
        if (!solver->ok)
            goto end;
    }

    #ifdef BIT_MORE_VERBOSITY
    cout << "c  #try to eliminate: " << numtry << endl;
    cout << "c  #var-elim: " << vars_elimed << endl;
    #endif
end:
    runStats.varElimTime += cpuTime() - myTime;

    return solver->ok;
}

void Subsumer::subsumeBinsWithBins()
{
    const double myTime = cpuTime();
    uint64_t numBinsBefore = solver->numBinsLearnt + solver->numBinsNonLearnt;
    toDecrease = &numMaxSubsume0;

    uint32_t wsLit = 0;
    for (vector<vec<Watched> >::iterator
        it = solver->watches.begin(), end = solver->watches.end()
        ; it != end
        ; it++, wsLit++
    ) {
        vec<Watched>& ws = *it;
        Lit lit = ~Lit::toLit(wsLit);
        if (ws.size() < 2) continue;

        std::sort(ws.begin(), ws.end(), BinSorter());
        *toDecrease -= ws.size();

        vec<Watched>::iterator i = ws.begin();
        vec<Watched>::iterator j = i;

        Lit lastLit = lit_Undef;
        bool lastLearnt = false;
        for (vec<Watched>::iterator end = ws.end(); i != end; i++) {
            if (!i->isBinary()) {
                *j++ = *i;
                continue;
            }
            if (i->getOtherLit() == lastLit) {
                //The sorting algorithm prefers non-learnt to learnt, so it is
                //impossible to have non-learnt before learnt
                assert(!(i->getLearnt() == false && lastLearnt == true));

                assert(i->getOtherLit().var() != lit.var());
                removeWBin(solver->watches, i->getOtherLit(), lit, i->getLearnt());
                if (i->getLearnt()) {
                    solver->learntsLits -= 2;
                    solver->numBinsLearnt--;
                } else {
                    solver->clausesLits -= 2;
                    solver->numBinsNonLearnt--;
                    touchedVars.touch(lit, i->getLearnt());
                    touchedVars.touch(i->getOtherLit(), i->getLearnt());
                }
            } else {
                lastLit = i->getOtherLit();
                lastLearnt = i->getLearnt();
                *j++ = *i;
            }
        }
        ws.shrink_(i-j);
    }

    if (solver->conf.verbosity  >= 1) {
        cout
        << "c bin-w-bin subsume "
        << "rem " << std::setw(10)
        << (numBinsBefore - solver->numBinsLearnt - solver->numBinsNonLearnt)

        << " time: " << std::fixed << std::setprecision(2) << std::setw(5)
        << (cpuTime() - myTime)
        << " s" << endl;
    }

    //Update global stats
    runStats.subsBinWithBinTime += cpuTime() - myTime;
    runStats.subsBinWithBin += (numBinsBefore - solver->numBinsLearnt - solver->numBinsNonLearnt);
}

bool Subsumer::propagate()
{
    assert(solver->ok);

    while (solver->qhead < solver->trail.size()) {
        Lit p = solver->trail[solver->qhead];
        solver->qhead++;
        Occur& ws = occur[(~p).toInt()];

        //Go through each occurrence list
        for (Occur::const_iterator
            it = ws.begin(), end = ws.end()
            ; it != end
            ; it++
        ) {
            const Clause& cl = *clauses[it->index];
            Lit lastUndef = lit_Undef;
            uint32_t numUndef = 0;
            bool satisfied = false;
            for (uint32_t i = 0; i < cl.size(); i++) {
                const lbool val = solver->value(cl[i]);
                if (val == l_True) {
                    satisfied = true;
                    break;
                }
                if (val == l_Undef) {
                    numUndef++;
                    if (numUndef > 1) break;
                    lastUndef = cl[i];
                }
            }

            //Satisfied
            if (satisfied)
                continue;

            //UNSAT
            if (numUndef == 0) {
                solver->ok = false;
                return false;
            }

            //Propagation
            if (numUndef == 1) {
                solver->enqueue(lastUndef);

                //Update stats
                if (cl.size() == 3)
                    solver->propStats.propsTri++;
                else {
                    if (cl.learnt())
                        solver->propStats.propsLongRed++;
                    else
                        solver->propStats.propsLongIrred++;
                }
            }
        }

        //Propagate binary clauses
        vec<Watched>& ws2 = solver->watches[p.toInt()];
        for (vec<Watched>::const_iterator
            it = ws2.begin(), end = ws2.end()
            ; it != end
            ; it++
        ) {
            //Only binary clauses
            if (!it->isBinary())
                continue;

            const lbool val = solver->value(it->getOtherLit());

            //UNSAT
            if (val == l_False) {
                solver->ok = false;
                return false;
            }

            //Propagation
            if (val == l_Undef) {
                solver->enqueue(it->getOtherLit());
                if (it->getLearnt())
                    solver->propStats.propsBinRed++;
                else
                    solver->propStats.propsBinIrred++;
            }
        }
        //cleanLitOfClauses(p);
    }

    return true;
}

bool Subsumer::loopSubsumeVarelim()
{
    assert(solver->ok);

    //Subsume, strengthen, and var-elim until time-out/limit-reached or fixedpoint
    const size_t origTrailSize = solver->trail.size();
    do {
        //Carry out subsume0
        performSubsume0();

        //Carry out strengthening
        if (!performSubsume1())
            goto end;

        //If no var elimination is needed, this IS fixedpoint
        if (!solver->conf.doVarElim)
            break;

        //Eliminate variables
        if (!eliminateVars())
            goto end;

        //Clean clauses as much as possible
        solver->clauseCleaner->removeSatisfiedBins();
    } while (
        (subsumeWith.nElems() > 0 && numMaxSubsume0 > 0)
        || (strengthenWith.nElems() > 0 && numMaxSubsume1 > 0)
        || (touchedVars.size() > 0 && numMaxElim > 0)
    );
    printLimits();

    assert(solver->ok);
    assert(verifyIntegrity());

    //remove 2-longs that contain eliminated variables
    removeWrongBins();

    //if variable got assigned in the meantime, uneliminate/unblock corresponding clauses
    removeAssignedVarsFromEliminated();

end:
    //Update global stats
    runStats.zeroDepthAssings = solver->trail.size() - origTrailSize;

    return solver->ok;
}

/**
@brief Main function in this class

Performs, recursively:
* backward-subsumption
* self-subsuming resolution
* variable elimination

*/
bool Subsumer::simplifyBySubsumption()
{
    //Test & debug
    solver->testAllClauseAttach();
    solver->checkNoWrongAttach();
    assert(solver->varReplacer->getNewToReplaceVars() == 0
            && "Cannot work in an environment when elimnated vars could be replaced by other vars");

    //Clean the clauses before playing with them
    solver->clauseCleaner->removeAndCleanAll();

    //If too many clauses, don't do it
    if (solver->getNumLongClauses() > 10000000UL
        || solver->clausesLits > 50000000UL
    )  return true;

    //Setup
    double myTime = cpuTime();
    clearAll();
    numCalls++;

    //touch all variables
    for (Var var = 0; var < solver->nVars(); var++) {
        if (solver->decision_var[var] && solver->assigns[var] == l_Undef)
            touchedVars.touch(var);
    }

    //Reserve space for clauses
    const uint32_t expected_size = solver->clauses.size() + solver->learnts.size();
    clauses.reserve(expected_size);
    strengthenWith.reserve(expected_size);
    subsumeWith.reserve(expected_size);

    //Detach all non-bins and non-tris, i.e. every long clause
    CompleteDetachReatacher reattacher(solver);
    reattacher.detachNonBinsNonTris(false);

    //Add non-learnt and learnt clauses to occur lists, touch lists, etc.
    toDecrease = &numMaxSubsume1;
    if (solver->clauses.size() < 10000000)
        std::sort(solver->clauses.begin(), solver->clauses.end(), sortBySize());
    runStats.origNumIrredLongClauses = solver->clauses.size();
    addedClauseLits += addFromSolver(solver->clauses);

    if (solver->learnts.size() < 300000)
        std::sort(solver->learnts.begin(), solver->learnts.end(), sortBySize());
    runStats.origNumRedLongClauses = solver->learnts.size();
    addedClauseLits += addFromSolver(solver->learnts);
    runStats.origNumFreeVars = solver->getNumFreeVars();
    setLimits();

    //Print link-in and startup time
    double linkInTime = cpuTime() - myTime;
    runStats.linkInTime += linkInTime;

    //Do stuff with binaries
    subsumeBinsWithBins();
    printLimits();

    #ifdef DEBUG_VAR_ELIM
    checkForElimedVars();
    #endif

    //XOR-finding
    if (solver->conf.doFindXors
        && !xorFinder->findXors()
    ) {
        goto end;
    }

    //Gate-finding
    if (solver->conf.doCache && solver->conf.doGateFind) {
        if (!gateFinder->doAll())
            goto end;
    }

    //Do asymtotic tautology elimination
    if (solver->conf.doBlockedClause)
        blockClauses();
    if (solver->conf.doAsymmTE)
        asymmTE();

    //Do subsumption & var-elim in loop
    if (!loopSubsumeVarelim())
        goto end;

end:
    myTime = cpuTime();

    //Remove tri-clauses
    removeAllTris();

    //Add back clauses to solver
    addBackToSolver();

    if (solver->ok) {
        //Reattach and clean
        reattacher.reattachNonBins();
    }

    //Update global stats
    runStats.finalCleanupTime += cpuTime() - myTime;
    globalStats += runStats;

    if (solver->ok)
        checkElimedUnassignedAndStats();

    //Print stats
    if (solver->conf.verbosity >= 1) {
        if (solver->conf.verbosity >= 3)
            runStats.print(solver->nVars());
        else
            runStats.printShort();
    }

    //Sanity checks
    solver->testAllClauseAttach();
    solver->checkNoWrongAttach();
    return solver->ok;
}

void Subsumer::checkForElimedVars()
{
    //First, sanity-check the long clauses
    for (size_t i = 0; i < clauses.size(); i++) {
        if (clauses[i] == NULL)
            continue;

        const Clause& cl = *clauses[i];
        for (uint32_t i = 0; i < cl.size(); i++) {
            if (var_elimed[cl[i].var()]) {
                cout
                << "Error: elmied var -- Lit " << cl[i] << " in clause"
                << endl
                << "wrongly left in clause: " << cl
                << endl;

                exit(-1);
            }
        }
    }

    //Then, sanity-check the binary clauses
    uint32_t wsLit = 0;
    for (vector<vec<Watched> >::const_iterator
        it = solver->watches.begin(), end = solver->watches.end()
        ; it != end
        ; it++, wsLit++
    ) {
        Lit lit = ~Lit::toLit(wsLit);
        const vec<Watched>& ws = *it;
        for (vec<Watched>::const_iterator it2 = ws.begin(), end2 = ws.end(); it2 != end2; it2++) {
            if (it2->isBinary()) {
                if (var_elimed[lit.var()] || var_elimed[it2->getOtherLit().var()]) {
                    cout
                    << "Error: A var is elimed in a binary clause: "
                    << lit << " , " << it2->getOtherLit()
                    << endl;

                    exit(-1);
                }
            }
        }
    }
}

/*const bool Subsumer::mixXorAndGates()
{
    assert(solver->ok);
    uint32_t fixed = 0;
    uint32_t ored = 0;
    double myTime = cpuTime();
    uint32_t oldTrailSize = solver->trail.size();
    vector<Lit> lits;
    vector<Lit> tmp;

    uint32_t index = 0;
    for (vector<Xor>::iterator it = xors.begin(), end = xors.end(); it != end; it++, index++) {
        const Xor& thisXor = *it;
        if (thisXor.vars.size() != 3) continue;

        for (uint32_t i = 0; i < thisXor.vars.size(); i++) {
            seen[thisXor.vars[i]] = true;
        }

//         for (uint32_t i = 0; i < thisXor.vars.size(); i++) {
//             Var var = thisXor.vars[i];
//             const vector<uint32_t>& occ1 = gateOccEq[Lit(var, true).toInt()];
//             for (vector<uint32_t>::const_iterator it = occ1.begin(), end = occ1.end(); it != end; it++) {
//                 const OrGate& orGate = orGates[*it];
//                 uint32_t OK = 0;
//                 for (uint32_t i2 = 0; i2 < orGate.lits.size(); i2++) {
//                     if (orGate.lits[i2].sign() &&
//                         seen[orGate.lits[i2].var()]) OK++;
//                 }
//                 if (OK>1) {
//                     cout << "XOR to look at:" << thisXor << endl;
//                     cout << "gate to look at : " << orGate << endl;
//                     cout << "---------------" << endl;
//                 }
//             }
//         }

        for (uint32_t i = 0; i < thisXor.vars.size(); i++) {
            Var var = thisXor.vars[i];
            Lit eqLit = Lit(var, true);
            const vector<uint32_t>& occ = gateOccEq[eqLit.toInt()];
            for (vector<uint32_t>::const_iterator it = occ.begin(), end = occ.end(); it != end; it++) {
                const OrGate& orGate = orGates[*it];
                assert(orGate.eqLit == eqLit);
                uint32_t OK = 0;
                lits.clear();
                bool sign = false;
                for (uint32_t i2 = 0; i2 < orGate.lits.size(); i2++) {
                    if (seen[orGate.lits[i2].var()]) {
                        OK++;
                        lits.push_back(orGate.lits[i2]  ^ true);
                        sign ^= !orGate.lits[i2].sign();
                    }
                }
                if (OK == 2) {
                    #ifdef VERBOSE_XORGATE_MIX
                    cout << "XOR to look at:" << thisXor << endl;
                    cout << "gate to look at : " << orGate << endl;
                    #endif

                    if (!thisXor.rhs^sign) {
                        fixed++;
                        tmp.clear();
                        tmp.push_back(~lits[0]);
                        #ifdef VERBOSE_XORGATE_MIX
                        cout << "setting: " << tmp[0] << endl;
                        #endif
                        solver->addClauseInt(tmp);
                        if (!solver->ok) goto end;

                        tmp.clear();
                        tmp.push_back(~lits[1]);
                        #ifdef VERBOSE_XORGATE_MIX
                        cout << "setting: " << tmp[0] << endl;
                        #endif
                        solver->addClauseInt(tmp);
                        if (!solver->ok) goto end;
                    } else {
                        ored++;
                        tmp.clear();
                        tmp.push_back(lits[0]);
                        tmp.push_back(lits[1]);
                        #ifdef VERBOSE_XORGATE_MIX
                        cout << "orIng: " << tmp << endl;
                        #endif
                        Clause* c = solver->addClauseInt(tmp, true);
                        assert(c == NULL);
                        if (!solver->ok) goto end;
                    }

                    #ifdef VERBOSE_XORGATE_MIX
                    cout << "---------------" << endl;
                    #endif
                }
            }
        }

        end:
        for (uint32_t i = 0; i < thisXor.vars.size(); i++) {
            seen[thisXor.vars[i]] = false;
        }
        if (!solver->ok) break;
    }

    if (solver->conf.verbosity >= 1) {
        cout << "c OrXorMix"
        << " Or: " << std::setw(6) << ored
        << " Fix: " << std::setw(6) << fixed
        << " Fixed: " << std::setw(4) << (solver->trail.size() - oldTrailSize)
        << " T: " << std::setprecision(2) << std::setw(5) << (cpuTime() - myTime) << " s"
        << endl;
    }

    return solver->ok;
}*/

void Subsumer::blockClauses()
{
    const double myTime = cpuTime();
    size_t blocked = 0;
    size_t blockedLits = 0;
    size_t wenThrough = 0;
    size_t index = clauses.size()-1;
    toDecrease = &numMaxBlocked;
    for (vector<Clause*>::reverse_iterator
        it = clauses.rbegin()
        , end = clauses.rend()
        ; it != end
        ; it++, index--
    ) {
        //Already removed
        if (*it == NULL)
            continue;

        //Ran out of time
        if (*toDecrease < 0) {
            break;
        }

        Clause& cl = **it;
        if ((**it).learnt())
            continue;

        wenThrough++;

        //Fill up temps
        bool toRemove = false;
        *toDecrease -= cl.size();
        for (const Lit *l = cl.begin(), *end = cl.end(); l != end; l++) {
            seen[l->toInt()] = 1;
        }

        //Blocked clause elimination
        for (const Lit* l = cl.begin(), *end = cl.end(); l != end; l++) {
            if (solver->varData[l->var()].elimed != ELIMED_NONE)
                continue;

            if (allTautologySlim(*l)) {
                vector<Lit> remCl(cl.size());
                std::copy(cl.begin(), cl.end(), remCl.begin());
                blockedClauses.push_back(BlockedClause(*l, remCl));

                blocked++;
                blockedLits += cl.size();
                toRemove = true;
                break;
            }
        }

        //Clear seen
        for (Clause::const_iterator l = cl.begin(), end = cl.end(); l != end; l++) {
            seen[l->toInt()] = 0;
        }

        if (toRemove) {
            unlinkClause(index);
        }
    }

    if (solver->conf.verbosity >= 1) {
        cout
        << "c blocking"
        << " through: " << wenThrough
        << " blocked: " << blocked
        << " T : " << std::fixed << std::setprecision(2) << std::setw(6) << (cpuTime() - myTime)
        << endl;
    }
    runStats.blocked += blocked;
    runStats.blockedSumLits += blockedLits;
    runStats.blockTime += cpuTime() - myTime;
}

void Subsumer::asymmTE()
{
    const double myTime = cpuTime();
    uint32_t blocked = 0;
    size_t blockedLits = 0;
    uint32_t asymmSubsumed = 0;
    uint32_t removed = 0;

    vector<Lit> tmpCl;
    uint32_t index = clauses.size()-1;
    for (vector<Clause*>::reverse_iterator it = clauses.rbegin(), end = clauses.rend(); it != end; it++, index--) {
        //Already removed
        if (*it == NULL)
            continue;

        //Ran out of time
        if (*toDecrease < 0) {
            break;
        }

        Clause& cl = **it;
        toDecrease = &numMaxAsymm;
        *toDecrease -= cl.size()*2;

        //Fill tmpCl, seen
        tmpCl.clear();
        for (const Lit *l = cl.begin(), *end = cl.end(); l != end; l++) {
            seen[l->toInt()] = true;
            tmpCl.push_back(*l);
        }

        //add to tmpCl literals that could be added through reverse strengthening
        //ONLY non-learnt
        for (const Lit *l = cl.begin(), *end = cl.end(); l != end; l++) {
            const vector<LitExtra>& cache = solver->implCache[l->toInt()].lits;
            *toDecrease -= cache.size();
            for (vector<LitExtra>::const_iterator cacheLit = cache.begin(), endCache = cache.end(); cacheLit != endCache; cacheLit++) {
                if (cacheLit->getOnlyNLBin()
                    && !seen[(~cacheLit->getLit()).toInt()]
                ) {
                    const Lit toAdd = ~(cacheLit->getLit());
                    tmpCl.push_back(toAdd);
                    seen[toAdd.toInt()] = true;
                }
            }
        }

        //subsumption with binary clauses
        bool toRemove = false;
        if (solver->conf.doExtBinSubs) {
            //for (vector<Lit>::const_iterator l = tmpCl.begin(), end = tmpCl.end(); l != end; l++) {
            for (const Lit* l = cl.begin(), *end = cl.end(); l != end; l++) {
                const vector<LitExtra>& cache = solver->implCache[l->toInt()].lits;
                *toDecrease -= cache.size();
                for (vector<LitExtra>::const_iterator cacheLit = cache.begin(), endCache = cache.end(); cacheLit != endCache; cacheLit++) {
                    if ((cacheLit->getOnlyNLBin() || (**it).learnt()) //subsume non-learnt with non-learnt
                        && seen[cacheLit->getLit().toInt()]
                    ) {
                        toRemove = true;
                        asymmSubsumed++;
                        #ifdef VERBOSE_DEBUG_ASYMTE
                        cout << "c AsymLitAdd removing: " << cl << endl;
                        #endif
                        goto next;
                    }
                }
            }
        }

        if ((**it).learnt())
            goto next;

        //Blocked clause elimination
        if (solver->conf.doBlockedClause && numMaxBlocked > 0) {
            toDecrease = &numMaxBlocked;
            for (const Lit* l = cl.begin(), *end = cl.end(); l != end; l++) {
                if (solver->varData[l->var()].elimed != ELIMED_NONE)
                    continue;

                if (allTautologySlim(*l)) {
                    vector<Lit> remCl(cl.size());
                    std::copy(cl.begin(), cl.end(), remCl.begin());
                    blockedClauses.push_back(BlockedClause(*l, remCl));

                    blocked++;
                    blockedLits += cl.size();
                    toRemove = true;
                    toDecrease = &numMaxAsymm;
                    goto next;
                }
            }
        }
        toDecrease = &numMaxAsymm;

        /*
        //subsumption with non-learnt larger clauses
        CL_ABST_TYPE abst;
        abst = calcAbstraction(tmpCl);
        *toDecrease -= tmpCl.size()*2;
        for (vector<Lit>::const_iterator it = tmpCl.begin(), end = tmpCl.end(); it != end; it++) {
            const Occur& occ = occur[it->toInt()];
            *toDecrease -= occ.size();
            for (Occur::const_iterator it2 = occ.begin(), end2 = occ.end(); it2 != end2; it2++) {
                if (it2->index != index
                    && subsetAbst(clauseData[it2->index].abst, abst)
                    && clauses[it2->index] != NULL
                    && !clauses[it2->index]->learnt()
                    && subsetReverse(*clauses[it2->index])
                )  {
                    #ifdef VERBOSE_DEBUG_ASYMTE
                    cout << "c AsymTE removing: " << cl << " -- subsumed by cl: " << *clauses[it2->index] << endl;
                    #endif
                    toRemove = true;
                    goto next;
                }
            }
        }*/

        next:
        if (toRemove) {
            unlinkClause(index);
            removed++;
        }

        //Clear seen
        for (vector<Lit>::const_iterator l = tmpCl.begin(), end = tmpCl.end(); l != end; l++) {
            seen[l->toInt()] = false;
        }
    }

    if (solver->conf.verbosity >= 1) {
        cout << "c AsymmTElim"
        << " asymm subsumed: " << asymmSubsumed
        << " blocked: " << blocked
        << " T : " << std::fixed << std::setprecision(2) << std::setw(6) << (cpuTime() - myTime)
        << endl;
    }
    runStats.asymmSubs += asymmSubsumed;
    runStats.blocked += blocked;
    runStats.blockedSumLits += blockedLits;
    runStats.asymmTime += cpuTime() - myTime;
}

/**
@brief Calculate limits for backw-subsumption, var elim, etc.

It is important to have limits, otherwise the time taken to perfom these tasks
could be huge. Furthermore, it seems that there is a benefit in doing these
simplifications slowly, instead of trying to use them as much as possible
from the beginning.
*/
void Subsumer::setLimits()
{
    numMaxSubsume0 = 170L*1000L*1000L;
    numMaxSubsume1 = 80L*1000L*1000L;
    numMaxElim     = 400L*1000L*1000L;
    numMaxAsymm    = 80L *1000L*1000L;
    numMaxBlocked  = 400L *1000L*1000L;
    numMaxVarElimAgressiveCheck  = 400L *1000L*1000L;

    //numMaxElim = 0;
    //numMaxElim = std::numeric_limits<int64_t>::max();

    #ifdef BIT_MORE_VERBOSITY
    cout << "c addedClauseLits: " << addedClauseLits << endl;
    #endif
    if (addedClauseLits < 10000000) {
        numMaxElim *= 2;
        numMaxSubsume0 *= 2;
        numMaxSubsume1 *= 2;
    }

    if (addedClauseLits < 3000000) {
        numMaxElim *= 2;
        numMaxSubsume0 *= 2;
        numMaxSubsume1 *= 2;
    }

    numMaxElimVars = ((double)solver->getNumFreeVars() * solver->conf.varElimRatioPerIter);
    runStats.origNumMaxElimVars = numMaxElimVars;

    if (!solver->conf.doSubsume1) {
        numMaxSubsume1 = 0;
    }

    clTouchedTodo = 2000;
    if (addedClauseLits > 3000000) clTouchedTodo /= 2;
    if (addedClauseLits > 10000000) clTouchedTodo /= 2;

    //For debugging

    //numMaxSubsume0 = 0;
    //numMaxSubsume1 = 0;
    //numMaxElimVars = 0;
    //numMaxElim = 0;
    //numMaxSubsume0 = std::numeric_limits<int64_t>::max();
    //numMaxSubsume1 = std::numeric_limits<int64_t>::max();
    //numMaxElimVars = std::numeric_limits<int32_t>::max();
    //numMaxElim     = std::numeric_limits<int64_t>::max();
}

/**
@brief Remove variables from var_elimed if it has been set

While doing, e.g. self-subsuming resolution, it might happen that the variable
that we JUST eliminated has been assigned a value. This could happen for example
if due to clause-cleaning some variable value got propagated that we just set.
Therefore, we must check at the very end if any variables that we eliminated
got set, and if so, the clauses linked to these variables can be fully removed
from elimedOutVar[].
*/
void Subsumer::removeAssignedVarsFromEliminated()
{
    vector<BlockedClause>::iterator i = blockedClauses.begin();
    vector<BlockedClause>::iterator j = blockedClauses.begin();

    for (vector<BlockedClause>::iterator end = blockedClauses.end(); i != end; i++) {
        if (solver->value(i->blockedOn) != l_Undef) {
            const Var var = i->blockedOn.var();
            if (solver->varData[var].elimed == ELIMED_VARELIM) {
                var_elimed[var] = false;
                solver->varData[var].elimed = ELIMED_NONE;
                solver->setDecisionVar(var);
                runStats.numVarsElimed--;
            }
        } else {
            *j++ = *i;
        }
    }
    blockedClauses.resize(blockedClauses.size()-(i-j));
}

/**
@brief Checks if clauses are subsumed or could be strenghtened with given clause

Checks if:
* any clause is subsumed with given clause
* the given clause could perform self-subsuming resolution on any other clause

Only takes into consideration clauses that are in the occurrence lists.

@param[in] ps The clause to perform the above listed algos with
@param[in] abs The abstraction of clause ps
@param[out] out_subsumed The clauses that could be modified by ps
@param[out] out_lits Defines HOW these clauses could be modified. By removing
literal, or by subsumption (in this case, there is lit_Undef here)
*/
template<class T>
void Subsumer::findSubsumed1(
    uint32_t index
    , const T& ps
    , const CL_ABST_TYPE abs
    , vector<ClauseIndex>& out_subsumed
    , vector<Lit>& out_lits
)
{
    #ifdef VERBOSE_DEBUG
    cout << "findSubsumed1: " << ps << endl;
    #endif

    Var minVar = var_Undef;
    uint32_t bestSize = std::numeric_limits<uint32_t>::max();
    for (uint32_t i = 0; i < ps.size(); i++){
        uint32_t newSize = occur[ps[i].toInt()].size()+ occur[(~ps[i]).toInt()].size();
        if (newSize < bestSize) {
            minVar = ps[i].var();
            bestSize = newSize;
        }
    }
    assert(minVar != var_Undef);
    *toDecrease -= ps.size();

    fillSubs(ps, index, abs, out_subsumed, out_lits, Lit(minVar, true));
    fillSubs(ps, index, abs, out_subsumed, out_lits, Lit(minVar, false));
}

/**
@brief Helper function for findSubsumed1

Used to avoid duplication of code
*/
template<class T>
void inline Subsumer::fillSubs(const T& ps, const uint32_t index, const CL_ABST_TYPE abs, vector<ClauseIndex>& out_subsumed, vector<Lit>& out_lits, const Lit lit)
{
    Lit litSub;
    const Occur& cs = occur[lit.toInt()];
    *toDecrease -= cs.size()*15 + 40;
    for (Occur::const_iterator it = cs.begin(), end = cs.end(); it != end; it++) {
        if (it->index != index
            && subsetAbst(abs, clauseData[it->index].abst)
            && ps.size() <= clauseData[it->index].size
        ) {
            *toDecrease -= ps.size() + clauseData[it->index].size;
            litSub = subset1(ps, *clauses[it->index]);
            if (litSub != lit_Error) {
                out_subsumed.push_back(*it);
                out_lits.push_back(litSub);
                #ifdef VERBOSE_DEBUG
                if (litSub == lit_Undef) cout << "subsume0-d: ";
                else cout << "subsume1-ed (lit: " << litSub << "): " << *clauses[it->index] << endl;
                #endif
            }
        }
    }
}


void Subsumer::removeClausesHelper(
    vector<ClAndBin>& todo
    , const Lit lit
) {
    for (uint32_t i = 0; i < todo.size(); i++) {
        ClAndBin& c = todo[i];

        #ifdef VERBOSE_DEBUG_VARELIM
        cout << "Removing clause due to var-elim on " << lit << " : ";
        #endif
        if (!c.isBin) {
            assert(clauses[c.clsimp.index] != NULL);
            #ifdef VERBOSE_DEBUG_VARELIM
            cout << *clauses[c.clsimp.index] << endl;
            #endif

            //Update stats
            if (clauses[c.clsimp.index]->learnt()) {
                runStats.longLearntClRemThroughElim++;
            } else {
                runStats.clauses_elimed_long++;
                runStats.clauses_elimed_sumsize += clauses[c.clsimp.index]->size();
            }

            unlinkClause(c.clsimp, lit);

        } else {
            #ifdef VERBOSE_DEBUG_VARELIM
            cout << c.lit1 << " , " << c.lit2 << endl;
            #endif

            //Remove binary clause
            assert(lit == c.lit1 || lit == c.lit2);
            removeWBin(solver->watches, c.lit1, c.lit2, c.learnt);
            removeWBin(solver->watches, c.lit2, c.lit1, c.learnt);

            //Update stats
            if (!c.learnt) {
                solver->clausesLits -= 2;
                runStats.clauses_elimed_bin++;
                runStats.clauses_elimed_sumsize += 2;
                solver->numBinsNonLearnt--;
            } else {
                solver->learntsLits -= 2;
                runStats.binLearntClRemThroughElim++;
                solver->numBinsLearnt--;
            }

            //Put clause into blocked status
            if (!c.learnt) {
                vector<Lit> lits;
                lits.push_back(c.lit1);
                lits.push_back(c.lit2);
                blockedClauses.push_back(BlockedClause(lit, lits));
            }

            //Touch literals
            touchedVars.touch(c.lit1, false);
            touchedVars.touch(c.lit2, false);
        }
    }
}

/**
@brief Used for variable elimination

Migrates clauses in poss to ps, and negs to ns
Also unlinks ass clauses is ps and ns. This is special unlinking, since it
actually saves the clauses for later re-use when extending the model, or re-
introducing the eliminated variables.

@param[in] poss The occurrence list of var where it is positive
@param[in] negs The occurrence list of var where it is negavite
@param[out] ps Where thre clauses from poss have been moved
@param[out] ns Where thre clauses from negs have been moved
@param[in] var The variable that is being eliminated
*/
void Subsumer::removeClauses(const Var var)
{
    removeClausesHelper(posAll, Lit(var, false));
    removeClausesHelper(negAll, Lit(var, true));
}

uint32_t Subsumer::numNonLearntBins(const Lit lit) const
{
    uint32_t num = 0;
    const vec<Watched>& ws = solver->watches[(~lit).toInt()];
    for (vec<Watched>::const_iterator it = ws.begin(), end = ws.end(); it != end; it++) {
        if (it->isBinary() && !it->getLearnt()) num++;
    }

    return num;
}

void Subsumer::fillClAndBin(vector<ClAndBin>& all, const Occur& cs, const Lit lit)
{
    for (Occur::const_iterator it = cs.begin(), end = cs.end(); it != end; it++) {
        if (it->index != std::numeric_limits<uint32_t>::max())
            all.push_back(ClAndBin(*it, clauses[it->index]->learnt()));
    }

    const vec<Watched>& ws = solver->watches[(~lit).toInt()];
    for (vec<Watched>::const_iterator it = ws.begin(), end = ws.end(); it != end; it++) {
        if (it->isBinary())
            all.push_back(ClAndBin(lit, it->getOtherLit(), it->getLearnt()));
    }
}

int Subsumer::testVarElim(Var var)
{
    assert(solver->ok);
    assert(!var_elimed[var]);
    assert(solver->varData[var].elimed == ELIMED_NONE);
    assert(solver->decision_var[var]);
    assert(solver->value(var) == l_Undef);
    const bool agressiveCheck = (numMaxVarElimAgressiveCheck > 0);

    //set-up
    const Lit lit = Lit(var, false);
    Occur& poss = occur[lit.toInt()];
    Occur& negs = occur[(~lit).toInt()];

    //Count statistic to help in doing heuristic cut-offs
    const uint32_t numNonLearntPos = numNonLearntBins(lit);
    const uint32_t numNonLearntNeg = numNonLearntBins(~lit);
    uint32_t before_3long = 0;
    uint32_t before_long = 0;
    int before_literals = numNonLearntNeg*2 + numNonLearntPos*2;

    //stats on positive
    uint32_t posSize = 0;
    for (Occur::const_iterator it = poss.begin(), end = poss.end(); it != end; it++)
        if (!clauses[it->index]->learnt()) {
            posSize++;
            before_literals += clauses[it->index]->size();
            switch(clauses[it->index]->size())
            {
                case 3:
                    before_3long++;
                    break;
                default:
                    before_long++;
            }
        }
    posSize += numNonLearntPos;

    //Stats on negative
    uint32_t negSize = 0;
    for (Occur::const_iterator it = negs.begin(), end = negs.end(); it != end; it++)
        if (!clauses[it->index]->learnt()) {
            negSize++;
            before_literals += clauses[it->index]->size();
            switch(clauses[it->index]->size())
            {
                case 3:
                    before_3long++;
                    break;
                default:
                    before_long++;
            }
        }
    negSize += numNonLearntNeg;

    *toDecrease -= (posSize + negSize)/2;

    /*// Heuristic CUT OFF:
    if (posSize >= 15 && negSize >= 15)
        return -1000;*/

    //Fill datastructs that will store data from occur & watchlists
    posAll.clear();
    negAll.clear();
    fillClAndBin(posAll, poss, lit);
    fillClAndBin(negAll, negs, ~lit);

    // Count clauses/literals after elimination:
    uint32_t before_clauses = posSize + negSize;
    uint32_t after_clauses = 0;
    uint32_t after_long = 0;
    int after_literals = 0;
    for (vector<ClAndBin>::const_iterator
        it = posAll.begin(), end = posAll.end()
        ; it != end
        ; it++
    ) {
        for (vector<ClAndBin>::const_iterator
            it2 = negAll.begin(), end2 = negAll.end()
            ; it2 != end2
            ; it2++
        ) {
            //If any of the two is learnt and long, and we don't keep it, skip
            if ((it->learnt || it2->learnt)) {
                continue;
            }

            //Decrement available time
            *toDecrease -= 2;

            // Merge clauses. If 'y' and '~y' exist, clause will not be created.
            bool ok = merge(*it, *it2, lit, ~lit, agressiveCheck, false);
            if (ok) {
                //Update after-stats
                if (dummy.size() > 3)
                    after_long++;

                after_clauses++;
                after_literals += dummy.size();

                //Early-abort
                if (after_clauses > before_clauses)
                    return -1000;
            }
        }
    }

    //return before_literals-after_literals;
    return before_long-after_long;
}

void Subsumer::varElimCheckUpdate(
    const vector<ClAndBin>& gothrough
    , vector<Var>& varElimToCheck
    , vector<char>& varElimToCheckHelper
) {
    for (vector<ClAndBin>::const_iterator
        it = gothrough.begin(), end = gothrough.end()
        ; it != end
        ; it++
    ) {
        if (it->isBin) {
            const Var var1 = it->lit1.var();
            if (!varElimToCheckHelper[var1]) {
                varElimToCheck.push_back(var1);
                varElimToCheckHelper[var1] = 1;
            }

            const Var var2 = it->lit2.var();
            if (!varElimToCheckHelper[var2]) {
                varElimToCheck.push_back(var2);
                varElimToCheckHelper[var2] = 1;
            }
        } else {
            Clause& cl = *clauses[it->clsimp.index];
            if (cl.learnt())
                continue;

            for(size_t i = 0; i < cl.size(); i++) {
                const Var var = cl[i].var();
                if (!varElimToCheckHelper[var]) {
                    varElimToCheck.push_back(var);
                    varElimToCheckHelper[var] = 1;
                }
            }
        }
    }
}

void Subsumer::varElimCheckUpdate(
    const vector<Lit>& cl
    , vector<Var>& varElimToCheck
    , vector<char>& varElimToCheckHelper
) {
   for(size_t i = 0; i < cl.size(); i++) {
        const Var var = cl[i].var();
        if (!varElimToCheckHelper[var]) {
            varElimToCheck.push_back(var);
            varElimToCheckHelper[var] = 1;
        }
    }
}

/**
@brief Tries to eliminate variable
*/
bool Subsumer::maybeEliminate(const Var var)
{
    //Print complexity stat for this var
    if (solver->conf.verbosity >= 5) {
        cout << "trying comlexity: "
        << varElimComplexity[var].first
        << ", " << varElimComplexity[var].second
        << endl;
    }

    //Update stats
    const bool agressiveCheck = (numMaxVarElimAgressiveCheck > 0);
    if (agressiveCheck)
        runStats.usedAgressiveCheckToELim++;
    runStats.triedToElimVars++;

    //Test if we should remove, and fill posAll&negAll
    if (testVarElim(var) == -1000)
        return false;

    const Lit lit = Lit(var, false);

    //Eliminate:
    if (solver->conf.verbosity >= 5) {
        cout << "Eliminating var " << lit << endl;
    }

    //Re-examine later the elimination complexity of these variables
    vector<Var> varElimToCheck;
    vector<char> varElimToCheckHelper(solver->nVars(), 0);
    varElimCheckUpdate(posAll, varElimToCheck, varElimToCheckHelper);
    varElimCheckUpdate(negAll, varElimToCheck, varElimToCheckHelper);

    //put clauses into blocked status, remove from occur[], but DON'T free&set to NULL
    occur[lit.toInt()].clear();
    occur[(~lit).toInt()].clear();
    removeClauses(var);

    //add newly dot-producted clauses
    for (vector<ClAndBin>::const_iterator
        it = posAll.begin(), end = posAll.end()
        ; it != end
        ; it++
    ) {
        for (vector<ClAndBin>::const_iterator
            it2 = negAll.begin(), end2 = negAll.end()
            ; it2 != end2
            ; it2++
        ) {
            //If any of the two is learnt and long, and we don't keep it, skip
            if ((it->learnt || it2->learnt))
                continue;

            //Decrement available time
            *toDecrease -= 2;

            //Create dot-product
            bool ok = merge(*it, *it2, lit, ~lit, true, true);

            //The merge resulted in a tautological clause
            if (!ok)
                continue;

            #ifdef VERBOSE_DEBUG_VARELIM
            cout << "Adding new clause due to varelim: " << dummy << endl;
            #endif

            //Calculate stats
            const bool learnt = it->learnt || it2->learnt;
            ClauseStats stats;
            if (it->isBin && !it2->isBin)
                stats = clauses[it2->clsimp.index]->stats;
            else if (!it->isBin && it2->isBin)
                stats = clauses[it->clsimp.index]->stats;
            else if (!it->isBin && !it2->isBin)
                stats = ClauseStats::combineStats(
                    clauses[it->clsimp.index]->stats
                    , clauses[it2->clsimp.index]->stats
            );

            //Add clause and do subsumption
            *toDecrease -= dummy.size();
            Clause* newCl = solver->addClauseInt(
                dummy //Literals in new clause
                , learnt //Is the new clause learnt?
                , stats //Statistics for this new clause (usage, etc.)
                , false //Should clause be attached?
                , &finalLits //Return final set of literals here
            );
            varElimCheckUpdate(finalLits, varElimToCheck, varElimToCheckHelper);

            if (!solver->ok)
                goto end;

            if (newCl != NULL) {
                ClauseIndex newClSimp = linkInClause(*newCl);
                subsume0(newClSimp, *newCl);
            } else if (finalLits.size() == 2) {
                Sub0Ret ret = subsume0(
                    std::numeric_limits<uint32_t>::max() //Index of this binary clause (non-existent)
                    , finalLits //Literals in this binary clause
                    , calcAbstraction(finalLits) //Abstraction of literals
                );
                if (ret.numSubsumed > 0) {
                    if (solver->conf.verbosity >= 5) {
                        cout << "Subsumed: " << ret.numSubsumed << endl;
                    }
                }
            }
        }
    }

    //Free & NULL clauses
    freeAfterVarelim(posAll);
    freeAfterVarelim(negAll);

    #ifndef NDEBUG
    //Check that no eliminated non-learnt binary clauses are left inside
    for (uint32_t i = 0; i < solver->watches[lit.toInt()].size(); i++) {
        Watched& w = solver->watches[lit.toInt()][i];
        assert(w.isTriClause() || (w.isBinary() && w.getLearnt()));
    }
    for (uint32_t i = 0; i < solver->watches[(~lit).toInt()].size(); i++) {
        Watched& w = solver->watches[(~lit).toInt()][i];
        assert(w.isTriClause() || (w.isBinary() && w.getLearnt()));
    }
    #endif

    //removeBinsAndTris(var);
    assert(occur[lit.toInt()].size() == 0 &&  occur[(~lit).toInt()].size() == 0);

    //cout << "varElimToCheck size: " << varElimToCheck.size() << endl;
    for(vector<Var>::const_iterator
        it = varElimToCheck.begin(), end = varElimToCheck.end()
        ; it != end
        ; it++
    ) {
        //No point in updating the score of this var
        //it's eliminated already, or not to be eliminated at all
        if (*it == var || !varElimOrder.inHeap(*it))
            continue;

        std::pair<int, int> cost = heuristicCalcVarElimScore(Lit(*it, false));
        varElimComplexity[*it] = cost;
        varElimOrder.update(*it);
    }

end:
    if (solver->conf.verbosity >= 5) {
        cout << "Elimination of var " << lit << " finished " << endl;
    }

    var_elimed[var] = true;
    solver->varData[var].elimed = ELIMED_VARELIM;
    runStats.numVarsElimed++;
    solver->unsetDecisionVar(var);
    return solver->ok;
}

void Subsumer::freeAfterVarelim(const vector<ClAndBin>& myset)
{
    for(uint32_t i = 0; i < myset.size(); i++) {
        //If binary, or has been removed due to subsume0 of merged clauses
        //then skip
        if (myset[i].isBin
            || clauses[myset[i].clsimp.index] == NULL
        ) continue;

        uint32_t index = myset[i].clsimp.index;
        #ifdef VERBOSE_DEBUG_VARELIM
        cout << "Freeing clause due to varelim: " << *clauses[index] << endl;
        #endif

        solver->clAllocator->clauseFree(clauses[index]);
        clauses[index] = NULL;
    }
}

void Subsumer::addLearntBinaries(const Var var)
{
    vector<Lit> tmp(2);
    Lit lit = Lit(var, false);
    const vec<Watched>& ws = solver->watches[lit.toInt()];
    const vec<Watched>& ws2 = solver->watches[(~lit).toInt()];

    for (vec<Watched>::const_iterator w1 = ws.begin(), end1 = ws.end(); w1 != end1; w1++) {
        if (!w1->isBinary()) continue;
        const bool numOneIsLearnt = w1->getLearnt();
        const Lit lit1 = w1->getOtherLit();
        if (solver->value(lit1) != l_Undef || var_elimed[lit1.var()]) continue;

        for (vec<Watched>::const_iterator w2 = ws2.begin(), end2 = ws2.end(); w2 != end2; w2++) {
            if (!w2->isBinary()) continue;
            const bool numTwoIsLearnt = w2->getLearnt();
            if (!numOneIsLearnt && !numTwoIsLearnt) {
                //At least one must be learnt
                continue;
            }

            const Lit lit2 = w2->getOtherLit();
            if (solver->value(lit2) != l_Undef || var_elimed[lit2.var()]) continue;

            tmp[0] = lit1;
            tmp[1] = lit2;
            Clause* tmpOK = solver->addClauseInt(tmp, true);
            runStats.numLearntBinVarRemAdded++;
            release_assert(tmpOK == NULL);
            release_assert(solver->ok);
        }
    }
    assert(solver->value(lit) == l_Undef);
}

/**
@brief Resolves two clauses on a variable

Clause ps must contain without_p
Clause ps must contain without_q
And without_p = ~without_q

@note: 'seen' is assumed to be cleared.

@param[in] var The variable that is being eliminated
@param useCache Use the cache to try to find that the resulting clause is a tautology
@return FALSE if clause is always satisfied ('out_clause' should not be used)
*/
bool Subsumer::merge(
    const ClAndBin& ps
    , const ClAndBin& qs
    , const Lit without_p
    , const Lit without_q
    , const bool useCache
    , const bool final
) {
    //If clause has already been freed, skip
    if (!ps.isBin && clauses[ps.clsimp.index] == NULL)
        return false;
    if (!qs.isBin && clauses[qs.clsimp.index] == NULL)
        return false;

    dummy.clear(); //The final clause
    dummy2.clear(); //Used to clear 'seen'

    bool retval = true;
    bool fancyRemove = false;
    if (ps.isBin) {
        assert(ps.lit1 == without_p);
        assert(ps.lit2 != without_p);

        seen[ps.lit2.toInt()] = 1;
        dummy.push_back(ps.lit2);
    } else {
        Clause& c = *clauses[ps.clsimp.index];
        //assert(!clauseData[ps.clsimp.index].defOfOrGate);
        *toDecrease -= c.size();
        for (uint32_t i = 0; i < c.size(); i++){
            //Skip without_p
            if (c[i] == without_p)
                continue;

            seen[c[i].toInt()] = 1;
            dummy.push_back(c[i]);
        }
    }

    if (qs.isBin) {
        assert(qs.lit1 == without_q);
        assert(qs.lit2 != without_q);

        if (seen[(~qs.lit2).toInt()]) {
            retval = false;
            dummy2 = dummy;
            goto end;
        }
        if (!seen[qs.lit2.toInt()]) {
            dummy.push_back(qs.lit2);
            seen[qs.lit2.toInt()] = 1;
        }
    } else {
        Clause& c = *clauses[qs.clsimp.index];
        //assert(!clauseData[qs.clsimp.index].defOfOrGate);
        *toDecrease -= c.size();
        for (uint32_t i = 0; i < c.size(); i++){
            //Skip without_q
            if (c[i] == without_q)
                continue;

            //Opposite is inside, nothing to add
            if (seen[(~c[i]).toInt()]) {
                retval = false;
                dummy2 = dummy;
                goto end;
            }

            //Add the literal
            if (!seen[c[i].toInt()]) {
                dummy.push_back(c[i]);
                seen[c[i].toInt()] = 1;
            }
        }
    }
    dummy2 = dummy;

    //We add to 'seen' what COULD be added to the clause
    //This is essentially the reverse of cache-based vivification
    if (useCache && solver->conf.doAsymmTE) {
        for (size_t i = 0; i < dummy.size(); i++) {
            const Lit lit = dummy2[i];

            //Use cache -- but only if none of the clauses were binary
            //Otherwise we cannot tell if the value in the cache is dependent
            //on the binary clause itself, so that would cause a circular de-
            //pendency
            if (!ps.isBin && !qs.isBin) {
                const vector<LitExtra>& cache = solver->implCache[lit.toInt()].lits;
                numMaxVarElimAgressiveCheck -= cache.size()/3;
                for(vector<LitExtra>::const_iterator
                    it = cache.begin(), end = cache.end()
                    ; it != end
                    ; it++
                ) {
                    //If learnt, that doesn't help
                    if (!it->getOnlyNLBin())
                        continue;

                    const Lit otherLit = it->getLit();

                    //If (a) was in original clause
                    //then (a V b) means -b can be put inside
                    if(!seen[(~otherLit).toInt()]) {
                        dummy2.push_back(~otherLit);
                        seen[(~otherLit).toInt()] = 1;
                    }

                    //If (a V b) is non-learnt in the clause, then done
                    if (seen[otherLit.toInt()]) {
                        retval = false;
                        fancyRemove = true;
                        goto end;
                    }
                }
            }

            //Use watchlists
            //(~lit) because watches are inverted...... this is CONFUSING
            const vec<Watched>& ws = solver->watches[(~lit).toInt()];
            numMaxVarElimAgressiveCheck -= ws.size()/3;
            for(vec<Watched>::const_iterator it =
                ws.begin(), end = ws.end()
                ; it != end
                ; it++
            ) {
                //Only care about binary clauses
                if (!it->isBinary())
                    continue;

                if (!it->isNonLearntBinary()) {
                    const Lit otherLit = it->getOtherLit();

                    //If (a V b) is learnt, make it non-learnt and we are done
                    if (seen[otherLit.toInt()]) {
                        //Only actually make the binary clause non-learnt if
                        //we are *actually* eliminating the variable
                        if (final) {
                            findWatchedOfBin(solver->watches, lit, otherLit, true).setLearnt(false);
                            findWatchedOfBin(solver->watches, otherLit, lit, true).setLearnt(false);
                            solver->numBinsLearnt--;
                            solver->numBinsNonLearnt++;
                            solver->learntsLits -= 2;
                            solver->clausesLits += 2;
                            //cout << "Removed using new technique!!" << endl;
                        }

                        retval = false;
                        fancyRemove = true;
                        goto end;
                    }

                    continue;
                }
                //It's surely a non-learnt binary now
                assert(it->isNonLearntBinary());

                const Lit otherLit = it->getOtherLit();

                //If (a) is in clause
                //then (a V b) means -b can be put inside
                if (!seen[(~otherLit).toInt()]) {
                    dummy2.push_back(~otherLit);
                    seen[(~otherLit).toInt()] = 1;
                }

                //If (a V b) is non-learnt, and in the clause, then we can remove
                if (seen[otherLit.toInt()]) {
                    retval = false;
                    fancyRemove = true;
                    goto end;
                }
            }
        }
    }

    end:
    //Clear 'seen'
    for (vector<Lit>::const_iterator
        it = dummy2.begin(), end = dummy2.end()
        ; it != end
        ; it++
    ) {
        seen[it->toInt()] = 0;
    }

    //If we are *really* eliminating, update the stats
    if (final) {
        runStats.newClauses++;
        if (fancyRemove)
            runStats.newClauseNotAddedFancy++;
        if (!retval)
            runStats.newClauseNotAdded++;
    }

    return retval;
}


std::pair<int, int> Subsumer::heuristicCalcVarElimScore(const Lit lit)
{
    //Count number of non-learnt long clauses with X inside
    size_t pos = 0;
    size_t posLit = 0;
    const Occur& poss = occur[lit.toInt()];
    *toDecrease -= poss.size();
    for (Occur::const_iterator it = poss.begin(), end = poss.end(); it != end; it++)
        if (!clauses[it->index]->learnt()) {
            posLit += clauses[it->index]->size();
            pos++;
        }

    //Count number of non-learnt long clauses with ~X inside
    size_t neg = 0;
    size_t negLit = 0;
    const Occur& negs = occur[(~lit).toInt()];
    *toDecrease -= negs.size();
    for (Occur::const_iterator it = negs.begin(), end = negs.end(); it != end; it++)
        if (!clauses[it->index]->learnt()) {
            negLit += clauses[it->index]->size();
            neg++;
        }

    uint32_t nNonLBinPos = numNonLearntBins(lit);
    uint32_t nNonLBinNeg = numNonLearntBins(~lit);

    int normCost = pos * neg //long clauses have a good chance of being tautologies
        + nNonLBinPos * neg * 2 //lower chance of tautology because of binary clause
        + nNonLBinNeg * pos * 2 //lower chance of tautology because of binary clause
        + nNonLBinPos * nNonLBinNeg * 5; //Very low chance of tautology


    if ((pos + nNonLBinPos) <= 2 && (neg + nNonLBinNeg) <= 2) {
        normCost = 0;
    }

    int litCost = posLit * negLit;

    return std::make_pair(normCost, litCost);
}

void Subsumer::orderVarsForElimInit()
{
    varElimOrder.clear();
    varElimComplexity.clear();
    varElimComplexity.resize(solver->nVars(), std::make_pair<int, int>(1000, 1000));

    //Go through all vars that have been touched
    for (vector<Var>::const_iterator
        it = touchedVars.begin(), end = touchedVars.end()
        ; it != end
        ; it++
    ){
        Lit x = Lit(*it, false);

        //Can this variable be eliminated at all?
        if (solver->value(*it) != l_Undef
            || solver->varData[*it].elimed != ELIMED_NONE
            || !gateFinder->canElim(*it)
        ) {
            continue;
        }

        assert(!varElimOrder.inHeap(*it));

        if (solver->conf.varelimStrategy == 0) {
            std::pair<int, int> cost = heuristicCalcVarElimScore(x);
            varElimComplexity[*it] = cost;             
            varElimOrder.insert(*it);
        } else {
            int ret = testVarElim(*it);

            //Cannot be eliminated
            if (ret == -1000)
                continue;

            varElimComplexity[*it].first = 1000-ret;
            varElimOrder.insert(*it);
        }
    }
    touchedVars.clear();
    assert(varElimOrder.heapProperty());

    //Print sorted listed list
    #ifdef VERBOSE_DEBUG_VARELIM
    cout << "-----------" << endl;
    for(size_t i = 0; i < varElimOrder.size(); i++) {
        cout
        << "varElimOrder[" << i << "]: "
        << " var: " << varElimOrder[i]+1
        << " val: " << varElimComplexity[varElimOrder[i]]
        << endl;
    }
    #endif
}

/**
@brief Verifies that occurrence lists are OK

Calculates how many occurences are of the varible in clauses[], and if that is
less than occur[var].size(), returns FALSE

@return TRUE if they are OK
*/
bool Subsumer::verifyIntegrity()
{
    vector<uint32_t> occurNum(solver->nVars()*2, 0);

    for (uint32_t i = 0; i < clauses.size(); i++) {
        if (clauses[i] == NULL) continue;
        Clause& c = *clauses[i];
        for (uint32_t i2 = 0; i2 < c.size(); i2++)
            occurNum[c[i2].toInt()]++;
    }

    for (uint32_t i = 0; i < occurNum.size(); i++) {
        #ifdef VERBOSE_DEBUG
        cout << "occurNum[i]:" << occurNum[i]
        << " occur[i]:" << occur[i].size()
        << "  --- i:" << i << endl;
        #endif //VERBOSE_DEBUG

        if (occurNum[i] != occur[i].size()) return false;
    }

    return true;
}

inline bool Subsumer::allTautologySlim(const Lit lit)
{
    //Binary clauses which contain '~lit'
    const vec<Watched>& ws = solver->watches[lit.toInt()];
    for (vec<Watched>::const_iterator it = ws.begin(), end = ws.end(); it != end; it++) {
        *toDecrease -= 2;
        if (!it->isNonLearntBinary())
            continue;

        if (seen[(~it->getOtherLit()).toInt()]) {
            assert(it->getOtherLit() != ~lit);
            continue;
        }

        return false;
    }

    //Long clauses that contain '~lit'
    const Occur& cs = occur[(~lit).toInt()];
    for (Occur::const_iterator it = cs.begin(), end = cs.end(); it != end; it++) {
        *toDecrease -= 2;
        const Clause& c = *clauses[it->index];
        if (c.learnt())
            continue;

        *toDecrease -= 5;
        for (const Lit *l = c.begin(), *end2 = c.end(); l != end2; l++) {
            *toDecrease -= 1;
            if (seen[(~(*l)).toInt()] && *l != ~lit) {
                goto next;
            }
        }
        return false;

        next:;
    }

    return true;
}

void Subsumer::checkElimedUnassignedAndStats() const
{
    assert(solver->ok);
    uint64_t checkNumElimed = 0;
    for (size_t i = 0; i < var_elimed.size(); i++) {
        if (var_elimed[i]) {
            checkNumElimed++;
            assert(solver->assigns[i] == l_Undef);
        }
    }
    assert(globalStats.numVarsElimed == checkNumElimed);
}

const GateFinder* Subsumer::getGateFinder() const
{
    return gateFinder;
}

