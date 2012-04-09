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
#include "time_mem.h"
class ThreadControl;

using std::string;
using std::cout;
using std::endl;

struct SolvingStats
{
    SolvingStats() :
        // Stats
        numConflicts(0)
        , numRestarts(0)

        //Decisions
        , decisions(0)
        , decisionsAssump(0)
        , decisionsRand(0)

        //To correctly count propagations
        , probe(0)
        , vivify(0)

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
    {};

    SolvingStats& operator+=(const SolvingStats& other)
    {
        numConflicts += other.numConflicts;
        numRestarts += other.numRestarts;

        //Decisions
        decisions += other.decisions;
        decisionsAssump += other.decisionsAssump;
        decisionsRand += other.decisionsRand;

        //To correctly count propagations
        probe += other.probe;
        vivify += other.vivify;

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

        conflsBinIrred += other.conflsBinIrred;
        conflsBinRed += other.conflsBinRed;
        conflsTri += other.conflsTri;
        conflsLongIrred += other.conflsLongIrred;
        conflsLongRed += other.conflsLongRed;

        return *this;
    }

    template<class T, class T2> void printStatsLine(string left, T value, T2 value2, string extra)
    {
        cout
        << std::fixed << std::left << std::setw(27) << left
        << ": " << std::setw(11) << std::setprecision(2) << value
        << " (" << std::left << std::setw(9) << std::setprecision(2) << value2
        << " " << extra << ")"
        << std::right
        << endl;
    }

    template<class T> void printStatsLine(string left, T value, string extra = "")
    {
        cout
        << std::fixed << std::left << std::setw(27) << left
        << ": " << std::setw(11) << std::setprecision(2)
        << value << extra
        << std::right
        << endl;
    }

    void printSolvingStats(double cpu_time, PropStats propStats)
    {
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

        /*cout << "c numConflicts: " << numConflicts << endl;
        cout
        << "c conflsBin + conflsTri + conflsLongIrred + conflsLongRed : "
        << (conflsBinIrred + conflsBinRed +  conflsTri + conflsLongIrred + conflsLongRed)
        << endl;
        cout
        << "c DIFF: "
        << ((int)numConflicts - (int)(conflsBinIrred + conflsBinRed + conflsTri + conflsLongIrred + conflsLongRed))
        << endl;*/
        assert(((int)numConflicts -
            (int)(conflsBinIrred + conflsBinRed
                    + conflsTri + conflsLongIrred + conflsLongRed)
        ) == 0);

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
        printStatsLine("c Mbogo-props", propStats.bogoProps/(1000*1000)
            , (double)propStats.bogoProps/(cpu_time*1000*1000)
            , "/ sec"
        );

        printStatsLine("c Mprops", propStats.propagations/(1000*1000)
            , (double)propStats.propagations/(cpu_time*1000*1000)
            , "/ sec"
        );

        printStatsLine("c decisions", decisions
            , (double)decisionsRand*100.0/(double)decisions
            , "% random"
        );

        printStatsLine("c propsUnit", propStats.propsUnit
            , 100.0*(double)propStats.propsUnit/(double)propStats.propagations
            , "% of propagations"
        );

        printStatsLine("c propsBinIrred", propStats.propsBinIrred
            , 100.0*(double)propStats.propsBinIrred/(double)propStats.propagations
            , "% of propagations"
        );

        printStatsLine("c propsBinRed", propStats.propsBinRed
            , 100.0*(double)propStats.propsBinRed/(double)propStats.propagations
            , "% of propagations"
        );

        printStatsLine("c propsTri", propStats.propsTri
            , 100.0*(double)propStats.propsTri/(double)propStats.propagations
            , "% of propagations"
        );

        printStatsLine("c propsLongIrred", propStats.propsLongIrred
            , 100.0*(double)propStats.propsLongIrred/(double)propStats.propagations
            , "% of propagations"
        );

        printStatsLine("c propsLongRed", propStats.propsLongRed
            , 100.0*(double)propStats.propsLongRed/(double)propStats.propagations
            , "% of propagations"
        );

        uint64_t totalProps = propStats.propsUnit
         + propStats.propsBinRed
         + propStats.propsBinIrred
         + propStats.propsTri + propStats.propsLongIrred
         + propStats.propsLongRed
         + probe + vivify
         + decisions + decisionsAssump;

        /*cout
        << "c totprops: "
        << totalProps
        << " missing: "
        << ((int64_t)propStats.propagations-(int64_t)totalProps)
        << endl;*/
        assert(propStats.propagations == totalProps);

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

    uint64_t  numConflicts;     ///<Number of conflicts
    uint64_t  numRestarts;      ///<Num restarts

    //Decisions
    uint64_t  decisions;        ///<Number of decisions made
    uint64_t  decisionsAssump;
    uint64_t  decisionsRand;    ///<Numer of random decisions made

    //To correctly count propagations
    uint64_t probe; //Proping tries
    uint64_t vivify; //Vivifying tries

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

    //Stats for conflicts
    uint64_t conflsBinIrred;
    uint64_t conflsBinRed;
    uint64_t conflsTri;
    uint64_t conflsLongIrred;
    uint64_t conflsLongRed;
};

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

    protected:
        friend class CalcDefPolars;

        //For connection with ThreadControl
        void  resetStats();

        // Statistics
        //
        void updateConflStats(); //Based on lastConflictCausedBy, update confl stats

        //Props stats
        uint64_t propsOrig;
        uint64_t propsBinOrig;
        uint64_t propsTriOrig;
        uint64_t propsLongIrredOrig;
        uint64_t propsLongRedOrig;

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
        const SolvingStats& getStats() const;

    private:
        double    startTime; ///<When solve() was started
        SolvingStats stats;
        PropStats oldPropStats;
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

inline const SolvingStats& CommandControl::getStats() const
{
    return stats;
}

#endif //__COMMAND_CONTROL_H__
