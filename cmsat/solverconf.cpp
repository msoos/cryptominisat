/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2011, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
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

SolverConf::SolverConf() :
        var_inc_start(128)
        , var_inc_multiplier(11)
        , var_inc_divider(10)
        , var_inc_variability(0)
        , rarely_bump_var_act(false)

        //Clause cleaning
        , clauseCleaningType(CLEAN_CLAUSES_PROPCONFL_BASED)
        , doPreClauseCleanPropAndConfl(false)
        , preClauseCleanLimit(2)
        , preCleanMinConflTime(10000)
        , doClearStatEveryClauseCleaning(true)
        , ratioRemoveClauses(1.0/2.0)
        , numCleanBetweenSimplify(1)
        , startClean(8000)
        , increaseClean(1.1)
        , maxNumLearntsRatio(10)
        , clauseDecayActivity(1.0/0.999)

        //var picking
        , random_var_freq(0)
        , restart_first(100)
        , restart_inc(1.2)
        , burstSearchLen(300)
        , restartType(agility_restart)

        , doRecursiveCCMin  (true)
        , polarity_mode    (polarity_auto)
        , verbosity        (0)

        //Agilities
        , agilityG                  (0.9999)
        , agilityLimit              (0.05)
        , agilityViolationLimit     (20)

        , doPerformPreSimp (true)

        //Glues
        , updateGlues(true)
        , shortTermHistorySize (100)

        //OTF
        , otfHyperbin      (true)
        , doOTFSubsume     (true)
        , doOTFGateShorten (true)
        , rewardShortenedClauseWithConfl(10)

        //SQL
        , doSQL            (false)
        , dumpTopNVars     (50)
        , dumpClauseDistribPer(20000)
        , dumpClauseDistribMaxSize(200)
        , dumpClauseDistribMaxGlue(50)
        , preparedDumpSizeScatter(100)
        , preparedDumpSizeVarData(40)
        , sqlServer ("localhost")
        , sqlUser ("cmsat_solver")
        , sqlPass ("")
        , sqlDatabase("cmsat")

        //Var-elim
        , doVarElim        (true)
        , updateVarElimComplexityOTF(true)
        , varelimStrategy  (0)
        , varElimCostEstimateStrategy(0)
        , varElimRatioPerIter(0.22)

        //Probing
        , doProbe          (true)
        , probeMultiplier  (1.0)
        , doBothProp       (true)
        , doTransRed       (true)
        , doStamp          (true)
        , doCache          (true)
        , cacheUpdateCutoff(200)

        //XOR
        , doFindXors       (true)
        , maxXorToFind     (5)
        , useCacheWhenFindingXors(false)
        , doEchelonizeXOR  (true)
        , maxXORMatrix     (10L*1000L*1000L)

        //Var-replacer
        , doFindAndReplaceEqLits(true)
        , doExtendedSCC         (true)
        , sccFindPercent        (0.02)

        //optimisations to do
        , printFullStats   (false)
        , doRenumberVars   (true)
        , dominPickFreq    (5)
        , flipPolarFreq    (300)

        , doSchedSimp      (true)
        , doSimplify       (true)
        , doHyperBinRes    (true)
        , doLHBR           (true)
        , doBlockClauses   (true)
        , doExtBinSubs     (true)
        , doSubsume1       (true)
        , doClausVivif     (true)
        , doSortWatched    (true)
        , doMinimLearntMore(true)
        , doGateFind       (true)
        , maxGateSize      (20)
        , doAlwaysFMinim   (false)
        , doER             (false)
        , doCalcReach      (true)
        , doAsymmTE        (true)
        , doShortenWithOrGates(true)
        , doRemClWithAndGates(true)
        , doFindEqLitsWithGates(true)
        , doMixXorAndGates (false)

        //Verbosity
        , doPrintGateDot   (false)
        , doPrintConflDot  (false)

        , needToDumpLearnts(false)
        , needToDumpSimplified (false)
        , maxDumpLearntsSize(std::numeric_limits<uint32_t>::max())
        , libraryUsage     (true)
        , greedyUnbound    (false)
        , origSeed(0)
{
}
