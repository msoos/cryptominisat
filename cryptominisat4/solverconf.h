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
#include <cstdlib>
#if defined(_MSC_VER) || __cplusplus>=201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
    #include <cstdint>
#else
    #include <stdint.h>
#endif

namespace CMSat {

enum ClauseCleaningTypes {
    clean_glue_based = 0
    , clean_size_based = 1
    , clean_sum_activity_based = 2
    #ifdef STATS_NEEDED
    , clean_sum_prop_confl_based = 3
    , clean_sum_confl_depth_based = 4
    #endif
    , clean_none = 5
};

enum PolarityMode {
    polarmode_pos
    , polarmode_neg
    , polarmode_rnd
    , polarmode_automatic
};

enum Restart {
    restart_type_glue
    , restart_type_glue_agility
    , restart_type_geom
    , restart_type_agility
    , restart_type_never
    , restart_type_automatic
};

inline std::string getNameOfCleanType(ClauseCleaningTypes clauseCleaningType)
{
    switch(clauseCleaningType) {
        case clean_glue_based :
            return "glue";

        case clean_size_based:
            return "size";

        case clean_sum_activity_based:
            return "activity";

        #ifdef STATS_NEEDED
        case clean_sum_prop_confl_based:
            return "prconf";

        case clean_sum_confl_depth_based:
            return "confdep";
        #endif

        default:
            std::exit(-1);
            //assert(false && "Unknown clause cleaning type?");
    };

    return "";
}

enum ElimStrategy {
    elimstrategy_heuristic
    , elimstrategy_calculate_exactly
};

inline std::string getNameOfElimStrategy(ElimStrategy strategy)
{
    switch(strategy)
    {
        case elimstrategy_heuristic:
            return "heuristic";

        case elimstrategy_calculate_exactly:
            return "calculate";

        default:
            std::exit(-1);
            //assert(false);

        return "";
    }
}

class SolverConf
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
        unsigned  var_inc_start;
        unsigned  var_inc_multiplier;
        unsigned  var_inc_divider;
        double random_var_freq;
        PolarityMode polarity_mode;
        int do_calc_polarity_first_time;
        int do_calc_polarity_every_time;

        //Clause cleaning
        double    clean_confl_multiplier;
        double    clean_prop_multiplier;
        int       doPreClauseCleanPropAndConfl;
        unsigned  long long preClauseCleanLimit;
        int       doClearStatEveryClauseCleaning;
        double    ratio_keep_clauses[10]; ///< Remove this ratio of clauses at every database reduction round
        unsigned  startClean;
        double    increaseClean;
        double    maxNumRedsRatio; ///<Number of red clauses must not be more than red*maxNumRedsRatio
        double    clauseDecayActivity;
        unsigned  min_time_in_db_before_eligible_for_cleaning;
        size_t   lock_uip_per_dbclean;
        double   multiplier_perf_values_after_cl_clean;

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

        //Clause minimisation
        int doRecursiveMinim;
        int doMinimRedMore;  ///<Perform learnt clause minimisation using watchists' binary and tertiary clauses? ("strong minimization" in PrecoSat)
        int doAlwaysFMinim; ///< Always try to minimise clause with cache&gates
        unsigned more_red_minim_limit_cache;
        unsigned more_red_minim_limit_binary;
        int extra_bump_var_activities_based_on_glue;

        //Verbosity
        int  verbosity;  ///<Verbosity level. 0=silent, 1=some progress report, 2=lots of report, 3 = all report       (default 2) preferentiality is turned off (i.e. picked randomly between [0, all])
        int  doPrintGateDot; ///< Print DOT file of gates
        int  doPrintConflDot; ///< Print DOT file for each conflict
        int  print_all_stats;
        int  verbStats;
        unsigned  doPrintLongestTrail;
        int  doPrintBestRedClauses;
        int do_print_times; ///Print times during verbose output

        //Limits
        double   maxTime;
        long maxConfl;

