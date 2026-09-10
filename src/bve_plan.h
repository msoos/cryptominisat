/******************************************
Copyright (C) 2026 Mate Soos

Cheap simulation of bounded variable elimination on a clause model, used to
pick the elimination-order heuristic before the real BVE runs.
***********************************************/

#pragma once

#include <cstdint>
#include <vector>
#include "solvertypes.h"

namespace CMSat {

struct BvePlanResult {
    uint32_t elimed = 0;
    uint32_t tried = 0;
    uint64_t cls = 0;
    uint64_t lits = 0;
    bool complete = true;
    double time = 0.0;
};

class BvePlanSim {
public:
    BvePlanSim(uint32_t nvars, const std::vector<std::vector<Lit>>& clauses,
               const std::vector<char>& can_elim, uint32_t max_grow, int max_resolvent,
               int64_t work_limit, int score_prod, int score_sum);
    BvePlanResult run(int heuristic, bool canon_ties);

    static int64_t score_of(int heuristic, uint32_t pos, uint32_t neg, uint64_t sum_pos, uint64_t sum_neg,
                            uint32_t degree, uint32_t fill, int score_prod, int score_sum);

private:
    struct VarInfo { uint32_t pos = 0, neg = 0; uint64_t sum_pos = 0, sum_neg = 0; uint32_t degree = 0, fill = 0; uint64_t hash = 0; };
    uint32_t nvars;
    std::vector<std::vector<Lit>> cls;
    std::vector<char> alive;
    std::vector<std::vector<uint32_t>> occ;
    std::vector<char> can_elim;
    std::vector<char> elimed;
    uint32_t max_grow;
    int max_resolvent;
    int64_t work;
    int score_prod, score_sum;
    std::vector<uint32_t> stamp, stamp2;
    uint32_t cur_stamp = 0;
    std::vector<uint32_t> nbs;

    VarInfo info(uint32_t v, int heuristic, bool canon_nb);
    int64_t score(uint32_t v, int heuristic, bool canon);
    bool try_elim(uint32_t v, uint32_t grow);
};

}
