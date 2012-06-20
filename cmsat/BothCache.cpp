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

#include "cmsat/BothCache.h"
#include "cmsat/time_mem.h"
#include <iomanip>
#include "cmsat/VarReplacer.h"
#include "cmsat/Subsumer.h"
#include "cmsat/XorSubsumer.h"

namespace CMSat {

BothCache::BothCache(Solver& _solver) :
    solver(_solver)
{}

bool BothCache::tryBoth()
{
    vec<bool> seen(solver.nVars(), 0);
    vec<bool> val(solver.nVars(), 0);
    vec<Lit> tmp;
    uint32_t bProp = 0;
    uint32_t bXProp = 0;
    double myTime = cpuTime();
    uint32_t backupTrailSize = solver.trail.size();

    for (Var var = 0; var < solver.nVars(); var++) {
        if (solver.value(var) != l_Undef
            || solver.subsumer->getVarElimed()[var]
            || solver.xorSubsumer->getVarElimed()[var]
            || solver.varReplacer->getReplaceTable()[var].var() != var)
            continue;

        Lit lit = Lit(var, false);
        vector<Lit> const* cache1;
        vector<Lit> const* cache2;

        bool startWithTrue;
        if (solver.transOTFCache[lit.toInt()].lits.size() < solver.transOTFCache[(~lit).toInt()].lits.size()) {
            cache1 = &solver.transOTFCache[lit.toInt()].lits;
            cache2 = &solver.transOTFCache[(~lit).toInt()].lits;
            startWithTrue = false;
        } else {
            cache1 = &solver.transOTFCache[(~lit).toInt()].lits;
            cache2 = &solver.transOTFCache[lit.toInt()].lits;
            startWithTrue = true;
        }

        if (cache1->size() == 0) continue;

        for (vector<Lit>::const_iterator it = cache1->begin(), end = cache1->end(); it != end; it++) {
            seen[it->var()] = true;
            val[it->var()] = it->sign();
        }

        for (vector<Lit>::const_iterator it = cache2->begin(), end = cache2->end(); it != end; it++) {
            if (seen[it->var()]) {
                Var var2 = it->var();
                if (solver.subsumer->getVarElimed()[var2]
                    || solver.xorSubsumer->getVarElimed()[var2]
                    || solver.varReplacer->getReplaceTable()[var2].var() != var2)
                    continue;

                if  (val[it->var()] == it->sign()) {
                    tmp.clear();
                    tmp.push(*it);
                    solver.addClauseInt(tmp, true);
                    if  (!solver.ok) goto end;
                    bProp++;
                } else {
                    tmp.clear();
                    tmp.push(Lit(var, false));
                    tmp.push(Lit(it->var(), false));
                    bool sign = true ^ startWithTrue ^ it->sign();
                    solver.addXorClauseInt(tmp, sign);
                    if  (!solver.ok) goto end;
                    bXProp++;
                }
            }
        }

        for (vector<Lit>::const_iterator it = cache1->begin(), end = cache1->end(); it != end; it++) {
            seen[it->var()] = false;
        }
    }

    end:
    if (solver.conf.verbosity >= 1) {
        std::cout << "c Cache " <<
        " BProp: " << bProp <<
        " Set: " << (solver.trail.size() - backupTrailSize) <<
        " BXProp: " << bXProp <<
        " T: " << (cpuTime() - myTime) <<
        std::endl;
    }

    return solver.ok;
}

}
