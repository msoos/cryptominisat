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

#include <string>
#include <vector>
#include <cstdlib>
#include <cassert>
#include "constants.h"
#include "solvertypesmini.h"

using std::string;

namespace CMSat {

inline std::string polarity_mode_to_long_string(PolarityMode polarmode)
{
    switch(polarmode) {
        case PolarityMode::polarmode_automatic :
            return "auto";
        case PolarityMode::polarmode_neg :
            return "neg";
        case PolarityMode::polarmode_pos :
            return "pos";
        case PolarityMode::polarmode_weighted :
            return "weighted";
        case PolarityMode::polarmode_rnd :
            return "rnd";
        default:
            release_assert(false && "Unknown polarity mode");
    }
}

class GaussConf
{
    public:

    GaussConf() :
        autodisable(true)
        , min_usefulness_cutoff(0.2)
        , max_matrix_columns(100000)
        , max_matrix_rows(100000)
        , min_matrix_rows(1)
        , max_num_matrices(1000000)
    {
    }

    bool autodisable;
    double min_usefulness_cutoff;
    uint32_t autodisable_min_calls = 200;
    uint32_t autodisable_check_every = 1024;
    uint32_t max_matrix_columns;
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
        PolarityMode polarity_mode;

        //Clause cleaning
        uint32_t pred_short_size;
        uint32_t pred_long_size;
        uint32_t pred_forever_size;
        uint32_t pred_forever_cutoff;
        uint32_t order_tier2_by;
        double pred_forever_size_pow;

        uint32_t pred_long_chunk;
        uint32_t pred_forever_chunk;
        int      pred_forever_chunk_mult; //true or false

        int move_from_tier0;
        int move_from_tier1;

        uint32_t pred_long_check_every_n;
        uint32_t pred_forever_check_every_n;
        int   pred_distill_only_smallgue;
        int   pred_dontmove_until_timeinside;

        unsigned every_pred_reduce;
        int      dump_pred_distrib;
        double    clause_decay;

        //Learnt clause DB reduction, as in CaDiCaL
        int      reduce;           ///<Enable clause DB reduction
        unsigned reduceint;        ///<Base reduce interval, in conflicts
        unsigned reducetarget;     ///<Percent of unused candidates removed per reduce
        unsigned reducetier1glue;  ///<Glue at/below which learnt clauses are kept forever
        unsigned reducetier2glue;  ///<Glue at/below which learnt clauses get a double life
        int      flush;            ///<Enable full flushing of unused redundant clauses
        unsigned flushfactor;      ///<Flush interval multiplier
        uint64_t flushint;         ///<Initial flush interval, in conflicts

        //Restarts, as in CaDiCaL
        int      do_restart;       ///<Enable restarts
        unsigned restartint;       ///<Minimum number of conflicts between restarts
        double   restartmargin;    ///<Percent the fast glue EMA must be above the slow one to restart
        double   emagluefast;      ///<Window of the fast glue EMA
        double   emaglueslow;      ///<Window of the slow glue EMA
        int      do_stabilize;     ///<Alternate stable (reluctant doubling) and focused (glue EMA) phases
        uint64_t stabilizeint;     ///<Length of first stabilizing phase, in conflicts
        double   stabilizefactor;  ///<Multiplier of phase length at each phase change
        uint64_t stabilizemaxint;  ///<Maximum phase length
        unsigned reluctantint;     ///<Reluctant doubling base period for stable-phase restarts, 0=never restart
        uint64_t reluctantmax;     ///<Maximum reluctant doubling period multiplier

        //Rephasing, as in CaDiCaL
        int      do_rephase;       ///<Enable resetting the saved phases
        uint64_t rephaseint;       ///<Rephase interval, in conflicts
        int      phase;            ///<Default decision polarity
        int      lucky;            ///<Search for lucky phases before the CDCL loop
        int      target_phases;    ///<Decide on target phases (1=stable phases only, 2=always)

        unsigned  shortTermHistorySize; ///< Rolling avg. glue window size
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

        int  doPrintGateDot; ///< Print DOT file of gates
        int  print_full_restart_stat;
        int  print_all_restarts;
        int  verbStats;
        int do_print_times; ///Print times during verbose output
        int print_restart_line_every_n_confl;

        //Limits
        double   maxTime;
        uint64_t max_confl;

        //Glues

        //Reason-side bumping, as in CaDiCaL. 0 = off
        uint32_t  bump_reason_depth;

        //All-UIP shrinking of learnt clauses, as in CaDiCaL
        int       do_shrink_uip;

        //On-the-fly strengthening during conflict analysis, as in CaDiCaL
        int       do_otfs;

        //chrono bt
        int diff_declev_for_chrono;
        int do_chrono_reuse_trail;
        int do_restart_reuse_trail;

        //decision-based conflict clause generation
        int       do_decision_based_cl;
        uint32_t  decision_based_cl_max_levels;
        uint32_t  decision_based_cl_min_learned_size;

        //SQL
        bool      dump_individual_restarts_and_clauses;
        double    dump_individual_cldata_ratio;
        int       sql_overwrite_file;
        double    lock_for_data_gen_ratio;

