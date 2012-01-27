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
        double getTotalTime() const;

    private:
        //Main
        bool tryBoth(const Lit lit);
        void printResults(const double myTime) const;
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
        vector<char> cacheUpdated;
        set<BinaryClause> uselessBin;
        uint32_t addedBin;
        uint32_t removedBins;
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

        //State for this run
        /**
        @brief Records num. var-replacement istructions between 2-long xor findings through longer xor shortening

        Finding 2-long xor claues by shortening is fine, but sometimes we find
        the same thing, or find something that is trivially a consequence of
        other 2-long xors that we already know. To filter out these bogus
        "findigs" from the statistics reported, we save in this value the
        real var-replacement insturctions before and after the 2-long xor
        finding through longer xor-shortening, and then compare the changes
        made
        */
        size_t origTrailSize;
        uint64_t origBogoProps; ///<Records num. of bogoprops at the start-up of search()
        uint32_t numFailed;     ///<Records num. of failed literals during search()
        uint32_t goodBothSame;  ///<Records num. of literals that have been propagated to the same value by both "var" and "~var"

        //State between runs
        double totalTime;
        double numPropsMultiplier; ///<If last time we called search() all went fine, then this is incremented, so we do more searching this time
        uint32_t lastTimeFoundTruths; ///<Records how many unit clauses we found last time we called search()
        uint32_t numCalls; ///<Number of times search() has been called
};

inline double FailedLitSearcher::getTotalTime() const
{
    return totalTime;
}


#endif //FAILEDVARSEARCHER_H

