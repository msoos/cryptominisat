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

#ifndef __SEARCHER_H__
#define __SEARCHER_H__

#include <array>

#include "propengine.h"
#include "solvertypes.h"
#include "time_mem.h"
#include "hyperengine.h"
#include "MersenneTwister.h"
#include "simplefile.h"
#include "searchstats.h"
#include "searchhist.h"

#ifdef CMS_TESTING_ENABLED
#include "gtest/gtest_prod.h"
#endif

namespace CMSat {

class Solver;
class SQLStats;
class VarReplacer;
class EGaussian;
class DistillerLong;

using std::string;
using std::cout;
using std::endl;

struct VariableVariance
{
    double avgDecLevelVarLT = 0;
    double avgTrailLevelVarLT= 0;
    double avgDecLevelVar = 0;
    double avgTrailLevelVar = 0;
};

struct ConflictData {
    uint32_t nHighestLevel;
};

class Searcher : public HyperEngine
{
    public:
        Searcher(const SolverConf* _conf, Solver* solver, std::atomic<bool>* _must_interrupt_inter);
        virtual ~Searcher();
        ///////////////////////////////
        // Solving
        //
        lbool solve(
            uint64_t max_confls
        );
        void finish_up_solve(lbool status);
        bool clean_clauses_if_needed();
        #ifdef STATS_NEEDED
        void check_calc_satzilla_features(bool force = false);
        #endif
        #ifdef STATS_NEEDED_BRANCH
        void check_calc_vardist_features(bool force = false);
        #endif
        void dump_search_loop_stats(double myTime);
        bool must_abort(lbool status);
        PropBy insert_gpu_clause(Lit* lits, uint32_t count);
        uint64_t luby_loop_num = 0;
        MTRand mtrand; ///< random number generator
        void set_seed(const uint32_t seed);


        vector<lbool>  model;
        vector<Lit>   conflict;     ///<If problem is unsatisfiable (possibly under assumptions), this vector represent the final conflict clause expressed in the assumptions.
        template<bool inprocess, bool red_also = true, bool distill_use = false>
        PropBy propagate();

        ///////////////////////////////
        // Stats
        //Restart print status
        uint64_t lastRestartPrint = 0;
        uint64_t lastRestartPrintHeader = 0;
        void     print_restart_stat();
        void     print_iteration_solving_stats();
        void     print_restart_header();
        void     print_restart_stat_line() const;
        void     print_restart_stats_base() const;
        void     print_clause_stats() const;
        uint64_t sumRestarts() const;
        const SearchHist& getHistory() const;
        void print_local_restart_budget();

        size_t hyper_bin_res_all(const bool check_for_set_values = true);
        std::pair<size_t, size_t> remove_useless_bins(bool except_marked = false);

        ///Returns l_Undef if not inside, l_True/l_False otherwise
        lbool var_inside_assumptions(const uint32_t var) const
        {
            #ifdef SLOW_DEBUG
            assert(var < nVars());
            #endif
            return varData[var].assumption;
        }
        lbool lit_inside_assumptions(const Lit lit) const
        {
            #ifdef SLOW_DEBUG
            assert(lit.var() < nVars());
            #endif
            if (varData[lit.var()].assumption == l_Undef) {
                return l_Undef;
            } else {
                lbool val = varData[lit.var()].assumption;
                return val ^ lit.sign();
            }
        }

        //ChronoBT
        template<bool do_insert_var_order = true, bool inprocess = false>
        void cancelUntil(uint32_t level); ///<Backtrack until a certain level.
        void cancelUntil_light();
        ConflictData find_conflict_level(PropBy& pb);
        uint32_t chrono_backtrack = 0;
        uint32_t non_chrono_backtrack = 0;
        void consolidate_watches(const bool full);

        //Gauss
        bool clear_gauss_matrices(const bool destruct = false);
        void print_matrix_stats();
        void check_need_gauss_jordan_disable();

