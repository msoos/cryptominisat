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

#include "searcher.h"
#include "simplifier.h"
#include "calcdefpolars.h"
#include "time_mem.h"
#include "solver.h"
#include <iomanip>
#include "sccfinder.h"
#include "varreplacer.h"
#include "clausecleaner.h"
#include "propbyforgraph.h"
#include <algorithm>

using namespace CMSat;
using std::cout;
using std::endl;

#ifdef USE_OMP
#include <omp.h>
#endif

//#define VERBOSE_DEBUG_GEN_CONFL_DOT

#ifdef VERBOSE_DEBUG
#define VERBOSE_DEBUG_GEN_CONFL_DOT
#endif

/**
@brief Sets a sane default config and allocates handler classes
*/
Searcher::Searcher(const SolverConf& _conf, Solver* _solver) :
        PropEngine(
            _solver->clAllocator
            , AgilityData(_conf.agilityG, _conf.agilityLimit)
            , _conf.updateGlues
            , _conf.doLHBR
        )

        //variables
        , solver(_solver)
        , conf(_conf)
        , needToInterrupt(false)
        , var_inc(_conf.var_inc_start)
        , order_heap(VarOrderLt(activities))
        , clauseActivityIncrease(1)
{
    mtrand.seed(conf.origSeed);
    hist.setSize(conf.shortTermHistorySize);
}

Searcher::~Searcher()
{
}

Var Searcher::newVar(const bool dvar)
{
    const Var var = PropEngine::newVar(dvar);
    assert(var == activities.size());
    activities.push_back(0);
    if (dvar) {
        insertVarOrder(var);
    }

    return var;
}

/**
@brief Revert to the state at given level
*/
void Searcher::cancelUntil(uint32_t level)
{
    #ifdef VERBOSE_DEBUG
    cout << "Canceling until level " << level;
    if (level > 0) cout << " sublevel: " << trail_lim[level];
    cout << endl;
    #endif

    if (decisionLevel() > level) {

        //Go through in reverse order, unassign & insert then
        //back to the vars to be branched upon
        for (int sublevel = trail.size()-1
            ; sublevel >= (int)trail_lim[level]
            ; sublevel--
        ) {
            #ifdef VERBOSE_DEBUG
            cout
            << "Canceling lit " << trail[sublevel]
            << " sublevel: " << sublevel
            << endl;
            #endif

            #ifdef ANIMATE3D
            std:cerr << "u " << var << endl;
            #endif

            const Var var = trail[sublevel].var();
            assert(value(var) != l_Undef);
            assigns[var] = l_Undef;
            insertVarOrder(var);
        }
        qhead = trail_lim[level];
        trail.resize(trail_lim[level]);
        trail_lim.resize(level);
    }

    #ifdef VERBOSE_DEBUG
    cout
    << "Canceling finished. Now at level: " << decisionLevel()
    << " sublevel: " << trail.size()-1
    << endl;
    #endif
}

void Searcher::analyzeHelper(
    const Lit lit
    , int& pathC
    , bool fromProber
) {
    const Var var = lit.var();
    assert(varData[var].elimed == ELIMED_NONE
        || varData[var].elimed == ELIMED_QUEUED_VARREPLACER
    );

    //If var is at level 0, don't do anything with it, just skip
    if (varData[var].level == 0)
        return;

    //Update our state of going through the conflict
    if (!seen[var]) {
        seen[var] = 1;

        varBumpActivity(var);
        //learnt_clause2.insert(lit);
        learnt_clause2_size++;
        seen2[lit.toInt()] = 1;
        learnt_clause2_abst |= abst_var(lit.var());

        if (varData[var].level == decisionLevel()) {
            pathC++;

            //Glucose 2.1
            if (!fromProber
                && params.rest_type != geom_restart
                && varData[var].reason != PropBy()
                && varData[var].reason.getType() == clause_t
            ) {
                Clause* cl = clAllocator->getPointer(varData[var].reason.getClause());
                if (cl->learnt()) {
                    lastDecisionLevel.push_back(std::make_pair(lit, cl->stats.glue));
                }
            }
        }
        else {
            learnt_clause.push_back(lit);
        }
    }
}

void Searcher::recursiveConfClauseMin()
{
    uint32_t abstract_level = 0;
    for (size_t i = 1; i < learnt_clause.size(); i++) {
        //(maintain an abstraction of levels involved in conflict)
        abstract_level |= abstractLevel(learnt_clause[i].var());
    }

    size_t i, j;
    for (i = j = 1; i < learnt_clause.size(); i++) {
        #ifdef DEBUG_LITREDUNDANT
        cout << "Calling litRedundant at i = " << i << endl;
        #endif
        if (varData[learnt_clause[i].var()].reason.isNULL()
            || !litRedundant(learnt_clause[i], abstract_level)
        ) {
            learnt_clause[j++] = learnt_clause[i];
        }
    }
    learnt_clause.resize(j);
}

void Searcher::doOTFSubsume(PropBy confl)
{
    ClOffset offset = confl.getClause();
    Clause& cl = *clAllocator->getPointer(offset);

    //Count if every one from learnt_clause is inside
    size_t num = 0;
    for (size_t j = 0, size = cl.size(); j < size; j++) {
        if (seen2[cl[j].toInt()]) {
            num++;
        }
    }

    //Does not subsume
    if (num != learnt_clause2_size)
        return;

    /*
    cout
    << "MATCH!"
    << " cl: " << cl << "learnt: " << cl.learnt() << endl
    << " learnt cl: ";
    for(const Lit
        *it = cl.begin(), *end = cl.end()
        ; it != end
        ; it++
    ) {
        if (seen2[it->toInt()]) {
            cout << *it << ", ";
        }
    }
    cout << endl;
    */

    //Final will be implicit
    if (num <= 3) {
        OTFClause newCl;
        newCl.size = 0;
        for(const Lit
            *it = cl.begin(), *end = cl.end()
            ; it != end
            ; it++
        ) {
            if (seen2[it->toInt()]) {
                assert(newCl.size < 3);
                newCl.lits[newCl.size] = *it;
                newCl.size++;
            }
        }
        otfMustAttach.push_back(newCl);
        #ifdef DRUP
        if (conf.verbosity >= 6) {
            cout << "New implicit clause that subsumes a long clause:";
            for(unsigned  i = 0; i < newCl.size; i++) {
                cout
                << newCl.lits[i] << " ";
            }
            cout  << endl;
        }

        if (drup) {
            for(unsigned  i = 0; i < newCl.size; i++) {
                *(drup)
                << newCl.lits[i] << " ";
            }
            (*drup) << " 0\n";
        }
        #endif //DRUP

        stats.otfSubsumed++;
        stats.otfSubsumedImplicit++;
        stats.otfSubsumedLearnt += cl.learnt();
        stats.otfSubsumedLitsGained += cl.size() - newCl.size;
    }

    //Final will not be implicit
    if (num > 3) {
        #ifdef DRUP
        vector<Lit> origCl(cl.size());
        std::copy(cl.begin(), cl.end(), origCl.begin());
        #endif
        solver->detachClause(cl, false);
        stats.otfSubsumed++;
        stats.otfSubsumedLong++;
        stats.otfSubsumedLearnt += cl.learnt();
        stats.otfSubsumedLitsGained += cl.size() - learnt_clause2_size;

        size_t i = 0;
        size_t i2 = 0;
        for (; i < cl.size(); i++) {
            if (seen2[cl[i].toInt()]) {
                cl[i2++] = cl[i];
            }
        }
        cl.shrink(i-i2);
        assert(cl.size() == learnt_clause2_size);
        #ifdef DRUP
        if (conf.verbosity >= 6) {
            cout
            << "New smaller clause OTF:" << cl << endl;
        }
        if (drup) {
            (*drup)
            << cl << " 0\n"
            << "d " << origCl << " 0\n"
            ;
        }
        #endif

        toAttachLater.push_back(offset);
    }
}

void Searcher::normalClMinim()
{
    size_t i,j;
    for (i = j = 1; i < learnt_clause.size(); i++) {
        const PropBy& reason = varData[learnt_clause[i].var()].reason;
        size_t size;
        Clause* cl = NULL;
        PropByType type = reason.getType();
        if (type == null_clause_t) {
            learnt_clause[j++] = learnt_clause[i];
            continue;
        }

        switch (type) {
            case clause_t:
                cl = clAllocator->getPointer(reason.getClause());
                size = cl->size()-1;
                break;

            case binary_t:
                size = 1;
                break;

            case tertiary_t:
                size = 2;
                break;

            case null_clause_t:
                release_assert(false);
                exit(-1);
                break;
        }

        for (size_t k = 0; k < size; k++) {
            Lit p;
            switch (type) {
                case clause_t:
                    p = (*cl)[k+1];
                    break;

                case binary_t:
                    p = reason.lit1();
                    break;

                case tertiary_t:
                    if (k == 0) {
                        p = reason.lit1();
                    } else {
                        p = reason.lit2();
                    }
                    break;

                case null_clause_t:
                    release_assert(false);
                    exit(-1);
                    break;
            }

            if (!seen[p.var()] && varData[p.var()].level > 0) {
                learnt_clause[j++] = learnt_clause[i];
                break;
            }
        }
    }
    learnt_clause.resize(j);
}

