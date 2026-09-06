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
#include "occsimplifier.h"
#include "solvertypesmini.h"
#include <cstdint>
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

// The irredundant clauses in cadiback's format: each clause 0-terminated.
static void build_cadiback_cnf(Solver* s, vector<int>& cnf, uint64_t& num_lits) {
    cnf.clear();
    num_lits = 0;
    for(auto const& off: s->longIrredCls) {
        Clause* cl = s->cl_alloc.ptr(off);
        for(auto const& l1: *cl) {
            num_lits++;
            cnf.push_back(s->PICOLIT(l1));
        }
        cnf.push_back(0);
    }
    for(uint32_t i = 0; i < s->nVars()*2; i++) {
        Lit l1 = Lit::toLit(i);
        for(auto const& w: s->watches[l1]) {
            if (!w.isBin() || w.red()) continue;
            const Lit l2 = w.lit2();
            if (l1 > l2) continue;

            num_lits+=2;
            cnf.push_back(s->PICOLIT(l1));
            cnf.push_back(s->PICOLIT(l2));
            cnf.push_back(0);
        }
    }
    for(uint32_t i = 0; i < s->nVars(); i++) {
        if (s->value(i) == l_Undef) continue;
        cnf.push_back(s->PICOLIT(Lit(i, s->value(i) == l_False)));
        cnf.push_back(0);
    }
}

static vector<vector<sspp::Lit>> build_ccnr_cls(Solver* s) {
    // A local search solver only sees the clauses we hand it. An XOR would be
    // invisible to it
    assert(s->xorclauses.empty());
    assert(s->gmatrices.empty());

    vector<vector<sspp::Lit>> cls;
    vector<sspp::Lit> tmp;
    for(auto const& off: s->longIrredCls) {
        tmp.clear();
        Clause* cl = s->cl_alloc.ptr(off);
        for(auto const& l1: *cl) tmp.push_back(orclit(l1));
        cls.push_back(tmp);
    }
    for(uint32_t i = 0; i < s->nVars()*2; i++) {
        Lit l1 = Lit::toLit(i);
        for(auto const& w: s->watches[l1]) {
            if (!w.isBin() || w.red()) continue;
            const Lit l2 = w.lit2();
            if (l1 > l2) continue;
            cls.push_back({orclit(l1), orclit(l2)});
        }
    }
    for(uint32_t i = 0; i < s->nVars(); i++) {
        if (s->value(i) == l_Undef) continue;
        cls.push_back({orclit(Lit(i, s->value(i) == l_False))});
    }
    return cls;
}

// Vars that differ between local search models cannot be backbone, so cadiback
// need not test them. ccnr's neighborhood is quadratic in clause size (one
// 25k-literal clause is 2.4GB of it), so everything is freed before we return.
static vector<int> ccnr_drop_cands(Solver* solver, uint64_t& num_cls) {
    vector<vector<sspp::Lit>> cls = build_ccnr_cls(solver);
    num_cls = cls.size();
    vector<int8_t> assump_map(solver->nVars()+1, 2);
    CCNROraclePre ccnr(solver);
    ccnr.init(cls, solver->nVars(), &assump_map);
    cls.clear(); // ccnr copied them
    cls.shrink_to_fit();

    vector<int> sols_found(solver->nVars()+1, -1);
    uint32_t ccnr_sols_found = 0;
    double ccnr_time = cpu_time();
    for(uint32_t nsols = 0; nsols < 10; nsols++) {
        ccnr.reinit();
        bool ret = ccnr.run(solver->conf.backbone_ccnr_mems_limitM*1000LL*1000LL);
        verb_print(3, "[backbone-ccnr] sol found: " << ret);
        if (!ret) continue;
        ccnr_sols_found++;
        const auto& sol = ccnr.get_sol();
        for(uint32_t v = 1; v <= solver->nVars(); v++) {
            if (sols_found[v] == -1) {
                sols_found[v] = sol[v];
                continue;
            }
            if (sols_found[v] == sol[v]) continue;
            sols_found[v] = 2;
        }
    }

    vector<int> drop_cands; // off by one, as in cadiback (i.e. vars start with 1)
    for(uint32_t i = 1; i <= solver->nVars(); i++) {
        if (sols_found[i] == 2) drop_cands.push_back(i);
    }
    verb_print(1, "[backbone-simpl] ccnr sols: " << ccnr_sols_found << " drop_cands: " << drop_cands.size()
            << " T: " << std::fixed << std::setprecision(2)
            << cpu_time()-ccnr_time);
    return drop_cands;
}

