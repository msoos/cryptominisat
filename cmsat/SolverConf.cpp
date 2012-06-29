/***************************************************************************************[Solver.cc]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2009, Niklas Sorensson
Copyright (c) 2009-2012, Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include "cmsat/SolverConf.h"
#include <limits>

using namespace CMSat;

SolverConf::SolverConf() :
        random_var_freq(0.001)
        , clause_decay (1 / 0.999)
        , restart_first(100)
        , restart_inc(1.5)
        , learntsize_factor((double)1/(double)3)

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
        #ifdef ENABLE_UNWIND_GLUE
        , doMaxGlueDel     (false)
        #endif //ENABLE_UNWIND_GLUE
        , doPrintAvgBranch (false)
        , doCacheOTFSSR    (true)
        , doCacheOTFSSRSet (true)
        , doExtendedSCC    (false)
        , doCalcReach      (true)
        , doBXor           (true)
        , doOTFSubsume     (true)
        , maxConfl         (std::numeric_limits<uint64_t>::max())
        , isPlain          (false)

        , maxRestarts      (std::numeric_limits<uint32_t>::max())
        , needToDumpLearnts(false)
        , needToDumpOrig   (false)
        , maxDumpLearntsSize(std::numeric_limits<uint32_t>::max())
        , libraryUsage     (true)
        , greedyUnbound    (false)
        #ifdef ENABLE_UNWIND_GLUE
        , maxGlue          (DEFAULT_MAX_GLUE)
        #endif //ENABLE_UNWIND_GLUE
        , fixRestartType   (auto_restart)
        , origSeed(0)

        #ifdef CMSAT_HAVE_MYSQL
        , serverConn(NULL)
        #endif
{
}