/**
@brief    Analyze conflict and produce a reason clause.

Post-condition: 'learnt_clause[0]' is the asserting literal at level 'out_btlevel'
*/
Clause* Searcher::analyze(
    PropBy confl
    , uint32_t& out_btlevel
    , uint32_t &glue
    , ResolutionTypes<uint16_t>& resolutions
    , bool fromProber
) {
    //Set up environment
    learnt_clause.clear();
    toClear.clear();
    lastDecisionLevel.clear();
    otfMustAttach.clear();
    toAttachLater.clear();
    //learnt_clause2.clear();
    learnt_clause2_size = 0;
    learnt_clause2_abst = 0;
    assert(decisionLevel() > 0);

    int pathC = 0;
    Lit p = lit_Undef;
    int index = trail.size() - 1;
    out_btlevel = 0;

    //cout << "---- Start analysis -----" << endl;
    learnt_clause.push_back(lit_Undef); //make space for ~p
    Clause* cl;
    do {
        #ifdef DEBUG_RESOLV
        cout << "p is: " << p << endl;
        #endif

        //This is for OTF subsumption ("OTF clause improvement" by Han&Somezi)
        if (p != lit_Undef) {
            //assert(learnt_clause2.find(~p) != learnt_clause2.end());
            //learnt_clause2.erase(~p);
            learnt_clause2_size--;
            assert(seen2[(~p).toInt()] == 1);
            seen2[(~p).toInt()] = 0;

            //We MUST under-estimate
            learnt_clause2_abst &= ~(abst_var((~p).var()));
        }

        //Add literals from 'confl' to clause
        cl = NULL;
        switch (confl.getType()) {
            case tertiary_t : {
                resolutions.tri++;
                stats.resolvs.tri++;
                #ifdef DEBUG_RESOLV
                cout << "resolv (tri): " << confl.lit2() << endl;
                #endif
                analyzeHelper(confl.lit2(), pathC, fromProber);
            }
            //NO BREAK, since tertiary is like binary, just one more lit

            case binary_t : {
                //We fall here even on TRI, so make sure
                if (confl.getType() == binary_t) {
                    resolutions.bin++;
                    stats.resolvs.bin++;
                }


                if (p == lit_Undef) {
                    analyzeHelper(failBinLit, pathC, fromProber);
                }

                analyzeHelper(confl.lit1(), pathC, fromProber);
                #ifdef DEBUG_RESOLV
                cout << "resolv (bin/tri): " << confl.lit1() << endl;
                #endif
                break;
            }

            case clause_t : {
                cl = clAllocator->getPointer(confl.getClause());
                #ifdef DEBUG_RESOLV
                cout << "resolv (long): " << *cl << endl;
                #endif

                if (cl->learnt()) {
                    resolutions.redL++;
                    stats.resolvs.redL++;
                } else {
                    resolutions.irredL++;
                    stats.resolvs.irredL++;
                }

                //Update stats
                cl->stats.numUsedUIP++;
                if (cl->learnt() && !fromProber) {
                    bumpClauseAct(cl);
                }

                for (size_t j = 0, size = cl->size(); j != size; j++) {

                    //This is the one that will be resolved out anyway, so just skip
                    if (p != lit_Undef && j == 0) {
                        continue;
                    }

                    analyzeHelper((*cl)[j], pathC, fromProber);
                }
                break;
            }

            case null_clause_t:
            default:
                //otherwise should be UIP
                assert(false && "Error in conflict analysis");
                break;
        }

        // Select next implication to look at
        while (!seen[trail[index--].var()]);

        p = trail[index+1];

        if (!fromProber
            && conf.doOTFSubsume
            && cl != NULL
            && cl->size() > learnt_clause2_size
            //Everything in learnt_cl_2 seems to be also in cl
            && ((cl->abst & learnt_clause2_abst) ==  learnt_clause2_abst)
            && pathC > 1
        ) {
            doOTFSubsume(confl);
        }

        //Saving old confl for OTF subsumption
        confl = varData[p.var()].reason;

        //This clears out vars that haven't been added to learnt_clause,
        //but their 'seen' has been set
        seen[p.var()] = 0;

        //Okay, one more path done
        pathC--;
    } while (pathC > 0);
    learnt_clause[0] = ~p;

    assert(pathC == 0);
    stats.litsLearntNonMin += learnt_clause.size();
    const size_t origSize = learnt_clause.size();

    //Recursive cc min
    toClear = learnt_clause;
    if (conf.doRecursiveMinim) {
        recursiveConfClauseMin();
    } else {
        normalClMinim();
    }

    //Clear seen
    for (size_t i = 0; i < toClear.size(); i++) {
        seen[toClear[i].var()] = 0;
        seen2[toClear[i].toInt()] = 0;
    }
    toClear.clear();

    stats.recMinCl += ((origSize - learnt_clause.size()) > 0);
    stats.recMinLitRem += origSize - learnt_clause.size();

    //Cache-based minimisation
    if (conf.doMinimLearntMore
        && learnt_clause.size() > 1
        && (conf.doAlwaysFMinim
            || calcGlue(learnt_clause) < 0.65*hist.glueHistLT.avg()
            || learnt_clause.size() < 0.65*hist.conflSizeHistLT.avg()
            || learnt_clause.size() < 10
            )
    ) {
        stats.moreMinimLitsStart += learnt_clause.size();

        //Binary&cache-based minim
        minimiseLearntFurther(learnt_clause);

        //Stamp-based minimization
        if (conf.doStamp) {
            stampBasedLearntMinim(learnt_clause);
        }

        stats.moreMinimLitsEnd += learnt_clause.size();
    }

    //Calc stats
    glue = calcGlue(learnt_clause);
    stats.litsLearntFinal += learnt_clause.size();

    //Print fully minimised clause
    #ifdef VERBOSE_DEBUG_OTF_GATE_SHORTEN
    cout << "Final clause: " << learnt_clause << endl;
    for (uint32_t i = 0; i < learnt_clause.size(); i++) {
        cout << "lev learnt_clause[" << i << "]:" << varData[learnt_clause[i].var()].level << endl;
    }
    #endif

    // Find correct backtrack level:
    if (learnt_clause.size() <= 1)
        out_btlevel = 0;
    else {
        uint32_t max_i = 1;
        for (uint32_t i = 2; i < learnt_clause.size(); i++)
            if (varData[learnt_clause[i].var()].level > varData[learnt_clause[max_i].var()].level)
                max_i = i;
        std::swap(learnt_clause[max_i], learnt_clause[1]);
        out_btlevel = varData[learnt_clause[1].var()].level;
    }

    //Glucose 2.1
    if (!fromProber
        && params.rest_type == glue_restart
    ) {
        for (vector<pair<Lit, size_t> >::const_iterator
            it = lastDecisionLevel.begin(), end = lastDecisionLevel.end()
            ; it != end
            ; it++
        ) {
            if (it->second < glue) {
                varBumpActivity(it->first.var());
            }
        }
    }
    lastDecisionLevel.clear();

    //We can only on-the-fly subsume with clauses that are not 2- or 3-long
    //furthermore, we cannot subsume a clause that is marked for deletion
    //due to its high glue value
    if (!conf.doOTFSubsume
        //Last was a lont clause
        || cl == NULL
        //Final clause will not be implicit
        || learnt_clause.size() <= 3
        //Larger or equivalent clauses cannot subsume the clause
        || learnt_clause.size() >= cl->size()
    ) {
        return NULL;
    }

    //Does it subsume?
    if (!subset(learnt_clause, *cl))
        return NULL;

    //on-the-fly subsumed the original clause
    stats.otfSubsumed++;
    stats.otfSubsumedLong++;
    stats.otfSubsumedLearnt += cl->learnt();
    stats.otfSubsumedLitsGained += cl->size() - learnt_clause.size();
    return cl;

}

bool Searcher::litRedundant(const Lit p, uint32_t abstract_levels)
{
    #ifdef DEBUG_LITREDUNDANT
    cout << "Litredundant called" << endl;
    #endif

    analyze_stack.clear();
    analyze_stack.push(p);

    size_t top = toClear.size();
    while (!analyze_stack.empty()) {
        #ifdef DEBUG_LITREDUNDANT
        cout << "At point in litRedundant: " << analyze_stack.top() << endl;
        #endif

        const PropBy reason = varData[analyze_stack.top().var()].reason;
        PropByType type = reason.getType();
        analyze_stack.pop();

        //Must have a reason
        assert(!reason.isNULL());

        size_t size;
        Clause* cl = NULL;
        switch (type) {
            case clause_t:
                cl = clAllocator->getPointer(reason.getClause());
                size = cl->size()-1;
                break;

            case binary_t:
                size = 1;
                break;

            case tertiary_t:
                size = 2;
                break;

            case null_clause_t:
                release_assert(false);
                exit(-1);
                break;
        }

        for (size_t i = 0
            ; i < size
            ; i++
        ) {
            Lit p;
            switch (type) {
                case clause_t:
                    p = (*cl)[i+1];
                    break;

                case binary_t:
                    p = reason.lit1();
                    break;

                case tertiary_t:
                    if (i == 0) {
                        p = reason.lit1();
                    } else {
                        p = reason.lit2();
                    }
                    break;

                case null_clause_t:
                    release_assert(false);
                    exit(-1);
                    break;
            }
            stats.recMinimCost++;

            if (!seen[p.var()] && varData[p.var()].level > 0) {
                if (!varData[p.var()].reason.isNULL()
                    && (abstractLevel(p.var()) & abstract_levels) != 0
                ) {
                    seen[p.var()] = 1;
                    analyze_stack.push(p);
                    toClear.push_back(p);
                } else {
                    //Return to where we started before function executed
                    for (size_t j = top; j < toClear.size(); j++) {
                        seen[toClear[j].var()] = 0;
                    }
                    toClear.resize(top);

                    return false;
                }
            }
        }
    }

    return true;
}


bool Searcher::subset(const vector<Lit>& A, const Clause& B)
{
    //Set seen
    for (uint32_t i = 0; i != B.size(); i++)
        seen[B[i].toInt()] = 1;

    bool ret = true;
    for (uint32_t i = 0; i != A.size(); i++) {
        if (!seen[A[i].toInt()]) {
            ret = false;
            break;
        }
    }

    //Clear seen
    for (uint32_t i = 0; i != B.size(); i++)
        seen[B[i].toInt()] = 0;

    return ret;
}

/**
@brief Specialized analysis procedure to express the final conflict in terms of assumptions.
Calculates the (possibly empty) set of assumptions that led to the assignment of 'p', and
stores the result in 'out_conflict'.
*/
void Searcher::analyzeFinal(const Lit p, vector<Lit>& out_conflict)
{
    out_conflict.clear();
    out_conflict.push_back(p);

    if (decisionLevel() == 0)
        return;

    seen[p.var()] = 1;

    for (int32_t i = (int32_t)trail.size()-1; i >= (int32_t)trail_lim[0]; i--) {
        const Var x = trail[i].var();
        if (!seen[x])
            break;

        if (varData[x].reason.isNULL()) {
            assert(varData[x].level > 0);
            out_conflict.push_back(~trail[i]);
        } else {
            PropBy confl = varData[x].reason;
            switch(confl.getType()) {
                case tertiary_t : {
                    const Lit lit2 = confl.lit2();
                    if (varData[lit2.var()].level > 0)
                        seen[lit2.var()] = 1;

                    //Intentionally no break, since tertiary is similar to binary
                }

                case binary_t : {
                    const Lit lit1 = confl.lit1();
                    if (varData[lit1.var()].level > 0)
                        seen[lit1.var()] = 1;
                    break;
                }

                case clause_t : {
                    const Clause& cl = *clAllocator->getPointer(confl.getClause());
                    for (uint32_t j = 1, size = cl.size(); j < size; j++) {
                        if (varData[cl[j].var()].level > 0)
                            seen[cl[j].var()] = 1;
                    }
                    break;
                }

                case null_clause_t :
                    assert(false && "Incorrect analyzeFinal");
                    break;
            }
        }
        seen[x] = 0;
    }

    seen[p.var()] = 0;
}

