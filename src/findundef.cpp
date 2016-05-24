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

void FindUndef::fill_potentials()
{
    int trail = solver->decisionLevel()-1;

    //Mark everything on the trail except at lev 0
    while(trail > 0) {
        assert(trail < (int)solver->trail_lim.size());
        uint32_t at = solver->trail_lim[trail];
        assert(at > 0);

        const uint32_t v = solver->trail[at].var();
        assert(solver->varData[v].removed == Removed::none);
        if (solver->value(v) != l_Undef
        ) {
            can_be_unset[v] = true;
            can_be_unsetSum++;
        }

        trail--;
    }

    //Mark variables replacing others as non-eligible
    vector<uint32_t> replacingVars = solver->varReplacer->get_vars_replacing_others();
    for (const uint32_t v: replacingVars) {
        if (can_be_unset[v]) {
            can_be_unset[v] = false;
            can_be_unsetSum--;
        }
    }
}

void FindUndef::unset_potentials()
{
    for (uint32_t i = 0; i < can_be_unset.size(); i++) {
        if (can_be_unset[i])
            solver->value(i) = l_Undef;
    }
}

const uint32_t FindUndef::unRoll()
{
    if (solver->decisionLevel() == 0)
        return 0;

    dontLookAtClause.resize(solver->longIrredCls.size(), false);
    can_be_unset.resize(solver->nVarsOuter(), false);
    fill_potentials();
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

        //Fix 'v' to be set to curent value
        can_be_unset[v] = false;
        can_be_unsetSum--;

        std::fill(satisfies.begin(), satisfies.end(), 0);
    }

    unset_potentials();

    return can_be_unsetSum;
}

template<class C>
bool FindUndef::look_at_one_clause(const C& c)
{
    bool satisfied = false;
    uint32_t v = var_Undef;
    uint32_t numTrue = 0;
    for (const Lit l: c) {
        if (solver->value(l) == l_True) {
            if (!can_be_unset[l.var()]) {
                //clause definitely satisfied
                return true;
            } else {
                numTrue ++;
                v = l.var();
            }
        }
    }

    //Greedy
    if (numTrue == 1) {
        assert(v != var_Undef);
        can_be_unset[v] = false;
        can_be_unsetSum--;
        //clause definitely satisfied
        return true;
    }

    //numTrue > 1
    all_sat = false;
    assert(numTrue > 1);
    for (const Lit l: c) {
        if (solver->value(l) == l_True)
            satisfies[l.var()]++;
    }

    //Clause is not definitely satisfied
    return false;
}

bool FindUndef::updateTables()
{
    all_sat = true;
    for (uint32_t i = 0
         ; i++
         ; i < solver->longIrredCls.size()
    ) {
        if (dontLookAtClause[i])
            continue;

        Clause& c = *solver->cl_alloc.ptr(i);
        if (look_at_one_clause(c)) {
            //clause definitely satisfied
            dontLookAtClause[i] = true;
        }
    }

    for(size_t i = 0;i < solver->watches.size(); i++) {
        const Lit l = Lit::toLit(i);
        if (!can_be_unset[l.var()] && solver->value(l) == l_True) {
            continue;
        }
        for(const Watched& w: solver->watches[l]) {
            if (w.isBin()) {
                uint32_t v = w.lit2().var();
                if (can_be_unset[v]) {
                    can_be_unset[v] = true;
                    can_be_unsetSum--;
                }
            }

            if (w.isTri()) {
                std::array<Lit, 2> c;
                c[0] = w.lit2().var();
                c[1] = w.lit3().var();
                look_at_one_clause(c);
            }
        }
    }

    return all_sat;
}

