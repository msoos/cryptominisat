/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file <soos.mate@gmail.com>

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

#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include "ccnr_oracle_pre.h"
#include "ccnr_oracle.h"
#include <iomanip>
#include "constants.h"
#include "oracle/utils.h"
#include "time_mem.h"
#include <iostream>
#include "solvertypes.h"

using namespace CMSat;
using std::setprecision;
using std::fixed;
using std::cout;
using std::endl;

CCNROraclePre::CCNROraclePre(Solver* solver) {
    ls = new OracleLS(solver);
}

CCNROraclePre::~CCNROraclePre() { delete ls; }

void CCNROraclePre::init(const vector<vector<sspp::Lit>>& cls, uint32_t num_vars,
        vector<int8_t>* _assump_map) {
    ls->assump_map = _assump_map;

    //It might not work well with few number of variables
    //rnovelty could also die/exit(-1), etc.
    ls->num_vars = num_vars;
    ls->num_cls = cls.size();
    ls->make_space();
    uint32_t cl_num = 0;
    for(auto& cl: cls) add_this_clause(cl, cl_num++);

    for (const auto& c: ls->cls) {
        for(auto& l: c.lits) {
            ls->vars[l.var_num].lits.push_back(l);
        }
    }
    ls->build_neighborhood();
    ls->initialize();
}

void CCNROraclePre::adjust_assumps(const vector<int>& assumps_changed) {
    ls->adjust_assumps(assumps_changed);
}


void CCNROraclePre::reinit() {
    ls->initialize();
}

bool CCNROraclePre::run(int64_t mems_limit) {
    /* double start_time = cpuTime(); */
    bool res = ls->local_search(mems_limit);
    /* double time_used = cpuTime()-start_time; */
    /* cout << "[ccnr] T: " << setprecision(2) << fixed << time_used << " res: " << res << endl; */
    return res;
}

const vector<int8_t>& CCNROraclePre::get_sol () const {
    return ls->get_sol();
}

void CCNROraclePre::add_this_clause(const vector<sspp::Lit>& cl, int cl_num) {
    uint32_t sz = 0;
    yals_lits.clear();
    for(const sspp::Lit& lit: cl) {
        int l = sspp::VarOf(lit);
        l *= sspp::IsNeg(lit) ? -1 : 1;
        yals_lits.push_back(l);
        sz++;
    }
    assert(sz > 0);

    for(auto& l: yals_lits) {
        ls->cls[cl_num].lits.push_back(Olit(l, cl_num));
    }
}
