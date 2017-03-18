/******************************************
Copyright (c) 2016, Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***********************************************/

#ifndef SIMPLIFIER_H
#define SIMPLIFIER_H


#include <map>
#include <vector>
#include <list>
#include <set>
#include <queue>
#include <map>
#include <iomanip>
#include <fstream>

#include "clause.h"
#include "solvertypes.h"
#include "heap.h"
#include "touchlist.h"
#include "varupdatehelper.h"
#include "watched.h"
#include "watcharray.h"
#include "simplefile.h"

namespace CMSat {

using std::vector;
using std::map;
using std::set;
using std::pair;
using std::priority_queue;

class ClauseCleaner;
class SolutionExtender;
class Solver;
class TopLevelGaussAbst;
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

    void save_to_file(SimpleOutFile& f) const
    {
        f.put_lit(blockedOn);
        f.put_uint32_t(toRemove);
        f.put_vector(lits);
        f.put_uint32_t(dummy);
    }

    void load_from_file(SimpleInFile& f)
    {
        blockedOn = f.get_lit();
        toRemove = f.get_uint32_t();
        f.get_vector(lits);
        dummy = f.get_uint32_t();
    }

    Lit blockedOn = lit_Undef;
    bool toRemove = false;
    vector<Lit> lits;
    bool dummy = false;
};

struct BVEStats
{
    uint64_t numCalls = 0;
    double timeUsed = 0.0;

    int64_t numVarsElimed = 0;
    uint64_t varElimTimeOut = 0;
    uint64_t clauses_elimed_long = 0;
    uint64_t clauses_elimed_tri = 0;
    uint64_t clauses_elimed_bin = 0;
    uint64_t clauses_elimed_sumsize = 0;
    uint64_t longRedClRemThroughElim = 0;
    uint64_t binRedClRemThroughElim = 0;
    uint64_t numRedBinVarRemAdded = 0;
    uint64_t testedToElimVars = 0;
    uint64_t triedToElimVars = 0;
    uint64_t usedAggressiveCheckToELim = 0;
    uint64_t newClauses = 0;
    uint64_t subsumedByVE = 0;

    BVEStats& operator+=(const BVEStats& other);

    void print() const
    {
        //About elimination
        cout
        << "c [occ-bve]"
        << " elimed: " << numVarsElimed
        << endl;

        cout
        << "c [occ-bve]"
        << " cl-new: " << newClauses
        << " tried: " << triedToElimVars
        << " tested: " << testedToElimVars
        << " ("
        << stats_line_percent(usedAggressiveCheckToELim, testedToElimVars)
        << " % aggressive)"
        << endl;

        cout
        << "c [occ-bve]"
        << " subs: "  << subsumedByVE
        << " red-bin rem: " << binRedClRemThroughElim
        << " red-long rem: " << longRedClRemThroughElim
        << endl;
    }

    void print()
    {
        print_stats_line("c timeouted"
            , stats_line_percent(varElimTimeOut, numCalls)
            , "% called"
        );
        print_stats_line("c v-elimed"
            , numVarsElimed
            , "% vars"
        );

        /*cout << "c"
        << " v-elimed: " << numVarsElimed
        << " / " << origNumMaxElimVars
        << " / " << origNumFreeVars
        << endl;*/

        print_stats_line("c cl-new"
            , newClauses
        );

        print_stats_line("c tried to elim"
            , triedToElimVars
            , stats_line_percent(usedAggressiveCheckToELim, triedToElimVars)
            , "% aggressively"
        );

        print_stats_line("c elim-bin-lt-cl"
            , binRedClRemThroughElim);

        print_stats_line("c elim-long-lt-cl"
            , longRedClRemThroughElim);

        print_stats_line("c lt-bin added due to v-elim"
            , numRedBinVarRemAdded);

        print_stats_line("c cl-elim-bin"
            , clauses_elimed_bin);

        print_stats_line("c cl-elim-tri"
            , clauses_elimed_tri);

        print_stats_line("c cl-elim-long"
            , clauses_elimed_long);

        print_stats_line("c cl-elim-avg-s",
            ((double)clauses_elimed_sumsize
            /(double)(clauses_elimed_bin + clauses_elimed_tri + clauses_elimed_long))
        );

        print_stats_line("c v-elim-sub"
            , subsumedByVE
        );
    }
    void clear() {
        BVEStats tmp;
        *this = tmp;
    }
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
    bool setup();
    bool simplify(const bool _startup, const std::string schedule);
    void new_var(const uint32_t orig_outer);
    void new_vars(const size_t n);
    void save_on_var_memory();
    bool uneliminate(const uint32_t var);
    size_t mem_used() const;
    size_t mem_used_xor() const;
    size_t mem_used_bva() const;
    void print_gatefinder_stats() const;
    void dump_blocked_clauses(std::ostream* outfile) const;

    //UnElimination
    void print_blocked_clauses_reverse() const;
    void extend_model(SolutionExtender* extender);
    uint32_t get_num_elimed_vars() const
    {
        return bvestats_global.numVarsElimed;
    }

    struct Stats
    {
        void print(const size_t nVars) const;
        void print_short() const;
        Stats& operator+=(const Stats& other);
        void clear();
        double total_time() const;

