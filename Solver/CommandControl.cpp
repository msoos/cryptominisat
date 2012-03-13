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
#include <iomanip>
#include <omp.h>
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
        Solver(_control->clAllocator, AgilityData(_conf.agilityG, _conf.agilityLimit))

        // Stats
        , numConflicts(0)
        , numRestarts(0)
        , decisions(0)
        , assumption_decisions(0)
        , decisions_rnd(0)

        //Conflict generation
        , numLitsLearntNonMinimised(0)
        , numLitsLearntMinimised(0)
        , furtherClMinim(0)
        , numShrinkedClause(0)
        , numShrinkedClauseLits(0)

        //Learnt stats
        , learntUnits(0)
        , learntBins(0)
        , learntTris(0)
        , learntLongs(0)

        //Conlf stats
        , conflsBinIrred(0)
        , conflsBinRed(0)
        , conflsTri(0)
        , conflsLongIrred(0)
        , conflsLongRed(0)

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

template<class T, class T2>
void CommandControl::printStatsLine(std::string left, T value, T2 value2, std::string extra)
{
    cout
    << std::fixed << std::left << std::setw(27) << left
    << ": " << std::setw(11) << std::setprecision(2) << value
    << " (" << std::left << std::setw(9) << std::setprecision(2) << value2
    << " " << extra << ")"
    << std::right
    << endl;
}

template<class T>
void CommandControl::printStatsLine(std::string left, T value, std::string extra)
{
    cout
    << std::fixed << std::left << std::setw(27) << left
    << ": " << std::setw(11) << std::setprecision(2)
    << value << extra
    << std::right
    << endl;
}

