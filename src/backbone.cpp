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

#include "ccnr_oracle_pre.h"
#include "constants.h"
#include "solver.h"
#include "solvertypesmini.h"
#include <cstdint>
#include "cryptominisat.h"
#include "varreplacer.h"
#include "cadiback.h"

using namespace CMSat;

inline int orclit(const Lit x) {
    return (x.sign() ? ((x.var()+1)*2+1) : (x.var()+1)*2);
}

inline Lit orc_to_lit(int x) {
    uint32_t var = x/2-1;
    bool neg = x&1;
    return Lit(var, neg);
}

bool Solver::backbone_simpl(int64_t /*orig_max_confl*/, bool /*cmsgen*/,
        bool& backbone_done)
{
    if (!okay()) return okay();
    if (nVars() == 0) return okay();
    double my_time = cpu_time();

    vector<int> cnf;
    /* for(uint32_t i = 0; i < nVars(); i++) picosat_inc_max_var(picosat); */
    CCNROraclePre ccnr(this);
    vector<vector<sspp::Lit>> cls;
    vector<sspp::Lit> cctmp;
    uint64_t num_lits = 0;
    for(auto const& off: longIrredCls) {
        cctmp.clear();
        Clause* cl = cl_alloc.ptr(off);
        for(auto const& l1: *cl) {
            num_lits++;
            cnf.push_back(PICOLIT(l1));
            cctmp.push_back(orclit(l1));
        }
        cnf.push_back(0);
        cls.push_back(cctmp);
    }
    for(uint32_t i = 0; i < nVars()*2; i++) {
        Lit l1 = Lit::toLit(i);
        for(auto const& w: watches[l1]) {
            if (!w.isBin() || w.red()) continue;
            const Lit l2 = w.lit2();
            if (l1 > l2) continue;

            num_lits+=2;
            cnf.push_back(PICOLIT(l1));
            cnf.push_back(PICOLIT(l2));
            cnf.push_back(0);

            cctmp.clear();
            cctmp.push_back(orclit(l1));
            cctmp.push_back(orclit(l2));
            cls.push_back(cctmp);
        }
    }
    for(uint32_t i = 0; i < nVars(); i++) {
        if (value(i) == l_Undef) continue;
        auto l = Lit(i, value(i) == l_False);
        cnf.push_back(PICOLIT(l));
        cnf.push_back(0);
        cls.push_back({orclit(l)});
    }
    uint64_t num_cls = cls.size();

    vector<int8_t> assump_map(nVars()+1, 2);
    ccnr.init(cls, nVars(), &assump_map);
    vector<int> sols_found(nVars()+1, -1);
    uint32_t ccnr_sols_found = 0;
    double ccnr_time = cpu_time();
    for(uint32_t nsols = 0; nsols < 10; nsols++) {
        ccnr.reinit();
        bool ret = ccnr.run(30LL*1000LL*1000LL);
        verb_print(3, "[backbone-ccnr] sol found: " << ret);
        if (!ret) continue;
        ccnr_sols_found++;
        const auto& sol = ccnr.get_sol();
        for(uint32_t v = 1; v <= nVars(); v++) {
            if (sols_found[v] == -1) {
                sols_found[v] = sol[v];
                continue;
            }
            if (sols_found[v] == sol[v]) continue;
            sols_found[v] = 2;
        }
    }
    cls.clear();

    vector<int> drop_cands; // off by one, as in cadiback (i.e. vars start with 1)
    for(uint32_t i = 1; i <= nVars(); i++) {
        if (sols_found[i] == 2) drop_cands.push_back(i);
    }
    verb_print(1, "[backbone-simpl] ccnr sols: " << ccnr_sols_found << " drop_cands: " << drop_cands.size()
            << " T: " << std::fixed << std::setprecision(2)
            << cpu_time()-ccnr_time);

    vector<int> learned_units;
    vector<int> learned_bins;
    verb_print(1, "[backbone-simpl] cadiback called with -- lits: " << num_lits
            << " num cls: " << num_cls << " num vars: " << nVars());
    vector<pair<int, int>> eqLits;
    int res = CadiBack::doit(cnf, std::max(0, conf.verbosity-1), drop_cands, learned_units, learned_bins, eqLits);
    uint32_t num_units = trail_size();
    uint32_t num_bins_added = 0;
    uint32_t num_eq_added = 0;
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
            if (!okay()) goto end;
        }

        for(const auto& p: eqLits) {
            int l1 = p.first;
            int l2 = p.second;
            const Lit lit1 = Lit(abs(l1)-1, l1 < 0);
            const Lit lit2 = Lit(abs(l2)-1, l2 < 0);
            tmp = {~lit1, lit2};
            auto ret = add_clause_int(tmp, true);
            assert(ret == nullptr);
            tmp = {lit1, ~lit2};
            ret = add_clause_int(tmp, true);
            assert(ret == nullptr);
            verb_print(4, "[backbone-simpl] added eq clause: " << lit1 << " = " << lit2);
            num_eq_added++;
        }

        tmp.clear();
        bool ignore = false;
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
            <<  " num units added: " << trail_size() - num_units
            <<  " num eq added: " << num_eq_added
            <<  " num bins: " << num_bins_added
            << " T: " << std::fixed << std::setprecision(2)
            << cpu_time() - my_time);
    return okay();
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
