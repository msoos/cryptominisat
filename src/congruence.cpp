/******************************************
Copyright (C) 2009-2026 Authors of CryptoMiniSat, see AUTHORS file

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

#include "congruence.h"
#include "solver.h"
#include "clauseallocator.h"
#include "varreplacer.h"
#include "time_mem.h"

#include <algorithm>
#include <unordered_map>

using namespace CMSat;
using std::vector;

namespace {
struct KeyHash {
    size_t operator()(const vector<uint32_t>& v) const {
        size_t h = v.size();
        for (const uint32_t x: v) h = h*1000003ULL ^ x;
        return h;
    }
};
}

Congruence::Congruence(Solver* _solver) : solver(_solver) {}

Lit Congruence::find(const Lit l)
{
    Lit r = Lit(l.var(), false);
    while (repr[r.var()] != Lit(r.var(), false)) r = repr[r.var()] ^ r.sign();
    repr[l.var()] = r;
    return r ^ l.sign();
}

void Congruence::unite(const Lit a, const Lit b)
{
    const Lit ra = find(a);
    const Lit rb = find(b);
    assert(ra.var() != rb.var());
    if (ra.var() < rb.var()) repr[rb.var()] = ra ^ rb.sign();
    else repr[ra.var()] = rb ^ ra.sign();
}

void Congruence::extract_ands()
{
    const uint32_t max_sz = solver->conf.congruence_and_max_size;
    vector<vector<ClOffset>> occs(solver->nVars()*2);
    for (const ClOffset offs: solver->long_irred_cls) {
        const Clause* cl = solver->cl_alloc.ptr(offs);
        if (cl->freed() || cl->get_removed() || cl->size() > max_sz) continue;
        bool assigned = false;
        for (const Lit l: *cl) if (solver->value(l) != l_Undef) assigned = true;
        if (assigned) continue;
        for (const Lit l: *cl) occs[l.toInt()].push_back(offs);
    }

    //g <-> AND(-l for l in C\g) needs C and (-g v -l) for every other l of C
    for (uint32_t i = 0; i < occs.size(); i++) {
        if (occs[i].empty()) continue;
        const Lit g = Lit::toLit(i);
        cur_stamp++;
        uint32_t num_bins = 0;
        for (const Watched& w: solver->watches[~g]) {
            if (!w.is_bin() || w.red()) continue;
            stamp[w.lit2().toInt()] = cur_stamp;
            num_bins++;
        }
        if (num_bins < 2) continue;
        for (const ClOffset offs: occs[i]) {
            const Clause* cl = solver->cl_alloc.ptr(offs);
            if (cl->size()-1 > num_bins) continue;
            bool ok = true;
            for (const Lit l: *cl) {
                if (l == g) continue;
                if (stamp[(~l).toInt()] != cur_stamp) { ok = false; break; }
            }
            if (!ok) continue;
            Gate gate;
            gate.out = g;
            gate.is_xor = false;
            for (const Lit l: *cl) if (l != g) gate.ins.push_back(~l);
            gates.push_back(std::move(gate));
            num_and++;
        }
    }
}

void Congruence::extract_xors()
{
    struct Ent {
        vector<uint32_t> vars;
        uint32_t mask;
        uint32_t par;
    };
    const uint32_t max_sz = solver->conf.congruence_xor_max_size;
    vector<Ent> ents;
    vector<Lit> lits;
    for (const ClOffset offs: solver->long_irred_cls) {
        const Clause* cl = solver->cl_alloc.ptr(offs);
        if (cl->freed() || cl->get_removed() || cl->size() > max_sz) continue;
        lits.assign(cl->begin(), cl->end());
        std::sort(lits.begin(), lits.end());
        bool bad = false;
        for (uint32_t k = 0; k < lits.size(); k++) {
            if (solver->value(lits[k]) != l_Undef) bad = true;
            if (k > 0 && lits[k].var() == lits[k-1].var()) bad = true;
        }
        if (bad) continue;
        Ent e;
        e.mask = 0;
        e.par = 0;
        for (uint32_t k = 0; k < lits.size(); k++) {
            e.vars.push_back(lits[k].var());
            if (lits[k].sign()) { e.mask |= 1U << k; e.par ^= 1; }
        }
        ents.push_back(std::move(e));
    }
    std::sort(ents.begin(), ents.end(), [](const Ent& a, const Ent& b) {
        if (a.vars != b.vars) return a.vars < b.vars;
        if (a.par != b.par) return a.par < b.par;
        return a.mask < b.mask;
    });

    //A clause is falsified by the assignment with parity = its number of
    //negations. All 2^(n-1) clauses of parity p exclude every assignment of
    //parity p, so the vars XOR to 1-p
    for (size_t i = 0; i < ents.size();) {
        size_t j = i;
        uint32_t distinct = 0;
        while (j < ents.size() && ents[j].vars == ents[i].vars && ents[j].par == ents[i].par) {
            if (j == i || ents[j].mask != ents[j-1].mask) distinct++;
            j++;
        }
        const uint32_t n = ents[i].vars.size();
        if (distinct == (1U << (n-1))) {
            const bool rhs = !ents[i].par;
            for (const uint32_t v: ents[i].vars) {
                Gate gate;
                gate.out = Lit(v, rhs);
                gate.is_xor = true;
                for (const uint32_t v2: ents[i].vars) if (v2 != v) gate.ins.push_back(Lit(v2, false));
                gates.push_back(std::move(gate));
            }
            num_xor++;
        }
        i = j;
    }
}

Congruence::Norm Congruence::normalize(const Gate& g, vector<uint32_t>& key, Lit& out, Lit& equiv)
{
    key.clear();
    out = g.out;
    if (solver->value(out) != l_Undef) return Norm::skip;
    for (const Lit l: g.ins) if (solver->value(l) != l_Undef) return Norm::skip;
    const uint32_t out_var = find(out).var();

    if (!g.is_xor) {
        vector<Lit> r;
        for (const Lit l: g.ins) r.push_back(find(l));
        std::sort(r.begin(), r.end());
        r.erase(std::unique(r.begin(), r.end()), r.end());
        for (uint32_t k = 1; k < r.size(); k++) if (r[k] == ~r[k-1]) return Norm::out_false;
        for (const Lit l: r) if (l.var() == out_var) return Norm::skip;
        if (r.size() == 1) { equiv = r[0]; return Norm::out_equiv; }
        key.push_back(0);
        for (const Lit l: r) key.push_back(l.toInt());
        return Norm::key;
    }

    bool flip = false;
    vector<uint32_t> vs;
    for (const Lit l: g.ins) {
        const Lit r = find(l);
        flip ^= r.sign();
        vs.push_back(r.var());
    }
    std::sort(vs.begin(), vs.end());
    uint32_t j = 0;
    for (uint32_t k = 0; k < vs.size(); k++) {
        if (j > 0 && vs[j-1] == vs[k]) j--; //x ^ x = 0
        else vs[j++] = vs[k];
    }
    vs.resize(j);
    out = out ^ flip;
    for (const uint32_t v: vs) if (v == out_var) return Norm::skip;
    if (vs.empty()) return Norm::out_false;
    if (vs.size() == 1) { equiv = Lit(vs[0], false); return Norm::out_equiv; }
    key.push_back(1);
    for (const uint32_t v: vs) key.push_back(v);
    return Norm::key;
}

//Makes 'cl' derivable by unit propagation, adding temporary clauses split on
//the vars of 'split' where propagation alone does not conflict (XOR gates)
bool Congruence::prove(const vector<Lit>& cl, vector<uint32_t> split)
{
    vector<int32_t> hints;
    if (solver->prop_hints_for_cl(cl, hints)) return true;
    while (!split.empty()) {
        const uint32_t v = split.back();
        split.pop_back();
        bool in_cl = false;
        for (const Lit l: cl) if (l.var() == v) in_cl = true;
        if (in_cl || solver->value(v) != l_Undef) continue;

        for (const bool sign: {false, true}) {
            vector<Lit> c = cl;
            c.push_back(Lit(v, sign));
            if (!prove(c, split)) return false;
            if (!solver->prop_hints_for_cl(c, hints)) return false;
            vector<Lit> fin;
            Clause* x = solver->add_clause_int(c, false, nullptr, true, &fin, true,
                lit_Undef, false, false, &hints);
            if (!solver->okay()) return false;
            if (x) tmps.push_back({solver->cl_alloc.get_offset(x), lit_Undef, lit_Undef, 0, false});
            else if (fin.size() == 2) tmps.push_back({0, fin[0], fin[1], solver->clause_id, true});
        }
        return solver->prop_hints_for_cl(cl, hints);
    }
    return false;
}

void Congruence::delete_tmps()
{
    for (auto it = tmps.rbegin(); it != tmps.rend(); ++it) {
        if (it->is_bin) {
            solver->detach_bin_clause(it->a, it->b, false, it->id);
            *solver->frat << del << it->id << it->a << it->b << fin;
        } else {
            solver->detach_clause(it->offs, true);
            solver->free_cl(it->offs);
        }
    }
    tmps.clear();
}

bool Congruence::add_implied(const vector<Lit>& cl, const vector<uint32_t>& split)
{
    vector<int32_t> hints;
    if (solver->frat->enabled()) {
        if (!prove(cl, split) || !solver->prop_hints_for_cl(cl, hints)) {
            if (!solver->okay()) { tmps.clear(); return false; }
            delete_tmps();
            num_failed++;
            return false;
        }
    }
    solver->add_clause_int(cl, true, nullptr, true, nullptr, true,
        lit_Undef, false, false, solver->frat->enabled() ? &hints : nullptr);
    if (solver->okay()) delete_tmps();
    else tmps.clear();
    return true;
}

bool Congruence::equate(const Lit a, const Lit b, const vector<uint32_t>& split)
{
    if (find(a) == find(b)) return true;
    if (!add_implied({~a, b}, split) || !solver->okay()) return false;
    if (!add_implied({a, ~b}, split) || !solver->okay()) return false;
    if (find(a) == ~find(b)) return true; //formula is UNSAT, clauses above made it so
    unite(a, b);
    num_merged++;
    return true;
}

bool Congruence::run()
{
    assert(solver->okay());
    assert(solver->decision_level() == 0);
    if (!solver->bnns.empty() || !solver->gmatrices.empty()) return true;
    const double my_time = cpu_time();

    gates.clear();
    repr.resize(solver->nVars());
    for (uint32_t v = 0; v < solver->nVars(); v++) repr[v] = Lit(v, false);
    stamp.assign(solver->nVars()*2, 0);
    cur_stamp = 0;
    num_and = num_xor = num_merged = num_units = num_failed = 0;
    extract_ands();
    extract_xors();

    vector<uint32_t> key;
    vector<uint32_t> split;
    bool changed = true;
    uint32_t passes = 0;
    while (changed && solver->okay() && passes < 1000) {
        passes++;
        changed = false;
        std::unordered_map<vector<uint32_t>, std::pair<Lit, uint32_t>, KeyHash> table;
        for (uint32_t i = 0; i < gates.size() && solver->okay(); i++) {
            Lit out, equiv;
            const Norm n = normalize(gates[i], key, out, equiv);
            if (n == Norm::skip) continue;
            split.clear();
            for (const Lit l: gates[i].ins) split.push_back(l.var());
            if (n == Norm::out_false) {
                const Lit u = ~out;
                if (solver->value(u) == l_True) continue;
                if (!add_implied({u}, split)) continue;
                num_units++;
                changed = true;
                continue;
            }
            if (n == Norm::out_equiv) {
                if (find(out) == find(equiv)) continue;
                if (equate(out, equiv, split)) changed = true;
                continue;
            }
            auto it = table.find(key);
            if (it == table.end()) {
                table[key] = {out, i};
                continue;
            }
            const Lit other = it->second.first;
            if (find(other) == find(out)) continue;
            for (const Lit l: gates[it->second.second].ins) split.push_back(l.var());
            if (equate(other, out, split)) changed = true;
        }
    }

    const double time_used = cpu_time() - my_time;
    verb_print(1, "[congruence] gates and: " << num_and << " xor: " << num_xor
        << " merged: " << num_merged << " units: " << num_units
        << " failed: " << num_failed << " passes: " << passes
        << solver->conf.print_times(time_used));
    gates.clear();
    gates.shrink_to_fit();

    if (solver->okay() && num_merged > 0) solver->var_replacer->replace_if_enough_is_found();
    return solver->okay();
}