/**
@brief Search for a model

Limits: must be below the specified number of conflicts and must keep the
number of learnt clauses below the provided limit

Use negative value for 'nof_conflicts' or 'nof_learnts' to indicate infinity.

Output: 'l_True' if a partial assigment that is consistent with respect to the
clauseset is found. If all variables are decision variables, this means
that the clause set is satisfiable. 'l_False' if the clause set is
unsatisfiable. 'l_Undef' if the bound on number of conflicts is reached.
*/
lbool Searcher::search(uint64_t* geom_max)
{
    assert(ok);

    //Stats reset & update
    if (params.update)
        stats.numRestarts ++;
    agility.reset(conf.agilityLimit);

    hist.clear();

    //Debug
    #ifdef VERBOSE_DEBUG
    cout << "c started Searcher::search()" << endl;
    #endif //VERBOSE_DEBUG

    //Loop until restart or finish (SAT/UNSAT)
    #ifdef STATS_NEEDED
    bool lastWasConflict = false;
    #endif
    while (true) {
        assert(ok);
        Lit failed;
        PropBy confl;

        //If decision level==1, then do hyperbin & transitive reduction
        if (conf.otfHyperbin && decisionLevel() == 1) {
            stats.advancedPropCalled++;
            solver->varData[trail.back().var()].depth = 0;
            failed = propagateFullBFS();
            if (failed != lit_Undef) {
                #ifdef DRUP
                if (drup) {
                    (*drup)
                    << (~failed)
                    << " 0\n";
                }
                #endif

                //Update conflict stats
                stats.learntUnits++;
                stats.conflStats.numConflicts++;
                stats.conflStats.update(lastConflictCausedBy);
                #ifdef STATS_NEEDED
                hist.conflictAfterConflict.push(lastWasConflict);
                lastWasConflict = true;
                #endif

                cancelUntil(0);
                stats.litsLearntNonMin += 1;
                stats.litsLearntFinal += 1;
                #ifdef STATS_NEEDED
                propStats.propsUnit++;
                #endif
                stats.hyperBinAdded += hyperBinResAll();
                std::pair<size_t, size_t> tmp = removeUselessBins();
                stats.transReduRemIrred += tmp.first;
                stats.transReduRemRed += tmp.second;
                solver->enqueue(~failed);

                if (!ok)
                    return l_False;

                continue;
            }

            //Update cache
            vector<Lit> toEnqueue;
            size_t numElems = trail.size() - trail_lim[0];
            if (conf.doCache
                && numElems <= conf.cacheUpdateCutoff
            ) {
                for (int64_t c = trail.size()-1; c > (int64_t)trail_lim[0]; c--) {
                    const Lit thisLit = trail[c];
                    const Lit ancestor = varData[thisLit.var()].reason.getAncestor();
                    assert(thisLit != trail[trail_lim[0]]);
                    const bool learntStep = varData[thisLit.var()].reason.getLearntStep();

                    assert(ancestor != lit_Undef);
                    bool taut = solver->implCache[(~ancestor).toInt()].merge(
                        solver->implCache[(~thisLit).toInt()].lits
                        , thisLit
                        , learntStep
                        , ancestor.var()
                        , solver->seen
                    );

                    //There is an ~ancestor V OTHER, ~ancestor V ~OTHER
                    //So enqueue ~ancestor
                    if (taut
                        && (solver->varData[ancestor.var()].elimed == ELIMED_NONE
                            || solver->varData[ancestor.var()].elimed == ELIMED_QUEUED_VARREPLACER)
                    ) {
                        toEnqueue.push_back(~ancestor);
                        #ifdef DRUP
                        if (drup) {
                            (*drup)
                            << (~ancestor) << " 0\n";
                        }
                        #endif
                    }
                }
            }

            //Lit lit = trail[trail_lim[0]];
            stats.hyperBinAdded += hyperBinResAll();
            std::pair<size_t, size_t> tmp = removeUselessBins();
            stats.transReduRemIrred += tmp.first;
            stats.transReduRemRed += tmp.second;

            //There are things to enqueue at top-level
            if (!toEnqueue.empty()) {
                solver->cancelUntil(0);
                bool ret = solver->enqueueThese(toEnqueue);
                if (!ret) {
                    return l_False;
                }

                //Start from beginning
                continue;
            }

        } else {
            //Decision level is higher than 1, so must do normal propagation
            confl = propagate(solver
                #ifdef STATS_NEEDED
                , &hist.watchListSizeTraversed
                //, &hist.litPropagatedSomething
                #endif
            );
        }

        #ifdef VERBOSE_DEBUG
        cout << "c Searcher::search() has finished propagation" << endl;
        #endif //VERBOSE_DEBUG

        if (!confl.isNULL()) {
            //Update conflict stats based on lastConflictCausedBy
            stats.conflStats.update(lastConflictCausedBy);

            //If restart is needed, set it as so
            checkNeedRestart(geom_max);
            #ifdef STATS_NEEDED
            hist.conflictAfterConflict.push(lastWasConflict);
            lastWasConflict = true;
            #endif

            if (!handle_conflict(confl))
                return l_False;

        } else {
            assert(ok);
            #ifdef STATS_NEEDED
            lastWasConflict = false;
            #endif

            //If restart is needed, restart here
            if (params.needToStopSearch
                || sumConflicts() > solver->getNextCleanLimit()
            ) {
                cancelUntil(0);
                return l_Undef;
            }

            const lbool ret = new_decision();
            if (ret != l_Undef)
                return ret;
        }
    }
}

/**
@brief Picks a new decision variable to branch on

@returns l_Undef if it should restart instead. l_False if it reached UNSAT
         (through simplification)
*/
lbool Searcher::new_decision()
{
    Lit next = lit_Undef;
    while (decisionLevel() < assumptions.size()) {
        // Perform user provided assumption:
        Lit p = assumptions[decisionLevel()];
        if (value(p) == l_True) {
            // Dummy decision level:
            newDecisionLevel();
        } else if (value(p) == l_False) {
            analyzeFinal(~p, conflict);
            return l_False;
        } else {
            stats.decisionsAssump++;
            next = p;
            break;
        }
    }

    if (next == lit_Undef) {
        // New variable decision:
        next = pickBranchLit();

        //No decision taken, because it's SAT
        if (next == lit_Undef)
            return l_True;

        //Update stats
        stats.decisions++;
        #ifdef STATS_NEEDED
        if (next.sign()) {
            varData[next.var()].stats.negDecided++;
        } else {
            varData[next.var()].stats.posDecided++;
        }
        #endif
    }

    // Increase decision level and enqueue 'next'
    assert(value(next) == l_Undef);
    newDecisionLevel();
    enqueue(next);

    return l_Undef;
}

void Searcher::checkNeedRestart(uint64_t* geom_max)
{
    if (needToInterrupt)  {
        if (conf.verbosity >= 3)
            cout << "c needToInterrupt is set, restartig as soon as possible!" << endl;
        params.needToStopSearch = true;
    }

    switch (params.rest_type) {

        case no_restart:
            //Just don't restart no matter what
            break;

        case geom_restart:
            assert(geom_max != NULL);
            if (params.conflictsDoneThisRestart > *geom_max)
                params.needToStopSearch = true;

            break;

        case glue_restart:
            if (hist.glueHist.isvalid()
                && 0.95*hist.glueHist.avg() > hist.glueHistLT.avg()
            ) {
                //If not optimising for UNSAT, we might have SAT, indicated by
                //a long trail
                if (!conf.optimiseUnsat
                    && hist.trailDepthHist.isvalid()
                    && hist.trailDepthHist.avg() > hist.trailDepthHistLT.avg()*1.8
                ) {
                    //Nothing
                } else {
                    params.needToStopSearch = true;
                }
            }

            break;

        case glue_agility_restart:
            if (hist.glueHist.isvalid()
                && 0.95*hist.glueHist.avg() > hist.glueHistLT.avg()
                && agility.getAgility() < conf.agilityLimit
            ) {
                params.numAgilityNeedRestart++;
                if (params.numAgilityNeedRestart > conf.agilityViolationLimit) {
                    params.needToStopSearch = true;
                }
            } else {
                //Reset counter
                params.numAgilityNeedRestart = 0;
            }

            break;

        case agility_restart:
            if (agility.getAgility() < conf.agilityLimit) {
                params.numAgilityNeedRestart++;
                if (params.numAgilityNeedRestart > conf.agilityViolationLimit) {
                    params.needToStopSearch = true;
                }
            } else {
                //Reset counter
                params.numAgilityNeedRestart = 0;
            }

            break;
        default:
            assert(false && "This should not happen, auto decision is make before this point");
            break;
    }

    //If agility was used and it's too high, print it if need be
    if (conf.verbosity >= 4
        && params.needToStopSearch
        && (conf.restartType == agility_restart
            || conf.restartType == glue_agility_restart)
    ) {
        cout << "c Agility was too low, restarting asap";
        printAgilityStats();
        cout << endl;
    }

    //Conflict limit reached?
    if (params.conflictsDoneThisRestart > params.conflictsToDo) {
        if (conf.verbosity >= 3)
            cout
            << "c Over limit of conflicts for this restart"
            << " -- restarting as soon as possible!" << endl;
        params.needToStopSearch = true;
    }
}

