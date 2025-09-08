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

#pragma once

#include <set>
#include <vector>
#include <limits>
#include "solvertypes.h"

using std::numeric_limits;
using std::set;

namespace CMSat {

class Solver;

class GetClauseQuery {
public:
    GetClauseQuery(Solver* solver);
    void start_getting_constraints(
           bool red,
           bool simplified = false,
           uint32_t max_len = std::numeric_limits<uint32_t>::max(),
           uint32_t max_glue = std::numeric_limits<uint32_t>::max());
    bool get_next_constraint(std::vector<Lit>& ret, bool& is_xor, bool& rhs);
    void end_getting_constraints();
    set<uint32_t> translate_sampl_set(
            const set<uint32_t>& sampl_set, bool also_removed);

private:
    Solver* solver;

    bool red = false;
    uint32_t max_len = numeric_limits<uint32_t>::max();
    uint32_t max_glue = numeric_limits<uint32_t>::max();
    uint32_t at = numeric_limits<uint32_t>::max();
    uint32_t at_lev[3];
    uint32_t varreplace_at = numeric_limits<uint32_t>::max();
    uint32_t units_at = numeric_limits<uint32_t>::max();
    uint32_t watched_at = numeric_limits<uint32_t>::max();
    uint32_t watched_at_sub = numeric_limits<uint32_t>::max();
    uint32_t comp_at = numeric_limits<uint32_t>::max();
    uint32_t comp_at_sum = numeric_limits<uint32_t>::max();
    uint32_t elimed_at = numeric_limits<uint32_t>::max();
    uint32_t elimed_at2 = numeric_limits<uint32_t>::max();
    uint32_t undef_at = numeric_limits<uint32_t>::max();
    uint32_t xor_at = numeric_limits<uint32_t>::max();
    bool simplified = false;

    bool all_vars_outside(const vector<Lit>& cl) const;
};
}
