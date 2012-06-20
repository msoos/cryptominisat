/***************************************************************************************[Solver.cc]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2009, Niklas Sorensson
Copyright (c) 2009-2012, Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include <iomanip>
#include "cmsat/UselessBinRemover.h"
#include "cmsat/VarReplacer.h"
#include "cmsat/ClauseCleaner.h"
#include "cmsat/time_mem.h"

//#define VERBOSE_DEBUG

using namespace CMSat;

UselessBinRemover::UselessBinRemover(Solver& _solver) :
    solver(_solver)
{
}

/**
@brief Time limiting
*/
#define MAX_REMOVE_BIN_FULL_PROPS 20000000
/**
@brief We measure time in (bogo)propagations and "extra" time, time not accountable in (bogo)props
*/
#define EXTRATIME_DIVIDER 3

/**
@brief Removes useless binary non-learnt clauses. See definiton of class for details

We pick variables starting randomly at a place and going on until we stop:
we limit ourselves in time using (bogo)propagations and "extratime"
*/
bool UselessBinRemover::removeUslessBinFull()
{
    double myTime = cpuTime();
    toDeleteSet.clear();
    toDeleteSet.growTo(solver.nVars()*2, 0);
    uint32_t origHeapSize = solver.order_heap.size();
    uint64_t origProps = solver.propagations;
    bool fixed = false;
    uint32_t extraTime = 0;
    uint32_t numBinsBefore = solver.numBins;
    solver.sortWatched(); //VERY important

    uint32_t startFrom = solver.mtrand.randInt(solver.order_heap.size());
    for (uint32_t i = 0; i != solver.order_heap.size(); i++) {
        Var var = solver.order_heap[(i+startFrom)%solver.order_heap.size()];
        if (solver.propagations - origProps + extraTime > MAX_REMOVE_BIN_FULL_PROPS) break;
        if (solver.assigns[var] != l_Undef || !solver.decision_var[var]) continue;

        Lit lit(var, true);
        if (!removeUselessBinaries(lit)) {
            fixed = true;
            solver.cancelUntilLight();
            solver.uncheckedEnqueue(~lit);
            solver.ok = (solver.propagate<false>().isNULL());
            if (!solver.ok) return false;
            continue;
        }

        lit = ~lit;
        if (!removeUselessBinaries(lit)) {
            fixed = true;
            solver.cancelUntilLight();
            solver.uncheckedEnqueue(~lit);
            solver.ok = (solver.propagate<false>().isNULL());
            if (!solver.ok) return false;
            continue;
        }
    }

    if (fixed) solver.order_heap.filter(Solver::VarFilter(solver));

    if (solver.conf.verbosity >= 1) {
        std::cout
        << "c Removed useless bin:" << std::setw(8) << (numBinsBefore - solver.numBins)
        << "  fixed: " << std::setw(5) << (origHeapSize - solver.order_heap.size())
        << "  props: " << std::fixed << std::setprecision(2) << std::setw(6) << (double)(solver.propagations - origProps)/1000000.0 << "M"
        << "  time: " << std::fixed << std::setprecision(2) << std::setw(5) << cpuTime() - myTime << " s"
        << std::endl;
    }

    return true;
}

