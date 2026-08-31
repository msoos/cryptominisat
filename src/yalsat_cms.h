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

#include <cstdint>
#include "solvertypes.h"
#include "xor.h"

struct Yals;

namespace CMSat {

class Solver;

// Binds xnfSAT (a YalSAT derivative that handles XOR constraints natively) to
// the solver's irredundant CNF and XOR clauses.
class CMS_yalsat {
public:
    CMS_yalsat(Solver* _solver);
    ~CMS_yalsat();

    // False if the formula is UNSAT under the assumptions.
    bool init_problem();

    // Initial value to start the search from. Must be called after init_problem().
    void set_phase(const uint32_t var, const bool val);

    // Runs one round, returns the number of clauses left unsatisfied by the
    // best assignment found.
    int64_t run(const int64_t mems, const uint64_t seed);

    // Only vars that made it into the handed-over formula have a value.
    bool has_value(const uint32_t var) const { return occurred[var]; }
    bool value(const uint32_t var) const;

    uint32_t get_num_cls() const { return cl_num; }

    // Independently counts the constraints the best assignment falsifies, split
    // by kind. Cross-checks yalsat's own count, and hence the XOR encoding.
    int64_t count_unsat(int64_t& cnf, int64_t& xr) const;

private:
    enum class add_cl_ret {added_cl, skipped_cl, unsat};
    template<class T> add_cl_ret add_this_clause(const T& cl);
    add_cl_ret add_this_xor(const Xor& x);

    Solver* solver;
    Yals* yals = nullptr;
    uint32_t cl_num = 0;
    vector<char> occurred;
    vector<int> lits;
};

}
