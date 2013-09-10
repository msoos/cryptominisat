/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.0 of the License, or (at your option) any later version.
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

#ifndef SCCFINDER_H
#define SCCFINDER_H

#include "vec.h"
#include "clause.h"
#include <stack>

namespace CMSat {

class Solver;

class SCCFinder {
    public:
        SCCFinder(Solver* _solver);
        bool find2LongXors();

        struct Stats
        {
            Stats() :
                numCalls(0)
                , cpu_time(0)
                , foundXors(0)
                , foundXorsNew(0)
            {}

            void clear()
            {
                Stats tmp;
                *this = tmp;
            }

            uint64_t numCalls;
            double cpu_time;
            uint64_t foundXors;
            uint64_t foundXorsNew;

            Stats& operator+=(const Stats& other)
            {
                numCalls += other.numCalls;
                cpu_time += other.cpu_time;
                foundXors += other.foundXors;
                foundXorsNew += other.foundXorsNew;

                return *this;
            }

            void print() const
            {
                cout << "c ----- SCC STATS --------" << endl;
                printStatsLine("c time"
                    , cpu_time
                    , cpu_time/(double)numCalls
                    , "per call"
                );

                printStatsLine("c called"
                    , numCalls
                    , (double)foundXorsNew/(double)numCalls
                    , "new found per call"
                );

                printStatsLine("c found"
                    , foundXorsNew
                    , (double)foundXorsNew/(double)foundXors
                    , "% of all found"
                );

                cout << "c ----- SCC STATS END --------" << endl;
            }

            void printShort() const
            {
                cout
                << "c SCC"
                //<< " found: " << foundXors
                << " new: " << foundXorsNew
                << " T: " << std::fixed << std::setprecision(2)
                <<  cpu_time << " s"
                << endl;
            }
        };

        const Stats& getStats() const;
        uint64_t memUsed() const;

    private:

        void tarjan(const uint32_t vertex);
        void doit(const Lit lit, const uint32_t vertex);

        uint32_t globalIndex;
        vector<uint32_t> index;
        vector<uint32_t> lowlink;
        std::stack<uint32_t, vector<uint32_t> > stack;
        vector<char> stackIndicator;
        vector<uint32_t> tmp;

        Solver* solver;

        //Stats
        Stats runStats;
        Stats globalStats;
};

inline void SCCFinder::doit(const Lit lit, const uint32_t vertex) {
    // Was successor v' visited?
    if (index[lit.toInt()] ==  std::numeric_limits<uint32_t>::max()) {
        tarjan(lit.toInt());
        lowlink[vertex] = std::min(lowlink[vertex], lowlink[lit.toInt()]);
    } else if (stackIndicator[lit.toInt()])  {
        lowlink[vertex] = std::min(lowlink[vertex], lowlink[lit.toInt()]);
    }
}

inline const SCCFinder::Stats& SCCFinder::getStats() const
{
    return globalStats;
}

} //end namespaceC

#endif //SCCFINDER_H
