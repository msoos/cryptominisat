/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
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

#ifndef SCCFINDER_H
#define SCCFINDER_H

#include "clause.h"
#include <stack>
#include <set>

namespace CMSat {

class Solver;

class SCCFinder {
    public:
        SCCFinder(Solver* _solver);
        bool performSCC(uint64_t* bogoprops_given = NULL);
        const std::set<BinaryXor>& get_binxors() const;
        size_t get_num_binxors_found() const;
        void clear_binxors();

        struct Stats
        {
            void clear()
            {
                Stats _tmp;
                *this = _tmp;
            }

            uint64_t numCalls = 0;
            double cpu_time = 0.0;
            uint64_t foundXors = 0;
            uint64_t foundXorsNew = 0;
            uint64_t bogoprops = 0;

            Stats& operator+=(const Stats& other)
            {
                numCalls += other.numCalls;
                cpu_time += other.cpu_time;
                foundXors += other.foundXors;
                foundXorsNew += other.foundXorsNew;
                bogoprops += other.bogoprops;

                return *this;
            }

            void print() const
            {
                cout << "c ----- SCC STATS --------" << endl;
                print_stats_line("c time"
                    , cpu_time
                    , cpu_time/(double)numCalls
                    , "per call"
                );

                print_stats_line("c called"
                    , numCalls
                    , (double)foundXorsNew/(double)numCalls
                    , "new found per call"
                );

                print_stats_line("c found"
                    , foundXorsNew
                    , stats_line_percent(foundXorsNew, foundXors)
                    , "% of all found"
                );

                print_stats_line("c bogoprops"
                    , bogoprops
                    , "% of all found"
                );

                cout << "c ----- SCC STATS END --------" << endl;
            }

            void print_short(Solver* solver) const;
        };

        const Stats& get_stats() const;
        size_t mem_used() const;

    private:
        void tarjan(const uint32_t vertex);
        void doit(const Lit lit, const uint32_t vertex);

        //temporaries
        uint32_t globalIndex;
        vector<uint32_t> index;
        vector<uint32_t> lowlink;
        std::stack<uint32_t, vector<uint32_t> > stack;
        vector<char> stackIndicator;
        vector<uint32_t> tmp;

        Solver* solver;
        std::set<BinaryXor> binxors;

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

inline const SCCFinder::Stats& SCCFinder::get_stats() const
{
    return globalStats;
}

inline const std::set<BinaryXor>& SCCFinder::get_binxors() const
{
    return binxors;
}

inline size_t SCCFinder::get_num_binxors_found() const
{
    return binxors.size();
}

inline void SCCFinder::clear_binxors()
{
    binxors.clear();
}

} //end namespaceC

#endif //SCCFINDER_H
