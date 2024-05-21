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

#include "solver.h"
extern "C" {
#include "mpicosat/mpicosat.h"
}
#include "cryptominisat.h"
#include "../cadiback/cadiback.h"

using namespace CMSat;

bool Solver::backbone_simpl(int64_t orig_max_confl, bool& finished)
{
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
    vector<int> ret;
    int sat = CadiBack::doit(cnf, conf.verbosity, ret);
    if (sat) {
        vector<Lit> tmp;
        for(const auto& l: ret) {
            if (l == 0) continue;
            const Lit lit = Lit(abs(l)-1, l < 0);
            if (value(lit.var()) != l_Undef) continue;
            if (varData[lit.var()].removed != Removed::none) continue;
            tmp.clear();
            tmp.push_back(lit);
            add_clause_int(tmp);
        }
        finished = true;
    }
    return sat != 20;
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
        int64_t remaining_confls = orig_max_confl/10;
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

    int64_t orig_max_props = orig_max_confl*1000LL;
    if (orig_max_props < orig_max_confl) orig_max_props = orig_max_confl;
    vector<int> old_model2(nVars(), 0);
    PicoSAT* picosat = build_picosat();
    picosat_set_propagation_limit(picosat, orig_max_props);
    auto ret = picosat_sat(picosat, -1);
    if (ret == PICOSAT_UNKNOWN || ret == PICOSAT_UNSATISFIABLE) goto end;
    if (ret == PICOSAT_SATISFIABLE) {
        for(uint32_t i = 0; i < nVars(); i++) {
            old_model2[i] = picosat_deref(picosat, i+1);
        }
    }

    for(const auto& var: var_order) {
        if (seen_flipped[var]) continue;
        if (value(var) != l_Undef) continue;
        if (varData[var].removed != Removed::none) continue;

        l = Lit(var, old_model2[var]==-1);
        auto top_val = picosat_deref_toplevel(picosat, l.var()+1);
        if (top_val != 0) {
            if (l.sign()) assert(top_val == -1);
            else assert(top_val == 1);
            goto next;
        }

        //There is definitely a solution with "l". Let's see if ~l fails.
        picosat_assume(picosat, PICOLIT(~l));
        ret = picosat_sat(picosat, -1);
        tried++;

        if (ret == PICOSAT_SATISFIABLE) {
            for(uint32_t i2 = 0; i2 < nVars(); i2++) {
                auto val = picosat_deref(picosat, i2+1);
                if (seen_flipped[i2] ||
                        value(i2) != l_Undef || val == 0 ||
                        varData[i2].removed != Removed::none) continue;
                if (val != old_model2[i2]) {
                    seen_flipped[i2] = 1;
                    num_seen_flipped++;
                }
            }
        } else if (ret == PICOSAT_UNSATISFIABLE) {
            next:
            tmp_clause.clear();
            tmp_clause.push_back(l);
            Clause* ptr = add_clause_int(tmp_clause);
            assert(ptr == nullptr);
            falses++;
            if (orig_max_props + 5000 > orig_max_props) orig_max_props += 5000;
            picosat_set_propagation_limit(picosat, orig_max_props);
            if (!okay()) goto end;
        } else {
            assert(ret == PICOSAT_UNKNOWN);
            undefs++;
            goto end;
        }
    }
    if (undefs==0) finished = true;
    assert(okay());

    end:
    const auto used_props = picosat_propagations(picosat);
    picosat_reset(picosat);
    uint32_t num_set = trail.size() - orig_trail_size;
    double time_used = cpuTime() - my_time;

    verb_print(1,
        "[backbone-simpl]"
        << " finished: " << finished
        << " falses: " << falses
        << " undefs: " << undefs
        << " tried: "  << tried
        << " set: " << num_set
        << " props used: " << print_value_kilo_mega(used_props)
        << " T: " << std::setprecision(2) << time_used);

    return okay();
}
