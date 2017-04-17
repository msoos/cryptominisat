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

#ifndef __SEARCHER_H__
#define __SEARCHER_H__

#include <array>

#include "propengine.h"
#include "solvertypes.h"
#include "time_mem.h"
#include "hyperengine.h"
#include "minisat_rnd.h"
#include "simplefile.h"
#include "searchstats.h"

namespace CMSat {

class Solver;
class SQLStats;
class VarReplacer;
class Gaussian;

using std::string;
using std::cout;
using std::endl;

struct OTFClause
{
    Lit lits[3];
    unsigned size;
};

struct VariableVariance
{
    double avgDecLevelVarLT = 0;
    double avgTrailLevelVarLT= 0;
    double avgDecLevelVar = 0;
    double avgTrailLevelVar = 0;
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
            uint64_t maxConfls
            , const unsigned upper_level_iteration_num
        );
        void finish_up_solve(lbool status);
        void reduce_db_if_needed();
        bool clean_clauses_if_needed();
        lbool perform_scc_and_varreplace_if_needed();
        void dump_search_loop_stats();
        bool must_abort(lbool status);
        void print_search_loop_num();
        uint64_t loop_num;
        MiniSatRnd mtrand; ///< random number generator


        vector<lbool>  model;
        vector<lbool>  full_model;
        vector<Lit>   conflict;     ///<If problem is unsatisfiable (possibly under assumptions), this vector represent the final conflict clause expressed in the assumptions.
        template<bool update_bogoprops>
        PropBy propagate();

        ///////////////////////////////
        // Stats
        //Restart print status
        uint64_t lastRestartPrint = 0;
        uint64_t lastRestartPrintHeader = 0;
        void     print_restart_stat();
        void     print_iteration_solving_stats();
        void     print_restart_header() const;
        void     print_restart_stat_line() const;
        void     printBaseStats() const;
        void     print_clause_stats() const;
        uint64_t sumRestarts() const;
        const SearchHist& getHistory() const;

        size_t hyper_bin_res_all(const bool check_for_set_values = true);
        std::pair<size_t, size_t> remove_useless_bins(bool except_marked = false);
        bool var_inside_assumptions(const uint32_t var) const
        {
            return assumptionsSet.at(var);
        }
        template<bool also_insert_var_order = true>
        void cancelUntil(uint32_t level); ///<Backtrack until a certain level.
        bool check_order_heap_sanity() const;

        //Gauss
        vector<Gaussian*> gauss_matrixes;
        SQLStats* sqlStats = NULL;
        void consolidate_watches();
        #ifdef USE_GAUSS
        void clear_gauss();
        #else
        void clear_gauss() {}
        #endif

        void testing_fill_assumptions_set()
        {
            assumptionsSet.clear();
            assumptionsSet.resize(nVars(), false);
        }
        double get_cla_inc() const
        {
            return cla_inc;
        }

        //Needed for tests around renumbering
        void rebuildOrderHeap();
        void clear_order_heap()
        {
            order_heap_vsids.clear();
            order_heap_maple.clear();
        }
        void bumpClauseAct(Clause* cl);

    protected:
        void new_var(const bool bva, const uint32_t orig_outer) override;
        void new_vars(const size_t n) override;
        void save_on_var_memory();
        void reset_temp_cl_num();
        void updateVars(
            const vector<uint32_t>& outerToInter
            , const vector<uint32_t>& interToOuter
        );
        void save_state(SimpleOutFile& f, const lbool status) const;
        void load_state(SimpleInFile& f, const lbool status);
        void write_long_cls(
            const vector<ClOffset>& clauses
            , SimpleOutFile& f
            , const bool red
        ) const;
        void read_long_cls(
            SimpleInFile& f
            , const bool red
        );
        uint64_t read_binary_cls(
            SimpleInFile& f
            , bool red
        );
        void write_binary_cls(
            SimpleOutFile& f
            , bool red
        ) const;

        //Misc
        void update_var_decay();
        void add_in_partial_solving_stats();


        struct AssumptionPair {
            AssumptionPair(const Lit _inter, const Lit _outer):
                lit_inter(_inter)
                , lit_orig_outside(_outer)
            {
            }

