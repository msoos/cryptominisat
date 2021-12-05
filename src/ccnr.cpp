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

#include "ccnr.h"
#include "ccnr_mersenne.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <cassert>

using namespace CCNR;

using std::cout;
using std::endl;
using std::string;


//constructor with default setting.
ls_solver::ls_solver(const bool aspiration)
{
    _max_tries = 100;
    _max_steps = 1*1000 * 1000;
    _random_seed = 1;
    _time_limit = 3000;
    _swt_threshold = 50;
    _swt_p = 0.3;
    _swt_q = 0.7;
    _aspiration = aspiration;
    _up_ratio = 0.3; //delete _up_ratio percents varibles
    verbosity = 0;
}

/**********************************build instance*******************************/
bool ls_solver::make_space()
{
    if (0 == _num_vars || 0 == _num_clauses) {
        cout << "c [ccnr] The formula size is zero."
        "You may have forgotten to read the formula." << endl;
        return false;
    }
    _vars.resize(_num_vars+1);
    _clauses.resize(_num_clauses+1);
    _solution.resize(_num_vars+1);
    _best_solution.resize(_num_vars+1);
    _index_in_unsat_clauses.resize(_num_clauses+1);
    _index_in_unsat_vars.resize(_num_vars+1);

    return true;
}

void ls_solver::build_neighborhood()
{
    vector<bool> neighbor_flag(_num_vars+1);
    for (uint32_t j = 0; j < neighbor_flag.size(); ++j) {
        neighbor_flag[j] = 0;
    }
    for (int v = 1; v <= _num_vars; ++v) {
        variable *vp = &(_vars[v]);
        for (lit lv: vp->literals) {
            int c = lv.clause_num;
            for (lit lc: _clauses[c].literals) {
                if (!neighbor_flag[lc.var_num] && lc.var_num != v) {
                    neighbor_flag[lc.var_num] = 1;
                    vp->neighbor_var_nums.push_back(lc.var_num);
                }
            }
        }
        for (uint32_t j = 0; j < vp->neighbor_var_nums.size(); ++j) {
            neighbor_flag[vp->neighbor_var_nums[j]] = 0;
        }
    }
}

/****************local search**********************************/
//bool  *return value modified
bool ls_solver::local_search(
    const vector<bool> *init_solution
    , long long int _mems_limit
) {
    bool result = false;
    _random_gen.seed(_random_seed);
    _best_found_cost = _num_clauses;
    _conflict_ct.clear();
    _conflict_ct.resize(_num_vars+1,0);

    for (int t = 0; t < _max_tries; t++) {
        initialize(init_solution);
        if (0 == _unsat_clauses.size()) {
            result = true;
            break;
        }

        for (_step = 0; _step < _max_steps; _step++) {
            int flipv = pick_var();
            flip(flipv);
            for(int var_idx:_unsat_vars) ++_conflict_ct[var_idx];
            if (_mems > _mems_limit) {
                return result;
            }


            if ((int)_unsat_clauses.size() < _best_found_cost) {
                _best_found_cost = _unsat_clauses.size();
                assert(_best_solution.size() == _solution.size());
                std::copy(_solution.begin(), _solution.end(),
                          _best_solution.begin());
            }

            if (_verbosity &&
                (_best_found_cost == 0 || (_step & 0x3ffff) == 0x3ffff)
            ) {
                cout << "c [ccnr] tries: "
                << t << " steps: " << _step
                << " best found: " << _best_found_cost
                << endl;
            }


            if (_best_found_cost == 0) {
                result = true;
                break;
            }
        }
        if (0 == _unsat_clauses.size()) {
            result = true;
            break;
        }
    }
    _end_step = _step;
    return result;
}

/**********************************initialize*******************************/
void ls_solver::clear_prev_data()
{
    _unsat_clauses.clear();
    _ccd_vars.clear();
    _unsat_vars.clear();
    for (int &item: _index_in_unsat_clauses)
        item = 0;
    for (int &item: _index_in_unsat_vars)
        item = 0;
}

