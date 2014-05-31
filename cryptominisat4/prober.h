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

#ifndef __PROBER_H__
#define __PROBER_H__

#include <set>
#include <map>
#include <vector>

#include "solvertypes.h"
#include "clause.h"

namespace CMSat {

using std::set;
using std::map;
using std::vector;

class Solver;

//#define DEBUG_REMOVE_USELESS_BIN

/**
@brief Responsible for doing failed var searching and related algorithms

Performs in seach():
1) Failed lit searching
2) Searching for lits that have been propagated by both "var" and "~var"
3) 2-long Xor clauses that have been found because when propagating "var" and
   "~var", they have been produced by normal xor-clauses shortening to this xor
   clause
4) If var1 propagates var2 and ~var1 propagates ~var2, then var=var2, and this
   is a 2-long XOR clause, this 2-long xor is added
5) Hyper-binary resolution

Perfoms in asymmBranch(): asymmetric branching, heuristically. Best paper
on this is 'Vivifying Propositional Clausal Formulae', though we do it much
more heuristically
*/
class Prober {
    public:
        Prober(Solver* _solver);

        bool probe();

        struct Stats
        {
            Stats() :
                //Time
                cpu_time(0)
                , timeAllocated(0)
                , numCalls(0)

                //Probe stats
                , numFailed(0)
                , numProbed(0)
                , numLoopIters(0)
                , numVarProbed(0)
                , numVisited(0)
                , zeroDepthAssigns(0)

                //Bins
                , addedBin(0)
                , removedIrredBin(0)
                , removedRedBin(0)

                //Compare against
                , origNumFreeVars(0)
                , origNumBins(0)

                //bothProp
                , bothSameAdded(0)
            {}

            void clear()
            {
                Stats tmp;
                *this = tmp;
            }

            Stats& operator +=(const Stats& other)
            {
                //Time
                cpu_time += other.cpu_time;
                timeAllocated += other.timeAllocated;
                numCalls += other.numCalls;

                //Probe stats
                numFailed += other.numFailed;
                numProbed += other.numProbed;
                numLoopIters += other.numLoopIters;
                numVarProbed += other.numVarProbed;
                numVisited += other.numVisited;
                zeroDepthAssigns += other.zeroDepthAssigns;

                //Propagation stats
                propStats += other.propStats;
                conflStats += other.conflStats;

                //Binary clause
                addedBin += other.addedBin;
                removedIrredBin += other.removedIrredBin;
                removedRedBin += other.removedRedBin;

                //Compare against
                origNumFreeVars += other.origNumFreeVars;
                origNumBins += other.origNumBins;

                //Bothprop
                bothSameAdded += other.bothSameAdded;

                return *this;
            }

            void print(const size_t nVars) const
            {
                cout << "c -------- PROBE STATS ----------" << endl;
                printStatsLine("c probe time"
                    , cpu_time
                    , (double)timeAllocated/(cpu_time*1000.0*1000.0)
                    , "(Mega BP+HP)/s"
                );

                printStatsLine("c called"
                    , numCalls
                    , cpu_time/(double)numCalls
                    , "s/call"
                );

                printStatsLine("c unused Mega BP+HP"
                    , (double)(timeAllocated - (propStats.bogoProps + propStats.otfHyperTime))/(1000.0*1000.0)
                    , (cpu_time/(double)(propStats.bogoProps + propStats.otfHyperTime))*(double)(timeAllocated - (propStats.bogoProps + propStats.otfHyperTime))
                    , "est. secs"
                );

                printStatsLine("c 0-depth-assigns"
                    , zeroDepthAssigns
                    , stats_line_percent(zeroDepthAssigns, nVars)
                    , "% vars");

                printStatsLine("c bothsame"
                    , bothSameAdded
                    , stats_line_percent(bothSameAdded, numVisited)
                    , "% visited"
                );

                printStatsLine("c probed"
                    , numProbed
                    , (double)numProbed/cpu_time
                    , "probe/sec"
                );

                printStatsLine("c loop iters"
                    , numLoopIters
                    , stats_line_percent(numVarProbed, numLoopIters)
                    , "% var probed"
                );

                printStatsLine("c failed"
                    , numFailed
                    , stats_line_percent(numFailed, numProbed)
                    , "% of probes"
                );

                printStatsLine("c visited"
                    , (double)numVisited/(1000.0*1000.0)
                    , "M lits"
                    , stats_line_percent(numVisited, origNumFreeVars*2)
                    , "% of available lits"
                );

//                 printStatsLine("c probe failed"
//                     , numFailed
//                     , (double)numFailed/(double)nVars*100.0
//                     , "% vars");

                printStatsLine("c bin add"
                    , addedBin
                    , stats_line_percent(addedBin, origNumBins)
                    , "% of bins"
                );

                printStatsLine("c irred bin rem"
                    , removedIrredBin
                    , stats_line_percent(removedIrredBin, origNumBins)
                    , "% of bins"
                );

                printStatsLine("c red bin rem"
                    , removedRedBin
                    , stats_line_percent(removedRedBin, origNumBins)
                    , "% of bins"
                );

                printStatsLine("c time"
                    , cpu_time
                    , "s");

                conflStats.print(cpu_time);
                propStats.print(cpu_time);
                cout << "c -------- PROBE STATS END ----------" << endl;
            }

