/*****************************************************************************
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "SolverConf.h"
#include <limits>

SolverConf::SolverConf() :
        random_var_freq(0.02)
        , restart_first(100)
        , restart_inc(1.5)
        , learntsize_factor((double)1/(double)3)
        , agilityG(0.9999)
        , agilityLimit(0.1)

        , expensive_ccmin  (true)
        , polarity_mode    (polarity_auto)
        , verbosity        (0)
        , restrictPickBranch(0)

        //Simplification
        , simpBurstSConf(NUM_CONFL_BURST_SEARCH)
        , simpStartMult(SIMPLIFY_MULTIPLIER)
        , simpStartMMult(SIMPLIFY_MULTIPLIER_MULTIPLIER)
        , doPerformPreSimp (true)
        , failedLitMultiplier(2.0)
        , syncEveryConf(SYNC_EVERY_CONFL)

        //optimisations to do
        , doFindXors       (true)
        , doFindEqLits     (true)
        , doRegFindEqLits  (true)
        , doReplace        (true)
        , doConglXors      (true)
        , doHeuleProcess   (true)
        , doSchedSimp      (true)
        , doSatELite       (true)
        , doXorSubsumption (true)
        , doPartHandler    (true)
        , doHyperBinRes    (true)
        , doBlockedClause  (false)
        , doVarElim        (true)
        , doSubsume1       (true)
        , doClausVivif     (true)
        , doSortWatched    (true)
        , doMinimLearntMore(true)
        , doMinimLMoreRecur(true)
        , doFailedLit      (true)
        , doRemUselessBins (true)
        , doSubsWBins      (true)
        , doSubsWNonExistBins(true)
        , doRemUselessLBins(true)
        , doMaxGlueDel     (false)
        , doPrintAvgBranch (false)
        , doCacheOTFSSR    (true)
        , doCacheNLBins    (true)
        , doExtendedSCC    (true)
        , doGateFind       (true)
        , doAlwaysFMinim   (true)
        , doER             (false)
        , doCalcReach      (true)

        , maxRestarts      (std::numeric_limits<uint32_t>::max())
        , needToDumpLearnts(false)
        , needToDumpOrig   (false)
        , maxDumpLearntsSize(std::numeric_limits<uint32_t>::max())
        , libraryUsage     (true)
        , greedyUnbound    (false)
        , maxGlue          (DEFAULT_MAX_GLUE)
        , fixRestartType   (auto_restart)
        , origSeed(0)
{
}