/**
@brief Handles a conflict that we reached through propagation

Handles on-the-fly subsumption: the OTF subsumption check is done in
conflict analysis, but this is the code that actually replaces the original
clause with that of the shorter one
@returns l_False if UNSAT
*/
bool Searcher::handle_conflict(PropBy confl)
{
    #ifdef VERBOSE_DEBUG
    cout << "Handling conflict" << endl;
    #endif

    //Stats
    uint32_t backtrack_level;
    uint32_t glue;
    ResolutionTypes<uint16_t> resolutions;
    stats.conflStats.numConflicts++;
    params.conflictsDoneThisRestart++;
    if (conf.doPrintConflDot)
        genConfGraph(confl);

    if (decisionLevel() == 0) {
        return false;
    }

    Clause* cl = analyze(
        confl
        , backtrack_level  //return backtrack level here
        , glue             //return glue here
        , resolutions   //return number of resolutions made here
        , false
    );

    #ifdef DRUP
    if (conf.verbosity >= 6) {
        cout
        << "c learnt clause: "
        << learnt_clause
        << endl;
    }
    if (drup) {
        (*drup)
        << learnt_clause << " 0\n";
    }
    #endif

    size_t orig_trail_size = trail.size();
    if (params.update) {
        //Update history
        hist.trailDepthHist.push(trail.size() - trail_lim[0]);
        hist.trailDepthHistLT.push(trail.size() - trail_lim[0]);

        hist.branchDepthHist.push(decisionLevel());
        hist.branchDepthHistLT.push(decisionLevel());

        hist.branchDepthDeltaHist.push(decisionLevel() - backtrack_level);
        hist.branchDepthDeltaHistLT.push(decisionLevel() - backtrack_level);

        hist.glueHist.push(glue);
        hist.glueHistLT.push(glue);

        hist.conflSizeHist.push(learnt_clause.size());
        hist.conflSizeHistLT.push(learnt_clause.size());

        hist.agilityHist.push(agility.getAgility());
        hist.agilityHistLT.push(agility.getAgility());

        hist.numResolutionsHist.push(resolutions.sum());
        hist.numResolutionsHistLT.push(resolutions.sum());

        #ifdef STATS_NEEDED
        if (conf.doSQL) {
            if (sumConflicts() % conf.dumpClauseDistribPer == 0) {
                printClauseDistribSQL();

                //Clear distributions
                std::fill(clauseSizeDistrib.begin(), clauseSizeDistrib.end(), 0);
                std::fill(clauseGlueDistrib.begin(), clauseGlueDistrib.end(), 0);
                for(size_t i = 0; i < sizeAndGlue.shape()[0]; i++) {
                    for(size_t i2 = 0; i2 < sizeAndGlue.shape()[1]; i2++) {
                        sizeAndGlue[i][i2] = 0;
                    }
                }
            }

            //Add this new clause to distributions
            uint32_t truncSize = std::min<uint32_t>(learnt_clause.size(), conf.dumpClauseDistribMaxSize-1);
            uint32_t truncGlue = std::min<uint32_t>(glue, conf.dumpClauseDistribMaxGlue-1);
            clauseSizeDistrib[truncSize]++;
            clauseGlueDistrib[truncGlue]++;
            sizeAndGlue[truncSize][truncGlue]++;
        }
        #endif
    }
    cancelUntil(backtrack_level);
    if (params.update) {
        hist.trailDepthDeltaHist.push(orig_trail_size - trail.size());
    }

    //Hande long OTF subsumption
    for(size_t i = 0; i < toAttachLater.size(); i++) {
        const ClOffset offset = toAttachLater[i];
        Clause& cl = *solver->clAllocator->getPointer(offset);
        cl.stats.numConfl += conf.rewardShortenedClauseWithConfl;

        //Find the l_Undef
        size_t at = std::numeric_limits<size_t>::max();
        for(size_t i2 = 0; i2 < cl.size(); i2++) {
            if (value(cl[i2]) == l_Undef) {
                at = i2;
                break;
            }
        }
        assert(at != std::numeric_limits<size_t>::max());
        std::swap(cl[at], cl[0]);
        assert(value(cl[0]) == l_Undef);

        //Find another l_Undef or an l_True
        at = 0;
        for(size_t i2 = 1; i2 < cl.size(); i2++) {
            if (value(cl[i2]) == l_Undef || value(cl[i2]) == l_True) {
                at = i2;
                break;
            }
        }
        assert(cl.size() > 3);

        if (at == 0) {
            //If none found, we have a propagating clause_t

            if (conf.otfHyperbin && decisionLevel() == 1) {
                addHyperBin(cl[0], cl);
            } else {
                enqueue(cl[0], decisionLevel() == 0 ? PropBy() : PropBy(offset));
                #ifdef DRUP
                if (drup && decisionLevel() == 0) {
                    *(drup)
                    << cl[0]
                    << " 0\n";
                }
                #endif
            }
        } else {
            //We have a non-propagating clause

            std::swap(cl[at], cl[1]);
            assert(value(cl[1]) == l_Undef || value(cl[1]) == l_True);
        }
        solver->attachClause(cl, false);
        cl.reCalcAbstraction();
    }
    toAttachLater.clear();

    //Handle implicit OTF subsumption
    for(vector<OTFClause>::iterator
        it = otfMustAttach.begin(), end = otfMustAttach.end()
        ; it != end
        ; it++
    ) {
        assert(it->size > 1);
        //Find the l_Undef
        size_t at = std::numeric_limits<size_t>::max();
        for(size_t i2 = 0; i2 < it->size; i2++) {
            if (value(it->lits[i2]) == l_Undef) {
                at = i2;
                break;
            }
        }
        assert(at != std::numeric_limits<size_t>::max());
        std::swap(it->lits[at], it->lits[0]);
        assert(value(it->lits[0]) == l_Undef);

        //Find another l_Undef or an l_True
        at = 0;
        for(size_t i2 = 1; i2 < it->size; i2++) {
            if (value(it->lits[i2]) == l_Undef
                || value(it->lits[i2]) == l_True
            ) {
                at = i2;
                break;
            }
        }

        if (at == 0) {
            //If none found, we have a propagation
            if (conf.otfHyperbin && decisionLevel() == 1) {
                if (it->size == 2) {
                    enqueueComplex(it->lits[0], ~it->lits[1], true);
                } else {
                    addHyperBin(it->lits[0], it->lits[1], it->lits[2]);
                }
            } else {
                //Calculate reason
                PropBy by = PropBy();

                //if decision level is non-zero, we have to be more careful
                if (decisionLevel() != 0) {
                    if (it->size == 2) {
                        by = PropBy(it->lits[1]);
                    } else {
                        by = PropBy(it->lits[1], it->lits[2]);
                    }
                }

                //Enqueue this literal, finally
                enqueue(
                    it->lits[0]
                    , by
                );
                #ifdef DRUP
                if (drup && decisionLevel() == 0) {
                    *(drup)
                    << it->lits[0] << " 0\n";
                }
                #endif
            }
        } else {
            //We have a non-propagating clause
            std::swap(it->lits[at], it->lits[1]);
            assert(value(it->lits[1]) == l_Undef
                || value(it->lits[1]) == l_True
            );

            //Attach new binary/tertiary clause
            if (it->size == 2) {
                solver->attachBinClause(it->lits[0], it->lits[1], true);
            } else {
                solver->attachTriClause(it->lits[0], it->lits[1], it->lits[2], true);
            }
        }
    }
    otfMustAttach.clear();

    //Debug
    #ifdef VERBOSE_DEBUG
    cout
    << "Learning:" << learnt_clause
    << endl
    << "reverting var " << learnt_clause[0].var()+1
    << " to " << !learnt_clause[0].sign()
    << endl;
    #endif
    assert(value(learnt_clause[0]) == l_Undef);

    //Set up everything to get the clause
    glue = std::min<uint32_t>(glue, std::numeric_limits<uint16_t>::max());

    //Is there on-the-fly subsumption?
    if (cl == NULL) {

        //Otherwise, we will attach it directly, below
        if (learnt_clause.size() > 3) {
            cl = clAllocator->Clause_new(learnt_clause, Searcher::sumConflicts());
            cl->makeLearnt(glue);
            ClOffset offset = clAllocator->getOffset(cl);
            solver->longRedCls.push_back(offset);
        }

    } else {
        //Detach & delete subsumed clause
        #ifdef DRUP
        if (conf.verbosity >= 6) {
            cout
            << "Detaching OTF subsumed (LAST) clause:"
            << *cl
            << endl;
        }
        #endif
        solver->detachClause(*cl);

        //Shrink clause
        for (uint32_t i = 0; i < learnt_clause.size(); i++) {
            (*cl)[i] = learnt_clause[i];
        }
        cl->shrink(cl->size() - learnt_clause.size());
        assert(cl->size() == learnt_clause.size());

        //Update stats
        if (cl->learnt() && cl->stats.glue > glue) {
            cl->stats.glue = glue;
        }
        cl->stats.numConfl += conf.rewardShortenedClauseWithConfl;
    }

    //Attach new clause
    switch (learnt_clause.size()) {
        case 1:
            //Unitary learnt
            stats.learntUnits++;
            enqueue(learnt_clause[0]);
            assert(backtrack_level == 0 && "Unit clause learnt, so must cancel until level 0");

            #ifdef STATS_NEEDED
            propStats.propsUnit++;
            #endif

            break;
        case 2:
            //Binary learnt
            stats.learntBins++;
            solver->attachBinClause(learnt_clause[0], learnt_clause[1], true);
            if (conf.otfHyperbin && decisionLevel() == 1)
                enqueueComplex(learnt_clause[0], ~learnt_clause[1], true);
            else
                enqueue(learnt_clause[0], PropBy(learnt_clause[1]));

            #ifdef STATS_NEEDED
            propStats.propsBinRed++;
            #endif
            break;

        case 3:
            //3-long learnt
            stats.learntTris++;
            std::sort((&learnt_clause[0])+1, (&learnt_clause[0])+3);
            solver->attachTriClause(learnt_clause[0], learnt_clause[1], learnt_clause[2], true);

            if (conf.otfHyperbin && decisionLevel() == 1)
                addHyperBin(learnt_clause[0], learnt_clause[1], learnt_clause[2]);
            else
                enqueue(learnt_clause[0], PropBy(learnt_clause[1], learnt_clause[2]));

            #ifdef STATS_NEEDED
            propStats.propsTriRed++;
            #endif
            break;

        default:
            //Normal learnt
            cl->stats.resolutions = resolutions;
            stats.learntLongs++;
            std::sort(learnt_clause.begin()+1, learnt_clause.end(), PolaritySorter(varData));
            solver->attachClause(*cl);
            if (conf.otfHyperbin && decisionLevel() == 1)
                addHyperBin(learnt_clause[0], *cl);
            else
                enqueue(learnt_clause[0], PropBy(clAllocator->getOffset(cl)));

            #ifdef STATS_NEEDED
            propStats.propsLongRed++;
            #endif

            break;
    }

    varDecayActivity();
    decayClauseAct();

    return true;
}

void Searcher::genRandomVarActMultDiv()
{
    uint32_t tosubstract = conf.var_inc_variability-mtrand.randInt(2*conf.var_inc_variability);
    var_inc_multiplier = conf.var_inc_multiplier - tosubstract;
    var_inc_divider = conf.var_inc_divider - tosubstract;

    if (conf.verbosity >= 1) {
        cout
        << "c Using var act-multip " << var_inc_multiplier
        << " instead of standard " << (conf.var_inc_multiplier)
        << " and act-divider " << var_inc_divider
        << " instead of standard " << (conf.var_inc_divider)
        << endl;
    }
}

