/*
 * This library header is under MIT license but the library itself is under
 * LGPLv2 license. This means you can link to this library as you wish in
 * proprietary code. Please read the LGPLv2 license for details.
*/

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
    clean_glue_based
    , clean_size_based
    , clean_sum_prop_confl_based
    , clean_sum_confl_depth_based
    , clean_sum_activity_based
    , clean_none
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

        case clean_sum_prop_confl_based:
            return "prconf";

        case clean_sum_confl_depth_based:
            return "confdep";

        case clean_sum_activity_based:
            return "activity";

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

        //Variable activities
        unsigned  var_inc_start;
        unsigned  var_inc_multiplier;
        unsigned  var_inc_divider;
        unsigned  var_inc_variability;
        double random_var_freq;
        PolarityMode polarity_mode;
        int do_calc_polarity_first_time;
        int do_calc_polarity_every_time;

        //Clause cleaning
        ClauseCleaningTypes clauseCleaningType;
        unsigned clean_confl_multiplier;
        int       doPreClauseCleanPropAndConfl;
        unsigned  long long preClauseCleanLimit;
        unsigned  long long preCleanMinConflTime;
        int       doClearStatEveryClauseCleaning;
        int       dont_remove_fresh_glue2;
        double    ratioRemoveClauses; ///< Remove this ratio of clauses at every database reduction round
        unsigned  numCleanBetweenSimplify; ///<Number of cleaning operations between simplify operations
        unsigned  startClean;
        double    increaseClean;
        double    maxNumRedsRatio; ///<Number of red clauses must not be more than red*maxNumRedsRatio
        double    clauseDecayActivity;
        unsigned  min_time_in_db_before_eligible_for_cleaning;
        size_t   lock_uip_per_dbclean;
        size_t   lock_topclean_per_dbclean;
        double   multiplier_perf_values_after_cl_clean;

        //For restarting
        unsigned    restart_first;      ///<The initial restart limit.                                                                (default 100)
        double    restart_inc;        ///<The factor with which the restart limit is multiplied in each restart.                    (default 1.5)
        unsigned   burstSearchLen;
        Restart  restartType;   ///<If set, the solver will always choose the given restart strategy
        int       do_blocking_restart;
        unsigned blocking_restart_trail_hist_length;
        double   blocking_restart_multip;

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
        int  printFullStats;
        int  verbStats;
        unsigned  doPrintLongestTrail;
        int  doPrintBestRedClauses;

        //Limits
        double   maxTime;
        long maxConfl;

        //Agility
        double    agilityG; ///See paper by Armin Biere on agilities
        double    agilityLimit; ///The agility below which the agility is considered too low
        unsigned  agilityViolationLimit;

        //Glues
        int       updateGlues;
        unsigned  shortTermHistorySize; ///< Rolling avg. glue window size

        //OTF stuff
        int       otfHyperbin;
        int       doOTFSubsume;
        int       doOTFGateShorten; ///<Shorten redundant clauses on the fly with gates
        int       rewardShortenedClauseWithConfl; //Shortened through OTF subsumption

        //SQL
        int       doSQL;
        size_t   dumpTopNVars; //Only dump information about the "top" N active variables
        int       dump_tree_variance_stats;
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
        int      updateVarElimComplexityOTF;
        unsigned updateVarElimComplexityOTF_limitvars;
        int      updateVarElimComplexityOTF_limitavg;
        ElimStrategy  var_elim_strategy; ///<Guess varelim order, or calculate?
        int      varElimCostEstimateStrategy;
        double    varElimRatioPerIter;
        int      skip_some_bve_resolvents;

        //BVA
        int      do_bva;
        unsigned bva_limit_per_call;
        int      bva_also_twolit_diff;
        long     bva_extra_lit_and_red_start;

        //Probing
        int      doProbe;
        unsigned long long   probe_bogoprops_timeoutM;
        int      doBothProp;
        int      doTransRed;   ///<Should carry out transitive reduction
        int      doStamp;
        int      doCache;
        unsigned   cacheUpdateCutoff;
        unsigned   maxCacheSizeMB;

        //XORs
        int      doFindXors;
        int      maxXorToFind;
        int      useCacheWhenFindingXors;
        int      doEchelonizeXOR;
        unsigned long long  maxXORMatrix;

        //Var-replacement
        int doFindAndReplaceEqLits;
        int doExtendedSCC;
        double sccFindPercent;

        //Propagation & searching
        int      doLHBR; ///<Do lazy hyper-binary resolution
        int      propBinFirst;
        unsigned  dominPickFreq;
        unsigned  polarity_flip_frequency_multiplier;

        //Simplifier
        int      simplify_at_startup;
        int      regularly_simplify_problem;
        int      perform_occur_based_simp;
        int      doStrengthen;         ///<Perform self-subsuming resolution
        unsigned maxRedLinkInSize;
        unsigned maxOccurIrredMB;
        unsigned maxOccurRedMB;
        unsigned long long maxOccurRedLitLinkedM;
        double   subsume_gothrough_multip;

        //Vivification
        int      doClausVivif;
        unsigned long long max_props_vivif_long_irred_clsM;
        long watch_cache_stamp_based_str_timeoutM;

        //Memory savings
        int       doRenumberVars;
        int       doSaveMem;

        //Component handling
        int       doFindComps;
        int       doCompHandler;
        unsigned  handlerFromSimpNum;
        size_t    compVarLimit;
        unsigned long long  compFindLimitMega;


        //Misc Optimisations
        int      doExtBinSubs;
        int      doSortWatched;      ///<Sort watchlists according to size&type: binary, tertiary, normal (>3-long), xor clauses
        int      doStrSubImplicit;
        long long  subsume_implicit_timeoutM;
        int      doCalcReach; ///<Calculate reachability, and influence variable decisions with that

        //Gates
        int      doGateFind; ///< Find OR gates
        unsigned maxGateBasedClReduceSize;
        int      doShortenWithOrGates; ///<Shorten clauses with or gates during subsumption
        int      doRemClWithAndGates; ///<Remove clauses using and gates during subsumption
        int      doFindEqLitsWithGates; ///<Find equivalent literals using gates during subsumption

        //interrupting & dumping
        bool      needResultFile;     ///<If set to TRUE, result will be written to a file
        std::string resultFilename;    ///<Write result to this file. Only active if "needResultFile" is set to TRUE
        unsigned  maxDumpRedsSize; ///<When dumping the redundant clauses, this is the maximum clause size that should be dumped
        unsigned origSeed;
        unsigned long long sync_every_confl;
};

} //end namespace

#endif //SOLVERCONF_H
