/******************************************
Copyright (C) 2026 Mate Soos
***********************************************/

#include "bve_plan.h"
#include "time_mem.h"
#include <algorithm>
#include <queue>

using namespace CMSat;
using std::vector;

BvePlanSim::BvePlanSim(uint32_t _nvars, const vector<vector<Lit>>& clauses, const vector<char>& _can_elim,
                       uint32_t _max_grow, int _max_resolvent, int64_t work_limit, int _score_prod, int _score_sum)
    : nvars(_nvars), cls(clauses), can_elim(_can_elim), max_grow(_max_grow), max_resolvent(_max_resolvent),
      work(work_limit), score_prod(_score_prod), score_sum(_score_sum)
{
    alive.assign(cls.size(), 1);
    occ.assign(2 * nvars, {});
    for (uint32_t i = 0; i < cls.size(); i++) for (const Lit l : cls[i]) occ[l.toInt()].push_back(i);
    elimed.assign(nvars, 0);
    stamp.assign(nvars, 0);
    stamp2.assign(nvars, 0);
}

int64_t BvePlanSim::score_of(int h, uint32_t pos, uint32_t neg, uint64_t sum_pos, uint64_t sum_neg,
                             uint32_t degree, uint32_t fill, int score_prod, int score_sum) {
    if (pos == 0) return -(int64_t)neg - (1LL << 30);
    if (neg == 0) return -(int64_t)pos - (1LL << 30);
    const int64_t prod = (int64_t)pos * neg;
    const int64_t growth = (int64_t)neg * sum_pos + (int64_t)pos * sum_neg - 2 * prod - (int64_t)sum_pos - (int64_t)sum_neg;
    switch (h) {
        case 1: return growth * 8 + prod;
        case 2: return (prod > (int64_t)pos + neg ? (1LL << 40) : 0) + growth * 8 + prod;
        case 3: return (int64_t)degree * (1LL << 20) + prod;
        case 4: return (int64_t)fill * (1LL << 20) + prod;
        default: return score_prod * prod + score_sum * (int64_t)(pos + neg);
    }
}

BvePlanSim::VarInfo BvePlanSim::info(uint32_t v, int h, bool canon_nb) {
    VarInfo vi;
    const Lit p(v, false);
    cur_stamp++;
    nbs.clear();
    for (int sgn = 0; sgn < 2; sgn++) {
        const Lit l = sgn ? ~p : p;
        for (const uint32_t ci : occ[l.toInt()]) {
            if (!alive[ci]) continue;
            const auto& c = cls[ci];
            work -= (int64_t)c.size();
            if (sgn) { vi.neg++; vi.sum_neg += c.size(); } else { vi.pos++; vi.sum_pos += c.size(); }
            if (h >= 3 || canon_nb) for (const Lit x : c) {
                if (x.var() == v || stamp[x.var()] == cur_stamp) continue;
                stamp[x.var()] = cur_stamp;
                nbs.push_back(x.var());
            }
        }
    }
    vi.degree = nbs.size();
    if (h == 4) {
        if (nbs.size() > 32) vi.fill = nbs.size() * nbs.size();
        else {
            const uint32_t nb_mark = cur_stamp;
            uint64_t missing = 0;
            for (const uint32_t u : nbs) {
                cur_stamp++;
                uint32_t adj = 0;
                for (int sgn = 0; sgn < 2 && adj < nbs.size(); sgn++) {
                    const auto& ol = occ[Lit(u, sgn).toInt()];
                    if (ol.size() > 400) { adj = nbs.size(); break; }
                    for (const uint32_t ci : ol) {
                        if (!alive[ci]) continue;
                        work -= (int64_t)cls[ci].size();
                        for (const Lit x : cls[ci]) {
                            const uint32_t w = x.var();
                            if (w == u || w == v || stamp[w] != nb_mark || stamp2[w] == cur_stamp) continue;
                            stamp2[w] = cur_stamp;
                            adj++;
                        }
                    }
                }
                missing += nbs.size() - 1 - std::min<uint32_t>(adj, nbs.size() - 1);
            }
            vi.fill = missing / 2;
        }
    }
    uint64_t hsh = 0x9e3779b97f4a7c15ULL * (vi.pos + 1) + 0xff51afd7ed558ccdULL * (vi.neg + 1) + vi.sum_pos * 31 + vi.sum_neg * 131;
    if (canon_nb)
        for (const uint32_t u : nbs) {
            uint64_t a = 0, b = 0;
            for (const uint32_t ci : occ[Lit(u, false).toInt()]) a += alive[ci];
            for (const uint32_t ci : occ[Lit(u, true).toInt()]) b += alive[ci];
            work -= (int64_t)(occ[Lit(u, false).toInt()].size() + occ[Lit(u, true).toInt()].size());
            hsh += (a * 0x9e3779b97f4a7c15ULL) ^ (b * 0xc2b2ae3d27d4eb4fULL) ^ ((a + b) << 17);
        }
    vi.hash = hsh;
    return vi;
}

