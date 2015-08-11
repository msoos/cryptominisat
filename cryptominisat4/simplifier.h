/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.0 of the License.
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

#ifndef SIMPLIFIER_H
#define SIMPLIFIER_H


#include <map>
#include <vector>
#include <list>
#include <set>
#include <queue>
#include <set>
#include <iomanip>
#include <fstream>

#include "clause.h"
#include "solvertypes.h"
#include "heap.h"
#include "touchlist.h"
#include "varupdatehelper.h"
#include "watched.h"
#include "watcharray.h"

namespace CMSat {

using std::vector;
using std::map;
using std::pair;
using std::priority_queue;

class ClauseCleaner;
class SolutionExtender;
class Solver;
class GateFinder;
class XorFinderAbst;
class SubsumeStrengthen;
class BVA;

struct BlockedClause {
    BlockedClause()
    {}

    BlockedClause(Lit dummyLit) :
        blockedOn(dummyLit)
        , toRemove(false)
        , dummy(true)
    {}

    BlockedClause(
        const Lit _blockedOn
        , const vector<Lit>& _lits
    ) :
        blockedOn(_blockedOn)
        , toRemove(false)
        , lits(_lits)
        , dummy(false)
    {
    }

    Lit blockedOn;
    bool toRemove;
    vector<Lit> lits;
    bool dummy;
};

/**
@brief Handles subsumption, self-subsuming resolution, variable elimination, and related algorithms
*/
class OccSimplifier
{
public:

    //Construct-destruct
    OccSimplifier(Solver* solver);
    ~OccSimplifier();

    //Called from main
    bool simplify(const bool startup);
    void new_var(const Var orig_outer);
    void new_vars(const size_t n);
    void save_on_var_memory();
    bool uneliminate(const Var var);
    size_t mem_used() const;
    size_t mem_used_xor() const;
    void print_gatefinder_stats() const;
    void dump_blocked_clauses(std::ostream* outfile) const;

    //UnElimination
    void print_blocked_clauses_reverse() const;
    void extend_model(SolutionExtender* extender);
    uint32_t get_num_elimed_vars() const
    {
        return globalStats.numVarsElimed;
    }

    struct Stats
    {
        void print(const size_t nVars) const;
        void print_short(const Solver* solver, const bool print_var_elim = true) const;
        Stats& operator+=(const Stats& other);
        void clear();
        double total_time() const;

        uint64_t numCalls = 0;

        //Time stats
        double linkInTime = 0;
        double blockTime = 0;
        double varElimTime = 0;
        double finalCleanupTime = 0;

        //Startup stats
        uint64_t origNumFreeVars = 0;
        uint64_t origNumMaxElimVars = 0;
        uint64_t origNumIrredLongClauses = 0;
        uint64_t origNumRedLongClauses = 0;

        //Each algorithm
        uint64_t subsumedByVE = 0;

        //Stats for var-elim
        int64_t numVarsElimed = 0;
        uint64_t varElimTimeOut = 0;
        uint64_t clauses_elimed_long = 0;
        uint64_t clauses_elimed_tri = 0;
        uint64_t clauses_elimed_bin = 0;
        uint64_t clauses_elimed_sumsize = 0;
        uint64_t longRedClRemThroughElim = 0;
        uint64_t triRedClRemThroughElim = 0;
        uint64_t binRedClRemThroughElim = 0;
        uint64_t numRedBinVarRemAdded = 0;
        uint64_t testedToElimVars = 0;
        uint64_t triedToElimVars = 0;
        uint64_t usedAggressiveCheckToELim = 0;
        uint64_t newClauses = 0;

        //General stat
        uint64_t zeroDepthAssings = 0;
    };

    const Stats& get_stats() const;
    const SubsumeStrengthen* getSubsumeStrengthen() const;
    void check_elimed_vars_are_unassigned() const;
    bool getAnythingHasBeenBlocked() const;
    void freeXorMem();

private:
    friend class SubsumeStrengthen;
    SubsumeStrengthen* subsumeStrengthen;
    friend class BVA;
    BVA* bva;
    bool startup = false;
    bool backward_subsume();
    bool execute_simplifier_sched(const string& strategy);

    //debug
    void check_elimed_vars_are_unassignedAndStats() const;
    bool subsetReverse(const Clause& B) const;

    bool fill_occur();
    bool fill_occur_and_print_stats();
    void finishUp(size_t origTrailSize);
    vector<ClOffset> clauses;

