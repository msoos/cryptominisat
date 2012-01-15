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
#include "RestartPrinter.h"
#include <omp.h>

//#define VERBOSE_DEBUG_GEN_CONFL_DOT

#ifdef VERBOSE_DEBUG
#define VERBOSE_DEBUG_GEN_CONFL_DOT
#endif

/**
@brief Sets a sane default config and allocates handler classes
*/
CommandControl::CommandControl(const SolverConf& _conf, ThreadControl* _control) :
        Solver(_control->clAllocator, AgilityData(_conf.agilityG, _conf.agilityLimit))

        //Sync
        , lastSumConfl(0)
        , lastLong(0)
        , lastBin(0)
        , lastUnit(0)

        // Stats
        , numConflicts(0)
        , numRestarts(0)
        , decisions(0)
        , decisions_rnd(0)

        //Conflict generation
        , max_literals(0)
        , tot_literals(0)
        , furtherClMinim(0)
        , numShrinkedClause(0)
        , numShrinkedClauseLits(0)

        //variables
        , control(_control)
        , conf(_conf)
        , needToInterrupt(false)
        , order_heap(VarOrderLt(varData))
{
    mtrand.seed(conf.origSeed);
}

CommandControl::~CommandControl()
{
}

Var CommandControl::newVar(const bool dvar)
{
    const Var var = Solver::newVar(dvar);
    if (dvar)
        insertVarOrder(var);

    return var;
}

template<class T, class T2>
void CommandControl::printStatsLine(std::string left, T value, T2 value2, std::string extra)
{
    std::cout << std::fixed << std::left << std::setw(27) << left << ": " << std::setw(11) << std::setprecision(2) << value << " (" << std::left << std::setw(9) << std::setprecision(2) << value2 << " " << extra << ")" << std::endl;
}

template<class T>
void CommandControl::printStatsLine(std::string left, T value, std::string extra)
{
    std::cout << std::fixed << std::left << std::setw(27) << left << ": " << std::setw(11) << std::setprecision(2) << value << extra << std::endl;
}

void CommandControl::printStats()
{
    double   cpu_time = cpuTime() - startTime;
    uint64_t mem_used = memUsed();

    //Restarts stats
    printStatsLine("c restarts", numRestarts);

    //Learnts stats
    printStatsLine("c unitaries"
                    , getNumUnitaries()
                    , (double)getNumUnitaries()/(double)nVars()*100.0
                    , "% of vars");

    //Clause-shrinking through watchlists
    printStatsLine("c OTF cl watch-shrink"
                    , numShrinkedClause
                    , (double)numShrinkedClause/(double)numConflicts
                    , "clauses/conflict");

    printStatsLine("c OTF cl watch-sh-lit"
                    , numShrinkedClauseLits
                    , (double)numShrinkedClauseLits/(double)numShrinkedClause
                    , " lits/clause");

    printStatsLine("c tried to recurMin cls"
                    , furtherClMinim
                    , (double)furtherClMinim/(double)numConflicts*100.0
                    , " % of conflicts");

    //Search stats
    printStatsLine("c conflicts", numConflicts, (double)numConflicts/cpu_time, "/ sec");
    printStatsLine("c decisions", decisions, (double)decisions_rnd*100.0/(double)decisions, "% random");
    printStatsLine("c bogo-props", bogoProps, (double)bogoProps/cpu_time, "/ sec");
    printStatsLine("c props", propagations, (double)propagations/cpu_time, "/ sec");
    printStatsLine("c conflict literals", tot_literals, (double)(max_literals - tot_literals)*100.0/ (double)max_literals, "% deleted");

    //General stats
    printStatsLine("c Memory used", (double)mem_used / 1048576.0, " MB");
    #if !defined(_MSC_VER) && defined(RUSAGE_THREAD)
    printStatsLine("c single-thread CPU time", cpu_time, " s");
    #else
    printStatsLine("c all-threads sum CPU time", cpu_time, " s");
    #endif
}

