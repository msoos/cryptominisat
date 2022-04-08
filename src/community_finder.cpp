/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***********************************************/

#include "community_finder.h"
#include "time_mem.h"
#include "solver.h"
#include "occsimplifier.h"
#include "clauseallocator.h"
#include "sqlstats.h"
#include <limits>
#include <map>
#include <utility>
#include <louvain_communities/louvain_communities.h>

using std::map;
using std::pair;
using std::make_pair;

using namespace CMSat;

CommunityFinder::CommunityFinder(Solver* _solver) :
    solver(_solver)
{
}

void CMSat::CommunityFinder::compute()
{
    //Clean it
    for(auto& v: solver->varData) {
        v.community_num = numeric_limits<uint32_t>::max();
    }

    //Check for too large
    if (solver->nVars() > 300ULL*1000ULL) {
        return;
    }

    double myTime = cpuTime();
    map<pair<uint32_t, uint32_t>, long double> edges;

    //Binary clauses
    for(uint32_t watched_at = 0; watched_at< solver->nVars()*2; watched_at++) {
        Lit l = Lit::toLit(watched_at);
        watch_subarray_const ws = solver->watches[l];
        for(uint32_t watched_at_sub = 0; watched_at_sub < ws.size(); watched_at_sub++) {
            const Watched& w = ws[watched_at_sub];
            if (w.isBin() &&
                w.lit2() < l &&
                !w.red()
            ) {
                //VIG graph
                uint32_t size = 2;
                long double weight = 1.0L/((long double)size*((long double)size-1.0L)/2.0L);

                uint32_t v1 = l.var();
                uint32_t v2 = w.lit2().var();
                //must start with smallest
                if (v2  < v1) {
                    std::swap(v1, v2);
                }
                auto edge = make_pair(v1, v2);
                auto it = edges.find(edge);
                if (it == edges.end()) {
                    edges[edge] = weight;
                } else {
                    it->second+=weight;
                }
            }
        }
    }

    //Non-binary clauses
    for(const auto& offs: solver->longIrredCls) {
        const Clause* cl = solver->cl_alloc.ptr(offs);
        //VIG graph
        uint32_t size = cl->size();
        long double weight = 1.0L/((long double)size*((long double)size-1.0L)/2.0L);
        for(uint32_t i = 0; i < cl->size(); i ++) {
            for(uint32_t i2 = i+1; i2 < cl->size(); i2 ++) {
                uint32_t v1 = (*cl)[i].var();
                uint32_t v2 = (*cl)[i2].var();

                //must start with smallest
                if (v2  < v1) {
                    std::swap(v1, v2);
                }
                auto edge = make_pair(v1, v2);
                auto it = edges.find(edge);
                if (it == edges.end()) {
                    edges[edge] = weight;
                } else {
                    it->second+=weight;
                }
            }
        }
    }

    LouvainC::Communities graph;
    for(const auto& it: edges) {
        graph.add_edge(it.first.first, it.first.second, it.second);
    }
    graph.calculate(true);
    auto mapping = graph.get_mapping();

    for(const auto& x: mapping) {
        assert(x.first < solver->nVars());
        assert(x.second < solver->nVars());
        solver->varData[x.first].community_num = x.second;
    }

    //Recompute connects_num_communities for all redundant clauses
    for(auto& cls: solver->longRedCls) {
        for(auto& offs: cls) {
            Clause* cl = solver->cl_alloc.ptr(offs);
            const uint32_t comms = solver->calc_connects_num_communities(*cl);
            solver->red_stats_extra[cl->stats.extra_pos].connects_num_communities = comms;
        }
    }


    double time_passed = cpuTime() - myTime;
    if (solver->conf.verbosity) {
        cout << "c [louvain] Louvain communities found. T: "
        << std::fixed << std::setprecision(2)
        << solver->conf.print_times(time_passed) << endl;
    }

    if (solver->sqlStats) {
        solver->sqlStats->time_passed_min(solver, "louvain", time_passed);
    }
}
