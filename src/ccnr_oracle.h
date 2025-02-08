/******************************************
Copyright (c) 2019, Shaowei Cai

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
#include <cstdlib>
#include <ostream>
#include <vector>
#include "ccnr_mersenne.h"
#include "ccnr_oracle_pre.h"

using std::vector;
using std::abs;
using std::ostream;

namespace CMSat {
class Solver;

struct Olit {
    int var_num;             //variable num, begin with 1
    uint32_t cl_num : 31;         //clause ID it belongs to, begin with 0
    uint32_t sense : 1;           //is 1 for true literals, 0 for false literals.
    Olit(int the_lit, int the_clause) {
        var_num = std::abs(the_lit);
        cl_num = the_clause;
        sense = the_lit > 0 ? 1 : 0;
    }

    void reset(void) {
        sense = 0;
        cl_num = 0;
        var_num = 0;
    }

    bool operator==(const struct Olit &l) const {
        return sense == l.sense && cl_num == l.cl_num && var_num == l.var_num;
    }

    bool operator!=(const struct Olit &l) const {
        return !(*this == l);
    }
};

inline ostream& operator<<(ostream& os, const Olit& l) {
    os << (l.sense ? "" : "-") << l.var_num;
    return os;
}

struct Ovariable {
    vector<Olit> lits;
    vector<int> neighbor_vars;
    long long score;
    long long last_flip_step;
    int unsat_appear; //how many unsat clauses it appears in
                      //
    bool cc_value;
    bool is_in_ccd_vars;
};

struct Oclause {
    vector<Olit> lits;
    int sat_count; //no. of satisfied literals
    int sat_var; // the variable that makes the clause satisfied
    long long weight;
};

struct Oconf {
    int verb;
    string prefix;
};

class OracleLS {
 public:
    OracleLS(Solver* _solver);
    bool local_search(int64_t mems_limit);
    void print_solution();
    void check_solution();

    //formula
    vector<Ovariable> vars;
    vector<Oclause> cls;
    int num_vars;
    int num_cls;

    //data structure used
    vector<int> unsat_cls; // list of unsatisfied clauses
    vector<int> idx_in_unsat_cls; // idx_in_unsat_cls[cl_id] tells where cl_id is in unsat_vars
                                  //
    vector<int> unsat_vars; // clauses are UNSAT due to these vars
    vector<int> idx_in_unsat_vars;
    vector<int8_t>* assump_map = nullptr; // always num_vars+1 size, if 2, it's a variable to flip, otherwise 1/0 for fixed vars
    vector<int8_t> sol; //solution information. 0 = false, 1 = true, 3 = unset
    vector<int> ccd_vars;
    long long delta_tot_cl_weight;

    //functions for building data structure
    bool make_space();
    void build_neighborhood();
    int get_cost() { return unsat_cls.size(); }
    void initialize();
    void adjust_assumps(const vector<int>& assumps_changed);

  private:
    Solver* solver;
    CCNR::Mersenne random_gen;
    Oconf conf;

    // Config
    long long max_steps;
    int64_t max_tries;
    float swt_p = 0.3;
    float swt_q = 0.7;
    int swt_thresh = 50;

    // internal stats
    int64_t step;
    int64_t mems = 0;
    int avg_cl_weight;

    //main functions
    void initialize_variable_datas();
    int pick_var();
    void flip(int flipv);
    void update_clause_weights();
    void smooth_clause_weights();
    void update_cc_after_flip(int flipv);

    //funcitons for basic operations
    void sat_a_clause(int the_clause);
    void unsat_a_clause(int the_clause);

    // debug
    void print_cl(int cl_id);
    void check_clause(int cid);
};

}
