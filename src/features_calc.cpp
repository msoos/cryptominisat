#include <vector>
#include <cmath>

#include "solver.h"
#include "features_calc.h"

using std::vector;
using namespace CMSat;

template<class Function, class Function2>
void FeaturesCalc::for_one_clause(
    const Watched& cl
    , const Lit lit
    ,  Function func_each_cl
    ,  Function2 func_each_lit
) const {
    unsigned neg_vars = 0;
    unsigned pos_vars = 0;
    unsigned size = 0;

    switch (cl.getType()) {
        case CMSat::watch_binary_t: {
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

        case CMSat::watch_tertiary_t: {
            if (cl.red()) {
                //only irred cls
                break;
            }
            if (lit > cl.lit2()) {
                //only count once
                break;
            }

            assert(cl.lit2() < cl.lit3());

            pos_vars += !lit.sign();
            pos_vars += !cl.lit2().sign();
            pos_vars += !cl.lit3().sign();
            size = 3;
            neg_vars = size - pos_vars;
            func_each_cl(size, pos_vars, neg_vars);
            func_each_lit(lit, size, pos_vars, neg_vars);
            func_each_lit(cl.lit2(), size, pos_vars, neg_vars);
            func_each_lit(cl.lit3(), size, pos_vars, neg_vars);
            break;
        }

        case CMSat::watch_clause_t: {
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
    }
}

template<class Function, class Function2>
void FeaturesCalc::for_all_clauses(Function func_each_cl, Function2 func_each_lit) const
{
    for (size_t i = 0; i < solver->nVars() * 2; i++) {
        Lit lit = Lit::toLit(i);
        for (const Watched & w : solver->watches[lit.toInt()]) {
            for_one_clause(w, lit, func_each_cl, func_each_lit);
        }
    }
}

void FeaturesCalc::fill_vars_cls()
{
    feat.numVars = solver->nVars();
    feat.numClauses = solver->longIrredCls.size() + solver->binTri.irredBins + solver->binTri.irredTris;
    myVars.resize(solver->nVars());

    auto func_each_cl = [&](unsigned /*size*/, unsigned pos_vars, unsigned /*neg_vars*/) -> bool {
        if (pos_vars <= 1 ) {
            feat.horn += 1;
            return true;
        }
        return false;
    };
    auto func_each_lit = [&](Lit lit, unsigned /*size*/, unsigned pos_vars, unsigned /*neg_vars*/) -> void {
        if (pos_vars <= 1 ) {
            myVars[lit.var()].horn++;
        }

        if (!lit.sign()) {
            myVars[lit.var()].numPos++;
        }
        myVars[lit.var()].size++;
    };
    for_all_clauses(func_each_cl, func_each_lit);
}

void FeaturesCalc::calculate_clause_stats()
{
    auto empty_func = [](const Lit, unsigned /*size*/, unsigned /*pos_vars*/, unsigned /*neg_vars*/) -> void {};
    auto func_each_cl = [&](unsigned size, unsigned pos_vars, unsigned /*neg_vars*/) -> void {
        if (size == 0 ) {
            return;
        }

        double _size = (double)size / (double)feat.numVars;
        feat.vcg_cls_min = std::min(feat.vcg_cls_min, _size);
        feat.vcg_cls_max = std::max(feat.vcg_cls_max, _size);
        feat.vcg_cls_mean += _size;

        double _pnr = 0.5 + ((2.0 * (double)pos_vars - (double)size) / (2.0 * (double)size));
        feat.pnr_cls_min = std::min(feat.pnr_cls_min, _pnr);
        feat.pnr_cls_max = std::min(feat.pnr_cls_max, _pnr);
        feat.pnr_cls_mean += _pnr;
    };
    for_all_clauses(func_each_cl, empty_func);

    feat.vcg_cls_mean /= (double)feat.numClauses;
    feat.pnr_cls_mean /= (double)feat.numClauses;
    feat.horn /= (double)feat.numClauses;
    feat.binary = (double)solver->binTri.irredBins/(double)feat.numClauses;
    feat.trinary = (double)solver->binTri.irredTris/(double)feat.numClauses;

    feat.vcg_cls_spread = feat.vcg_cls_max - feat.vcg_cls_min;
    feat.pnr_cls_spread = feat.pnr_cls_max - feat.pnr_cls_min;
}

void FeaturesCalc::calculate_variable_stats()
{
    if (feat.numVars == 0)
        return;

    for ( int vv = 0; vv < (int)myVars.size(); vv++ ) {
        if ( myVars[vv].size == 0 ) {
            continue;
        }

        double _size = myVars[vv].size / (double)feat.numClauses;
        feat.vcg_var_min = std::min(feat.vcg_var_min, _size);
        feat.vcg_var_max = std::min(feat.vcg_var_max, _size);
        feat.vcg_var_mean += _size;

        double _pnr = 0.5 + ((2.0 * myVars[vv].numPos - myVars[vv].size)
                             / (2.0 * myVars[vv].size));
        feat.pnr_var_min = std::min(feat.pnr_var_min, _pnr);
        feat.pnr_var_max = std::min(feat.pnr_var_max, _pnr);
        feat.pnr_var_mean += _pnr;

        double _horn = myVars[vv].horn / (double)feat.numClauses;
        feat.horn_min = std::min(feat.horn_max, _horn);
        feat.horn_max = std::min(feat.horn_max, _horn);
        feat.horn_mean += _horn;
    }

    if (feat.vcg_var_mean > 0) {
        feat.vcg_var_mean /= (double)feat.numVars;
    }
    if (feat.pnr_var_mean > 0) {
        feat.pnr_var_mean /= (double)feat.numVars;
    }
    if (feat.horn_mean > 0) {
        feat.horn_mean /= (double)feat.numVars;
    }

    feat.vcg_var_spread = feat.vcg_var_max - feat.vcg_var_min;
    feat.pnr_var_spread = feat.pnr_var_max - feat.pnr_var_min;
    feat.horn_spread = feat.horn_max - feat.horn_min;
}

void FeaturesCalc::calculate_extra_clause_stats()
{
    auto empty_func = [](const Lit, unsigned /*size*/, unsigned /*pos_vars*/, unsigned /*neg_vars*/) -> void {};
    auto each_clause = [&](unsigned size, unsigned pos_vars, unsigned /*neg_vars*/) -> void {
        if ( size == 0 ) {
            return;
        }

        double _size = (double)size / (double)feat.numVars;
        feat.vcg_cls_std += (feat.vcg_cls_mean - _size) * (feat.vcg_cls_mean - _size);

        double _pnr = 0.5 + ((2.0 * (double)pos_vars - (double)size) / (2.0 * (double)size));
        feat.pnr_cls_std += (feat.pnr_cls_mean - _pnr) * (feat.pnr_cls_mean - _pnr);
    };
    for_all_clauses(each_clause, empty_func);

    if ( feat.vcg_cls_std > feat.eps && feat.vcg_cls_mean > feat.eps ) {
        feat.vcg_cls_std = sqrt(feat.vcg_cls_std / (double)feat.numClauses) / feat.vcg_cls_mean;
    } else {
        feat.vcg_cls_std = 0;
    }
    if ( feat.pnr_cls_std > feat.eps && feat.pnr_cls_mean > feat.eps ) {
        feat.pnr_cls_std = sqrt(feat.pnr_cls_std / (double)feat.numClauses) / feat.pnr_cls_mean;
    } else {
        feat.pnr_cls_std = 0;
    }
}

void FeaturesCalc::calculate_extra_var_stats()
{
    if (feat.numVars == 0)
        return;

    for ( int vv = 0; vv < (int)myVars.size(); vv++ ) {
        if ( myVars[vv].size == 0 ) {
            continue;
        }

        double _size = myVars[vv].size / (double)feat.numClauses;
        feat.vcg_var_std += (feat.vcg_var_mean - _size) * (feat.vcg_var_mean - _size);

        double _pnr = 0.5 + ((2.0 * myVars[vv].numPos - myVars[vv].size) / (2.0 * myVars[vv].size));
        feat.pnr_var_std += (feat.pnr_var_mean - _pnr) * (feat.pnr_var_mean - _pnr);

        double _horn = myVars[vv].horn / (double)feat.numClauses;
        feat.horn_std += (feat.horn_mean - _horn) * (feat.horn_mean - _horn);
    }
    if ( feat.vcg_var_std > feat.eps && feat.vcg_var_mean > feat.eps ) {
        feat.vcg_var_std = sqrt(feat.vcg_var_std / (double)feat.numVars) / feat.vcg_var_mean;
    } else {
        feat.vcg_var_std = 0;
    }

    if ( feat.pnr_var_std > feat.eps && feat.pnr_var_mean > feat.eps
        && feat.pnr_var_mean != 0
    ) {
        feat.pnr_var_std = sqrt(feat.pnr_var_std / (double)feat.numVars) / feat.pnr_var_mean;
    } else {
        feat.pnr_var_std = 0;
    }

    if ( feat.horn_std / (double)feat.numVars > feat.eps && feat.horn_mean > feat.eps
        && feat.horn_mean != 0
    ) {
        feat.horn_std = sqrt(feat.horn_std / (double)feat.numVars) / feat.horn_mean;
    } else {
        feat.horn_std = 0;
    }
}

void FeaturesCalc::calculate_cl_distributions(
    const vector<ClOffset>& clauses
    , struct Features::Distrib& distrib_data
) {
    if (clauses.empty()) {
        return;
    }

    double glue_mean = 0;
    double glue_var = 0;

    double size_mean = 0;
    double size_var = 0;

    double uip_use_mean = 0;
    double uip_use_var = 0;

    double activity_mean = 0;
    double activity_var = 0;

    //Calculate means
    for(ClOffset off: clauses)
    {
        const Clause& cl = *solver->cl_alloc.ptr(off);
        size_mean += cl.size();
        glue_mean += cl.stats.glue;
        activity_mean += cl.stats.activity;
        #ifdef STATS_NEEDED
        uip_use_mean += cl.stats.used_for_uip_creation;
        #endif
    }
    size_mean /= clauses.size();
    glue_mean /= clauses.size();
    activity_mean /= clauses.size();
    #ifdef STATS_NEEDED
    uip_use_mean /= clauses.size();
    #endif

    //Calculate variances
    for(ClOffset off: clauses)
    {
        const Clause& cl = *solver->cl_alloc.ptr(off);
        size_var += std::pow(size_mean-cl.size(), 2);
        glue_var += std::pow(glue_mean-cl.stats.glue, 2);
        activity_var += std::pow(activity_mean-cl.stats.activity, 2);
        #ifdef STATS_NEEDED
        uip_use_var += std::pow(uip_use_mean-cl.stats.used_for_uip_creation, 2);
        #endif
    }
    size_var /= clauses.size();
    glue_var /= clauses.size();
    activity_var /= clauses.size();
    #ifdef STATS_NEEDED
    uip_use_var /= clauses.size();
    #endif

    //Assign calculated values
    distrib_data.glue_distr_mean = glue_mean;
    distrib_data.glue_distr_var = glue_var;
    distrib_data.size_distr_mean = size_mean;
    distrib_data.size_distr_var = size_var;
    distrib_data.uip_use_distr_mean = uip_use_mean;
    distrib_data.uip_use_distr_var = uip_use_var;
    distrib_data.activity_distr_mean = activity_mean;
    distrib_data.activity_distr_var = activity_var;
}

Features FeaturesCalc::extract()
{
    double start_time = cpuTime();
    fill_vars_cls();

    feat.numVars = 0;
    for ( int vv = 0; vv < (int)myVars.size(); vv++ ) {
        if ( myVars[vv].size > 0 ) {
            feat.numVars++;
        }
    }
    if (feat.numVars > 0) {
        feat.var_cl_ratio = (double)feat.numVars/ (double)feat.numClauses;
    }

    calculate_clause_stats();
    calculate_variable_stats();

    calculate_extra_clause_stats();
    calculate_extra_var_stats();

    calculate_cl_distributions(solver->longRedCls, feat.red_cl_distrib);
    calculate_cl_distributions(solver->longIrredCls, feat.irred_cl_distrib);

    if (solver->conf.verbosity >= 2) {
        cout << "c [features] extracted"
        << solver->conf.print_times(cpuTime() - start_time)
        << endl;
    }

    return feat;
}
