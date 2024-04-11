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

#include "constants.h"
#include "solver.h"
#include <random>
#include "varreplacer.h"

using namespace CMSat;

bool Solver::full_probe(const bool bin_only)
{
    assert(okay());
    assert(decisionLevel() == 0);
    frat_func_start();

    const size_t orig_num_free_vars = solver->get_num_free_vars();
    double my_time = cpuTime();
    int64_t start_bogoprops = solver->propStats.bogoProps;
    int64_t bogoprops_to_use =
        solver->conf.full_probe_time_limitM*1000ULL*1000ULL
        *solver->conf.global_timeout_multiplier;
    uint64_t probed = 0;
    const auto orig_repl = varReplacer->get_num_replaced_vars();

    vector<uint32_t> vars;
    for(uint32_t i = 0; i < nVars(); i++) {
        Lit l(i, false);
        if (value(l) == l_Undef && varData[i].removed == Removed::none)
            vars.push_back(i);
    }
    std::shuffle(vars.begin(), vars.end(), mtrand);

    for(auto const& v: vars) {
        if ((int64_t)solver->propStats.bogoProps > start_bogoprops + bogoprops_to_use)
            break;

        uint32_t min_props;
        Lit l(v, false);

        //we have seen it in every combination, nothing will be learnt
        if (seen2[l.var()] == 3) continue;

        if (value(l) == l_Undef &&
            varData[v].removed == Removed::none)
        {
            probed++;
            bool ret;
            if (bin_only) ret = probe_inter<true>(l, min_props);
            else ret = probe_inter<false>(l, min_props);
            if (!ret) goto cleanup;

            if (conf.verbosity >= 5) {
                const double time_remain = 1.0-float_div(
                (int64_t)solver->propStats.bogoProps-start_bogoprops, bogoprops_to_use);
                cout << "c probe time remain: " << time_remain << " probed: " << probed
                << " set: "  << (orig_num_free_vars - solver->get_num_free_vars())
                << " T: " << (cpuTime() - my_time)
                << endl;
            }
        }
    }

    cleanup:
    std::fill(seen2.begin(), seen2.end(), 0);

    const double time_used = cpuTime() - my_time;
    const double time_remain = 1.0-float_div(
        (int64_t)solver->propStats.bogoProps-start_bogoprops, bogoprops_to_use);
    const bool time_out = ((int64_t)solver->propStats.bogoProps > start_bogoprops + bogoprops_to_use);

    verb_print(1,
        "[full-probe] "
        << " bin_only: " << bin_only
        << " set: "
        << (orig_num_free_vars - solver->get_num_free_vars())
        << " repl: " << (varReplacer->get_num_replaced_vars() - orig_repl)
        << solver->conf.print_times(time_used,  time_out, time_remain));


    if (solver->sqlStats) {
        solver->sqlStats->time_passed(
            solver
            , "full-probe"
            , time_used
            , time_out
            , time_remain
        );
    }
    frat_func_end();

    return okay();
}

template<bool bin_only> bool Solver::probe_inter(const Lit l, uint32_t& min_props)
{
    propStats.bogoProps+=2;

    //Probe l
    uint32_t old_trail_size = trail.size();
    new_decision_level();
    enqueue_light(l);
    PropBy p = propagate_light<bin_only>();
    min_props = trail.size() - old_trail_size;
    for(uint32_t i = old_trail_size+1; i < trail.size(); i++) {
        toClear.push_back(trail[i].lit);
        //seen[x] == 0 -> not propagated
        //seen[x] == 1 -> propagated as POS
        //seen[x] == 2 -> propagated as NEG
        const auto var = trail[i].lit.var();
        seen[var] = 1+(int)trail[i].lit.sign();
        seen2[var] |= 1+(int)trail[i].lit.sign();
    }
    cancelUntil_light();

    //Check result
    if (!p.isnullptr()) {
        vector<Lit> lits = {~l};
        add_clause_int(lits);
        goto end;
    }

    //Probe ~l
    old_trail_size = trail.size();
    new_decision_level();
    enqueue_light(~l);
    p = propagate_light<bin_only>();
    min_props = std::min<uint32_t>(min_props, trail.size() - old_trail_size);
    probe_inter_tmp.clear();
    for(uint32_t i = old_trail_size+1; i < trail.size(); i++) {
        Lit lit = trail[i].lit;
        uint32_t var = trail[i].lit.var();
        seen2[var] |= 1+(int)trail[i].lit.sign();
        if (seen[var] == 0) continue;

        if (lit.sign() == seen[var]-1) {
            //Same sign both times (set value of literal)
            probe_inter_tmp.push_back(lit);
        } else {
            //Inverse sign in the 2 cases (literal equivalence)
            probe_inter_tmp.push_back(lit_Undef);
            probe_inter_tmp.push_back(~lit);
        }
    }
    cancelUntil_light();

    //Check result
    if (!p.isnullptr()) {
        vector<Lit> lits = {l};
        add_clause_int(lits);
        goto end;
    }

    //Deal with bothprop
    for(uint32_t i = 0; i < probe_inter_tmp.size(); i++) {
        Lit bp_lit = probe_inter_tmp[i];
        if (bp_lit != lit_Undef) {
            //I am not going to deal with the messy version of it already being set
            if (value(bp_lit) == l_Undef) {
                *solver->frat << add << ++clauseID << ~l << bp_lit << fin;
                const int32_t c1 = clauseID;
                *solver->frat << add << ++clauseID << l << bp_lit << fin;
                const int32_t c2 = clauseID;
                enqueue<true>(bp_lit);
                *solver->frat << del << c1 << ~l << bp_lit << fin;
                *solver->frat << del << c2 << l << bp_lit << fin;
            }
        } else {
            //First we must propagate all the enqueued facts
            p = propagate<true>();
            if (!p.isnullptr()) {
                ok = false;
                goto end;
            }

            //Add binary XOR
            i++;
            bp_lit = probe_inter_tmp[i];
            vector<Lit> lits(2);
            lits[0] = l; lits[1] = ~bp_lit;
            add_clause_int(lits);
            if (okay()) {
                lits[0]^=true; lits[1]^=true;
                add_clause_int(lits);
            }
            if (!okay()) goto end;
        }
    }

    //Propagate all enqueued facts due to bprop
    p = propagate<true>();
    if (!p.isnullptr()) {
        ok = false;
        goto end;
    }

    end:
    for(auto clear_l: toClear) seen[clear_l.var()] = 0;
    toClear.clear();
    return okay();
}

lbool Solver::probe_outside(Lit l, uint32_t& min_props)
{
    assert(decisionLevel() == 0);
    assert(l.var() < nVarsOuter());
    if (!ok) return l_False;

    l = varReplacer->get_lit_replaced_with_outer(l);
    l = map_outer_to_inter(l);
    if (varData[l.var()].removed != Removed::none) {
        //TODO
        return l_Undef;
    }
    if (value(l) != l_Undef) {
        return l_Undef;
    }

    probe_inter<false>(l, min_props);
    if (!okay()) return l_False;
    return l_Undef;
}
