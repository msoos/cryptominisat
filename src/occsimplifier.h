/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

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

#pragma once

#include <map>
#include <vector>
#include <set>
#include <map>

#include "clause.h"
#include "solvertypes.h"
#include "heap.h"
#include "touchlist.h"
#include "watched.h"
#include "watcharray.h"
struct PicoSAT;

namespace CMSat {

using std::vector;
using std::map;
using std::set;
using std::pair;

class ClauseCleaner;
class SolutionExtender;
class Solver;
class SubsumeStrengthen;
class GateFinder;

struct ElimedClauses {
    ElimedClauses() = default;
    explicit ElimedClauses(size_t _start, size_t _end, bool _is_xor):
        start(_start) , end(_end),  is_xor(_is_xor) {}
    const Lit& at(const uint64_t at, const vector<Lit>& elimed_cls_lits) const {
        return elimed_cls_lits[start+at];
    }
    Lit& at(const uint64_t at, vector<Lit>& elimed_cls_lits) {
        return elimed_cls_lits[start+at];
    }

    uint64_t size() const { return end-start; }
    uint64_t start;
    uint64_t end;
    bool toRemove = false;
    bool is_xor = false;
};

struct BVEStats
{
    uint64_t numCalls = 0;
    double timeUsed = 0.0;

    int64_t numVarsElimed = 0;
    uint64_t varElimTimeOut = 0;
    uint64_t clauses_elimed_long = 0;
    uint64_t clauses_elimed_bin = 0;
    uint64_t clauses_elimed_sumsize = 0;
    uint64_t testedToElimVars = 0;
    uint64_t triedToElimVars = 0;
    uint64_t newClauses = 0;
    uint64_t subsumedByVE = 0;
    uint64_t gatefind_timeouts = 0;

    BVEStats& operator+=(const BVEStats& other);

    void print_short() const {
        cout << "c [occ-bve]" << " elimed: " << numVarsElimed
            << " gatefind timeout: " << gatefind_timeouts << endl;
        cout << "c [occ-bve]" << " cl-new: " << newClauses << " tried: " << triedToElimVars
            << " tested: " << testedToElimVars << endl;
        cout << "c [occ-bve]" << " subs: "  << subsumedByVE << endl;
    }