            Lit lit_inter;
            Lit lit_orig_outside; //not outer, but outside(!)

            bool operator<(const AssumptionPair& other) const
            {
                //Yes, we need reverse in terms of inverseness
                return ~lit_inter < ~other.lit_inter;
            }
        };
        void fill_assumptions_set_from(const vector<AssumptionPair>& fill_from);
        void unfill_assumptions_set_from(const vector<AssumptionPair>& unfill_from);
        void renumber_assumptions(const vector<uint32_t>& outerToInter);
        //we cannot eliminate / component-handle such vars
        //Needed so checking is fast
        vector<char> assumptionsSet;

        //Note that this array can have the same internal variable more than
        //once, in case one has been replaced with the other. So if var 1 =  var 2
        //and var 1 was set to TRUE and var 2 to be FALSE, then we'll have var 1
        //insided this array twice, once it needs to be set to TRUE and once FALSE
        vector<AssumptionPair> assumptions;

        void update_assump_conflict_to_orig_outside(vector<Lit>& out_conflict);


        //For connection with Solver
        void  resetStats();

        SearchHist hist;

        /////////////////
        //Settings
        Solver*   solver;          ///< Thread control class

        /////////////////
        // Searching
        /// Search for a given number of conflicts.
        template<bool update_bogoprops>
        lbool search();
        lbool burst_search();
        template<bool update_bogoprops>
        bool  handle_conflict(PropBy confl);// Handles the conflict clause
        void  update_history_stats(size_t backtrack_level, uint32_t glue);
        void  attach_and_enqueue_learnt_clause(Clause* cl, bool enq = true);
        void  print_learning_debug_info() const;
        void  print_learnt_clause() const;
        void  add_otf_subsume_long_clauses();
        void  add_otf_subsume_implicit_clause();
        Clause* handle_last_confl_otf_subsumption(
            Clause* cl
            , const uint32_t glue
            , const uint32_t backtrack_level
        );
        lbool new_decision();  // Handles the case when decision must be made
        void  check_need_restart();     // Helper function to decide if we need to restart during search
        Lit   pickBranchLit();

        ///////////////
        // Conflicting
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
        vector<Lit> learnt_clause;
        vector<Lit> decision_clause;
        template<bool update_bogoprops>
        Clause* analyze_conflict(
            PropBy confl //The conflict that we are investigating
            , uint32_t& out_btlevel      //backtrack level
            , uint32_t &glue         //glue of the learnt clause
        );
        void update_clause_glue_from_analysis(Clause* cl);
        template<bool update_bogoprops>
        void minimize_learnt_clause();
        void watch_based_learnt_minim();
        void minimize_using_permdiff();
        void print_fully_minimized_learnt_clause() const;
        size_t find_backtrack_level_of_learnt();
        template<bool update_bogoprops>
        void bump_var_activities_based_on_implied_by_learnts(const uint32_t glue);
        Clause* otf_subsume_last_resolved_clause(Clause* last_resolved_long_cl);
        void print_debug_resolution_data(const PropBy confl);
        template<bool update_bogoprops>
        Clause* create_learnt_clause(PropBy confl);
        int pathC;
        AtecedentData<uint16_t> antec_data;

        vector<std::pair<uint32_t, uint32_t> > implied_by_learnts; //for glue-based extra var activity bumping

        /////////////////
        //Graphical conflict generation
        void   create_graphviz_confl_graph     (PropBy conflPart);
        string analyze_confl_for_graphviz_graph (PropBy conflHalf, uint32_t& out_btlevel, uint32_t &glue);
        void print_edges_for_graphviz_file(std::ofstream& file) const;
        void print_vertex_definitions_for_graphviz_file(std::ofstream& file);
        void fill_seen_for_lits_connected_to_conflict_graph(
            vector<Lit>& lits
        );
        vector<Lit> get_lits_from_conflict(const PropBy conflPart);



        /////////////////
        // Variable activity
        double var_inc;
        void insert_var_order(const uint32_t x);  ///< Insert a variable in current heap
        void insert_var_order_all(const uint32_t x);  ///< Insert a variable in all heaps