    //Persistent data
    Solver*  solver;              ///<The solver this simplifier is connected to
    vector<uint16_t>& seen;
    vector<uint16_t>& seen2;
    vector<Lit>& toClear;

    //Temporaries
    vector<Lit>     dummy;       ///<Used by merge()
    vector<Lit>     gate_lits_of_elim_cls;

    //Limits
    uint64_t clause_lits_added;
    int64_t  strengthening_time_limit;              ///<Max. number self-subsuming resolution tries to do this run
    int64_t  subsumption_time_limit;              ///<Max. number backward-subsumption tries to do this run
    int64_t  norm_varelim_time_limit;
    int64_t  empty_varelim_time_limit;
    int64_t  varelim_num_limit;
    int64_t  aggressive_elim_time_limit;
    int64_t* limit_to_decrease;

    //Start-up
    bool add_from_solver(
        vector<ClOffset>& toAdd
        , bool alsoOccur
        , bool irred
    );
    struct LinkInData
    {
        LinkInData()
        {}

        LinkInData(uint64_t _cl_linked, uint64_t _cl_not_linked) :
            cl_linked(_cl_linked)
            , cl_not_linked(_cl_not_linked)
        {}

        uint64_t cl_linked = 0;
        uint64_t cl_not_linked = 0;
    };
    uint64_t calc_mem_usage_of_occur(const vector<ClOffset>& toAdd) const;
    void     print_mem_usage_of_occur(bool irred, uint64_t memUsage) const;
    void     print_linkin_data(const LinkInData link_in_data) const;
    bool     decide_occur_limit(bool irred, uint64_t memUsage);
    LinkInData link_in_clauses(
        const vector<ClOffset>& toAdd
        , bool irred
        , bool alsoOccur
    );
    void set_limits();

    //Finish-up
    void remove_by_drup_recently_blocked_clauses(size_t origBlockedSize);
    void add_back_to_solver();
    bool check_varelim_when_adding_back_cl(const Clause* cl) const;
    void remove_all_longs_from_watches();
    bool complete_clean_clause(Clause& ps);

    //Clause update
    lbool       clean_clause(ClOffset c);
    void        unlink_clause(ClOffset cc, bool drup = true, bool allow_empty_watch = false, bool only_set_is_removed = false);
    void        linkInClause(Clause& cl);
    bool        handleUpdatedClause(ClOffset c);

    struct watch_sort_smallest_first {
        bool operator()(const Watched& first, const Watched& second)
        {
            //Anything but clause!
            if (first.isClause())
                return false;
            if (second.isClause())
                return true;

            //BIN is better than TRI
            if (first.isBinary() && second.isTri()) return true;

            return false;
        }
    };

    /////////////////////
    //Variable elimination

    vector<pair<int, int> > varElimComplexity;
    ///Order variables according to their complexity of elimination
    struct VarOrderLt {
        const vector<pair<int, int> >&  varElimComplexity;
        bool operator () (const size_t x, const size_t y) const
        {
            //Smallest cost first
            if (varElimComplexity[x].first != varElimComplexity[y].first)
                return varElimComplexity[x].first < varElimComplexity[y].first;

            //Smallest cost first
            return varElimComplexity[x].second < varElimComplexity[y].second;
        }

        VarOrderLt(
            const vector<pair<int,int> >& _varElimComplexity
        ) :
            varElimComplexity(_varElimComplexity)
        {}
    };
    void        order_vars_for_elim();
    Heap<VarOrderLt> velim_order;
    //void        addRedBinaries(const Var var);
    size_t      rem_cls_from_watch_due_to_varelim(watch_subarray_const todo, const Lit lit);
    void        add_clause_to_blck(const Lit lit, const vector<Lit>& lits);
    void        set_var_as_eliminated(const Var var, const Lit lit);
    bool        can_eliminate_var(const Var var) const;


