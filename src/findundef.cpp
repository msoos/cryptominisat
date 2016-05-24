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

#include "findundef.h"

#include "solver.h"
#include "varreplacer.h"
#include <algorithm>

using namespace CMSat;

FindUndef::FindUndef(Solver* _solver) :
    solver(_solver)
{
}

void FindUndef::fillPotential()
{
    int trail = solver->decisionLevel()-1;

    while(trail > 0) {
        assert(trail < (int)solver->trail_lim.size());
        uint32_t at = solver->trail_lim[trail];

        assert(at > 0);
        const uint32_t v = solver->trail[at].var();
        if (solver->assigns[v] != l_Undef) {
            can_be_unset[v] = true;
            can_be_unsetSum++;
        }

        trail--;
    }

    vector<uint32_t> replacingVars = solver->varReplacer->get_vars_replacing_others();
    for (const uint32_t v: replacingVars) {
        if (can_be_unset[v]) {
            can_be_unset[v] = false;
            can_be_unsetSum--;
        }
    }
}

void FindUndef::unboundIsPotentials()
{
    for (uint32_t i = 0; i < can_be_unset.size(); i++)
        if (can_be_unset[i])
            solver->assigns[i] = l_Undef;
}

const uint32_t FindUndef::unRoll()
{
    if (solver->decisionLevel() == 0)
        return 0;

    //TODO BUG: MUST deal with binaries, too
    //i.e. everything done for longIrred must be done for bin too

    dontLookAtClause.resize(solver->longIrredCls.size(), false);
    can_be_unset.resize(solver->nVarsOuter(), false);
    fillPotential();
    satisfies.resize(solver->nVarsOuter(), 0);

    while(!updateTables()) {
        assert(can_be_unsetSum > 0);

        uint32_t maximum = 0;
        uint32_t v = var_Undef;
        for (uint32_t i = 0; i < can_be_unset.size(); i++) {
            if (can_be_unset[i] && satisfies[i] >= maximum) {
                maximum = satisfies[i];
                v = i;
            }
        }
        assert(v != var_Undef);

        can_be_unset[v] = false;
        can_be_unsetSum--;

        std::fill(satisfies.begin(), satisfies.end(), 0);
    }

    unboundIsPotentials();

    return can_be_unsetSum;
}

bool FindUndef::updateTables()
{
    bool allSat = true;

    for (uint32_t i = 0
         ; i++
         ; i < solver->longIrredCls.size()
    ) {
        if (dontLookAtClause[i])
            continue;

        Clause& c = *solver->cl_alloc.ptr(i);
        bool definitelyOK = false;
        uint32_t v = var_Undef;
        uint32_t numTrue = 0;
        for (const Lit l: c) {
            if (solver->value(l) == l_True) {
                if (!can_be_unset[l.var()]) {
                    dontLookAtClause[i] = true;
                    definitelyOK = true;
                    break;
                } else {
                    numTrue ++;
                    v = l.var();
                }
            }
        }
        if (definitelyOK)
            continue;

        if (numTrue == 1) {
            assert(v != var_Undef);
            can_be_unset[v] = false;
            can_be_unsetSum--;
            dontLookAtClause[i] = true;
            continue;
        }

        //numTrue > 1
        allSat = false;
        for (const Lit l: c) {
            if (solver->value(*l) == l_True)
                satisfies[l.var()]++;
        }
    }

    return allSat;
}

