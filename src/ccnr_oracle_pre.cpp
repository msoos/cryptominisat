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

#include <cstdio>
#include <cmath>
#include <cstdlib>
#include "ccnr_oracle_pre.h"
#include "ccnr_oracle.h"
#include <iomanip>
#include "constants.h"
#include "time_mem.h"
#include <iostream>
#include "solvertypes.h"

using namespace CMSat;
using std::setprecision;
using std::fixed;
using std::cout;
using std::endl;

CCNROraclePre::CCNROraclePre(uint32_t _verb) {
    ls_s = new OracleLS();
    ls_s->set_verbosity(1);
}

CCNROraclePre::~CCNROraclePre() { delete ls_s; }

void CCNROraclePre::init(const vector<vector<Lit>>& cls, uint32_t _num_vars) {
    num_vars = _num_vars;
    num_clauses = cls.size();

    //It might not work well with few number of variables
    //rnovelty could also die/exit(-1), etc.
    if (num_vars == 0 || cls.size() == 0) {
        release_assert(false);
        return;
    }

    ls_s->num_vars = num_vars;
    ls_s->num_cls = cls.size();
    ls_s->assump_map.clear();
    ls_s->assump_map.resize(num_vars, 0);
    ls_s->make_space();
    for(auto& cl: cls) add_this_clause(cl);

    for (int c=0; c < ls_s->num_cls; c++) {
        for(auto& l: ls_s->cls[c].lits) {
            ls_s->vars[l.var_num].lits.push_back(l);
        }
    }
    ls_s->build_neighborhood();
}

void CCNROraclePre::run(vector<uint8_t>& assump_map, vector<uint8_t>& lit_set, vector<uint8_t>& lit_unsat) {
    double start_time = cpuTime();
    int res = ls_s->local_search(30LL*1000LL, "c o");
    double time_used = cpuTime()-start_time;
    cout << "[ccnr] T: " << setprecision(2) << fixed << time_used << " res: " << res << endl;
}

template<class T>
void CCNROraclePre::add_this_clause(const T& cl) {
    uint32_t sz = 0;
    yals_lits.clear();
    for(auto& lit: cl) {
        int l = lit.var()+1;
        l *= lit.sign() ? -1 : 1;
        yals_lits.push_back(l);
        sz++;
    }
    assert(sz > 0);

    for(auto& lit: yals_lits) {
        ls_s->cls[cl_num].lits.push_back(Olit(lit, cl_num));
    }
    cl_num++;
}
