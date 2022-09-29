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

#ifndef SOLVER_H
#define SOLVER_H

#include "constants.h"
#include <vector>
#include <fstream>
#include <iostream>
#include <utility>
#include <string>
#include <algorithm>

#include "constants.h"
#include "solvertypes.h"
#include "propengine.h"
#include "searcher.h"
#include "searchstats.h"
#ifdef CMS_TESTING_ENABLED
#include "gtest/gtest_prod.h"
#endif
#ifdef STATS_NEEDED
#include "satzilla_features.h"
#endif

namespace CMSat {

using std::vector;
using std::pair;
using std::string;

class VarReplacer;
class ClauseCleaner;
class OccSimplifier;
class SCCFinder;
class DistillerLong;
class DistillerBin;
class DistillerLitRem;
class DistillerLongWithImpl;
class StrImplWImpl;
class CalcDefPolars;
class SolutionExtender;
class CardFinder;
class SubsumeStrengthen;
class SubsumeImplicit;
class DataSync;
class SharedData;
class ReduceDB;
class InTree;
class BreakID;
class GetClauseQuery;

struct SolveStats
{
    uint32_t num_simplify = 0;
    uint32_t num_simplify_this_solve_call = 0;
    uint32_t num_solve_calls = 0;
};

class Solver : public Searcher
{
    public:
        Solver(const SolverConf *_conf = NULL,
               std::atomic<bool>* _must_interrupt_inter = NULL);
        virtual ~Solver() override;

        void add_sql_tag(const string& name, const string& val);
        const vector<std::pair<string, string> >& get_sql_tags() const;
        void new_external_var();
        void new_external_vars(size_t n);
        bool add_clause_outside(const vector<Lit>& lits);
        bool add_xor_clause_outside(const vector<uint32_t>& vars, bool rhs);
        bool add_bnn_clause_outside(
            const vector<Lit>& lits,
            const int32_t cutoff,
            Lit out);
        void set_var_weight(Lit lit, double weight);

        lbool solve_with_assumptions(
            const vector<Lit>* _assumptions = NULL,
            bool only_indep_solution = false);
        lbool simplify_with_assumptions(const vector<Lit>* _assumptions = NULL, const string* strategy = NULL);
        void  set_shared_data(SharedData* shared_data);
        vector<Lit> probe_inter_tmp;
        lbool probe_outside(Lit l, uint32_t& min_props);
        void set_max_confl(uint64_t max_confl);

        //frat for SAT problems
        void add_empty_cl_to_frat();

        //Querying model
        lbool model_value (const Lit p) const;  ///<Found model value for lit
        lbool model_value (const uint32_t p) const;  ///<Found model value for var
        lbool full_model_value (const Lit p) const;  ///<Found model value for lit
        lbool full_model_value (const uint32_t p) const;  ///<Found model value for var
        const vector<lbool>& get_model() const;
        const vector<Lit>& get_final_conflict() const;
        vector<double> get_vsids_scores() const;
        vector<Lit> implied_by_tmp_lits;
        bool implied_by(const std::vector<Lit>& lits,
            std::vector<Lit>& out_implied
        );

        //get clauses
        void start_getting_small_clauses(
            uint32_t max_len, uint32_t max_glue, bool red = true,
            bool bva_vars = false, bool simplified = false);
        bool get_next_small_clause(std::vector<Lit>& out, bool all_in_one);
        void end_getting_small_clauses();
        void get_all_irred_clauses(vector<Lit>& out);
        vector<uint32_t> translate_sampl_set(const vector<uint32_t>& sampl_set);

        //Version
        static const char* get_version_tag();
        static const char* get_version_sha1();
        static const char* get_compilation_env();

        vector<Lit> get_zero_assigned_lits(const bool backnumber = true, bool only_nvars = false) const;
        void     print_stats(
            const double cpu_time,
            const double cpu_time_total,
            double wallclock_time_started = 0) const;
        void     print_stats_time(
            const double cpu_time,
            const double cpu_time_total,
            double wallclock_time_started = 0) const;
        void     print_clause_stats() const;
        size_t get_num_free_vars() const;
        size_t get_num_nonfree_vars() const;
        const SolverConf& getConf() const;
        void setConf(const SolverConf& conf);
        const BinTriStats& getBinTriStats() const;
        size_t   get_num_long_irred_cls() const;
        size_t   get_num_long_red_cls() const;
        size_t get_num_vars_elimed() const;
        uint32_t num_active_vars() const;
        void print_mem_stats() const;
        uint64_t print_watch_mem_used(uint64_t totalMem) const;
        const SolveStats& get_solve_stats() const;
        const SearchStats& get_stats() const;
        void add_in_partial_solving_stats();
        void check_implicit_stats(const bool onlypairs = false) const;
        void check_stats(const bool allowFreed = false) const;
        void reset_vsids();


