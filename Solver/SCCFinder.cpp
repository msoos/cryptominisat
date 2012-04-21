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

#include <iostream>
#include <vector>
#include <iomanip>
using std::cout;
using std::endl;

#include "SolverTypes.h"
#include "SCCFinder.h"
#include "VarReplacer.h"
#include "time_mem.h"
#include "ThreadControl.h"

SCCFinder::SCCFinder(ThreadControl* _control) :
    control(_control)
{}

bool SCCFinder::find2LongXors()
{
    runStats.numCalls = 1;
    runStats.clear();
    const double myTime = cpuTime();
    size_t oldNumReplace = control->varReplacer->getNewToReplaceVars();

    globalIndex = 0;
    index.clear();
    index.resize(control->nVars()*2, std::numeric_limits<uint32_t>::max());
    lowlink.clear();
    lowlink.resize(control->nVars()*2, std::numeric_limits<uint32_t>::max());
    stackIndicator.clear();
    stackIndicator.resize(control->nVars()*2, false);
    assert(stack.empty());

    for (uint32_t vertex = 0; vertex < control->nVars()*2; vertex++) {
        //Start a DFS at each node we haven't visited yet
        if (index[vertex] == std::numeric_limits<uint32_t>::max()) {
            tarjan(vertex);
            assert(stack.empty());
        }
    }

    if (control->ok)
        control->varReplacer->addLaterAddBinXor();

    //Update & print stats
    runStats.cpu_time = cpuTime() - myTime;
    runStats.foundXorsNew = control->varReplacer->getNewToReplaceVars() - oldNumReplace;
    if (control->conf.verbosity >= 1) {
        if (control->conf.verbosity >= 3)
            runStats.print();
        else
            runStats.printShort();
    }
    globalStats += runStats;

    return control->ok;
}

void SCCFinder::tarjan(const uint32_t vertex)
{
    index[vertex] = globalIndex;  // Set the depth index for v
    lowlink[vertex] = globalIndex;
    globalIndex++;
    stack.push(vertex); // Push v on the stack
    stackIndicator[vertex] = true;

    Var vertexVar = Lit::toLit(vertex).var();
    if (control->varData[vertexVar].elimed == ELIMED_NONE
        || control->varData[vertexVar].elimed == ELIMED_QUEUED_VARREPLACER
    ) {
        Lit vertLit = Lit::toLit(vertex);
        vector<LitExtra>& transCache = control->implCache[(~vertLit).toInt()].lits;

        //Prefetch cache in case we are doing extended SCC
        if (control->conf.doExtendedSCC
            && transCache.size() > 0
        ) __builtin_prefetch(&transCache[0]);

        //Go through the watch
        const vec<Watched>& ws = control->watches[vertex];
        for (vec<Watched>::const_iterator it = ws.begin(), end = ws.end(); it != end; it++) {
            //Only binary clauses matter
            if (!it->isBinary())
                continue;

            const Lit lit = it->getOtherLit();

            doit(lit, vertex);
        }

        if (control->conf.doExtendedSCC && control->conf.doCache) {
            vector<LitExtra>::iterator it = transCache.begin();
            for (vector<LitExtra>::iterator end = transCache.end(); it != end; it++) {
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
                if (!control->ok) break;
                vector<Lit> lits(2);
                lits[0] = Lit::toLit(tmp[0]).unsign();
                lits[1] = Lit::toLit(tmp[i]).unsign();
                const bool xorEqualsFalse = Lit::toLit(tmp[0]).sign()
                                            ^ Lit::toLit(tmp[i]).sign()
                                            ^ true;

                //Both are UNDEF, so this is a proper binary XOR
                if (control->value(lits[0]) == l_Undef
                    && control->value(lits[1]) == l_Undef
                ) {
                    runStats.foundXors++;
                    control->varReplacer->replace(
                        lits[0]
                        , lits[1]
                        , xorEqualsFalse
                        , control->conf.doExtendedSCC && control->conf.doCache
                    );
                }
            }
        }
    }
}
