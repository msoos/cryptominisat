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

#include "get_clause_query.h"

using namespace CMSat;

#include <limits>
#include "solver.h"
#include "occsimplifier.h"
#include "varreplacer.h"

GetClauseQuery::GetClauseQuery(Solver* _solver) :
    solver(_solver)
{}

void GetClauseQuery::start_getting_small_clauses(
    const uint32_t _max_len, const uint32_t _max_glue, bool _red,
    bool _bva_vars, bool _simplified)
{
    if (!outer_to_without_bva_map.empty()) {
        std::cerr << "ERROR: You forgot to call end_getting_small_clauses() last time!" <<endl;
        exit(-1);
    }

    assert(at == numeric_limits<uint32_t>::max());
    assert(watched_at == numeric_limits<uint32_t>::max());
    assert(watched_at_sub == numeric_limits<uint32_t>::max());
    assert(max_len >= 2);

    if (!red) {
        //We'd need to implement getting blocked clauses
        assert(solver->occsimplifier->get_num_elimed_vars() == 0);
    }
    red = _red;
    at = 0;
    watched_at = 0;
    watched_at_sub = 0;
    max_len = _max_len;
    max_glue = _max_glue;
    varreplace_at = 0;
    units_at = 0;
    comp_at = 0;
    comp_at_sum = 0;
    blocked_at = 0;
    blocked_at2 = 0;
    undef_at = 0;
    xor_detached_at = 0;
    bva_vars = _bva_vars;
    simplified = _simplified;
    if (simplified) {
        bva_vars = true;
        if (solver->get_num_bva_vars() != 0) {
            cout << "ERRROR! You must not have BVA variables for simplified CNF getting" << endl;
            exit(-1);
        }
        release_assert(red == false);
        release_assert(solver->get_num_bva_vars() == 0);
    }
    if (bva_vars) {
        outer_to_without_bva_map = solver->build_outer_to_without_bva_map_extended();
    } else {
        outer_to_without_bva_map = solver->build_outer_to_without_bva_map();
    }
    tmp_cl.clear();
}

vector<uint32_t> GetClauseQuery::translate_sampl_set(
    const vector<uint32_t>& sampl_set)
{
    if (simplified) {
        assert(solver->get_num_bva_vars() == 0);
        vector<uint32_t> ret;
        for(uint32_t v: sampl_set) {
            v = solver->varReplacer->get_var_replaced_with_outer(v);
            v = solver->map_outer_to_inter(v);
            assert(solver->varData[v].removed == Removed::none);
            if (!solver->seen[v]) {
                ret.push_back(v);
                solver->seen[v] = 1;
            }
        }
        for(uint32_t v: sampl_set) {
            v = solver->varReplacer->get_var_replaced_with_outer(v);
            v = solver->map_outer_to_inter(v);
            solver->seen[v] = 0;
        }
        return ret;
    } else {
        return sampl_set;
    }
}

void GetClauseQuery::get_all_irred_clauses(vector<Lit>& out)
{
    start_getting_small_clauses(
        numeric_limits<uint32_t>::max(),
        numeric_limits<uint32_t>::max(),
        false);
    bool ret = get_next_small_clause(out, true);
    assert(ret == false); //we must get all clauses, so nothing left
    end_getting_small_clauses();
}