void CommandControl::printStats()
{
    double   cpu_time = cpuTime() - startTime;
    uint64_t mem_used = memUsed();

    //Restarts stats
    printStatsLine("c restarts", numRestarts);

    //Search stats
    cout << "c CONFLS stats" << endl;
    printStatsLine("c conflicts", numConflicts
        , (double)numConflicts/cpu_time
        , "/ sec"
    );

    printStatsLine("c conflsBinIrred", conflsBinIrred
        , 100.0*(double)conflsBinIrred/(double)numConflicts
        , "%"
    );

    printStatsLine("c conflsBinRed", conflsBinRed
        , 100.0*(double)conflsBinRed/(double)numConflicts
        , "%"
    );

    printStatsLine("c conflsTri", conflsTri
        , 100.0*(double)conflsTri/(double)numConflicts
        , "%"
    );

    printStatsLine("c conflsLongIrred" , conflsLongIrred
        , 100.0*(double)conflsLongIrred/(double)numConflicts
        , "%"
    );

    printStatsLine("c conflsLongRed", conflsLongRed
        , 100.0*(double)conflsLongRed/(double)numConflicts
        , "%"
    );

    cout << "c numConflicts: " << numConflicts << endl;
    cout
    << "c conflsBin + conflsTri + conflsLongIrred + conflsLongRed : "
    << (conflsBinIrred + conflsBinRed +  conflsTri + conflsLongIrred + conflsLongRed)
    << endl;
    cout
    << "c DIFF: "
    << ((int)numConflicts - (int)(conflsBinIrred + conflsBinRed + conflsTri + conflsLongIrred + conflsLongRed))
    << endl;

    /*assert(numConflicts
        == conflsBin + conflsTri + conflsLongIrred + conflsLongRed);*/

    cout << "c LEARNT stats" << endl;
    printStatsLine("c units learnt"
                    , learntUnits
                    , (double)learntUnits/(double)numConflicts*100.0
                    , "% of conflicts");

    printStatsLine("c bins learnt"
                    , learntBins
                    , (double)learntBins/(double)numConflicts*100.0
                    , "% of conflicts");

    printStatsLine("c tris learnt"
                    , learntTris
                    , (double)learntTris/(double)numConflicts*100.0
                    , "% of conflicts");

    printStatsLine("c long learnt"
                    , learntLongs
                    , (double)learntLongs/(double)numConflicts*100.0
                    , "% of conflicts");

    //Clause-shrinking through watchlists
    cout << "c SHRINKING stats" << endl;
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

    //Props
    cout << "c PROPS stats" << endl;
    printStatsLine("c Mbogo-props", bogoProps/(1000*1000)
        , (double)bogoProps/(cpu_time*1000*1000)
        , "/ sec"
    );

    const uint64_t thisProps = propagations - propsOrig;
    printStatsLine("c Mprops", thisProps/(1000*1000)
        , (double)thisProps/(cpu_time*1000*1000)
        , "/ sec"
    );

    printStatsLine("c decisions", decisions
        , (double)decisions_rnd*100.0/(double)decisions
        , "% random"
    );

    const uint64_t thisPropsBin = propsBin - propsBinOrig;
    const uint64_t thisPropsTri = propsTri - propsTriOrig;
    const uint64_t thisPropsLongIrred = propsLongIrred - propsLongIrredOrig;
    const uint64_t thisPropsLongRed = propsLongRed - propsLongRedOrig;

    printStatsLine("c propsBin", thisPropsBin
        , 100.0*(double)thisPropsBin/(double)thisProps
        , "% of propagations"
    );

    printStatsLine("c propsTri", thisPropsTri
        , 100.0*(double)thisPropsTri/(double)thisProps
        , "% of propagations"
    );

    printStatsLine("c propsLongIrred", thisPropsLongIrred
        , 100.0*(double)thisPropsLongIrred/(double)thisProps
        , "% of propagations"
    );

    printStatsLine("c propsLongRed", thisPropsLongRed
        , 100.0*(double)thisPropsLongRed/(double)thisProps
        , "% of propagations"
    );

    uint64_t totalProps =
    thisPropsBin + thisPropsTri + thisPropsLongIrred + thisPropsLongRed
     + decisions + assumption_decisions
     + numConflicts;

    cout
    << "c totprops: "
    << totalProps
    << " missing: "
    << ((int64_t)thisProps-(int64_t)totalProps)
    << endl;
    //assert(propagations == totalProps);

    printStatsLine("c conflict literals", numLitsLearntNonMinimised
        , (double)(numLitsLearntNonMinimised - numLitsLearntMinimised)*100.0/ (double)numLitsLearntNonMinimised
        , "% deleted"
    );

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
    cout << "Canceling finished. (now at level: " << decisionLevel() << " sublevel: " << trail.size()-1 << ")" << endl;
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
void CommandControl::analyze(
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
    numLitsLearntNonMinimised += out_learnt.size();

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

    //Cache-based minimisation
    if (conf.doCache
        && conf.doMinimLearntMore
        && out_learnt.size() > 1
        && (conf.doAlwaysFMinim
            || calcNBLevels(out_learnt) < 0.65*glueHist.getAvgAll()
            || out_learnt.size() < 0.65*conflSizeHist.getAvgAll()
            || out_learnt.size() < 10
            )
    ) {
        minimiseLearntFurther(out_learnt);
    }

    //Calc stats
    glue = calcNBLevels(out_learnt);
    numLitsLearntMinimised += out_learnt.size();

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

void CommandControl::updateConflStats()
{
    switch(lastConflictCausedBy) {
        case CONFL_BY_BIN_IRRED_CLAUSE :
            conflsBinIrred++;
            break;
        case CONFL_BY_BIN_RED_CLAUSE :
            conflsBinRed++;
            break;
        case CONFL_BY_TRI_CLAUSE :
            conflsTri++;
            break;
        case CONFL_BY_LONG_IRRED_CLAUSE :
            conflsLongIrred++;
            break;
        case CONFL_BY_LONG_RED_CLAUSE :
            conflsLongRed++;
            break;
        default:
            assert(false);
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
    cout << "c started CommandControl::search()" << endl;
    #endif //VERBOSE_DEBUG

    //Loop until restart or finish (SAT/UNSAT)
    while (true) {
        assert(ok);
        const PropBy confl = propagate();

        #ifdef VERBOSE_DEBUG
        cout << "c CommandControl::search() has finished propagation" << endl;
        #endif //VERBOSE_DEBUG

        if (!confl.isNULL()) {
            //Update conflict stats based on lastConflictCausedBy
            updateConflStats();

            //If restart is needed, set it as so
            checkNeedRestart(params, rest);

            if (!handle_conflict(params, confl))
                return l_False;

        } else {
            assert(ok);
            //TODO Enable this through some ingenious locking
            /*if (conf.doCache && decisionLevel() == 1)
                saveOTFData();*/

            //If restart is needed, restart here
            if (params.needToStopSearch
                || control->getSumConflicts() > control->getNextCleanLimit()
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
            assumption_decisions++;
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
            cout << "c needToInterrupt is set, restartig as soon as possible!" << endl;
        params.needToStopSearch = true;
    }

    if (     (conf.restartType == geom_restart
            && params.conflictsDoneThisRestart > rest
        ) || (conf.restartType == glue_restart
            && glueHist.isvalid()
            && 0.95*glueHist.getAvg() > glueHist.getAvgAll()
        ) || (conf.restartType == agility_restart
            && agilityHist.isvalid()
            && agilityHist.getAvg() < conf.agilityLimit
        ) || (conf.restartType == branch_depth_delta_restart
            && branchDepthDeltaHist.isvalid()
            && 0.95*branchDepthDeltaHist.getAvg() > branchDepthDeltaHist.getAvgAll()
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
            printAgilityStats();

            if (conf.verbosity >= 3)
                cout << "c Agility was too low, restarting as soon as possible!" << endl;
            params.needToStopSearch = true;
        } else {
            rest *= conf.restart_inc;
        }
    }

    if (params.conflictsDoneThisRestart > params.conflictsToDo) {
        if (conf.verbosity >= 3)
            cout << "c Over limit of conflicts for this restart, restarting as soon as possible!" << endl;
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
    cout << "Learning:" << learnt_clause << endl;
    cout << "reverting var " << learnt_clause[0].var()+1 << " to " << !learnt_clause[0].sign() << endl;
    #endif
    assert(value(learnt_clause[0]) == l_Undef);

    //Set up everything to get the clause
    std::sort(learnt_clause.begin()+1, learnt_clause.end(), PolaritySorter(varData));
    glue = std::min(glue, MAX_THEORETICAL_GLUE);
    Clause *cl;

    //Get new clause
    cl = control->newClauseByThread(learnt_clause, glue);

    //Attach new clause
    switch (learnt_clause.size()) {
        case 1:
            //Unitary learnt
            learntUnits++;
            enqueue(learnt_clause[0]);
            assert(backtrack_level == 0 && "Unit clause learnt, so must cancel until level 0, right?");

            break;
        case 2:
            //Binary learnt
            learntBins++;
            attachBinClause(learnt_clause[0], learnt_clause[1], true);
            enqueue(learnt_clause[0], PropBy(learnt_clause[1]));
            break;

        case 3:
            //3-long almost-normal learnt
            learntTris++;
            attachClause(*cl);
            enqueue(learnt_clause[0], PropBy(learnt_clause[1], learnt_clause[2]));
            break;

        default:
            //Normal learnt
            learntLongs++;
            attachClause(*cl);
            enqueue(learnt_clause[0], PropBy(clAllocator->getOffset(cl)));
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

    //Confl stats
    numConflicts = 0;
    conflsLongRed = 0;
    conflsBinIrred = 0;
    conflsBinRed = 0;
    conflsTri = 0;
    conflsLongIrred = 0;

    //Restarts
    numRestarts = 0;

    //Decisions
    decisions = 0;
    assumption_decisions = 0;
    decisions_rnd = 0;

    //Conflict minimisation stats
    numLitsLearntNonMinimised = 0;
    numLitsLearntMinimised = 0;
    furtherClMinim = 0;
    numShrinkedClause = 0;
    numShrinkedClauseLits = 0;

    //Learnt stats
    learntUnits = 0;
    learntBins = 0;
    learntTris = 0;
    learntLongs = 0;

    //Props stats
    propsOrig = propagations;
    propsBinOrig = propsBin;
    propsTriOrig = propsTri;
    propsLongIrredOrig = propsLongIrred;
    propsLongRedOrig = propsLongRed;

    //Set already set vars
    origTrailSize = trail.size();

    order_heap.filter(VarFilter(this, control));
}

lbool CommandControl::burstSearch()
{
    //Print what we will be doing
    if (conf.verbosity >= 2) {
        cout
        << "Doing bust search for " << conf.burstSearchLen << " conflicts"
        << endl;
    }

    //Save old config
    const double backup_rand = conf.random_var_freq;
    const RestType backup_restType = conf.restartType;
    const int backup_polar_mode = conf.polarity_mode;
    uint32_t backup_var_inc_divider = var_inc_divider;
    uint32_t backup_var_inc_multiplier = var_inc_multiplier;

    //Set burst config
    conf.random_var_freq = 1;
    conf.polarity_mode = polarity_rnd;
    uint64_t rest_burst = conf.burstSearchLen;
    var_inc_divider = 1;
    var_inc_multiplier = 1;

    //Do burst
    lbool status = search(SearchFuncParams(rest_burst), rest_burst);

    //Restore config
    conf.random_var_freq = backup_rand;
    conf.restartType = backup_restType;
    conf.polarity_mode = backup_polar_mode;
    var_inc_divider = backup_var_inc_divider;
    var_inc_multiplier = backup_var_inc_multiplier;

    //Print what has happened
    if (conf.verbosity >= 2) {
        printRestartStat();

        cout
        << "c Burst finished"
        << endl;
    }

    return status;
}

void CommandControl::printRestartStat()
{
    cout << "c " << omp_get_thread_num()
    << " " << std::setw(6) << numRestarts
    << " " << std::setw(7) << control->getSumConflicts()
    << " " << std::setw(7) << control->getNumFreeVarsAdv(trail.size())

    << " glue"
    << " " << std::right << glueHist.getAvgPrint(1, 5)
    << "/" << std::left << glueHist.getAvgAllPrint(1, 5)

    << " agil"
    << " " << std::right << agilityHist.getAvgPrint(3, 5)
    << "/" << std::left<< agilityHist.getAvgAllPrint(3, 5)

    << " confllen"
    << " " << std::right << conflSizeHist.getAvgPrint(1, 5)
    << "/" << std::left << conflSizeHist.getAvgAllPrint(1, 5)

    << " branchd"
    << " " << std::right << branchDepthHist.getAvgPrint(1, 5)
    << "/" << std::left  << branchDepthHist.getAvgAllPrint(1, 5)
    << " branchdd"

    << " " << std::right << branchDepthDeltaHist.getAvgPrint(1, 4)
    << "/" << std::left << branchDepthDeltaHist.getAvgAllPrint(1, 4)

    << " traild"
    << " " << std::right << trailDepthHist.getAvgPrint(0, 7)
    << "/" << std::left << trailDepthHist.getAvgAllPrint(0, 7)

    << " traildd"
    << " " << std::right << trailDepthDeltaHist.getAvgPrint(0, 5)
    << "/" << std::left << trailDepthDeltaHist.getAvgAllPrint(0, 5)
    << endl;

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
    startTime = cpuTime();
    resetStats();

    //Current solving status
    lbool status = l_Undef;

    uint64_t lastRestartPrint = numConflicts;

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
        && numConflicts < maxConfls
    ) {
        assert(numConflicts < maxConfls);

        status = search(SearchFuncParams(maxConfls-numConflicts), rest);
        rest *= conf.restart_inc;
        if (status != l_Undef)
            break;

        //Print restart stat
        if (conf.verbosity >= 1
            && (lastRestartPrint + 800) < numConflicts
        ) {
            printRestartStat();
            lastRestartPrint = numConflicts;
        }

        if (numConflicts >= maxConfls) {
            if (conf.verbosity >= 1) {
                cout
                << "c thread(maxconfl) Trail size: " << trail.size()
                << " over maxConfls"
                << endl;
            }
            break;
        }

        if (control->getSumConflicts() > control->getNextCleanLimit()) {
            if (conf.verbosity >= 1) {
                cout
                << "c th " << omp_get_thread_num() << " cleaning"
                << " getNextCleanLimit(): " << control->getNextCleanLimit()
                << " numConflicts : " << numConflicts
                << " SumConfl: " << control->getSumConflicts()
                << " maxConfls:" << maxConfls
                << " Trail size: " << trail.size() << endl;
            }
            control->fullReduce();
            control->consolidateMem();

            control->restPrinter->printRestartStat("N");
            genRandomVarActMultDiv();
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

    //#ifdef VERBOSE_DEBUG
    if (conf.verbosity >= 1) {
        cout << "c th " << omp_get_thread_num()
        << " ---------" << endl;

        cout << "c CommandControl::solve() finished"
        << " status: " << status
        << " control->getNextCleanLimit(): " << control->getNextCleanLimit()
        << " numConflicts : " << numConflicts
        << " SumConfl: " << control->getSumConflicts()
        << " maxConfls:" << maxConfls
        << endl;
        printStats();

        cout << "c th " << omp_get_thread_num()
        << " ---------" << endl;
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
            decisions_rnd++;
            next = Lit(next_var, !getPolarity(next_var));
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
        next = Lit(next_var, !getPolarity(next_var));
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
    furtherClMinim++;

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
    numShrinkedClause += (removedLits > 0);
    numShrinkedClauseLits += removedLits;
    cl.resize(cl.size() - (i-j));

    #ifdef VERBOSE_DEBUG
    cout << "c Removed further " << removedLits << " lits" << endl;
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

bool CommandControl::VarFilter::operator()(uint32_t var) const
{
    return (cc->value(var) == l_Undef && control->decision_var[var]);
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
        //&& numConflicts % 100 == 99
    ) {
        cout
        << ", confl: " << std::setw(6) << numConflicts
        << ", rest: " << std::setw(6) << numRestarts
        << ", agility : " << std::setw(6) << std::fixed << std::setprecision(2) << agility.getAgility()
        << ", agilityLimit : " << std::setw(6) << std::fixed << std::setprecision(2) << conf.agilityLimit
        << ", agilityHist: " << std::setw(6) << std::fixed << std::setprecision(3) << agilityHist.getAvg()
        << ", agilityHistAll: " << std::setw(6) << std::fixed << std::setprecision(3) << agilityHist.getAvgAll()
        << endl;
    }
}
