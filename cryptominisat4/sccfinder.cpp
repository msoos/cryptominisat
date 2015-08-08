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

#include <iostream>
#include <vector>
#include <iomanip>

#include "solvertypes.h"
#include "sccfinder.h"
#include "varreplacer.h"
#include "time_mem.h"
#include "solver.h"
#include "sqlstats.h"

using namespace CMSat;
using std::cout;
using std::endl;

SCCFinder::SCCFinder(Solver* _solver) :
    globalIndex(0)
    , solver(_solver)
{}

bool SCCFinder::performSCC(uint64_t* bogoprops_given)
{
    assert(binxors.empty());
    runStats.clear();
    runStats.numCalls = 1;
    const double myTime = cpuTime();

    globalIndex = 0;
    index.clear();
    index.resize(solver->nVars()*2, std::numeric_limits<uint32_t>::max());
    lowlink.clear();
    lowlink.resize(solver->nVars()*2, std::numeric_limits<uint32_t>::max());
    stackIndicator.clear();
    stackIndicator.resize(solver->nVars()*2, false);
    assert(stack.empty());

    for (uint32_t vertex = 0; vertex < solver->nVars()*2; vertex++) {
        //Start a DFS at each node we haven't visited yet
        const Var v = vertex>>1;
        if (solver->value(v) != l_Undef) {
            continue;
        }
        if (index[vertex] == std::numeric_limits<uint32_t>::max()) {
            tarjan(vertex);
            assert(stack.empty());
        }
    }

    //Update & print stats
    runStats.cpu_time = cpuTime() - myTime;
    runStats.foundXorsNew = binxors.size();
    if (solver->conf.verbosity >= 1) {
        if (solver->conf.verbosity >= 3)
            runStats.print();
        else
            runStats.print_short(solver);
    }
    globalStats += runStats;
    solver->binTri.numNewBinsSinceSCC = 0;

    if (bogoprops_given) {
        *bogoprops_given += runStats.bogoprops;
    }

    return solver->ok;
}

void SCCFinder::tarjan(const uint32_t vertex)
{
    runStats.bogoprops += 1;
    index[vertex] = globalIndex;  // Set the depth index for v
    lowlink[vertex] = globalIndex;
    globalIndex++;
    stack.push(vertex); // Push v on the stack
    stackIndicator[vertex] = true;

    Var vertexVar = Lit::toLit(vertex).var();
    if (solver->varData[vertexVar].removed == Removed::none) {
        Lit vertLit = Lit::toLit(vertex);

        vector<LitExtra>* transCache = NULL;

        if (solver->conf.doCache
            && solver->conf.doExtendedSCC
            && (!solver->drup->enabled() || solver->conf.otfHyperbin)
        ) {
            transCache = &(solver->implCache[(~vertLit).toInt()].lits);
            __builtin_prefetch(transCache->data());
        }

        //Go through the watch
        watch_subarray_const ws = solver->watches[(~vertLit).toInt()];
        runStats.bogoprops += ws.size()/4;
        for (watch_subarray_const::const_iterator
            it = ws.begin(), end = ws.end()
            ; it != end
            ; ++it
        ) {
            //Only binary clauses matter
            if (!it->isBinary())
                continue;

            const Lit lit = it->lit2();
            if (solver->value(lit) != l_Undef) {
                continue;
            }
            doit(lit, vertex);
        }

        if (transCache) {
            runStats.bogoprops += transCache->size()/4;
            for (vector<LitExtra>::iterator
                it = transCache->begin(), end = transCache->end()
                ; it != end
                ; ++it
            ) {
                Lit lit = it->getLit();
                if (solver->value(lit) != l_Undef) {
                    continue;
                }
                if (lit != ~vertLit) {
                    doit(lit, vertex);
                }
            }
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
            tmp.push_back(vprime);
        } while (vprime != vertex);
        if (tmp.size() >= 2) {
            runStats.bogoprops += 3;
            for (uint32_t i = 1; i < tmp.size(); i++) {
                if (!solver->ok) {
                    break;
                }

                bool rhs = Lit::toLit(tmp[0]).sign()
                    ^ Lit::toLit(tmp[i]).sign();

                BinaryXor binxor(Lit::toLit(tmp[0]).var(), Lit::toLit(tmp[i]).var(), rhs);
                binxors.insert(binxor);

                //Both are UNDEF, so this is a proper binary XOR
                if (solver->value(binxor.vars[0]) == l_Undef
                    && solver->value(binxor.vars[1]) == l_Undef
                ) {
                    runStats.foundXors++;
                    #ifdef VERBOSE_DEBUG
                    cout << "SCC says: "
                    << binxor.vars[0] +1
                    << " XOR "
                    << binxor.vars[1] +1
                    << " = " << binxor.rhs
                    << endl;
                    #endif
                }
            }
        }
    }
}

void SCCFinder::Stats::print_short(Solver* solver) const
{
    cout
    << "c [scc]"
    << " new: " << foundXorsNew
    << " BP " << bogoprops/(1000*1000) << "M";
    if (solver) {
        cout << solver->conf.print_times(cpu_time);
    } else {
        cout << "  T: " << std::setprecision(2) << std::fixed << cpu_time;
    }
    cout << endl;

    if (solver && solver->sqlStats) {
        solver->sqlStats->time_passed_min(
            solver
            , "scc"
            , cpu_time
        );
    }
}

size_t SCCFinder::mem_used() const
{
    size_t mem = 0;
    mem += index.capacity()*sizeof(uint32_t);
    mem += lowlink.capacity()*sizeof(uint32_t);
    mem += stack.size()*sizeof(uint32_t); //TODO under-estimates
    mem += stackIndicator.capacity()*sizeof(char);
    mem += tmp.capacity()*sizeof(uint32_t);

    return mem;
}
