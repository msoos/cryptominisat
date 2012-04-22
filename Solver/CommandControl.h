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

#ifndef __COMMAND_CONTROL_H__
#define __COMMAND_CONTROL_H__

#include "Solver.h"
#include "SolverTypes.h"
#include "time_mem.h"
class ThreadControl;

using std::string;
using std::cout;
using std::endl;

class CommandControl : public Solver
{
    public:
        CommandControl(const SolverConf& _conf, ThreadControl* control);
        ~CommandControl();

        //////////////////////////////
        // Problem specification:
        Var newVar(bool dvar = true); // Add a new variable that can be decided on or not

        ///////////////////////////////
        // Solving:
        ///Search for a model that respects a given set of assumptions.
        lbool solve(
            const vector<Lit>& assumps
            , const uint64_t maxConfls = std::numeric_limits<uint64_t>::max()
        );

        ///Search without assumptions.
        lbool solve(
            const uint64_t maxConfls = std::numeric_limits<uint64_t>::max()
        );
        vector<lbool> solution;     ///<Filled only if solve() returned l_True
        vector<Lit>   conflict;     ///<If problem is unsatisfiable (possibly under assumptions), this vector represent the final conflict clause expressed in the assumptions.

        ///////////////////////////////
        // Stats
        void     printRestartStats();
        void     printBaseStats();
        void     printSearchStats();
        void     printClauseStats();

        void     setNeedToInterrupt();
        uint32_t getSavedActivity(Var var) const;
        uint32_t getVarInc() const;

        struct Stats
        {
            Stats() :
                // Stats
                numRestarts(0)

                //Decisions
                , decisions(0)
                , decisionsAssump(0)
                , decisionsRand(0)

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

                //Time
                , cpu_time(0)

            {};

            void clear()
            {
                Stats stats;
                *this = stats;
            }

            Stats& operator+=(const Stats& other)
            {
                numRestarts += other.numRestarts;

                //Decisions
                decisions += other.decisions;
                decisionsAssump += other.decisionsAssump;
                decisionsRand += other.decisionsRand;

                //Conflict minimisation stats
                numLitsLearntNonMinimised += other.numLitsLearntNonMinimised;
                numLitsLearntMinimised    += other.numLitsLearntMinimised;
                furtherClMinim            += other.furtherClMinim;
                numShrinkedClause         += other.numShrinkedClause;
                numShrinkedClauseLits     += other.numShrinkedClauseLits;

                //Learnt stats
                learntUnits += other.learntUnits;
                learntBins += other.learntBins;
                learntTris += other.learntTris;
                learntLongs += other.learntLongs;

                //Stat structs
                conflStats += other.conflStats;
                propStats += other.propStats;

                //Time
                cpu_time += other.cpu_time;

                return *this;
            }

            void print()
            {
                uint64_t mem_used = memUsed();

                //Restarts stats
                printStatsLine("c restarts", numRestarts);
                printStatsLine("c time", cpu_time);

                conflStats.print(cpu_time);

                /*assert(numConflicts
                    == conflsBin + conflsTri + conflsLongIrred + conflsLongRed);*/

                cout << "c LEARNT stats" << endl;
                printStatsLine("c units learnt"
                                , learntUnits
                                , (double)learntUnits/(double)conflStats.numConflicts*100.0
                                , "% of conflicts");

                printStatsLine("c bins learnt"
                                , learntBins
                                , (double)learntBins/(double)conflStats.numConflicts*100.0
                                , "% of conflicts");

                printStatsLine("c tris learnt"
                                , learntTris
                                , (double)learntTris/(double)conflStats.numConflicts*100.0
                                , "% of conflicts");

                printStatsLine("c long learnt"
                                , learntLongs
                                , (double)learntLongs/(double)conflStats.numConflicts*100.0
                                , "% of conflicts");

                //Clause-shrinking through watchlists
                cout << "c SHRINKING stats" << endl;
                printStatsLine("c OTF cl watch-shrink"
                                , numShrinkedClause
                                , (double)numShrinkedClause/(double)conflStats.numConflicts
                                , "clauses/conflict");

                printStatsLine("c OTF cl watch-sh-lit"
                                , numShrinkedClauseLits
                                , (double)numShrinkedClauseLits/(double)numShrinkedClause
                                , "lits/clause");

                printStatsLine("c tried to recurMin cls"
                                , furtherClMinim
                                , (double)furtherClMinim/(double)conflStats.numConflicts*100.0
                                , "% of conflicts");

                printStatsLine("c decisions", decisions
                    , (double)decisionsRand*100.0/(double)decisions
                    , "% random"
                );

                //Props
                propStats.print(cpu_time);

                uint64_t totalProps = propStats.propsUnit
                 + propStats.propsBinRed
                 + propStats.propsBinIrred
                 + propStats.propsTri + propStats.propsLongIrred
                 + propStats.propsLongRed
                 + decisions + decisionsAssump;

                cout
                << "c DEBUG"
                << " ((int64_t)propStats.propagations-(int64_t)totalProps): "
                << ((int64_t)propStats.propagations-(int64_t)totalProps)
                << endl;
                //assert(propStats.propagations == totalProps);

                printStatsLine("c confl lits nonmin ", numLitsLearntNonMinimised);
                printStatsLine("c confl lits minim", numLitsLearntMinimised
                    , (double)(numLitsLearntNonMinimised - numLitsLearntMinimised)*100.0/ (double)numLitsLearntNonMinimised
                    , "% smaller"
                );

                //General stats
                printStatsLine("c Memory used", (double)mem_used / 1048576.0, " MB");
                #if !defined(_MSC_VER) && defined(RUSAGE_THREAD)
                printStatsLine("c single-thread CPU time", cpu_time, " s");
                #else
                printStatsLine("c all-threads sum CPU time", cpu_time, " s");
                #endif
            }