int64_t BvePlanSim::score(uint32_t v, int h, bool canon) {
    const VarInfo vi = info(v, h, canon);
    const int64_t primary = score_of(h, vi.pos, vi.neg, vi.sum_pos, vi.sum_neg, vi.degree, vi.fill, score_prod, score_sum);
    return primary * (1LL << 22) + (canon ? (int64_t)(vi.hash & ((1ULL << 22) - 1)) : 0);
}

bool BvePlanSim::try_elim(uint32_t v, uint32_t grow) {
    const Lit p(v, false);
    vector<uint32_t> ps, ns;
    for (const uint32_t ci : occ[p.toInt()]) if (alive[ci]) ps.push_back(ci);
    for (const uint32_t ci : occ[(~p).toInt()]) if (alive[ci]) ns.push_back(ci);
    if (ps.empty() || ns.empty()) {
        for (const uint32_t ci : ps) alive[ci] = 0;
        for (const uint32_t ci : ns) alive[ci] = 0;
        return true;
    }
    const uint64_t limit = ps.size() + ns.size() + grow;
    if ((uint64_t)ps.size() * ns.size() > 4 * limit + 64) return false;
    vector<vector<Lit>> res;
    for (const uint32_t a : ps) {
        cur_stamp++;
        for (const Lit x : cls[a]) if (x.var() != v) stamp[x.var()] = cur_stamp, stamp2[x.var()] = x.sign();
        for (const uint32_t b : ns) {
            work -= (int64_t)(cls[a].size() + cls[b].size());
            bool taut = false;
            vector<Lit> r;
            for (const Lit x : cls[a]) if (x.var() != v) r.push_back(x);
            for (const Lit x : cls[b]) {
                if (x.var() == v) continue;
                if (stamp[x.var()] == cur_stamp) { if (stamp2[x.var()] != (uint32_t)x.sign()) { taut = true; break; } continue; }
                r.push_back(x);
            }
            if (taut) continue;
            if (max_resolvent >= 0 && (int)r.size() > max_resolvent) return false;
            res.push_back(std::move(r));
            if (res.size() > limit) return false;
        }
    }
    for (const uint32_t ci : ps) alive[ci] = 0;
    for (const uint32_t ci : ns) alive[ci] = 0;
    for (auto& r : res) {
        const uint32_t id = cls.size();
        for (const Lit l : r) occ[l.toInt()].push_back(id);
        cls.push_back(std::move(r));
        alive.push_back(1);
    }
    return true;
}

BvePlanResult BvePlanSim::run(int h, bool canon) {
    const double t0 = cpu_time();
    BvePlanResult ret;
    using Item = std::pair<int64_t, uint32_t>;
    vector<uint32_t> grows{0};
    for (uint32_t g = 1; g <= max_grow; g *= 2) grows.push_back(g);
    if (max_grow > 0 && grows.back() != max_grow) grows.push_back(max_grow);
    for (const uint32_t grow : grows) {
        std::priority_queue<Item, vector<Item>, std::greater<Item>> heap;
        for (uint32_t v = 0; v < nvars; v++) {
            if (!can_elim[v] || elimed[v]) continue;
            if (occ[Lit(v, false).toInt()].empty() && occ[Lit(v, true).toInt()].empty()) continue;
            heap.push({score(v, h, canon), v});
        }
        while (!heap.empty() && work > 0) {
            const Item it = heap.top(); heap.pop();
            const uint32_t v = it.second;
            if (elimed[v]) continue;
            const int64_t sc = score(v, h, canon);
            if (sc > it.first) { heap.push({sc, v}); continue; }
            ret.tried++;
            if (try_elim(v, grow)) { elimed[v] = 1; ret.elimed++; }
        }
        if (work <= 0) { ret.complete = false; break; }
    }
    for (uint32_t i = 0; i < cls.size(); i++) if (alive[i]) { ret.cls++; ret.lits += cls[i].size(); }
    ret.time = cpu_time() - t0;
    return ret;
}
