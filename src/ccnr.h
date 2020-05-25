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

#ifndef CCNR_H
#define CCNR_H

#include <string>
#include <vector>
#include "ccnr_mersenne.h"

using std::vector;

namespace CCNR {

//--------------------------
//functions in basis.h & basis.cpp
struct lit {
    unsigned char sense : 1; //is 1 for true literals, 0 for false literals.
    int clause_num : 31;     //clause num, begin with 0
    int var_num;             //variable num, begin with 1
    lit(int the_lit, int the_clause)
    {
        var_num = abs(the_lit);
        clause_num = the_clause;
        sense = the_lit > 0 ? 1 : 0;
    }
    struct lit &operator^=(const struct lit &l)
    {
        sense ^= l.sense;
        clause_num ^= l.clause_num;
        var_num ^= l.var_num;
        return *this;
    }
    void reset(void)
    {
        sense = 0;
        clause_num = 0;
        var_num = 0;
    }
    bool operator==(const struct lit &l) const
    {
        return sense == l.sense && clause_num == l.clause_num && var_num == l.var_num;
    }
    bool operator!=(const struct lit &l) const
    {
        return !(*this == l);
    }
};
struct variable {
    vector<lit> literals;
    vector<int> neighbor_var_nums;
    long long score;
    long long last_flip_step;
    int unsat_appear; //how many unsat clauses it appears in
    bool cc_value;
    bool is_in_ccd_vars;
};
struct clause {
    vector<lit> literals;
    int sat_count; //no. of satisfied literals
    int sat_var;
    long long weight;
};

//---------------------------
//functions in mersenne.h & mersenne.cpp

class ls_solver
{
   public:
    ls_solver(const bool aspiration);
    bool parse_arguments(int argc, char **argv);
    bool build_instance(std::string inst);
    bool local_search(
        const vector<bool> *init_solution = 0
        , long long int _mems_limit = 100*1000*1000
    );
    void print_solution(bool need_verify = 0);
    void simple_print();
    int get_best_cost()
    {
        return _best_found_cost;
    }
    void set_verbosity(uint32_t verb);

    //formula
    vector<variable> _vars;
    vector<clause> _clauses;
    int _num_vars;
    int _num_clauses;

    //data structure used
    vector<int> _conflict_ct;
    vector<int> _unsat_clauses; // list of unsatisfied clauses
    vector<int> _index_in_unsat_clauses; // _index_in_unsat_clauses[var] tells where "var" is in _unsat_vars
    vector<int> _unsat_vars; // clauses are UNSAT due to these vars
    vector<int> _index_in_unsat_vars;
    vector<int> _ccd_vars;

    //solution information
    vector<uint8_t> _solution;
    vector<uint8_t> _best_solution;

    //functions for buiding data structure
    bool make_space();
    void build_neighborhood();
    int get_cost() { return _unsat_clauses.size(); }

    private:
    int _best_found_cost;
    long long _mems = 0;
    long long _step;
    long long _max_steps;
    int _max_tries;
    int _time_limit;

    //aiding data structure
    Mersenne _random_gen; //random generator
    int _random_seed;

    ///////////////////////////
    //algorithmic parameters
    ///////////////////////////
    int _aspiration_score;

    //clause weighting
    int _swt_threshold;
    float _swt_p; //w=w*p+ave_w*q
    float _swt_q;
    int _avg_clause_weight;
    //-------------------
    bool _aspiration;
    float _up_ratio; //control how much variables need to be delete and assigned by up

    //=================
    long long _delta_total_clause_weight;

    //main functions
    void initialize(const vector<bool> *init_solution = 0);
    void initialize_variable_datas();
    void clear_prev_data();
    int pick_var();
    void flip(int flipv);
    void update_cc_after_flip(int flipv);
    void update_clause_weights();
    void smooth_clause_weights();

    //funcitons for basic operations
    void sat_a_clause(int the_clause);
    void unsat_a_clause(int the_clause);

    //--------------------
    long long _end_step;
    uint32_t _verbosity = 0;

    long long up_times = 0;
    long long flip_numbers = 0;
    int verbosity; // 0 print sat/unsat & infomation; 1 print everything;
};

} // namespace CCNR

#endif
