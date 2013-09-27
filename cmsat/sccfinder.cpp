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

#include <iostream>
#include <vector>
#include <iomanip>

#include "solvertypes.h"
#include "sccfinder.h"
#include "varreplacer.h"
#include "time_mem.h"
#include "solver.h"

using namespace CMSat;
using std::cout;
using std::endl;

SCCFinder::SCCFinder(Solver* _solver) :
    globalIndex(0)
    , solver(_solver)
{}

bool SCCFinder::performSCC()
{
    runStats.clear();
    runStats.numCalls = 1;
    const double myTime = cpuTime();
    size_t oldNumReplace = solver->varReplacer->getNewToReplaceVars();

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
        if (index[vertex] == std::numeric_limits<uint32_t>::max()) {
            tarjan(vertex);
            assert(stack.empty());
        }
    }

    if (solver->ok)
        solver->varReplacer->addLaterAddBinXor();

    //Update & print stats
    runStats.cpu_time = cpuTime() - myTime;
    runStats.foundXorsNew = solver->varReplacer->getNewToReplaceVars() - oldNumReplace;
    if (solver->conf.verbosity >= 1) {
        if (solver->conf.verbosity >= 3)
            runStats.print();
        else
            runStats.printShort();
    }
    globalStats += runStats;
    solver->binTri.numNewBinsSinceSCC = 0;

    return solver->ok;
}

void SCCFinder::tarjan(const uint32_t vertex)
{
    index[vertex] = globalIndex;  // Set the depth index for v
    lowlink[vertex] = globalIndex;
    globalIndex++;
    stack.push(vertex); // Push v on the stack
    stackIndicator[vertex] = true;

    Var vertexVar = Lit::toLit(vertex).var();
    if (solver->varData[vertexVar].removed == Removed::none
        || solver->varData[vertexVar].removed == Removed::queued_replacer
    ) {
        Lit vertLit = Lit::toLit(vertex);

        vector<LitExtra>* transCache = NULL;

        if (solver->conf.doCache) {
            transCache = &(solver->implCache[(~vertLit).toInt()].lits);
        }

        //Prefetch cache in case we are doing extended SCC
        if (solver->conf.doExtendedSCC
            && transCache
            && transCache->size() > 0
        ) {
            __builtin_prefetch(transCache->data());
        }


        //Go through the watch
        const vec<Watched>& ws = solver->watches[(~vertLit).toInt()];
        for (vec<Watched>::const_iterator
            it = ws.begin(), end = ws.end()
            ; it != end
            ; it++
        ) {
            //Only binary clauses matter
            if (!it->isBinary())
                continue;

            const Lit lit = it->lit2();

            doit(lit, vertex);
        }

        if (transCache) {
            for (vector<LitExtra>::iterator
                it = transCache->begin(), end = transCache->end()
                ; it != end
                ; it++
            ) {
                Lit lit = it->getLit();
                if (lit != ~vertLit) doit(lit, vertex);
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
            for (uint32_t i = 1; i < tmp.size(); i++) {
                if (!solver->ok) break;
                vector<Lit> lits(2);
                lits[0] = Lit::toLit(tmp[0]).unsign();
                lits[1] = Lit::toLit(tmp[i]).unsign();
                const bool xorEqualsFalse = Lit::toLit(tmp[0]).sign()
                                            ^ Lit::toLit(tmp[i]).sign()
                                            ^ true;

                //Both are UNDEF, so this is a proper binary XOR
                if (solver->value(lits[0]) == l_Undef
                    && solver->value(lits[1]) == l_Undef
                ) {
                    runStats.foundXors++;
                    #ifdef VERBOSE_DEBUG
                    cout << "SCC says: "
                    << lits[0]
                    << " XOR "
                    << lits[1]
                    << " = " << !xorEqualsFalse
                    << endl;
                    #endif
                    solver->varReplacer->replace(
                        lits[0]
                        , lits[1]
                        , xorEqualsFalse
                        //Because otherwise queued varreplacer could be reducible
                        //and during var-elim, we would remove one of the binary clauses
                        //and then we would be in a giant mess: the equivalence is stored in replacer
                        //but if its parent/child is set, the child/parent won't be set :S
                        , true
                    );
                }
            }
        }
    }
}

uint64_t SCCFinder::memUsed() const
{
    uint64_t mem = 0;
    mem += index.capacity()*sizeof(uint32_t);
    mem += lowlink.capacity()*sizeof(uint32_t);
    mem += stack.size()*sizeof(uint32_t); //TODO under-estimates
    mem += stackIndicator.capacity()*sizeof(char);
    mem += tmp.capacity()*sizeof(uint32_t);

    return mem;
}
