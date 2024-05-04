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

#include <limits>

#include "get_clause_query.h"
#include "solver.h"
#include "occsimplifier.h"
#include "varreplacer.h"

using namespace CMSat;

GetClauseQuery::GetClauseQuery(Solver* _solver) :
    solver(_solver)
{}

void GetClauseQuery::start_getting_constraints(bool _red, bool _simplified,
    uint32_t _max_len, uint32_t _max_glue) {

    assert(at == numeric_limits<uint32_t>::max());
    assert(watched_at == numeric_limits<uint32_t>::max());
    assert(watched_at_sub == numeric_limits<uint32_t>::max());
    assert(max_len >= 2);
    solver->clear_gauss_matrices(false);

    red = _red;
    at = 0;
    at_lev[0] = 0;
    at_lev[1] = 0;
    at_lev[2] = 0;
    watched_at = 0;
    watched_at_sub = 0;
    max_len = _max_len;
    max_glue = _max_glue;
    varreplace_at = 0;
    units_at = 0;
    comp_at = 0;
    comp_at_sum = 0;
    elimed_at = 0;
    elimed_at2 = 0;
    undef_at = 0;
    xor_at = 0;
    simplified = _simplified;
    if (simplified) {
        if (solver->get_num_bva_vars() != 0) {
            cout << "ERROR! You must not have BVA variables for simplified CNF getting" << endl;
            exit(-1);
        }
        release_assert(solver->get_num_bva_vars() == 0);
    }
}

// sampl_set is in OUTER notation
vector<uint32_t> GetClauseQuery::translate_sampl_set(
    const vector<uint32_t>& sampl_set)
{
    assert(solver->toClear.empty());
    if (simplified) {
        assert(solver->get_num_bva_vars() == 0);
        vector<uint32_t> ret;
        set<uint32_t> ret_set;
        for(uint32_t v: sampl_set) {
            v = solver->varReplacer->get_var_replaced_with_outer(v);
            v = solver->map_outer_to_inter(v);
            if (solver->value(v) != l_Undef) continue;
            /* assert(solver->varData[v].removed == Removed::none); */
            ret_set.insert(v);
        }
        for(uint32_t v: ret_set) ret.push_back(v);
        return ret;
    } else {
        return sampl_set;
    }
}

bool GetClauseQuery::get_next_constraint(std::vector<Lit>& out, bool& is_xor, bool& rhs) {
    out.clear();
    is_xor = false;
    rhs = true;
    if (!solver->okay()) {
        if (at == 0) {
            at++;
            return true;
        }
        return false;
    }

    //Adding units
    while (
        (!simplified && units_at < solver->nVarsOuter()) ||
        (simplified && units_at < solver->nVars()))
    {
        const uint32_t v = units_at;
        if (solver->value(v) != l_Undef) {
            out.clear();
            out.push_back(Lit(v, solver->value(v) == l_False));
            if (!simplified) out = solver->clause_outer_numbered(out);
            if (all_vars_outside(out)) {
                if (!simplified) if (!simplified) solver->map_inter_to_outer(out);
                units_at++;
                return true;
            }
        }
        units_at++;
    }

    //Adding binaries
    while(watched_at < solver->nVars()*2) {
        Lit l = Lit::toLit(watched_at);
        watch_subarray_const ws = solver->watches[l];
        while(watched_at_sub < ws.size()) {
            const Watched& w = ws[watched_at_sub];
            if (w.isBin() &&
                w.lit2() < l &&
                (w.red() == red)
            ) {
                out.clear();
                out.push_back(l);
                out.push_back(w.lit2());
                if (!simplified) out = solver->clause_outer_numbered(out);
                if (all_vars_outside(out)) {
                    if (!simplified) solver->map_inter_to_outer(out);
                    watched_at_sub++;
                    return true;
                }
            }
            watched_at_sub++;
        }
        watched_at++;
        watched_at_sub = 0;
    }

    //Replaced variables
    //These are already in OUTER notation
    while (varreplace_at < solver->nVarsOuter()*2 && !simplified) {
        Lit l = Lit::toLit(varreplace_at);
        Lit l2 = solver->varReplacer->get_lit_replaced_with_outer(l);
        if (l2 != l) {
            out.clear();
            out.push_back(l);
            out.push_back(~l2);
            if (all_vars_outside(out)) {
                varreplace_at++;
                return true;
            }
        }
        varreplace_at++;
    }

    if (red) {
        for(uint32_t lev = 0; lev < 3; lev++)
            while(at_lev[lev] < solver->longRedCls[lev].size()) {
                const ClOffset offs = solver->longRedCls[lev][at_lev[lev]];
                const Clause* cl = solver->cl_alloc.ptr(offs);
                if (cl->size() <= max_len
                    && cl->stats.glue <= max_glue
                ) {
                    if (!simplified) out = solver->clause_outer_numbered(*cl);
                    else {out.clear(); for(const auto& l: *cl) out.push_back(l);}
                    if (all_vars_outside(out)) {
                        if (!simplified) solver->map_inter_to_outer(out);
                        at_lev[lev]++;
                        return true;
                    }
                }
                at_lev[lev]++;
            }
    }

    //Irred long clauses
    while(!red && at < solver->longIrredCls.size()) {
        const ClOffset offs = solver->longIrredCls[at];
        const Clause* cl = solver->cl_alloc.ptr(offs);
        if (cl->size() <= max_len) {
            if (!simplified) out = solver->clause_outer_numbered(*cl);
            else {
                out.clear();
                for(const auto& l: *cl) out.push_back(l);
            }
            if (all_vars_outside(out)) {
                if (!simplified) solver->map_inter_to_outer(out);
                at++;
                return true;
            }
        }
        at++;
    }

    //XOR clauses
    while(!red && xor_at < solver->xorclauses.size()) {
        const auto x = solver->xorclauses[xor_at];
        if (x.size() <= max_len) {
            if (!simplified) out = solver->clause_outer_numbered(x.vars);
            else {
                out.clear();
                for(const auto& v: x.vars) out.push_back(Lit(v, false));
            }
            // NOTE; we must NEVER return the trivial XOR
            if (!x.trivial() && all_vars_outside(out)) {
                if (!simplified) solver->map_inter_to_outer(out);
                is_xor = true;
                rhs = x.rhs;
                xor_at++;
                return true;
            }
        }
        xor_at++;
    }

    //Elimed clauses
    //These are already in OUTER notation
    bool ret = true;
    while (!red && ret && solver->occsimplifier && !simplified) {
        ret = solver->occsimplifier->get_elimed_clause_at(elimed_at, elimed_at2, out, is_xor);
        if (ret && all_vars_outside(out)) {
            return true;
        }
    }

    //Clauses that have a variable like a V ~a which means the variable MUST take a value
    //These are already in OUTER notation
    while (!red && undef_at < solver->undef_must_set_vars.size() && !simplified) {
        uint32_t v = undef_at;
        if (solver->undef_must_set_vars[v]) {
            out.clear();
            out.push_back(Lit(v, false));
            out.push_back(Lit(v, true));
            if (all_vars_outside(out)) {
                undef_at++;
                return true;
            }
        }
        undef_at++;
    }
    return false;
}

bool GetClauseQuery::all_vars_outside(const vector<Lit>& cl) const {
    for(const auto& l: cl) if (solver->varData[solver->map_outer_to_inter(l.var())].is_bva)
        return false;

    return true;
}

void GetClauseQuery::end_getting_constraints() {}