        //Checks
        void check_implicit_propagated() const;
        bool find_with_watchlist_a_or_b(Lit a, Lit b, int64_t* limit) const;

        //Systems that are used to accompilsh the tasks
        ClauseCleaner*         clauseCleaner = NULL;
        VarReplacer*           varReplacer = NULL;
        SubsumeImplicit*       subsumeImplicit = NULL;
        DataSync*              datasync = NULL;
        ReduceDB*              reduceDB = NULL;
        InTree*                intree = NULL;
        BreakID*               breakid = NULL;
        OccSimplifier*         occsimplifier = NULL;
        DistillerLong*         distill_long_cls = NULL;
        DistillerBin*          distill_bin_cls = NULL;
        DistillerLitRem*       distill_lit_rem = NULL;
        DistillerLongWithImpl* dist_long_with_impl = NULL;
        StrImplWImpl* dist_impl_with_impl = NULL;
        CardFinder*            card_finder = NULL;
        GetClauseQuery*        get_clause_query = NULL;

        SearchStats sumSearchStats;
        PropStats sumPropStats;

        bool prop_at_head() const;
        void set_decision_var(const uint32_t var);
        bool fully_enqueue_these(const vector<Lit>& toEnqueue);
        bool fully_enqueue_this(const Lit lit_ID);
        void update_assumptions_after_varreplace();

        //State load/unload
        template<typename A>
        void parse_v_line(A* in, const size_t lineNum);
        lbool load_solution_from_file(const string& fname);

        uint64_t getNumLongClauses() const;
        bool add_clause_outer_copylits(const vector<Lit>& ps);
        bool add_xor_clause_inter(
            const vector< Lit >& lits
            , bool rhs
            , bool attach
            , bool addDrat = true
            , bool red = false
        );
        void new_var(
            const bool bva = false,
            const uint32_t orig_outer = numeric_limits<uint32_t>::max(),
            const bool insert_varorder = true
        ) override;
        void new_vars(const size_t n) override;
        void bva_changed();

        //Attaching-detaching clauses
        void attachClause(
            const Clause& c
            #ifdef DEBUG_ATTACH
            , const bool checkAttach = true
            #else
            , const bool checkAttach = false
            #endif
        );
        void attach_bnn(const uint32_t bnn_idx);
        lbool bnn_eval(BNN& bnn);
        bool bnn_to_cnf(BNN& bnn);
        void attach_bin_clause(
            const Lit lit1
            , const Lit lit2
            , const bool red
            , const int32_t ID
            , [[maybe_unused]] const bool checkUnassignedFirst = true
        );
        void detach_bin_clause(
            Lit lit1
            , Lit lit2
            , bool red
            , const uint64_t ID
            , bool allow_empty_watch = false
            , bool allow_change_order = false
        ) {
            if (red) {
                binTri.redBins--;
            } else {
                binTri.irredBins--;
            }

            PropEngine::detach_bin_clause(lit1, lit2, red, ID, allow_empty_watch, allow_change_order);
        }
        void detachClause(const Clause& c, const bool removeDrat = true);
        void detachClause(const ClOffset offset, const bool removeDrat = true);
        void detach_modified_clause(
            const Lit lit1
            , const Lit lit2
            , const uint32_t origSize
            , const Clause* address
        );
        Clause* add_clause_int(
            const vector<Lit>& lits
            , const bool red = false
            , const ClauseStats* const stats = NULL
            , const bool attach = true
            , vector<Lit>* finalLits = NULL
            , bool addDrat = true
            , const Lit frat_first = lit_Undef
            , const bool sorted = false
            , const bool remove_frat = false
        );
        void add_bnn_clause_inter(
            vector<Lit>& lits,
            int32_t cutoff,
            Lit out
        );

