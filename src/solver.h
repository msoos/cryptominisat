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

#include "constants.h"
#include <vector>
#include <fstream>
#include <iostream>
#include <array>
#include <utility>
#include <string>
#include <algorithm>

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
#define ORACLE_DAT_SIZE 4

namespace CMSat {
    struct SATSolver;
}
using std::vector;
using std::pair;
using std::string;
using std::array;
struct PicoSAT;

namespace CMSat {

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
        Solver(const SolverConf *_conf = nullptr,
               std::atomic<bool>* _must_interrupt_inter = nullptr);
        virtual ~Solver() override;

        void add_sql_tag(const string& name, const string& val);
        const vector<std::pair<string, string> >& get_sql_tags() const;
        void new_external_var();
        void new_external_vars(size_t n);
        bool add_clause_outside(const vector<Lit>& lits, bool red = false, bool restore = false);
        bool add_xor_clause_outside(const vector<uint32_t>& vars, const bool rhs);
        bool add_xor_clause_outside(const vector<Lit>& lits_out, bool rhs);
        bool add_bnn_clause_outside(
            const vector<Lit>& lits,
            const int32_t cutoff,
            Lit out);
        void set_lit_weight(Lit lit, double weight);
        void get_weights(map<Lit,double>& weights,
                const vector<uint32_t>& sampling_vars,
                const vector<uint32_t>& orig_sampl_vars) const;

        lbool solve_with_assumptions(
            const vector<Lit>* _assumptions = nullptr,
            bool only_indep_solution = false);
        lbool simplify_with_assumptions(const vector<Lit>* _assumptions = nullptr, const string* strategy = nullptr);
        void  set_shared_data(SharedData* shared_data);
        vector<Lit> probe_inter_tmp;
        lbool probe_outside(Lit l, uint32_t& min_props);
        void set_max_confl(uint64_t max_confl);
        //frat for SAT problems
        void add_empty_cl_to_frat();
        void conclude_idrup (lbool);
        void changed_sampling_vars();

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
        void start_getting_constraints(
               bool red, // only redundant, otherwise only irred
               bool simplified = false,
               uint32_t max_len = std::numeric_limits<uint32_t>::max(),
               uint32_t max_glue = std::numeric_limits<uint32_t>::max());
        bool get_next_constraint(std::vector<Lit>& ret, bool& is_xor, bool& rhs);
        void end_getting_constraints();
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
        size_t get_num_vars_elimed() const;
        uint32_t num_active_vars() const;
        void print_mem_stats() const;
        uint64_t print_watch_mem_used(uint64_t total_mem) const;
        const SolveStats& get_solve_stats() const;
        const SearchStats& get_stats() const;
        void add_in_partial_solving_stats();
        bool check_xor_clause_satisfied_model(const Xor& x) const;
        void check_implicit_stats(const bool onlypairs = false) const;
        void check_implicit_propagated() const;
        void check_all_clause_propagated() const;
        void check_clause_propagated(const ClOffset& offs) const;
        void check_clause_propagated(const Xor& x) const;
        void check_stats(const bool allowFreed = false) const;
        void reset_vsids();
        bool minimize_clause(vector<Lit>& cl);

        //Checks

        //Systems that are used to accompilsh the tasks
        ClauseCleaner*         clauseCleaner = nullptr;
        VarReplacer*           varReplacer = nullptr;
        SubsumeImplicit*       subsumeImplicit = nullptr;
        DataSync*              datasync = nullptr;
        ReduceDB*              reduceDB = nullptr;
        InTree*                intree = nullptr;
        BreakID*               breakid = nullptr;
        OccSimplifier*         occsimplifier = nullptr;
        DistillerLong*         distill_long_cls = nullptr;
        DistillerBin*          distill_bin_cls = nullptr;
        DistillerLitRem*       distill_lit_rem = nullptr;
        DistillerLongWithImpl* dist_long_with_impl = nullptr;
        StrImplWImpl* dist_impl_with_impl = nullptr;
        CardFinder*            card_finder = nullptr;
        GetClauseQuery*        get_clause_query = nullptr;