        //Var-elim
        int      doVarElim;          ///<Perform variable elimination
        uint32_t varelim_occ_cutoff; ///<CaDiCaL's elimocclim: cap on the *larger* polarity's occurrences
        int      do_empty_varelim;
        int      do_full_varelim;
        int      do_xor_varelim;
        long long empty_varelim_time_limitM;
        long long varelim_time_limitM;
        long long varelim_sub_str_limitM;
        double    varElimRatioPerIter;
        int velim_resolvent_too_large; //-1 == no limit
        int varelim_score_prod; ///<CaDiCaL's elimprod, weight of pos*neg in the elim score
        int varelim_score_sum;  ///<CaDiCaL's elimsum, weight of pos+neg in the elim score
        int var_linkin_limit_MB;
        int varelim_gate_find_limit;
        int picosat_gate_limitK;
        uint32_t xor_gate_find_maxsize; ///<Largest clause XOR-gate finding will look at, before the log2 occurrence cap
        int picosat_confl_limit;
        int varelim_check_resolvent_subs;

        //Subs, str limits for simplifier
        long long subsumption_time_limitM;
        long long weaken_time_limitM;
        double subsumption_time_limit_ratio_sub_str_w_bin;
        double subsumption_time_limit_ratio_sub_w_long;
        long long strengthening_time_limitM;
        long long occ_based_lit_rem_time_limitM;

        //Ternary resolution
        bool doTernary;
        long long ternary_res_time_limitM;
        double ternary_max_create;
        int    allow_ternary_bin_create;

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
        int non_stop_bve;
        unsigned bva_limit_per_call;
        int      bva_also_twolit_diff;
        long     bva_extra_lit_and_red_start;
        long long bva_time_limitM;
        uint32_t  bva_every_n;

        //Probing
        int      do_full_probe;
        int      doIntreeProbe;
        int      doTransRed;   ///<carry out transitive reduction
        unsigned long long   full_probe_time_limitM;
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

        //Cardinality
        int      doFindCard;

        #ifdef FINAL_PREDICTOR
        //Predictor system
        std::string pred_conf_location;
        std::string pred_tables = "110";
        std::string predictor_type = "xgb";
        std::string predict_best_feat_fname;
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

        //Simplification
        int      perform_occur_based_simp;
        int      do_strengthen_with_occur;         ///<Perform self-subsuming resolution
        unsigned maxRedLinkInSize;
        double maxOccurIrredMB;
        double maxOccurRedMB;
        double maxOccurRedLitLinkedM;
        double   subsume_gothrough_multip;

        //Local search, as in CaDiCaL
        int      doSLS;            ///<Enable local search ('walk') during rephasing
        int      walknonstable;    ///<Also run local search during focused phases
        int      walkseedphase;    ///<Start local search off the CDCL phases, as CaDiCaL does
        uint32_t walkinitially;    ///<Local search rounds to run before simplifying and searching
        uint32_t walkxorweight;    ///<Weight of XOR constraints in yalsat's break values, times 100
        uint64_t walkmineff;       ///<Minimum local search effort, in yalsat mems
        uint64_t walkmaxeff;       ///<Maximum local search effort, in yalsat mems
        uint64_t walkreleff;       ///<Local search effort per mille of search propagations
        uint32_t sls_memoutMB;     ///<Skip local search if handing over the formula needs more

        //Distillation
        int      do_distill_clauses;
        int      do_distill_bin_clauses;
        unsigned long long distill_long_cls_time_limitM;
        long watch_based_str_time_limitM;
        double distill_increase_conf_ratio;
        long distill_min_confl;
        unsigned distill_red_releff;   ///<Per-mille of props since last round, as CaDiCaL's vivifyreleff
        int    distill_instantiate;    ///<Try removing the last literal, as CaDiCaL's vivifyinst
        int    distill_rem_level;      ///<Clause removal during distillation: 0 = never, 1 = only on a real conflict, 2 = also on a positively implied literal
        double distill_irred_alsoremove_ratio;
        double distill_irred_noremove_ratio;

        //Memory savings
        int       doRenumberVars;
        int       must_renumber; ///< if set, all "renumber" is treated as a "must-renumber"
        int       doSaveMem;
        uint64_t  full_watch_consolidate_every_n_confl;
        int must_always_conslidate = 0; // only used for debugging

        //Misc Optimisations
        int      doStrSubImplicit;
        long long  subsume_implicit_time_limitM;
        long long  distill_implicit_with_implicit_time_limitM;
        int do_subs_with_resolvent_clauses;

        //Gates
        int doGateFind; ///< Find OR gates
        long long gatefinder_time_limitM;

        //Gauss
        GaussConf gaussconf;

        //Sampling
        std::vector<uint32_t> sampling_vars;
        bool sampling_vars_set = false;
        std::vector<uint32_t> opt_sampling_vars;
        bool opt_sampling_vars_set = false;

        //Timeouts
        double global_next_multiplier;
        double orig_global_timeout_multiplier;
        double global_timeout_multiplier;
        double global_timeout_multiplier_multiplier;
        double global_multiplier_multiplier_max;
        double var_and_mem_out_mult;
        double oracle_mult;

        //Multi-thread, MPI
        unsigned long long sync_every_confl;
        uint32_t every_n_mpi_sync;
        unsigned thread_num;
        uint32_t is_mpi;

        // Oracle
        int oracle_get_learnts; // get oracle learnt clauses
        int oracle_removed_is_learnt; // clauses removed by Oracle should be learnt
        int oracle_find_bins;

        //Misc
        unsigned origSeed;
        int      conf_needed = true;
        string   prefix;
};

} //end namespace