        template<class T> vector<Lit> clause_outer_numbered(const T& cl) const;
        template<class T> vector<uint32_t> xor_outer_numbered(const T& cl) const;
        size_t mem_used() const;
        void dump_memory_stats_to_sql();
        void dump_clauses_at_finishup_as_last();
        void set_sqlite(const string filename);
        //Not Private for testing (maybe could be called from outside)
        bool renumber_variables(bool must_renumber = true);

        // Gates
        vector<OrGate> get_recovered_or_gates();
        vector<ITEGate> get_recovered_ite_gates();
        vector<pair<Lit, Lit> > get_all_binary_xors() const;
        vector<Xor> get_recovered_xors(const bool xor_together_xors);
        vector<uint32_t> remove_definable_by_irreg_gate(const vector<uint32_t>& vars);
        void find_equiv_subformula(
            vector<uint32_t>& sampl_vars, vector<uint32_t>& empty_vars, const bool mirror_empty);

        bool remove_and_clean_all();
        vector<Lit> get_toplevel_units_internal(bool outer_numbering) const;

        // Gauss-Jordan
        bool init_all_matrices();
        void detach_xor_clauses(
            const set<uint32_t>& clash_vars_unused
        );
        bool fully_undo_xor_detach();
        bool no_irred_nonxor_contains_clash_vars();
        bool assump_contains_xor_clash();
        void extend_model_to_detached_xors();
        void unset_clash_decision_vars(const vector<Xor>& xors);
        void set_clash_decision_vars();
        bool find_and_init_all_matrices();

        //assumptions
        void set_assumptions();
        vector<Lit> inter_assumptions_tmp; //used by set_assumptions() ONLY
        void add_assumption(const Lit assump);
        void check_assigns_for_assumptions() const;
        bool check_assumptions_contradict_foced_assignment() const;

        //Deleting clauses
        void free_cl(Clause* cl, bool also_remove_clid = true);
        void free_cl(ClOffset offs, bool also_remove_clid = true);
        #ifdef STATS_NEEDED
        void stats_del_cl(Clause* cl);
        void stats_del_cl(ClOffset offs);
        SatZillaFeatures calculate_satzilla_features();
        SatZillaFeatures last_solve_satzilla_feature;
        #endif

        //Helper
        void renumber_xors_to_outside(const vector<Xor>& xors, vector<Xor>& xors_ret);
        void testing_set_solver_not_fresh();
        bool full_probe(const bool bin_only);

    private:
        friend class ClauseDumper;
        #ifdef CMS_TESTING_ENABLED
        FRIEND_TEST(SearcherTest, pickpolar_auto_not_changed_by_simp);
        #endif

        //FRAT
        void write_final_frat_clauses();


        struct OracleBin {
            OracleBin (const Lit _l1, const Lit _l2, const int32_t _ID):
                l1(_l1), l2(_l2), ID(_ID) {}

            OracleBin() {}

            Lit l1;
            Lit l2;
            int32_t ID;
        };

        struct OracleDat {
            OracleDat(vector<int>& _val, ClOffset _off) :
                val(_val), off(_off) {which = 0;}
            OracleDat(vector<int>& _val, OracleBin _bin) :
                val(_val), bin(_bin) {which = 1;}


            vector<int> val;
            ClOffset off;
            OracleBin bin;
            int which;

            bool operator<(const OracleDat& other) const {
                return val < other.val;
            }
        };
        void dump_cls_oracle(const string fname, const vector<OracleDat>& cs);
        bool find_equivs();
        bool oracle_vivif(bool& finished);
        bool sparsify();
        template<bool bin_only> lbool probe_inter(const Lit l, uint32_t& min_props);
        void reset_for_solving();
        vector<Lit> add_clause_int_tmp_cl;
        lbool iterate_until_solved();
        uint64_t mem_used_vardata() const;
        uint64_t calc_num_confl_to_do_this_iter(const size_t iteration_num) const;

        bool sort_and_clean_clause(
            vector<Lit>& ps
            , const vector<Lit>& origCl
            , const bool red
            , const bool sorted = false
        );
        void sort_and_clean_bnn(BNN& bnn);
        void set_up_sql_writer();
        vector<std::pair<string, string> > sql_tags;