        SearchStats sumSearchStats;
        PropStats sumPropStats;

        bool prop_at_head() const;
        void set_decision_var(const uint32_t var);
        bool fully_enqueue_these(const vector<Lit>& toEnqueue);
        bool fully_enqueue_this(const Lit lit_ID);

        //State load/unload
        string serialize_solution_reconstruction_data() const;
        void create_from_solution_reconstruction_data(const string& str);
        pair<lbool, vector<lbool>> extend_minimized_model(const vector<lbool>& m);

        // Clauses
        bool add_xor_clause_inter(
            const vector< Lit >& lits
            , bool rhs
            , bool attach
            , int32_t XID
        );
        void new_var(
            const bool bva = false,
            const uint32_t orig_outer = numeric_limits<uint32_t>::max(),
            const bool insert_varorder = true
        ) override;
        void new_vars(const size_t n) override;

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
        void detachClause(const Clause& c, const bool remove_frat = true);
        void detachClause(const ClOffset offset, const bool remove_frat = true);
        void detach_modified_clause(
            const Lit lit1
            , const Lit lit2
            , const uint32_t origSize
            , const Clause* address
        );
        void add_clause_int_frat(const vector<Lit>& cl, const uint32_t ID);
        Clause* add_clause_int(
            const vector<Lit>& lits
            , const bool red = false
            , const ClauseStats* const stats = nullptr
            , const bool attach = true
            , vector<Lit>* finalLits = nullptr
            , bool addFrat = true
            , const Lit frat_first = lit_Undef
            , const bool sorted = false
            , const bool remove_frat = false
        );
        void add_bnn_clause_inter(
            vector<Lit>& lits,
            int32_t cutoff,
            Lit out
        );

        lbool bnn_eval(BNN& bnn);
        bool bnn_to_cnf(BNN& bnn);
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
        vector<uint32_t> remove_definable_by_irreg_gate(const vector<uint32_t>& vars);
        void get_empties(vector<uint32_t>& sampl_vars, vector<uint32_t>& empty_vars);

        bool remove_and_clean_all();
        bool remove_and_clean_detached_xors(vector<Xor>& xors);
        vector<Lit> get_toplevel_units_internal(bool outer_numbering) const;

        // Gauss-Jordan
        vector<Xor> get_recovered_xors();
        bool init_all_matrices();
        bool find_and_init_all_matrices();
        void detach_clauses_in_xors();
        vector<Lit> tmp_repr;
        bool check_clause_represented_by_xor(const Clause& cl);
        void hash_uint32_t(const uint32_t v, uint32_t& hash) const {
            uint8_t* s = (uint8_t*)(&v);
            for(uint32_t i = 0; i < 4; i++, s++) { hash += *s; }
            s = (uint8_t*)(&v);
            for(uint32_t i = 0; i < 4; i++, s++) { hash ^= *s; }
        }

        uint32_t hash_xcl(const Xor& x) const {
            uint32_t hash = 0;
            for(const auto& v: x) hash_uint32_t(v, hash);
            return hash;
        }

        uint32_t hash_xcl(const Clause* cl) const {
            uint32_t hash = 0;
            for(const auto& l: *cl) hash_uint32_t(l.var(), hash);
            return hash;
        }


        //assumptions
        void set_assumptions();
        void add_assumption(const Lit assump);
        void check_assigns_for_assumptions() const;
        bool check_assumptions_contradict_foced_assignment() const;
        void uneliminate_sampling_set();

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
        bool full_probe(const bool bin_only);

        int PICOLIT(const Lit x) { return ((((int)(x).var()+1)) * ((x).sign() ? -1:1)); }
        PicoSAT* build_picosat();
        void copy_to_simp(SATSolver* s2);
        bool backbone_simpl(int64_t max_confl, bool& finished);
        bool removed_var_ext(uint32_t var) const;

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
            OracleDat(array<int, ORACLE_DAT_SIZE>& _val, ClOffset _off) :
                val(_val), off(_off) {binary = 0;}
            OracleDat(array<int, ORACLE_DAT_SIZE>& _val, OracleBin _bin) :
                val(_val), bin(_bin) {binary = 1;}

