/******************************************
Copyright (c) 2016, Yuri Malitsky and Horst Samulowitz
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

#include <vector>
#include <cmath>

#include "solver.h"
#include "sqlstats.h"
#include "satzilla_features_calc.h"

using std::vector;
using namespace CMSat;

template<class Function, class Function2>
void SatZillaFeaturesCalc::for_one_clause(
    const Watched& cl
    , const Lit lit
    ,  Function func_each_cl
    ,  Function2 func_each_lit
) const {
    unsigned neg_vars = 0;
    unsigned pos_vars = 0;
    unsigned size = 0;

    switch (cl.getType()) {
        case WatchType::watch_binary_t: {
            if (cl.red()) {
                //only irred cls
                break;
            }
            if (lit > cl.lit2()) {
                //only count once
                break;
            }

            pos_vars += !lit.sign();
            pos_vars += !cl.lit2().sign();
            size = 2;
            neg_vars = size - pos_vars;
            func_each_cl(size, pos_vars, neg_vars);
            func_each_lit(lit, size, pos_vars, neg_vars);
            func_each_lit(cl.lit2(), size, pos_vars, neg_vars);
            break;
        }
        case WatchType::watch_bnn_t:
            release_assert(false && "Not implemented");
            break;

        case WatchType::watch_clause_t: {
            const Clause& clause = *solver->cl_alloc.ptr(cl.get_offset());
            if (clause.red()) {
                //only irred cls
                break;
            }
            if (clause[0] < clause[1]) {
                //only count once
                break;
            }

            for (const Lit cl_lit : clause) {
                pos_vars += !cl_lit.sign();
            }
            size = clause.size();
            neg_vars = size - pos_vars;
            func_each_cl(size, pos_vars, neg_vars);
            for (const Lit cl_lit : clause) {
                func_each_lit(cl_lit, size, pos_vars, neg_vars);
            }
            break;
        }

        case WatchType::watch_idx_t: {
             // This should never be here
            assert(false);
            exit(-1);
            break;
        }
    }
}

template<class Function, class Function2>
void SatZillaFeaturesCalc::for_all_clauses(Function func_each_cl, Function2 func_each_lit) const
{
    for (size_t i = 0; i < solver->nVars() * 2; i++) {
        Lit lit = Lit::toLit(i);
        for (const Watched & w : solver->watches[lit]) {
            for_one_clause(w, lit, func_each_cl, func_each_lit);
        }
    }
}

void SatZillaFeaturesCalc::fill_vars_cls()
{
    satzilla_feat.numClauses = solver->long_irred_cls.size() + solver->bin_tri.irred_bins;
    var_occs.clear();
    var_occs.resize(solver->nVars(), 0);
    auto func_each_cl = [&](unsigned /*size*/, unsigned pos_vars, unsigned /*neg_vars*/) -> void {
        if (pos_vars <= 1 ) satzilla_feat.horn += 1;
    };
    auto func_each_lit = [&](Lit lit, unsigned /*size*/, unsigned /*pos_vars*/, unsigned /*neg_vars*/) -> void {
        var_occs[lit.var()]++;
    };
    for_all_clauses(func_each_cl, func_each_lit);

    satzilla_feat.numVars = 0;
    for(const auto& occ: var_occs) satzilla_feat.numVars += (occ > 0);
    if (satzilla_feat.numClauses > 0) {
        satzilla_feat.var_cl_ratio = satzilla_feat.numVars / satzilla_feat.numClauses;
        satzilla_feat.horn /= satzilla_feat.numClauses;
        satzilla_feat.binary = float_div(solver->bin_tri.irred_bins, satzilla_feat.numClauses);
    }
}

void SatZillaFeaturesCalc::calculate_red_distributions()
{
    const auto& clauses = solver->long_red_cls[0];
    if (clauses.empty()) return;

    double glue_mean = 0;
    double size_mean = 0;
    for(ClOffset off: clauses) {
        const Clause& cl = *solver->cl_alloc.ptr(off);
        size_mean += cl.size();
        glue_mean += cl.stats.glue;
    }
    size_mean /= clauses.size();
    glue_mean /= clauses.size();

    double glue_var = 0;
    double size_var = 0;
    for(ClOffset off: clauses) {
        const Clause& cl = *solver->cl_alloc.ptr(off);
        size_var += std::pow(size_mean-cl.size(), 2);
        glue_var += std::pow(glue_mean-cl.stats.glue, 2);
    }
    satzilla_feat.red_glue_distr_mean = glue_mean;
    satzilla_feat.red_glue_distr_var = glue_var / clauses.size();
    satzilla_feat.red_size_distr_mean = size_mean;
    satzilla_feat.red_size_distr_var = size_var / clauses.size();
}

SatZillaFeatures SatZillaFeaturesCalc::extract()
{
    double start_time = cpu_time();
    fill_vars_cls();
    calculate_red_distributions();

    double time_used = cpu_time() - start_time;
    if (solver->conf.verbosity) {
        cout << solver->conf.prefix << "[szfeat] satzilla features extracted "
        << solver->conf.print_times(time_used)
        << endl;
    }
    if (solver->sql_stats) {
        solver->sql_stats->time_passed_min(
            solver
            , "satzilla"
            , time_used
        );
    }

    return satzilla_feat;
}
