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
#include "solvertypesmini.h"
#include <cstdint>
extern "C" {
#include "mpicosat/mpicosat.h"
}
#include "cryptominisat.h"
#include "varreplacer.h"
#include "../cadiback/cadiback.h"

using namespace CMSat;

bool Solver::backbone_simpl(int64_t orig_max_confl, bool cmsgen,
        const vector<uint32_t>& only_over, bool& backbone_done)
{
    if (!okay()) return false;
    vector<int> cnf;
    /* for(uint32_t i = 0; i < nVars(); i++) picosat_inc_max_var(picosat); */

    for(auto const& off: longIrredCls) {
        Clause* cl = cl_alloc.ptr(off);
        for(auto const& l1: *cl) {
            cnf.push_back(PICOLIT(l1));
        }
        cnf.push_back(0);
    }
    for(uint32_t i = 0; i < nVars()*2; i++) {
        Lit l1 = Lit::toLit(i);
        for(auto const& w: watches[l1]) {
            if (!w.isBin() || w.red()) continue;
            const Lit l2 = w.lit2();
            if (l1 > l2) continue;

            cnf.push_back(PICOLIT(l1));
            cnf.push_back(PICOLIT(l2));
            cnf.push_back(0);
        }
    }
    for(uint32_t i = 0; i < nVars(); i++) {
        if (value(i) == l_Undef) continue;
        cnf.push_back(PICOLIT(Lit(i, value(i) == l_False)));
        cnf.push_back(0);
    }

    vector<int> drop_cands;
    if (!only_over.empty()) {
        set<int> only_over_int;
        for(const auto& v: only_over) {
            Lit l = Lit(v, false);
            l = varReplacer->get_lit_replaced_with_outer(l);
            l = map_outer_to_inter(l);
            if (l.var() >= nVars()) continue;
            only_over_int.insert(l.var());
        }

        for(uint32_t i = 0; i < nVars(); i++) {
            if (value(i) != l_Undef) continue;
            if (varData[i].removed != Removed::none) continue;
            if (only_over_int.count(i)) continue;
            drop_cands.push_back(i+1);
        }
    }

    vector<int> learned_units;
    vector<int> learned_bins;
    int res = CadiBack::doit(cnf, conf.verbosity, drop_cands, learned_bins, learned_units);
    uint32_t num_units_added = 0;
    uint32_t num_bins_added = 0;
    if (res == 10) {
        vector<Lit> tmp;
        for(const auto& l: learned_units) {
            if (l == 0) continue;
            const Lit lit = Lit(abs(l)-1, l < 0);
            if (value(lit.var()) != l_Undef) continue;
            if (varData[lit.var()].removed != Removed::none) continue;
            tmp.clear();
            tmp.push_back(lit);
            add_clause_int(tmp);
            num_units_added++;
            if (!okay()) goto end;
        }
        bool ignore = false;

        tmp.clear();
        for(const auto& l: learned_bins) {
            if (l == 0) {
                if (ignore) {
                    ignore = false;
                    tmp.clear();
                    continue;
                }
                assert(tmp.size() == 2);
                auto ret = add_clause_int(tmp, true);
                assert(ret == nullptr);
                num_bins_added++;
                if (!okay()) goto end;
                ignore = false;
                tmp.clear();
                continue;
            }
            const Lit lit = Lit(abs(l)-1, l < 0);
            cout << "lit: " << lit << endl;
            if (varData[lit.var()].removed != Removed::none) {ignore = true; continue;}
            if (value(lit.var()) != l_Undef) {ignore = true; continue;}
            tmp.push_back(lit);
        }
        backbone_done = true;
    } else {
        ok = false;
    }
end:
    verb_print(1, "[backbone-simpl] res: " << res
            <<  " num units: " << num_units_added
            <<  " num bins: " << num_bins_added
            << " T: " << std::fixed << std::setprecision(2) << cpuTime());
    return res == 10;
}

void Solver::detach_and_free_all_irred_cls()
{
    for(auto& ws: watches) {
        uint32_t j = 0;
        for(uint32_t i = 0; i < ws.size(); i++) {
            if (ws[i].isBin()) {
                if (ws[i].red()) ws[j++] = ws[i];
                continue;
            }
            assert(!ws[i].isBNN());
            assert(ws[i].isClause());
            Clause* cl = cl_alloc.ptr(ws[i].get_offset());
            if (cl->red()) ws[j++] = ws[i];
        }
        ws.resize(j);
    }
    binTri.irredBins = 0;
    for(auto& c: longIrredCls) free_cl(c);
    longIrredCls.clear();
    litStats.irredLits = 0;
    cl_alloc.consolidate(this, true);
}