        uint64_t more_red_minim_limit_binary_actual;
        uint64_t more_red_minim_limit_cache_actual;
        const SearchStats& get_stats() const;
        size_t mem_used() const;

        int64_t max_confl_phase;
        int64_t max_confl_this_phase;

        uint64_t next_lev1_reduce;
        uint64_t next_lev2_reduce;

    private:
        //////////////
        // Conflict minimisation
        bool litRedundant(Lit p, uint32_t abstract_levels);
        void recursiveConfClauseMin();
        void normalClMinim();
        MyStack<Lit> analyze_stack;
        uint32_t        abstractLevel(const uint32_t x) const;

        //OTF subsumption during learning
        vector<ClOffset> otf_subsuming_long_cls;
        vector<OTFClause> otf_subsuming_short_cls;
        void check_otf_subsume(const ClOffset offset, Clause& cl);
        void create_otf_subsuming_implicit_clause(const Clause& cl);
        void create_otf_subsuming_long_clause(Clause& cl, ClOffset offset);
        template<bool update_bogoprops>
        Clause* add_literals_from_confl_to_learnt(const PropBy confl, const Lit p);
        void debug_print_resolving_clause(const PropBy confl) const;
        template<bool update_bogoprops>
        void add_lit_to_learnt(Lit lit);
        void analyze_final_confl_with_assumptions(const Lit p, vector<Lit>& out_conflict);
        size_t tmp_learnt_clause_size;
        cl_abst_type tmp_learnt_clause_abst;

        //Restarts
        uint64_t max_confl_per_search_solve_call;
        bool blocked_restart = false;
        void check_blocking_restart();
        uint32_t num_search_called = 0;
        uint64_t lastRestartConfl;
        double luby(double y, int x);
        void adjust_phases_restarts();

        void print_solution_varreplace_status() const;
        void dump_search_sql(const double myTime);

        ////////////
        // Transitive on-the-fly self-subsuming resolution
        void   minimise_redundant_more(vector<Lit>& cl);
        void   binary_based_more_minim(vector<Lit>& cl);
        void   cache_based_more_minim(vector<Lit>& cl);
        void   stamp_based_more_minim(vector<Lit>& cl);

        //Variable activities
        struct VarFilter { ///Filter out vars that have been set or is not decision from heap
            const Searcher* cc;
            const Solver* solver;
            VarFilter(const Searcher* _cc, Solver* _solver) :
                cc(_cc)
                ,solver(_solver)
            {}
            bool operator()(uint32_t var) const;
        };
        friend class Gaussian;

        ///Decay all variables with the specified factor. Implemented by increasing the 'bump' value instead.
        void     varDecayActivity ();
        ///Increase a variable with the current 'bump' value.
        template<bool update_bogoprops>
        void     bump_vsids_var_act(uint32_t v, double mult = 1.0);

        //Clause activites
        double cla_inc;
        void decayClauseAct();
        unsigned guess_clause_array(
            const uint32_t glue
            , const uint32_t backtrack_lev
            , const double vsids_cutoff
            , double backtrack_cutoff = 0.2
            , const double offset_percent = 0.0
            , bool count_antec_glue_long_reds = false
        ) const;

        //SQL
        #ifdef STATS_NEEDED
        void dump_restart_sql();
        PropStats lastSQLPropStats;
        SearchStats lastSQLGlobalStats;
        void dump_sql_clause_data(
            const uint32_t glue
            , const uint32_t backtrack_level
        );
        #endif


        //Other
        void print_solution_type(const lbool status) const;
        void clearGaussMatrixes();

        //Picking polarity when doing decision
        bool     pickPolarity(const uint32_t var);

        //Last time we clean()-ed the clauses, the number of zero-depth assigns was this many
        size_t   lastCleanZeroDepthAssigns;

        //Used for on-the-fly subsumption. Does A subsume B?
        //Uses 'seen' to do its work
        bool subset(const vector<Lit>& A, const Clause& B);

