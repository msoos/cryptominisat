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

#include <atomic>
#include <vector>
#include <set>
#include <map>
#include <utility>
#include <string>
#include <limits>
#include <cstdio>
#include "solvertypesmini.h"

namespace CMSat {
    struct CMSatPrivateData;
    #ifdef _WIN32
    class __declspec(dllexport) SATSolver
    #else
    class SATSolver
    #endif
    {
    public:
        ////////////////////////////
        // You can pass in a variable that if set to TRUE, will abort the
        // solver as soon as possible. This bool can be set through a timer,
        // or through a thread, etc. This gives you the possiblity to abort
        // the solver any time you like, depending on some external factor
        // such as time, or your own code's inner workings.
        SATSolver(void* config = nullptr
        , std::atomic<bool>* interrupt_asap = nullptr
        );
        ~SATSolver();

        ////////////////////////////
        // Adding variables and clauses
        ///////////////`/////////////

        void new_var(); //add a new variable to the solver
        void new_vars(const size_t n); //and many new variables to the solver -- much faster
        unsigned nVars() const; //get number of variables inside the solver
        bool add_clause(const std::vector<Lit>& lits);
        bool add_red_clause(const std::vector<Lit>& lits);
        bool add_xor_clause(const std::vector<unsigned>& vars, bool rhs);
        bool add_xor_clause(const std::vector<Lit>& lits, bool rhs = true);
        bool add_bnn_clause(
            const std::vector<Lit>& lits,
            signed cutoff,
            Lit out = lit_Undef
        );
        // Special. Must be between 0 and 1, inclusive. Sets the weight of the
        // literal, and the negation of it to 1.0-weight. Used ONLY when polarmode
        // is set to PolarityMode::polarmode_weighted.
        template<typename T>
        void set_lit_weight(const Lit lit, const T& val) {
            static constexpr bool is_float = (std::is_same<T, float>::value ||
                    std::is_same<T, double>::value);
            if constexpr (is_float) set_lit_weight_internal(lit, val);
        }

        ////////////////////////////
        // Solving and simplifying
        // You can call solve() multiple times: incremental mode is supported!
        ////////////////////////////

        lbool solve(const std::vector<Lit>* assumptions = nullptr, bool only_indep_solution = false); //solve the problem, optionally with assumptions. If only_indep_solution is set, only the independent variables set with set_independent_vars() are returned in the solution
        lbool simplify(const std::vector<Lit>* assumptions = nullptr, const std::string* strategy = nullptr); //simplify the problem, optionally with assumptions
        const std::vector<lbool>& get_model() const; //get model that satisfies the problem. Only makes sense if previous solve()/simplify() call was l_True
        const std::vector<Lit>& get_conflict() const; //get conflict in terms of the assumptions given in case the previous call to solve() was l_False
        bool okay() const; //the problem is still solveable, i.e. the empty clause hasn't been derived
        const std::vector<Lit>& get_decisions_reaching_model() const; //get decisions that lead to model. may NOT work, in case the decisions needed were internal, extended variables. exit(-1)'s in case of such a case. you MUST check decisions_reaching_computed().

        ////////////////////////////
        // Debug all calls for later replay with --debuglit FILENAME
        ////////////////////////////
        void log_to_file(std::string filename);

        ////////////////////////////
        // SQLite for statistics gathering
        ////////////////////////////
        void set_sqlite(std::string filename);
        void add_sql_tag(const std::string& tagname, const std::string& tag);
        unsigned long get_sql_id() const;

        ////////////////////////////
        // Configuration
        // -- Note that nothing else can be changed, only these.
        // -- The main.cpp has access to the internal config, but it changes
        // -- all the time and hence exposing it to the outside world would
        // -- be very brittle.
        ////////////////////////////

        void set_num_threads(unsigned n); //Number of threads to use. Must be set before any vars/clauses are added
        void set_allow_otf_gauss(); //allow on-the-fly gaussian elimination
        /**
         * CPU time (in seconds) that can be consumed before the next call to solve() must return
         *
         * Because the elapsed CPU time depends on both the number of
         * threads, and the activity of these threads, the elapsed time
         * can wildly differ from wall clock time.
         *
         * \pre max_time >= 0 */
        void set_max_time(double max_time);
        /**
         * Conflicts that can be consumed before the next call to solve() must return
         *
         * \pre max_confl >= 0
         */
        void set_max_confl(uint64_t max_confl);
        void set_verbosity(unsigned verbosity = 0); //default is 0, silent
        uint32_t get_verbosity() const;
        void set_default_polarity(bool polarity); //default polarity when branching for all vars
        void set_polarity_mode(CMSat::PolarityMode mode); //set polarity type
        CMSat::PolarityMode get_polarity_mode() const;
        void set_no_simplify(); //never simplify
        void set_no_simplify_at_startup(); //doesn't simplify at start, faster startup time
        void set_no_equivalent_lit_replacement(); //don't replace equivalent literals
        void set_no_bva(); //No bounded variable addition
        void set_no_bve(); //No bounded variable elimination
        void set_bve(int bve);
        void set_bve_too_large_resolvent(int too_large_resolvent);
        void set_greedy_undef(); //Try to set variables to l_Undef in solution
        void set_timeout_all_calls(double secs); //max timeout on all subsequent solve() or simplify
        void set_up_for_scalmc(); //used to set the solver up for ScalMC configuration
        void set_up_for_arjun();
        void set_up_for_sample_counter(const uint32_t fixed_restart);
        void set_single_run(); //we promise to call solve() EXACTLY once
        void set_intree_probe(int val);
        void set_sls(int val);
        void set_full_bve(int val);
        void set_full_bve_iter_ratio(double val);
        void set_scc(int val);
        void set_bva(int val);
        void set_distill(int val);
        void reset_vsids();
        void set_no_confl_needed(); //assumptions-based conflict will NOT be calculated for next solve run
        void set_simplify(const bool simp);
        void set_find_xors(bool do_find_xors);
        void set_min_bva_gain(uint32_t min_bva_gain);
        void set_varelim_check_resolvent_subs(bool varelim_check_resolvent_subs); //check subumption and literal during varelim
        void set_max_red_linkin_size(uint32_t sz);
        void set_seed(const uint32_t seed);
        void set_renumber(const bool renumber);
        void set_weaken_time_limitM(const uint32_t lim);
        void set_picosat_gate_limitK(const uint32_t lim);
        void set_picosat_confl_limit(const uint32_t lim);
        void set_occ_based_lit_rem_time_limitM(const uint32_t lim);
        void set_orig_global_timeout_multiplier(const double mult);
        void set_oracle_get_learnts(bool val);
        void set_oracle_removed_is_learnt(bool val);
        void set_oracle_find_bins(int val);
        double get_orig_global_timeout_multiplier();
        bool minimize_clause(std::vector<Lit>& cl);
        void set_prefix(const char* prefix);
        void set_do_subs_with_resolvent_clauses(int subs);
        void set_gates(const bool gates);

        ////////////////////////////
        // Predictive system tuning
        ////////////////////////////
        void set_pred_short_size(int32_t sz = -1);
        void set_pred_long_size(int32_t sz = -1);
        void set_pred_forever_size(int32_t sz = -1);
        void set_pred_long_chunk(int32_t sz = -1);
        void set_pred_forever_chunk(int32_t sz = -1);
        void set_pred_forever_cutoff(int32_t sz = -1);
        void set_every_pred_reduce(int32_t sz = -1);

        ////////////////////////////
        // Get generic info
        ////////////////////////////
        static const char* get_version(); //get solver version in string format
        static const char* get_version_sha1(); //get SHA1 version string of the solver
        static const char* get_compilation_env(); //get compilation environment string
        static std::string get_thanks_info(const char* prefix = "c ");  //get printable version and copyright text


        ////////////////////////////
        // Get info about only the last solve() OR simplify() call
        // summed for all threads
        ////////////////////////////
        uint64_t get_last_conflicts(); //get total number of conflicts of last solve() or simplify() call of all threads
        uint64_t get_last_propagations();  //get total number of propagations of last solve() or simplify() call made by all threads
        uint64_t get_last_decisions(); //get total number of decisions of last solve() or simplify() call made by all threads


        ////////////////////////////
        //Get info about total sum of all time of all threads
        ////////////////////////////

        uint64_t get_sum_conflicts(); //get total number of conflicts of all time of all threads
        uint64_t get_sum_conflicts() const; //!< Return sum of all conflicts since construction across all the threads
        uint64_t get_sum_propagations();  //get total number of propagations of all time made by all threads
        uint64_t get_sum_propagations() const; //!< Returns sum of all propagations since construction across all the threads
        uint64_t get_sum_decisions(); //get total number of decisions of all time made by all threads
        uint64_t get_sum_decisions() const; //!< Returns sum of all decisions since construction across all the threads

        void print_stats(double wallclock_time_started = 0) const; //print solving stats. Call after solve()/simplify()
        void set_frat(FILE* os); //set frat to ostream, e.g. stdout or a file
        void set_idrup(FILE* os); //set idrup to ostream, e.g. stdout or a file
        void interrupt_asap(); //call this asynchronously, and the solver will try to cleanly abort asap
        void add_in_partial_solving_stats(); //used only by Ctrl+C handler. Ignore.

        ////////////////////////////
        // Extract useful information from the solver
        // This can be used in the theory solver

        ////////////////////////////
        std::vector<Lit> get_zero_assigned_lits() const; //get literals of fixed value
        std::vector<std::pair<Lit, Lit> > get_all_binary_xors() const; //get all binary XORs that are = 0

        //////////////////////
        // EXPERIMENTAL
        std::vector<std::pair<std::vector<uint32_t>, bool> > get_recovered_xors(bool xor_together_xors) const; //get XORs recovered. If "xor_together_xors" is TRUE, then xors that share a variable (and ONLY they share them) will be XORed together
        std::vector<OrGate> get_recovered_or_gates();
        std::vector<ITEGate> get_recovered_ite_gates();
        std::vector<uint32_t> remove_definable_by_irreg_gate(const std::vector<uint32_t>& vars);
        std::vector<uint32_t> extend_definable_by_irreg_gate(const std::vector<uint32_t>& vars);
        void clean_sampl_get_empties(std::vector<uint32_t>& sampl_vars, std::vector<uint32_t>& empty_vars);
        std::vector<uint32_t> get_var_incidence();
        std::vector<uint32_t> get_lit_incidence();
        std::vector<uint32_t> get_var_incidence_also_red();
        std::vector<double> get_vsids_scores();

        lbool find_fast_backw(FastBackwData fast_backw);
        void remove_and_clean_all();
        lbool probe(Lit l, uint32_t& min_props);
        bool backbone_simpl(int64_t max_confl, bool& finished);
        std::vector<std::vector<uint8_t>> many_sls(int64_t mems, uint32_t num);

        //Given a set of literals to enqueue, returns:
        // 1) Whether they imply UNSAT. If "false": UNSAT
        // 2) into "out_implied" the set of literals they imply, including the literals themselves
        // NOTES:
        // * In case some variables have been eliminated, they cannot be implied
        // * You must not put any variable into it that's been eliminated. Pass a pointer to solve() and simplif() to make sure some variables are never elimianted, or call set_no_bve()
        // * You may get back some of the literals you gave
        // * Order is not guaranteed: literals you gave as input may end up at the end or may not end up at all
        // * It only returns literals that are newly implied. So you must call get_zero_assigned_lits() before to be sure you know what literals are implied at decision level 0
        bool implied_by(
            const std::vector<Lit>& lits, std::vector<Lit>& out_implied);

        //////////////////////
        //Below must be done in-order. Multi-threading not allowed.
        void start_getting_constraints(
               bool red, // only redundant, otherwise only irred
               bool simplified = false,
               uint32_t max_len = std::numeric_limits<uint32_t>::max(),
               uint32_t max_glue = std::numeric_limits<uint32_t>::max());
        bool get_next_constraint(std::vector<Lit>& ret, bool& is_xor, bool& rhs);
        void end_getting_constraints();

        uint32_t simplified_nvars();
        std::set<uint32_t> translate_sampl_set(
                const std::set<uint32_t>& sampl_set, bool also_removed);

        std::vector<Lit> get_weight_translation() const;
        std::map<uint32_t, VarMap> update_var_mapping(const std::map<uint32_t, VarMap>& vmap);
        std::vector<uint32_t> get_elimed_vars() const;
        std::vector<std::vector<Lit>> get_cls_defining_var(uint32_t v) const;
        void reverse_bce();

        // Solution reconstruction after minimization
        std::string serialize_solution_reconstruction_data() const;
        static void* create_extend_solution_setup(std::string& data);
        static std::pair<lbool, std::vector<lbool>> extend_solution(void* s, const std::vector<lbool>& simp_sol);
        static void delete_extend_solution_setup(void* s);

        /////////////////////
        // Backwards compatibility, implemented using the above "small clauses" functions
        void open_file_and_dump_irred_clauses(const char* fname);
        bool removed_var(uint32_t var) const;

        bool get_weighted() const {return false;}
        void set_weighted(const bool) {}
        void set_projected(const bool) {}
        template<class T> void set_multiplier_weight(const T&) {}
        const std::vector<uint32_t>& get_sampl_vars() const;
        void set_sampl_vars(const std::vector<uint32_t>& vars);
        bool get_sampl_vars_set() const;
        const std::vector<uint32_t>& get_opt_sampl_vars() const;
        void set_opt_sampl_vars(const std::vector<uint32_t>& vars);
        bool get_opt_sampl_vars_set() const;

    private:
        void set_lit_weight_internal(const Lit lit, double val);

        ////////////////////////////
        // Do not bother with this, it's private
        ////////////////////////////

        CMSatPrivateData *data;
    };

    template<class T, class T2>
    void copy_solver_to_solver(T* solver, T2* solver2) {
        solver2->new_vars(solver->nVars());
        solver->start_getting_constraints(false);
        std::vector<Lit> c; bool is_xor; bool rhs; bool ret = true;
        while (ret) {
            ret = solver->get_next_constraint(c, is_xor, rhs);
            if (!ret) break;
            if (is_xor) solver2->add_xor_clause(c, rhs);
            else solver2->add_clause(c);
        }
        solver->end_getting_constraints();
    }

    template<class T, class T2>
    void copy_simp_solver_to_solver(T* solver, T2* solver2) {
        solver2->new_vars(solver->simplified_nvars());
        solver->start_getting_constraints(false, true);
        std::vector<Lit> c; bool is_xor; bool rhs; bool ret = true;
        while (ret) {
            ret = solver->get_next_constraint(c, is_xor, rhs);
            if (!ret) break;
            if (is_xor) solver2->add_xor_clause(c, rhs);
            else solver2->add_clause(c);
        }
        solver->end_getting_constraints();
    }
}
