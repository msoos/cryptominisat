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

#include "cmsat/FindUndef.h"

#include "cmsat/Solver.h"
#include "cmsat/VarReplacer.h"
#include <algorithm>

FindUndef::FindUndef(Solver& _solver) :
    solver(_solver)
    , isPotentialSum(0)
{
}

void FindUndef::fillPotential()
{
    int trail = solver.decisionLevel()-1;

    while(trail > 0) {
        assert(trail < (int)solver.trail_lim.size());
        uint32_t at = solver.trail_lim[trail];

        assert(at > 0);
        Var v = solver.trail[at].var();
        if (solver.assigns[v] != l_Undef) {
            isPotential[v] = true;
            isPotentialSum++;
        }

        trail--;
    }

    for (XorClause** it = solver.xorclauses.getData(), **end = it + solver.xorclauses.size(); it != end; it++) {
        XorClause& c = **it;
        for (Lit *l = c.getData(), *end = l + c.size(); l != end; l++) {
            if (isPotential[l->var()]) {
                isPotential[l->var()] = false;
                isPotentialSum--;
            }
            assert(!solver.value(*l).isUndef());
        }
    }

    vector<Var> replacingVars = solver.varReplacer->getReplacingVars();
    for (Var *it = &replacingVars[0], *end = it + replacingVars.size(); it != end; it++) {
        if (isPotential[*it]) {
            isPotential[*it] = false;
            isPotentialSum--;
        }
    }
}

void FindUndef::unboundIsPotentials()
{
    for (uint32_t i = 0; i < isPotential.size(); i++)
        if (isPotential[i])
            solver.assigns[i] = l_Undef;
}

void FindUndef::moveBinToNormal()
{
    binPosition = solver.clauses.size();
    for (uint32_t i = 0; i != solver.binaryClauses.size(); i++)
        solver.clauses.push(solver.binaryClauses[i]);
    solver.binaryClauses.clear();
}

void FindUndef::moveBinFromNormal()
{
    for (uint32_t i = binPosition; i != solver.clauses.size(); i++)
        solver.binaryClauses.push(solver.clauses[i]);
    solver.clauses.shrink(solver.clauses.size() - binPosition);
}

const uint32_t FindUndef::unRoll()
{
    if (solver.decisionLevel() == 0) return 0;

    moveBinToNormal();

    dontLookAtClause.resize(solver.clauses.size(), false);
    isPotential.resize(solver.nVars(), false);
    fillPotential();
    satisfies.resize(solver.nVars(), 0);

    while(!updateTables()) {
        assert(isPotentialSum > 0);

        uint32_t maximum = 0;
        Var v = var_Undef;
        for (uint32_t i = 0; i < isPotential.size(); i++) {
            if (isPotential[i] && satisfies[i] >= maximum) {
                maximum = satisfies[i];
                v = i;
            }
        }
        assert(v != var_Undef);

        isPotential[v] = false;
        isPotentialSum--;

        std::fill(satisfies.begin(), satisfies.end(), 0);
    }

    unboundIsPotentials();
    moveBinFromNormal();

    return isPotentialSum;
}

bool FindUndef::updateTables()
{
    bool allSat = true;

    uint32_t i = 0;
    for (Clause** it = solver.clauses.getData(), **end = it + solver.clauses.size(); it != end; it++, i++) {
        if (dontLookAtClause[i])
            continue;

        Clause& c = **it;
        bool definitelyOK = false;
        Var v = var_Undef;
        uint32_t numTrue = 0;
        for (Lit *l = c.getData(), *end = l + c.size(); l != end; l++) {
            if (solver.value(*l) == l_True) {
                if (!isPotential[l->var()]) {
                    dontLookAtClause[i] = true;
                    definitelyOK = true;
                    break;
                } else {
                    numTrue ++;
                    v = l->var();
                }
            }
        }
        if (definitelyOK)
            continue;

        if (numTrue == 1) {
            assert(v != var_Undef);
            isPotential[v] = false;
            isPotentialSum--;
            dontLookAtClause[i] = true;
            continue;
        }

        //numTrue > 1
        allSat = false;
        for (Lit *l = c.getData(), *end = l + c.size(); l != end; l++) {
            if (solver.value(*l) == l_True)
                satisfies[l->var()]++;
        }
    }

    return allSat;
}