bool GetClauseQuery::get_next_small_clause(vector<Lit>& out, bool all_in_one_go)
{
    out.clear();
    if (!solver->okay()) {
        if (at == 0) {
            at++;
            if (!all_in_one_go) {
                return true;
            } else {
                out.push_back(lit_Undef);
            }
        }
        return false;
    }

    //Adding units
    while (
        (!simplified && units_at < solver->nVarsOuter()) ||
        (simplified && units_at < solver->nVars())
    ) {
        const uint32_t v = units_at;
        if (solver->value(v) != l_Undef) {
            tmp_cl.clear();
            tmp_cl.push_back(Lit(v, solver->value(v) == l_False));
            if (!simplified) {
                tmp_cl = solver->clause_outer_numbered(tmp_cl);
            }
            if (bva_vars || all_vars_outside(tmp_cl)) {
                map_without_bva(tmp_cl);
                out.insert(out.end(), tmp_cl.begin(), tmp_cl.end());
                if (!all_in_one_go) {
                    units_at++;
                    return true;
                } else {
                    out.push_back(lit_Undef);
                }
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
                tmp_cl.clear();
                tmp_cl.push_back(l);
                tmp_cl.push_back(w.lit2());
                if (!simplified) {
                    tmp_cl = solver->clause_outer_numbered(tmp_cl);
                }
                if (bva_vars || all_vars_outside(tmp_cl)) {
                    map_without_bva(tmp_cl);
                    out.insert(out.end(), tmp_cl.begin(), tmp_cl.end());
                    if (!all_in_one_go) {
                        watched_at_sub++;
                        return true;
                    } else {
                        out.push_back(lit_Undef);
                    }
                }
            }
            watched_at_sub++;
        }
        watched_at++;
        watched_at_sub = 0;
    }

    //Replaced variables
    while (varreplace_at < solver->nVarsOuter()*2 && !simplified) {
        Lit l = Lit::toLit(varreplace_at);
        Lit l2 = solver->varReplacer->get_lit_replaced_with_outer(l);
        if (l2 != l) {
            tmp_cl.clear();
            tmp_cl.push_back(l);
            tmp_cl.push_back(~l2);
            if (bva_vars || all_vars_outside(tmp_cl)) {
                map_without_bva(tmp_cl);
                out.insert(out.end(), tmp_cl.begin(), tmp_cl.end());
                if (!all_in_one_go) {
                    varreplace_at++;
                    return true;
                } else {
                    out.push_back(lit_Undef);
                }
            }
        }
        varreplace_at++;
    }

    if (red) {
        assert(!simplified);
        while(at < solver->longRedCls[0].size()) {
            const ClOffset offs = solver->longRedCls[0][at];
            const Clause* cl = solver->cl_alloc.ptr(offs);
            if (cl->size() <= max_len
                && cl->stats.glue <= max_glue
            ) {
                tmp_cl = solver->clause_outer_numbered(*cl);
                if (bva_vars || all_vars_outside(tmp_cl)) {
                    map_without_bva(tmp_cl);
                    out.insert(out.end(), tmp_cl.begin(), tmp_cl.end());
                    if (!all_in_one_go) {
                        at++;
                        return true;
                    } else {
                        out.push_back(lit_Undef);
                    }
                }
            }
            at++;
        }

        assert(at >= solver->longRedCls[0].size());
        uint32_t at_lev1 = at-solver->longRedCls[0].size();
        while(at_lev1 < solver->longRedCls[1].size()) {
            const ClOffset offs = solver->longRedCls[1][at_lev1];
            const Clause* cl = solver->cl_alloc.ptr(offs);
            if (cl->size() <= max_len) {
                tmp_cl = solver->clause_outer_numbered(*cl);
                if (bva_vars || all_vars_outside(tmp_cl)) {
                    map_without_bva(tmp_cl);
                    out.insert(out.end(), tmp_cl.begin(), tmp_cl.end());
                    if (!all_in_one_go) {
                        at++;
                        return true;
                    } else {
                        out.push_back(lit_Undef);
                    }
                }
            }
            at++;
            at_lev1++;
        }
    } else {
        //Irred long clauses
        while(at < solver->longIrredCls.size()) {
            const ClOffset offs = solver->longIrredCls[at];
            const Clause* cl = solver->cl_alloc.ptr(offs);
            if (cl->size() <= max_len) {
                if (!simplified) {
                    tmp_cl = solver->clause_outer_numbered(*cl);
                } else {
                    tmp_cl.clear();
                    for(const auto& l: *cl) {
                        tmp_cl.push_back(l);
                    }
                }
                if (bva_vars || all_vars_outside(tmp_cl)) {
                    map_without_bva(tmp_cl);
                    out.insert(out.end(), tmp_cl.begin(), tmp_cl.end());
                    if (!all_in_one_go) {
                        at++;
                        return true;
                    } else {
                        out.push_back(lit_Undef);
                    }
                }
            }
            at++;
        }

        //Detached XOR clauses
        if (solver->detached_xor_clauses) {
            while(xor_detached_at < solver->detached_xor_repr_cls.size()) {
                Clause* cl = solver->cl_alloc.ptr(
                    solver->detached_xor_repr_cls[xor_detached_at]);
                assert(cl->_xor_is_detached);
                assert(cl->used_in_xor() && cl->used_in_xor_full());
                if (cl->size() <= max_len) {
                    if (!simplified) {
                        tmp_cl = solver->clause_outer_numbered(*cl);
                    } else {
                        tmp_cl.clear();
                        for(const auto& l: *cl) {
                            tmp_cl.push_back(l);
                        }
                    }
                    if (bva_vars || all_vars_outside(tmp_cl)) {
                        map_without_bva(tmp_cl);
                        out.insert(out.end(), tmp_cl.begin(), tmp_cl.end());
                        if (!all_in_one_go) {
                            xor_detached_at++;
                            return true;
                        } else {
                            out.push_back(lit_Undef);
                        }
                    }
                }
                xor_detached_at++;
            }
        }

        //Blocked clauses (already in OUTER notation)
        bool ret = true;
        while (ret && solver->occsimplifier && !simplified) {
            ret = solver->occsimplifier->get_blocked_clause_at(blocked_at, blocked_at2, tmp_cl);
            if (ret && all_vars_outside(tmp_cl)) {
                map_without_bva(tmp_cl);
                out.insert(out.end(), tmp_cl.begin(), tmp_cl.end());
                if (!all_in_one_go) {
                    return true;
                } else {
                    out.push_back(lit_Undef);
                }
            }
        }

        //Clauses that have a variable like a V ~a which means the variable MUST take a value
        //These are already in OUTER notation
        while (undef_at < solver->undef_must_set_vars.size() && !simplified) {
            uint32_t v = undef_at;
            if (solver->undef_must_set_vars[v]) {
                tmp_cl.clear();
                tmp_cl.push_back(Lit(v, false));
                tmp_cl.push_back(Lit(v, true));
                if (bva_vars || all_vars_outside(tmp_cl)) {
                    map_without_bva(tmp_cl);
                    out.insert(out.end(), tmp_cl.begin(), tmp_cl.end());
                    if (!all_in_one_go) {
                        undef_at++;
                        return true;
                    } else {
                        out.push_back(lit_Undef);
                    }
                }
            }
            undef_at++;
        }

    }
    return false;
}

void GetClauseQuery::end_getting_small_clauses()
{
    outer_to_without_bva_map.clear();
    outer_to_without_bva_map.shrink_to_fit();
}

bool GetClauseQuery::all_vars_outside(const vector<Lit>& cl) const
{
    for(const auto& l: cl) {
        if (solver->varData[solver->map_outer_to_inter(l.var())].is_bva)
            return false;
    }
    return true;
}

void GetClauseQuery::map_without_bva(vector<Lit>& cl)
{
    for(auto& l: cl) {
        l = Lit(outer_to_without_bva_map[l.var()], l.sign());
    }
}