        double get_cla_inc() const
        {
            return cla_inc;
        }

        //assumptions
        void check_assumptions_sanity();
        void unfill_assumptions_set();
        bool check_order_heap_sanity();

        template<bool inprocess>
        void bump_cl_act(Clause* cl);
        void simple_create_learnt_clause(
            PropBy confl,
            vector<Lit>& out_learnt,
            bool True_confl
        );

        #ifdef STATS_NEEDED
        void dump_restart_sql(rst_dat_type type, int64_t clauseID = -1);
        uint64_t last_dumped_conflict_rst_data_for_var = numeric_limits<uint64_t>::max();
        template<class T>
        uint32_t calc_connects_num_communities(const T& cl);
        #endif

        /////////////////////
        // Branching
        /////////////////////
        double var_inc_vsids;
        void insert_var_order(const uint32_t x, const branch type);
        void insert_var_order(const uint32_t x);
        void insert_var_order_all(const uint32_t x);
        vector<uint32_t> implied_by_learnts; //for glue-based extra var activity bumping
        template<bool inprocess>
        lbool new_decision();
        Lit pickBranchLit();
        uint32_t pick_var_vsids();
        void vsids_decay_var_act();
        template<bool inprocess> void vsids_bump_var_act(const uint32_t v);
        double backup_random_var_freq = -1; ///<if restart has full random var branch, we save old value here
        void check_var_in_branch_strategy(const uint32_t var, const branch str) const;
        void check_all_in_vmtf_branch_strategy(const vector<uint32_t>& vars);
        uint32_t branch_strategy_change = 0;
        uint32_t branch_strategy_at = 0;
        void setup_branch_strategy();
        void rebuildOrderHeap();
        void rebuildOrderHeapVMTF(vector<uint32_t>& vs);
        void print_order_heap();
        void clear_order_heap()
        {
            order_heap_vsids.clear();
            order_heap_rand.clear();
        }
        uint32_t branch_strategy_num = 0;
        void bump_var_importance(const uint32_t var);
        void bump_var_importance_all(const uint32_t var);

        /////////////////
        // Polarities
        bool   pick_polarity(const uint32_t var);
        void   setup_polarity_strategy();
        void   update_polarities_on_backtrack(const uint32_t btlevel);
        uint32_t polarity_strategy_at = 0;
        uint32_t polarity_strategy_change = 0;

        //Stats
        SearchStats stats;
        SearchHist hist;

    protected:
        Solver* solver;
        lbool search();

        // Distill
        uint64_t next_cls_distill = 0;
        lbool distill_clauses_if_needed();
        uint64_t next_bins_distill = 0;
        bool distill_bins_if_needed();

        // Str impl with impl
        uint64_t next_str_impl_with_impl = 0;
        bool str_impl_with_impl_if_needed();

        // Full Probe
        uint64_t next_full_probe = 0;
        uint64_t full_probe_iter = 0;
        lbool full_probe_if_needed();

        // sub-str with bin
        uint64_t next_sub_str_with_bin = 0;
        bool sub_str_with_bin_if_needed();

        // sub-str with bin
        uint64_t next_intree = 0;
        bool intree_if_needed();

        // SLS
        uint64_t next_sls = 0;
        void sls_if_needed();

        // Fast backward for Arjun
        lbool new_decision_fast_backw();
        void create_new_fast_backw_assumption();

        ///////////////
        // Variables
        ///////////////
        void new_var(
            const bool bva,
            const uint32_t orig_outer,
            const bool insert_varorder
        ) override;
        void new_vars(const size_t n) override;
        void save_on_var_memory();
        void updateVars(
            const vector<uint32_t>& outerToInter
            , const vector<uint32_t>& interToOuter
        );

        //Misc
        void add_in_partial_solving_stats();


        void fill_assumptions_set();
        void update_assump_conflict_to_orig_outside(vector<Lit>& out_conflict);

