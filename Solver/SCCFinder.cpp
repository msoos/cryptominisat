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
    solver(_solver)
{}

const bool SCCFinder::find2LongXors()
{
    double myTime = cpuTime();
    uint32_t oldNumReplace = solver.varReplacer->getNewToReplaceVars();

    globalIndex = 0;
    index.clear();
    index.resize(solver.nVars()*2, std::numeric_limits<uint32_t>::max());
    lowlink.clear();
    lowlink.resize(solver.nVars()*2, std::numeric_limits<uint32_t>::max());
    stackIndicator.clear();
    stackIndicator.growTo(solver.nVars()*2, false);
    assert(stack.empty());

    for (uint32_t vertex = 0; vertex < solver.nVars()*2; vertex++) {
        //Start a DFS at each node we haven't visited yet
        if (index[vertex] == std::numeric_limits<uint32_t>::max())
             tarjan(vertex);
    }

    if (solver.conf.verbosity  > 2 || (solver.conflicts == 0 && solver.conf.verbosity  >= 1)) {
        std::cout << "c Finding binary XORs  T: "
        << std::fixed << std::setprecision(2) << std::setw(8) <<  (cpuTime() - myTime) << " s"
        << "  found: " << std::setw(7) << solver.varReplacer->getNewToReplaceVars() - oldNumReplace
        << std::endl;
    }

    return solver.ok;
}

void SCCFinder::tarjan(uint32_t vertex)
{
    index[vertex] = globalIndex;  // Set the depth index for v
    lowlink[vertex] = globalIndex;
    globalIndex++;
    stack.push(vertex); // Push v on the stack
    stackIndicator[vertex] = true;

    vec<Watched>& ws = solver.watches[(Lit::toLit(vertex)).toInt()];
    // Consider successors of v
    for (Watched *it = ws.getData(), *end = ws.getDataEnd(); it != end; it++) {
        if (!it->isBinary()) continue;
        Lit lit = it->getOtherLit();

        // Was successor v' visited?
        if (index[lit.toInt()] ==  std::numeric_limits<uint32_t>::max()) {
            tarjan(lit.toInt());
            lowlink[vertex] = std::min(lowlink[vertex], lowlink[lit.toInt()]);
        } else if (stackIndicator[lit.toInt()])  {
            lowlink[vertex] = std::min(lowlink[vertex], lowlink[lit.toInt()]);
        }
    }

    // Is v the root of an SCC?
    if (lowlink[vertex] == index[vertex]) {
        uint32_t vprime;
        tmp.clear();
        do {
            assert(!stack.empty());
            vprime = stack.top();
            stack.pop();
            stackIndicator[vprime] = false;
            tmp.push(vprime);
        } while (vprime != vertex);
        if (tmp.size() >= 2) {
            for (uint32_t i = 1; i < tmp.size(); i++) {
                Lit lit1 = Lit::toLit(tmp[0]);
                Lit lit2 = Lit::toLit(tmp[i]);
                bool inverted = lit1.sign() ^ lit2.sign() ^ true;
                if (solver.ok) {
                    vec<Lit> lits(2);
                    lits[0] = lit1.unsign();
                    lits[1] = lit2.unsign();
                    solver.addXorClause(lits, inverted);
                }
            }
        }
    }
}
