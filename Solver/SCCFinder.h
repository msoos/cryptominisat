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

#ifndef SCCFINDER_H
#define SCCFINDER_H

#include "Vec.h"
#include "Clause.h"
#include <stack>

class ThreadControl;

class SCCFinder {
    public:
        SCCFinder(ThreadControl* _control);
        bool find2LongXors();
        double getTotalTime() const;

    private:
        void tarjan(const uint32_t vertex);
        void doit(const Lit lit, const uint32_t vertex);

        uint32_t globalIndex;
        vector<uint32_t> index;
        vector<uint32_t> lowlink;
        std::stack<uint32_t> stack;
        vector<char> stackIndicator;
        vector<uint32_t> tmp;

        ThreadControl* control;
        const vector<Lit>& replaceTable;
        double totalTime;
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

inline double SCCFinder::getTotalTime() const
{
    return totalTime;
}

#endif //SCCFINDER_H