void ls_solver::initialize(const vector<bool> *init_solution)
{
    clear_prev_data();
    if (!init_solution) {
        //default random generation
        for (int v = 1; v <= _num_vars; v++) {
            _solution[v] = (_random_gen.next(2) == 0 ? 0 : 1);
        }
    } else {
        if ((int)init_solution->size() != _num_vars+1) {
            cout
            << "ERROR: the init solution's size"
            " is not equal to the number of variables."
            << endl;
            exit(-1);
        }
        for (int v = 1; v <= _num_vars; v++) {
            _solution[v] = init_solution->at(v);
        }
    }

    //unsat_appears, will be updated when calling unsat_a_clause function.
    for (int v = 1; v <= _num_vars; v++) {
        _vars[v].unsat_appear = 0;
    }

    //initialize data structure of clauses according to init solution
    for (int c = 0; c < _num_clauses; c++) {
        _clauses[c].sat_count = 0;
        _clauses[c].sat_var = -1;
        _clauses[c].weight = 1;

        for (lit l: _clauses[c].literals) {
            if (_solution[l.var_num] == l.sense) {
                _clauses[c].sat_count++;
                _clauses[c].sat_var = l.var_num;
            }
        }
        if (0 == _clauses[c].sat_count) {
            unsat_a_clause(c);
        }
    }
    _avg_clause_weight = 1;
    _delta_total_clause_weight = 0;
    initialize_variable_datas();
}
void ls_solver::initialize_variable_datas()
{
    variable *vp;
    //scores
    for (int v = 1; v <= _num_vars; v++) {
        vp = &(_vars[v]);
        vp->score = 0;
        for (lit l: vp->literals) {
            int c = l.clause_num;
            if (0 == _clauses[c].sat_count) {
                vp->score += _clauses[c].weight;
            } else if (1 == _clauses[c].sat_count && l.sense == _solution[l.var_num]) {
                vp->score -= _clauses[c].weight;
            }
        }
    }
    //last flip step
    for (int v = 1; v <= _num_vars; v++) {
        _vars[v].last_flip_step = 0;
    }
    //cc datas
    for (int v = 1; v <= _num_vars; v++) {
        vp = &(_vars[v]);
        vp->cc_value = 1;
        if (vp->score > 0) //&&_vars[v].cc_value==1
        {
            _ccd_vars.push_back(v);
            vp->is_in_ccd_vars = 1;
        } else {
            vp->is_in_ccd_vars = 0;
        }
    }
    //the virtual var 0
    vp = &(_vars[0]);
    vp->score = 0;
    vp->cc_value = 0;
    vp->is_in_ccd_vars = 0;
    vp->last_flip_step = 0;
}


/**********************pick variable*******************************************/
int ls_solver::pick_var()
{
    //First, try to get the var with the highest score from _ccd_vars if any
    //----------------------------------------
    int best_var = 0;
    _mems += _ccd_vars.size()/8;
    if (_ccd_vars.size() > 0) {
        best_var = _ccd_vars[0];
        for (int v: _ccd_vars) {
            if (_vars[v].score > _vars[best_var].score) {
                best_var = v;
            } else if (_vars[v].score == _vars[best_var].score &&
                       _vars[v].last_flip_step < _vars[best_var].last_flip_step) {
                best_var = v;
            }
        }
        return best_var;
    }

    //Aspriation Mode
    //----------------------------------------
    if (_aspiration) {
        _aspiration_score = _avg_clause_weight;
        size_t i;
        for (i = 0; i < _unsat_vars.size(); ++i) {
            int v = _unsat_vars[i];
            if (_vars[v].score > _aspiration_score) {
                best_var = v;
                break;
            }
        }
        for (++i; i < _unsat_vars.size(); ++i) {
            int v = _unsat_vars[i];
            if (_vars[v].score > _vars[best_var].score)
                best_var = v;
            else if (_vars[v].score == _vars[best_var].score &&
                     _vars[v].last_flip_step < _vars[best_var].last_flip_step)
                best_var = v;
        }
        if (best_var != 0)
            return best_var;
    }
    //=========================================c

    /**Diversification Mode**/
    update_clause_weights();

    /*focused random walk*/
    int c = _unsat_clauses[_random_gen.next(_unsat_clauses.size())];
    clause *cp = &(_clauses[c]);
    best_var = cp->literals[0].var_num;
    for (size_t k = 1; k < cp->literals.size(); k++) {
        int v = cp->literals[k].var_num;
        if (_vars[v].score > _vars[best_var].score) {
            best_var = v;
        } else if (_vars[v].score == _vars[best_var].score &&
                   _vars[v].last_flip_step < _vars[best_var].last_flip_step) {
            best_var = v;
        }
    }
    return best_var;
}

