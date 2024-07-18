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
#ifdef INSTALLED_CADIBACK
#include <cadiback.h>
#else
#include "../cadiback/cadiback.h"
#endif

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
