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

#ifndef SOLVERCONF_H
#define SOLVERCONF_H

#include <string>
#include <vector>
#include <cstdlib>
#include <cassert>
#include "constants.h"
#include "cryptominisat5/solvertypesmini.h"

using std::string;

namespace CMSat {

enum class ClauseClean {
    glue = 0
    , activity = 1
};

inline unsigned clean_to_int(ClauseClean t)
{
    switch(t)
    {
        case ClauseClean::glue:
            return 0;

        case ClauseClean::activity:
            return 1;
    }

    assert(false);
}

enum class PolarityMode {
    polarmode_pos
    , polarmode_neg
    , polarmode_rnd
    , polarmode_automatic
    , polarmode_stable
    , polarmode_best_inv
    , polarmode_best
    , polarmode_weighted
};

enum class Restart {
    glue
    , geom
    , glue_geom
    , luby
    , never
};

inline std::string getNameOfPolarmodeType(PolarityMode polarmode)
{
    switch(polarmode) {
        case PolarityMode::polarmode_automatic :
            return "auto";
        case PolarityMode::polarmode_stable :
            return "stb";
        case PolarityMode::polarmode_best_inv :
            return "ibst";
        case PolarityMode::polarmode_best :
            return "bst";
        case PolarityMode::polarmode_neg :
            return "neg";
        case PolarityMode::polarmode_pos :
            return "pos";
        case PolarityMode::polarmode_weighted :
            return "weighted";
        case PolarityMode::polarmode_rnd :
            return "rnd";
    }
}

inline std::string restart_type_to_short_string(const Restart type)
{
    switch(type) {
        case Restart::glue:
            return "glue";

        case Restart::geom:
            return "geom";

        case Restart::luby:
            return "luby";

        case Restart::glue_geom:
            return "gl/g";

        case Restart::never:
            return "neve";
    }

        assert(false && "oops, one of the restart types has no string name");

        return "ERR: undefined!";
}

inline std::string polarity_mode_to_short_string(PolarityMode polarmode)
{
    switch(polarmode) {
        case PolarityMode::polarmode_automatic :
            return "auto";
        case PolarityMode::polarmode_stable :
            return "stb";
        case PolarityMode::polarmode_best_inv :
            return "istb";
        case PolarityMode::polarmode_best :
            return "bstb";
        case PolarityMode::polarmode_neg :
            return "neg";
        case PolarityMode::polarmode_pos :
            return "pos";
        case PolarityMode::polarmode_weighted :
            return "wght";
        case PolarityMode::polarmode_rnd :
            return "rnd";
    }
}

inline std::string getNameOfRestartType(Restart rest_type)
{
    switch(rest_type) {
        case Restart::glue :
            return "glue";

        case Restart::geom:
            return "geometric";

        case Restart::glue_geom:
            return "regularly switch between glue and geometric";

        case Restart::luby:
            return "luby";

        case Restart::never:
            return "never";

        default:
            assert(false && "Unknown clause cleaning type?");
    }
}

inline std::string getNameOfCleanType(ClauseClean clauseCleaningType)
{
    switch(clauseCleaningType) {
        case ClauseClean::glue :
            return "glue";

        case ClauseClean::activity:
            return "activity";

        default:
            assert(false && "Unknown clause cleaning type?");
            std::exit(-1);
    }
}

class GaussConf
{
    public:

    GaussConf() :
        autodisable(true)
        , min_usefulness_cutoff(0.2)
        , max_matrix_rows(5000)
        , min_matrix_rows(3)
        , max_num_matrices(5)
    {
    }

    bool autodisable;
    double min_usefulness_cutoff;
    uint32_t max_matrix_rows; //The maximum matrix size -- no. of rows
    uint32_t min_matrix_rows; //The minimum matrix size -- no. of rows
    uint32_t max_num_matrices; //Maximum number of matrices

    //Matrix extraction config
    bool doMatrixFind = true;
    uint32_t min_gauss_xor_clauses = 2;
    uint32_t max_gauss_xor_clauses = 500000;
};

class DLL_PUBLIC SolverConf
{
    public:
        SolverConf();
        std::string print_times(
            const double time_used
            , const bool time_out
            , const double time_remain
        ) const;
        std::string print_times(
            const double time_used
            , const bool time_out
        ) const;
        std::string print_times(
            const double time_used
        ) const;

        //Variable polarities
        int do_lucky_polar_every_n;
        PolarityMode polarity_mode;
        int polar_stable_every_n;
        int polar_best_inv_multip_n;
        int polar_best_multip_n;

        //Clause cleaning
        float pred_short_size_mult;
        float pred_long_size_mult;
        float pred_forever_size_mult;
        float pred_long_chunk_mult;
        float pred_forever_chunk_mult;

        //if non-zero, we reduce at every X conflicts.
        //Reduced according to whether it's been used recently
        //Otherwise, we *never* reduce
        unsigned every_lev1_reduce;

