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
#include "constants.h"
#include "solver.h"
#include "solvertypesmini.h"
#include "varreplacer.h"
#include "occsimplifier.h"
#include "gaussian.h"

//#define VERBOSE_DEBUG_SOLUTIONEXTENDER

using namespace CMSat;

SolutionExtender::SolutionExtender(Solver* _solver, OccSimplifier* _simplifier) :
    solver(_solver)
    , simplifier(_simplifier)
{}

//Model at this point has OUTER variables
void SolutionExtender::extend() {
    verb_print(10, "Exteding solution -- SolutionExtender::extend()");

    #ifdef SLOW_DEBUG
    for(uint32_t i = 0; i < solver->varData.size(); i++) {
        uint32_t v_inter = solver->map_outer_to_inter(i);
        if (
            //decomposed's solution has beed added already, it SHOULD be set
            //but everything else is NOT OK
            solver->varData[v_inter].removed != Removed::none
        ) {
            if (solver->model[i] != l_Undef)
                cout << "ERROR: variable " << i + 1 << " set even though it's removed: "
                << removed_type_to_string(solver->varData[v_inter].removed) << endl;
            assert(solver->model[i] == l_Undef);
        }
    }
    #endif

    for(const auto& x: solver->xorclauses) for(auto v: x) {
        v = solver->map_inter_to_outer(v);
        assert(solver->model_value(v) != l_Undef);
    }
    for(const auto& gj: solver->gmatrices) for(const auto& x: gj->xorclauses) for(auto v: x) {
        v = solver->map_inter_to_outer(v);
        assert(solver->model_value(v) != l_Undef);
    }

    //Extend variables already set
    solver->varReplacer->extend_model_already_set();
    if (simplifier) simplifier->extend_model(this);

    //clause has been added with "lit, ~lit" so var must be set
    for(size_t i = 0; i < solver->undef_must_set_vars.size(); i++) {
        if (solver->undef_must_set_vars[i] && solver->model_value(i) == l_Undef) {
            solver->model[i] = l_False;
        }
    }
    solver->varReplacer->extend_model_all();
}

inline bool SolutionExtender::satisfied(const vector< Lit >& lits) const {
    for(const Lit lit: lits) if (solver->model_value(lit) == l_True) return true;
    return false;
}

inline bool SolutionExtender::xor_satisfied(const vector< Lit >& lits) const {
    bool rhs = false;
    for(const Lit lit: lits) rhs ^= solver->model_value(lit) == l_True;
    return rhs == true;
}

//called with _outer_ variable in "elimed_on"
void SolutionExtender::dummy_elimed(const uint32_t elimed_on)
{
    #ifdef VERBOSE_DEBUG_SOLUTIONEXTENDER
    cout << "dummy elimed lit (outer) " << elimed_on + 1 << endl;
    #endif

    #ifdef SLOW_DEBUG
    const uint32_t elimedOn_inter = solver->map_outer_to_inter(elimed_on);
    assert(solver->varData[elimedOn_inter].removed == Removed::elimed);
    #endif

    //Elimed clauses set its value already
    if (solver->model_value(elimed_on) != l_Undef) return;

    solver->model[elimed_on] = l_False;

    //If var is replacing something else, it MUST be set.
    if (solver->varReplacer->var_is_replacing(elimed_on)) {
        solver->varReplacer->extend_model(elimed_on);
    }
}

void SolutionExtender::set_pre_checks(const vector<Lit>& lits, const uint32_t elimed_on) {
    #ifdef VERBOSE_DEBUG_SOLUTIONEXTENDER
    cout << "outer clause: " << lits << endl;
    #endif

    #ifdef SLOW_DEBUG
    const uint32_t elimed_on_inter = solver->map_outer_to_inter(elimed_on);
    assert(solver->varData[elimed_on_inter].removed == Removed::elimed);
    assert(contains_var(lits, elimed_on));
    #endif

    if (solver->conf.verbosity >= 10) {
        for(Lit lit: lits) {
            Lit lit_inter = solver->map_outer_to_inter(lit);
            cout << lit << ": " << solver->model_value(lit)
            << "(elim: " << removed_type_to_string(solver->varData[lit_inter.var()].removed) << ")"
            << ", ";
        }
        cout << "elimed on: " <<  elimed_on+1 << endl;
    }

    if (solver->model_value(elimed_on) != l_Undef) {
        cout << "ERROR: Model value for var " << elimed_on+1 << " is "
        << solver->model_value(elimed_on)
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
    assert(solver->model_value(elimed_on) == l_Undef);
}

bool SolutionExtender::add_xor_cl(const vector<Lit>& lits, const uint32_t elimed_on)
{
    Lit actual_lit = lit_Undef;
    bool rhs = false;
    for(Lit l: lits) {
        lbool model_value = solver-> model_value(l);
        if (l.var() == elimed_on) actual_lit = l;
        else {
            assert(model_value != l_Undef);
            rhs ^= solver->model_value(l) == l_True;
        }
    }
    assert(actual_lit != lit_Undef);
    lbool val = boolToLBool(actual_lit.sign() ^ !rhs);
    solver->model[elimed_on] = val;

    verb_print(10,"Extending VELIM cls (xor). -- setting model for var "
        << elimed_on + 1 << " to " << solver->model[elimed_on]);
    solver->varReplacer->extend_model(elimed_on);

    assert(xor_satisfied(lits));
    return true;
}

bool SolutionExtender::add_cl(const vector<Lit>& lits, const uint32_t elimed_on)
{
    Lit actual_lit = lit_Undef;
    for(Lit l: lits) {
        lbool model_value = solver-> model_value(l);
        assert(model_value != l_True);
        if (l.var() == elimed_on) actual_lit = l;
        else if (model_value != l_Undef) assert(model_value == l_False);
    }
    assert(actual_lit != lit_Undef);
    lbool val = actual_lit.sign() ? l_False : l_True;
    solver->model[elimed_on] = val;

    verb_print(10,"Extending VELIM cls (norm cl). -- setting model for var "
        << elimed_on + 1 << " to " << solver->model[elimed_on]);
    solver->varReplacer->extend_model(elimed_on);

    assert(satisfied(lits));
    return true;
}