/**
@brief Removes useless binaries of the graph portion that starts with lit

We try binary-space propagation on lit. Then, we check that we cannot reach any
of its one-hop neighboours from any of its other one-hope neighbours. If we can,
we remove the one-hop neighbour from the neightbours (i.e. remove the binary
clause). Example:

\li a->b, a->c, b->c
\li In claues: (-a V b), (-a V c), (-b V c)

One-hop neighbours of a: b, c. But c can be reached through b! So, we remove
a->c, the one-hop neighbour that is useless.
*/
bool UselessBinRemover::removeUselessBinaries(const Lit lit)
{
    solver.newDecisionLevel();
    solver.uncheckedEnqueueLight(lit);
    //Propagate only one hop
    failed = !solver.propagateBinOneLevel();
    if (failed) return false;
    bool ret = true;

    oneHopAway.clear();
    assert(solver.decisionLevel() > 0);
    int c;
    if (solver.trail.size()-solver.trail_lim[0] == 0) {
        solver.cancelUntilLight();
        goto end;
    }
    //Fill oneHopAway and toDeleteSet with lits that are 1 hop away
    extraTime += (solver.trail.size() - solver.trail_lim[0]) / EXTRATIME_DIVIDER;
    for (c = solver.trail.size()-1; c > (int)solver.trail_lim[0]; c--) {
        Lit x = solver.trail[c];
        toDeleteSet[x.toInt()] = true;
        oneHopAway.push(x);
        solver.assigns[x.var()] = l_Undef;
    }
    solver.assigns[solver.trail[c].var()] = l_Undef;

    solver.qhead = solver.trail_lim[0];
    solver.trail.shrink_(solver.trail.size() - solver.trail_lim[0]);
    solver.trail_lim.clear();
    //solver.cancelUntil(0);

    wrong.clear();
    //We now try to reach the one-hop-away nodes from other one-hop-away
    //nodes, but this time we propagate all the way
    for(uint32_t i = 0; i < oneHopAway.size(); i++) {
        //no need to visit it if it already queued for removal
        //basically, we check if it's in 'wrong'
        if (toDeleteSet[oneHopAway[i].toInt()]) {
            if (!fillBinImpliesMinusLast(lit, oneHopAway[i], wrong)) {
                ret = false;
                goto end;
            }
        }
    }

    for (uint32_t i = 0; i < wrong.size(); i++) {
        removeBin(~lit, wrong[i]);
    }

    end:
    for(uint32_t i = 0; i < oneHopAway.size(); i++) {
        toDeleteSet[oneHopAway[i].toInt()] = false;
    }

    return ret;
}

/**
@brief Removes a binary clause (lit1 V lit2)

The binary clause might be in twice, three times, etc. Take care to remove
all instances of it.
*/
void UselessBinRemover::removeBin(const Lit lit1, const Lit lit2)
{
    #ifdef VERBOSE_DEBUG
    std::cout << "Removing useless bin: " << lit1 << " " << lit2 << std::endl;
    #endif //VERBOSE_DEBUG

    std::pair<uint32_t, uint32_t> removed1 = removeWBinAll(solver.watches[(~lit1).toInt()], lit2);
    std::pair<uint32_t, uint32_t> removed2 = removeWBinAll(solver.watches[(~lit2).toInt()], lit1);
    assert(removed1 == removed2);

    assert((removed1.first + removed2.first) % 2 == 0);
    assert((removed1.second + removed2.second) % 2 == 0);
    solver.learnts_literals -= (removed1.first + removed2.first);
    solver.clauses_literals -= (removed1.second + removed2.second);
    solver.numBins -= (removed1.first + removed2.first + removed1.second + removed2.second)/2;
}

/**
@brief Propagates all the way lit, but doesn't propagate origLit

Removes adds to "wrong" the set of one-hop lits that can be reached from
lit AND are one-hop away from origLit. These later need to be removed
*/
bool UselessBinRemover::fillBinImpliesMinusLast(const Lit origLit, const Lit lit, vec<Lit>& wrong)
{
    solver.newDecisionLevel();
    solver.uncheckedEnqueueLight(lit);
    //if it's a cycle, it doesn't work, so don't propagate origLit
    failed = !solver.propagateBinExcept(origLit);
    if (failed) return false;

    assert(solver.decisionLevel() > 0);
    int c;
    extraTime += (solver.trail.size() - solver.trail_lim[0]) / EXTRATIME_DIVIDER;
    for (c = solver.trail.size()-1; c > (int)solver.trail_lim[0]; c--) {
        Lit x = solver.trail[c];
        if (toDeleteSet[x.toInt()]) {
            wrong.push(x);
            toDeleteSet[x.toInt()] = false;
        };
        solver.assigns[x.var()] = l_Undef;
    }
    solver.assigns[solver.trail[c].var()] = l_Undef;

    solver.qhead = solver.trail_lim[0];
    solver.trail.shrink_(solver.trail.size() - solver.trail_lim[0]);
    solver.trail_lim.clear();
    //solver.cancelUntil(0);

    return true;
}
