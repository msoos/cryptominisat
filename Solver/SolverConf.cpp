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
        , var_inc_multiplier(21)
        , var_inc_divider(20)
        , ratioRemoveClauses(1.0/3.0)
        , random_var_freq(0)
        , restart_first(100)
        , restart_inc(1.2)
        , burstSearchLen(500)
        , restartType(glue_restart)

        , expensive_ccmin  (true)
        , polarity_mode    (polarity_auto)
        , verbosity        (0)

        //Agilities
        , agilityG                  (0.9999)
        , agilityLimit              (0.20)

        //Simplification
        , simpBurstSConf   (500)
        , maxConflBtwSimp  (500000)

        , doPerformPreSimp (true)
        , failedLitMultiplier(1.0)
        , nbClBeforeRedStart(20000)

        //Glues
        , shortTermGlueHistorySize (100)

        //optimisations to do
        , doFindXors       (true)
        , doFindEqLits     (true)
        , doReplace        (true)
        , doSchedSimp      (true)
        , doSatELite       (true)
        , doHyperBinRes    (true)
        , doBlockedClause  (true)
        , doExtBinSubs     (true)
        , doVarElim        (true)
        , doSubsume1       (true)
        , doClausVivif     (true)
        , doSortWatched    (true)
        , doMinimLearntMore(true)
        , doFailedLit      (true)
        , doRemUselessBins (true)
        , doPrintAvgBranch (false)
        , doCache          (true)
        , doExtendedSCC    (false)
        , doGateFind       (true)
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
        , verboseSubsumer  (false)
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