        double   startTime; ///<When solve() was started
        SearchStats stats;
        double   var_decay;
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
    Heap<VarOrderLt> &order_heap = VSIDS ? order_heap_vsids : order_heap_maple;
    if (!order_heap.inHeap(x)) {
        #ifdef SLOW_DEUG
        assert(varData[x].removed == Removed::none
            && "All variables should be decision vars unless removed");
        #endif

        order_heap.insert(x);
    }
}

inline void Searcher::insert_var_order_all(const uint32_t x)
{
    if (!order_heap_vsids.inHeap(x)) {
        #ifdef SLOW_DEUG
        assert(varData[x].removed == Removed::none
            && "All variables should be decision vars unless removed");
        #endif

        order_heap_vsids.insert(x);
    }
    if (!order_heap_maple.inHeap(x)) {
        #ifdef SLOW_DEUG
        assert(varData[x].removed == Removed::none
            && "All variables should be decision vars unless removed");
        #endif

        order_heap_maple.insert(x);
    }
}

inline void Searcher::bumpClauseAct(Clause* cl)
{
    assert(!cl->getRemoved());

    cl->stats.activity += cla_inc;
    if (cl->stats.activity > 1e20 ) {
        // Rescale
        for(ClOffset offs: longRedCls[2]) {
            cl_alloc.ptr(offs)->stats.activity *= 1e-20;
        }
        cla_inc *= 1e-20;
    }
}

inline void Searcher::decayClauseAct()
{
    cla_inc *= (1 / conf.clause_decay);
}

inline bool Searcher::check_order_heap_sanity() const
{
    for(size_t i = 0; i < nVars(); i++)
    {
        if (varData[i].removed == Removed::none
            && value(i) == l_Undef)
        {
            if (!order_heap_vsids.inHeap(i)) {
                cout << "ERROR var " << i+1 << " not in VSIDS heap."
                << " value: " << value(i)
                << " removed: " << removed_type_to_string(varData[i].removed)
                << endl;
                return false;
            }
            if (!order_heap_maple.inHeap(i)) {
                cout << "ERROR var " << i+1 << " not in !VSIDS heap."
                << " value: " << value(i)
                << " removed: " << removed_type_to_string(varData[i].removed)
                << endl;
                return false;
            }
        }
    }
    assert(order_heap_vsids.heap_property());
    assert(order_heap_maple.heap_property());

    return true;
}

inline bool Searcher::pickPolarity(const uint32_t var)
{
    switch(conf.polarity_mode) {
        case PolarityMode::polarmode_neg:
            return false;

        case PolarityMode::polarmode_pos:
            return true;

        case PolarityMode::polarmode_rnd:
            return mtrand.randInt(1);

        case PolarityMode::polarmode_automatic:
            return varData[var].polarity;

        default:
            assert(false);
    }

    return true;
}

template<bool update_bogoprops>
void Searcher::bump_var_activities_based_on_implied_by_learnts(
    const uint32_t glue
) {
    assert(!update_bogoprops);

    for (const auto dat :implied_by_learnts) {
        const uint32_t v_glue = dat.second;
        if (v_glue < glue) {
            bump_vsids_var_act<update_bogoprops>(dat.first);
        }
    }
}

template<bool update_bogoprops>
inline void Searcher::bump_vsids_var_act(uint32_t var, double mult)
{
    if (update_bogoprops) {
        return;
    }

    var_act_vsids[var] += var_inc * mult;

    #ifdef SLOW_DEBUG
    bool rescaled = false;
    #endif
    if (var_act_vsids[var] > 1e100) {
        // Rescale:
        for (double& act : var_act_vsids) {
            act *= 1e-100;
        }
        #ifdef SLOW_DEBUG
        rescaled = true;
        #endif

        //Reset var_inc
        var_inc *= 1e-100;
    }

    // Update order_heap with respect to new activity:
    if (order_heap_vsids.inHeap(var)) {
        order_heap_vsids.decrease(var);
    }

    #ifdef SLOW_DEBUG
    if (rescaled) {
        assert(order_heap_vsids.heap_property());
    }
    #endif
}


} //end namespace

#endif //__SEARCHER_H__
