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
        void           printStats();
        uint64_t getNumConflicts() const;
        void           setNeedToInterrupt();
        bool           getSavedPolarity(Var var) const;

    protected:
        friend class CalcDefPolars;

        //For connection with ThreadControl
        void      initialiseSolver();
        void      addToThreads(const size_t oldTrailSize);
        uint64_t  lastSumConfl;     ///<Sum conflicts last time told by ThreadControl
        vector<Clause*> longToAdd;
        vector<BinaryClause> binToAdd;
        vector<Lit> unitToAdd;
        size_t    lastLong;
        size_t    lastBin;
        size_t    lastUnit;
        bool      handleNewBin(const BinaryClause& binCl);
        bool      handleNewLong(const Clause& cl);
        void      syncFromThreadControl();
        bool      addOtherClauses();

        // Statistics
        //
        double           startTime;        ///<When solve() was started
        uint64_t         numConflicts;     ///<Number of conflicts
        uint64_t         numRestarts;      ///<Num restarts
        uint64_t         decisions;        ///<Number of decisions made
        uint64_t         decisions_rnd;    ///<Numer of random decisions made
        uint64_t         max_literals;     ///<Number of learnt literals without minimisation
        uint64_t         tot_literals;     ///<Number of learnt literals with minimisation
        uint64_t         nShrinkedCl;      ///<Num clauses improved using on-the-fly self-subsuming resolution
        uint64_t         nShrinkedClLits;  ///<Num literals removed by on-the-fly self-subsuming resolution
        uint64_t         furtherClMinim;  ///<Decided to carry out transitive on-the-fly self-subsuming resolution on this many clauses
        uint64_t         numShrinkedClause; ///<Number of times we tried to further shrink clauses with cache
        uint64_t         numShrinkedClauseLits; ///<Number or literals removed while shinking clauses with cache

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
        void     varDecayActivity ();      ///<Decay all variables with the specified factor. Implemented by increasing the 'bump' value instead.
        void     varBumpActivity  (Var v); ///<Increase a variable with the current 'bump' value.
        void     setDecisionVar(const Var v, const bool b);
        bool     getPolarity(const Var var);
        struct VarOrderLt { ///Order variables according to their activities
            const vector<VarData>&  varData;
            bool operator () (const Var x, const Var y) const {
                return varData[x].activity > varData[y].activity;
            }

            VarOrderLt(const vector<VarData>& _varData) : varData(_varData) { }
        };
        struct VarFilter { ///Filter out vars that have been set or is not decision from heap
            const CommandControl* cc;
            const ThreadControl* control;
            VarFilter(const CommandControl* _cc, ThreadControl* _control) :
                cc(_cc)
                ,control(_control)
            {}
            bool operator()(Var v) const;
        };
        Heap<VarOrderLt>  order_heap;                   ///< activity-ordered heap of decision variables
        void              insertVarOrder(const Var x);  ///< Insert a variable in heap

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
    conf.var_inc *= 11;
    conf.var_inc /= 10;
}
inline void CommandControl::varBumpActivity(Var v)
{
    if ( (varData[v].activity += conf.var_inc) > (0x1U) << 24 ) {
        // Rescale:
        for (Var var = 0; var != nVars(); var++) {
            varData[var].activity >>= 14;
        }
        conf.var_inc = 128;;
    }

    // Update order_heap with respect to new activity:
    if (order_heap.inHeap(v))
        order_heap.decrease(v);
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
            if (avgBranchDepth.isvalid()) {
                return varData[var].polarity ^ (mtrand.randInt(3*avgBranchDepth.getAvgUInt()) == 1);
            } else {
                return varData[var].polarity;
            }
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

#endif //__COMMAND_CONTROL_H__