        /////////////////////
        // Learning
        /////////////////////
        vector<Lit> learnt_clause;
        vector<Lit> decision_clause;
        template<bool inprocess>
        void analyze_conflict(
            PropBy confl //The conflict that we are investigating
            , uint32_t& out_btlevel  //backtrack level
            , uint32_t &glue         //glue of the learnt clause
            , uint32_t &glue_before_minim     //glue of the unminimised learnt clause
            , uint32_t &size_before_minim     //size of the unminimised learnt clause
        );
        bool  handle_conflict(PropBy confl);// Handles the conflict clause
        void  update_history_stats(
            size_t backtrack_level,
            uint32_t glue,
            uint32_t connects_num_communities);
        template<bool inprocess>
        void  attach_and_enqueue_learnt_clause(
            Clause* cl,
            const uint32_t level,
            const bool enqueue,
            const uint64_t ID);
        void  print_learning_debug_info(const int32_t ID) const;
        void  print_learnt_clause() const;
        template<bool inprocess>
        void add_lits_to_learnt(const PropBy confl, const Lit p, uint32_t nDecisionLevel);
        template<bool inprocess>
        void create_learnt_clause(PropBy confl);
        void debug_print_resolving_clause(const PropBy confl) const;
        template<bool inprocess>
        void add_lit_to_learnt(Lit lit, const uint32_t nDecisionLevel);
        void analyze_final_confl_with_assumptions(const Lit p, vector<Lit>& out_conflict);
        void update_glue_from_analysis(Clause* cl);
        template<bool inprocess>
        void minimize_learnt_clause();
        void minimize_using_bins();
        void print_fully_minimized_learnt_clause() const;
        size_t find_backtrack_level_of_learnt();
        Clause* otf_subsume_last_resolved_clause(Clause* last_resolved_long_cl);
        void print_debug_resolution_data(const PropBy confl);
        int pathC;
        uint64_t more_red_minim_limit_binary_actual;
        #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
        AtecedentData<uint16_t> antec_data;
        #endif
        Clause* handle_last_confl(
            const uint32_t glue
            , const uint32_t old_decision_level
            , const uint32_t glue_before_minim
            , const uint32_t size_before_minim
            , const bool is_decision
            , const uint32_t connects_num_communities
            , int32_t& ID
        );

        /////////////////////
        // Search Stats
        /////////////////////
        const SearchStats& get_stats() const;
        size_t mem_used() const;
        void reset_temp_cl_num();
        void  resetStats(); //For connection with Solver
        double   startTime; ///<When solve() was started

        /////////////////////
        // Clause database reduction
        /////////////////////
        void reduce_db_if_needed();
        uint64_t next_lev1_reduce;
        uint64_t next_lev2_reduce;
        uint64_t next_pred_reduce;

        ///////////////
        // Restart parameters
        ///////////////
        struct SearchParams
        {
            SearchParams()
            {
                clear();
            }

            void clear()
            {
                needToStopSearch = false;
                conflictsDoneThisRestart = 0;
            }

            bool needToStopSearch;
            uint64_t conflictsDoneThisRestart;
            uint64_t max_confl_to_do;
            Restart rest_type = Restart::never;
        };
        SearchParams params;
        int64_t increasing_phase_size;
        int64_t max_confl_this_restart;
        void  check_need_restart();
        void  check_blocking_restart();
        bool blocked_restart = false;
        uint64_t max_confl_per_search_solve_call;
        uint32_t num_search_called = 0;
        double luby(double y, int x);
        CMSat::Restart cur_rest_type;
        uint32_t restart_strategy_change = 0;
        uint32_t restart_strategy_at = 0;
        void adjust_restart_strategy_cutoffs();
        void setup_restart_strategy(const bool force);

        ///////
        // GPU
        //////
        void find_largest_level(Lit* lits, uint32_t count, uint32_t start);
        vector<Lit> tmp_gpu_clause;
        PropBy learn_gpu_clause(Lit* lits, uint32_t count);

