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
class ThreadControl;

using std::string;

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
        void     printStats();
        void     printRestartStat();
        uint64_t getNumConflicts() const;
        void     setNeedToInterrupt();
        bool     getSavedPolarity(Var var) const;
        uint32_t getSavedActivity(Var var) const;
        uint32_t getVarInc() const;

    protected:
        friend class CalcDefPolars;

        //For connection with ThreadControl
        void  resetStats();

        // Statistics
        //
        double           startTime;        ///<When solve() was started
        uint64_t         numConflicts;     ///<Number of conflicts
        uint64_t         numRestarts;      ///<Num restarts
        uint64_t         decisions;        ///<Number of decisions made
        uint64_t         assumption_decisions;
        uint64_t         decisions_rnd;    ///<Numer of random decisions made
        uint64_t         numLitsLearntNonMinimised;     ///<Number of learnt literals without minimisation
        uint64_t         numLitsLearntMinimised;     ///<Number of learnt literals with minimisation
        uint64_t         nShrinkedCl;      ///<Num clauses improved using on-the-fly self-subsuming resolution
        uint64_t         nShrinkedClLits;  ///<Num literals removed by on-the-fly self-subsuming resolution
        uint64_t         furtherClMinim;  ///<Decided to carry out transitive on-the-fly self-subsuming resolution on this many clauses
        uint64_t         numShrinkedClause; ///<Number of times we tried to further shrink clauses with cache
        uint64_t         numShrinkedClauseLits; ///<Number or literals removed while shinking clauses with cache

        //Learnt stats
        uint64_t learntUnits;
        uint64_t learntBins;
        uint64_t learntTris;
        uint64_t learntLongs;
        size_t origTrailSize;

        //Stats for conflicts
        uint64_t conflsBinIrred;
        uint64_t conflsBinRed;
        uint64_t conflsTri;
        uint64_t conflsLongIrred;
        uint64_t conflsLongRed;
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

        /////////////////
        //Settings
        ThreadControl*   control;          ///< Thread control class
        MTRand           mtrand;           ///< random number generator
        SolverConf       conf;             ///< Solver config for this thread
        bool             needToInterrupt;  ///<If set to TRUE, interrupt cleanly ASAP

        //Stats printing
        template<class T, class T2> void printStatsLine(string left, T value, T2 value2, string extra);
        template<class T> void printStatsLine(string left, T value, string extra = "");
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
        template<class T> uint32_t calcNBLevels(const T& ps); ///<Calculates the glue of a clause

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
        uint32_t var_inc_multiplier;
        uint32_t var_inc_divider;

        ///////////
        // Learnt clause removal
        bool    locked(const Clause& c) const; // Returns TRUE if a clause is a reason for some implication in the current state.

        ////////////
        // Transitive on-the-fly self-subsuming resolution
        void   minimiseLearntFurther(vector<Lit>& cl);
};

/**
@brief Calculates the glue of a clause

Used to calculate the Glue of a new clause, or to update the glue of an
existing clause. Only used if the glue-based activity heuristic is enabled,
i.e. if we are in GLUCOSE mode (not MiniSat mode)
*/
template<class T>
inline uint32_t CommandControl::calcNBLevels(const T& ps)
{
    uint32_t nbLevels = 0;
    typename T::const_iterator l, end;

    for(l = ps.begin(), end = ps.end(); l != end; l++) {
        int32_t lev = varData[l->var()].level;
        if (!seen2[lev]) {
            nbLevels++;
            seen2[lev] = 1;
        }
    }

    for(l = ps.begin(), end = ps.end(); l != end; l++) {
        int32_t lev = varData[l->var()].level;
        seen2[lev] = 0;
    }
    return nbLevels;
}

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
        var_inc = conf.var_inc_start;
    }

    // Update order_heap with respect to new activity:
    if (order_heap.inHeap(var))
        order_heap.decrease(var);
}

inline bool CommandControl::locked(const Clause& c) const
{
    if (c.size() <= 3) return true; //we don't know in this case :I
    const ClauseData& data = clauseData[c.getNum()];
    const PropBy from1(varData[c[data[0]].var()].reason);
    const PropBy from2(varData[c[data[1]].var()].reason);

    if (from1.isClause()
        && !from1.isNULL()
        && from1.getWatchNum() == 0
        && from1.getClause() == clAllocator->getOffset(&c)
        && value(c[data[0]]) == l_True
    ) return true;

    if (from2.isClause()
        && !from2.isNULL()
        && from2.getWatchNum() == 1
        && from2.getClause() == clAllocator->getOffset(&c)
        && value(c[data[1]]) == l_True
        ) return true;

    return false;
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
                ^ (mtrand.randInt(conf.flipPolarFreq*branchDepthDeltaHist.getAvgAll()) == 1);
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

inline bool CommandControl::getSavedPolarity(const Var var) const
{
    return varData[var].polarity;
}

inline uint32_t CommandControl::getSavedActivity(Var var) const
{
    return activities[var];
}

inline uint32_t CommandControl::getVarInc() const
{
    return var_inc;
}


#endif //__COMMAND_CONTROL_H__
