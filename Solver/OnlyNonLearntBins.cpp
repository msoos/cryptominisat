/***********************************************************************
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
************************************************************************/

#include "OnlyNonLearntBins.h"

#include <iomanip>
#include "Solver.h"
#include "Clause.h"
#include "VarReplacer.h"
#include "ClauseCleaner.h"
#include "time_mem.h"

OnlyNonLearntBins::OnlyNonLearntBins(Solver& _solver) :
    solver(_solver)
{}

/**
@brief Propagate recursively on non-learnt binaries
*/
const bool OnlyNonLearntBins::propagate()
{
    while (solver.qhead < solver.trail.size()) {
        Lit p   = solver.trail[solver.qhead++];
        vec<WatchedBin> & wbin = binwatches[p.toInt()];
        solver.propagations += wbin.size()/2;
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
@brief Propagate recursively on non-learnt binaries, but do not propagate exceptLit if we reach it
*/
const bool OnlyNonLearntBins::propagateBinExcept(const Lit& exceptLit)
{
    while (solver.qhead < solver.trail.size()) {
        Lit p   = solver.trail[solver.qhead++];
        vec<WatchedBin> & wbin = binwatches[p.toInt()];
        solver.propagations += wbin.size()/2;
        for(WatchedBin *k = wbin.getData(), *end = wbin.getDataEnd(); k != end; k++) {
            lbool val = solver.value(k->impliedLit);
            if (val.isUndef() && k->impliedLit != exceptLit) {
                solver.uncheckedEnqueueLight(k->impliedLit);
            } else if (val == l_False) {
                return false;
            }
        }
    }

    return true;
}

/**
@brief Propagate only for one hop(=non-recursively) on non-learnt bins
*/
const bool OnlyNonLearntBins::propagateBinOneLevel()
{
    Lit p   = solver.trail[solver.qhead];
    vec<WatchedBin> & wbin = binwatches[p.toInt()];
    solver.propagations += wbin.size()/2;
    for(WatchedBin *k = wbin.getData(), *end = wbin.getDataEnd(); k != end; k++) {
        lbool val = solver.value(k->impliedLit);
        if (val.isUndef()) {
            solver.uncheckedEnqueueLight(k->impliedLit);
        } else if (val == l_False) {
            return false;
        }
    }

    return true;
}

/**
@brief Fill internal watchlists with non-binary clauses
*/
const bool OnlyNonLearntBins::fill()
{
    double myTime = cpuTime();
    assert(solver.doReplace);
    while (solver.doReplace && solver.varReplacer->getClauses().size() > 0) {
        if (!solver.varReplacer->performReplace(true)) return false;
        solver.clauseCleaner->removeAndCleanAll(true);
    }
    assert(solver.varReplacer->getClauses().size() == 0);
    solver.clauseCleaner->moveBinClausesToBinClauses();
    binwatches.growTo(solver.nVars()*2);

    for(Clause **i = solver.binaryClauses.getData(), **end = solver.binaryClauses.getDataEnd(); i != end; i++) {
        Clause& c = **i;
        if (c.learnt()) continue;

        attachBin(c);
    }

    if (solver.verbosity >= 2) {
        std::cout << "c Time to fill non-learnt binary watchlists:"
        << std::fixed << std::setprecision(2) << std::setw(5)
        << cpuTime() - myTime << " s" << std::endl;
    }

    return true;
}

void OnlyNonLearntBins::attachBin(const Clause& c)
{
    binwatches[(~c[0]).toInt()].push(WatchedBin(c[1]));
    binwatches[(~c[1]).toInt()].push(WatchedBin(c[0]));
}


void OnlyNonLearntBins::removeBin(Lit lit1, Lit lit2)
{
    uint32_t index0 = lit1.toInt();
    uint32_t index1 = lit2.toInt();
    if (index1 > index0) std::swap(index0, index1);
    uint64_t final = index0;
    final |= ((uint64_t)index1) << 32;
    toRemove.insert(final);

    OnlyNonLearntBins::WatchedBin::removeWBinAll(binwatches[(~lit1).toInt()], lit2);
    OnlyNonLearntBins::WatchedBin::removeWBinAll(binwatches[(~lit2).toInt()], lit1);
}

/**
@brief Remove all binary clauses marked to be removed
*/
const uint32_t OnlyNonLearntBins::removeBins()
{
    Clause **i, **j;
    i = j = solver.binaryClauses.getData();
    uint32_t num = 0;
    for (Clause **end = solver.binaryClauses.getDataEnd(); i != end; i++, num++) {
        Clause& c = **i;
        uint32_t index0 = c[0].toInt();
        uint32_t index1 = c[1].toInt();
        if (index1 > index0) std::swap(index0, index1);
        uint64_t final = index0;
        final |= ((uint64_t)index1) << 32;

        if (toRemove.find(final) == toRemove.end()) {
            *j++ = *i;
        } else {
            solver.clauseAllocator.clauseFree(*i);
        }
    }
    solver.binaryClauses.shrink(i-j);
    solver.clauses_literals -= (i-j)*2;
    return (i - j);
}


