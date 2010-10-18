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

#ifndef SCCFINDER_H
#define SCCFINDER_H

#include "Vec.h"
#include "Clause.h"
#include "Solver.h"

#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/graph/strong_components.hpp>

class SCCFinder {
    public:
        SCCFinder(Solver& solver);
        const bool find2LongXors();

    private:
        boost::adjacency_list <boost::vecS, boost::vecS, boost::directedS> graph;
        void fillGraph();
        void readGraph();
        const bool findSCC();

        uint32_t numXors;
        Solver& solver;
};

#endif //SCCFINDER_H