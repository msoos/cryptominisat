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


#include "vardistgen.h"
#include "solver.h"
#include "sqlstats.h"

using namespace CMSat;

VarDistGen::VarDistGen(Solver* _solver) :
    solver(_solver)
{}

double VarDistGen::compute_tot_act_vsids(Clause* cl) const
{
    double tot_var_acts = 0.0;
    for(Lit l: *cl) {
        tot_var_acts += solver->var_act_vsids[l.var()];
    }
    tot_var_acts += 10e-300;
    //NOTE Kuldeep wants to re-visit
    tot_var_acts = std::log2(tot_var_acts)/std::log2(solver->max_vsids_act+10e-300);
    return tot_var_acts;
}

void VarDistGen::calc()
{
    double myTime = cpuTime();
    data.clear();
    data.resize(solver->nVars());

    for(auto& off: solver->longIrredCls) {
        Clause* cl = solver->cl_alloc.ptr(off);
        double tot_var_acts = compute_tot_act_vsids(cl);

        for(Lit l: *cl) {
            data[l.var()].irred.num_times_in_long_clause++;
            data[l.var()].irred.tot_num_lit_of_long_cls_it_appears_in+=cl->size();
            if (solver->varData[l.var()].stable_polarity ^ !l.sign()) {
                data[l.var()].irred.satisfies_cl++;
            } else {
                data[l.var()].irred.falsifies_cl++;
            }
            data[l.var()].irred.sum_var_act_of_cls += tot_var_acts;
        }
    }

    for(auto& x: solver->longRedCls) {
        for(auto& off: x) {
            Clause* cl = solver->cl_alloc.ptr(off);
            double tot_var_acts = compute_tot_act_vsids(cl);
            for(Lit l: *cl) {
                data[l.var()].red.num_times_in_long_clause++;
                data[l.var()].red.tot_num_lit_of_long_cls_it_appears_in+=cl->size();
                if (std::log2(solver->max_cl_act+10e-300) != 0) {
                    data[l.var()].tot_act_long_red_cls +=
                        std::log2((double)cl->stats.activity+10e-300)
                            /std::log2(solver->max_cl_act+10e-300);
                }

                if (solver->varData[l.var()].stable_polarity ^ !l.sign()) {
                    data[l.var()].red.satisfies_cl++;
                } else {
                    data[l.var()].red.falsifies_cl++;
                }
                data[l.var()].red.sum_var_act_of_cls += tot_var_acts;
            }
        }
    }

    for(uint32_t i = 0; i < solver->nVars()*2; i++) {
        Lit l = Lit::toLit(i);
        for(Watched& w: solver->watches[l]) {
            if (w.isBin() && l < w.lit2()) {
                if (w.red()) {
                    data[l.var()].red.num_times_in_bin_clause++;
                    data[l.var()].red.tot_num_lit_of_bin_it_appears_in+=2;
                    if (solver->varData[l.var()].stable_polarity ^ !l.sign()) {
                        data[l.var()].red.satisfies_cl++;
                    } else {
                        data[l.var()].red.falsifies_cl++;
                    }
                } else {
                    data[l.var()].irred.num_times_in_bin_clause++;
                    data[l.var()].irred.tot_num_lit_of_bin_it_appears_in+=2;
                    if (solver->varData[l.var()].stable_polarity ^ !l.sign()) {
                        data[l.var()].irred.satisfies_cl++;
                    } else {
                        data[l.var()].irred.falsifies_cl++;
                    }
                }
            }
        }
    }

    double time_used = cpuTime() - myTime;
    if (solver->conf.verbosity) {
        cout << "c [vardistgen] generated var distribution data "
        << solver->conf.print_times(time_used)
        << endl;
    }

    if (solver->sqlStats) {
        solver->sqlStats->time_passed_min(
            solver
            , "var-dist-gen"
            , time_used
        );
    }
}

#ifdef STATS_NEEDED_BRANCH
void VarDistGen::dump()
{
    for(uint32_t i = 0; i < solver->nVars(); i++) {
        uint32_t outer_var = solver->map_inter_to_outer(i);
        solver->sqlStats->var_dist(
            outer_var, data[i], solver);
    }
}
#endif
