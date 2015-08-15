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

#ifndef SOLVER_H
#define SOLVER_H

#include "constants.h"
#include <vector>
#include <fstream>

#include "constants.h"
#include "solvertypes.h"
#include "implcache.h"
#include "propengine.h"
#include "searcher.h"
#include "cleaningstats.h"
#include "clauseusagestats.h"
#include "features.h"

namespace CMSat {

using std::vector;
using std::pair;
using std::string;

class VarReplacer;
class ClauseCleaner;
class Prober;
class OccSimplifier;
class SCCFinder;
class Distiller;
class Strengthener;
class CalcDefPolars;
class SolutionExtender;
class SQLStats;
class ImplCache;
class CompFinder;
class CompHandler;
class SubsumeStrengthen;
class SubsumeImplicit;
class DataSync;
class SharedData;
class ReduceDB;
class InTree;

class LitReachData {
    public:
        LitReachData() :
            lit(lit_Undef)
            , numInCache(0)
        {}
        Lit lit;
        uint32_t numInCache;
};


class Solver : public Searcher
{
    public:
        Solver(const SolverConf *_conf = NULL, bool* _needToInterrupt = NULL);
        ~Solver() override;

        void add_sql_tag(const string& tagname, const string& tag);
        const vector<std::pair<string, string> >& get_sql_tags() const;
        void new_external_var();
        void new_external_vars(size_t n);
        bool add_clause_outer(const vector<Lit>& lits);
        bool add_xor_clause_outer(const vector<Var>& vars, bool rhs);

        lbool solve_with_assumptions(const vector<Lit>* _assumptions = NULL);
        void  set_shared_data(SharedData* shared_data);
        lbool model_value (const Lit p) const;  ///<Found model value for lit
        lbool model_value (const Var p) const;  ///<Found model value for var
        const vector<lbool>& get_model() const;
        const vector<Lit>& get_final_conflict() const;
        void open_file_and_dump_irred_clauses(string fname) const;
        void open_file_and_dump_red_clauses(string fname) const;
        vector<pair<Lit, Lit> > get_all_binary_xors() const;

        struct SolveStats
        {
            uint64_t numSimplify = 0;
            uint32_t num_solve_calls = 0;
        };
        static const char* get_version_tag();
        static const char* get_version_sha1();
        static const char* get_compilation_env();

        vector<Lit> get_zero_assigned_lits() const;
        void     print_stats() const;
        void     print_clause_stats() const;
        size_t get_num_free_vars() const;
        size_t get_num_nonfree_vars() const;
        const SolverConf& getConf() const;
        void setConf(const SolverConf& conf);
        const vector<std::pair<string, string> >& get_tags() const;
        const BinTriStats& getBinTriStats() const;
        size_t   get_num_long_irred_cls() const;
        size_t   get_num_long_red_cls() const;
        size_t get_num_vars_elimed() const;
        size_t get_num_vars_replaced() const;
        Var num_active_vars() const;
        void print_mem_stats() const;
        uint64_t print_watch_mem_used(uint64_t totalMem) const;
        unsigned long get_sql_id() const;
        const SolveStats& get_solve_stats() const;
        void add_in_partial_solving_stats();


        ///Return number of variables waiting to be replaced
        const Stats& get_stats() const;


        //Checks
        void check_implicit_propagated() const;
        void check_stats(const bool allowFreed = false) const;
        uint64_t count_lits(
            const vector<ClOffset>& clause_array
            , bool allowFreed
        ) const;
        void check_implicit_stats() const;
        bool find_with_stamp_a_or_b(Lit a, Lit b) const;
        bool find_with_cache_a_or_b(Lit a, Lit b, int64_t* limit) const;
        bool find_with_watchlist_a_or_b(Lit a, Lit b, int64_t* limit) const;

        SQLStats* sqlStats = NULL;
        ClauseCleaner *clauseCleaner = NULL;
        VarReplacer *varReplacer = NULL;
        SubsumeImplicit *subsumeImplicit = NULL;
        DataSync *datasync = NULL;
        ReduceDB* reduceDB = NULL;
        vector<LitReachData> litReachable;

        Stats sumStats;
        PropStats sumPropStats;

        void test_all_clause_attached() const;
        void check_wrong_attach() const;
        bool prop_at_head() const;
        void set_decision_var(const uint32_t var);
        void unset_decision_var(const uint32_t var);
        bool fully_enqueue_these(const vector<Lit>& toEnqueue);
        bool fully_enqueue_this(const Lit lit);
        void update_assumptions_after_varreplace();