        void check_and_upd_config_parameters();
        vector<uint32_t> tmp_xor_clash_vars;
        void check_xor_cut_config_sanity() const;
        void handle_found_solution(const lbool status, const bool only_indep_solution);
        void add_every_combination_xor(const vector<Lit>& lits, bool attach, const bool addDrat, const bool red);
        void add_xor_clause_inter_cleaned_cut(const vector<Lit>& lits, bool attach, bool addDrat, const bool red);
        unsigned num_bits_set(const size_t x, const unsigned max_size) const;
        void check_too_large_variable_number(const vector<Lit>& lits) const;

        lbool simplify_problem_outside(const string* strategy = NULL);
        void move_to_outside_assumps(const vector<Lit>* assumps);
        vector<Lit> back_number_from_outside_to_outer_tmp;
        void back_number_from_outside_to_outer(const vector<Lit>& lits)
        {
            back_number_from_outside_to_outer_tmp.clear();
            for (const Lit lit: lits) {
                assert(lit.var() < nVarsOutside());
                if (get_num_bva_vars() > 0 || !fresh_solver) {
                    back_number_from_outside_to_outer_tmp.push_back(map_to_with_bva(lit));
                    assert(back_number_from_outside_to_outer_tmp.back().var() < nVarsOuter());
                } else {
                    back_number_from_outside_to_outer_tmp.push_back(lit);
                }
            }
        }
        vector<Lit> outside_assumptions;
        Lit back_number_from_outside_to_outer(const Lit lit)
        {
            assert(lit.var() < nVarsOutside());
            if (get_num_bva_vars() > 0 || !fresh_solver) {
                Lit ret = map_to_with_bva(lit);
                assert(ret.var() < nVarsOuter());
                return ret;
            } else {
                return lit;
            }
        }

        uint32_t back_number_from_outside_to_outer(const uint32_t var)
        {
            Lit lit = Lit(var, false);
            return back_number_from_outside_to_outer(lit).var();
        }


        //Stats printing
        void print_norm_stats(
            const double cpu_time,
            const double cpu_time_total,
            const double wallclock_time_started=0) const;
        void print_full_stats(
            const double cpu_time,
            const double cpu_time_total,
            const double wallclock_time_started=0) const;

        lbool simplify_problem(const bool startup, const string& strategy);
        lbool execute_inprocess_strategy(const bool startup, const string& strategy);
        SolveStats solveStats;
        void check_minimization_effectiveness(lbool status);
        void check_recursive_minimization_effectiveness(const lbool status);
        void extend_solution(const bool only_indep_solution);
        void check_too_many_in_tier0();
        bool adjusted_glue_cutoff_if_too_many = false;

        /////////////////////////////
        // Temporary datastructs -- must be cleared before use
        mutable std::vector<Lit> tmpCl;
        mutable std::vector<uint32_t> tmpXor;

        /////////////////////////////
        //Renumberer
        double calc_renumber_saving();
        void free_unused_watches();
        uint64_t last_full_watch_consolidate = 0;
        void save_on_var_memory(uint32_t newNumVars);
        void unSaveVarMem();
        size_t calculate_interToOuter_and_outerToInter(
            vector<uint32_t>& outerToInter
            , vector<uint32_t>& interToOuter
        );
        void renumber_clauses(const vector<uint32_t>& outerToInter);
        void test_renumbering() const;
        bool clean_xor_clauses_from_duplicate_and_set_vars();
        bool update_vars_of_xors(vector<Xor>& xors);

        /////////////////////////////
        // SAT solution verification
        bool verify_model() const;
        bool verify_model_implicit_clauses() const;
        bool verify_model_long_clauses(const vector<ClOffset>& cs) const;


        /////////////////////
        // Data
        size_t               zeroLevAssignsByCNF = 0;
        struct GivenW {
            bool pos = false;
            bool neg = false;
        };
        vector<GivenW> weights_given;

        /////////////////////
        // Clauses
        bool addClauseHelper(vector<Lit>& ps);
        bool add_clause_outer(vector<Lit>& ps);

        /////////////////
        // Debug

