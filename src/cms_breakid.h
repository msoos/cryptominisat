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

#ifndef CMS_BREAKID_H
#define CMS_BREAKID_H

#include <vector>
#include <unordered_map>
#include "solvertypes.h"
#include "cloffset.h"

using std::vector;
using std::unordered_map;

namespace BID {
class BreakID;
}

namespace CMSat {

class Solver;

class BreakID {
public:
    BreakID(Solver* solver);
    bool doit();
    void finished_solving();
    void start_new_solving();
    void updateVars(
    const vector<uint32_t>& outerToInter
    , const vector<uint32_t>& interToOuter);
    void update_var_after_varreplace();
    Lit get_assumed_lit() const;

    static uint32_t hash_clause(const Lit* lits, const uint32_t size) {
        uint32_t seed = size;
        for(uint32_t i = 0; i < size; i++) {
            uint32_t val = lits[i].toInt();
            seed ^= val + 0x9e3779b9 + (val << 6) + (val >> 2);
        }
        return seed;
    }

private:
    void break_symms_in_cms();
    void get_outer_permutations();
    void remove_duplicates();
    void set_up_time_lim();
    bool add_clauses();
    bool check_limits();

    enum class add_cl_ret {added_cl, skipped_cl, unsat};
    template<class T>
    add_cl_ret add_this_clause(const T& cl);
    vector<Lit> brkid_lits;

    ///Valid permutations. Contains outer lits
    vector<unordered_map<Lit, Lit> > perms_outer;

    bool already_called = false;
    //variable that is to be assumed to break symmetries
    uint32_t symm_var = var_Undef;
    int64_t set_time_lim;
    uint64_t num_lits_in_graph;
    vector<ClOffset> dedup_cls;

    Solver* solver;
    BID::BreakID* breakid = NULL;
};

}

#endif
