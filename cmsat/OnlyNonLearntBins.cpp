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

#include "cmsat/OnlyNonLearntBins.h"

#include <iomanip>
#include "cmsat/Solver.h"
#include "cmsat/Clause.h"
#include "cmsat/VarReplacer.h"
#include "cmsat/ClauseCleaner.h"
#include "cmsat/time_mem.h"

using namespace CMSat;

OnlyNonLearntBins::OnlyNonLearntBins(Solver& _solver) :
    solver(_solver)
{}

/**
@brief Propagate recursively on non-learnt binaries
*/
bool OnlyNonLearntBins::propagate()
{
    while (solver.qhead < solver.trail.size()) {
        Lit p = solver.trail[solver.qhead++];
        vec<WatchedBin> & wbin = binwatches[p.toInt()];
        solver.propagations += wbin.size()/2 + 2;
        for(WatchedBin *k = wbin.getData(), *end = wbin.getDataEnd(); k != end; k++) {
            lbool val = solver.value(k->impliedLit);
            if (val.isUndef()) {
                solver.uncheckedEnqueueLight(k->impliedLit);
            } else if (val == l_False) {
                return false;
            }
        }
    }

    return true;
}

/**
@brief Fill internal watchlists with non-binary clauses
*/
bool OnlyNonLearntBins::fill()
{
    uint32_t numBins = 0;
    double myTime = cpuTime();
    binwatches.growTo(solver.nVars()*2);

    uint32_t wsLit = 0;
    for (const vec<Watched> *it = solver.watches.getData(), *end = solver.watches.getDataEnd(); it != end; it++, wsLit++) {
        const vec<Watched>& ws = *it;
        for (vec<Watched>::const_iterator it2 = ws.getData(), end2 = ws.getDataEnd(); it2 != end2; it2++) {
            if (it2->isBinary() && !it2->getLearnt()) {
                binwatches[wsLit].push(WatchedBin(it2->getOtherLit()));
                numBins++;
            }
        }
    }

    if (solver.conf.verbosity >= 3) {
        std::cout << "c Time to fill non-learnt binary watchlists:"
        << std::fixed << std::setprecision(2) << std::setw(5)
        << cpuTime() - myTime << " s"
        << " num non-learnt bins: " << std::setw(10) << numBins
        << std::endl;
    }

    return true;
}