        //////////////
        // Debug
        //////////////
        void print_solution_varreplace_status() const;

        //////////////
        // Conflict minimisation
        bool litRedundant(Lit p, uint32_t abstract_levels);
        void recursiveConfClauseMin();
        void normalClMinim();
        MyStack<Lit> analyze_stack;
        uint32_t abstractLevel(const uint32_t x) const;
        bool subset(const vector<Lit>& A, const Clause& B); //Used for on-the-fly subsumption. Does A subsume B? Uses 'seen' to do its work
        void   minimise_redundant_more_more(vector<Lit>& cl);
        void   binary_based_morem_minim(vector<Lit>& cl);

        friend class Gaussian;
        friend class DistillerLong;
        #ifdef CMS_TESTING_ENABLED
        FRIEND_TEST(SearcherTest, pickpolar_rnd);
        FRIEND_TEST(SearcherTest, pickpolar_pos);
        FRIEND_TEST(SearcherTest, pickpolar_neg);
        FRIEND_TEST(SearcherTest, pickpolar_auto);
        FRIEND_TEST(SearcherTest, pickpolar_auto_not_changed_by_simp);
        #endif

        //Clause activites
        double cla_inc;
        template<bool inprocess> void decayClauseAct();

        //SQL
        void dump_search_sql(const double myTime);
        void set_clause_data(
            Clause* cl
            , const uint32_t glue
            , const uint32_t glue_before_minim
            , const uint32_t old_decision_level);
        #ifdef STATS_NEEDED
        PropStats lastSQLPropStats;
        SearchStats lastSQLGlobalStats;
        void dump_sql_clause_data(
            const uint32_t glue,
            const uint32_t size,
            const uint32_t glue_before_minim,
            const uint32_t size_before_minim,
            const uint32_t old_decision_level,
            const uint64_t clid,
            const bool decision_cl,
            const uint32_t connects_num_communities
        );
        int dump_this_many_cldata_in_stream = 0;
        void dump_var_for_learnt_cl(const uint32_t v,
                                    const uint64_t clid,
                                    const bool is_decision);
        #endif

        #if defined(STATS_NEEDED_BRANCH) || defined(FINAL_PREDICTOR_BRANCH)
        vector<uint32_t> level_used_for_cl;
        vector<uint32_t> vars_used_for_cl;
        vector<unsigned char> level_used_for_cl_arr;
        #endif

        //Other
        void print_solution_type(const lbool status) const;