        //Agility
        double    agilityG; ///See paper by Armin Biere on agilities
        double    agilityLimit; ///The agility below which the agility is considered too low
        unsigned  agilityViolationLimit;

        //Glues
        int       update_glues_on_prop;
        int       update_glues_on_analyze;

        //OTF stuff
        int       otfHyperbin;
        int       doOTFSubsume;
        int       doOTFGateShorten; ///<Shorten redundant clauses on the fly with gates
        int       rewardShortenedClauseWithConfl; //Shortened through OTF subsumption

        //SQL
        int       doSQL;
        int       whichSQL;
        bool      dump_individual_search_time;
        std::string sqlite_filename;
        #ifdef STATS_NEEDED_EXTRA
        unsigned    dumpClauseDistribPer;
        unsigned    dumpClauseDistribMaxSize;
        unsigned    dumpClauseDistribMaxGlue;
        unsigned    preparedDumpSizeScatter;
        unsigned    preparedDumpSizeVarData;
        #endif
        std::string    sqlServer;
        std::string    sqlUser;
        std::string    sqlPass;
        std::string    sqlDatabase;

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
        int      maxXorToFind;
        int      useCacheWhenFindingXors;
        int      doEchelonizeXOR;
        unsigned long long  maxXORMatrix;
        long long xor_finder_time_limitM;

        //Var-replacement
        int doFindAndReplaceEqLits;
        int doExtendedSCC;
        double sccFindPercent;

        //Propagation & searching
        int      doLHBR; ///<Do lazy hyper-binary resolution
        int      propBinFirst;
        unsigned  dominPickFreq;

        //Simplifier
        int      simplify_at_startup; //simplify at 1st startup (only)
        int      simplify_at_every_startup; //always simplify at startup, not only at 1st startup
        int      regularly_simplify_problem;
        int      perform_occur_based_simp;
        int      do_strengthen_with_occur;         ///<Perform self-subsuming resolution
        unsigned maxRedLinkInSize;
        unsigned maxOccurIrredMB;
        unsigned maxOccurRedMB;
        unsigned long long maxOccurRedLitLinkedM;
        double   subsume_gothrough_multip;

        //Distillation
        int      do_distill_clauses;
        unsigned long long distill_long_irred_cls_time_limitM;
        long watch_cache_stamp_based_str_time_limitM;
        long long distill_time_limitM;

        //Memory savings
        int       doRenumberVars;
        int       doSaveMem;

        //Component handling
        int       doFindComps;
        int       doCompHandler;
        unsigned  handlerFromSimpNum;
        size_t    compVarLimit;
        unsigned long long  comp_find_time_limitM;


        //Misc Optimisations
        int      doExtBinSubs;
        int      doSortWatched;      ///<Sort watchlists according to size&type: binary, tertiary, normal (>3-long), xor clauses
        int      doStrSubImplicit;
        long long  subsume_implicit_time_limitM;
        long long  strengthen_implicit_time_limitM;
        int      doCalcReach; ///<Calculate reachability, and influence variable decisions with that

        //Gates
        int      doGateFind; ///< Find OR gates
        unsigned maxGateBasedClReduceSize;
        int      doShortenWithOrGates; ///<Shorten clauses with or gates during subsumption
        int      doRemClWithAndGates; ///<Remove clauses using and gates during subsumption
        int      doFindEqLitsWithGates; ///<Find equivalent literals using gates during subsumption
        long long gatefinder_time_limitM;
        long long shorten_with_gates_time_limitM;
        long long remove_cl_with_gates_time_limitM;

        //interrupting & dumping
        double orig_global_timeout_multiplier;
        double global_timeout_multiplier;
        double global_timeout_multiplier_multiplier;
        unsigned  maxDumpRedsSize; ///<When dumping the redundant clauses, this is the maximum clause size that should be dumped
        unsigned origSeed;
        unsigned long long sync_every_confl;
        double clean_after_perc_zero_depth_assigns;
};

} //end namespace

#endif //SOLVERCONF_H