        uint64_t numCalls = 0;

        //Time stats
        double linkInTime = 0;
        double blockTime = 0;
        double varElimTime = 0;
        double finalCleanupTime = 0;

        //General stat
        uint64_t zeroDepthAssings = 0;
    };

    BVEStats bvestats;
    BVEStats bvestats_global;

    const Stats& get_stats() const;
    const SubsumeStrengthen* getSubsumeStrengthen() const;
    void check_elimed_vars_are_unassigned() const;
    bool getAnythingHasBeenBlocked() const;
    void freeXorMem();
    void save_state(SimpleOutFile& f);
    void load_state(SimpleInFile& f);
    vector<ClOffset> sub_str_with;
    TouchListLit impl_sub_lits;
    vector<ClOffset> clauses;
    void check_elimed_vars_are_unassignedAndStats() const;
    void unlink_clause(ClOffset cc
        , bool drat = true
        , bool allow_empty_watch = false
        , bool only_set_is_removed = false
    );
    void free_clauses_to_free();
    void cleanBlockedClausesIfDirty();

private:
    friend class SubsumeStrengthen;
    SubsumeStrengthen* sub_str;
    friend class BVA;
    BVA* bva;
    bool startup = false;
    bool backward_sub_str();
    bool execute_simplifier_strategy(const string& strategy);

    //debug
    bool subsetReverse(const Clause& B) const;

    bool fill_occur();
    bool fill_occur_and_print_stats();
    void finishUp(size_t origTrailSize);

    //Persistent data
    Solver*  solver;              ///<The solver this simplifier is connected to
    vector<uint16_t>& seen;
    vector<uint8_t>& seen2;
    vector<Lit>& toClear;

    //Temporaries
    vector<Lit>     dummy;       ///<Used by merge()

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
    void     print_mem_usage_of_occur(uint64_t memUsage) const;
    void     print_linkin_data(const LinkInData link_in_data) const;
    bool     decide_occur_limit(bool irred, uint64_t memUsage);
    OccSimplifier::LinkInData link_in_clauses(
        const vector<ClOffset>& toAdd
        , bool alsoOccur
        , uint32_t max_size
        , int64_t link_in_lit_limit
    );
    void set_limits();

    //Finish-up
    void remove_by_drat_recently_blocked_clauses(size_t origBlockedSize);
    void add_back_to_solver();
    bool check_varelim_when_adding_back_cl(const Clause* cl) const;
    void remove_all_longs_from_watches();
    bool complete_clean_clause(Clause& ps);

    //Clause update
    lbool       clean_clause(ClOffset c);
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

            //Both are bin
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
    size_t      rem_cls_from_watch_due_to_varelim(watch_subarray todo, const Lit lit);
    vector<Lit> tmp_rem_lits;
    vec<Watched> tmp_rem_cls_copy;
    void        add_clause_to_blck(const Lit lit, const vector<Lit>& lits);
    void        set_var_as_eliminated(const uint32_t var, const Lit lit);
    bool        can_eliminate_var(const uint32_t var) const;


    TouchList   touched;
    vector<ClOffset> cl_to_free_later;
    bool        maybe_eliminate(const uint32_t x);
    void        create_dummy_blocked_clause(const Lit lit);
    int         test_elim_and_fill_resolvents(uint32_t var);
    void        mark_gate_in_poss_negs(Lit elim_lit, watch_subarray_const poss, watch_subarray_const negs);
    void        find_gate(Lit elim_lit, watch_subarray_const a, watch_subarray_const b);
    void        print_var_eliminate_stat(Lit lit) const;
    bool        add_varelim_resolvent(vector<Lit>& finalLits, const ClauseStats& stats);
    void        update_varelim_complexity_heap(const uint32_t var);
    void        print_var_elim_complexity_stats(const uint32_t var) const;
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
    Clause* gate_varelim_clause;

    struct HeuristicData
    {
        HeuristicData() :
            bin(0)
            , longer(0)
            , lit(0)
            , count(std::numeric_limits<uint32_t>::max())
        {}

        uint32_t totalCls() const
        {
            return bin + longer;
        }

        uint32_t bin;
        uint32_t longer;
        uint32_t lit;
        uint32_t count; //resolution count (if can be counted, otherwise MAX)
    };
    HeuristicData calc_data_for_heuristic(const Lit lit);
    std::pair<int, int> strategyCalcVarElimScore(const uint32_t var);
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

    pair<int, int>  heuristicCalcVarElimScore(const uint32_t var);
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
    friend class TopLevelGaussAbst;
    //friend class GateFinder;
    TopLevelGaussAbst *topLevelGauss;
    //GateFinder *gateFinder;

    /////////////////////
    //Blocked clause elimination
    bool anythingHasBeenBlocked;
    vector<BlockedClause> blockedClauses;
    map<uint32_t, vector<size_t> > blk_var_to_cl;
    bool blockedMapBuilt;
    void buildBlockedMap();
    void cleanBlockedClauses();
    bool can_remove_blocked_clauses = false;

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
    return sub_str;
}

} //end namespace

#endif //SIMPLIFIER_H