/************************flip and update functions*****************************/
void ls_solver::flip(int flipv)
{
    _solution[flipv] = 1 - _solution[flipv];
    int org_flipv_score = _vars[flipv].score;
    _mems += _vars[flipv].literals.size();

    // Go through each clause the literal is in and update status
    for (lit l: _vars[flipv].literals) {
        clause *cp = &(_clauses[l.clause_num]);
        if (_solution[flipv] == l.sense) {
            cp->sat_count++;
            if (1 == cp->sat_count) {
                sat_a_clause(l.clause_num);
                cp->sat_var = flipv;
                for (lit lc: cp->literals) {
                    _vars[lc.var_num].score -= cp->weight;
                }
            } else if (2 == cp->sat_count) {
                _vars[cp->sat_var].score += cp->weight;
            }
        } else {
            cp->sat_count--;
            if (0 == cp->sat_count) {
                unsat_a_clause(l.clause_num);
                for (lit lc: cp->literals) {
                    _vars[lc.var_num].score += cp->weight;
                }
            } else if (1 == cp->sat_count) {
                for (lit lc: cp->literals) {
                    if (_solution[lc.var_num] == lc.sense) {
                        _vars[lc.var_num].score -= cp->weight;
                        cp->sat_var = lc.var_num;
                        break;
                    }
                }
            }
        }
    }
    _vars[flipv].score = -org_flipv_score;
    _vars[flipv].last_flip_step = _step;
    //update cc_values
    update_cc_after_flip(flipv);
}
void ls_solver::update_cc_after_flip(int flipv)
{
    int last_item;
    variable *vp = &(_vars[flipv]);
    vp->cc_value = 0;
    _mems += _ccd_vars.size()/4;
    for (int index = _ccd_vars.size() - 1; index >= 0; index--) {
        int v = _ccd_vars[index];
        if (_vars[v].score <= 0) {
            last_item = _ccd_vars.back();
            _ccd_vars.pop_back();
            if (index < (int)_ccd_vars.size()) {
                _ccd_vars[index] = last_item;
            }

            _vars[v].is_in_ccd_vars = 0;
        }
    }

    //update all flipv's neighbor's cc to be 1
    _mems += vp->neighbor_var_nums.size()/4;
    for (int v: vp->neighbor_var_nums) {
        _vars[v].cc_value = 1;
        if (_vars[v].score > 0 && !(_vars[v].is_in_ccd_vars)) {
            _ccd_vars.push_back(v);
            _vars[v].is_in_ccd_vars = 1;
        }
    }
}

/*********************functions for basic operations***************************/
void ls_solver::sat_a_clause(int the_clause)
{
    //use the position of the clause to store the last unsat clause in stack
    int last_item = _unsat_clauses.back();
    _unsat_clauses.pop_back();
    int index = _index_in_unsat_clauses[the_clause];
    if (index < (int)_unsat_clauses.size()) {
        _unsat_clauses[index] = last_item;
    }
    _index_in_unsat_clauses[last_item] = index;
    //update unsat_appear and unsat_vars
    for (lit l: _clauses[the_clause].literals) {
        _vars[l.var_num].unsat_appear--;
        if (0 == _vars[l.var_num].unsat_appear) {
            last_item = _unsat_vars.back();
            _unsat_vars.pop_back();
            index = _index_in_unsat_vars[l.var_num];
            if (index < (int)_unsat_vars.size()) {
                _unsat_vars[index] = last_item;
            }
            _index_in_unsat_vars[last_item] = index;
        }
    }
}
void ls_solver::unsat_a_clause(int the_clause)
{
    _index_in_unsat_clauses[the_clause] = _unsat_clauses.size();
    _unsat_clauses.push_back(the_clause);
    //update unsat_appear and unsat_vars
    for (lit l: _clauses[the_clause].literals) {
        _vars[l.var_num].unsat_appear++;
        if (1 == _vars[l.var_num].unsat_appear) {
            _index_in_unsat_vars[l.var_num] = _unsat_vars.size();
            _unsat_vars.push_back(l.var_num);
        }
    }
}

