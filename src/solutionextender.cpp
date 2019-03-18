/******************************************
Copyright (c) 2016, Mate Soos

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
                && solver->varData[v_inter].removed != Removed::decomposed
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

    //cout << "aft simp unset       : " << count_num_unset_model() << endl;

    //clause has been added with "lit, ~lit" so var must be set
    for(size_t i = 0; i < solver->undef_must_set_vars.size(); i++) {
        if (solver->undef_must_set_vars[i]
            && solver->model_value(i) == l_Undef
        ) {
            //any setting would work, let's set to l_False (MiniSat default)
            solver->model[i] = l_False;
            solver->decisions_reaching_model.push_back(Lit(i, true));
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

    //Picking l_False because MiniSat likes False solutions. Could pick anything.
    solver->model[blockedOn] = l_False;
    solver->decisions_reaching_model.push_back(Lit(blockedOn, true));

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

    //Note: we need to do this even if solver->conf.greedy_undef is FALSE
    //because the solution we are given (when used as a preprocessor)
    //may not be full

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
    bool all_values_false = true;
    for(Lit l: lits) {
        lbool model_value = solver-> model_value(l);
        assert(model_value != l_True);
        if (l.var() == blockedOn) {
            actual_lit = l;
        } else {
            if (model_value == l_Undef) {
                all_values_false = false;
            }
        }
    }
    assert(actual_lit != lit_Undef);
    lbool val = actual_lit.sign() ? l_False : l_True;
    solver->model[blockedOn] = val;
    if (!all_values_false) {
        solver->decisions_reaching_model.push_back(Lit(blockedOn, val == l_False));
        //cout << "Adding dec addClause: " << Lit(blockedOn, val == l_False) << endl;
    } else {
        //cout << "Would be forced anyway" << endl;
    }

    if (solver->conf.verbosity >= 10) {
        cout << "Extending VELIM cls. -- setting model for var "
        << blockedOn + 1 << " to " << solver->model[blockedOn] << endl;
    }
    solver->varReplacer->extend_model(blockedOn);

    assert(satisfied(lits));

    //it's been set now
    return true;
}

size_t SolutionExtender::count_num_unset_model() const
{
    size_t num_unset = 0;
    if (solver->conf.sampling_vars) {
        for(size_t i = 0; i < solver->conf.sampling_vars->size(); i++) {
            uint32_t var = (*solver->conf.sampling_vars)[i];
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