        //if non-zero, we reduce at every X conflicts.
        //Otherwise we geometrically keep around max_temp_lev2_learnt_clauses*(inc**N)
        unsigned every_lev2_reduce;

        #if defined(FINAL_PREDICTOR) || defined(STATS_NEEDED)
        unsigned every_lev3_reduce;
        #endif

        uint32_t must_touch_lev1_within;
        unsigned  max_temp_lev2_learnt_clauses;
        double    inc_max_temp_lev2_red_cls;

        unsigned protect_cl_if_improved_glue_below_this_glue_for_one_turn;
        unsigned glue_put_lev0_if_below_or_eq;
        unsigned glue_put_lev1_if_below_or_eq;
        double    ratio_keep_clauses[2]; ///< Remove this ratio of clauses at every database reduction round
        double    clause_decay;

        //If too many (in percentage) low glues after min_num_confl_adjust_glue_cutoff, adjust glue lower
        double   adjust_glue_if_too_many_low;
        uint64_t min_num_confl_adjust_glue_cutoff;

        //For restarting
        unsigned    restart_first;      ///<The initial restart limit.                                                                (default 100)
        double    restart_inc;        ///<The factor with which the restart limit is multiplied in each restart.                    (default 1.5)
        Restart  restartType;   ///<If set, the solver will always choose the given restart strategy
        int      do_blocking_restart;
        unsigned blocking_restart_trail_hist_length;
        double   blocking_restart_multip;

        double   local_glue_multiplier;
        unsigned  shortTermHistorySize; ///< Rolling avg. glue window size
        unsigned lower_bound_for_blocking_restart;
        double   ratio_glue_geom; //higher the number, the more glue will be done. 2 is 2x glue 1x geom
        int doAlwaysFMinim;

        //Branch strategy
        string branch_strategy_setup;

        //Clause minimisation
        int doRecursiveMinim;
        int doMinimRedMore;  ///<Perform learnt clause minimisation using watchists' binary and tertiary clauses? ("strong minimization" in PrecoSat)
        int doMinimRedMoreMore;
        unsigned max_glue_more_minim;
        unsigned max_size_more_minim;
        unsigned more_red_minim_limit_binary;
        unsigned max_num_lits_more_more_red_min;

        //Verbosity
        int  verbosity;  ///<Verbosity level 0-2: normal  3+ extreme
        int  xor_detach_verb; ///to debug XOR detach issues

        int  doPrintGateDot; ///< Print DOT file of gates
        int  print_full_restart_stat;
        int  print_all_restarts;
        int  verbStats;
        int do_print_times; ///Print times during verbose output
        int print_restart_line_every_n_confl;

        //Limits
        double   maxTime;
        long max_confl;

        //Glues
        int       update_glues_on_analyze;
        uint32_t  max_glue_cutoff_gluehistltlimited;

        //chrono bt
        int diff_declev_for_chrono;

        //decision-based conflict clause generation
        int       do_decision_based_cl;
        uint32_t  decision_based_cl_max_levels;
        uint32_t  decision_based_cl_min_learned_size;

        //SQL
        bool      dump_individual_restarts_and_clauses;
        double    dump_individual_cldata_ratio;
        int       sql_overwrite_file;
        double    lock_for_data_gen_ratio;

        //Steps
        double orig_step_size = 0.40;
        double step_size_dec = 0.000001;
        double min_step_size = 0.06;

        //Var-elim
        int      doVarElim;          ///<Perform variable elimination
        uint64_t varelim_cutoff_too_many_clauses;
        int      do_empty_varelim;
        int      do_full_varelim;
        long long empty_varelim_time_limitM;
        long long varelim_time_limitM;
        long long varelim_sub_str_limit;
        double    varElimRatioPerIter;
        int      skip_some_bve_resolvents;
        int velim_resolvent_too_large; //-1 == no limit
        int var_linkin_limit_MB;

        //Subs, str limits for simplifier
        long long subsumption_time_limitM;
        double subsumption_time_limit_ratio_sub_str_w_bin;
        double subsumption_time_limit_ratio_sub_w_long;
        long long strengthening_time_limitM;

        //Ternary resolution
        bool doTernary;
        long long ternary_res_time_limitM;
        double ternary_keep_mult;
        double ternary_max_create;
        int    allow_ternary_bin_create;

        //Bosphorus
        int do_bosphorus;
        uint32_t bosphorus_every_n;

        //BreakID
        bool doBreakid;
        bool breakid_use_assump; ///< If false breaks library use of solver
        uint32_t breakid_every_n;
        uint32_t breakid_vars_limit_K;
        uint64_t breakid_cls_limit_K;
        uint64_t breakid_lits_limit_K;
        int64_t breakid_time_limit_K;
        int breakid_max_constr_per_permut;
        bool breakid_matrix_detect;