        uint64_t getNumLongClauses() const;
        bool addClause(const vector<Lit>& ps);
        bool add_xor_clause_inter(
            const vector< Lit >& lits
            , bool rhs
            , bool attach
            , bool addDrup = true
        );
        void new_var(const bool bva = false, const Var orig_outer = std::numeric_limits<Var>::max()) override;
        void new_vars(const size_t n) override;
        void bva_changed();

        //Attaching-detaching clauses
        void attachClause(
            const Clause& c
            , const bool checkAttach = true
        ) override;
        void attach_bin_clause(
            const Lit lit1
            , const Lit lit2
            , const bool red
            , const bool checkUnassignedFirst = true
        ) override;
        void attach_tri_clause(
            const Lit lit1
            , const Lit lit2
            , const Lit lit3
            , const bool red
        ) override;
        void detach_tri_clause(
            Lit lit1
            , Lit lit2
            , Lit lit3
            , bool red
            , bool allow_empty_watch = false
        ) override;
        void detach_bin_clause(
            Lit lit1
            , Lit lit2
            , bool red
            , bool allow_empty_watch = false
        ) override;
        void detachClause(const Clause& c, const bool removeDrup = true);
        void detachClause(const ClOffset offset, const bool removeDrup = true);
        void detach_modified_clause(
            const Lit lit1
            , const Lit lit2
            , const uint32_t origSize
            , const Clause* address
        ) override;
        Clause* add_clause_int(
            const vector<Lit>& lits
            , const bool red = false
            , const ClauseStats stats = ClauseStats()
            , const bool attach = true
            , vector<Lit>* finalLits = NULL
            , bool addDrup = true
            , const Lit drup_first = lit_Undef
        );
        void clear_clauses_stats();
        template<class T> vector<Lit> clauseBackNumbered(const T& cl) const;
        void consolidate_mem();
        size_t mem_used() const;

    private:
        friend class Prober;
        friend class ClauseDumper;
        lbool iterate_until_solved();
        void parse_sql_option();
        void dump_memory_stats_to_sql();
        uint64_t mem_used_vardata() const;
        Features calculate_features() const;
        void reconfigure(int val);
        void reset_reason_levels_of_vars_to_zero();

        vector<Lit> finalCl_tmp;
        bool sort_and_clean_clause(vector<Lit>& ps, const vector<Lit>& origCl);
        void set_up_sql_writer();
        vector<std::pair<string, string> > sql_tags;

        void check_config_parameters() const;
        void handle_found_solution(const lbool status);
        void add_every_combination_xor(const vector<Lit>& lits, bool attach, bool addDrup);
        void add_xor_clause_inter_cleaned_cut(const vector<Lit>& lits, bool attach, bool addDrup);
        unsigned num_bits_set(const size_t x, const unsigned max_size) const;
        void check_too_large_variable_number(const vector<Lit>& lits) const;
        void set_assumptions();
        struct ReachabilityStats
        {
            ReachabilityStats& operator+=(const ReachabilityStats& other);
            void print() const;
            void print_short(const Solver* solver) const;

            double cpu_time = 0.0;
            size_t numLits = 0;
            size_t dominators = 0;
            size_t numLitsDependent = 0;
        };

        lbool solve();
        vector<Lit> back_number_from_outside_to_outer_tmp;
        template<class T>
        void back_number_from_outside_to_outer(const vector<T>& lits)
        {
            back_number_from_outside_to_outer_tmp.clear();
            for (const T& lit: lits) {
                assert(lit.var() < nVarsOutside());
                back_number_from_outside_to_outer_tmp.push_back(map_to_with_bva(lit));
                assert(back_number_from_outside_to_outer_tmp.back().var() < nVarsOuter());
            }
        }
        void check_switchoff_limits_newvar(size_t n = 1);
        vector<Lit> outside_assumptions;
        void checkDecisionVarCorrectness() const;

        //Stats printing
        void print_min_stats() const;
        void print_all_stats() const;

        lbool simplify_problem(const bool startup);
        bool execute_inprocess_strategy(const string& strategy, bool startup);
        SolveStats solveStats;
        void check_minimization_effectiveness(lbool status);
        void check_recursive_minimization_effectiveness(const lbool status);
        void extend_solution();

        /////////////////////
        // Objects that help us accomplish the task
        Prober              *prober = NULL;
        InTree              *intree = NULL;
        OccSimplifier          *simplifier = NULL;
        Distiller           *distiller = NULL;
        Strengthener        *strengthener = NULL;
        CompHandler         *compHandler = NULL;

        /////////////////////////////
        // Temporary datastructs -- must be cleared before use
        mutable std::vector<Lit> tmpCl;

        /////////////////////////////
        //Renumberer
        void renumber_variables();
        void free_unused_watches();
        void save_on_var_memory(uint32_t newNumVars);
        void unSaveVarMem();
        size_t calculate_interToOuter_and_outerToInter(
            vector<Var>& outerToInter
            , vector<Var>& interToOuter
        );
        void renumber_clauses(const vector<Var>& outerToInter);
        void test_renumbering() const;