            array<int, ORACLE_DAT_SIZE> val;
            ClOffset off;
            OracleBin bin;
            int binary;

            bool operator<(const OracleDat& other) const {
                return val < other.val;
            }
        };

        vector<vector<int>> get_irred_cls_for_oracle() const;
        vector<vector<uint16_t>> compute_edge_weights() const;
        vector<OracleDat> order_clauses_for_oracle() const;
        void dump_cls_oracle(const string fname, const vector<OracleDat>& cs);
        bool find_equivs();
        bool oracle_vivif(bool& finished);
        bool oracle_sparsify();
        void print_cs_ordering(const vector<OracleDat>& cs) const;
        template<bool bin_only> bool probe_inter(const Lit l, uint32_t& min_props);
        void reset_for_solving();
        vector<Lit> add_clause_int_tmp_cl;
        lbool iterate_until_solved();
        uint64_t mem_used_vardata() const;
        uint64_t calc_num_confl_to_do_this_iter(const size_t iteration_num) const;
        void detach_and_free_all_irred_cls();

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
        void copy_assumptions(const vector<Lit>* assumps);
        void handle_found_solution(const lbool status, const bool only_indep_solution);
        unsigned num_bits_set(const size_t x, const unsigned max_size) const;
        void check_too_large_variable_number(const vector<Lit>& lits) const;
        lbool simplify_problem_outside(const string* strategy = nullptr);

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
        size_t calculate_inter_to_outer_and_outer_to_inter(
            vector<uint32_t>& outer_to_inter
            , vector<uint32_t>& inter_to_outer
        );
        void renumber_clauses(const vector<uint32_t>& outer_to_inter);
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
        size_t zeroLevAssignsByCNF = 0;

        /////////////////////
        // Clauses
        bool add_clause_helper(vector<Lit>& ps);
        bool add_clause_outer(vector<Lit>& ps, const vector<Lit>& outer_ps, bool red = false, bool restore = false);

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

inline const SearchStats& Solver::get_stats() const
{
    return sumSearchStats;
}

inline const SolveStats& Solver::get_solve_stats() const
{
    return solveStats;
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

template<> inline vector<Lit> Solver::clause_outer_numbered(const vector<uint32_t>& cl) const {
    tmpCl.clear();
    for(const auto& l: cl) tmpCl.push_back(Lit(map_inter_to_outer(l), false));

    return tmpCl;
}

template<class T> inline vector<Lit> Solver::clause_outer_numbered(const T& cl) const {
    tmpCl.clear();
    for(const auto& l: cl) tmpCl.push_back(map_inter_to_outer(l));

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

inline void Solver::copy_assumptions(const vector<Lit>* assumps) {
    assumptions.clear();
    if (assumps) {
        for(const Lit lit: *assumps) {
            if (lit.var() >= nVarsOuter()) {
                cout << "ERROR: Assumption variable " << (lit.var()+1)
                << " is too large, you never inserted that variable into the solver. Exiting."
                << endl;
                assert(false);
                exit(-1);
            }
            assumptions.push_back(lit);
        }
        if (frat->incremental()) { *frat << assump << *assumps << fin; }
    }
}

inline lbool Solver::simplify_with_assumptions(
    const vector<Lit>* _assumptions,
    const string* strategy
) {
    copy_assumptions(_assumptions);
    return simplify_problem_outside(strategy);
}

inline const vector<lbool>& Solver::get_model() const { return model; }
inline const vector<Lit>& Solver::get_final_conflict() const { return conflict; }
inline void Solver::setConf(const SolverConf& _conf) { conf = _conf; }
inline bool Solver::prop_at_head() const { return qhead == trail.size(); }
inline lbool Solver::model_value (const Lit p) const {
    if (model[p.var()] == l_Undef) return l_Undef;
    return model[p.var()] ^ p.sign();
}

inline lbool Solver::model_value (const uint32_t p) const { return model[p]; }

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
