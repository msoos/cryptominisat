/******************************************
Copyright (C) 2026 Authors of CryptoMiniSat, see AUTHORS file

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

#include "sweeper.h"

#include <algorithm>
#include <limits>

#include "kitten.h"
#include "solver.h"
#include "occsimplifier.h"
#include "clauseallocator.h"
#include "sqlstats.h"
#include "time_mem.h"

using namespace CMSat;

Sweeper::Sweeper(OccSimplifier* _occs, Solver* _solver) :
    occs(_occs)
    , solver(_solver)
{}

Sweeper::~Sweeper()
{
    assert(kit == nullptr);
}

Lit Sweeper::repr(Lit l)
{
    Lit res = l;
    while (reprs[res.toInt()] != res) res = reprs[res.toInt()];
    if (res == l) return res;

    Lit prev = l;
    for (;;) {
        const Lit next = reprs[prev.toInt()];
        if (next == res) break;
        reprs[(~prev).toInt()] = ~res;
        reprs[prev.toInt()] = res;
        prev = next;
    }
    return res;
}

void Sweeper::add_lit_to_env(uint32_t depth, Lit l)
{
    if (repr(l) != l) return;
    const uint32_t v = l.var();
    if (depths[v]) return;
    depths[v] = depth + 1;
    env_vars.push_back(v);
}

bool Sweeper::env_add_long(uint32_t depth, ClOffset off)
{
    Clause& cl = *solver->cl_alloc.ptr(off);
    if (cl.stats.marked_clause) return true;
    if (cl.red() || cl.freed() || cl.get_removed()) return true;
    *occs->limit_to_decrease -= 1 + (int64_t)cl.size()/4;

    kit_cl_tmp.clear();
    for (const Lit l : cl) {
        const lbool val = solver->value(l);
        if (val == l_True) return true;
        if (val == l_False) { unit_at_collect[l.var()] = 1; continue; }
        kit_cl_tmp.push_back(l.toInt());
    }
    cl.stats.marked_clause = 1;
    env_marked.push_back(off);
    env_cls.emplace_back();
    env_cls.back().id = cl.stats.id;
    env_cls.back().lits.assign(cl.begin(), cl.end());
    kitten_clause_with_id_and_exception(
        kit, env_cls.size()-1, kit_cl_tmp.size(), kit_cl_tmp.data(),
        std::numeric_limits<unsigned>::max());
    env_encoded++;
    for (const unsigned ul : kit_cl_tmp) add_lit_to_env(depth, Lit::toLit(ul));
    return true;
}

void Sweeper::env_add_bin(uint32_t depth, Lit l1, Lit l2, int32_t id)
{
    //NOTE: red bins are swept too, like in CaDiCaL
    if (solver->value(l1) == l_True || solver->value(l2) == l_True) return;
    assert(solver->value(l1) == l_Undef && solver->value(l2) == l_Undef);
    if (!env_bin_ids.insert(id).second) return;

    env_cls.emplace_back();
    env_cls.back().id = id;
    env_cls.back().lits = {l1, l2};
    unsigned k[2] = {(unsigned)l1.toInt(), (unsigned)l2.toInt()};
    kitten_clause_with_id_and_exception(
        kit, env_cls.size()-1, 2, k, std::numeric_limits<unsigned>::max());
    env_encoded++;
    add_lit_to_env(depth, l1);
    add_lit_to_env(depth, l2);
}

void Sweeper::collect_environment(uint32_t v)
{
    add_lit_to_env(0, Lit(v, false));
    bool limit_reached = false;
    size_t expand = 0;
    size_t next = 1;
    uint32_t depth = 1;
    while (!limit_reached) {
        if (env_encoded >= limit.clauses) break;
        if (expand == next) {
            if (depth >= limit.depth) break;
            next = env_vars.size();
            if (expand == next) break;
            depth++;
        }
        const uint32_t idx = env_vars[expand];
        for (uint32_t sign = 0; sign < 2 && !limit_reached; sign++) {
            const Lit l(idx, sign == 1);
            watch_subarray_const ws = solver->watches[l];
            *occs->limit_to_decrease -= 1 + (int64_t)ws.size()/2;
            for (const Watched& w : ws) {
                if (w.isBin()) env_add_bin(depth, l, w.lit2(), w.get_id());
                else if (w.isClause()) env_add_long(depth, w.get_offset());
                if (env_vars.size() >= limit.vars) {
                    limit_reached = true;
                    break;
                }
            }
        }
        expand++;
    }
}

void Sweeper::charge_kitten_ticks()
{
    *occs->limit_to_decrease -= (int64_t)kitten_current_ticks(kit);
}

bool Sweeper::ticks_out() const
{
    if (solver->must_interrupt_asap()) return true;
    return (*occs->limit_to_decrease - (int64_t)kitten_current_ticks(kit)) <= 0;
}

void Sweeper::clear_env()
{
    charge_kitten_ticks();
    for (const ClOffset off : env_marked) {
        Clause& cl = *solver->cl_alloc.ptr(off);
        assert(cl.stats.marked_clause);
        cl.stats.marked_clause = 0;
    }
    env_marked.clear();
    env_bin_ids.clear();
    for (const uint32_t v : env_vars) depths[v] = 0;
    env_vars.clear();
    env_cls.clear();
    backbone.clear();
    partition.clear();
    env_encoded = 0;
}

int Sweeper::solve()
{
    solves++;
    kitten_randomize_phases(kit);
    return kitten_solve(kit);
}

bool Sweeper::flip(Lit l)
{
    flips++;
    return kitten_flip_literal(kit, l.toInt());
}

extern "C" {
static void sweep_core_cb_plain(
    void* state, unsigned id, bool learned, size_t sz, const unsigned* lits)
{
    ((Sweeper*)state)->core_cb_plain(id, learned, sz, lits);
}

static void sweep_core_cb_chain(
    void* state, unsigned cid, unsigned id, bool learned,
    size_t sz, const unsigned* lits, size_t chsz, const unsigned* chain)
{
    ((Sweeper*)state)->core_cb_chain(cid, id, learned, sz, lits, chsz, chain);
}
}

void Sweeper::core_cb_plain(
    unsigned id, bool learned, size_t sz, const unsigned* lits)
{
    if (!solver->okay()) return;
    core[core_save].emplace_back();
    ProofCl& pc = core[core_save].back();
    pc.learned = learned;
    if (!learned) {
        assert(id < env_cls.size());
        pc.sweep_id = id;
        pc.cms_id = env_cls[id].id;
    }
    for (size_t i = 0; i < sz; i++) pc.lits.push_back(Lit::toLit(lits[i]));
}

void Sweeper::core_cb_chain(
    unsigned cid, unsigned id, bool learned,
    size_t sz, const unsigned* lits, size_t chsz, const unsigned* chain)
{
    if (!solver->okay()) return;
    core[core_save].emplace_back();
    ProofCl& pc = core[core_save].back();
    pc.kit_id = cid;
    pc.learned = learned;
    if (!learned) {
        assert(!chsz);
        assert(id < env_cls.size());
        pc.sweep_id = id;
        pc.cms_id = env_cls[id].id;
        for (size_t i = 0; i < sz; i++) pc.lits.push_back(Lit::toLit(lits[i]));
    } else {
        assert(chsz);
        for (size_t i = 0; i < sz; i++) pc.lits.push_back(Lit::toLit(lits[i]));
        //kitten hands the chain in reverse resolution order
        for (const unsigned* p = chain + chsz; p != chain; p--)
            pc.chain.push_back(*(p-1));
    }
}

void Sweeper::save_core(uint32_t which)
{
    core_save = which;
    assert(core[which].empty());
    kitten_compute_clausal_core(kit, nullptr);
    if (fr) kitten_trace_core(kit, this, sweep_core_cb_chain);
    else kitten_traverse_core_clauses_with_id(kit, this, sweep_core_cb_plain);
}

void Sweeper::add_core(uint32_t which)
{
    if (!solver->okay()) return;
    auto& cr = core[which];
    for (auto& pc : cr) {
        if (!pc.learned) continue;

        tmp_hints.clear();
        if (fr) {
            for (const unsigned cid : pc.chain) {
                int32_t chain_id = 0;
                for (const auto& cpc : cr) {
                    if (cpc.kit_id != cid) continue;
                    if (!cpc.learned) {
                        //units that falsified literals of the original clause
                        //at collection time must come before the clause itself
                        for (const Lit l : env_cls[cpc.sweep_id].lits) {
                            if (solver->value(l) != l_False) continue;
                            if (occs->seen[l.toInt()]) continue;
                            if (!unit_at_collect[l.var()]) continue;
                            occs->seen[l.toInt()] = 1;
                            occs->toClear.push_back(l);
                            assert(solver->unit_cl_IDs[l.var()] != 0);
                            tmp_hints.push_back(solver->unit_cl_IDs[l.var()]);
                        }
                    }
                    chain_id = cpc.cms_id;
                    break;
                }
                assert(chain_id != 0);
                tmp_hints.push_back(chain_id);
            }
            for (const Lit l : occs->toClear) occs->seen[l.toInt()] = 0;
            occs->toClear.clear();
        }

        const size_t nsz = pc.lits.size();
        if (nsz == 0) {
            const int32_t id = ++solver->clauseID;
            *solver->frat << add << id;
            if (fr && !tmp_hints.empty()) *solver->frat << fratchain << tmp_hints;
            *solver->frat << fin;
            set_unsat_cl_id(id);
            solver->ok = false;
            return;
        }

        if (nsz == 1) {
            const Lit u = pc.lits[0];
            if (solver->value(u) == l_True) {
                if (fr) pc.cms_id = solver->unit_cl_IDs[u.var()];
            } else if (solver->value(u) == l_False) {
                const int32_t id = ++solver->clauseID;
                if (fr) {
                    assert(solver->unit_cl_IDs[u.var()] != 0);
                    tmp_hints.push_back(solver->unit_cl_IDs[u.var()]);
                }
                *solver->frat << add << id;
                if (fr) *solver->frat << fratchain << tmp_hints;
                *solver->frat << fin;
                set_unsat_cl_id(id);
                solver->ok = false;
                return;
            } else {
                const int32_t id = ++solver->clauseID;
                if (fr) {
                    *solver->frat << add << id << u;
                    if (!tmp_hints.empty()) *solver->frat << fratchain << tmp_hints;
                    *solver->frat << fin;
                    solver->enqueue_registered_unit<false>(u, id);
                } else {
                    solver->enqueue<false>(u);
                }
                pc.cms_id = id;
                found_units++;
            }
            continue;
        }

        //larger lemma: needed in the proof only, deleted in clear_core
        if (fr) {
            const int32_t id = ++solver->clauseID;
            *solver->frat << add << id << pc.lits;
            if (!tmp_hints.empty()) *solver->frat << fratchain << tmp_hints;
            *solver->frat << fin;
            pc.cms_id = id;
        }
    }
}

void Sweeper::clear_core(uint32_t which)
{
    auto& cr = core[which];
    if (fr && solver->okay()) {
        for (const auto& pc : cr) {
            if (pc.learned && pc.lits.size() > 1 && pc.cms_id != 0)
                *solver->frat << del << pc.cms_id << pc.lits << fin;
        }
    }
    cr.clear();
}

void Sweeper::save_add_clear_core()
{
    save_core(0);
    add_core(0);
    clear_core(0);
}

void Sweeper::init_backbone_and_partition()
{
    backbone.clear();
    partition.clear();
    for (const uint32_t idx : env_vars) {
        if (solver->value(idx) != l_Undef) continue;
        if (solver->varData[idx].removed != Removed::none) continue;
        const Lit pos(idx, false);
        const signed char tmp = kitten_value(kit, pos.toInt());
        const Lit cand = (tmp < 0) ? ~pos : pos;
        backbone.push_back(cand);
        partition.push_back(cand);
    }
    partition.push_back(lit_Undef);
}

void Sweeper::refine_backbone()
{
    size_t q = 0;
    for (size_t p = 0; p < backbone.size(); p++) {
        const Lit l = backbone[p];
        if (solver->value(l) != l_Undef) continue;
        const signed char v = kitten_value(kit, l.toInt());
        if (v > 0) backbone[q++] = l;
    }
    backbone.resize(q);
}

void Sweeper::refine_partition()
{
    std::vector<Lit> newp;
    size_t i = 0;
    const size_t end = partition.size();
    while (i < end) {
        size_t j = i;
        while (partition[j] != lit_Undef) j++;

        uint32_t n_true = 0;
        for (size_t k = i; k < j; k++) {
            const Lit o = partition[k];
            if (repr(o) != o) continue;
            if (solver->value(o) != l_Undef) continue;
            if (kitten_value(kit, o.toInt()) > 0) { newp.push_back(o); n_true++; }
        }
        if (n_true == 1) newp.pop_back();
        else if (n_true > 1) newp.push_back(lit_Undef);

        uint32_t n_false = 0;
        for (size_t k = i; k < j; k++) {
            const Lit o = partition[k];
            if (repr(o) != o) continue;
            if (solver->value(o) != l_Undef) continue;
            if (kitten_value(kit, o.toInt()) < 0) { newp.push_back(o); n_false++; }
        }
        if (n_false == 1) newp.pop_back();
        else if (n_false > 1) newp.push_back(lit_Undef);

        i = j + 1;
    }
    partition.swap(newp);
}

void Sweeper::refine()
{
    assert(kitten_status(kit) == 10);
    if (!backbone.empty()) refine_backbone();
    if (!partition.empty()) refine_partition();
}

void Sweeper::flip_backbone_literals()
{
    const uint32_t max_rounds = solver->conf.sweep_flip_rounds;
    if (!max_rounds) return;
    assert(!backbone.empty());
    if (kitten_status(kit) != 10) return;

    uint32_t round = 0;
    uint32_t flipped;
    do {
        round++;
        flipped = 0;
        size_t q = 0;
        bool limit_hit = false;
        for (size_t p = 0; p < backbone.size(); p++) {
            const Lit l = backbone[p];
            if (limit_hit || ticks_out()) {
                limit_hit = true;
                backbone[q++] = l;
                continue;
            }
            if (flip(l)) flipped++;
            else backbone[q++] = l;
        }
        backbone.resize(q);
        if (limit_hit || ticks_out()) break;
    } while (flipped && round < max_rounds);
}

void Sweeper::flip_partition_literals()
{
    const uint32_t max_rounds = solver->conf.sweep_flip_rounds;
    if (!max_rounds) return;
    assert(!partition.empty());
    if (kitten_status(kit) != 10) return;

    std::vector<Lit> newp;
    uint32_t round = 0;
    uint32_t flipped;
    do {
        round++;
        flipped = 0;
        newp.clear();
        bool limit_hit = false;
        size_t i = 0;
        while (i < partition.size()) {
            size_t j = i;
            while (partition[j] != lit_Undef) j++;
            const size_t cls_start = newp.size();
            size_t size = j - i;
            assert(size > 1);
            for (size_t k = i; k < j; k++) {
                const Lit l = partition[k];
                if (limit_hit || ticks_out()) {
                    limit_hit = true;
                    newp.push_back(l);
                    continue;
                }
                if (flip(l)) {
                    flipped++;
                    if (--size < 2) break; //rest of class is a singleton
                } else {
                    newp.push_back(l);
                }
            }
            if (newp.size() - cls_start > 1) newp.push_back(lit_Undef);
            else newp.resize(cls_start);
            i = j + 1;
        }
        partition.swap(newp);
        if (limit_hit || ticks_out()) break;
    } while (flipped && round < max_rounds);
}

bool Sweeper::extract_fixed(Lit l)
{
    kitten_assume(kit, (~l).toInt());
    const int res = solve();
    if (!res) return false;
    assert(res == 20);
    save_add_clear_core();
    return true;
}

bool Sweeper::backbone_candidate(Lit l)
{
    const signed char v = kitten_fixed(kit, l.toInt());
    if (v) {
        assert(v > 0);
        if (solver->value(l) != l_True) return extract_fixed(l);
        return false;
    }

    int res = kitten_status(kit);
    if (res == 10 && flip(l)) return false;

    kitten_assume(kit, (~l).toInt());
    res = solve();
    if (res == 10) {
        refine();
        return false;
    }
    if (res == 20) {
        save_add_clear_core();
        return true;
    }
    return false;
}

bool Sweeper::add_equiv_bin(const ProofCl* pc, Lit a, Lit b)
{
    if (!solver->okay()) return false;
    if (solver->value(a) != l_Undef || solver->value(b) != l_Undef) return false;

    tmp_hints.clear();
    if (fr) {
        assert(pc != nullptr);
        for (const Lit pl : pc->lits) {
            if (solver->value(pl) == l_Undef) continue;
            assert(solver->value(pl) == l_False);
            assert(solver->unit_cl_IDs[pl.var()] != 0);
            tmp_hints.push_back(solver->unit_cl_IDs[pl.var()]);
        }
        assert(pc->cms_id != 0);
        tmp_hints.push_back(pc->cms_id);
    }
    tmp_bin.clear();
    tmp_bin.push_back(a);
    tmp_bin.push_back(b);
    occs->full_add_clause(tmp_bin, tmp_fin, nullptr, false, fr ? &tmp_hints : nullptr);
    return solver->okay();
}

void Sweeper::partition_squash_or_drop_last(Lit lit, Lit other, bool drop_first)
{
    const size_t sz = partition.size();
    assert(sz >= 3);
    assert(partition[sz-3] == lit);
    assert(partition[sz-2] == other);
    const bool two_only = (sz == 3) || (partition[sz-4] == lit_Undef);
    if (two_only) {
        partition.resize(sz-3);
        return;
    }
    if (drop_first) partition[sz-3] = other;
    partition[sz-2] = lit_Undef;
    partition.resize(sz-1);
}

void Sweeper::partition_remove(Lit l)
{
    size_t p = 0;
    while (partition[p] != l) {
        p++;
        assert(p < partition.size());
    }
    size_t begin_class = p;
    while (begin_class != 0 && partition[begin_class-1] != lit_Undef) begin_class--;
    size_t end_class = p;
    while (partition[end_class] != lit_Undef) end_class++;

    const size_t size = end_class - begin_class;
    assert(size > 1);
    size_t q = begin_class;
    if (size == 2) {
        //the remaining literal is a singleton, squash the whole class
        for (size_t r = end_class + 1; r < partition.size(); r++)
            partition[q++] = partition[r];
    } else {
        for (size_t r = begin_class; r < partition.size(); r++)
            if (r != p) partition[q++] = partition[r];
    }
    partition.resize(q);
}

bool Sweeper::equivalence_candidates(Lit lit, Lit other)
{
    int res = kitten_status(kit);
    if (res == 10) {
        if (flip(lit)) {
            partition_squash_or_drop_last(lit, other, true);
            return false;
        }
        if (flip(other)) {
            partition_squash_or_drop_last(lit, other, false);
            return false;
        }
    }

    kitten_assume(kit, (~lit).toInt());
    kitten_assume(kit, other.toInt());
    res = solve();
    if (res == 10) refine();
    if (res != 20) return false;
    save_core(0);

    kitten_assume(kit, lit.toInt());
    kitten_assume(kit, (~other).toInt());
    res = solve();
    if (res == 10) refine();
    if (res != 20) {
        core[0].clear();
        return false;
    }
    save_core(1);

    //core[0] proves (lit | ~other), core[1] proves (~lit | other)
    add_core(0);
    add_core(1);
    if (!solver->okay()) {
        core[0].clear();
        core[1].clear();
        return false;
    }

    if (solver->value(lit) == l_Undef && solver->value(other) == l_Undef) {
        //kitten_trace_core replays lemmas in derivation order, so with FRAT on
        //the last learned lemma of core[i] is the proven binary itself
        const ProofCl* pc0 = nullptr;
        const ProofCl* pc1 = nullptr;
        if (fr) {
            assert(!core[0].empty() && core[0].back().learned);
            assert(!core[1].empty() && core[1].back().learned);
            pc0 = &core[0].back();
            pc1 = &core[1].back();
        }
        bool ok = add_equiv_bin(pc0, lit, ~other);
        if (ok) ok = add_equiv_bin(pc1, ~lit, other);
        if (ok) found_equivs++;
    }
    clear_core(0);
    clear_core(1);
    if (!solver->okay()) return false;

    //smaller variable index becomes the representative
    if (lit.var() < other.var()) {
        reprs[other.toInt()] = lit;
        reprs[(~other).toInt()] = ~lit;
        partition_remove(other);
    } else {
        reprs[lit.toInt()] = other;
        reprs[(~lit).toInt()] = ~other;
        partition_remove(lit);
    }
    return true;
}

void Sweeper::sweep_variable(uint32_t v)
{
    if (solver->value(v) != l_Undef) return;
    if (solver->varData[v].removed != Removed::none) return;
    const Lit start(v, false);
    if (repr(start) != start) return;
    swept_vars++;

    kitten_clear(kit);
    kitten_track_antecedents(kit);
    assert(env_vars.empty() && env_cls.empty() && env_marked.empty());
    collect_environment(v);

    if (env_vars.size() <= 1) {
        clear_env();
        return;
    }

    const int64_t rem = *occs->limit_to_decrease;
    kitten_set_ticks_limit(kit, rem < 0 ? 0 : (uint64_t)rem);
    int res = solve();
    if (res == 10) {
        init_backbone_and_partition();
        while (!backbone.empty()) {
            if (!solver->okay() || ticks_out()) break;
            flip_backbone_literals();
            if (ticks_out()) break;
            if (backbone.empty()) break;
            const Lit l = backbone.back();
            backbone.pop_back();
            if (solver->value(l) != l_Undef) continue;
            backbone_candidate(l);
        }
        while (solver->okay() && !partition.empty()) {
            if (ticks_out()) break;
            flip_partition_literals();
            if (ticks_out()) break;
            if (partition.empty()) break;
            if (partition.size() > 2) {
                const Lit lit = partition[partition.size()-3];
                const Lit other = partition[partition.size()-2];
                equivalence_candidates(lit, other);
            } else {
                partition.clear();
            }
        }
    } else if (res == 20) {
        //environment alone is UNSAT: replaying its core derives the empty clause
        save_add_clear_core();
        assert(!solver->okay());
    }

    clear_env();
    if (solver->okay())
        solver->ok = solver->propagate_occur<false>(occs->limit_to_decrease);
}

bool Sweeper::sweep()
{
    assert(solver->okay());
    assert(solver->prop_at_head());
    assert(solver->decisionLevel() == 0);
    frat_func_start();
    fr = solver->frat->enabled();
    swept_vars = found_units = found_equivs = 0;
    solves = flips = 0;
    const double my_time = cpu_time();
    const int64_t start_budget = *occs->limit_to_decrease;
    const SolverConf& conf = solver->conf;

    const auto calc_limits = [&]() {
        const uint32_t sh = std::min<uint32_t>(epoch, 20);
        limit.vars = std::min<uint64_t>((uint64_t)conf.sweep_vars << sh, conf.sweep_max_vars);
        limit.clauses = std::min<uint64_t>((uint64_t)conf.sweep_clauses << sh, conf.sweep_max_clauses);
        limit.depth = std::min<uint32_t>(conf.sweep_depth + epoch, conf.sweep_max_depth);
    };
    calc_limits();

    const uint32_t nvars = solver->nVars();
    reprs.clear();
    reprs.reserve(2*nvars);
    for (uint32_t i = 0; i < 2*nvars; i++) reprs.push_back(Lit::toLit(i));
    depths.assign(nvars, 0);
    unit_at_collect.assign(nvars, 0);
    for (uint32_t v = 0; v < nvars; v++)
        if (solver->value(v) != l_Undef) unit_at_collect[v] = 1;
    if (done_outer.size() < solver->nVarsOuter()) done_outer.resize(solver->nVarsOuter(), 0);

    //schedule: fewest occurrences first
    struct Cand {
        uint64_t rank;
        uint32_t v;
        bool operator<(const Cand& o) const {
            if (rank != o.rank) return rank < o.rank;
            return v < o.v;
        }
    };
    std::vector<Cand> cands;
    const auto collect_cands = [&]() {
        cands.clear();
        for (uint32_t v = 0; v < nvars; v++) {
            if (solver->value(v) != l_Undef) continue;
            if (solver->varData[v].removed != Removed::none) continue;
            const uint64_t pos = occs->n_occurs[Lit(v, false).toInt()];
            const uint64_t neg = occs->n_occurs[Lit(v, true).toInt()];
            if (!pos || !neg) continue;
            if (pos > limit.clauses || neg > limit.clauses) continue;
            if (done_outer[solver->map_inter_to_outer(v)]) continue;
            cands.push_back(Cand{pos+neg, v});
        }
    };
    collect_cands();
    if (cands.empty()) {
        //everything was swept at this size: grow limits, start a new epoch
        epoch++;
        std::fill(done_outer.begin(), done_outer.end(), 0);
        calc_limits();
        collect_cands();
    }
    std::sort(cands.begin(), cands.end());

    assert(kit == nullptr);
    kit = kitten_init();
    for (const Cand& c : cands) {
        if (!solver->okay()) break;
        if (*occs->limit_to_decrease <= 0 || solver->must_interrupt_asap()) break;
        done_outer[solver->map_inter_to_outer(c.v)] = 1;
        sweep_variable(c.v);
    }
    kitten_release(kit);
    kit = nullptr;

    const bool time_out = (*occs->limit_to_decrease <= 0);
    const double time_used = cpu_time() - my_time;
    const double time_remain = float_div(*occs->limit_to_decrease, start_budget);
    verb_print(1, "[occ-sweep]"
        << " swept: " << swept_vars << "/" << cands.size()
        << " units: " << found_units
        << " equivs: " << found_equivs
        << " epoch: " << epoch
        << " solves: " << solves
        << " flips: " << flips
        << solver->conf.print_times(time_used, time_out, time_remain));
    if (solver->sqlStats) {
        solver->sqlStats->time_passed(
            solver, "occ-sweep", time_used, time_out, time_remain);
    }
    frat_func_end();
    return solver->okay();
}