        //BVA
        int      do_bva;
        int min_bva_gain;
        unsigned bva_limit_per_call;
        int      bva_also_twolit_diff;
        long     bva_extra_lit_and_red_start;
        long long bva_time_limitM;
        uint32_t  bva_every_n;

        //Probing
        int      doIntreeProbe;
        int      doTransRed;   ///<carry out transitive reduction
        unsigned long long   intree_time_limitM;
        unsigned long long intree_scc_varreplace_time_limitM;
        int       do_hyperbin_and_transred;

        //XORs
        int      doFindXors;
        unsigned maxXorToFind;
        unsigned maxXorToFindSlow;
        uint64_t maxXORMatrix;
        uint64_t xor_finder_time_limitM;
        int      allow_elim_xor_vars;
        unsigned xor_var_per_cut;
        int      force_preserve_xors;

        //Cardinality
        int      doFindCard;

        #ifdef FINAL_PREDICTOR
        //Predictor system
        std::string pred_conf_short;
        std::string pred_conf_long;
        std::string pred_conf_forever;
        float pred_keep_above;
        #endif

        //Var-replacement
        int doFindAndReplaceEqLits;
        int max_scc_depth;

        //Iterative Alo Scheduling
        int      simplify_at_startup; //simplify at 1st startup (only)
        int      simplify_at_every_startup; //always simplify at startup, not only at 1st startup
        int      do_simplify_problem;
        int      full_simplify_at_startup;
        int      never_stop_search;
        uint64_t num_conflicts_of_search;
        double   num_conflicts_of_search_inc;
        double   num_conflicts_of_search_inc_max;
        uint32_t max_num_simplify_per_solve_call;
        string   simplify_schedule_startup;
        string   simplify_schedule_nonstartup;
        string   simplify_schedule_preproc;

        //Simplification
        int      perform_occur_based_simp;
        int      do_strengthen_with_occur;         ///<Perform self-subsuming resolution
        unsigned maxRedLinkInSize;
        double maxOccurIrredMB;
        double maxOccurRedMB;
        double maxOccurRedLitLinkedM;
        double   subsume_gothrough_multip;

        //Walksat
        int doSLS;
        uint32_t sls_every_n;
        uint32_t yalsat_max_mems;
        uint32_t sls_memoutMB;
        uint32_t walksat_max_runs;
        int      sls_get_phase;
        int      sls_ccnr_asipire;
        string   which_sls;
        uint32_t sls_how_many_to_bump;
        uint32_t sls_bump_var_max_n_times;
        uint32_t sls_bump_type;
        int      sls_set_offset;

        //Distillation
        int      do_distill_clauses;
        unsigned long long distill_long_cls_time_limitM;
        long watch_based_str_time_limitM;
        long long distill_time_limitM;
        double distill_increase_conf_ratio;
        long distill_min_confl;
        double distill_red_tier1_ratio;

        //Memory savings
        int       doRenumberVars;
        int       must_renumber; ///< if set, all "renumber" is treated as a "must-renumber"
        int       doSaveMem;
        uint64_t  full_watch_consolidate_every_n_confl;

        //Component handling
        int       doCompHandler;
        unsigned  handlerFromSimpNum;
        size_t    compVarLimit;
        unsigned long long  comp_find_time_limitM;


        //Misc Optimisations
        int      doStrSubImplicit;
        long long  subsume_implicit_time_limitM;
        long long  distill_implicit_with_implicit_time_limitM;

        //Gates
        int      doGateFind; ///< Find OR gates
        unsigned maxGateBasedClReduceSize;
        int      doShortenWithOrGates; ///<Shorten clauses with or gates during subsumption
        int      doRemClWithAndGates; ///<Remove clauses using and gates during subsumption
        int      doFindEqLitsWithGates; ///<Find equivalent literals using gates during subsumption
        long long gatefinder_time_limitM;
        long long shorten_with_gates_time_limitM;
        long long remove_cl_with_gates_time_limitM;

        //Gauss
        GaussConf gaussconf;
        bool doM4RI;
        bool xor_detach_reattach;
        bool force_use_all_matrixes;

        //Sampling
        std::vector<uint32_t>* sampling_vars;

        //Timeouts
        double orig_global_timeout_multiplier;
        double global_timeout_multiplier;
        double global_timeout_multiplier_multiplier;
        double global_multiplier_multiplier_max;
        double var_and_mem_out_mult;

        //Multi-thread, MPI
        unsigned long long sync_every_confl;
        unsigned thread_num;

        //Misc
        unsigned origSeed;
        unsigned reconfigure_val;
        unsigned reconfigure_at;
        unsigned preprocess;
        int      simulate_drat;
        int      conf_needed = true;
        std::string simplified_cnf;
        std::string solution_file;
        std::string saved_state_file;
};

} //end namespace

#endif //SOLVERCONF_H
