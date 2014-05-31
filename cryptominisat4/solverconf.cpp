/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
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

#include "solverconf.h"
#include <limits>
using namespace CMSat;

SolverConf::SolverConf() :
        //Variable activities
        var_inc_start(128)
        , var_inc_multiplier(11)
        , var_inc_divider(10)
        , var_inc_variability(0)
        , random_var_freq(0)
        , polarity_mode(polarmode_automatic)
        , do_calc_polarity_first_time(true)
        , do_calc_polarity_every_time(true)

        //Clause cleaning
        , clauseCleaningType(clean_sum_prop_confl_based)
        , clean_confl_multiplier(1ULL)
        , doPreClauseCleanPropAndConfl(false)
        , preClauseCleanLimit(2)
        , preCleanMinConflTime(10000)
        , doClearStatEveryClauseCleaning(true)
        , dont_remove_fresh_glue2(false)
        , ratioRemoveClauses(0.5)
        , numCleanBetweenSimplify(2)
        , startClean(10000)
        , increaseClean(1.1)
        , maxNumRedsRatio(10)
        , clauseDecayActivity(1.0/0.999)
        , min_time_in_db_before_eligible_for_cleaning(10ULL*1000ULL)
        , lock_uip_per_dbclean(500)
        , lock_topclean_per_dbclean(0)
        , multiplier_perf_values_after_cl_clean(0)

        //Restarting
        , restart_first(300)
        , restart_inc(2)
        , burstSearchLen(300)
        , restartType(restart_type_automatic)
        , do_blocking_restart(1)
        , blocking_restart_trail_hist_length(5000)
        , blocking_restart_multip(1.4)

        //Clause minimisation
        , doRecursiveMinim (true)
        , doMinimRedMore(true)
        , doAlwaysFMinim   (false)
        , more_red_minim_limit_cache(200)
        , more_red_minim_limit_binary(100)
        , extra_bump_var_activities_based_on_glue(true)

        //Verbosity
        , verbosity        (0)
        , doPrintGateDot   (false)
        , doPrintConflDot  (false)
        , printFullStats   (false)
        , verbStats        (0)
        , doPrintLongestTrail(0)
        , doPrintBestRedClauses(0)

        //Limits
        , maxTime          (std::numeric_limits<double>::max())
        , maxConfl         (std::numeric_limits<long>::max())

        //Agilities
        , agilityG                  (0.9999)
        , agilityLimit              (0.03)
        , agilityViolationLimit     (20)

        //Glues
        , updateGlues(true)
        , shortTermHistorySize (100)

        //OTF
        , otfHyperbin      (true)
        , doOTFSubsume     (true)
        , doOTFGateShorten (true)
        , rewardShortenedClauseWithConfl(3)

        //SQL
        , doSQL            (1)
        , dumpTopNVars     (0)
        , dump_tree_variance_stats(0)
        #ifdef STATS_NEEDED_EXTRA
        , dumpClauseDistribPer(0)
        , dumpClauseDistribMaxSize(200)
        , dumpClauseDistribMaxGlue(50)
        , preparedDumpSizeScatter(100)
        , preparedDumpSizeVarData(40)
        #endif
        , sqlServer ("localhost")
        , sqlUser ("cmsat_solver")
        , sqlPass ("")
        , sqlDatabase("cmsat")

        //Var-elim
        , doVarElim        (true)
        , varelim_cutoff_too_many_clauses(40)
        , do_empty_varelim (true)
        , updateVarElimComplexityOTF(true)
        , updateVarElimComplexityOTF_limitvars(200)
        , updateVarElimComplexityOTF_limitavg(40ULL*1000ULL)
        , var_elim_strategy  (elimstrategy_heuristic)
        , varElimCostEstimateStrategy(0)
        , varElimRatioPerIter(0.12)
        , skip_some_bve_resolvents(false)

        //Bounded variable addition
        , do_bva(true)
        , bva_limit_per_call(150000)
        , bva_also_twolit_diff(true)
        , bva_extra_lit_and_red_start(0)

        //Probing
        , doProbe          (true)
        , probe_bogoprops_timeoutM  (1900ULL)
        , doBothProp       (true)
        , doTransRed       (true)
        , doStamp          (true)
        , doCache          (true)
        , cacheUpdateCutoff(2000)
        , maxCacheSizeMB   (2048)

        //XOR
        , doFindXors       (true)
        , maxXorToFind     (5)
        , useCacheWhenFindingXors(false)
        , doEchelonizeXOR  (true)
        , maxXORMatrix     (10LL*1000LL*1000LL)

        //Var-replacer
        , doFindAndReplaceEqLits(true)
        , doExtendedSCC         (true)
        , sccFindPercent        (0.02)

        //Propagation & search
        , doLHBR           (false)
        , propBinFirst     (false)
        , dominPickFreq    (400)
        , polarity_flip_frequency_multiplier(0)

        //Simplifier
        , simplify_at_startup(false)
        , regularly_simplify_problem(true)
        , perform_occur_based_simp(true)
        , doStrengthen       (true)
        , maxRedLinkInSize (200)
        , maxOccurIrredMB  (800)
        , maxOccurRedMB    (800)
        , maxOccurRedLitLinkedM(50)
        , subsume_gothrough_multip(4.0)

        //Vivification
        , doClausVivif(true)
        , max_props_vivif_long_irred_clsM(20ULL)
        , watch_cache_stamp_based_str_timeoutM(400LL)

        //Memory savings
        , doRenumberVars   (true)
        , doSaveMem        (true)

        //Component finding
        , doFindComps     (false)
        , doCompHandler    (true)
        , handlerFromSimpNum (0)
        , compVarLimit      (1ULL*1000ULL*1000ULL)
        , compFindLimitMega (500)

        //Misc optimisations
        , doExtBinSubs     (true)
        , doSortWatched    (true)
        , doStrSubImplicit (true)
        , subsume_implicit_timeoutM(1900LL)
        , doCalcReach      (true)

        //Gates
        , doGateFind       (true)
        , maxGateBasedClReduceSize(20)
        , doShortenWithOrGates(true)
        , doRemClWithAndGates(true)
        , doFindEqLitsWithGates(true)

        //Misc
        , maxDumpRedsSize(std::numeric_limits<uint32_t>::max())
        , origSeed(0)
        , sync_every_confl(6000)
{
}