    void print() const {
        print_stats_line("c timeouted" , stats_line_percent(varElimTimeOut, numCalls) , "% called");
        print_stats_line("c v-elimed" , numVarsElimed , "% vars");
        print_stats_line("c cl-new" , newClauses);
        print_stats_line("c tried to elim" , triedToElimVars);
        print_stats_line("c cl-elim-bin" , clauses_elimed_bin);
        print_stats_line("c cl-elim-long" , clauses_elimed_long);
        print_stats_line("c cl-elim-avg-s",
            ((double)clauses_elimed_sumsize
            /(double)(clauses_elimed_bin + clauses_elimed_long))
        );
        print_stats_line("c v-elim-sub" , subsumedByVE);
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
    explicit OccSimplifier(Solver* solver);
    ~OccSimplifier();

    //Called from main
    vector<OrGate> recover_or_gates();
    vector<ITEGate> recover_ite_gates();

    // definable vars
    vector<uint32_t> remove_definable_by_irreg_gate(const vector<uint32_t>& vars);
    vector<uint32_t> extend_definable_by_irreg_gate(const vector<uint32_t>& vars);
    void clean_sampl_get_empties(vector<uint32_t>& sampl_vars, set<uint32_t>& empty_vars);
    bool elim_var_by_str(uint32_t var, const vector<pair<ClOffset, ClOffset>>& cls);
    uint32_t add_cls_to_picosat_definable(const Lit wsLit);
    PicoSAT* picosat = nullptr;
    int lit_to_picolit(const Lit l);
    uint64_t picolits_added = 0;
    vector<int> var_to_picovar;
    vector<uint32_t> picovars_used;
    vector<std::pair<ClOffset, ClOffset>> equiv_subformula_cls;
    bool check_equiv_subformula(Lit lit);

    bool simplify(const bool _startup, const std::string& schedule);
    void new_var(const uint32_t orig_outer);
    void new_vars(const size_t n);
    void save_on_var_memory();
    bool uneliminate(const uint32_t var);
    size_t mem_used() const;
    uint32_t dump_elimed_clauses(std::ostream* outfile) const;
    bool get_elimed_clause_at(uint32_t& at,uint32_t& at2, vector<Lit>& out, bool& is_xor) const;
    vector<vector<Lit>> get_elimed_clauses_for(uint32_t outer_v);
    void subs_with_resolvent_clauses();
    void fill_tocheck_seen(const vec<Watched>& ws, vector<uint32_t>& tocheck);
    void delete_component_unconnected_to_assumps(); //for arjun
    void strengthen_dummy_with_bins(const bool avoid_redundant);
    void reverse_blocked_clause_elim();
    bool lit_rem_with_or_gates();
    bool cl_rem_with_or_gates();

    //UnElimination
    void print_elimed_clauses_reverse() const;
    void extend_model(SolutionExtender* extender);
    uint32_t get_num_elimed_vars() const
    {
        return bvestats_global.numVarsElimed;
    }

    struct Stats
    {
        void print(const size_t nVars, OccSimplifier* occs) const;
        void print_extra_times(const char* prefix) const;
        Stats& operator+=(const Stats& other);
        void clear();
        double total_time(OccSimplifier* occs) const;

        uint64_t numCalls = 0;
        uint64_t ternary_added_tri = 0;
        uint64_t ternary_added_bin = 0;

        //Time stats
        double linkInTime = 0;
        double varElimTime = 0;
        double xorTime = 0;
        double triresolveTime = 0;
        double finalCleanupTime = 0;

        //General stat
        uint64_t zeroDepthAssings = 0;
    };

    BVEStats bvestats;
    BVEStats bvestats_global;

    const Stats& get_stats() const;
    const SubsumeStrengthen* get_sub_str() const;

    //validity checking
    void check_elimed_vars_are_unassigned() const;
    void check_no_marked_clauses();
    void sanityCheckElimedVars() const;
    void printOccur(const Lit lit) const;
    void check_clauses_lits_ordered() const;

    /// Used ONLY for XOR, changes occur setup
    void sort_occurs_and_set_abst();
    vector<ClOffset> added_long_cl;
    TouchList added_cl_to_var; // to perform forward-subsumption of clauses added as part of BVE
                               // contains a list of literals that have been added as part of
                               // the clauses that have been added as resolvents
    vector<uint32_t> n_occurs;
    TouchList removed_cl_with_var;
    vector<std::pair<Lit, Lit> > added_irred_bin;
    vector<ClOffset> clauses;
    void check_elimed_vars_are_unassignedAndStats() const;
    void unlink_clause(ClOffset cc
        , bool frat = true
        , bool allow_empty_watch = false
        , bool only_set_is_removed = false
    );
    void free_clauses_to_free();

    //Setup and teardown. Should be private, but testing needs it to be public
    bool setup();
    void finish_up(size_t origTrailSize);

    //Ternary resolution. Should be private but testing needs it to be public
    bool ternary_res();

#ifdef ARJUN_SERIALIZE
    template<class T>
    void serialize_elimed_cls  (T& ar) const;
    template<class T>
    void unserialize_elimed_cls(T& ar);
#endif

private:
    friend class SubsumeStrengthen;
    SubsumeStrengthen* sub_str;
    void check_cls_sanity();

    bool startup = false;
    bool backward_sub_str();
    bool gate_based_eqlit();
    void backward_sub();
    bool execute_simplifier_strategy(const string& strategy);
    bool remove_literal(ClOffset c, const Lit toRemoveLit, bool only_set_is_removed);

    //Ternary resolution
    vector<Lit> finalLits_ternary;
    vector<Lit> tmp_tern_res;
    bool perform_ternary(Clause* cl, ClOffset offs, Sub1Ret& sub1_ret);
    void check_ternary_cl(Clause* cl, ClOffset offs, watch_subarray ws);
    struct Tri {
        Lit lits[3];
        uint32_t size = 0;
        Tri () : size(0) {}
        Tri(const Tri & other)
        {
            memcpy(lits, other.lits, sizeof(Lit)*3);
            size = other.size;
        }
    };
    vector<Tri> cl_to_add_ternary;

    //Persistent data
    Solver*  solver;              ///<The solver this simplifier is connected to
    vector<uint32_t>& seen;
    vector<uint8_t>& seen2;
    vector<Lit>& toClear;
    vector<bool> sampling_vars_occsimp;
    vector<bool> xorclauses_vars;

    //Temporaries
    vector<Lit>     dummy;       ///<Used by merge()

    //Time Limits
    uint64_t clause_lits_added;
    int64_t  strengthening_time_limit;
    int64_t  gate_based_litrem_time_limit;
    int64_t  subsumption_time_limit;
    int64_t  norm_varelim_time_limit;
    int64_t  resolvent_sub_time_limit;
    int64_t  xor_varelim_time_limit;
    int64_t  empty_varelim_time_limit;
    int64_t  varelim_num_limit;
    int64_t  varelim_sub_str_limit;
    int64_t  ternary_res_time_limit;
    int64_t  ternary_res_cls_limit;
    int64_t  occ_based_lit_rem_time_limit;
    int64_t  weaken_time_limit;
    int64_t  dummy_str_time_limit;
    int64_t* limit_to_decrease;

    //Memory limits
    int64_t  varelim_linkin_limit_bytes;

    //Start-up
    bool fill_occur();
    bool fill_occur_and_print_stats();
    struct LinkInData
    {
        LinkInData()
        {}

        LinkInData(uint64_t _cl_linked, uint64_t _cl_not_linked) :
            cl_linked(_cl_linked)
            , cl_not_linked(_cl_not_linked)
        {}

        LinkInData& combine(const LinkInData& other)
        {
            cl_linked += other.cl_linked;
            cl_not_linked += other.cl_not_linked;
            return *this;
        }

        uint64_t cl_linked = 0;
        uint64_t cl_not_linked = 0;
    };
    LinkInData link_in_data_irred;
    LinkInData link_in_data_red;
    uint64_t calc_mem_usage_of_occur(const vector<ClOffset>& toAdd) const;
    void     print_mem_usage_of_occur(uint64_t memUsage) const;
    void     print_linkin_data(const LinkInData link_in_data) const;
    OccSimplifier::LinkInData link_in_clauses(
        const vector<ClOffset>& toAdd
        , bool alsoOccur
        , uint32_t max_size
        , int64_t link_in_lit_limit
    );
    void set_limits();

    //Finish-up
    void remove_by_frat_recently_elimed_clauses(size_t origElimedSize);
    void add_back_to_solver();
    bool check_varelim_when_adding_back_cl(const Clause* cl) const;
    void remove_all_longs_from_watches();
    bool complete_clean_clause(Clause& ps);

    //Clause update
    bool clean_clause(ClOffset c, bool only_set_is_removed);
    void        link_in_clause(Clause& cl);
    bool        handleUpdatedClause(ClOffset c);
    uint32_t    sum_irred_cls_longs() const;
    uint32_t    sum_irred_cls_longs_lits() const;
    Clause *    full_add_clause(
        const vector<Lit>& tmp_cl,
        vector<Lit>& finalLits,
        ClauseStats* cl_stats,
        bool red
    );

    ///////
    // Clearing clauses
    vector<ClOffset> cls_to_clean_tmp;
    size_t last_trail_cleared;
    bool clear_vars_from_cls_that_have_been_set();

    /////////////////////
    //Variable elimination
    uint32_t grow = 0; /// maximum grow rate for clauses
    vector<uint64_t> varElimComplexity;
    ///Order variables according to their complexity of elimination
    struct VarOrderLt {
        const vector<uint64_t>&  varElimComplexity;
        bool operator () (const uint64_t x, const uint64_t y) const
        {
            return varElimComplexity[x] < varElimComplexity[y];
        }

        explicit VarOrderLt(
            const vector<uint64_t>& _varElimComplexity
        ) :
            varElimComplexity(_varElimComplexity)
        {}
    };
    void        order_vars_for_elim();
    Heap<VarOrderLt> velim_order;
    void        rem_cls_from_watch_due_to_varelim(const Lit lit, bool only_set_is_removed = true);
    vector<Lit> tmp_rem_lits;
    vec<Watched> tmp_rem_cls_copy;
    void        add_clause_to_blck(const vector<Lit>& lits, const int32_t ID);
    void        set_var_as_eliminated(const uint32_t var);
    bool        can_eliminate_var(const uint32_t var, const bool ignore_xor = false) const;
    bool        mark_and_push_to_added_long_cl_cls_containing(const Lit lit);
    bool        simulate_frw_sub_str_with_added_cl_to_var();


    TouchList   elim_calc_need_update;
    vector<ClOffset> cl_to_free_later;
    bool        maybe_eliminate(const uint32_t var);
    bool        forward_subsume_irred(
        const Lit lit,
        cl_abst_type abs,
        const uint32_t size);
    vector<Lit> weaken_dummy;
    bool check_taut_weaken_dummy(const uint32_t dontuse);
    vector<Lit> antec_poss_weakened;
    vector<Lit> antec_negs_weakened;
    void weaken(const Lit lit, const vec<Watched>& in, vector<Lit>& out);
    bool generate_resolvents(
        vec<Watched>& tmp_poss,
        vec<Watched>& tmp_negs,
        Lit lit,
        const uint32_t limit);
    bool generate_resolvents_weakened(
        vector<Lit>& tmp_poss,
        vector<Lit>& tmp_negs,
        vec<Watched>& tmp_poss2,
        vec<Watched>& tmp_negs2,
        Lit lit,
        const uint32_t limit);
    void get_antecedents(
        const vec<Watched>& gates,
        const vec<Watched>& full_set,
        vec<Watched>& output);
    bool sub_str_with_added_long_and_bin(const bool verbose = true);
    bool sub_with_added_long_and_bin(const bool verbose = true);
    vector<Lit> tmp_bin_cl;
    vec<Watched> gates_poss;
    vec<Watched> gates_negs;
    vec<Watched> antec_poss;
    vec<Watched> antec_negs;
    vec<Watched> poss;
    vec<Watched> negs;
    void clean_from_satisfied(vec<Watched>& in);
    void clean_from_red_or_removed(
        const vec<Watched>& in,
        vec<Watched>& out);
    void  create_dummy_elimed_clause(Lit lit, bool is_xor = false);
    vector<OccurClause> tmp_subs;
    bool        test_elim_and_fill_resolvents(uint32_t var);
    void        get_gate(Lit elim_lit, watch_subarray_const poss, watch_subarray_const negs);
    bool find_or_gate(
        Lit lit,
        watch_subarray_const a,
        watch_subarray_const b,
        vec<Watched>& out_a,
        vec<Watched>& out_b
    );
    void add_picosat_cls(const vec<Watched>& ws, const Lit elim_lit, map<int, Watched>& picosat_cl_to_cms_cl);
    bool turned_off_irreg_gate = false;
    bool resolve_gate;
    bool find_irreg_gate(
        Lit elim_lit,
        watch_subarray_const a,
        watch_subarray_const b,
        vec<Watched>& out_a,
        vec<Watched>& out_b);
    bool find_equivalence_gate(
        Lit lit
        , watch_subarray_const a
        , watch_subarray_const b
        , vec<Watched>& out_a
        , vec<Watched>& out_b);
    bool find_xor_gate(
        Lit lit
        , watch_subarray_const a
        , watch_subarray_const b
        , vec<Watched>& out_a
        , vec<Watched>& out_b);
    bool find_ite_gate(
        Lit elim_lit
        , watch_subarray_const a
        , watch_subarray_const b
        , vec<Watched>& out_a
        , vec<Watched>& out_b
        , vec<Watched>* out_a_all = nullptr
    );
    vector<Clause*> toclear_marked_cls;
    set<uint32_t> parities_found;
    void        print_var_eliminate_stat(Lit lit) const;
    bool        add_varelim_resolvent(vector<Lit>& finalLits, const ClauseStats& stats);
    void        update_varelim_complexity_heap();
    void        print_var_elim_complexity_stats(const uint32_t var) const;

    //OccSimp
    bool        try_remove_lit_via_occurrence_simpl(const OccurClause& occ_cl);
    bool        occurrence_simp_based_resolv_skip(const Watched* it, const Lit lit);
    bool        occ_based_lit_rem(uint32_t var, uint32_t& removed);
    bool        all_occ_based_lit_rem();


    struct Resolvents {
        uint32_t at = 0;
        vector<vector<Lit>> resolvents_lits;
        vector<ClauseStats> resolvents_stats;
        void clear() {
            at = 0;
        }
        void add_resolvent(const vector<Lit>& res, const ClauseStats& stats) {
            if (resolvents_lits.size() < at+1) {
                resolvents_lits.resize(at+1);
                resolvents_stats.resize(at+1);
            }

            resolvents_lits[at] = res;
            resolvents_stats[at] = stats;
            at++;
        }
        vector<Lit>& back_lits() {
            assert(at > 0);
            return resolvents_lits[at-1];
        }
        const ClauseStats& back_stats() const {
            assert(at > 0);
            return resolvents_stats[at-1];
        }
        void pop() {
            at--;
        }
        bool empty() const {
            return at == 0;
        }
        uint32_t size() const {
            return at;
        }
    };
    Resolvents resolvents;
    uint32_t calc_data_for_heuristic(const Lit lit);
    uint64_t time_spent_on_calc_otf_update;
    uint64_t num_otf_update_until_now;

    //for n_occur checking only
    uint32_t calc_occ_data(const Lit lit);
    void     check_n_occur();

    //For empty resolvents
    enum class ResolvCount{count, set, unset};
    bool check_empty_resolvent(const Lit lit);
    int check_empty_resolvent_action(
        const Lit lit
        , ResolvCount action
        , int otherSize
    );

    uint64_t heuristicCalcVarElimScore(const uint32_t var);
    bool resolve_clauses(
        const Watched& ps
        , const Watched& qs
        , const Lit& noPosLit
    );
    void add_pos_lits_to_dummy_and_seen(
        const Watched& ps
        , const Lit& posLit
    );
    bool add_neg_lits_to_dummy_and_seen(
        const Watched& qs
        , const Lit& posLit
    );
    bool eliminate_vars();
    void eliminate_empty_resolvent_vars();
    void eliminate_xor_vars();
    bool only_red_and_idx_occ(const Lit l) const;

    /////////////////////
    //Helpers
    friend class GateFinder;
    GateFinder *gateFinder = nullptr;

    /////////////////////
    //Elimed clause elimination
    vector<Lit> elimed_cls_lits;
    vector<ElimedClauses> elimed_cls; ///<maps var(outer!!) to postion in elimedClauses
    vector<uint32_t> blk_var_to_cls; //< indexed by OUTER var
    vector<int32_t> newly_elimed_cls_IDs; // temporary storage for newly elimed cls' IDs
    bool elimed_map_built = false;
    void build_elimed_map();
    void clean_elimed_cls();
    bool can_remove_elimed_clauses = false;

    ///Stats from this run
    Stats runStats;

    ///Stats globally
    Stats globalStats;
};

inline const OccSimplifier::Stats& OccSimplifier::get_stats() const
{
    return globalStats;
}

inline const SubsumeStrengthen* OccSimplifier::get_sub_str() const
{
    return sub_str;
}

#ifdef ARJUN_SERIALIZE
template<class T>
void OccSimplifier::unserialize_elimed_cls(T& ar)
{
    ar >> eClsLits;
    ar >> elimedClauses;
}

template<class T>
void OccSimplifier::serialize_elimed_cls(T& ar) const
{
    ar << eClsLits;
    ar << elimedClauses;
}
#endif

} //end namespace