static bool add_backbone_units(Solver* solver, const vector<int>& learned_units) {
    vector<Lit> tmp;
    for(const auto& l: learned_units) {
        if (l == 0) continue;
        const Lit lit = Lit(abs(l)-1, l < 0);
        if (solver->value(lit.var()) != l_Undef) continue;
        if (solver->varData[lit.var()].removed != Removed::none) continue;
        tmp.clear();
        tmp.push_back(lit);
        solver->add_clause_int(tmp);
        if (!solver->okay()) return false;
    }
    return true;
}

static uint32_t add_backbone_eq_lits(Solver* solver, const vector<pair<int, int>>& eqLits) {
    uint32_t num_eq_added = 0;
    vector<Lit> tmp;
    for(const auto& p: eqLits) {
        const Lit lit1 = Lit(abs(p.first)-1, p.first < 0);
        const Lit lit2 = Lit(abs(p.second)-1, p.second < 0);
        tmp = {~lit1, lit2};
        auto ret = solver->add_clause_int(tmp, true);
        assert(ret == nullptr);
        tmp = {lit1, ~lit2};
        ret = solver->add_clause_int(tmp, true);
        assert(ret == nullptr);
        verb_print(4, "[backbone-simpl] added eq clause: " << lit1 << " = " << lit2);
        num_eq_added++;
    }
    return num_eq_added;
}

// Stops early if a clause makes us UNSAT, so the caller must re-check okay().
static uint32_t add_backbone_bins(Solver* solver, const vector<int>& learned_bins) {
    uint32_t num_bins_added = 0;
    vector<Lit> tmp;
    bool ignore = false;
    for(const auto& l: learned_bins) {
        if (l == 0) {
            if (!ignore) {
                assert(tmp.size() == 2);
                auto ret = solver->add_clause_int(tmp, true);
                assert(ret == nullptr);
                num_bins_added++;
                if (!solver->okay()) return num_bins_added;
            }
            ignore = false;
            tmp.clear();
            continue;
        }
        const Lit lit = Lit(abs(l)-1, l < 0);
        if (solver->varData[lit.var()].removed != Removed::none) {ignore = true; continue;}
        if (solver->value(lit.var()) != l_Undef) {ignore = true; continue;}
        tmp.push_back(lit);
    }
    return num_bins_added;
}

bool Solver::backbone_simpl(int64_t orig_max_confl, bool /*cmsgen*/,
        bool& backbone_done)
{
    if (!okay()) return okay();
    if (nVars() == 0) return okay();
    double my_time = cpu_time();
    print_simp_stats_before("backbone-simpl");

    vector<int> cnf;
    uint64_t num_lits = 0;
    build_cadiback_cnf(this, cnf, num_lits);

    uint64_t num_cls = 0;
    vector<int> drop_cands = ccnr_drop_cands(this, num_cls);

    vector<int> learned_units;
    vector<int> learned_bins;
    verb_print(1, "[backbone-simpl] cadiback called with -- lits: " << num_lits
            << " num cls: " << num_cls << " num vars: " << nVars());
    vector<pair<int, int>> eqLits;
    bool backbone_limit_hit = false;
    int res = CadiBack::doit(cnf, std::max(0, conf.verbosity-1), drop_cands, learned_units, learned_bins, eqLits,
        orig_max_confl, &backbone_limit_hit);
    uint32_t num_units = trail_size();
    uint32_t num_bins_added = 0;
    uint32_t num_eq_added = 0;
    if (res == 10) {
        if (add_backbone_units(this, learned_units)) {
            num_eq_added = add_backbone_eq_lits(this, eqLits);
            num_bins_added = add_backbone_bins(this, learned_bins);
            if (okay() && !backbone_limit_hit) backbone_done = true;
        }
    } else if (res != 0) {
        // res == 20 means UNSAT, res == 0 means limit hit (not an error)
        ok = false;
    }
    verb_print(1, "[backbone-simpl] res: " << res
            <<  " num units added: " << trail_size() - num_units
            <<  " num eq added: " << num_eq_added
            <<  " num bins: " << num_bins_added
            << " T: " << std::fixed << std::setprecision(2)
            << cpu_time() - my_time);
    print_simp_stats_after("backbone-simpl");
    return okay();
}

size_t Solver::num_long_irred_cls_anywhere() const
{
    size_t n = longIrredCls.size();
    if (occsimplifier != nullptr) n += occsimplifier->num_long_irred_linked_in();
    return n;
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
