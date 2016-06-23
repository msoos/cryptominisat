/******************************************
Copyright (c) 2014, Mate Soos

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
#include "gaussconfig.h"

using std::string;

namespace CMSat {

enum class ClauseClean {
    glue = 0
    , size = 1
    , activity = 2
};

inline unsigned clean_to_int(ClauseClean t)
{
    switch(t)
    {
        case ClauseClean::glue:
            return 0;

        case ClauseClean::size:
            return 1;

        case ClauseClean::activity:
            return 2;
    }

    assert(false);
}

enum class PolarityMode {
    polarmode_pos
    , polarmode_neg
    , polarmode_rnd
    , polarmode_automatic
};

enum class Restart {
    glue
    , geom
    , glue_geom
    , luby
    , never
};

inline std::string getNameOfRestartType(Restart rest_type)
{
    switch(rest_type) {
        case Restart::glue :
            return "glue";

        case Restart::geom:
            return "geometric";

        case Restart::luby:
            return "luby";

        case Restart::never:
            return "never";

        default:
            release_assert(false && "Unknown clause cleaning type?");
    };
}

inline std::string getNameOfCleanType(ClauseClean clauseCleaningType)
{
    switch(clauseCleaningType) {
        case ClauseClean::glue :
            return "glue";

        case ClauseClean::size:
            return "size";

        case ClauseClean::activity:
            return "activity";

        default:
            assert(false && "Unknown clause cleaning type?");
            std::exit(-1);
    };
}

enum class ElimStrategy {
    heuristic
    , calculate_exactly
};

inline std::string getNameOfElimStrategy(ElimStrategy strategy)
{
    switch(strategy)
    {
        case ElimStrategy::heuristic:
            return "heuristic";

        case ElimStrategy::calculate_exactly:
            return "calculate";
    }

    assert(false && "Unknown elimination strategy type");
}

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

        //Variable activities
        double  var_inc_start;
        double  var_decay_start;
        double  var_decay_max;
        double random_var_freq;
        PolarityMode polarity_mode;

        //Clause cleaning
        unsigned  max_temporary_learnt_clauses;
        unsigned  cur_max_temp_red_cls;
        unsigned protect_cl_if_improved_glue_below_this_glue_for_one_turn;
        double    clean_confl_multiplier;
        double    clean_prop_multiplier;
        int       doPreClauseCleanPropAndConfl;
        unsigned  long long preClauseCleanLimit;
        double    ratio_keep_clauses[10]; ///< Remove this ratio of clauses at every database reduction round
        double    inc_max_temp_red_cls;
        double    clause_decay;
        unsigned  min_time_in_db_before_eligible_for_cleaning;
        unsigned glue_must_keep_clause_if_below_or_eq;
        double   adjust_glue_if_too_many_low;
        int      guess_cl_effectiveness;
        int      hash_relearn_check;

        //For restarting
        unsigned    restart_first;      ///<The initial restart limit.                                                                (default 100)
        double    restart_inc;        ///<The factor with which the restart limit is multiplied in each restart.                    (default 1.5)
        unsigned   burst_search_len;
        Restart  restartType;   ///<If set, the solver will always choose the given restart strategy
        int       do_blocking_restart;
        unsigned blocking_restart_trail_hist_length;
        double   blocking_restart_multip;
        double   local_glue_multiplier;
        unsigned  shortTermHistorySize; ///< Rolling avg. glue window size
        unsigned lower_bound_for_blocking_restart;
        int more_otf_shrink_with_cache;
        int more_otf_shrink_with_stamp;
        int abort_searcher_solve_on_geom_phase;

        //Clause minimisation
        int doRecursiveMinim;
        int doMinimRedMore;  ///<Perform learnt clause minimisation using watchists' binary and tertiary clauses? ("strong minimization" in PrecoSat)
        int doAlwaysFMinim; ///< Always try to minimise clause with cache&gates
        unsigned max_glue_more_minim;
        unsigned max_size_more_minim;
        unsigned more_red_minim_limit_cache;
        unsigned more_red_minim_limit_binary;
        unsigned max_num_lits_more_red_min;
        int extra_bump_var_activities_based_on_glue;

        //Verbosity
        int  verbosity;  ///<Verbosity level. 0=silent, 1=some progress report, 2=lots of report, 3 = all report       (default 2) preferentiality is turned off (i.e. picked randomly between [0, all])
        int  doPrintGateDot; ///< Print DOT file of gates
        int  doPrintConflDot; ///< Print DOT file for each conflict
        int  print_all_stats;
        int  verbStats;
        int do_print_times; ///Print times during verbose output
        int print_restart_line_every_n_confl;

        //Limits
        double   maxTime;
        long maxConfl;

        //Glues
        int       update_glues_on_prop;
        int       update_glues_on_analyze;

        //OTF stuff
        int       otfHyperbin;
        int       doOTFSubsume;
        int       doOTFSubsumeOnlyAtOrBelowGlue;
        int       rewardShortenedClauseWithConfl; //Shortened through OTF subsumption

        //SQL
        bool      dump_individual_search_time;
        bool      dump_individual_restarts_and_clauses;

        //Var-elim
        int      doVarElim;          ///<Perform variable elimination
        unsigned varelim_cutoff_too_many_clauses;
        int      do_empty_varelim;
        long long empty_varelim_time_limitM;
        long long varelim_time_limitM;
        int      updateVarElimComplexityOTF;
        unsigned updateVarElimComplexityOTF_limitvars;
        int      updateVarElimComplexityOTF_limitavg;
        ElimStrategy  var_elim_strategy; ///<Guess varelim order, or calculate?
        int      varElimCostEstimateStrategy;
        double    varElimRatioPerIter;
        int      skip_some_bve_resolvents;
        int velim_resolvent_too_large; //-1 == no limit

        //Subs, str limits for simplifier
        long long subsumption_time_limitM;
        long long strengthening_time_limitM;
        long long aggressive_elim_time_limitM;

        //BVA
        int      do_bva;
        unsigned bva_limit_per_call;
        int      bva_also_twolit_diff;
        long     bva_extra_lit_and_red_start;
        long long bva_time_limitM;

        //Probing
        int      doProbe;
        int      doIntreeProbe;
        unsigned long long   probe_bogoprops_time_limitM;
        unsigned long long   intree_time_limitM;
        unsigned long long intree_scc_varreplace_time_limitM;
        int      doBothProp;
        int      doTransRed;   ///<Should carry out transitive reduction
        int      doStamp;
        int      doCache;
        unsigned   cacheUpdateCutoff;
        unsigned   maxCacheSizeMB;
        unsigned long long otf_hyper_time_limitM;
        double  otf_hyper_ratio_limit;
        double single_probe_time_limit_perc;

        //XORs
        int      doFindXors;
        unsigned maxXorToFind;
        int      useCacheWhenFindingXors;
        int      doEchelonizeXOR;
        unsigned long long  maxXORMatrix;
        long long xor_finder_time_limitM;

        //Var-replacement
        int doFindAndReplaceEqLits;
        int doExtendedSCC;
        double sccFindPercent;

        //Propagation & searching
        int      propBinFirst;

        //Iterative Alo Scheduling
        int      simplify_at_startup; //simplify at 1st startup (only)
        int      simplify_at_every_startup; //always simplify at startup, not only at 1st startup
        int      do_simplify_problem;
        int      full_simplify_at_startup;
        int      never_stop_search;
        uint64_t num_conflicts_of_search;
        double   num_conflicts_of_search_inc;
        double   num_conflicts_of_search_inc_max;
        string   simplify_schedule_startup;
        string   simplify_schedule_nonstartup;
        string   simplify_schedule_preproc;

        //Simplification
        int      perform_occur_based_simp;
        int      do_strengthen_with_occur;         ///<Perform self-subsuming resolution
        unsigned maxRedLinkInSize;
        unsigned maxOccurIrredMB;
        unsigned maxOccurRedMB;
        unsigned long long maxOccurRedLitLinkedM;
        double   subsume_gothrough_multip;

        //Distillation
        uint32_t distill_queue_by;
        int      do_distill_clauses;
        unsigned long long distill_long_irred_cls_time_limitM;
        long watch_cache_stamp_based_str_time_limitM;
        long long distill_time_limitM;

        //Memory savings
        int       doRenumberVars;
        int       doSaveMem;

        //Component handling
        int       doCompHandler;
        unsigned  handlerFromSimpNum;
        size_t    compVarLimit;
        unsigned long long  comp_find_time_limitM;


        //Misc Optimisations
        int      doExtBinSubs;
        int      doSortWatched;      ///<Sort watchlists according to size&type: binary, tertiary, normal (>3-long), xor clauses
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

        //Greedy undef
        int      greedy_undef;
        std::vector<uint32_t>* independent_vars;

        //Timeouts
        double orig_global_timeout_multiplier;
        double global_timeout_multiplier;
        double global_timeout_multiplier_multiplier;
        double global_multiplier_multiplier_max;

        //Misc
        unsigned  maxDumpRedsSize; ///<When dumping the redundant clauses, this is the maximum clause size that should be dumped
        unsigned origSeed;
        unsigned long long sync_every_confl;
        unsigned reconfigure_val;
        unsigned reconfigure_at;
        unsigned preprocess;
        std::string simplified_cnf;
        std::string solution_file;
        std::string saved_state_file;
};

} //end namespace

#endif //SOLVERCONF_H