            void printShort() const
            {
                cout
                << "c [probe]"
                << " 0-depth assigns: " << zeroDepthAssigns
                << " bsame: " << bothSameAdded
                << " Flit: " << numFailed

                // x2 because it's LITERAL visit
                << " Visited: " << numVisited << "/" << (origNumFreeVars*2)
                << "(" << std::setprecision(1)
                << stats_line_percent(numVisited, origNumFreeVars*2)
                << "%)"
                << endl;

                cout
                << "c [probe]"
                << " probed: " << numProbed
                << "(" << std::setprecision(1)
                // x2 because it's LITERAL probed
                << stats_line_percent(numProbed, origNumFreeVars*2)
                << "%)"

                << " hyperBin:" << addedBin
                << " transR-Irred:" << removedIrredBin
                << " transR-Red:" << removedRedBin
                << endl;

                cout
                << "c [probe]"
                << " BP: " << std::fixed << std::setprecision(1)
                << (double)(propStats.bogoProps)/1000000.0  << "M"
                << " HP: " << std::fixed << std::setprecision(1)
                << (double)(propStats.otfHyperTime)/1000000.0  << "M"

                << " T: " << std::fixed << std::setprecision(2)
                << cpu_time
                << endl;
            }

            //Time
            double cpu_time;
            uint64_t timeAllocated;
            uint64_t numCalls;

            //Probe stats
            uint64_t numFailed;
            uint64_t numProbed;
            uint64_t numLoopIters;
            uint64_t numVarProbed;
            uint64_t numVisited;
            uint64_t zeroDepthAssigns;

            //Propagation stats
            PropStats propStats;
            ConflStats conflStats;

            //Binary clause
            uint64_t addedBin;
            uint64_t removedIrredBin;
            uint64_t removedRedBin;

            //Compare against
            uint64_t origNumFreeVars;
            uint64_t origNumBins;

            //Bothprop
            uint64_t bothSameAdded;
        };

        const Stats& getStats() const;
        size_t memUsed() const;

    private:
        //Main
        bool tryThis(const Lit lit, const bool first);
        vector<char> visitedAlready;
        Solver* solver; ///<The solver we are updating&working with
        void checkOTFRatio();
        uint64_t limit_used() const;
        void reset_stats_and_state();
        uint64_t calc_numpropstodo();
        void clean_clauses_before_probe();
        uint64_t update_numpropstodo_based_on_prev_performance(uint64_t numPropsTodo);
        void clean_clauses_after_probe();
        void check_if_must_disable_otf_hyperbin_and_tred(const uint64_t numPropsTodo);
        void check_if_must_disable_cache_update();
        vector<Var> randomize_possible_choices();
        vector<size_t> create_fast_random_lookup(const vector<Var>& poss_choice);
        Lit update_lit_for_dominator(
            Lit lit
            , vector<Var>& poss_choice
            , const vector<size_t>& fast_rnd_lookup
        );
        void update_and_print_stats(const double myTime, const uint64_t numPropsTodo);
        bool check_timeout_due_to_hyperbin();

        //For bothprop
        vector<uint32_t> propagatedBitSet;
        vector<bool> propagated; ///<These lits have been propagated by propagating the lit picked
        vector<bool> propValue; ///<The value (0 or 1) of the lits propagated set in "propagated"
        vector<Lit> toEnqueue;
        vector<Lit> tmp_lits;
        void clearUpBeforeFirstSet();

        void updateCache(Lit thisLit, Lit lit, size_t numElemsSet);
        void checkAndSetBothProp(Var var, bool first);
        void addRestOfLitsToCache(Lit lit);
        void handleFailedLit(Lit lit, Lit failed);

        //For hyper-bin resolution
        #ifdef DEBUG_REMOVE_USELESS_BIN
        void testBinRemoval(const Lit origLit);
        void fillTestUselessBinRemoval(const Lit lit);
        vector<Var> origNLBEnqueuedVars;
        vector<Var> origEnqueuedVars;
        #endif

        //Multi-level
        void calcNegPosDist();
        bool tryMultiLevel(const vector<Var>& vars, uint32_t& enqueued, uint32_t& finished, uint32_t& numFailed);
        bool tryMultiLevelAll();
        void fillToTry(vector<Var>& toTry);

        //Used to count extra time, must be cleared at every startup
        uint64_t extraTime;
        uint64_t extraTimeCache;

        //Stats
        Stats runStats;
        Stats globalStats;

        ///If last time we were successful, do it more
        double numPropsMultiplier;
        ///How successful were we last time?
        uint32_t lastTimeZeroDepthAssings;

};

inline const Prober::Stats& Prober::getStats() const
{
    return globalStats;
}

} //end namespace;

#endif //__PROBER_H__

