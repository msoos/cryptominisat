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

//Model at this point has OUTER variables
void SolutionExtender::extend()
{
    if (solver->conf.verbosity >= 10) {
        cout << "c Exteding solution -- SolutionExtender::extend()" << endl;
    }

    #ifdef SLOW_DEBUG
    for(uint32_t i = 0; i < solver->varData.size(); i++) {
        uint32_t v_inter = solver->map_outer_to_inter(i);
        if (
            //decomposed's solution has beed added already, it SHOULD be set
            //but everything else is NOT OK
            (solver->varData[v_inter].removed != Removed::none
                && solver->varData[v_inter].removed != Removed::clashed
            )
            && solver->model[i] != l_Undef
        ) {
            cout << "ERROR: variable " << i + 1
            << " set even though it's removed: "
            << removed_type_to_string(solver->varData[v_inter].removed) << endl;
            //solver->model[i] = l_Undef;
            assert(solver->model[i] == l_Undef);
        }
    }
    #endif

    //Extend variables already set
    solver->varReplacer->extend_model_already_set();

    if (simplifier) {
        simplifier->extend_model(this);
    }

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

inline bool SolutionExtender::satisfied(const vector< Lit >& lits) const
{
    for(const Lit lit: lits) {
        if (solver->model_value(lit) == l_True)
            return true;
    }

    return false;
}

//called with _outer_ variable in "blockedOn"
void SolutionExtender::dummyBlocked(const uint32_t blockedOn)
{
    #ifdef VERBOSE_DEBUG_SOLUTIONEXTENDER
    cout
    << "dummy blocked lit (outer) "
    << blockedOn + 1
    << endl;
    #endif

    #ifdef SLOW_DEBUG
    const uint32_t blockedOn_inter = solver->map_outer_to_inter(blockedOn);
    assert(solver->varData[blockedOn_inter].removed == Removed::elimed);
    #endif

    //Blocked clauses set its value already
    if (solver->model_value(blockedOn) != l_Undef)
        return;

    solver->model[blockedOn] = l_False;

    //If var is replacing something else, it MUST be set.
    if (solver->varReplacer->var_is_replacing(blockedOn)) {
        solver->varReplacer->extend_model(blockedOn);
    }
}

bool SolutionExtender::addClause(const vector<Lit>& lits, const uint32_t blockedOn)
{
    #ifdef VERBOSE_DEBUG_SOLUTIONEXTENDER
    cout
    << "outer clause: "
    << lits
    << endl;
    #endif

    #ifdef SLOW_DEBUG
    const uint32_t blocked_on_inter = solver->map_outer_to_inter(blockedOn);
    assert(solver->varData[blocked_on_inter].removed == Removed::elimed);
    assert(contains_var(lits, blockedOn));
    #endif

    //Try to extend through setting variables that have been blocked but
    //were not required to be set until now
    /*for(Lit l: lits) {
        if (solver->model_value(l) == l_Undef
            && var_has_been_blocked[l.var()]
        ) {
            solver->model[l.var()] = l.sign() ? l_False : l_True;
            solver->varReplacer->extend_model(l.var());
            return false;
        }
    }*/

    //Try to set var that hasn't been set
//     for(Lit l: lits) {
//         uint32_t v_inter = solver->map_outer_to_inter(l.var());
//         if (solver->model_value(l) == l_Undef
//             && solver->varData[v_inter].removed == Removed::none
//         ) {
//             solver->model[l.var()] = l.sign() ? l_False : l_True;
//             solver->varReplacer->extend_model(l.var());
//             return false;
//         }
//     }

    if (solver->conf.verbosity >= 10) {
        for(Lit lit: lits) {
            Lit lit_inter = solver->map_outer_to_inter(lit);
            cout
            << lit << ": " << solver->model_value(lit)
            << "(elim: " << removed_type_to_string(solver->varData[lit_inter.var()].removed) << ")"
            << ", ";
        }
        cout << "blocked on: " <<  blockedOn+1 << endl;
    }

    if (solver->model_value(blockedOn) != l_Undef) {
        cout << "ERROR: Model value for var " << blockedOn+1 << " is "
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

    //satisfy this one clause
    Lit actual_lit = lit_Undef;
    for(Lit l: lits) {
        lbool model_value = solver-> model_value(l);
        assert(model_value != l_True);
        if (l.var() == blockedOn) {
            actual_lit = l;
        } else {
            if (model_value == l_Undef) {
            } else {
                assert(model_value == l_False);
            }
        }
    }
    assert(actual_lit != lit_Undef);
    lbool val = actual_lit.sign() ? l_False : l_True;
    solver->model[blockedOn] = val;

    if (solver->conf.verbosity >= 10) {
        cout << "Extending VELIM cls. -- setting model for var "
        << blockedOn + 1 << " to " << solver->model[blockedOn] << endl;
    }
    solver->varReplacer->extend_model(blockedOn);

    assert(satisfied(lits));

    //it's been set now
    return true;
}