    TouchList   touched;
    vector<ClOffset> cl_to_free_later;
    bool        maybe_eliminate(const Var x);
    void        free_clauses_to_free();
    void        try_to_subsume_with_new_bin_or_tri(const vector<Lit>& lits);
    void        create_dummy_blocked_clause(const Lit lit);
    int         test_elim_and_fill_resolvents(Var var);
    void        mark_gate_in_poss_negs(Lit elim_lit, watch_subarray_const poss, watch_subarray_const negs);
    void        mark_gate_parts(
        Lit elim_lit
        , watch_subarray_const a
        , watch_subarray_const b
        , vector<char>& a_mark
        , vector<char>& b_mark
    );
    bool        find_gate(Lit elim_lit, watch_subarray_const a, watch_subarray_const b);
    bool        skip_resolution_thanks_to_gate(const size_t at_poss, const size_t at_negs) const;
    void        print_var_eliminate_stat(Lit lit) const;
    bool        add_varelim_resolvent(vector<Lit>& finalLits, const ClauseStats& stats);
    bool        check_if_new_2_long_subsumes_3_long_return_already_inside(const vector<Lit>& lits);
    void        update_varelim_complexity_heap(const Var var);
    void        print_var_elim_complexity_stats(const Var var) const;
    struct Resolvent {
        Resolvent(const vector<Lit>& _lits, const ClauseStats _stats) :
            lits(_lits)
            , stats(_stats)
        {}
        vector<Lit> lits;
        ClauseStats stats;
        bool operator<(const Resolvent& other) const
        {
            return lits.size() > other.lits.size();
        }
    };
    vector<Resolvent> resolvents;
    vector<char> poss_gate_parts;
    vector<char> negs_gate_parts;
    bool gate_found_elim;

    struct HeuristicData
    {
        HeuristicData() :
            bin(0)
            , tri(0)
            , longer(0)
            , lit(0)
            , count(std::numeric_limits<uint32_t>::max())
        {}

        uint32_t totalCls() const
        {
            return bin + tri + longer;
        }

        uint32_t bin;
        uint32_t tri;
        uint32_t longer;
        uint32_t lit;
        uint32_t count; //resolution count (if can be counted, otherwise MAX)
    };
    HeuristicData calc_data_for_heuristic(const Lit lit);
    std::pair<int, int> strategyCalcVarElimScore(const Var var);
    uint64_t time_spent_on_calc_otf_update;
    uint64_t num_otf_update_until_now;

    //For empty resolvents
    enum class ResolvCount{count, set, unset};
    bool check_empty_resolvent(const Lit lit);
    int check_empty_resolvent_action(
        const Lit lit
        , ResolvCount action
        , int otherSize
    );

    pair<int, int>  heuristicCalcVarElimScore(const Var var);
    bool resolve_clauses(
        const Watched ps
        , const Watched qs
        , const Lit noPosLit
        , const bool useCache
    );
    void add_pos_lits_to_dummy_and_seen(
        const Watched ps
        , const Lit posLit
    );
    bool add_neg_lits_to_dummy_and_seen(
        const Watched qs
        , const Lit posLit
    );
    bool reverse_distillation_of_dummy(
        const Watched ps
        , const Watched qs
        , const Lit posLit
    );
    bool subsume_dummy_through_stamping(
       const Watched ps
        , const Watched qs
    );
    bool aggressiveCheck(
        const Lit lit
        , const Lit noPosLit
        , bool& retval
    );
    bool eliminate_vars();
    void eliminate_empty_resolvent_vars();

    /////////////////////
    //Helpers
    friend class XorFinder;
    friend class GateFinder;
    XorFinderAbst *xorFinder;
    GateFinder *gateFinder;

    /////////////////////
    //Blocked clause elimination
    bool anythingHasBeenBlocked;
    vector<BlockedClause> blockedClauses;
    map<Var, vector<size_t> > blk_var_to_cl;
    bool blockedMapBuilt;
    void buildBlockedMap();
    void cleanBlockedClauses();

    //validity checking
    void sanityCheckElimedVars();
    void printOccur(const Lit lit) const;

    ///Stats from this run
    Stats runStats;

    ///Stats globally
    Stats globalStats;
};

inline const OccSimplifier::Stats& OccSimplifier::get_stats() const
{
    return globalStats;
}

inline bool OccSimplifier::getAnythingHasBeenBlocked() const
{
    return anythingHasBeenBlocked;
}

inline std::ostream& operator<<(std::ostream& os, const BlockedClause& bl)
{
    os << bl.lits << " blocked on: " << bl.blockedOn;

    return os;
}

inline bool OccSimplifier::subsetReverse(const Clause& B) const
{
    for (uint32_t i = 0; i != B.size(); i++) {
        if (!seen[B[i].toInt()])
            return false;
    }
    return true;
}

inline const SubsumeStrengthen* OccSimplifier::getSubsumeStrengthen() const
{
    return subsumeStrengthen;
}

} //end namespace

#endif //SIMPLIFIER_H