/**
@brief Initialises model, restarts, learnt cluause cleaning, burst-search, etc.
*/
void Searcher::resetStats()
{
    assert(ok);

    //Set up time
    startTime = cpuTime();

    //Clear up previous stuff like model, final conflict
    conflict.clear();

    //About vars
    #ifdef STATS_NEEDED
    for(vector<VarData>::iterator
        it = varData.begin(), end = varData.end()
        ; it != end
        ; it++
    ) {
        it->stats.reset();
    }

    //Clause data
    clauseSizeDistrib.resize(conf.dumpClauseDistribMaxSize, 0);
    clauseGlueDistrib.resize(conf.dumpClauseDistribMaxGlue, 0);
    sizeAndGlue.resize(boost::extents[conf.dumpClauseDistribMaxSize][conf.dumpClauseDistribMaxGlue]);
    for(size_t i = 0; i < sizeAndGlue.shape()[0]; i++) {
        for(size_t i2 = 0; i2 < sizeAndGlue.shape()[1]; i2++) {
            sizeAndGlue[i][i2] = 0;
        }
    }
    #endif

    //Rest solving stats
    stats.clear();
    propStats.clear();
    #ifdef STATS_NEEDED
    lastSQLPropStats = propStats;
    lastSQLGlobalStats = stats;
    #endif

    //Set already set vars
    origTrailSize = trail.size();

    order_heap.filter(VarFilter(this, solver));
    lastCleanZeroDepthAssigns = trail.size();
}

lbool Searcher::burstSearch()
{
    //Print what we will be doing
    if (conf.verbosity >= 2) {
        cout
        << "c Doing burst search for " << conf.burstSearchLen << " conflicts"
        << endl;
    }
    const size_t numUnitsUntilNow = stats.learntUnits;
    const size_t numBinsUntilNow = stats.learntBins;
    #ifdef STATS_NEEDED
    const size_t numTriLHBRUntilNow = propStats.triLHBR;
    const size_t numLongLHBRUntilNow = propStats.longLHBR;
    #endif

    //Save old config
    const double backup_rand = conf.random_var_freq;
    const int backup_polar_mode = conf.polarity_mode;
    uint32_t backup_var_inc_divider = var_inc_divider;
    uint32_t backup_var_inc_multiplier = var_inc_multiplier;

    //Set burst config
    conf.random_var_freq = 1;
    conf.polarity_mode = polarity_rnd;
    var_inc_divider = 1;
    var_inc_multiplier = 1;

    //Do burst
    params.clear();
    params.conflictsToDo = conf.burstSearchLen;
    params.rest_type = no_restart;
    lbool status = search(NULL);

    //Restore config
    conf.random_var_freq = backup_rand;
    conf.polarity_mode = backup_polar_mode;
    var_inc_divider = backup_var_inc_divider;
    var_inc_multiplier = backup_var_inc_multiplier;

    //Print what has happened
    if (conf.verbosity >= 2) {
        cout
        << "c "
        << conf.burstSearchLen << "-long burst search "
        << " learnt units:" << (stats.learntUnits - numUnitsUntilNow)
        << " learnt bins: " << (stats.learntBins - numBinsUntilNow)
        #ifdef STATS_NEEDED
        << " LHBR: "
        << (propStats.triLHBR + propStats.longLHBR - numLongLHBRUntilNow - numTriLHBRUntilNow)
        #endif
        << endl;
    }

    return status;
}

void Searcher::printRestartHeader() const
{
    cout
    << "c"
    << " " << std::setw(5) << "rest"
    << " " << std::setw(5) << "conf"
    << " " << std::setw(7) << "freevar"
    << " " << std::setw(5) << "IrrL"
    << " " << std::setw(5) << "IrrT"
    << " " << std::setw(5) << "IrrB"
    << " " << std::setw(5) << "l/c"
    << " " << std::setw(5) << "RedL"
    << " " << std::setw(5) << "RedT"
    << " " << std::setw(5) << "RedB"
    << " " << std::setw(5) << "l/c"
    << endl;
}

void Searcher::printRestartStats() const
{
    printBaseStats();
    if (conf.printFullStats) {
        solver->printClauseStats();
        hist.print();
    } else {
        solver->printClauseStats();
    }

    cout << endl;
}

void Searcher::printBaseStats() const
{
    cout
    << "c"
    //<< omp_get_thread_num()
    << " " << std::setw(5) << sumRestarts();

    if (sumConflicts() >  20000) {
        cout
        << " " << std::setw(4) << sumConflicts()/1000 << "K";
    } else {
        cout
        << " " << std::setw(5) << sumConflicts();
    }

    cout
    << " " << std::setw(7) << solver->getNumFreeVars()
    ;
}

struct MyInvSorter {
    bool operator()(size_t num, size_t num2)
    {
        return num > num2;
    }
};

struct MyPolarData
{
    MyPolarData (size_t _pos, size_t _neg, size_t _flipped) :
        pos(_pos)
        , neg(_neg)
        , flipped(_flipped)
    {}

    size_t pos;
    size_t neg;
    size_t flipped;

    bool operator<(const MyPolarData& other) const
    {
        return (pos + neg) > (other.pos + other.neg);
    }
};

/*void Searcher::printVarStatsSQL()
{
    vector<MyPolarData> polarData;
    for(size_t i = 0; i < varData.size(); i++) {
        if (varData[i].posPolarSet == 0 && varData[i].negPolarSet == 0)
            continue;

        polarData.push_back(MyPolarData(
            varData[i].posPolarSet
            , varData[i].negPolarSet
            , varData[i].flippedPolarity
        ));
    }
    std::sort(polarData.begin(), polarData.end());

    for(size_t i = 0; i < polarData.size(); i++) {
        solver->sqlFile
        << "insert into `polarSet`"
        << "("
        << " `runID`, `simplifications`"
        << " , `order`, `pos`, `neg`, `total`, `flipped`"
        << ")"
        << " values ("
        //Position
        << "  " << solver->getSolveStats().runID
        << ", " << solver->getSolveStats().numSimplify
        //Data
        << ", " << i
        << ", " << polarData[i].pos
        << ", " << polarData[i].neg
        << ", " << polarData[i].pos + polarData[i].neg
        << ", " << polarData[i].flipped
        << " );" << endl;
    }
}*/

#ifdef STATS_NEEDED
void Searcher::calcVariancesLT(
    double& avgDecLevelVar
    , double& avgTrailLevelVar
) {
    double sumVarDec = 0;
    double sumVarTrail = 0;
    size_t num = 0;
    size_t maxDecLevel = 0;
    for(size_t i = 0; i < varDataLT.size(); i++) {
        if (varDataLT[i].posPolarSet || varDataLT[i].negPolarSet) {
            sumVarDec += sqrt(varDataLT[i].decLevelHist.var());
            sumVarTrail += sqrt(varDataLT[i].decLevelHist.var());
            maxDecLevel = std::max<size_t>(varDataLT[i].decLevelHist.getMax(), maxDecLevel);
            num++;
        }
    }

    avgDecLevelVar = sumVarDec/(double)num;
    avgTrailLevelVar = sumVarTrail/(double)num;
}

void Searcher::calcVariances(
    double& avgDecLevelVar
    , double& avgTrailLevelVar
) {
    double sumVarDec = 0;
    double sumVarTrail = 0;
    size_t num = 0;
    size_t maxDecLevel = 0;
    for(size_t i = 0; i < varData.size(); i++) {
        if (varData[i].stats.posPolarSet || varData[i].stats.negPolarSet) {
            sumVarDec += sqrt(varData[i].stats.decLevelHist.var());
            sumVarTrail += sqrt(varData[i].stats.decLevelHist.var());
            maxDecLevel = std::max<size_t>(varData[i].stats.decLevelHist.getMax(), maxDecLevel);
            num++;
        }
    }

    avgDecLevelVar = sumVarDec/(double)num;
    avgTrailLevelVar = sumVarTrail/(double)num;
}

void Searcher::printRestartSQL()
{
    //Propagation stats
    PropStats thisPropStats = propStats - lastSQLPropStats;
    Stats thisStats = stats - lastSQLGlobalStats;

    //Print variance
    VariableVariance variableVarianceStat;
    calcVariances(variableVarianceStat.avgDecLevelVar, variableVarianceStat.avgTrailLevelVar);
    calcVariancesLT(variableVarianceStat.avgDecLevelVarLT, variableVarianceStat.avgTrailLevelVarLT);

    solver->sqlStats->restart(
        thisPropStats
        , thisStats
        , variableVarianceStat
        , solver
        , this
    );

    lastSQLPropStats = propStats;
    lastSQLGlobalStats = stats;

    //Variable stats
    solver->sqlStats->varDataDump(solver, this, calcVarsToDump(), varData);
}
#endif

struct VarDumpOrder
{
    VarDumpOrder(size_t _var, size_t _polarSetSum) :
        var(_var)
        , polarSetSum(_polarSetSum)
    {}

    size_t var;
    size_t polarSetSum;

    bool operator<(const VarDumpOrder& other) const
    {
        //Order by largest polarSetSum first
        return polarSetSum > other.polarSetSum;
    }
};

#ifdef STATS_NEEDED
vector<Var> Searcher::calcVarsToDump() const
{
    //How much to dump per criteria
    const size_t numToDump = std::min(varData.size(), conf.dumpTopNVars);

    //Collect what needs to be dumped here
    set<Var> todump;

    //Top N vars polarity set
    vector<VarDumpOrder> order;
    for(size_t i = 0; i < varData.size(); i++) {
        if (varData[i].stats.posPolarSet + varData[i].stats.negPolarSet > 0) {
            order.push_back(
                VarDumpOrder(
                    i
                    , varData[i].stats.posPolarSet
                        + varData[i].stats.negPolarSet
                )
            );
        }
    }
    std::sort(order.begin(), order.end());

    //These vars need to be dumped according to above stat
    for(size_t i = 0; i < std::min(numToDump, order.size()); i++) {
        todump.insert(order[i].var);
    }

    //Top N vars, number of times decided on
    order.clear();
    for(size_t i = 0; i < varData.size(); i++) {
        if (varData[i].stats.posDecided + varData[i].stats.negDecided > 0) {
            order.push_back(
                VarDumpOrder(
                    i
                    , varData[i].stats.negDecided
                        + varData[i].stats.posDecided
                )
            );
        }
    }
    std::sort(order.begin(), order.end());

    //These vars need to be dumped according to above stat
    for(size_t i = 0; i < std::min(numToDump, order.size()); i++) {
        todump.insert(order[i].var);
    }

    vector<Var> toDumpVec;
    for(set<Var>::const_iterator
        it = todump.begin(), end = todump.end()
        ; it != end
        ; it++
    ) {
        toDumpVec.push_back(*it);
    }

    return toDumpVec;
}