        //Last time we clean()-ed the clauses, the number of zero-depth assigns was this many
        size_t   lastCleanZeroDepthAssigns;
};

inline uint32_t Searcher::abstractLevel(const uint32_t x) const
{
    return ((uint32_t)1) << (varData[x].level & 31);
}

inline const SearchStats& Searcher::get_stats() const
{
    return stats;
}

inline const SearchHist& Searcher::getHistory() const
{
    return hist;
}

inline void Searcher::add_in_partial_solving_stats()
{
    stats.cpu_time = cpuTime() - startTime;
}

inline void Searcher::insert_var_order(const uint32_t x)
{
    insert_var_order(x, branch_strategy);
}

inline void Searcher::insert_var_order(const uint32_t var, const branch type)
{
    #ifdef SLOW_DEUG
    assert(varData[x].removed == Removed::none
        && "All variables should be decision vars unless removed");
    #endif

    switch(type) {
        case branch::vsids:
            if (!order_heap_vsids.inHeap(var)) {
                order_heap_vsids.insert(var);
            }
            break;

        case branch::vmtf:
            // For VMTF we need to update the 'queue.unassigned' pointer in case this
            // variables sits after the variable to which 'queue.unassigned' currently
            // points.  See our SAT'15 paper for more details on this aspect.
            //
            VERBOSE_PRINT("vmtf Inserting back: " << var
                << " vmtf_queue.vmtf_bumped: " << vmtf_queue.vmtf_bumped
                << " vmtf_btab[var]: " << vmtf_btab[var]);

            if (vmtf_queue.vmtf_bumped < vmtf_btab[var]) {
                vmtf_update_queue_unassigned(var);
            }
            break;

        case branch::rand:
            if (!order_heap_rand.inHeap(var)) {
                order_heap_rand.insert(var);
            }
            break;
        default:
            assert(false);
            exit(-1);
            break;
    }

}

inline void Searcher::insert_var_order_all(const uint32_t x)
{
    assert(!order_heap_vsids.inHeap(x));
    SLOW_DEBUG_DO(assert(varData[x].removed == Removed::none &&
        "All variables should be decision vars unless removed"));
    order_heap_vsids.insert(x);

    assert(!order_heap_rand.inHeap(x));
    order_heap_rand.insert(x);

    vmtf_init_enqueue(x);
}

template<bool inprocess>
inline void Searcher::bump_cl_act(Clause* cl)
{
    if (inprocess)
        return;

    assert(!cl->getRemoved());

    double new_val = cla_inc + (double)cl->stats.activity;
    cl->stats.activity = (float)new_val;
    if (max_cl_act < new_val) {
        max_cl_act = new_val;
    }


    if (cl->stats.activity > 1e20F ) {
        // Rescale. For STATS_NEEDED we rescale ALL
        #if !defined(STATS_NEEDED) && !defined (FINAL_PREDICTOR)
        for(ClOffset offs: longRedCls[2]) {
            cl_alloc.ptr(offs)->stats.activity *= static_cast<float>(1e-20);
        }
        #else
        for(auto& lrcs: longRedCls) {
            for(ClOffset offs: lrcs) {
                cl_alloc.ptr(offs)->stats.activity *= static_cast<float>(1e-20);
            }
        }
        #endif
        cla_inc *= 1e-20;
        max_cl_act *= 1e-20;
        assert(cla_inc != 0);
    }
}

template<bool inprocess>
inline void Searcher::decayClauseAct()
{
    if (inprocess)
        return;

    cla_inc *= (1 / conf.clause_decay);
}

inline bool Searcher::pick_polarity(const uint32_t var)
{
    switch(polarity_mode) {
        case PolarityMode::polarmode_neg:
            return false;

        case PolarityMode::polarmode_pos:
            return true;

        case PolarityMode::polarmode_rnd:
            return mtrand.randInt(1);

        case PolarityMode::polarmode_automatic:
            assert(false);

        case PolarityMode::polarmode_stable:
            return varData[var].stable_polarity;

        case PolarityMode::polarmode_best_inv:
            return !varData[var].inv_polarity;

        case PolarityMode::polarmode_best:
            return varData[var].best_polarity;

        case PolarityMode::polarmode_saved:
            return varData[var].saved_polarity;

        #ifdef WEIGHTED_SAMPLING
        case PolarityMode::polarmode_weighted: {
            double rnd = mtrand.randDblExc();
            return rnd < varData[var].weight;
        }
        #endif

        default:
            assert(false);
    }

    return true;
}

template<bool inprocess>
inline void Searcher::vsids_bump_var_act(const uint32_t var)
{
    if (inprocess) return;
    var_act_vsids[var] += var_inc_vsids;
    max_vsids_act = std::max(max_vsids_act,  var_act_vsids[var]);

    #ifdef SLOW_DEBUG
    bool rescaled = false;
    #endif
    if (var_act_vsids[var] > 1e100) {
        SLOW_DEBUG_DO(rescaled = true);
        for (auto& v: var_act_vsids) v *= 1e-100;
        max_vsids_act *= 1e-100;
        var_inc_vsids *= 1e-100;
    }

    // Update order_heap with respect to new activity
    if (order_heap_vsids.inHeap(var)) {
        order_heap_vsids.decrease(var);
    }

    SLOW_DEBUG_DO(if (rescaled) assert(order_heap_vsids.heap_property()));
}

} //end namespace

#endif //__SEARCHER_H__
