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

#include <iostream>
#include <vector>
#include "../Solver/SolverTypes.h"
#include "SCCFinder.h"
#include "VarReplacer.h"
#include <iomanip>
#include "time_mem.h"

SCCFinder::SCCFinder(Solver& _solver) :
    graph(_solver.nVars()*2)
    , solver(_solver)
{}

void SCCFinder::fillGraph()
{
    for (Clause **it = solver.binaryClauses.getData(), **end = solver.binaryClauses.getDataEnd(); it != end; it++) {
        Clause& c = **it;
        Lit lit1 = c[0];
        Lit lit2 = c[1];
        boost::add_edge((~lit1).toInt(), (lit2).toInt(), graph);
        boost::add_edge((~lit2).toInt(), (lit1).toInt(), graph);
    }
}

void SCCFinder::readGraph()
{
    boost::graph_traits < boost::adjacency_list <> >::vertex_iterator it, end;
    boost::graph_traits < boost::adjacency_list <> >::adjacency_iterator ai, a_end;
    boost::property_map < boost::adjacency_list <>, boost::vertex_index_t >::type
    index_map = get(boost::vertex_index, graph);

    for (boost::tie(it, end) = boost::vertices(graph); it != end; ++it) {
        std::cout << Lit::toLit(get(index_map, *it));
        boost::tie(ai, a_end) = boost::adjacent_vertices(*it, graph);
        if (ai == a_end)
            std::cout << " has no children";
        else
            std::cout << " is the parent of ";
        for (; ai != a_end; ++ai) {
            std::cout << Lit::toLit(get(index_map, *ai));
            if (boost::next(ai) != a_end)   std::cout << ", ";
        }
        std::cout << std::endl;
    }
}

const bool SCCFinder::find2LongXors()
{
    double myTime = cpuTime();
    numXors = 0;

    fillGraph();
    const bool retval = findSCC();

    if (solver.verbosity >= 2 || (solver.conflicts == 0 && solver.verbosity >= 1)) {
        std::cout << "c Finding binary XORs  T: "
        << std::fixed << std::setprecision(2) << std::setw(8) <<  (cpuTime() - myTime) << " s"
        << "  found: " << std::setw(7) << (numXors/2)
        << std::endl;
    }

    return retval;
}

const bool SCCFinder::findSCC()
{
    std::vector<uint32_t> component(boost::num_vertices(graph));
    std::vector<uint32_t> root(boost::num_vertices(graph));
    boost::strong_components(graph, &component[0], boost::root_map(&root[0]));

    vec<Lit> lits(2);
    for (std::vector<uint32_t>::size_type i = 0; i != component.size(); i++) {
        if (i != boost::get(&root[0], i)) {
            //std::cout << "Vertex " << Lit::toLit(i) << " is in component " << component[i] << std::endl;
            //std::cout << "Its root vertex is:" << Lit::toLit(boost::get(&root[0], i)) << std::endl;
            lits[0] = Lit::toLit(i);
            lits[1] = Lit::toLit(boost::get(&root[0], i));
            bool xorEqualsFalse = true;
            xorEqualsFalse ^= lits[0].sign() ^ lits[1].sign();
            lits[0] = lits[0].unsign();
            lits[1] = lits[1].unsign();

            if (!solver.varReplacer->replace(lits, xorEqualsFalse, 0))
                return false;
            numXors++;
        }
    }

    return true;
}