bool Solver::backbone_simpl_old(int64_t orig_max_confl, bool cmsgen, bool& finished)
{
    execute_inprocess_strategy(false, "must-renumber");
    if (!okay()) return false;
    assert(get_num_bva_vars() == 0);
    verb_print(1, "[backbone-simpl] starting backbone simplification...");

    double my_time = cpuTime();
    Lit l;
    uint32_t undefs = 0;
    uint32_t falses = 0;
    uint32_t tried = 0;
    const auto orig_trail_size = trail.size();

    vector<Lit> tmp_clause;
    vector<lbool> old_model;
    uint32_t num_seen_flipped = 0;
    vector<char> seen_flipped(nVars(), 0);

    if (cmsgen) {
        //CMSGen-based seen_flipped detection, so we don't need to query so much
        SATSolver s2;
        copy_to_simp(&s2);

        uint64_t last_num_conflicts = 0;
        int64_t remaining_confls = orig_max_confl;
        s2.set_max_confl(remaining_confls);
        uint32_t num_runs = 0;
        auto s2_ret = s2.solve();
        remaining_confls -= s2.get_sum_conflicts() - last_num_conflicts;
        if (s2_ret == l_True) {
            old_model = s2.get_model();
            s2.set_up_for_sample_counter(100);
            for(uint32_t i = 0; i < 30 && remaining_confls > 0; i++) {
                last_num_conflicts = s2.get_sum_conflicts();
                s2.set_max_confl(remaining_confls);
                s2_ret = s2.solve();
                remaining_confls -= s2.get_sum_conflicts() - last_num_conflicts;
                if (s2_ret == l_Undef) break;
                num_runs++;
                const auto& this_model = s2.get_model();
                for(uint32_t i2 = 0, max = s2.nVars(); i2 < max; i2++) {
                    if (value(i2) != l_Undef) continue;
                    if (varData[i2].removed != Removed::none) continue;
                    if (seen_flipped[i2]) continue;
                    if (this_model[i2] != old_model[i2]) {
                        seen_flipped[i2] = 1;
                        num_seen_flipped++;
                    }
                }
            }
        }
        verb_print(1, "[backbone-simpl] num seen flipped: "
            << num_seen_flipped
            << " conflicts used: " << print_value_kilo_mega(s2.get_sum_conflicts())
            << " num runs succeeded: " << num_runs
            << " T: " << std::fixed << std::setprecision(2) << (cpuTime() - my_time));
    }
    my_time = cpuTime();

    // random order
    vector<uint32_t> var_order;
    for(uint32_t var = 0; var < nVars(); var++) {
        if (seen_flipped[var]) continue;
        if (value(var) != l_Undef) continue;
        if (varData[var].removed != Removed::none) continue;
        var_order.push_back(var);
    }
    std::shuffle(var_order.begin(), var_order.end(), mtrand);

    vector<lbool> old_model2(nVars(), l_Undef);
    vector<Lit> assumps;
    SATSolver* s = new SATSolver();

        s->new_vars(nVarsOuter());
        start_getting_constraints(false);
        std::vector<Lit> c; bool is_xor; bool rhs; bool ret2 = true;
        while (ret2) {
            ret2 = get_next_constraint(c, is_xor, rhs);
            if (!ret2) break;
            if (is_xor) s->add_xor_clause(c, rhs);
            else s->add_clause(c);
        }
        end_getting_constraints();

    s->set_max_confl(orig_max_confl*20);
    auto ret = s->solve();
    if (ret == l_Undef || ret == l_False) goto end;
    if (ret == l_True) {
        for(uint32_t i = 0; i < nVars(); i++) {
            assert(s->get_model()[i] != l_Undef);
            old_model2[i] = s->get_model()[i];
        }
    }

    verb_print(1, "[backbone-simpl] candidates: " << var_order.size());
    for(const auto& var: var_order) {
        if (seen_flipped[var]) continue;
        if (value(var) != l_Undef) continue;
        if (varData[var].removed != Removed::none) continue;

        l = Lit(var, old_model2[var]==l_False);

        //There is definitely a solution with "l". Let's see if ~l fails.
        assumps.clear();
        assumps.push_back(~l);
        s->set_max_confl(orig_max_confl*2);
        ret = s->solve(&assumps);
        verb_print(1, "ret: " << ret << " confl: " << s->get_sum_conflicts() << " max: " << orig_max_confl);
        tried++;

        if (ret == l_True) {
            for(uint32_t i2 = 0; i2 < nVars(); i2++) {
                auto val = s->get_model()[i2];
                if (seen_flipped[i2] ||
                        value(i2) != l_Undef || val == l_Undef ||
                        varData[i2].removed != Removed::none) continue;
                if (val != old_model2[i2]) {
                    seen_flipped[i2] = 1;
                    /* cout << "seen flipped: " << i2 << endl; */
                    num_seen_flipped++;
                }
            }
        } else if (ret == l_False) {
            tmp_clause.clear();
            tmp_clause.push_back(l);
            Clause* ptr = add_clause_int(tmp_clause);
            assert(ptr == nullptr);
            s->add_clause(tmp_clause);
            falses++;
            if (!okay()) goto end;
        } else {
            assert(ret == l_Undef);
            undefs++;
        }
    }
    if (undefs==0) finished = true;
    assert(okay());

    end:
    delete s;
    uint32_t num_set = trail.size() - orig_trail_size;
    double time_used = cpuTime() - my_time;

    verb_print(1,
        "[backbone-simpl]"
        << " finished: " << finished
        << " falses: " << falses
        << " undefs: " << undefs
        << " tried: "  << tried
        << " set: " << num_set
        << " T: " << std::setprecision(2) << time_used);

    return okay();
}