/************************clause weighting********************************/
void ls_solver::update_clause_weights()
{
    for (int c: _unsat_clauses) {
        _clauses[c].weight++;
    }
    for (int v: _unsat_vars) {
        _vars[v].score += _vars[v].unsat_appear;
        if (_vars[v].score > 0 && 1 == _vars[v].cc_value && !(_vars[v].is_in_ccd_vars)) {
            _ccd_vars.push_back(v);
            _vars[v].is_in_ccd_vars = 1;
        }
    }
    _delta_total_clause_weight += _unsat_clauses.size();
    if (_delta_total_clause_weight >= _num_clauses) {
        _avg_clause_weight += 1;
        _delta_total_clause_weight -= _num_clauses;
        if (_avg_clause_weight > _swt_threshold) {
            smooth_clause_weights();
        }
    }
}
void ls_solver::smooth_clause_weights()
{
    for (int v = 1; v <= _num_vars; v++) {
        _vars[v].score = 0;
    }
    int scale_avg = _avg_clause_weight * _swt_q;
    _avg_clause_weight = 0;
    _delta_total_clause_weight = 0;
    _mems += _num_clauses;
    for (int c = 0; c < _num_clauses; ++c) {
        clause *cp = &(_clauses[c]);
        cp->weight = cp->weight * _swt_p + scale_avg;
        if (cp->weight < 1)
            cp->weight = 1;
        _delta_total_clause_weight += cp->weight;
        if (_delta_total_clause_weight >= _num_clauses) {
            _avg_clause_weight += 1;
            _delta_total_clause_weight -= _num_clauses;
        }
        if (0 == cp->sat_count) {
            for (lit l: cp->literals) {
                _vars[l.var_num].score += cp->weight;
            }
        } else if (1 == cp->sat_count) {
            _vars[cp->sat_var].score -= cp->weight;
        }
    }

    //reset ccd_vars
    _ccd_vars.clear();
    for (int v = 1; v <= _num_vars; v++) {
        variable* vp = &(_vars[v]);
        if (vp->score > 0 && 1 == vp->cc_value) {
            _ccd_vars.push_back(v);
            vp->is_in_ccd_vars = 1;
        } else {
            vp->is_in_ccd_vars = 0;
        }
    }
}

/*****print solution*****************/
void ls_solver::print_solution(bool need_verify)
{
    if (0 == get_cost())
        cout << "s SATISFIABLE" << endl;
    else
        cout << "s UNKNOWN" << endl;

    bool sat_flag = false;
    cout << "c UP numbers: " << up_times << " times" << endl;
    cout << "c flip numbers: " << flip_numbers << " times" << endl;
    cout << "c UP avg flip number: "
        << (double)(flip_numbers + 0.0) / up_times << " s" << endl;
    if (need_verify) {
        for (int c = 0; c < _num_clauses; c++) {
            sat_flag = false;
            for (lit l: _clauses[c].literals) {
                if (_solution[l.var_num] == l.sense) {
                    sat_flag = true;
                    break;
                }
            }
            if (!sat_flag) {
                cout << "c Error: verify error in clause " << c << endl;
                return;
            }
        }
        cout << "c Verified." << endl;
    }
    if (verbosity > 0) {
        cout << "v";
        for (int v = 1; v <= _num_vars; v++) {
            cout << ' ';
            if (_solution[v] == 0)
                cout << '-';
            cout << v;
        }
        cout << endl;
    }
}

void ls_solver::set_verbosity(uint32_t verb)
{
    _verbosity = verb;
}