void Searcher::printClauseDistribSQL()
{
    solver->sqlStats->clauseSizeDistrib(
        sumConflicts()
        , clauseSizeDistrib
    );
    solver->sqlStats->clauseGlueDistrib(
        sumConflicts()
        , clauseGlueDistrib
    );

    solver->sqlStats->clauseSizeGlueScatter(
        sumConflicts()
        , sizeAndGlue
    );
}
#endif

RestartType Searcher::decide_restart_type() const
{
    RestartType rest_type = conf.restartType;
    if (rest_type == auto_restart) {
        if (solver->sumPropStats.propagations == 0) {

            //If no data yet, default to glue_restart
            rest_type = glue_restart;
        } else {
            //Otherwise, choose according to % of pos/neg polarities
            double total = solver->sumPropStats.varSetNeg + solver->sumPropStats.varSetPos;
            double percent = ((double)solver->sumPropStats.varSetNeg)/total*100.0;
            if (percent > 60.0
                || percent < 40.0
            ) {
                rest_type = glue_restart;
            } else {
                rest_type = geom_restart;
            }

            if (conf.verbosity >= 1) {
                cout
                << "c percent of negative polarities set: "
                << std::setprecision(2) << percent
                << " % (this is used to choose restart type)"
                << endl;
            }
        }

        if (conf.verbosity >= 2) {
            cout
            << "c Chose restart type "
            << restart_type_to_string(rest_type)
            << endl;
        }
    }

    return rest_type;
}

void Searcher::check_if_print_restart_stat(const lbool status)
{
    //Print restart stat
    if (conf.verbosity >= 2
        && status == l_Undef
        && ((lastRestartPrint + 800) < stats.conflStats.numConflicts
            || conf.printAllRestarts)
    ) {
        //Print restart output header
        if (lastRestartPrintHeader == 0
            ||(lastRestartPrintHeader + 20000) < stats.conflStats.numConflicts
        ) {
            printRestartHeader();
            lastRestartPrintHeader = stats.conflStats.numConflicts;
        }
        printRestartStats();
        lastRestartPrint = stats.conflStats.numConflicts;
    }
}


/**
@brief The main solve loop that glues everything together

We clear everything needed, pre-simplify the problem, calculate default
polarities, and start the loop. Finally, we either report UNSAT or extend the
found solution with all the intermediary simplifications (e.g. variable
elimination, etc.) and output the solution.
*/
lbool Searcher::solve(const vector<Lit>& assumps, const uint64_t maxConfls)
{
    assert(ok);
    assert(qhead == trail.size());

    if (solver->conf.verbosity >= 6) {
        cout
        << "c Searcher::solve() called"
        << endl;
    }

    assumptions = assumps;
    resetStats();

    //Current solving status
    lbool status = l_Undef;

    //Burst seach
    status = burstSearch();
    if (status == l_Undef) {
        //Restore polarities and activities
        for(size_t i = 0; i < nVars(); i++) {
            varData[i].polarity = solver->getSavedPolarity(i);
            activities[i] = solver->getSavedActivity(i);
        }
        var_inc = solver->getSavedActivityInc();

        //Restore order_heap
        order_heap.clear();
        for(size_t var = 0; var < nVars(); var++) {
            if (solver->decisionVar[var]
                && value(var) == l_Undef
            ) {
                insertVarOrder(var);
            }
        }
        assert(order_heap.heapProperty());

        //Set up data for search
        params.rest_type = decide_restart_type();
        genRandomVarActMultDiv();

        //Set up restart printing status
        lastRestartPrint = stats.conflStats.numConflicts;
        lastRestartPrintHeader = stats.conflStats.numConflicts;
        if (conf.verbosity >= 1) {
            printRestartStats();
        }
    }

    //Search loop final setup
    size_t loopNum = 0;
    uint64_t geom_max = conf.restart_first;
    while (status == l_Undef
        && !needToInterrupt
        && stats.conflStats.numConflicts < maxConfls
        && cpuTime() < conf.maxTime
    ) {
        //Print search loop number if needed
        if (conf.verbosity >= 6) {
            cout
            << "c search loop " << loopNum
            << endl;
        }
        loopNum++;

        assert(stats.conflStats.numConflicts < maxConfls);

        lastRestartConfl = sumConflicts();

        //Set up params
        params.clear();
        params.conflictsToDo = maxConfls-stats.conflStats.numConflicts;
        status = search(&geom_max);
        geom_max *= conf.restart_inc;
        check_if_print_restart_stat(status);

        if (status != l_Undef) {
            if (conf.verbosity >= 6) {
                cout
                << "c Returned status of search() is non-l_Undef at loop "
                << loopNum
                << " confl:"
                << sumConflicts()
                << endl;
            }
            break;
        }

        //Check if we should abort
        if (stats.conflStats.numConflicts >= maxConfls) {
            if (conf.verbosity >= 3) {
                cout
                << "c thread(maxconfl) Trail size: " << trail.size()
                << " over maxConfls"
                << endl;
            }
            break;
        }

        //Check if we should do DBcleaning
        if (sumConflicts() > solver->getNextCleanLimit()) {
            if (conf.verbosity >= 3) {
                cout
                << "c th "
                #ifdef USE_OMP
                << omp_get_thread_num()
                #else
                << "1"
                #endif
                << " cleaning"
                << " getNextCleanLimit(): " << solver->getNextCleanLimit()
                << " numConflicts : " << stats.conflStats.numConflicts
                << " SumConfl: " << sumConflicts()
                << " maxConfls:" << maxConfls
                << " Trail size: " << trail.size() << endl;
            }
            solver->fullReduce();

            genRandomVarActMultDiv();
        }

        //Check if we should do SCC
        //cout << "numNewBinsSinceSCC: " << solver->numNewBinsSinceSCC << endl;
        const size_t newZeroDepthAss = trail.size() - lastCleanZeroDepthAssigns;
        if (newZeroDepthAss > ((double)solver->getNumFreeVars()*0.005))  {
            if (conf.verbosity >= 2) {
                cout << "c newZeroDepthAss : " << newZeroDepthAss  << endl;
            }

            lastCleanZeroDepthAssigns = trail.size();
            solver->clauseCleaner->removeAndCleanAll();
        }

        //Eq-lit finding has been enabled? If so, let's see if there might be
        //a reason to do it
        if (conf.doFindAndReplaceEqLits
            && (solver->binTri.numNewBinsSinceSCC
                > ((double)solver->getNumFreeVars()*conf.sccFindPercent))
        ) {
            if (conf.verbosity >= 1) {
                cout
                << "c new bins since last SCC: "
                << std::setw(2)
                << solver->binTri.numNewBinsSinceSCC
                << " free vars %:"
                << std::fixed << std::setprecision(2) << std::setw(4)
                << (double)solver->binTri.numNewBinsSinceSCC
                /(double)solver->getNumFreeVars()*100.0
                << endl;
            }

            solver->clauseCleaner->removeAndCleanAll();

            //Find eq lits
            if (!solver->sCCFinder->find2LongXors()) {
                status = l_False;
                break;
            }
            lastCleanZeroDepthAssigns = trail.size();

            //If enough new variables have been found to be replaced, replace them
            if (solver->varReplacer->getNewToReplaceVars() > ((double)solver->getNumFreeVars()*0.001)) {
                //Perform equivalent variable replacement
                if (!solver->varReplacer->performReplace()) {
                    status = l_False;
                    break;
                }
            }
        }

        #ifdef STATS_NEEDED
        if (conf.doSQL) {
            printRestartSQL();
        }

        //Update varDataLT
        for(size_t i = 0; i < varData.size(); i++) {
            varDataLT[i].addData(varData[i].stats);
            varData[i].stats.reset();
        }
        #endif
    }

    #ifdef VERBOSE_DEBUG
    if (status == l_True)
        cout << "Solution  is SAT" << endl;
    else if (status == l_False)
        cout << "Solution is UNSAT" << endl;
    else
        cout << "Solutions is UNKNOWN" << endl;
    #endif //VERBOSE_DEBUG

    if (status == l_True) {
        solution = assigns;
    } else if (status == l_False) {
        if (conflict.size() == 0)
            ok = false;
    }
    cancelUntil(0);

    stats.cpu_time = cpuTime() - startTime;
    if (conf.verbosity >= 4) {
        cout << "c Searcher::solve() finished"
        << " status: " << status
        << " solver->getNextCleanLimit(): " << solver->getNextCleanLimit()
        << " numConflicts : " << stats.conflStats.numConflicts
        << " SumConfl: " << sumConflicts()
        << " maxConfls:" << maxConfls
        << endl;
    }

    #ifdef STATS_NEEDED
    if (conf.doSQL) {
        printRestartSQL();
        //printVarStatsSQL();

        //Print clause distib SQL until here
        printClauseDistribSQL();
        std::fill(clauseSizeDistrib.begin(), clauseSizeDistrib.end(), 0);
    }
    #endif

    if (conf.verbosity >= 3) {
        cout << "c ------ THIS ITERATION SOLVING STATS -------" << endl;
        stats.print();
        propStats.print(stats.cpu_time);
        printStatsLine("c props/decision"
            , (double)propStats.propagations/(double)stats.decisions
        );
        printStatsLine("c props/conflict"
            , (double)propStats.propagations/(double)stats.conflStats.numConflicts
        );
        cout << "c ------ THIS ITERATION SOLVING STATS -------" << endl;
    }

    return status;
}

inline int64_t abs64(int64_t a)
{
    if (a < 0) return -a;
    return a;
}

bool Searcher::pickPolarity(const Var var)
{
    switch(conf.polarity_mode) {
        case polarity_false:
            return false;

        case polarity_true:
            return true;

        case polarity_rnd:
            return mtrand.randInt(1);

        case polarity_auto:
            return getStoredPolarity(var);
        default:
            assert(false);
    }

    return true;
}

