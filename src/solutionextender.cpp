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

#include "solutionextender.h"
#include "solver.h"
#include "varreplacer.h"
#include "occsimplifier.h"

//#define VERBOSE_DEBUG_SOLUTIONEXTENDER

using namespace CMSat;

SolutionExtender::SolutionExtender(Solver* _solver, OccSimplifier* _simplifier) :
    solver(_solver)
    , simplifier(_simplifier)
{
}

void SolutionExtender::extend()
{
    var_has_been_blocked.resize(solver->nVarsOuter(), false);
    //cout << "c [extend] start num unset: " << solver->count_num_unset_model() << endl;

    //Extend variables already set
    solver->varReplacer->extend_model_already_set();
    //cout << "aft varreplacer unset: " << count_num_unset_model() << endl;

    if (simplifier)
        simplifier->extend_model(this);

    //cout << "aft simp unset       : " << count_num_unset_model() << endl;

    //clause has been added with "lit, ~lit" so var must be set
    for(size_t i = 0; i < solver->undef_must_set_vars.size(); i++) {
        if (solver->undef_must_set_vars[i]
            && solver->model_value(i) == l_Undef
        ) {
            solver->model[i] = l_False;
        }
    }

    //All variables, not just those set
    solver->varReplacer->extend_model_set_undef();
}

bool SolutionExtender::satisfied(const vector< Lit >& lits) const
{
    for(const Lit lit: lits) {
        if (solver->model_value(lit) == l_True)
            return true;
    }

    return false;
}

bool SolutionExtender::contains_lit(
    const vector<Lit>& lits
    , const Lit tocontain
) const {
    for(const Lit lit: lits) {
        if (lit == tocontain)
            return true;
    }

    return false;
}

void SolutionExtender::dummyBlocked(const Lit blockedOn)
{
    #ifdef VERBOSE_DEBUG_SOLUTIONEXTENDER
    cout
    << "dummy blocked lit "
    << solver->map_inter_to_outer(blockedOn)
    << endl;
    #endif

    const uint32_t blockedOn_inter = solver->map_outer_to_inter(blockedOn.var());
    assert(solver->varData[blockedOn_inter].removed == Removed::elimed);

    //Blocked clauses set its value already
    if (solver->model_value(blockedOn) != l_Undef)
        return;


    //If var is replacing something else, it MUST be set.
    if (solver->varReplacer->var_is_replacing(blockedOn.var())) {
        //Picking l_False because MiniSat likes False solutions. Could pick anything.
        solver->model[blockedOn.var()] = l_False;
        solver->varReplacer->extend_model(blockedOn.var());
    }

    //If greedy undef is not set, set model to value
    if (!solver->conf.greedy_undef) {
        solver->model[blockedOn.var()] = l_False;
    } else {
        var_has_been_blocked[blockedOn.var()] = true;
    }
}

void SolutionExtender::addClause(const vector<Lit>& lits, const Lit blockedOn)
{
    const uint32_t blocked_on_inter = solver->map_outer_to_inter(blockedOn.var());
    assert(solver->varData[blocked_on_inter].removed == Removed::elimed);
    assert(contains_lit(lits, blockedOn));
    if (satisfied(lits)) {
        return;
    } else {
        //Note: we need to do this even if solver->conf.greedy_undef is FALSE
        //because the solution we are given (when used as a preprocessor)
        //may not be full

        //Try to extend through setting variables that have been blocked but
        //were not required to be set until now
        for(Lit l: lits) {
            if (solver->model_value(l) == l_Undef
                && var_has_been_blocked[l.var()]
            ) {
                solver->model[l.var()] = l.sign() ? l_False : l_True;
                solver->varReplacer->extend_model(l.var());
                return;
            }
        }

        //Try to set var that hasn't been set
        for(Lit l: lits) {
            uint32_t v_inter = solver->map_outer_to_inter(l.var());
            if (solver->model_value(l) == l_Undef
                && solver->varData[v_inter].removed == Removed::none
            ) {
                solver->model[l.var()] = l.sign() ? l_False : l_True;
                solver->varReplacer->extend_model(l.var());
                return;
            }
        }
    }

    #ifdef VERBOSE_DEBUG_SOLUTIONEXTENDER
    for(Lit lit: lits) {
        Lit lit_inter = solver->map_outer_to_inter(lit);
        cout
        << lit << ": " << solver->model_value(lit)
        << "(elim: " << removed_type_to_string(solver->varData[lit_inter.var()].removed) << ")"
        << ", ";
    }
    cout << "blocked on: " <<  blockedOn << endl;
    #endif

    if (solver->model_value(blockedOn) != l_Undef) {
        cout << "ERROR: Model value for var " << blockedOn.unsign() << " is "
        << solver->model_value(blockedOn)
        << " but that doesn't satisfy a v-elim clause on the stack!"
        << " clause is: " << lits
        << endl;

        for(Lit l: lits) {
            uint32_t v_inter = solver->map_outer_to_inter(l.var());
            cout << "Value of " << l << " : " << solver-> model_value(l)
            << " removed: " << removed_type_to_string(solver->varData[v_inter].removed)
            << endl;
        }
    }
    assert(solver->model_value(blockedOn) == l_Undef);
    solver->model[blockedOn.var()] = blockedOn.sign() ? l_False : l_True;
    if (solver->conf.verbosity >= 10) {
        cout << "Extending VELIM cls. -- setting model for var "
        << blockedOn.unsign() << " to " << solver->model[blockedOn.var()] << endl;
    }
    solver->varReplacer->extend_model(blockedOn.var());

    assert(satisfied(lits));
}

size_t SolutionExtender::count_num_unset_model() const
{
    size_t num_unset = 0;
    if (solver->conf.independent_vars) {
        for(size_t i = 0; i < solver->conf.independent_vars->size(); i++) {
            uint32_t var = (*solver->conf.independent_vars)[i];
            if (solver->model_value(var) == l_Undef) {
                num_unset++;
            }
        }
    } else {
        for(size_t i = 0; i < solver->nVars(); i++) {
            if (solver->model_value(i) == l_Undef) {
                num_unset++;
            }
        }
    }
    return num_unset;
}
