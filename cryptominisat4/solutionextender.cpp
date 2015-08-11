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
    if (solver->varReplacer)
        solver->varReplacer->extend_model();

    if (simplifier)
        simplifier->extend_model(this);
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

    const Var blockedOn_inter = solver->map_outer_to_inter(blockedOn.var());
    assert(solver->varData[blockedOn_inter].removed == Removed::elimed);

    //Oher blocked clauses set its value already
    if (solver->model_value(blockedOn) != l_Undef)
        return;

    assert(solver->model_value(blockedOn) == l_Undef);
    solver->model[blockedOn.var()] = l_True;
    solver->varReplacer->extend_model(blockedOn.var());

    #ifdef VERBOSE_DEBUG_SOLUTIONEXTENDER
    cout << "dummy now: " << solver->model_value(blockedOn) << endl;
    #endif
}

void SolutionExtender::addClause(const vector<Lit>& lits, const Lit blockedOn)
{
    const Var blocked_on_inter = solver->map_outer_to_inter(blockedOn.var());
    assert(solver->varData[blocked_on_inter].removed == Removed::elimed);
    assert(contains_lit(lits, blockedOn));
    if (satisfied(lits))
        return;

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

    assert(solver->model_value(blockedOn) == l_Undef);
    solver->model[blockedOn.var()] = blockedOn.sign() ? l_False : l_True;
    assert(satisfied(lits));

    solver->varReplacer->extend_model(blockedOn.var());
}

