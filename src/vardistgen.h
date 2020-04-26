/******************************************
Copyright (c) 2016, Mate Soos

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

#include <vector>
#include <cstdint>

#ifndef VARDISTGEN_H
#define VARDISTGEN_H

using std::vector;

namespace CMSat {

class Solver;
class Clause;

struct VarData2
{
    struct Dat {
        uint32_t num_times_in_bin_clause = 0;
        uint32_t num_times_in_long_clause = 0;
        uint32_t satisfies_cl = 0;
        uint32_t falsifies_cl = 0;
        uint32_t tot_num_lit_of_bin_it_appears_in =0;
        uint32_t tot_num_lit_of_long_cls_it_appears_in = 0;
        double sum_var_act_of_cls;
    };
    Dat irred;
    Dat red;
    double tot_act_long_red_cls;
};

class VarDistGen {
public:
    VarDistGen(Solver* solver);
    void calc();
    #ifdef STATS_NEEDED_BRANCH
    void dump();
    #endif

private:
    double compute_tot_act_vsids(Clause* cl) const;

    Solver* solver;
    vector<VarData2> data;

};

}

#endif