/**
@brief Picks a branching variable and its value (True/False)

We do three things here:
-# Try to do random decision (rare, less than 2%)
-# Try acitivity-based decision

Then, we pick a sign (True/False):
\li If we are in search-burst mode ("simplifying" is set), we pick a sign
totally randomly
\li Otherwise, we simply take the saved polarity
*/
Lit Searcher::pickBranchLit()
{
    #ifdef VERBOSE_DEBUG
    cout << "decision level: " << decisionLevel() << " ";
    #endif

    Lit next = lit_Undef;

    // Random decision:
    double rand = mtrand.randDblExc();
    if (rand < conf.random_var_freq
        && !order_heap.empty()
    ) {
        const Var next_var = order_heap[mtrand.randInt(order_heap.size()-1)];
        if (value(next_var) == l_Undef
            && solver->decisionVar[next_var]
        ) {
            stats.decisionsRand++;
            next = Lit(next_var, !pickPolarity(next_var));
        }
    }

    // Activity based decision:
    while (next == lit_Undef
      || value(next.var()) != l_Undef
      || !solver->decisionVar[next.var()]
    ) {
        //There is no more to branch on. Satisfying assignment found.
        if (order_heap.empty()) {
            next = lit_Undef;
            break;
        }

        const Var next_var = order_heap.removeMin();
        bool oldPolar = getStoredPolarity(next_var);
        bool newPolar = pickPolarity(next_var);
        next = Lit(next_var, !newPolar);
        if (oldPolar != newPolar) {
            stats.decisionFlippedPolar++;
        }
    }

    //Flip polaritiy if need be
    if (next != lit_Undef) {
        next ^= (mtrand.randInt(conf.flipPolarFreq*hist.branchDepthDeltaHistLT.avg()) == 1);
    }

    //Try to update to dominator
    if (next != lit_Undef
        && (mtrand.randInt(conf.dominPickFreq) == 1)
    ) {
        Lit lit2 = lit_Undef;
        /*if (conf.doStamp) {
            //Use timestamps
            lit2 = stamp.tstamp[next.toInt()].dominator[STAMP_RED];
        } else*/ if (conf.doCache) {
            //Use cache
            lit2 = solver->litReachable[next.toInt()].lit;
        }

        //Update
        if (lit2 != lit_Undef
            && value(lit2.var()) == l_Undef
            && solver->decisionVar[lit2.var()]
        ) {
            //Dominator may not actually dominate this variabe
            //So just to be sure, re-insert it
            insertVarOrder(next.var());

            //Save this literal & sign
            next = lit2;
        }
    }

    //No vars in heap: solution found
    #ifdef VERBOSE_DEBUG
    if (next == lit_Undef) {
        cout << "SAT!" << endl;
    } else {
        cout << "decided on: " << next << endl;
    }
    #endif

    assert(next == lit_Undef || solver->decisionVar[next.var()]);
    return next;
}

/**
@brief Performs on-the-fly self-subsuming resolution

Only uses binary and tertiary clauses already in the watchlists in native
form to carry out the forward-self-subsuming resolution
*/
void Searcher::minimiseLearntFurther(vector<Lit>& cl)
{
    stats.furtherShrinkAttempt++;

    //Set all literals' seen[lit] = 1 in learnt clause
    //We will 'clean' the learnt clause by setting these to 0
    for (vector<Lit>::const_iterator
        it = cl.begin(), end = cl.end()
        ; it != end
        ; it++
    ) {
        seen[it->toInt()] = 1;
    }

    //Do cache-based minimisation and watchlist-based minimisation
    //one-by-one on the literals. Order could be enforced to get smallest
    //clause, but it doesn't really matter, I think
    size_t timeSpent = 0;
    for (vector<Lit>::iterator
        l = cl.begin(), end = cl.end()
        ; l != end && timeSpent < conf.moreMinimLimit
        ; l++
    ) {
        if (seen[l->toInt()] == 0)
            continue;

        Lit lit = *l;

        if (conf.doCache) {
            const TransCache& cache1 = solver->implCache[l->toInt()];
            timeSpent += cache1.lits.size()/2;
            for (vector<LitExtra>::const_iterator
                it = cache1.lits.begin(), end2 = cache1.lits.end()
                ; it != end2
                ; it++
            ) {
                if (seen[(~(it->getLit())).toInt()]) {
                    stats.cacheShrinkedClause++;
                    seen[(~(it->getLit())).toInt()] = 0;
                }
            }
        }

        //Watchlist-based minimisation
        const vec<Watched>& ws = watches[lit.toInt()];
        for (vec<Watched>::const_iterator
            i = ws.begin()
            , end = ws.end()
            ; i != end && timeSpent < conf.moreMinimLimit
            ; i++
        ) {
            timeSpent++;
            if (i->isBinary()) {
                if (seen[(~i->lit1()).toInt()]) {
                    stats.binTriShrinkedClause++;
                    seen[(~i->lit1()).toInt()] = 0;
                }
                continue;
            }

            if (i->isTri()) {
                if (seen[i->lit2().toInt()]) {
                    if (seen[(~i->lit1()).toInt()]) {
                        stats.binTriShrinkedClause++;
                        seen[(~i->lit1()).toInt()] = 0;
                    }
                }
                if (seen[i->lit1().toInt()]) {
                    if (seen[(~i->lit2()).toInt()]) {
                        stats.binTriShrinkedClause++;
                        seen[(~i->lit2()).toInt()] = 0;
                    }
                }
            }
        }
    }

    //Finally, remove the literals that have seen[literal] = 0
    //Here, we can count do stats, etc.
    bool changedClause  = false;
    vector<Lit>::iterator i = cl.begin();
    vector<Lit>::iterator j= i;

    //never remove the 0th literal -- TODO this is a bad thing
    //we should be able to remove this, but I can't figure out how to
    //reorder the clause then
    seen[cl[0].toInt()] = 1;
    for (vector<Lit>::iterator end = cl.end(); i != end; i++) {
        if (seen[i->toInt()])
            *j++ = *i;
        else
            changedClause = true;

        seen[i->toInt()] = 0;
    }
    stats.furtherShrinkedSuccess += changedClause;
    cl.resize(cl.size() - (i-j));
}

void Searcher::stampBasedLearntMinim(vector<Lit>& cl)
{
    //Stamp-based minimization
    stats.stampShrinkAttempt++;
    const size_t origSize = cl.size();

    Lit firstLit = cl[0];
    std::pair<size_t, size_t> tmp;
    tmp = stamp.stampBasedLitRem(cl, STAMP_RED);
    if (tmp.first || tmp.second) {
        //cout << "Rem RED: " << tmp.first + tmp.second << endl;
    }
    tmp = stamp.stampBasedLitRem(cl, STAMP_IRRED);
    if (tmp.first || tmp.second) {
        //cout << "Rem IRRED: " << tmp.first + tmp.second << endl;
    }

    //Handle removal or moving of the first literal
    size_t at = std::numeric_limits<size_t>::max();
    for(size_t i = 0; i < cl.size(); i++) {
        if (cl[i] == firstLit) {
            at = i;
            break;
        }
    }
    if (at != std::numeric_limits<size_t>::max()) {
        //Make original first lit first in the final clause, too
        std::swap(cl[0], cl[at]);
    } else {
        //Re-add first lit
        cl.push_back(lit_Undef);
        for(int i = ((int)cl.size())-1; i >= 1; i--) {
            cl[i] = cl[i-1];
        }
        cl[0] = firstLit;
    }

    stats.stampShrinkCl += ((origSize - cl.size()) > 0);
    stats.stampShrinkLit += origSize - cl.size();
}

void Searcher::insertVarOrder(const Var x)
{
    if (!order_heap.inHeap(x)
        && solver->decisionVar[x]
    ) {
        order_heap.insert(x);
    }
}

bool Searcher::VarFilter::operator()(uint32_t var) const
{
    return (cc->value(var) == l_Undef && solver->decisionVar[var]);
}

void Searcher::setNeedToInterrupt()
{
    needToInterrupt = true;
}

void Searcher::printAgilityStats()
{
    cout
    << " -- "
    << " confl:" << std::setw(6) << params.conflictsDoneThisRestart
    << ", rest:" << std::setw(3) << stats.numRestarts
    << ", ag:" << std::setw(4) << std::fixed << std::setprecision(2)
    << agility.getAgility()

    << ", agLim:" << std::setw(4) << std::fixed << std::setprecision(2)
    << conf.agilityLimit

    << ", agHist:" << std::setw(4) << std::fixed << std::setprecision(3)
    << hist.agilityHist.avg()

    /*<< ", agilityHistLong: " << std::setw(6) << std::fixed << std::setprecision(3)
    << agilityHist.avgLong()*/
    ;
}

uint64_t Searcher::sumConflicts() const
{
    return solver->sumStats.conflStats.numConflicts + stats.conflStats.numConflicts;
}

uint64_t Searcher::sumRestarts() const
{
    return stats.numRestarts + solver->getStats().numRestarts;
}

size_t Searcher::hyperBinResAll()
{
    size_t added = 0;

    for(std::set<BinaryClause>::const_iterator
        it = solver->needToAddBinClause.begin()
        , end = solver->needToAddBinClause.end()
        ; it != end
        ; it++
    ) {
        lbool val1 = value(it->getLit1());
        lbool val2 = value(it->getLit2());

        if (conf.verbosity >= 6) {
            cout
            << "c Attached hyper-bin: "
            << it->getLit1() << "(val: " << val1 << " )"
            << ", " << it->getLit2() << "(val: " << val2 << " )"
            << endl;
        }

        //If binary is satisfied, skip
        if (val1 == l_True || val2 == l_True) {
            continue;
        }

        assert(val1 == l_Undef && val2 == l_Undef);
        solver->attachBinClause(it->getLit1(), it->getLit2(), true, false);
        added++;
    }

    return added;
}

std::pair<size_t, size_t> Searcher::removeUselessBins()
{
    size_t removedIrred = 0;
    size_t removedRed = 0;

    if (conf.doTransRed) {
        for(std::set<BinaryClause>::iterator
            it = uselessBin.begin()
            , end = uselessBin.end()
            ; it != end
            ; it++
        ) {
            propStats.otfHyperTime += 2;
            //cout << "Removing binary clause: " << *it << endl;
            propStats.otfHyperTime += solver->watches[it->getLit1().toInt()].size()/2;
            propStats.otfHyperTime += solver->watches[it->getLit2().toInt()].size()/2;
            removeWBin(solver->watches, it->getLit1(), it->getLit2(), it->getLearnt());
            removeWBin(solver->watches, it->getLit2(), it->getLit1(), it->getLearnt());

            //Update stats
            if (it->getLearnt()) {
                solver->binTri.redLits -= 2;
                solver->binTri.redBins--;
                removedRed++;
            } else {
                solver->binTri.irredLits -= 2;
                solver->binTri.irredBins--;
                removedIrred++;
            }

            #ifdef DRUP
            if (drup) {
                (*drup)
                << "d "
                << it->getLit1() << " " << it->getLit2()
                << " 0\n";
            }
            #endif

            #ifdef VERBOSE_DEBUG_FULLPROP
            cout << "Removed bin: "
            << it->getLit1() << " , " << it->getLit2()
            << " , learnt: " << it->getLearnt() << endl;
            #endif
        }
    }
    uselessBin.clear();

    return std::make_pair(removedIrred, removedRed);
}

