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

#ifndef FAILEDLITSEARCHER_H
#define FAILEDLITSEARCHER_H

#include <set>
#include <map>
#include <vector>

#include "SolverTypes.h"
#include "Clause.h"
#include "BitArray.h"

using std::set;
using std::map;
using std::vector;

class ThreadControl;

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
class FailedLitSearcher {
    public:
        FailedLitSearcher(ThreadControl* _control);

        bool search();

        struct Stats
        {
            Stats() :
                //Time
                cpu_time(0)

                //Probe stats
                , numFailed(0)
                , numProbed(0)
                , numVisited(0)
                , zeroDepthAssigns(0)

                //Bins
                , addedBin(0)
                , removedBin(0)

                //Compare against
                , origNumFreeVars(0)
                , origNumBins(0)
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

                //Probe stats
                numFailed += other.numFailed;
                numProbed += other.numProbed;
                numVisited += other.numVisited;
                zeroDepthAssigns += other.zeroDepthAssigns;

                //Propagation stats
                propStats += other.propStats;
                conflStats += other.conflStats;

                //Binary clause
                addedBin += other.addedBin;
                removedBin += other.removedBin;

                //Compare against
                origNumFreeVars += other.origNumFreeVars;
                origNumBins += other.origNumBins;

                return *this;
            }

            void print(const size_t nVars) const
            {
                cout << "c -------- PROBE STATS ----------" << endl;
                printStatsLine("c probe time"
                    , cpu_time
                );

                printStatsLine("c 0-depth-assigns"
                    , zeroDepthAssigns
                    , (double)zeroDepthAssigns/(double)nVars*100.0
                    , "% vars");

                printStatsLine("c probed"
                    , numProbed
                    , (double)numProbed/cpu_time
                    , "probe/sec");

                printStatsLine("c failed"
                    , numFailed
                    , 100.0*(double)numFailed
                    /(double)numProbed
                    , "% of probes");

                printStatsLine("c visited"
                    , (double)numVisited/(1000.0*1000.0)
                    , "M lits"
                    , (100.0*(double)numVisited/(double)(origNumFreeVars*2))
                    , "% of available lits");

//                 printStatsLine("c probe failed"
//                     , numFailed
//                     , (double)numFailed/(double)nVars*100.0
//                     , "% vars");

                printStatsLine("c bin add"
                    , addedBin
                    , (double)addedBin/(double)origNumBins*100.0
                    , "% of bins"
                );

                printStatsLine("c bin rem"
                    , removedBin
                    , (double)removedBin/(double)origNumBins*100.0
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
                << "c"
                << " 0-depth assigns: " << zeroDepthAssigns
                << " Flit: " << numFailed
                << " Visited: " << numVisited << " / " << (origNumFreeVars*2) // x2 because it's LITERAL visit
                << "(" << std::setprecision(1) << (100.0*(double)numVisited/(double)(origNumFreeVars*2)) << "%)"
                << " probed: " << numProbed
                << " Bin:" << addedBin
                << " RemBin:" << removedBin

                << " P: " << std::fixed << std::setprecision(1)
                << (double)(propStats.bogoProps)/1000000.0  << "M"

                << " T: " << std::fixed << std::setprecision(2)
                << cpu_time
                << endl;
            }

            //Time
            double cpu_time;

            //Probe stats
            uint64_t numFailed;
            uint64_t numProbed;
            uint64_t numVisited;
            uint64_t zeroDepthAssigns;

            //Propagation stats
            PropStats propStats;
            ConflStats conflStats;

            //Binary clause
            uint64_t addedBin;
            uint64_t removedBin;

            //Compare against
            uint64_t origNumFreeVars;
            uint64_t origNumBins;
        };

        const Stats& getStats() const;

    private:
        //Main
        bool tryThis(const Lit lit);
        vector<char> visitedAlready;

        ThreadControl* control; ///<The solver we are updating&working with

        /**
        @brief Lits that have been propagated to the same value both by "var" and "~var"

        value that the literal has been propagated to is available in propValue
        */
        vector<Lit> bothSame;

        //2-long xor-finding
        /**
        @brief used to find 2-long xor by shortening longer xors to this size

        -# We propagate "var" and record all xors that become 2-long
        -# We propagate "~var" and record all xors that become 2-long
        -# if (1) and (2) have something in common, we add it as a variable
        replacement instruction

        We must be able to order these 2-long xors, so that we can search
        for matching couples fast. This class is used for that
        */
        class TwoLongXor
        {
        public:
            bool operator==(const TwoLongXor& other) const
            {
                if (var[0] == other.var[0]
                    && var[1] == other.var[1]
                    && inverted == other.inverted)
                    return true;
                return false;
            }
            bool operator<(const TwoLongXor& other) const
            {
                if (var[0] < other.var[0]) return true;
                if (var[0] > other.var[0]) return false;

                if (var[1] < other.var[1]) return true;
                if (var[1] > other.var[1]) return false;

                if (inverted < other.inverted) return true;
                if (inverted > other.inverted) return false;

                return false;
            }

            Var var[2];
            bool inverted;
        };

        //For hyper-bin resolution
        vector<uint32_t> cacheUpdated;
        set<BinaryClause> uselessBin;
        void hyperBinResAll();
        void removeUselessBins();
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

        //Temporaries
        vector<Lit> tmpPs;

        //Used to count extra time, must be cleared at every startup
        size_t extraTime;

        //Stats
        Stats runStats;
        Stats globalStats;

        ///If last time we were successful, do it more
        double numPropsMultiplier;
        ///How successful were we last time?
        uint32_t lastTimeZeroDepthAssings;

        ///How many times we tried to do failed lit probing
        uint32_t numCalls;
};

inline const FailedLitSearcher::Stats& FailedLitSearcher::getStats() const
{
    return globalStats;
}


#endif //FAILEDVARSEARCHER_H