/**
@brief Revert to the state at given level
*/
void CommandControl::cancelUntil(uint32_t level)
{
    #ifdef VERBOSE_DEBUG
    std::cout << "Canceling until level " << level;
    if (level > 0) std::cout << " sublevel: " << trail_lim[level];
    std::cout << std::endl;
    #endif

    if (decisionLevel() > level) {

        for (int sublevel = trail.size()-1; sublevel >= (int)trail_lim[level]; sublevel--) {
            Var var = trail[sublevel].var();
            #ifdef VERBOSE_DEBUG
            std::cout << "Canceling var " << var+1 << " sublevel: " << sublevel << std::endl;
            #endif
            assert(value(var) != l_Undef);
            assigns[var] = l_Undef;
            #ifdef ANIMATE3D
            std:cerr << "u " << var << std::endl;
            #endif
            insertVarOrder(var);
        }
        qhead = trail_lim[level];
        trail.resize(trail_lim[level]);
        trail_lim.resize(level);
    }

    #ifdef VERBOSE_DEBUG
    std::cout << "Canceling finished. (now at level: " << decisionLevel() << " sublevel: " << trail.size()-1 << ")" << std::endl;
    #endif
}

/**
@brief    Analyze conflict and produce a reason clause.

Post-condition: 'out_learnt[0]' is the asserting literal at level 'out_btlevel'
*/
void CommandControl::analyze(PropBy confl, vector<Lit>& out_learnt, uint32_t& out_btlevel, uint32_t &glue)
{
    assert(out_learnt.empty());
    assert(decisionLevel() > 0);

    int pathC = 0;
    Lit p = lit_Undef;
    int index = trail.size() - 1;
    out_btlevel = 0;

    uint32_t numIterations = 0;

    //std::cout << "---- Start analysis -----" << std::endl;
    out_learnt.push_back(lit_Undef); //make space for ~p
    do {
        numIterations++;

        //Add literals from 'confl' to clause
        switch (confl.getType()) {
            case tertiary_t : {
                const Var var = confl.getOtherLit2().var();
                if (!seen[var] && varData[var].level > 0) {
                    varBumpActivity(var);
                    seen[var] = 1;
                    if (varData[var].level == decisionLevel())
                        pathC++;
                    else
                        out_learnt.push_back(confl.getOtherLit2());
                }
            }
            //NO BREAK, since tertiary is like binary, just one more lit

            case binary_t : {
                if (p == lit_Undef) {
                    const Var var = failBinLit.var();
                    if (!seen[var] && varData[var].level > 0) {
                        varBumpActivity(var);
                        seen[var] = 1;
                        if (varData[var].level == decisionLevel())
                            pathC++;
                        else
                            out_learnt.push_back(failBinLit);
                    }
                }

                const Var var = confl.getOtherLit().var();
                if (!seen[var] && varData[var].level > 0) {
                    varBumpActivity(var);
                    seen[var] = 1;
                    if (varData[var].level == decisionLevel())
                        pathC++;
                    else
                        out_learnt.push_back(confl.getOtherLit());
                }
                break;
            }

            case clause_t : {
                Clause& cl = *clAllocator->getPointer(confl.getClause());
                for (uint32_t j = 0, size = cl.size(); j != size; j++) {
                    if (p != lit_Undef
                        && j == clauseData[cl.getNum()].litPos[confl.getWatchNum()])
                    continue;

                    const Lit q = cl[j];
                    const Var var = q.var();
                    assert(varData[var].level <= decisionLevel());

                    if (!seen[var] && varData[var].level > 0) {
                        varBumpActivity(var);
                        seen[var] = 1;
                        if (varData[var].level == decisionLevel())
                            pathC++;
                        else
                            out_learnt.push_back(q);
                    }
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
        confl = varData[p.var()].reason;
        seen[p.var()] = 0; //This clears out vars that haven't been added to out_learnt, but their 'seen' has been set
        pathC--;
        //std::cout << "Next 'p' to look at: " << p << std::endl;
    } while (pathC > 0);
    out_learnt[0] = ~p;

    assert(pathC == 0);
    max_literals += out_learnt.size();
    toClear = out_learnt;

    //Recursive-simplify conflict clause:
    trace_reasons.clear();
    uint32_t abstract_level = 0;
    for (size_t i = 1; i < out_learnt.size(); i++)
        abstract_level |= abstractLevel(out_learnt[i].var()); // (maintain an abstraction of levels involved in conflict)

    find_removable(out_learnt, abstract_level);
    prune_removable(out_learnt);

    //Clear seen
    for (vector<Lit>::const_iterator it = toClear.begin(), end = toClear.end(); it != end; it++)
        seen[it->var()] = 0;
    toClear.clear();

    //Cache-based minimisation
    if (conf.doCache
        && conf.doMinimLearntMore
        && out_learnt.size() > 1
        && (conf.doAlwaysFMinim
            || calcNBLevels(out_learnt) < 0.65*glueHist.getAvgAll()
            || out_learnt.size() < 0.65*conflSizeHist.getAvgAll()
            )
    ) {
        minimiseLearntFurther(out_learnt);
    }

    //Calc stats
    glue = calcNBLevels(out_learnt);
    tot_literals += out_learnt.size();

    //Print fully minimised clause
    #ifdef VERBOSE_DEBUG_OTF_GATE_SHORTEN
    std::cout << "Final clause: " << out_learnt << std::endl;
    for (uint32_t i = 0; i < out_learnt.size(); i++) {
        std::cout << "lev out_learnt[" << i << "]:" << varData[out_learnt[i].var()].level << std::endl;
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
    std::cout << "out_btlevel: " << out_btlevel << std::endl;
    #endif
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
                if (i == clauseData[cl.getNum()].litPos[rp.getWatchNum()])
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
                if (i == clauseData[cl.getNum()].litPos[rp.getWatchNum()])
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

void CommandControl::addToThreads(const size_t oldTrailSize)
{
    vector<Lit> lits(1);
    #pragma omp critical
    {
        for(size_t i = oldTrailSize; i < trail.size(); i++) {
            lits[0] = trail[i];
            control->newClauseByThread(lits, 1, lastSumConfl);
            lastUnit++;
        }
    }
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
        numRestarts ++;
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
    std::cout << "c started CommandControl::search()" << std::endl;
    #endif //VERBOSE_DEBUG

    //Loop until restart or finish (SAT/UNSAT)
    while (true) {
        assert(ok);
        const PropBy confl = propagate();

        #ifdef VERBOSE_DEBUG
        std::cout << "c CommandControl::search() has finished propagation" << std::endl;
        #endif //VERBOSE_DEBUG

        if (!confl.isNULL()) {
            printAgilityStats();

            //If restart is needed, set it as so
            checkNeedRestart(params, rest);

            if (!handle_conflict(params, confl))
                return l_False;

            if (!addOtherClauses())
                return l_False;

        } else {
            assert(ok);
            //TODO Enable this through some ingenious locking
            /*if (conf.doCache && decisionLevel() == 1)
                saveOTFData();*/

            //If restart is needed, restart here
            if (params.needToStopSearch
                || lastSumConfl > control->getNextCleanLimit()
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
            next = p;
            break;
        }
    }

    if (next == lit_Undef) {
        // New variable decision:
        decisions++;
        next = pickBranchLit();

        if (next == lit_Undef)
            return l_True;
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
            std::cout << "c needToInterrupt is set, restartig as soon as possible!" << std::endl;
        params.needToStopSearch = true;
    }

    // Reached bound on number of conflicts?
    if (agility.getAgility() < conf.agilityLimit)
        agility.tooLow(params.conflictsDoneThisRestart);

    if ((conf.restartType == geom_restart
            && params.conflictsDoneThisRestart > rest
        ) || (conf.restartType == glue_restart
            && glueHistory.isvalid()
            && 0.95*glueHistory.getAvgDouble() > glueHistory.getAvgAllDouble()
        )
    ) {
        //Now check agility
        if (agility.getNumTooLow() > conf.numTooLowAgilitiesLimit) {
            #ifdef DEBUG_DYNAMIC_RESTART
            if (glueHistory.isvalid()) {
                std::cout << "glueHistory.getavg():" << glueHistory.getavg() <<std::endl;
                std::cout << "totalSumOfGlue:" << totalSumOfGlue << std::endl;
                std::cout << "conflicts:" << conflicts<< std::endl;
                std::cout << "compTotSumGlue:" << compTotSumGlue << std::endl;
                std::cout << "conflicts-compTotSumGlue:" << conflicts-compTotSumGlue<< std::endl;
            }
            #endif

            if (conf.verbosity >= 3)
                std::cout << "c Agility was too low, restarting as soon as possible!" << std::endl;
            params.needToStopSearch = true;
        } else {
            rest *= conf.restart_inc;
        }
    }

    if (params.conflictsDoneThisRestart > params.conflictsToDo) {
        if (conf.verbosity >= 3)
            std::cout << "c Over limit of conflicts for this restart, restarting as soon as possible!" << std::endl;
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
    std::cout << "Handling conflict" << std::endl;
    #endif

    //Stats
    uint32_t backtrack_level;
    uint32_t glue;
    vector<Lit> learnt_clause;
    numConflicts++;
    params.conflictsDoneThisRestart++;
    if (conf.doPrintConflDot)
        genConfGraph(confl);

    if (decisionLevel() == 0)
        return false;

    analyze(confl, learnt_clause, backtrack_level, glue);
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
    std::cout << "Learning:" << learnt_clause << std::endl;
    std::cout << "reverting var " << learnt_clause[0].var()+1 << " to " << !learnt_clause[0].sign() << std::endl;
    #endif
    assert(value(learnt_clause[0]) == l_Undef);

    //Set up everything to get the clause
    std::sort(learnt_clause.begin()+1, learnt_clause.end(), PolaritySorter(varData));
    glue = std::min(glue, MAX_THEORETICAL_GLUE);
    Clause *cl;

    //Get new clause
    #pragma omp critical
    {
        syncFromThreadControl();
        cl = control->newClauseByThread(learnt_clause, glue, lastSumConfl);
    }

    //Attach new clause
    switch (learnt_clause.size()) {
        case 1:
            //Unitary learnt
            lastUnit++;
            enqueue(learnt_clause[0]);
            assert(backtrack_level == 0 && "Unit clause learnt, so must cancel until level 0, right?");

            break;
        case 2:
            //Binary learnt
            lastBin++;
            attachBinClause(learnt_clause[0], learnt_clause[1], true);
            enqueue(learnt_clause[0], PropBy(learnt_clause[1]));
            break;

        case 3:
            //3-long almost-normal learnt
            lastLong++;
            attachClause(*cl);
            enqueue(learnt_clause[0], PropBy(learnt_clause[1], learnt_clause[2]));
            break;

        default:
            //Normal learnt
            lastLong++;
            attachClause(*cl);
            enqueue(learnt_clause[0], PropBy(clAllocator->getOffset(cl), 0));
            break;
    }

    varDecayActivity();

    return true;
}

/**
@brief Initialises model, restarts, learnt cluause cleaning, burst-search, etc.
*/
void CommandControl::initialiseSolver()
{
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
    numRestarts = 0;

    //Set up sync
    #pragma omp critical //sync can only be done in critical section
    syncFromThreadControl();
    const bool ret = addOtherClauses();
    assert(ret);

    //Set up vars
    for(Var i = 0; i < control->nVars(); i++) {
        newVar(control->decision_var[i]);
    }

    //Set elimed/replaced
    size_t i = 0;
    for(vector<VarData>::iterator it = varData.begin(), end = varData.end(); it != end; it++, i++) {
        it->elimed = control->varData[i].elimed;
    }

    //Set already set vars
    for(vector<Lit>::const_iterator it = control->trail.begin(), end = control->trail.end(); it != end; it++) {
        enqueue(*it);
    }
    ok = propagate().isNULL();
    assert(ok);

    order_heap.filter(VarFilter(this, control));

    //Attach every binary clause
    uint32_t wsLit = 0;
    for(vector<vec<Watched> >::const_iterator it = control->watches.begin(), end = control->watches.end(); it != end; it++, wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        for(vec<Watched>::const_iterator it2 = it->begin(), end2 = it->end(); it2 != end2; it2++) {
            //Only binary clause
            if (!it2->isBinary())
                continue;

            //Only attach the clause once
            if (it2->getOtherLit() < lit)
                attachBinClause(lit, it2->getOtherLit(), it2->getLearnt());
        }
    }

    //Set up clauses & prop data
    for(vector<Clause*>::const_iterator it = control->clauses.begin(), end = control->clauses.end(); it != end; it++) {
        attachClause(**it);
    }

    for(vector<Clause*>::const_iterator it = control->learnts.begin(), end = control->learnts.end(); it != end; it++) {
        attachClause(**it);
    }
}

void CommandControl::syncFromThreadControl()
{
    for(size_t i = lastLong; i < control->longLearntsToAdd.size(); i++)
    {
        longToAdd.push_back(control->longLearntsToAdd[i]);
    }
    lastLong = control->longLearntsToAdd.size();

    for(size_t i = lastBin; i < control->binLearntsToAdd.size(); i++)
    {
        binToAdd.push_back(control->binLearntsToAdd[i]);
    }
    lastBin = control->binLearntsToAdd.size();

    for(size_t i = lastUnit; i < control->unitLearntsToAdd.size(); i++)
    {
        unitToAdd.push_back(control->unitLearntsToAdd[i]);
    }
    lastUnit = control->unitLearntsToAdd.size();

    /*if (!longToAdd.empty() || !binToAdd.empty() || !unitToAdd.empty()) {
        std::cout << "thread num: " << omp_get_thread_num() << std::endl;
        std::cout << "longToAdd size: " << longToAdd.size() << std::endl;
        std::cout << "binToAdd size: " << binToAdd.size() << std::endl;
        std::cout << "unitToAdd size: " << unitToAdd.size() << std::endl;
        std::cout << "----------------" << std::endl;
    }*/
}

struct MyAttachSorter
{
    MyAttachSorter(const vector<VarData>& _varData, const vector<lbool>& _assigns, const Clause& _cl) :
        varData(_varData)
        , assigns(_assigns)
        , cl(_cl)
    {
    }

    bool operator()(const uint16_t& a, const uint16_t& b) const
    {
        const Lit first = cl[a];
        const Lit second = cl[b];

        const lbool val1 = assigns[first.var()] ^ first.sign();
        const lbool val2 = assigns[second.var()] ^ second.sign();

        //True is better than anything else
        if (val1 == l_True && val2 != l_True) return true;
        if (val2 == l_True && val1 != l_True) return false;

        //After True, Undef is better
        if (val1 == l_Undef && val2 != l_Undef) return true;
        if (val2 == l_Undef && val1 != l_Undef) return false;
        //Note: l_False is last

        assert(val1 == val2);

        //Highest level at the beginning
        return (varData[first.var()].level > varData[second.var()].level);
    }

    const vector<VarData>& varData;
    const vector<lbool>& assigns;
    const Clause& cl;
};

bool CommandControl::addOtherClauses()
{
    assert(ok);
    PropBy ret;
    PropBy backupRet;

    size_t i;
    for(i = 0; i < unitToAdd.size(); i++) {
        const Lit lit = unitToAdd[i];

        //set at level 0, all is fine and dandy! Skip.
        if (value(lit) == l_True && varData[lit.var()].level == 0)
            continue;

        //Either not set, not at level 0, etc.
        cancelUntil(0);

        //Undef, enqueue it
        if (value(lit) == l_Undef) {
            enqueue(lit);
            continue;
        }

        //Only option remaining: it's false
        assert(value(lit) == l_False);
        return false;
    }
    unitToAdd.clear();

    for(i = 0; i < binToAdd.size(); i++) {
        const BinaryClause binCl = binToAdd[i];
        if (!handleNewBin(binCl))
            return false;
    }
    binToAdd.clear();

    for(i = 0; i < longToAdd.size(); i++) {
        const Clause& cl = *longToAdd[i];
        if (!handleNewLong(cl))
            return false;
    }
    longToAdd.clear();

    return true;
}

bool CommandControl::handleNewLong(const Clause& cl)
{
    //A bit of indirection. We will sort the indexes of the literals
    vector<uint16_t> lits(cl.size());
    for(uint16_t i = 0; i < cl.size(); i++) {
        lits[i] = i;
    }
    //Special sort
    std::sort(lits.begin(), lits.end(), MyAttachSorter(varData, assigns, cl));

    attachClause(cl, lits[0], lits[1], false);
    //std::cout << "Attaching clause " << cl << " to thread: " << omp_get_thread_num() << std::endl;

    //If both l_Undef or one is l_True, then 'simple' attach
    if ((value(cl[lits[0]]) == l_Undef && value(cl[lits[1]]) == l_Undef)
        || (value(cl[lits[0]]) == l_True)
    ) {
        return true;
    }

    //At this point, it is for sure that everything above 0 position is l_False
    for(uint16_t i = 1; i < cl.size(); i++) {
        assert(value(cl[lits[i]]) == l_False);
    }

    const ClauseOffset offset = clAllocator->getOffset(&cl);

    //Exactly one l_Undef, rest is l_False
    if (value(cl[lits[0]]) == l_Undef) {
        if (cl.size() == 3) {
            enqueue(cl[lits[0]], PropBy(cl[lits[1]], cl[lits[2]]));
        } else {
            enqueue(cl[lits[0]], PropBy(offset, 0)); //0 because 'handle_conflict'-s enqeue() is also with 0
        }

        //std::cout << "Attached fun 1, thread: " << omp_get_thread_num() << std::endl;
        return true;
    }

    const size_t lastLevel = varData[cl[lits[0]].var()].level;

    //All are level 0
    if (lastLevel == 0) {
        ok = false;
        return false;
    }

    //Cancel at least the first literal
    assert(value(cl[lits[0]]) == l_False);
    cancelUntil(lastLevel-1);
    assert(value(cl[lits[0]]) == l_Undef);

    //If only first got unassigned at this level
    if (value(cl[lits[1]]) == l_False) {
        for(uint16_t i = 1; i < cl.size(); i++) {
            assert(value(cl[lits[i]]) == l_False);
        }
        if (cl.size() == 3) {
            enqueue(cl[lits[0]], PropBy(cl[lits[1]], cl[lits[2]]));
        } else {
            enqueue(cl[lits[0]], PropBy(offset, 0)); //0 because 'handle_conflict'-s enqeue() is also with 0
        }

        //std::cout << "Attached fun 2, thread: " << omp_get_thread_num() << std::endl;
        return true;
    } else {
        assert(varData[cl[lits[0]].var()].level == varData[cl[lits[1]].var()].level);
        //Nothing to do, it's all l_Undef now, which is fine
    }

    return true;
}

bool CommandControl::handleNewBin(const BinaryClause& binCl)
{
    Lit lits[2];
    lits[0] = binCl.getLit1();
    lits[1] = binCl.getLit2();

    //We need to attach, no matter what
    attachBinClause(lits[0], lits[1], binCl.getLearnt(), false);

    //If satisfied, simple attach
    if (value(lits[0]) == l_True || value(lits[1]) == l_True)
        return true;

    //If one is unassigned, it should be the first
    if (value(lits[1]) == l_Undef) {
        std::swap(lits[0], lits[1]);
    }

    //Both l_Undef
    if (value(lits[1]) == l_Undef) {
        assert(value(lits[0]) == l_Undef);
        return true;
    }

    //One Undef, one False, so enqueue
    if (value(lits[0]) == l_Undef) {
        assert(value(lits[1]) == l_False);
        enqueue(lits[0], PropBy(lits[1]));
        return true;
    }

    //Both false, oops, cancel, then enqueue
    assert(value(lits[0]) == l_False);
    assert(value(lits[1]) == l_False);

    //lit[0] is assigned at the highest level
    if (varData[lits[0].var()].level < varData[lits[1].var()].level)
        std::swap(lits[0], lits[1]);

    //Both are assigned at level 0
    if (varData[lits[0].var()].level == 0) {
        cancelUntil(0);
        ok = false;
        return false;
    }

    //Cancel until the point
    cancelUntil(varData[lits[0].var()].level - 1);

    //If the other lit didn't get unassigned, then enqueue
    if (value(lits[1]) == l_False) {
        enqueue(lits[0], PropBy(lits[1]));
        return true;
    } else {
        //If both got unassigned, that's only possible, because they were on the same level
        assert(varData[lits[0].var()].level == varData[lits[1].var()].level);
        //Nothing to do, it's all l_Undef now, which is fine
    }

    return true;
}

lbool CommandControl::burstSearch()
{
    lbool status;

    const double backup_rand = conf.random_var_freq;
    const RestType backup_restType = conf.restartType;
    const int backup_polar_mode = conf.polarity_mode;

    conf.random_var_freq = 1;
    conf.polarity_mode = polarity_rnd;
    uint64_t rest_burst = conf.burstSearchLen;
    status = search(SearchFuncParams(rest_burst), rest_burst);

    conf.random_var_freq = backup_rand;
    conf.restartType = backup_restType;
    conf.polarity_mode = backup_polar_mode;

    #pragma omp critical
    std::cout << "c after burst" << omp_get_thread_num()
    << " " << numRestarts
    << " " << numConflicts
    << " " << order_heap.size()
    << std::endl;

    return status;
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
    startTime = cpuTime();
    initialiseSolver();
    lbool status = l_Undef; //Current status

    uint64_t lastRestartPrint = numConflicts;

    //Burst seach
    status = burstSearch();
    for(size_t i = 0; i < control->nVars(); i++) {
        varData[i].polarity = control->getSavedPolarity(i);
    }

    // Search:
    uint64_t rest = conf.restart_first;
    while (status == l_Undef
        && !needToInterrupt
        && lastSumConfl < maxConfls
    ) {
        assert(numConflicts < maxConfls);
        status = search(SearchFuncParams(maxConfls-numConflicts), rest);
        rest *= conf.restart_inc;
        if (status != l_Undef)
            break;

        if (lastSumConfl >= maxConfls) {
            std::cout << "c thread(maxconfl) Trail size: " << trail.size() << std::endl;
            break;
        }

        if (lastSumConfl > control->getNextCleanLimit()) {
            std::cout << "c th " << omp_get_thread_num() << " cleaning"
            << " getNextCleanLimit(): " << control->getNextCleanLimit()
            << " numConflicts : " << numConflicts
            << " lastSumConfl: " << lastSumConfl
            << " maxConfls:" << maxConfls << std::endl;
            std::cout << "c thread " << omp_get_thread_num()
            << " Trail size: " << trail.size() << std::endl;

            //Have to wait for everyone to be here, i.e. shared their data
            //with threadcontrol, so we can all be up to sync
            #pragma omp barrier
            syncFromThreadControl();
            #pragma omp barrier
            bool ret = addOtherClauses();
            assert(ret && "TODO: must handle this correctly!");

            #pragma omp barrier
            #pragma omp single
            control->moveReduce(); //This also clears unit, bin, and long

            //Detach clauses that have been scheduled
            for(vector<Clause*>::const_iterator it = control->toDetach.begin(), end = control->toDetach.end(); it != end; it++) {
                //std::cout << "Detaching clause " << **it << " from thread: " << omp_get_thread_num() << std::endl;
                detachClause(**it);
            }

            //moveReduce() clear()-ed these, so we have to zero them now
            lastUnit = 0;
            lastBin = 0;
            lastLong = 0;


            #pragma omp barrier
            #pragma omp single
            {
                control->toDetachFree();
                control->consolidateMem();
            }
            #pragma omp barrier

            if (!ret) {
                status = l_False;
                break;
            }
        }

        //if ((lastRestartPrint + 5000) < numConflicts) {
            #pragma omp critical
            std::cout << "c " << omp_get_thread_num()
            << " " << std::setw(6) << numRestarts
            << " " << std::setw(7) << numConflicts
            << " " << std::setw(7) << order_heap.size()
            << " glue"
            << " " << std::right << glueHist.getAvgPrint(1, 5)
            << "/" << std::left << glueHist.getAvgAllPrint(1, 5)
            << " agil"
            << " " << std::right << agilityHist.getAvgPrint(3, 5)
            << "/" << std::left<< agilityHist.getAvgAllPrint(3, 5)
            << " confllen"
            << " " << std::right << conflSizeHist.getAvgPrint(1, 5)
            << "/" << std::left << conflSizeHist.getAvgAllPrint(1, 5)
            << " branch"
            << " " << std::right << branchDepthHist.getAvgPrint(1, 5)
            << "/" << std::left  << branchDepthHist.getAvgAllPrint(1, 5)
            << " branchd"
            << " " << std::right << branchDepthDeltaHist.getAvgPrint(1, 4)
            << "/" << std::left << branchDepthDeltaHist.getAvgAllPrint(1, 4)

            << " trail"
            << " " << std::right << trailDepthHist.getAvgPrint(0, 7)
            << "/" << std::left << trailDepthHist.getAvgAllPrint(0, 7)

            << " traild"
            << " " << std::right << trailDepthDeltaHist.getAvgPrint(0, 5)
            << "/" << std::left << trailDepthDeltaHist.getAvgAllPrint(0, 5)
            << std::endl;
            lastRestartPrint = numConflicts;
        //}
    }

    #ifdef VERBOSE_DEBUG
    if (status == l_True)
        std::cout << "Solution  is SAT" << std::endl;
    else if (status == l_False)
        std::cout << "Solution is UNSAT" << std::endl;
    else
        std::cout << "Solutions is UNKNOWN" << std::endl;
    #endif //VERBOSE_DEBUG

    if (status == l_True) {
        solution = assigns;
    } else if (status == l_False) {
        if (conflict.size() == 0)
            ok = false;
    }
    cancelUntil(0);

    //#ifdef VERBOSE_DEBUG
#pragma omp critical
    {
        std::cout << "c th " << omp_get_thread_num()
        << " ---------" << std::endl;

        std::cout << "c CommandControl::solve() finished"
        << " status: " << status
        << " control->getNextCleanLimit(): " << control->getNextCleanLimit()
        << " numConflicts : " << numConflicts
        << " lastSumConfl: " << lastSumConfl
        << " maxConfls:" << maxConfls
        << std::endl;
        printStats();

        std::cout << "c th " << omp_get_thread_num()
        << " ---------" << std::endl;

    }
    //#endif

    return status;
}

inline int64_t abs64(int64_t a)
{
    if (a < 0) return -a;
    return a;
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
    std::cout << "decision level: " << decisionLevel() << " ";
    #endif

    Var next = var_Undef;
    bool sign = true;

    // Random decision:
    double rand = mtrand.randDblExc();
    if (next == var_Undef
        && rand < conf.random_var_freq
        && !order_heap.empty()
    ) {
        next = order_heap[mtrand.randInt(order_heap.size()-1)];

        if (value(next) == l_Undef && control->decision_var[next]) {
            decisions_rnd++;
            sign = !getPolarity(next);
        }
    }

    // Activity based decision:
    while (next == var_Undef
      || value(next) != l_Undef
      || !control->decision_var[next]
    ) {
        //There is no more to branch on. Satisfying assignment found.
        if (order_heap.empty()) {
            next = var_Undef;
            break;
        }

        next = order_heap.removeMin();

        //Try to use reachability to pick a literal that dominates this one
        if (value(next) == l_Undef
            && control->decision_var[next]
        ) {
            sign = !getPolarity(next);

            const Lit nextLit = Lit(next, sign);
            const Lit lit2 = control->litReachable[nextLit.toInt()].lit;
            if (lit2 != lit_Undef
                && value(lit2.var()) == l_Undef
                && control->decision_var[lit2.var()]
                && mtrand.randInt(1) == 1  //only pick dominating literal 50% of the time
            ) {
                //insert this one back, just in case the litReachable isn't entirely correct
                insertVarOrder(next);

                //Save this literal & sign
                next = lit2.var();

                //if '-a V b' then litReachable[a] = b, so we must pick '-b'
                //therefore, must invert here
                sign = !lit2.sign();
            }
        }
    }

    //No vars in heap: solution found
    if (next == var_Undef) {
        #ifdef VERBOSE_DEBUG
        std::cout << "SAT!" << std::endl;
        #endif
        return lit_Undef;
    }

    const Lit toPick(next, sign);
    #ifdef VERBOSE_DEBUG
    std::cout << "decided on: " << toPick << std::endl;
    #endif

    assert(control->decision_var[toPick.var()]);
    return toPick;
}

/**
@brief Performs on-the-fly self-subsuming resolution

Only uses binary and tertiary clauses already in the watchlists in native
form to carry out the forward-self-subsuming resolution
*/
void CommandControl::minimiseLearntFurther(vector<Lit>& cl)
{
    assert(conf.doCache);
    furtherClMinim++;

    //Set all literals' seen[lit] = 1 in learnt clause
    //We will 'clean' the learnt clause by setting these to 0
    for (uint32_t i = 0; i < cl.size(); i++)
        seen[cl[i].toInt()] = 1;

    //Do cache-based minimisation and watchlist-based minimisation
    //one-by-one on the literals. Order could be enforced to get smallest
    //clause, but it doesn't really matter, I think
    for (vector<Lit>::iterator l = cl.begin(), end = cl.end(); l != end; l++) {
        if (seen[l->toInt()] == 0)
            continue;

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
    numShrinkedClause += (removedLits > 0);
    numShrinkedClauseLits += removedLits;
    cl.resize(cl.size() - (i-j));

    #ifdef VERBOSE_DEBUG
    std::cout << "c Removed further " << removedLits << " lits" << std::endl;
    #endif
}

/*void CommandControl::saveOTFData()
{
    assert(false && "in multi-threaded this will fail badly");
    assert(decisionLevel() == 1);

    Lit lev0Lit = trail[trail_lim[0]];
    TransCache& oTFCache = control->implCache[(~lev0Lit).toInt()];

    vector<Lit> lits;
    for (int sublevel = trail.size()-1; sublevel > (int)trail_lim[0]; sublevel--) {
        Lit lit = trail[sublevel];
        lits.push_back(lit);
    }
    oTFCache.merge(lits, false, seen);
}*/

void CommandControl::insertVarOrder(const Var x)
{
    if (!order_heap.inHeap(x)
        && control->decision_var[x]
    ) {
        order_heap.insert(x);
    }
}

bool CommandControl::VarFilter::operator()(Var v) const
{
    return (cc->value(v) == l_Undef && control->decision_var[v]);
}

uint64_t CommandControl::getNumConflicts() const
{
    return numConflicts;
}

void CommandControl::setNeedToInterrupt()
{
    needToInterrupt = true;
}

void CommandControl::printAgilityStats()
{
    if (conf.verbosity >= 3
        && numConflicts % 100 == 99
    ) {
        std::cout
        << ", confl: " << std::setw(6) << numConflicts
        << ", rest: " << std::setw(6) << numRestarts
        << ", agility : " << std::setw(6) << std::fixed << std::setprecision(2) << agility.getAgility()
        << ", agilityTooLow: " << std::setw(4) << agility.getNumTooLow()
        << ", agilityLimit : " << std::setw(6) << std::fixed << std::setprecision(3) << conf.agilityLimit << std::endl;
    }
}
