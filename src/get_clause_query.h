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


#ifndef GET_CLAUSE_QUERY_H
#define GET_CLAUSE_QUERY_H

#include "constants.h"
#include <vector>
#include <string>
#include <algorithm>

#include "constants.h"
#include "solvertypes.h"

namespace CMSat {

class Solver;

class GetClauseQuery {
public:
    GetClauseQuery(Solver* solver);
    void start_getting_small_clauses(
            uint32_t max_len, uint32_t max_glue, bool red = true,
            bool bva_vars = false, bool simplified = false);
    bool get_next_small_clause(std::vector<Lit>& out, bool all_in_one_go = false);
    void end_getting_small_clauses();
    void get_all_irred_clauses(vector<Lit>& out);
    vector<uint32_t> translate_sampl_set(const vector<uint32_t>& sampl_set);

private:
    Solver* solver;

    bool red = true;
    uint32_t max_len = numeric_limits<uint32_t>::max();
    uint32_t max_glue = numeric_limits<uint32_t>::max();
    uint32_t at = numeric_limits<uint32_t>::max();
    uint32_t varreplace_at = numeric_limits<uint32_t>::max();
    uint32_t units_at = numeric_limits<uint32_t>::max();
    uint32_t watched_at = numeric_limits<uint32_t>::max();
    uint32_t watched_at_sub = numeric_limits<uint32_t>::max();
    uint32_t comp_at = numeric_limits<uint32_t>::max();
    uint32_t comp_at_sum = numeric_limits<uint32_t>::max();
    uint32_t blocked_at = numeric_limits<uint32_t>::max();
    uint32_t blocked_at2 = numeric_limits<uint32_t>::max();
    uint32_t xor_detached_at = numeric_limits<uint32_t>::max();
    uint32_t undef_at = numeric_limits<uint32_t>::max();
    bool simplified = false;
    bool bva_vars = false;

    vector<uint32_t> outer_to_without_bva_map;
    bool all_vars_outside(const vector<Lit>& cl) const;
    void map_without_bva(vector<Lit>& cl);
    vector<Lit> tmp_cl;
};


}

#endif

