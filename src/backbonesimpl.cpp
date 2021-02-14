/******************************************
Copyright (C) 2021 Authors of CryptoMiniSat, see AUTHORS file

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

#include "backbonesimpl.h"
#include "solver.h"

using namespace CMSat;

BackboneSimpl::BackboneSimpl(Solver* _solver) :
    solver(_solver)
{}

struct IncSorterAsc
{
    IncSorterAsc(const vector<uint32_t>& _inc) :
        inc(_inc)
    {}

    bool operator()(const uint32_t a, const uint32_t b) const {
        //Return that the order is OK when "a" has less incidence than "b"
        return inc[a] < inc[b];
    }

    const vector<uint32_t>& inc;
};

// See Preprocessing for Propositional Model Counting by Jean-Marie Lagniez and Pierre Marquis
lbool BackboneSimpl::backbone_simpl(uint64_t max_confl)
{
    if (solver->conf.verbosity) {
        cout << "c [backbone-simpl] starting backbone simplification..." << endl;
    }

    double myTime = cpuTime();
    uint32_t orig_vars_set = solver->get_zero_assigned_lits().size();
    bool finished = false;
    Lit l;
    uint32_t backup_verb = solver->conf.verbosity;
    solver->conf.verbosity = 0;

    vector<Lit> tmp_clause;
    vector<Lit> assumps;
    vector<lbool> model;
    vector<char> model_enabled;

    vector<uint32_t> var_order(solver->nVarsOutside());
    for(uint32_t i = 0; i < solver->nVarsOutside(); i++) {
        var_order[i] = i;
    }
    const auto inc = solver->get_outside_lit_incidence();
    std::sort(var_order.begin(), var_order.end(), IncSorterAsc(inc));

    solver->set_max_confl(max_confl);
    lbool ret = solver->solve_with_assumptions();
    if (ret == l_False) {
        solver->conf.verbosity = backup_verb;
        return l_False;
    }
    if (ret == l_Undef) {
        goto end;
    }

    model = solver->get_model();
    model_enabled.resize(solver->nVarsOutside(), 1);

    for(const uint32_t var: var_order) {
        if (!model_enabled[var]) {
            continue;
        }

        l = Lit(var, model[var] == l_False);

        //There is definitely a solution with "l". Let's see if ~l fails.
        assumps.clear();
        assumps.push_back(~l);
        solver->set_max_confl(max_confl);
        ret = solver->solve_with_assumptions(&assumps);
        if (ret == l_True) {
            for(uint32_t i2 = 0; i2 < solver->nVarsOutside(); i2++) {
                if (solver->get_model()[i2] != model[i2]) {
                    model_enabled[i2] = 0;
                }
            }
        } else if (ret == l_False) {
            tmp_clause.clear();
            tmp_clause.push_back(l);
            if (!solver->add_clause_outside(tmp_clause)) {
                solver->conf.verbosity = backup_verb;
                return l_False;
            }
        } else {
            assert(ret == l_Undef);
            goto end;
        }
    }
    finished = true;
    assert(solver->okay());

    end:
    uint32_t num_set = solver->get_zero_assigned_lits().size() - orig_vars_set;
    double time_used = cpuTime() - myTime;
    solver->conf.verbosity = backup_verb;

    if (solver->conf.verbosity) {
        if (!finished) {
            cout << "c [backbone-simpl] "
            << "skipping, taking too many conflicts."
            << endl;
        }
        cout << "c [backbone-simpl]"
        << " set: " << num_set
        << " T: " << std::setprecision(2) << time_used
        << endl;
    }

    if (!finished) {
        return l_Undef;
    }

    return l_True;
}
