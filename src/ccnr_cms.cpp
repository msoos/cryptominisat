/******************************************
Copyright (c) 2018, Mate Soos <soos.mate@gmail.com>

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

#include "time_mem.h"
#include <limits>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include "constants.h"
#include "ccnr_cms.h"
#include "solver.h"
#include "ccnr.h"
#include "sqlstats.h"
//#define SLOW_DEBUG

using namespace CMSat;

CMS_ccnr::CMS_ccnr(Solver* _solver) :
    solver(_solver),
    seen(_solver->seen),
    toClear(_solver->toClear)
{
    ls_s = new CCNR::ls_solver(solver->conf.sls_ccnr_asipire);
    ls_s->set_verbosity(solver->conf.verbosity);
}

CMS_ccnr::~CMS_ccnr()
{
    delete ls_s;
}

lbool CMS_ccnr::main(const uint32_t num_sls_called)
{
    //It might not work well with few number of variables
    //rnovelty could also die/exit(-1), etc.
    if (solver->nVars() < 50 ||
        solver->binTri.irredBins + solver->longIrredCls.size() < 10
    ) {
        if (solver->conf.verbosity) {
            cout << "c [ccnr] too few variables & clauses"
            << endl;
        }
        return l_Undef;
    }
    double startTime = cpuTime();

    if (!init_problem()) {
        //it's actually l_False under assumptions
        //but we'll set the real SAT solver deal with that
        if (solver->conf.verbosity) {
            cout << "c [ccnr] problem UNSAT under assumptions, returning to main solver"
            << endl;
        }
        return l_Undef;
    }

    vector<bool> phases(solver->nVars()+1);
    for(uint32_t i = 0; i < solver->nVars(); i++) {
        phases[i+1] = solver->varData[i].polarity;
    }

    int res = ls_s->local_search(&phases, solver->conf.yalsat_max_mems*2*1000*1000);
    lbool ret = deal_with_solution(res, num_sls_called);

    double time_used = cpuTime()-startTime;
    if (solver->conf.verbosity) {
        cout << "c [ccnr] time: " << time_used << endl;
    }
    if (solver->sqlStats) {
        solver->sqlStats->time_passed_min(
            solver
            , "sls-ccnr"
            , time_used
        );
    }

    return ret;
}

template<class T>
CMS_ccnr::add_cl_ret CMS_ccnr::add_this_clause(const T& cl)
{
    uint32_t sz = 0;
    bool sat = false;
    yals_lits.clear();
    for(size_t i3 = 0; i3 < cl.size(); i3++) {
        Lit lit = cl[i3];
        assert(solver->varData[lit.var()].removed == Removed::none);
        lbool val = l_Undef;
        if (solver->value(lit) != l_Undef) {
            val = solver->value(lit);
        } else {
            val = solver->lit_inside_assumptions(lit);
        }

        if (val == l_True) {
            //clause is SAT, skip!
            sat = true;
            continue;
        } else if (val == l_False) {
            continue;
        }
        int l = lit.var()+1;
        l *= lit.sign() ? -1 : 1;
        yals_lits.push_back(l);
        sz++;
    }
    if (sat) {
        return add_cl_ret::skipped_cl;
    }
    if (sz == 0) {
        //it's unsat because of assumptions
        if (solver->conf.verbosity) {
            cout << "c [walksat] UNSAT because of assumptions in clause: " << cl << endl;
        }
        return add_cl_ret::unsat;
    }

    for(auto& lit: yals_lits) {
        ls_s->_clauses[cl_num].literals.push_back(CCNR::lit(lit, cl_num));
    }
    cl_num++;

    return add_cl_ret::added_cl;
}

bool CMS_ccnr::init_problem()
{
    if (solver->check_assumptions_contradict_foced_assignment())
    {
        return false;
    }
    #ifdef SLOWDEBUG
    solver->check_stats();
    #endif

    ls_s->_num_vars = solver->nVars();
    ls_s->_num_clauses = solver->longIrredCls.size() + solver->binTri.irredBins;
    ls_s->make_space();

    vector<Lit> this_clause;
    for(size_t i2 = 0; i2 < solver->nVars()*2; i2++) {
        Lit lit = Lit::toLit(i2);
        for(const Watched& w: solver->watches[lit]) {
            if (w.isBin() && !w.red() && lit < w.lit2()) {
                this_clause.clear();
                this_clause.push_back(lit);
                this_clause.push_back(w.lit2());

                if (add_this_clause(this_clause) == add_cl_ret::unsat) {
                    return false;
                }
            }
        }
    }
    for(ClOffset offs: solver->longIrredCls) {
        const Clause* cl = solver->cl_alloc.ptr(offs);
        assert(!cl->freed());
        assert(!cl->getRemoved());

        if (add_this_clause(*cl) == add_cl_ret::unsat) {
            return false;
        }
    }

    //Shrink the space if we have to
    assert(ls_s->_num_clauses >= (int)cl_num);
    ls_s->_num_clauses = (int)cl_num;
    ls_s->make_space();

    for (int c=0; c < ls_s->_num_clauses; c++) {
        for(CCNR::lit item: ls_s->_clauses[c].literals) {
            int v = item.var_num;
            ls_s->_vars[v].literals.push_back(item);
        }
    }
    ls_s->build_neighborhood();

    return true;
}

struct ClWeightSorter
{
    bool operator()(const CCNR::clause& a, const CCNR::clause& b)
    {
        return a.weight > b.weight;
    }
};

struct VarAndVal {
    VarAndVal(uint32_t _var, long long _score) :
        var(_var),
        val(_score)
    {
    }
    uint32_t var;
    long long val;
};

struct VarValSorter
{
    bool operator()(const VarAndVal& a, const VarAndVal& b) {
        return a.val > b.val;
    }
};

vector<pair<uint32_t, double>> CMS_ccnr::get_bump_based_on_cls()
{
    if (solver->conf.verbosity) {
        cout << "c [ccnr] bumping based on clause weights" << endl;
    }
    //Check prerequisites
    assert(toClear.empty());
    #ifdef SLOW_DEBUG
    for(const auto x: seen) {
        assert(x == 0);
    }
    #endif

    vector<pair<uint32_t, double>> tobump_cl_var;
    std::sort(ls_s->_clauses.begin(), ls_s->_clauses.end(), ClWeightSorter());
    uint32_t vars_bumped = 0;
    uint32_t individual_vars_bumped = 0;
    for(const auto& c: ls_s->_clauses) {
        if (vars_bumped > solver->conf.sls_how_many_to_bump)
            break;

        for(uint32_t i = 0; i < c.literals.size(); i++) {
            uint32_t v = c.literals[i].var_num-1;
            if (v < solver->nVars() &&
                solver->varData[v].removed == Removed::none &&
                solver->value(v) == l_Undef &&
                seen[v] < solver->conf.sls_bump_var_max_n_times
            ) {
                if (seen[v] == 0) {
                    individual_vars_bumped++;
                }
                seen[v]++;
                toClear.push_back(Lit(v, false));
                tobump_cl_var.push_back(std::make_pair(v, 3.0));
                vars_bumped++;
            }
        }
    }

    //Clear up
    for(const auto x: toClear) {
        seen[x.var()] = 0;
    }
    toClear.clear();

    return tobump_cl_var;
}

vector<pair<uint32_t, double>> CMS_ccnr::get_bump_based_on_var_scores()
{
    vector<VarAndVal> vs;
    for(uint32_t i = 1; i < ls_s->_vars.size(); i++) {
        vs.push_back(VarAndVal(i-1, ls_s->_vars[i].score));
    }
    std::sort(vs.begin(), vs.end(), VarValSorter());

    vector<pair<uint32_t, double>> tobump;
    for(uint32_t i = 0; i < solver->conf.sls_how_many_to_bump; i++) {
//         cout << "var: " << vs[i].var + 1 << " score: " <<  vs[i].val << endl;
        tobump.push_back(std::make_pair(vs[i].var, 3.0));
    }
    return tobump;
}

vector<pair<uint32_t, double>> CMS_ccnr::get_bump_based_on_conflict_ct()
{
    if (solver->conf.verbosity) {
        cout << "c [ccnr] bumping based on var unsat frequency: conflict_ct" << endl;
    }

    vector<pair<uint32_t, double>> tobump;
    int mymax = 0;
    for(uint32_t i = 1; i < ls_s->_conflict_ct.size(); i++) {
        mymax = std::max(mymax, ls_s->_conflict_ct[i]);
    }

    for(uint32_t i = 1; i < ls_s->_conflict_ct.size(); i++) {
        double val = ls_s->_conflict_ct[i];
        tobump.push_back(std::make_pair(i-1, (double)val/(double)mymax * 3.0));
//         if (tobump.back().second > 0) {
//             cout << "var: " << tobump.back().first << " bump by: " << tobump.back().second << endl;
//         }
    }
    return tobump;
}

lbool CMS_ccnr::deal_with_solution(int res, const uint32_t num_sls_called)
{
    if (solver->conf.sls_get_phase || res) {
        if (solver->conf.verbosity) {
            cout
            << "c [ccnr] saving best assignment phase"
            << endl;
        }

        for(size_t i = 0; i < solver->nVars(); i++) {
            solver->varData[i].polarity = ls_s->_best_solution[i+1];
            if (res) {
                solver->varData[i].best_polarity = solver->varData[i].polarity;
                solver->longest_trail_ever = solver->nVarsOuter();
            }
        }
    }

    //Clause score sorting
    vector<pair<uint32_t, double>> tobump;
    switch (solver->conf.sls_bump_type) {
        case 1:
            tobump = get_bump_based_on_cls();
            break;
        case 2:
            assert(false && "Does not work, removed");
            break;
        case 3:
            tobump = get_bump_based_on_var_scores();
            break;
        case 4:
            tobump = get_bump_based_on_conflict_ct();
            break;
        case 5:
            if (num_sls_called % 3 == 0) {
                tobump = get_bump_based_on_conflict_ct();
            } else {
                tobump = get_bump_based_on_cls();
            }
            break;
        case 6:
            if (num_sls_called % 3 == 0) {
                tobump = get_bump_based_on_cls();
            } else {
                tobump = get_bump_based_on_conflict_ct();
            }
            break;
        default:
            assert(false && "No such SLS bump type");
            exit(-1);
    }

    for(uint32_t i = 0; i < solver->nVars(); i++) {
        solver->var_act_vsids[i].offset = 1.0;
        solver->var_act_maple[i].offset = 1.0;
    }

    if (solver->conf.sls_set_offset == 1) {
        for(const auto& v: tobump) {
            solver->var_act_vsids[v.first].offset = 1.0+v.second*v.second;
            solver->var_act_maple[v.first].offset = 1.0+v.second*v.second;
        }
    } else if (solver->conf.sls_set_offset == 2) {
        for(const auto& v: tobump) {
            solver->var_act_vsids[v.first].offset = 1.0+v.second*v.second*30;
            solver->var_act_maple[v.first].offset = 1.0+v.second*v.second*30;
        }
    } else if (solver->conf.sls_set_offset == 3) {
        for(const auto& v: tobump) {
            solver->var_act_vsids[v.first].offset = 1.0+v.second;
            solver->var_act_maple[v.first].offset = 1.0+v.second;
        }
    } else if (solver->conf.sls_set_offset == 4) {
        for(const auto& v: tobump) {
            solver->var_act_vsids[v.first].offset = 1.0+v.second*30;
            solver->var_act_maple[v.first].offset = 1.0+v.second*30;
        }
    } else if (solver->conf.sls_set_offset == 5) {
        for(const auto& v: tobump) {
            solver->bump_var_importance_all(v.first, true, v.second*30);
        }

        if (solver->branch_strategy == branch::vsids) {
            solver->vsids_decay_var_act();
        }
    } else if (solver->conf.sls_set_offset == 0) {
        for(const auto& v: tobump) {
            solver->bump_var_importance_all(v.first, true, v.second/3.0);
        }

        if (solver->branch_strategy == branch::vsids) {
            solver->vsids_decay_var_act();
        }
    } else {
        assert(false && "No such sls_set_offset variabt");
    }

    if (solver->conf.verbosity) {
        cout << "c [ccnr] Bumped/set offset to vars: " << tobump.size()
        << " offset type: " << solver->conf.sls_set_offset
        << " bump type: " << solver->conf.sls_bump_type
        << endl;
    }
    if (!res) {
        if (solver->conf.verbosity >= 2) {
            cout << "c [ccnr] ASSIGNMENT NOT FOUND" << endl;
        }
    } else {
        if (solver->conf.verbosity) {
            cout << "c [ccnr] ASSIGNMENT FOUND" << endl;
        }
    }

    return l_Undef;
}
