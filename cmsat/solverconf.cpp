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
using namespace CMSat;

SolverConf::SolverConf() :
        //Variable activities
        var_inc_start(128)
        , var_inc_multiplier(11)
        , var_inc_divider(10)
        , var_inc_variability(0)
        , random_var_freq(0)
        , polarity_mode(polarity_auto)

        //Clause cleaning
        , clauseCleaningType(CLEAN_CLAUSES_PROPCONFL_BASED)
        , doPreClauseCleanPropAndConfl(false)
        , preClauseCleanLimit(2)
        , preCleanMinConflTime(10000)
        , doClearStatEveryClauseCleaning(true)
        , ratioRemoveClauses(0.5)
        , numCleanBetweenSimplify(2)
        , startClean(10000)
        , increaseClean(1.1)
        , maxNumLearntsRatio(10)
        , clauseDecayActivity(1.0/0.999)

        //Restarting
        , restart_first(300)
        , restart_inc(2)
        , burstSearchLen(300)
        , restartType(auto_restart)

        //Clause minimisation
        , doRecursiveMinim (true)
        , doMinimLearntMore(true)
        , doAlwaysFMinim   (false)
        , moreMinimLimit   (300)

        //Verbosity
        , verbosity        (0)
        , doPrintGateDot   (false)
        , doPrintConflDot  (false)
        , printFullStats   (false)
        , verbStats        (0)

        //Limits
        , maxTime          (std::numeric_limits<double>::max())
        , maxConfl         (std::numeric_limits<size_t>::max())

        //Agilities
        , agilityG                  (0.9999)
        , agilityLimit              (0.03)
        , agilityViolationLimit     (20)

        //Glues
        , updateGlues(true)
        , shortTermHistorySize (100)

        //OTF
        , otfHyperbin      (true)
        #ifdef DRUP
        , doOTFSubsume     (false)
        #else
        , doOTFSubsume     (true)
        #endif
        , doOTFGateShorten (true)
        , rewardShortenedClauseWithConfl(3)

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
        , varElimRatioPerIter(0.14)

        //Probing
        , doProbe          (true)
        , probeMultiplier  (1.0)
        , doBothProp       (true)
        , doTransRed       (true)
        #ifdef DRUP
        , doStamp          (false)
        , doCache          (false)
        #else
        , doStamp          (true)
        , doCache          (true)
        #endif
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
        , flipPolarFreq    (300)

        //Simplifier
        , doSimplify       (true)
        , doSchedSimpProblem(true)
        , doPreSchedSimpProblem (true)
        , doSubsume1       (true)
        , doBlockClauses   (true)
        , doAsymmTE        (true)
        , maxRedLinkInSize (200)
        , maxOccurIrredMB  (800)
        , maxOccurRedMB    (800)
        , maxOccurRedLitLinkedM(50)

        //optimisations to do
        #ifdef DRUP
        , doRenumberVars   (false)
        #else
        , doRenumberVars   (true)
        #endif

        //Component finding
        , doFindComps     (false)
        #ifdef DRUP
        , doCompHandler    (false)
        #else
        , doCompHandler    (true)
        #endif
        , handlerFromSimpNum (0)
        , compVarLimit      (1ULL*1000ULL*1000ULL)
        , compFindLimitMega (500)

        , doExtBinSubs     (true)
        , doClausVivif     (true)
        , doSortWatched    (true)
        , doStrSubImplicit (true)


        , doGateFind       (false)
        , maxGateSize      (20)

        , doER             (false)
        , doCalcReach      (true)
        , doShortenWithOrGates(true)
        , doRemClWithAndGates(true)
        , doFindEqLitsWithGates(true)
        , doMixXorAndGates (false)

        , needToDumpLearnts(false)
        , needToDumpSimplified (false)
        , needResultFile       (false)
        , maxDumpLearntsSize(std::numeric_limits<uint32_t>::max())
        , libraryUsage     (true)
        , origSeed(0)
{
}