            uint64_t  numRestarts;      ///<Num restarts

            //Decisions
            uint64_t  decisions;        ///<Number of decisions made
            uint64_t  decisionsAssump;
            uint64_t  decisionsRand;    ///<Numer of random decisions made

            uint64_t  numLitsLearntNonMinimised;     ///<Number of learnt literals without minimisation
            uint64_t  numLitsLearntMinimised;     ///<Number of learnt literals with minimisation
            uint64_t  nShrinkedCl;      ///<Num clauses improved using on-the-fly self-subsuming resolution
            uint64_t  nShrinkedClLits;  ///<Num literals removed by on-the-fly self-subsuming resolution
            uint64_t  furtherClMinim;  ///<Decided to carry out transitive on-the-fly self-subsuming resolution on this many clauses
            uint64_t  numShrinkedClause; ///<Number of times we tried to further shrink clauses with cache
            uint64_t  numShrinkedClauseLits; ///<Number or literals removed while shinking clauses with cache

            //Learnt stats
            uint64_t learntUnits;
            uint64_t learntBins;
            uint64_t learntTris;
            uint64_t learntLongs;

            //Stat structs
            ConflStats conflStats;
            PropStats propStats;

            //Time
            double cpu_time;
        };

    protected:
        friend class CalcDefPolars;

        //For connection with ThreadControl
        void  resetStats();
        void  addInPartialSolvingStat();

        //Stats for clean
        size_t lastCleanZeroDepthAssigns;

        //History statistics
        bqueue<uint32_t> branchDepthHist;   ///< Avg branch depth in current restart
        bqueue<uint32_t> branchDepthDeltaHist;
        bqueue<uint32_t> trailDepthHist;
        bqueue<uint32_t> trailDepthDeltaHist;
        bqueue<uint32_t> glueHist;      ///< Set of last decision levels in (glue of) conflict clauses
        bqueue<uint32_t> conflSizeHist;    ///< Conflict size history
        bqueue<double, double>  agilityHist;
        uint64_t sumConflicts() const;

        /////////////////
        //Settings
        ThreadControl*   control;          ///< Thread control class
        MTRand           mtrand;           ///< random number generator
        SolverConf       conf;             ///< Solver config for this thread
        bool             needToInterrupt;  ///<If set to TRUE, interrupt cleanly ASAP

        //Stats printing
        void printAgilityStats();

        /////////////////
        // Searching
        /// Search for a given number of conflicts.
        lbool search(
            const SearchFuncParams _params
            , uint64_t& rest
        );
        lbool burstSearch();
        bool  handle_conflict(SearchFuncParams& params, PropBy confl);// Handles the conflict clause
        lbool new_decision();  // Handles the case when decision must be made
        void  checkNeedRestart(SearchFuncParams& params, uint64_t& rest);     // Helper function to decide if we need to restart during search
        Lit   pickBranchLit();                             // Return the next decision variable.

        ///////////////
        // Conflicting
        void     cancelUntil      (uint32_t level);                        ///<Backtrack until a certain level.
        void     analyze          (PropBy confl, vector<Lit>& out_learnt, uint32_t& out_btlevel, uint32_t &nblevels);
        void     analyzeHelper    (
            Lit lit
            , int& pathC
            , vector<Lit>& out_learnt
            , bool var_bump_necessary
        );
        void     analyzeFinal     (const Lit p, vector<Lit>& out_conflict);

