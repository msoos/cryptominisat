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

#include "SolverConf.h"
#include <limits>

SolverConf::SolverConf() :
        var_inc_start(128)
        , var_inc_multiplier(11)
        , var_inc_divider(10)
        , var_inc_variability(0)
        , rarely_bump_var_act(false)

        //Clause cleaning
        , clauseCleaningType(CLEAN_CLAUSES_PROPCONFL_BASED)
        , doPreClauseCleanPropAndConfl(true)
        , preClauseCleanLimit(2)
        , preCleanMinConflTime(10000)
        , doClearStatEveryClauseCleaning(true)
        , ratioRemoveClauses(1.0/2.0)
        , clauseCleanNeverCleanAtOrAboveThisPropConfl(10)
        , numCleanBetweenSimplify(1)
        , startClean(8000)
        , increaseClean(1.101)

        //var picking
        , random_var_freq(0)
        , restart_first(100)
        , restart_inc(1.2)
        , burstSearchLen(300)
        , restartType(glue_restart)

        , doRecursiveCCMin  (true)
        , polarity_mode    (polarity_auto)
        , verbosity        (0)

        //Agilities
        , agilityG                  (0.9999)
        , agilityLimit              (0.20)

        , doPerformPreSimp (true)
        , probeMultiplier(1.0)

        //Glues
        , updateGlues(true)
        , shortTermGlueHistorySize (100)

        //optimisations to do
        , doSQL            (false)
        , dumpClauseDistribPer(2000)
        , dumpClauseDistribMax(300)
        , doOTFSubsume     (true)
        , rewardShortenedClauseWithConfl(10)
        , printFullStats   (false)
        , doRenumberVars   (true)
        , dominPickFreq    (5)
        , flipPolarFreq    (300)
        , doFindXors       (true)
        , maxXorToFind     (5)
        , useCacheWhenFindingXors(false)
        , doEchelonizeXOR  (true)
        , maxXORMatrix     (10L*1000L*1000L)
        , doFindAndReplaceEqLits     (true)
        , doSchedSimp      (true)
        , doSatELite       (true)
        , doHyperBinRes    (true)
        , doLHBR           (true)
        , doBlockedClause  (true)
        , doExtBinSubs     (true)
        , doVarElim        (true)
        , varelimStrategy  (0)
        , varElimRatioPerIter(0.30)
        , doSubsume1       (true)
        , doClausVivif     (true)
        , doSortWatched    (true)
        , doMinimLearntMore(true)
        , doProbe          (true)
        , doBothProp       (true)
        , doTransRed (true)
        , doCache          (true)
        , cacheUpdateCutoff(100)
        , doExtendedSCC    (false)
        , doGateFind       (true)
        , maxGateSize      (20)
        , doAlwaysFMinim   (false)
        , doER             (false)
        , doCalcReach      (true)
        , doAsymmTE        (true)
        , doOTFGateShorten (true)
        , doShortenWithOrGates(true)
        , doRemClWithAndGates(true)
        , doFindEqLitsWithGates(true)
        , doMixXorAndGates (false)

        //Verbosity
        , doPrintGateDot   (false)
        , doPrintConflDot  (false)

        , needToDumpLearnts(false)
        , needToDumpOrig   (false)
        , maxDumpLearntsSize(std::numeric_limits<uint32_t>::max())
        , libraryUsage     (true)
        , greedyUnbound    (false)
        , origSeed(0)
{
}