        /////////////////////////////
        // SAT solution verification
        bool verify_model() const;
        bool verify_implicit_clauses() const;
        bool verify_long_clauses(const vector<ClOffset>& cs) const;


        /////////////////////
        // Data
        size_t               zeroLevAssignsByCNF = 0;
        size_t               zero_level_assigns_by_searcher = 0;
        void calculate_reachability();

        //Main up stats
        ReachabilityStats reachStats;

        /////////////////////
        // Clauses
        bool addClauseHelper(vector<Lit>& ps);
        void print_all_clauses() const;

        /////////////////
        // Debug

        bool normClauseIsAttached(const ClOffset offset) const;
        void find_all_attach() const;
        void find_all_attach(const vector<ClOffset>& cs) const;
        bool find_clause(const ClOffset offset) const;
        void print_watch_list(watch_subarray_const ws, const Lit lit) const;
        void print_clause_size_distrib();
        void print_prop_confl_stats(
            std::string name
            , const vector<ClauseUsageStats>& stats
        ) const;
        void check_model_for_assumptions() const;
};

inline void Solver::set_decision_var(const uint32_t var)
{
    if (!varData[var].is_decision) {
        varData[var].is_decision = true;
        insertVarOrder(var);
    }
}

inline void Solver::unset_decision_var(const uint32_t var)
{
    if (varData[var].is_decision) {
        varData[var].is_decision = false;
    }
}

inline uint64_t Solver::getNumLongClauses() const
{
    return longIrredCls.size() + longRedCls.size();
}

inline const Searcher::Stats& Solver::get_stats() const
{
    return sumStats;
}

inline const Solver::SolveStats& Solver::get_solve_stats() const
{
    return solveStats;
}

inline void Solver::add_sql_tag(const string& tagname, const string& tag)
{
    sql_tags.push_back(std::make_pair(tagname, tag));
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

inline const Solver::BinTriStats& Solver::getBinTriStats() const
{
    return binTri;
}

template<class T>
inline vector<Lit> Solver::clauseBackNumbered(const T& cl) const
{
    tmpCl.clear();
    for(size_t i = 0; i < cl.size(); i++) {
        tmpCl.push_back(map_inter_to_outer(cl[i]));
    }

    return tmpCl;
}

inline lbool Solver::solve_with_assumptions(
    const vector<Lit>* _assumptions
) {
    outside_assumptions.clear();
    if (_assumptions) {
        for(const Lit lit: *_assumptions) {
            outside_assumptions.push_back(lit);
        }
    }
    return solve();
}

inline bool Solver::find_with_stamp_a_or_b(Lit a, const Lit b) const
{
    //start STAMP of A < start STAMP of B
    //end STAMP of A > start STAMP of B
    //means: ~A V B is inside
    //so, invert A
    a = ~a;

    const uint64_t start_inv_other = solver->stamp.tstamp[(a).toInt()].start[STAMP_IRRED];
    const uint64_t start_eqLit = solver->stamp.tstamp[b.toInt()].end[STAMP_IRRED];
    if (start_inv_other < start_eqLit) {
        const uint64_t end_inv_other = solver->stamp.tstamp[(a).toInt()].end[STAMP_IRRED];
        const uint64_t end_eqLit = solver->stamp.tstamp[b.toInt()].end[STAMP_IRRED];
        if (end_inv_other > end_eqLit) {
            return true;
        }
    }

    return false;
}

inline bool Solver::find_with_cache_a_or_b(Lit a, Lit b, int64_t* limit) const
{
    const vector<LitExtra>& cache = solver->implCache[a.toInt()].lits;
    *limit -= cache.size();
    for (LitExtra cacheLit: cache) {
        if (cacheLit.getOnlyIrredBin()
            && cacheLit.getLit() == b
        ) {
            return true;
        }
    }

    std::swap(a,b);

    const vector<LitExtra>& cache2 = solver->implCache[a.toInt()].lits;
    *limit -= cache2.size();
    for (LitExtra cacheLit: cache) {
        if (cacheLit.getOnlyIrredBin()
            && cacheLit.getLit() == b
        ) {
            return true;
        }
    }

    return false;
}

inline bool Solver::find_with_watchlist_a_or_b(Lit a, Lit b, int64_t* limit) const
{
    if (watches[a.toInt()].size() > watches[b.toInt()].size()) {
        std::swap(a,b);
    }

    watch_subarray_const ws = watches[a.toInt()];
    *limit -= ws.size();
    for (const Watched w: ws) {
        if (!w.isBinary())
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

} //end namespace

#endif //SOLVER_H