        void print_watch_list(watch_subarray_const ws, const Lit lit) const;
        void print_clause_size_distrib();
        void check_model_for_assumptions() const;
};

inline void Solver::set_decision_var(const uint32_t var)
{
    insert_var_order_all(var);
}

inline uint64_t Solver::getNumLongClauses() const
{
    return longIrredCls.size() + longRedCls.size();
}

inline const SearchStats& Solver::get_stats() const
{
    return sumSearchStats;
}

inline const SolveStats& Solver::get_solve_stats() const
{
    return solveStats;
}

inline size_t Solver::get_num_long_irred_cls() const
{
    return longIrredCls.size();
}

inline size_t Solver::get_num_long_red_cls() const
{
    return longRedCls.size();
}

inline const SolverConf& Solver::getConf() const
{
    return conf;
}

inline const vector<std::pair<string, string> >& Solver::get_sql_tags() const
{
    return sql_tags;
}

inline const BinTriStats& Solver::getBinTriStats() const
{
    return binTri;
}

template<class T>
inline vector<Lit> Solver::clause_outer_numbered(const T& cl) const
{
    tmpCl.clear();
    for(size_t i = 0; i < cl.size(); i++) {
        tmpCl.push_back(map_inter_to_outer(cl[i]));
    }

    return tmpCl;
}

template<class T>
inline vector<uint32_t> Solver::xor_outer_numbered(const T& cl) const
{
    tmpXor.clear();
    for(size_t i = 0; i < cl.size(); i++) {
        tmpXor.push_back(map_inter_to_outer(cl[i]));
    }

    return tmpXor;
}

inline void Solver::move_to_outside_assumps(const vector<Lit>* assumps)
{

    if (assumps) {
        #ifdef SLOW_DEBUG
        outside_assumptions.clear();
        for(const Lit lit: *assumps) {
            if (lit.var() >= nVarsOutside()) {
                std::cerr << "ERROR: Assumption variable " << (lit.var()+1)
                << " is too large, you never"
                << " inserted that variable into the solver. Exiting."
                << endl;
                exit(-1);
            }
            outside_assumptions.push_back(lit);
        }
        #else
        outside_assumptions.resize(assumps->size());
        std::copy(assumps->begin(), assumps->end(), outside_assumptions.begin());
        #endif
    } else {
        outside_assumptions.clear();
    }
}

inline lbool Solver::simplify_with_assumptions(
    const vector<Lit>* _assumptions,
    const string* strategy
) {
    fresh_solver = false;
    move_to_outside_assumps(_assumptions);
    return simplify_problem_outside(strategy);
}

inline bool Solver::find_with_watchlist_a_or_b(Lit a, Lit b, int64_t* limit) const
{
    if (watches[a].size() > watches[b].size()) {
        std::swap(a,b);
    }

    watch_subarray_const ws = watches[a];
    *limit -= ws.size();
    for (const Watched w: ws) {
        if (!w.isBin())
            continue;

        if (!w.red()
            && w.lit2() == b
        ) {
            return true;
        }
    }

    return false;
}

inline const vector<lbool>& Solver::get_model() const
{
    return model;
}

inline const vector<Lit>& Solver::get_final_conflict() const
{
    return conflict;
}

inline void Solver::setConf(const SolverConf& _conf)
{
    conf = _conf;
}

inline bool Solver::prop_at_head() const
{
    return qhead == trail.size();
}

inline lbool Solver::model_value (const Lit p) const
{
    if (model[p.var()] == l_Undef)
        return l_Undef;

    return model[p.var()] ^ p.sign();
}

inline lbool Solver::model_value (const uint32_t p) const
{
    return model[p];
}

inline void Solver::testing_set_solver_not_fresh()
{
    fresh_solver = false;
}

inline void Solver::free_cl(
    Clause* cl,
    bool
    #ifdef STATS_NEEDED
    also_remove_clid
    #endif
) {
    #ifdef STATS_NEEDED
    if (also_remove_clid) {
        stats_del_cl(cl);
    }
    #endif
    cl_alloc.clauseFree(cl);
}

inline void Solver::free_cl(
    ClOffset offs,
    bool
    #ifdef STATS_NEEDED
    also_remove_clid
    #endif
) {
    #ifdef STATS_NEEDED
    if (also_remove_clid) {
        stats_del_cl(offs);
    }
    #endif
    cl_alloc.clauseFree(offs);
}

} //end namespace

#endif //SOLVER_H