//Only used to generate nice Graphviz graphs
string Searcher::simplAnalyseGraph(
    PropBy conflHalf
    , uint32_t& out_btlevel
    , uint32_t &glue
) {
    int pathC = 0;
    Lit p = lit_Undef;

    learnt_clause.clear();
    learnt_clause.push_back(lit_Undef);      // (leave room for the asserting literal)
    int index   = trail.size() - 1;
    out_btlevel = 0;
    std::stringstream resolutions;

    PropByForGraph confl(conflHalf, failBinLit, *clAllocator);
    do {
        assert(!confl.isNULL());          // (otherwise should be UIP)

        //Update resolutions output
        if (p != lit_Undef) {
            resolutions << " | ";
        }
        resolutions << "{ " << confl << " | " << pathC << " -- ";

        for (uint32_t j = (p == lit_Undef) ? 0 : 1, size = confl.size(); j != size; j++) {
            Lit q = confl[j];
            const Var my_var = q.var();

            if (!seen[my_var] //if already handled, don't care
                && varData[my_var].level > 0 //if it's assigned at level 0, it's assigned FALSE, so leave it out
            ) {
                seen[my_var] = 1;
                assert(varData[my_var].level <= decisionLevel());

                if (varData[my_var].level == decisionLevel()) {
                    pathC++;
                } else {
                    learnt_clause.push_back(q);

                    //Backtracking level is largest of thosee inside the clause
                    if (varData[my_var].level > out_btlevel)
                        out_btlevel = varData[my_var].level;
                }
            }
        }
        resolutions << pathC << " }";

        //Go through the trail backwards, select the one that is to be resolved
        while (!seen[trail[index--].var()]);

        p = trail[index+1];
        confl = PropByForGraph(varData[p.var()].reason, p, *clAllocator);
        seen[p.var()] = 0; // this one is resolved
        pathC--;
    } while (pathC > 0); //UIP when eveything goes through this one
    assert(pathC == 0);
    learnt_clause[0] = ~p;

    // clear out seen
    for (uint32_t j = 0; j != learnt_clause.size(); j++)
        seen[learnt_clause[j].var()] = 0;    // ('seen[]' is now cleared)

    //Calculate glue
    glue = calcGlue(learnt_clause);

    return resolutions.str();
}

//Only used to generate nice Graphviz graphs
void Searcher::genConfGraph(const PropBy conflPart)
{
    assert(ok);
    assert(!conflPart.isNULL());

    static int num = 0;
    num++;
    std::stringstream s;
    s << "confls/" << "confl" << num << ".dot";
    std::string filename = s.str();

    std::ofstream file;
    file.open(filename.c_str());
    if (!file) {
        cout << "Couldn't open filename " << filename << endl;
        cout << "Maybe you forgot to create subdirectory 'confls'" << endl;
        exit(-1);
    }
    file << "digraph G {" << endl;

    //Special vertex indicating final conflict clause (to help us)
    uint32_t out_btlevel, glue;
    const std::string res = simplAnalyseGraph(conflPart, out_btlevel, glue);
    file << "vertK -> dummy;";
    file << "dummy "
    << "[ "
    << " shape=record"
    << " , label=\"{"
    << " clause: " << learnt_clause
    << " | btlevel: " << out_btlevel
    << " | glue: " << glue
    << " | {resol: | " << res << " }"
    << "}\""
    << " , fontsize=8"
    << " ];" << endl;

    PropByForGraph confl(conflPart, failBinLit, *clAllocator);
    #ifdef VERBOSE_DEBUG_GEN_CONFL_DOT
    cout << "conflict: "<< confl << endl;
    #endif

    vector<Lit> lits;
    for (uint32_t i = 0; i < confl.size(); i++) {
        const Lit lit = confl[i];
        assert(value(lit) == l_False);
        lits.push_back(lit);

        //Put these into the impl. graph for sure
        seen[lit.var()] = true;
    }

    for (vector<Lit>::const_iterator it = lits.begin(), end = lits.end(); it != end; it++) {
        file << "x" << it->unsign() << " -> vertK "
        << "[ "
        << " label=\"" << lits << "\""
        << " , fontsize=8"
        << " ];" << endl;
    }

    //Special conflict vertex
    file << "vertK"
    << " [ "
    << "shape=\"box\""
    << ", style=\"filled\""
    << ", color=\"darkseagreen\""
    << ", label=\"K : " << lits << "\""
    << "];" << endl;

    //Calculate which literals are directly connected with the conflict
    vector<Lit> insideImplGraph;
    while(!lits.empty())
    {
        vector<Lit> newLits;
        for (size_t i = 0; i < lits.size(); i++) {
            PropBy reason = varData[lits[i].var()].reason;
            //Reason in NULL, so remove: it's got no antedecent
            if (reason.isNULL()) continue;

            #ifdef VERBOSE_DEBUG_GEN_CONFL_DOT
            cout << "Reason for lit " << lits[i] << " : " << reason << endl;
            #endif

            PropByForGraph prop(reason, lits[i], *clAllocator);
            for (uint32_t i2 = 0; i2 < prop.size(); i2++) {
                const Lit lit = prop[i2];
                assert(value(lit) != l_Undef);

                //Don't put into the impl. graph lits at 0 decision level
                if (varData[lit.var()].level == 0) continue;

                //Already added, just drop
                if (seen[lit.var()]) continue;

                seen[lit.var()] = true;
                newLits.push_back(lit);
                insideImplGraph.push_back(lit);
            }
        }
        lits = newLits;
    }

    //Print edges
    for (size_t i = 0; i < trail.size(); i++) {
        const Lit lit = trail[i];

        //0-decision level means it's pretty useless to put into the impl. graph
        if (varData[lit.var()].level == 0) continue;

        //Not directly connected with the conflict, drop
        if (!seen[lit.var()]) continue;

        PropBy reason = varData[lit.var()].reason;

        //A decision variable, it is not propagated by any clause
        if (reason.isNULL()) continue;

        PropByForGraph prop(reason, lit, *clAllocator);
        for (uint32_t i = 0; i < prop.size(); i++) {
            if (prop[i] == lit //This is being propagated, don't make a circular line
                || varData[prop[i].var()].level == 0 //'clean' clauses of 0-level lits
            ) continue;

            file << "x" << prop[i].unsign() << " -> x" << lit.unsign() << " "
            << "[ "
            << " label=\"";
            for(uint32_t i2 = 0; i2 < prop.size();) {
                //'clean' clauses of 0-level lits
                if (varData[prop[i2].var()].level == 0) {
                    i2++;
                    continue;
                }

                file << prop[i2];
                i2++;
                if (i2 != prop.size()) file << " ";
            }
            file << "\""
            << " , fontsize=8"
            << " ];" << endl;
        }
    }

    //Print vertex definitions
    for (size_t i = 0; i < trail.size(); i++) {
        Lit lit = trail[i];

        //Only vertexes that really have been used
        if (seen[lit.var()] == 0) continue;
        seen[lit.var()] = 0;

        file << "x" << lit.unsign()
        << " [ "
        << " shape=\"box\""
        //<< ", size = 0.8"
        << ", style=\"filled\"";
        if (varData[lit.var()].reason.isNULL())
            file << ", color=\"darkorange2\""; //decision var
        else
            file << ", color=\"darkseagreen4\""; //propagated var

        //Print label
        file
        << ", label=\"" << (lit.sign() ? "-" : "") << "x" << lit.unsign()
        << " @ " << varData[lit.var()].level << "\""
        << " ];" << endl;
    }

    file  << "}" << endl;
    file.close();

    cout << "c Printed implication graph (with conflict clauses) to file "
    << filename << endl;
}

void Searcher::decayClauseAct()
{
    clauseActivityIncrease *= conf.clauseDecayActivity;
}

void Searcher::bumpClauseAct(Clause* cl)
{
    cl->stats.activity += clauseActivityIncrease;
    if (cl->stats.activity > 1e20 ) {
        // Rescale
        for(vector<ClOffset>::iterator
            it = solver->longRedCls.begin(), end = solver->longRedCls.end()
            ; it != end
            ; it++
        ) {
            clAllocator->getPointer(*it)->stats.activity *= 1e-20;
        }
        clauseActivityIncrease *= 1e-20;
        clauseActivityIncrease = std::max(clauseActivityIncrease, 1.0);
    }
}

PropBy Searcher::propagate(
    Solver* solver2
    #ifdef STATS_NEEDED
    , AvgCalc<size_t>* watchListSizeTraversed
    //, AvgCalc<bool>* litPropagatedSomething
    #endif
) {
    #ifdef DRUP
    size_t origTrailSize = trail.size();
    #endif

    PropBy ret;
    if (solver2 != NULL
        && conf.propBinFirst
    ) {
        ret = propagateBinFirst(
            solver2
            #ifdef STATS_NEEDED
            , AvgCalc<size_t>* watchListSizeTraversed
            //, AvgCalc<bool>* litPropagatedSomething
            #endif
        );
    } else {
        ret = propagateAnyOrder();
    }

    #ifdef DRUP
    //If declevel 0 propagation, we have to add the unitaries
    if (drup && decisionLevel() == 0) {
        for(size_t i = origTrailSize; i < trail.size(); i++) {
            #ifdef DEBUG_DRUP
            if (conf.verbosity >= 6) {
                cout
                << "c 0-level enqueue:"
                << trail[i]
                << endl;
            }
            #endif

            (*drup)
            << trail[i]
            << " 0\n";
        }
        if (!ret.isNULL()) {
            (*drup)
            << "0\n";
        }
    }
    #endif

    return ret;
}

uint64_t Searcher::memUsedSearch() const
{
    uint64_t mem = 0;
    mem += otfMustAttach.capacity()*sizeof(OTFClause);
    mem += toAttachLater.capacity()*sizeof(ClOffset);
    mem += toClear.capacity()*sizeof(Lit);
    mem += trail.capacity()*sizeof(Lit);
    mem += trail_lim.capacity()*sizeof(uint32_t);
    mem += activities.capacity()*sizeof(uint32_t);
    mem += order_heap.memUsed();
    mem += learnt_clause.capacity()*sizeof(Lit);
    mem += hist.memUsed();
    mem += conflict.capacity()*sizeof(Lit);
    mem += analyze_stack.capacity()*sizeof(Lit);
    mem += solution.capacity()*sizeof(lbool);

    return mem;
}

void Searcher::redoOrderHeap()
{
    assert(decisionLevel() == 0);
    order_heap.clear();
    for(size_t var = 0; var < nVars(); var++) {
        if (solver->decisionVar[var]
            && value(var) == l_Undef
            && (varData[var].elimed == ELIMED_NONE
                || varData[var].elimed == ELIMED_QUEUED_VARREPLACER)
        ) {
            insertVarOrder(var);
        }
    }
}

