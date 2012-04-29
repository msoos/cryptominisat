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

#include "CommandControl.h"
#include "Subsumer.h"
#include "CalcDefPolars.h"
#include "time_mem.h"
#include "ThreadControl.h"
#include <iomanip>
#include <omp.h>
#include "SCCFinder.h"
#include "VarReplacer.h"
#include "ClauseCleaner.h"
#include "PropByForGraph.h"
using std::cout;
using std::endl;

//#define VERBOSE_DEBUG_GEN_CONFL_DOT

#ifdef VERBOSE_DEBUG
#define VERBOSE_DEBUG_GEN_CONFL_DOT
#endif

/**
@brief Sets a sane default config and allocates handler classes
*/
CommandControl::CommandControl(const SolverConf& _conf, ThreadControl* _control) :
        Solver(
            _control->clAllocator
            , AgilityData(_conf.agilityG, _conf.agilityLimit)
            , _conf.updateGlues
        )

        //variables
        , control(_control)
        , conf(_conf)
        , needToInterrupt(false)
        , order_heap(VarOrderLt(activities))
        , var_inc(_conf.var_inc_start)
{
    mtrand.seed(conf.origSeed);
}

CommandControl::~CommandControl()
{
}

Var CommandControl::newVar(const bool dvar)
{
    const Var var = Solver::newVar(dvar);
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
void CommandControl::cancelUntil(uint32_t level)
{
    #ifdef VERBOSE_DEBUG
    cout << "Canceling until level " << level;
    if (level > 0) cout << " sublevel: " << trail_lim[level];
    cout << endl;
    #endif

    if (decisionLevel() > level) {

        for (int sublevel = trail.size()-1; sublevel >= (int)trail_lim[level]; sublevel--) {
            const Var var = trail[sublevel].var();
            #ifdef VERBOSE_DEBUG
            cout << "Canceling var " << var+1 << " sublevel: " << sublevel << endl;
            #endif
            assert(value(var) != l_Undef);
            assigns[var] = l_Undef;
            #ifdef ANIMATE3D
            std:cerr << "u " << var << endl;
            #endif
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

void CommandControl::analyzeHelper(
    const Lit lit
    , int& pathC
    , vector<Lit>& out_learnt
    , bool var_bump_necessary
) {
    const Var var = lit.var();
    assert(varData[var].elimed == ELIMED_NONE
        || varData[var].elimed == ELIMED_QUEUED_VARREPLACER);

    //If var is at level 0, don't do anything with it, just skip
    if (varData[var].level == 0)
        return;

    if (seen2[var] == 0 && //hasn't been bumped yet
        ((var_bump_necessary
            && conf.rarely_bump_var_act) //rarely bump, but bump this time
          || !conf.rarely_bump_var_act //always bump
        )
    ) {
        varBumpActivity(var);
        seen2[var] = 1;
        toClear.push_back(Lit(var, false));
    }

    //Update our state of going through the conflict
    if (!seen[var]) {
        seen[var] = 1;

        if (varData[var].level == decisionLevel())
            pathC++;
        else
            out_learnt.push_back(lit);
    }
}

/**
@brief    Analyze conflict and produce a reason clause.

Post-condition: 'out_learnt[0]' is the asserting literal at level 'out_btlevel'
*/
Clause* CommandControl::analyze(
    PropBy confl
    , vector<Lit>& out_learnt
    , uint32_t& out_btlevel
    , uint32_t &glue
) {
    assert(out_learnt.empty());
    assert(decisionLevel() > 0);

    int pathC = 0;
    Lit p = lit_Undef;
    int index = trail.size() - 1;
    out_btlevel = 0;
    PropBy oldConfl;

    uint32_t numIterations = 0;

    //cout << "---- Start analysis -----" << endl;
    toClear.clear();
    out_learnt.push_back(lit_Undef); //make space for ~p
    do {
        numIterations++;

        //Add literals from 'confl' to clause
        switch (confl.getType()) {
            case tertiary_t : {
                analyzeHelper(confl.getOtherLit2(), pathC, out_learnt, true);
            }
            //NO BREAK, since tertiary is like binary, just one more lit

            case binary_t : {
                if (p == lit_Undef)
                    analyzeHelper(failBinLit, pathC, out_learnt, true);

                analyzeHelper(confl.getOtherLit(), pathC, out_learnt, true);
                break;
            }

            case clause_t : {
                const Clause& cl = *clAllocator->getPointer(confl.getClause());
                for (size_t j = 0, size = cl.size(); j != size; j++) {

                    //This is the one that will be resolved out anyway, so just skip
                    if (p != lit_Undef && j == 0)
                        continue;

                    analyzeHelper(cl[j], pathC, out_learnt, !cl.learnt());
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
        oldConfl = confl;
        confl = varData[p.var()].reason;
        seen[p.var()] = 0; //This clears out vars that haven't been added to out_learnt, but their 'seen' has been set
        pathC--;
        //cout << "Next 'p' to look at: " << p << endl;
    } while (pathC > 0);
    out_learnt[0] = ~p;

    //Clear seen2, which was used to mark literals that have been bumped
    for (vector<Lit>::const_iterator
        it = toClear.begin(), end = toClear.end()
        ; it != end
        ; it++
    ) {
        seen2[it->var()] = 0;
    }
    toClear.clear();

    assert(pathC == 0);
    stats.litsLearntNonMin += out_learnt.size();

    //Recursive-simplify conflict clause:
    if (conf.doRecursiveCCMin) {
        toClear = out_learnt;
        trace_reasons.clear();


        uint32_t abstract_level = 0;
        for (size_t i = 1; i < out_learnt.size(); i++)
            abstract_level |= abstractLevel(out_learnt[i].var()); // (maintain an abstraction of levels involved in conflict)

        find_removable(out_learnt, abstract_level);
        prune_removable(out_learnt);

        //Clear 'seen'
        for (vector<Lit>::const_iterator
            it = toClear.begin(), end = toClear.end()
            ; it != end
            ; it++
        ) {
            seen[it->var()] = 0;
        }
        toClear.clear();
    }
    stats.litsLearntRecMin += out_learnt.size();

    //Cache-based minimisation
    if (conf.doCache
        && conf.doMinimLearntMore
        && out_learnt.size() > 1
        && (conf.doAlwaysFMinim
            || calcGlue(out_learnt) < 0.65*glueHist.getAvgLong()
            || out_learnt.size() < 0.65*conflSizeHist.getAvgLong()
            || out_learnt.size() < 10
            )
    ) {
        minimiseLearntFurther(out_learnt);
    }

    //Calc stats
    glue = calcGlue(out_learnt);
    stats.litsLearntFinal += out_learnt.size();

    //Print fully minimised clause
    #ifdef VERBOSE_DEBUG_OTF_GATE_SHORTEN
    cout << "Final clause: " << out_learnt << endl;
    for (uint32_t i = 0; i < out_learnt.size(); i++) {
        cout << "lev out_learnt[" << i << "]:" << varData[out_learnt[i].var()].level << endl;
    }
    #endif

    // Find correct backtrack level:
    if (out_learnt.size() <= 1)
        out_btlevel = 0;
    else {
        uint32_t max_i = 1;
        for (uint32_t i = 2; i < out_learnt.size(); i++)
            if (varData[out_learnt[i].var()].level > varData[out_learnt[max_i].var()].level)
                max_i = i;
        std::swap(out_learnt[max_i], out_learnt[1]);
        out_btlevel = varData[out_learnt[1].var()].level;
    }
    #ifdef VERBOSE_DEBUG_OTF_GATE_SHORTEN
    cout << "out_btlevel: " << out_btlevel << endl;
    #endif

    //We can only on-the-fly subsume with clauses that are not 2- or 3-long
    //furthermore, we cannot subsume a clause that is marked for deletion
    //due to its high glue value
    if (!conf.doOTFSubsume
        || out_learnt.size() <= 3
        || !oldConfl.isClause()
    ) {
        return NULL;
    }

    Clause* cl = clAllocator->getPointer(oldConfl.getClause());

    //Larger or equivalent clauses cannot subsume the clause
    if (out_learnt.size() >= cl->size())
        return NULL;

    //Does it subsume?
    if (!subset(out_learnt, *cl))
        return NULL;

    //on-the-fly subsumed the original clause
    stats.otfSubsumed++;
    stats.otfSubsumedLearnt += cl->learnt();
    stats.otfSubsumedLitsGained += cl->size() - out_learnt.size();
    return cl;

}

bool CommandControl::subset(const vector<Lit>& A, const Clause& B)
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

void CommandControl::prune_removable(vector<Lit>& out_learnt)
{
    int j = 1;
    for (int i = 1, sz = out_learnt.size(); i < sz; i++) {
        if ((seen[out_learnt[i].var()] & (1|2)) == (1|2)) {
            assert((seen[out_learnt[i].var()] & (4|8)) == 0);
            out_learnt[j++] = out_learnt[i];
        }
    }
    out_learnt.resize(j);
}

void CommandControl::find_removable(const vector<Lit>& out_learnt, const uint32_t abstract_level)
{
    bool found_some = false;
    trace_lits_minim.clear();
    for (int i = 1, sz = out_learnt.size(); i < sz; i++) {
        const Lit curLit = out_learnt[i];
        assert(varData[curLit.var()].level > 0);

        if ((seen[curLit.var()] & (2|4|8)) == 0) {
            found_some |= dfs_removable(curLit, abstract_level);
        }
    }

    if (found_some)
       res_removable();
}

int CommandControl::quick_keeper(Lit p, uint32_t abstract_level, const bool maykeep)
{
    // See if I can kill myself right away.
    // maykeep == 1 if I am in the original conflict clause.
    if (varData[p.var()].reason.isNULL()) {
        return (maykeep ? 2 : 8);
    } else if ((abstractLevel(p.var()) & abstract_level) == 0) {
        assert(maykeep == 0);
        return 8;
    } else {
        return 0;
    }
}

//seen[x.var()] = 2  -> cannot be removed
//seen[x.var()] = 2 or 4 or 8 -> DFS has been done

int CommandControl::dfs_removable(Lit p, uint32_t abstract_level)
{
    int pseen = seen[p.var()];
    assert((pseen & (2|4|8)) == 0);

    bool maykeep = pseen & (1);
    int pstatus = quick_keeper(p, abstract_level, maykeep);
    if (pstatus) {
        seen[p.var()] |= (char) pstatus;
        if (pseen == 0) toClear.push_back(p);
        return 0;
    }

    int found_some = 0;
    pstatus = 4;

    // rp[0] is p.  The rest of rp are predecessors of p.
    const PropBy rp = varData[p.var()].reason;
    switch (rp.getType()) {
        case tertiary_t : {
            const Lit q = rp.getOtherLit2();
            if (varData[q.var()].level > 0) {
                if ((seen[q.var()] & (2|4|8)) == 0) {
                    found_some |= dfs_removable(q, abstract_level);
                }
                int qseen = seen[q.var()];
                if (qseen & (8)) {
                    pstatus = (maykeep ? 2 : 8);
                    break;
                 }
                 assert((qseen & (2|4)));
            }
             //NO BREAK
        }

        case binary_t: {
            const Lit q = rp.getOtherLit();
            if (varData[q.var()].level > 0) {
                if ((seen[q.var()] & (2|4|8)) == 0) {
                    found_some |= dfs_removable(q, abstract_level);
                }
                int qseen = seen[q.var()];
                if (qseen & (8)) {
                    pstatus = (maykeep ? 2 : 8);
                    break;
                 }
                 assert((qseen & (2|4)));
            }
            break;
        }

        case clause_t : {
            const Clause& cl = *clAllocator->getPointer(rp.getClause());
            for (int i = 0, sz = cl.size(); i < sz; i++) {
                if (i == 0)
                    continue;

                const Lit q = cl[i];
                if (varData[q.var()].level > 0) {
                    if ((seen[q.var()] & (2|4|8)) == 0) {
                        found_some |= dfs_removable(q, abstract_level);
                    }
                    int qseen = seen[q.var()];
                    if (qseen & (8)) {
                        pstatus = (maykeep ? 2 : 8);
                        break;
                     }
                     assert((qseen & (2|4)));
                }
            }
            break;
        }

        case null_clause_t :
        default:
            assert(false);
            break;
    }


    if (pstatus == 4) {
        // We might want to resolve p out.  See res_removable().
        trace_lits_minim.push_back(p);
    }
    seen[p.var()] |= (char) pstatus;
    if (pseen == 0) toClear.push_back(p);
    found_some |= maykeep;
    return found_some;
}

void CommandControl::mark_needed_removable(const Lit p)
{
    const PropBy rp = varData[p.var()].reason;
    switch (rp.getType()) {
        case tertiary_t : {
            const Lit q = rp.getOtherLit2();
            if (varData[q.var()].level > 0) {
                const int qseen = seen[q.var()];
                if ((qseen & (1)) == 0 && !varData[q.var()].reason.isNULL()) {
                    seen[q.var()] |= 1;
                    if (qseen == 0) toClear.push_back(q);
                }
            }
            //NO BREAK
        }

        case binary_t : {
            const Lit q  = rp.getOtherLit();
            if (varData[q.var()].level > 0) {
                const int qseen = seen[q.var()];
                if ((qseen & (1)) == 0 && !varData[q.var()].reason.isNULL()) {
                    seen[q.var()] |= 1;
                    if (qseen == 0) toClear.push_back(q);
                }
            }
            break;
        }

        case clause_t : {
            const Clause& cl = *clAllocator->getPointer(rp.getClause());
            for (int i = 0; i < cl.size(); i++){
                if (i == 0)
                    continue;

                const Lit q  = cl[i];
                if (varData[q.var()].level > 0) {
                    const int qseen = seen[q.var()];
                    if ((qseen & (1)) == 0 && !varData[q.var()].reason.isNULL()) {
                        seen[q.var()] |= 1;
                        if (qseen == 0) toClear.push_back(q);
                    }
                }
            }
            break;
        }

        case null_clause_t :
        default:
            assert(false);
            break;
    }

    return;
}


int  CommandControl::res_removable()
{
    int minim_res_ctr = 0;
    while (trace_lits_minim.size() > 0){
        const Lit p = trace_lits_minim.back();
        trace_lits_minim.pop_back();
        assert(!varData[p.var()].reason.isNULL());

        int pseen = seen[p.var()];
        if (pseen & (1)) {
            minim_res_ctr ++;
            trace_reasons.push_back(varData[p.var()].reason);
            mark_needed_removable(p);
        }
    }
    return minim_res_ctr;
}

/**
@brief Specialized analysis procedure to express the final conflict in terms of assumptions.
Calculates the (possibly empty) set of assumptions that led to the assignment of 'p', and
stores the result in 'out_conflict'.
*/
void CommandControl::analyzeFinal(const Lit p, vector<Lit>& out_conflict)
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
                    const Lit lit2 = confl.getOtherLit2();
                    if (varData[lit2.var()].level > 0)
                        seen[lit2.var()] = 1;

                    //Intentionally no break, since tertiary is similar to binary
                }

                case binary_t : {
                    const Lit lit1 = confl.getOtherLit();
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
lbool CommandControl::search(SearchFuncParams _params, uint64_t& rest)
{
    assert(ok);

    //Stats reset & update
    SearchFuncParams params(_params);
    if (params.update)
        stats.numRestarts ++;
    agility.reset(conf.agilityLimit);
    agilityHist.fastclear();
    glueHist.fastclear();
    conflSizeHist.fastclear();
    branchDepthHist.fastclear();
    branchDepthDeltaHist.fastclear();
    trailDepthHist.fastclear();
    trailDepthDeltaHist.fastclear();

    //Debug
    #ifdef VERBOSE_DEBUG
    cout << "c started CommandControl::search()" << endl;
    #endif //VERBOSE_DEBUG

    //Loop until restart or finish (SAT/UNSAT)
    while (true) {
        assert(ok);
        Lit failed;
        PropBy confl;

        //If decision level==1, then try to do all the good stuff
        if (decisionLevel() == 1) {
            stats.advancedPropCalled++;
            failed = propagateFull();
            if (failed != lit_Undef) {
                //Update conflict stats
                stats.learntUnits++;
                stats.conflStats.numConflicts++;
                stats.conflStats.update(lastConflictCausedBy);

                cancelUntil(0);
                trail.push_back(~failed);
                stats.litsLearntNonMin += 1;
                stats.litsLearntRecMin += 1;
                stats.litsLearntFinal += 1;
                propStats.propsUnit++;
                stats.hyperBinAdded += hyperBinResAll();
                stats.transRedRemoved += removeUselessBins();

                if (!ok)
                    return l_False;

                continue;
            }

            //Update cache
            for (int64_t c = trail.size()-1; c != (int64_t)trail_lim[0]; c--) {
                const Lit thisLit = trail[c];
                const Lit ancestor = varData[thisLit.var()].reason.getAncestor();
                if (control->conf.doCache) {
                    assert(thisLit != trail[trail_lim[0]]);
                    const bool learntStep = varData[thisLit.var()].reason.getLearntStep();

                    assert(ancestor != lit_Undef);
                    control->implCache[(~ancestor).toInt()].merge(
                        control->implCache[(~thisLit).toInt()].lits
                        , thisLit
                        , learntStep
                        , ancestor
                        , control->seen
                    );
                }
            }
            stats.hyperBinAdded += hyperBinResAll();
            stats.transRedRemoved += removeUselessBins();
        } else {
            confl = propagate();
        }

        #ifdef VERBOSE_DEBUG
        cout << "c CommandControl::search() has finished propagation" << endl;
        #endif //VERBOSE_DEBUG

        if (!confl.isNULL()) {
            //Update conflict stats based on lastConflictCausedBy
            stats.conflStats.update(lastConflictCausedBy);

            //If restart is needed, set it as so
            checkNeedRestart(params, rest);

            if (!handle_conflict(params, confl))
                return l_False;

        } else {
            assert(ok);

            //If restart is needed, restart here
            if (params.needToStopSearch
                || sumConflicts() > control->getNextCleanLimit()
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
lbool CommandControl::new_decision()
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

        stats.decisions++;
    }

    // Increase decision level and enqueue 'next'
    assert(value(next) == l_Undef);
    newDecisionLevel();
    enqueue(next);

    return l_Undef;
}

void CommandControl::checkNeedRestart(SearchFuncParams& params, uint64_t& rest)
{
    if (needToInterrupt)  {
        if (conf.verbosity >= 3)
            cout << "c needToInterrupt is set, restartig as soon as possible!" << endl;
        params.needToStopSearch = true;
    }

    if (     (conf.restartType == geom_restart
            && params.conflictsDoneThisRestart > rest
        ) || (conf.restartType == glue_restart
            && glueHist.isvalid()
            && 0.95*glueHist.getAvg() > glueHist.getAvgLong()
        ) || (conf.restartType == agility_restart
            && agilityHist.isvalid()
            && agilityHist.getAvg() < conf.agilityLimit
        ) || (conf.restartType == branch_depth_delta_restart
            && branchDepthDeltaHist.isvalid()
            && 0.95*branchDepthDeltaHist.getAvg() > branchDepthDeltaHist.getAvgLong()
        )
    ) {
        //Now check agility
        if (agility.getAgility() < conf.agilityLimit) {
            #ifdef DEBUG_DYNAMIC_RESTART
            if (glueHistory.isvalid()) {
                cout << "glueHistory.getavg():" << glueHistory.getavg() <<endl;
                cout << "totalSumOfGlue:" << totalSumOfGlue << endl;
                cout << "conflicts:" << conflicts<< endl;
                cout << "compTotSumGlue:" << compTotSumGlue << endl;
                cout << "conflicts-compTotSumGlue:" << conflicts-compTotSumGlue<< endl;
            }
            #endif

            if (conf.verbosity >= 4) {
                printAgilityStats();
                cout << "c Agility was too low, restarting as soon as possible!" << endl;
            }
            params.needToStopSearch = true;
        } else {
            rest *= conf.restart_inc;
        }
    }

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
bool CommandControl::handle_conflict(SearchFuncParams& params, PropBy confl)
{
    #ifdef VERBOSE_DEBUG
    cout << "Handling conflict" << endl;
    #endif

    //Stats
    uint32_t backtrack_level;
    uint32_t glue;
    vector<Lit> learnt_clause;
    stats.conflStats.numConflicts++;
    params.conflictsDoneThisRestart++;
    if (conf.doPrintConflDot)
        genConfGraph(confl);

    if (decisionLevel() == 0)
        return false;

    Clause* cl = analyze(confl, learnt_clause, backtrack_level, glue);
    size_t orig_trail_size = trail.size();
    if (params.update) {
        trailDepthHist.push(trail.size() - trail_lim[0]);
        branchDepthHist.push(decisionLevel());
        branchDepthDeltaHist.push(decisionLevel() - backtrack_level);
        glueHist.push(glue);
        conflSizeHist.push(learnt_clause.size());
        agilityHist.push(agility.getAgility());
    }
    cancelUntil(backtrack_level);
    if (params.update) {
        trailDepthDeltaHist.push(orig_trail_size - trail.size());
    }

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
    std::sort(learnt_clause.begin()+1, learnt_clause.end(), PolaritySorter(varData));
    glue = std::min<uint32_t>(glue, std::numeric_limits<uint16_t>::max());

    //Is there on-the-fly subsumption?
    if (cl == NULL) {
        //Get new clause
        cl = control->newClauseByThread(learnt_clause, glue);
    } else {
        uint32_t origSize = cl->size();
        control->detachClause(*cl);
        for (uint32_t i = 0; i != learnt_clause.size(); i++)
            (*cl)[i] = learnt_clause[i];
        cl->shrink(origSize - learnt_clause.size());
        if (cl->learnt() && cl->stats.glue > glue)
            cl->stats.glue = glue;
        cl->stats.numPropAndConfl += conf.rewardShortenedClauseWithPropConfl;
    }

    //Attach new clause
    switch (learnt_clause.size()) {
        case 1:
            //Unitary learnt
            stats.learntUnits++;
            enqueue(learnt_clause[0]);
            propStats.propsUnit++;
            assert(backtrack_level == 0 && "Unit clause learnt, so must cancel until level 0, right?");

            break;
        case 2:
            //Binary learnt
            stats.learntBins++;
            control->attachBinClause(learnt_clause[0], learnt_clause[1], true);
            if (decisionLevel() == 1)
                enqueueComplex(learnt_clause[0], ~learnt_clause[1], true);
            else
                enqueue(learnt_clause[0], PropBy(learnt_clause[1]));

            propStats.propsBinRed++;
            break;

        case 3:
            //3-long almost-normal learnt
            stats.learntTris++;
            control->attachClause(*cl);

            if (decisionLevel() == 1)
                addHyperBin(learnt_clause[0], learnt_clause[1], learnt_clause[2]);
            else
                enqueue(learnt_clause[0], PropBy(learnt_clause[1], learnt_clause[2]));

            propStats.propsTri++;
            break;

        default:
            //Normal learnt
            stats.learntLongs++;
            control->attachClause(*cl);
            if (decisionLevel() == 1)
                addHyperBin(learnt_clause[0], *cl);
            else
                enqueue(learnt_clause[0], PropBy(clAllocator->getOffset(cl)));
            propStats.propsLongRed++;
            break;
    }

    varDecayActivity();

    return true;
}

void CommandControl::genRandomVarActMultDiv()
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
void CommandControl::resetStats()
{
    assert(ok);

    //Set up time
    startTime = cpuTime();

    //Clear up previous stuff like model, final conflict
    conflict.clear();

    //Initialise stats
    branchDepthHist.clear();
    branchDepthHist.resize(100);
    branchDepthDeltaHist.clear();
    branchDepthDeltaHist.resize(100);
    trailDepthHist.clear();
    trailDepthHist.resize(100);
    trailDepthDeltaHist.clear();
    trailDepthDeltaHist.resize(100);
    glueHist.clear();
    glueHist.resize(conf.shortTermGlueHistorySize);
    conflSizeHist.clear();
    conflSizeHist.resize(100);
    agilityHist.clear();
    agilityHist.resize(100);

    //Rest solving stats
    stats.clear();
    propStats.clear();

    //Set already set vars
    origTrailSize = trail.size();

    order_heap.filter(VarFilter(this, control));
    lastCleanZeroDepthAssigns = trail.size();
}

lbool CommandControl::burstSearch()
{
    //Print what we will be doing
    if (conf.verbosity >= 2) {
        cout
        << "c Doing bust search for " << conf.burstSearchLen << " conflicts"
        << endl;
    }
    const size_t numUnitsUntilNow = stats.learntUnits;
    const size_t numBinsUntilNow = stats.learntBins;

    //Save old config
    const double backup_rand = conf.random_var_freq;
    const RestType backup_restType = conf.restartType;
    const int backup_polar_mode = conf.polarity_mode;
    uint32_t backup_var_inc_divider = var_inc_divider;
    uint32_t backup_var_inc_multiplier = var_inc_multiplier;

    //Set burst config
    conf.random_var_freq = 1;
    conf.polarity_mode = polarity_rnd;
    var_inc_divider = 1;
    var_inc_multiplier = 1;

    //Do burst
    uint64_t rest_burst = conf.burstSearchLen;
    lbool status = search(SearchFuncParams(rest_burst), rest_burst);

    //Restore config
    conf.random_var_freq = backup_rand;
    conf.restartType = backup_restType;
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
        << endl;
    }

    return status;
}

void CommandControl::printRestartStats()
{
    printBaseStats();
    if (conf.printFullStats)
        printSearchStats();
    else
        control->printClauseStats();

    cout << endl;
}

void CommandControl::printBaseStats()
{
    cout
    << "c"
    //<< omp_get_thread_num()
    << " " << std::setw(5) << stats.numRestarts + control->getStats().numRestarts
    << " " << std::setw(7) << sumConflicts()
    << " " << std::setw(7) << control->getNumFreeVarsAdv(trail.size())
    ;
}

void CommandControl::printSearchStats()
{
    cout
    << " glue"
    << " " << std::right << glueHist.getAvgMidPrint(1, 5)
    << "/" << std::left << glueHist.getAvgLongPrint(1, 5)

    << " agil"
    << " " << std::right << agilityHist.getAvgMidPrint(3, 5)
    << "/" << std::left<< agilityHist.getAvgLongPrint(3, 5)

    << " confllen"
    << " " << std::right << conflSizeHist.getAvgMidPrint(1, 5)
    << "/" << std::left << conflSizeHist.getAvgLongPrint(1, 5)

    << " branchd"
    << " " << std::right << branchDepthHist.getAvgMidPrint(1, 5)
    << "/" << std::left  << branchDepthHist.getAvgLongPrint(1, 5)
    << " branchdd"

    << " " << std::right << branchDepthDeltaHist.getAvgMidPrint(1, 4)
    << "/" << std::left << branchDepthDeltaHist.getAvgLongPrint(1, 4)

    << " traild"
    << " " << std::right << trailDepthHist.getAvgMidPrint(0, 7)
    << "/" << std::left << trailDepthHist.getAvgLongPrint(0, 7)

    << " traildd"
    << " " << std::right << trailDepthDeltaHist.getAvgMidPrint(0, 5)
    << "/" << std::left << trailDepthDeltaHist.getAvgLongPrint(0, 5)
    ;

    cout << std::right;
}

/**
@brief The main solve loop that glues everything together

We clear everything needed, pre-simplify the problem, calculate default
polarities, and start the loop. Finally, we either report UNSAT or extend the
found solution with all the intermediary simplifications (e.g. variable
elimination, etc.) and output the solution.
*/
lbool CommandControl::solve(const vector<Lit>& assumps, const uint64_t maxConfls)
{
    assert(ok);
    assert(qhead == trail.size());

    assumptions = assumps;
    resetStats();

    //Current solving status
    lbool status = l_Undef;

    uint64_t lastRestartPrint = stats.conflStats.numConflicts;

    //Burst seach
    status = burstSearch();

    //Restore some data
    for(size_t i = 0; i < control->nVars(); i++) {
        varData[i].polarity = control->getSavedPolarity(i);
        activities[i] = control->getSavedActivity(i);
    }
    var_inc = control->getSavedActivityInc();

    order_heap.clear();
    for(size_t var = 0; var < nVars(); var++) {
        if (control->decision_var[var]
            && value(var) == l_Undef
        ) {
            insertVarOrder(var);
        }
    }
    assert(order_heap.heapProperty());

    // Search:
    genRandomVarActMultDiv();
    uint64_t rest = conf.restart_first;
    while (status == l_Undef
        && !needToInterrupt
        && stats.conflStats.numConflicts < maxConfls
    ) {
        assert(stats.conflStats.numConflicts < maxConfls);

        status = search(SearchFuncParams(maxConfls-stats.conflStats.numConflicts), rest);
        rest *= conf.restart_inc;
        if (status != l_Undef)
            break;

        //Print restart stat
        if (conf.verbosity >= 1
            && ((lastRestartPrint + 800) < stats.conflStats.numConflicts
                || conf.printAllRestarts)
        ) {
            printRestartStats();
            lastRestartPrint = stats.conflStats.numConflicts;
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
        if (sumConflicts() > control->getNextCleanLimit()) {
            if (conf.verbosity >= 3) {
                cout
                << "c th " << omp_get_thread_num() << " cleaning"
                << " getNextCleanLimit(): " << control->getNextCleanLimit()
                << " numConflicts : " << stats.conflStats.numConflicts
                << " SumConfl: " << sumConflicts()
                << " maxConfls:" << maxConfls
                << " Trail size: " << trail.size() << endl;
            }
            control->fullReduce();

            genRandomVarActMultDiv();
        }

        //Check if we should do SCC
        //cout << "numNewBinsSinceSCC: " << control->numNewBinsSinceSCC << endl;
        const size_t newZeroDepthAss = trail.size() - lastCleanZeroDepthAssigns;
        if (newZeroDepthAss > ((double)control->getNumFreeVars()*0.001))  {
            cout << "newZeroDepthAss : " << newZeroDepthAss  << endl;
            lastCleanZeroDepthAssigns = trail.size();
            control->clauseCleaner->removeAndCleanAll();
        }

        if (control->numNewBinsSinceSCC/2 > ((double)control->getNumFreeVars()*0.003)) {
            control->clauseCleaner->removeAndCleanAll();
            if (!control->sCCFinder->find2LongXors()) {
                status = l_False;
                break;
            }

            if (conf.doReplace && !control->varReplacer->performReplace()) {
                status = l_False;
                break;
            }
            lastCleanZeroDepthAssigns = trail.size();
        }
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
        cout << "c CommandControl::solve() finished"
        << " status: " << status
        << " control->getNextCleanLimit(): " << control->getNextCleanLimit()
        << " numConflicts : " << stats.conflStats.numConflicts
        << " SumConfl: " << sumConflicts()
        << " maxConfls:" << maxConfls
        << endl;
    }

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

bool CommandControl::pickPolarity(const Var var)
{
    switch(conf.polarity_mode) {
        case polarity_false:
            return false;

        case polarity_true:
            return true;

        case polarity_rnd:
            return mtrand.randInt(1);

        case polarity_auto:
            return getStoredPolarity(var)
                ^ (mtrand.randInt(conf.flipPolarFreq*branchDepthDeltaHist.getAvgLong()) == 1);
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
Lit CommandControl::pickBranchLit()
{
    #ifdef VERBOSE_DEBUG
    cout << "decision level: " << decisionLevel() << " ";
    #endif

    Lit next = lit_Undef;

    // Random decision:
    double rand = mtrand.randDblExc();
    if (next == lit_Undef
        && rand < conf.random_var_freq
        && !order_heap.empty()
    ) {
        const Var next_var = order_heap[mtrand.randInt(order_heap.size()-1)];
        if (value(next_var) == l_Undef
            && control->decision_var[next_var]
        ) {
            stats.decisionsRand++;
            next = Lit(next_var, !pickPolarity(next_var));
        }
    }

    // Activity based decision:
    while (next == lit_Undef
      || value(next.var()) != l_Undef
      || !control->decision_var[next.var()]
    ) {
        //There is no more to branch on. Satisfying assignment found.
        if (order_heap.empty()) {
            next = lit_Undef;
            break;
        }

        const Var next_var = order_heap.removeMin();
        next = Lit(next_var, !pickPolarity(next_var));
    }

    //Try to use reachability to pick a literal that dominates this one
    if (next != lit_Undef
        && (mtrand.randInt(conf.dominPickFreq) == 1)
    ) {
        const Lit lit2 = control->litReachable[next.toInt()].lit;
        if (lit2 != lit_Undef
            && value(lit2.var()) == l_Undef
            && control->decision_var[lit2.var()]
        ) {
            //insert this one back, just in case the litReachable isn't entirely correct
            //which would be a MAJOR bug, btw
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

    assert(next == lit_Undef || control->decision_var[next.var()]);
    return next;
}

/**
@brief Performs on-the-fly self-subsuming resolution

Only uses binary and tertiary clauses already in the watchlists in native
form to carry out the forward-self-subsuming resolution
*/
void CommandControl::minimiseLearntFurther(vector<Lit>& cl)
{
    assert(conf.doCache);
    stats.OTFShrinkAttempted++;

    //Set all literals' seen[lit] = 1 in learnt clause
    //We will 'clean' the learnt clause by setting these to 0
    for (uint32_t i = 0; i < cl.size(); i++)
        seen[cl[i].toInt()] = 1;

    //Do cache-based minimisation and watchlist-based minimisation
    //one-by-one on the literals. Order could be enforced to get smallest
    //clause, but it doesn't really matter, I think
    size_t done = 0;
    for (vector<Lit>::iterator
        l = cl.begin(), end = cl.end()
        ; l != end && done < 30
        ; l++
    ) {
        if (seen[l->toInt()] == 0)
            continue;
        done++;

        Lit lit = *l;

        //Cache-based minimisation
        const TransCache& cache1 = control->implCache[l->toInt()];
        for (vector<LitExtra>::const_iterator it = cache1.lits.begin(), end2 = cache1.lits.end(); it != end2; it++) {
            seen[(~(it->getLit())).toInt()] = 0;
        }

        //Watchlist-based minimisation
        const vec<Watched>& ws = watches[(~lit).toInt()];
        for (vec<Watched>::const_iterator
            i = ws.begin()
            , end = ws.end()
            ; i != end
            ; i++
        ) {
            if (i->isBinary()) {
                seen[(~i->getOtherLit()).toInt()] = 0;
                continue;
            }

            if (i->isTriClause()) {
                if (seen[i->getOtherLit2().toInt()]) {
                    seen[(~i->getOtherLit()).toInt()] = 0;
                }
                if (seen[i->getOtherLit().toInt()]) {
                    seen[(~i->getOtherLit2()).toInt()] = 0;
                }
            }
        }
    }

    //Finally, remove the literals that have seen[literal] = 0
    //Here, we can count do stats, etc.
    uint32_t removedLits = 0;
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
            removedLits++;

        seen[i->toInt()] = 0;
    }
    stats.OTFShrinkedClause += (removedLits > 0);
    cl.resize(cl.size() - (i-j));

    #ifdef VERBOSE_DEBUG
    cout << "c Removed further " << removedLits << " lits" << endl;
    #endif
}

void CommandControl::insertVarOrder(const Var x)
{
    if (!order_heap.inHeap(x)
        && control->decision_var[x]
    ) {
        order_heap.insert(x);
    }
}

bool CommandControl::VarFilter::operator()(uint32_t var) const
{
    return (cc->value(var) == l_Undef && control->decision_var[var]);
}

void CommandControl::setNeedToInterrupt()
{
    needToInterrupt = true;
}

void CommandControl::printAgilityStats()
{
    cout
    << ", confl: " << std::setw(6) << stats.conflStats.numConflicts
    << ", rest: " << std::setw(6) << stats.numRestarts
    << ", agility : " << std::setw(6) << std::fixed << std::setprecision(2)
    << agility.getAgility()

    << ", agilityLimit : " << std::setw(6) << std::fixed << std::setprecision(2)
    << conf.agilityLimit

    << ", agilityHist: " << std::setw(6) << std::fixed << std::setprecision(3)
    << agilityHist.getAvg()

    << ", agilityHistLong: " << std::setw(6) << std::fixed << std::setprecision(3)
    << agilityHist.getAvgLong()
    << endl;
}

uint64_t CommandControl::sumConflicts() const
{
    return control->sumStats.conflStats.numConflicts + stats.conflStats.numConflicts;
}

size_t CommandControl::hyperBinResAll()
{
    size_t added = 0;

    for(std::set<BinaryClause>::const_iterator
        it = control->needToAddBinClause.begin()
        , end = control->needToAddBinClause.end()
        ; it != end
        ; it++
    ) {
        control->attachBinClause(it->getLit1(), it->getLit2(), true, false);
        added++;
    }

    return added;
}

size_t CommandControl::removeUselessBins()
{
    size_t removed = 0;
    if (conf.doRemUselessBins) {
        for(std::set<BinaryClause>::iterator
            it = uselessBin.begin()
            , end = uselessBin.end()
            ; it != end
            ; it++
        ) {
            //cout << "Removing binary clause: " << *it << endl;
            removeWBin(control->watches, it->getLit1(), it->getLit2(), it->getLearnt());
            removeWBin(control->watches, it->getLit2(), it->getLit1(), it->getLearnt());

            //Update stats
            if (it->getLearnt()) {
                control->learntsLits -= 2;
                control->numBinsLearnt--;
            } else {
                control->clausesLits -= 2;
                control->numBinsNonLearnt--;
            }
            removed++;

            #ifdef VERBOSE_DEBUG_FULLPROP
            cout << "Removed bin: "
            << it->getLit1() << " , " << it->getLit2()
            << " , learnt: " << it->getLearnt() << endl;
            #endif
        }
    }
    uselessBin.clear();

    return removed;
}

//Only used to generate nice Graphviz graphs
string CommandControl::simplAnalyseGraph(
    PropBy conflHalf
    , vector<Lit>& out_learnt
    , uint32_t& out_btlevel, uint32_t &glue
) {
    int pathC = 0;
    Lit p = lit_Undef;

    out_learnt.push_back(lit_Undef);      // (leave room for the asserting literal)
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
                    out_learnt.push_back(q);

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
    out_learnt[0] = ~p;

    // clear out seen
    for (uint32_t j = 0; j != out_learnt.size(); j++)
        seen[out_learnt[j].var()] = 0;    // ('seen[]' is now cleared)

    //Calculate glue
    glue = calcGlue(out_learnt);

    return resolutions.str();
}

//Only used to generate nice Graphviz graphs
void CommandControl::genConfGraph(const PropBy conflPart)
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
    vector<Lit> out_learnt;
    uint32_t out_btlevel, glue;
    const std::string res = simplAnalyseGraph(conflPart, out_learnt, out_btlevel, glue);
    file << "vertK -> dummy;";
    file << "dummy "
    << "[ "
    << " shape=record"
    << " , label=\"{"
    << " clause: " << out_learnt
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