        //////////////
        // Conflict minimisation
        void            prune_removable(vector<Lit>& out_learnt);
        void            find_removable(const vector<Lit>& out_learnt, uint32_t abstract_level);
        int             quick_keeper(Lit p, uint32_t abstract_level, bool maykeep);
        int             dfs_removable(Lit p, uint32_t abstract_level);
        void            mark_needed_removable(Lit p);
        int             res_removable();
        uint32_t        abstractLevel(const Var x) const;
        vector<PropBy> trace_reasons; // clauses to resolve to give CC
        vector<Lit>     trace_lits_minim; // lits maybe used in minimization


        /////////////////
        //Graphical conflict generation
        void         genConfGraph     (PropBy conflPart);
        string simplAnalyseGraph (PropBy conflHalf, vector<Lit>& out_learnt, uint32_t& out_btlevel, uint32_t &glue);

        /////////////////
        // Variable activity
        vector<uint32_t> activities;
        void     varDecayActivity ();      ///<Decay all variables with the specified factor. Implemented by increasing the 'bump' value instead.
        void     varBumpActivity  (Var v); ///<Increase a variable with the current 'bump' value.
        bool     getPolarity(const Var var);
        struct VarOrderLt { ///Order variables according to their activities
            const vector<uint32_t>&  activities;
            bool operator () (const uint32_t x, const uint32_t y) const
            {
                return activities[x] > activities[y];
            }

            VarOrderLt(const vector<uint32_t>& _activities) :
                activities(_activities)
            {}
        };

        struct VarFilter { ///Filter out vars that have been set or is not decision from heap
            const CommandControl* cc;
            const ThreadControl* control;
            VarFilter(const CommandControl* _cc, ThreadControl* _control) :
                cc(_cc)
                ,control(_control)
            {}
            bool operator()(uint32_t var) const;
        };
        Heap<VarOrderLt>  order_heap;                   ///< activity-ordered heap of decision variables
        uint32_t var_inc;
        void              insertVarOrder(const Var x);  ///< Insert a variable in heap
        void  genRandomVarActMultDiv();

        ////////////
        // Transitive on-the-fly self-subsuming resolution
        void   minimiseLearntFurther(vector<Lit>& cl);
        const Stats& getStats() const;

    private:
        double    startTime; ///<When solve() was started
        Stats stats;
        size_t origTrailSize;
        uint32_t var_inc_multiplier;
        uint32_t var_inc_divider;
};

inline void CommandControl::varDecayActivity()
{
    var_inc *= var_inc_multiplier;
    var_inc /= var_inc_divider;
}
inline void CommandControl::varBumpActivity(Var var)
{
    if ( (activities[var] += var_inc) > (0x1U) << 24 ) {
        // Rescale:
        for (vector<uint32_t>::iterator
            it = activities.begin()
            , end = activities.end()
            ; it != end
            ; it++
        ) {
            *it >>= 14;
        }

        //Reset var_inc
        var_inc >>= 14;

        //If var_inc is smaller than var_inc_start then this MUST be corrected
        //otherwise the 'varDecayActivity' may not decay anything in fact
        if (var_inc < conf.var_inc_start)
            var_inc = conf.var_inc_start;
    }

    // Update order_heap with respect to new activity:
    if (order_heap.inHeap(var))
        order_heap.decrease(var);
}

inline uint32_t CommandControl::abstractLevel(const Var x) const
{
    return ((uint32_t)1) << (varData[x].level % 32);
}

inline bool CommandControl::getPolarity(const Var var)
{
    switch(conf.polarity_mode) {
        case polarity_false:
            return false;

        case polarity_true:
            return true;

        case polarity_rnd:
            return mtrand.randInt(1);

        case polarity_auto:
            return varData[var].polarity
                ^ (mtrand.randInt(conf.flipPolarFreq*branchDepthDeltaHist.getAvgLong()) == 1);
        default:
            assert(false);
    }

    return true;
}

inline lbool CommandControl::solve(const uint64_t maxConfls)
{
    vector<Lit> tmp;
    return solve(tmp, maxConfls);
}

inline uint32_t CommandControl::getSavedActivity(Var var) const
{
    return activities[var];
}

inline uint32_t CommandControl::getVarInc() const
{
    return var_inc;
}

inline const CommandControl::Stats& CommandControl::getStats() const
{
    return stats;
}

inline void CommandControl::addInPartialSolvingStat()
{
    stats.cpu_time = cpuTime() - startTime;
    stats.propStats = propStats;
}

#endif //__COMMAND_CONTROL_H__
