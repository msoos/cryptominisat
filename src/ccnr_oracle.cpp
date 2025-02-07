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

#include "ccnr_oracle.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <cassert>
#include "constants.h"
#include "solver.h"

using namespace CMSat;

using std::cout;
using std::endl;
using std::setw;
using std::string;

OracleLS::OracleLS(Solver* _solver) : solver(_solver) {
    max_tries = 10LL;
    max_steps = 10 * 1000;
    random_gen.seed(1337);
}

bool OracleLS::make_space() {
    if (num_vars == 0) return false;
    vars.resize(num_vars+1);
    sol.resize(num_vars+1, 2);
    idx_in_unsat_vars.resize(num_vars+1);

    cls.resize(num_cls);
    idx_in_unsat_cls.resize(num_cls);

    return true;
}

// Updates variables' neighbor_vars, ran once
void OracleLS::build_neighborhood() {
    vector<uint8_t> flag(num_vars+1, 0);
    for (int vnum = 1; vnum <= num_vars; ++vnum) {
        Ovariable& v = vars[vnum];
        for (const auto& l: v.lits) {
            int c_id = l.cl_num;
            for (const auto& lc: cls[c_id].lits) {
                if (!flag[lc.var_num] && lc.var_num != vnum) {
                    flag[lc.var_num] = 1;
                    v.neighbor_vars.push_back(lc.var_num);
                }
            }
        }
        for (const auto& v2 : v.neighbor_vars) flag[v2] = 0;
    }
}

bool OracleLS::local_search(int64_t mems_limit) {
    mems = 0;
    int t;
    for (t = 0; t < max_tries; t++) {
        for (step = 0; step < max_steps; step++) {
            if (unsat_cls.empty()) {
                cout <<  "[ccnr] YAY, mems: " << mems << " steps: " << step << endl;
                /* verb_print(1, "[ccnr] YAY, mems: " << mems << " steps: " << step); */
                check_solution();
                return true;
            }

            int flipv = pick_var();
            if (flipv == -1) {
              verb_print(1, "[ccnr] no var to flip, restart");
              continue;
            }
            assert(assump_map != nullptr && (*assump_map)[flipv] == 2);

            flip(flipv);
            if (mems > mems_limit) {
              cout << "mems limit reached, try: " << t << " step: " << setw(8) << step
                  << " unsat cls: " <<  unsat_cls.size() << endl;
              return false;
            }
            /* cout << "num unsat cls: " << unsat_cls.size() << endl; */

            int u_cost = unsat_cls.size();
            if ((step & 0x3ffff) == 0x3ffff) {
                verb_print(3, "[ccnr] tries: " << t << " steps: " << step
                        << " unsat found: " << u_cost);
            }
        }
        initialize();
    }
    cout << "restart&steps limit reached, try: " << t << " step: " << setw(8) << step
        << " unsat cls: " <<  unsat_cls.size() << endl;
    return false;
}

void OracleLS::initialize() {
    assert(assump_map != nullptr);
    assert((int)assump_map->size() == num_vars+1);
    unsat_cls.clear();
    unsat_vars.clear();
    for (auto &i: idx_in_unsat_cls) i = 0;
    for (auto &i: idx_in_unsat_vars) i = 0;
    for (int v = 1; v <= num_vars; v++) {
      if ((*assump_map)[v] == 2) {
        sol[v] = random_gen.next(2);
      } else {
        sol[v] = (*assump_map)[v];
      }
      /* cout << "sol[" << v << "]: " << (int)sol[v] << endl; */
     }

    //unsat_appears, will be updated when calling unsat_a_clause function.
    for (int v = 1; v <= num_vars; v++) vars[v].unsat_appear = 0;

    //initialize data structure of clauses according to init solution
    for (int cid = 0; cid < num_cls; cid++) {
        auto& cl = cls[cid];
        cl.sat_count = 0;
        cl.sat_var = -1;
        cl.weight = 1;

        for (const Olit& l: cl.lits) {
            const auto val = sol[l.var_num];
            if (val == l.sense) {
                cl.sat_count++;
                cl.sat_var = l.var_num;
            }
        }
        if (cl.sat_count == 0) unsat_a_clause(cid);
    }
    avg_cl_weight = 1;
    delta_tot_cl_weight = 0;
    initialize_variable_datas();
}

void OracleLS::initialize_variable_datas() {
    //scores
    for (int v = 1; v <= num_vars; v++) {
        auto & vp = vars[v];
        vp.score = 0;
        for (const auto& l: vp.lits) {
            int c = l.cl_num;
            if (cls[c].sat_count == 0) {
                vp.score += cls[c].weight;
            } else if (1 == cls[c].sat_count && l.sense == sol[l.var_num]) {
                vp.score -= cls[c].weight;
            }
        }
    }
    //last flip step
    for (int v = 1; v <= num_vars; v++) vars[v].last_flip_step = 0;

    //the virtual var 0
    auto& vp = vars[0];
    vp.score = 0;
    vp.last_flip_step = 0;
}

void OracleLS::adjust_assumps(const vector<int>& assumps_changed) {
    for(const auto& v: assumps_changed) {
        /* cout << "adjust v: " << (int)v << " sol[v]:" << (int)sol[v] << endl; */
        int val = (*assump_map)[v];
        assert(val != 2);
        assert(sol[v] != 2);
        if (sol[v] == val) continue;
        flip(v);
    }
}

void OracleLS::print_cl(int cid) {
    for(auto& l: cls[cid].lits) {
      cout << l << " ";
    }; cout << "0 " << " sat_cnt: " << cls[cid].sat_count << endl;
}

int OracleLS::pick_var() {
    update_clause_weights();
    uint32_t tries = 0;
    bool ok = false;
    int cid;
    while (!ok && tries < 100) {
      cid = unsat_cls[random_gen.next(unsat_cls.size())];
      assert(cid < (int)cls.size());

      const auto& cl = cls[cid];
      for (auto& l: cl.lits) {
        if ((*assump_map)[l.var_num] == 2) {
          ok = true;
          break;
        }
      }
      tries++;
    }
    if (!ok) return -1;
    /* cout << "decided on cl_id: " << cid << " -- "; print_cl(cid); */

    const auto& cl = cls[cid];
    int best_var = -1;
    int best_score = std::numeric_limits<int>::min();
    for (auto& l: cl.lits) {
        int v = l.var_num;
        if ((*assump_map)[v] != 2) continue;

        int score = vars[v].score;
        if (score > best_score) {
            best_var = v;
            best_score = score;
        } else if (score == best_score &&
                   vars[v].last_flip_step < vars[best_var].last_flip_step) {
            best_var = v;
        }
    }
    assert(best_var != -1);
    /* cout << "decided on var: " << best_var << endl; */
    return best_var;
}

void OracleLS::check_clause(int cid) {
  int sat_cnt = 0;
  for (const auto& l: cls[cid].lits) {
    if (sol[l.var_num] == l.sense) sat_cnt++;
  }
  /* cout << "Checking cl_id: " << cid << " -- "; print_cl(cid); */
  /* cout << "sat_cnt: " << sat_cnt << endl; */
  /* cout << "cls[cid].sat_count: " << cls[cid].sat_count << endl; */
  assert(cls[cid].sat_count == sat_cnt);

  if (sat_cnt == 0) {
    bool found = false;
    for(int unsat_cl : unsat_cls) {
      if (unsat_cl == cid) {
        found = true;
        break;
      }
    }
    /* if (!found) { cout << "NOT found in unsat cls" << endl; } */
    assert(idx_in_unsat_cls[cid] < (int)unsat_cls.size());
    assert(unsat_cls[idx_in_unsat_cls[cid]] == cid);
    assert(found);
  }
}

void OracleLS::flip(int v) {
    assert(assump_map != nullptr);
#ifdef SLOW_DEBUG
    for (uint32_t i = 0; i < cls.size(); i++) check_clause(i);
    for(uint32_t i = 0; i < unsat_cls.size(); i++) {
      uint32_t clid = unsat_cls[i];
      assert(idx_in_unsat_cls[clid] == (int)i);
      assert(cls[clid].sat_count == 0);
    }
#endif

    assert(sol[v] == 0 || sol[v] == 1);
    sol[v] = 1 - sol[v];

    const int orig_score = vars[v].score;
    mems += vars[v].lits.size();

    // Go through each clause the literal is in and update status
    for (const auto& l: vars[v].lits) {
        auto& cl = cls[l.cl_num];
        assert(cl.sat_count >= 0);
        assert(cl.sat_count <= (int)cl.lits.size());
        /* cout << "checking effect on cl_id: " << l.cl_num << " -- "; print_cl(l.cl_num); */

        if (sol[v] == l.sense) {
            // make it sat
            cl.sat_count++;

            if (cl.sat_count == 1) {
                sat_a_clause(l.cl_num);
                cl.sat_var = v;
                for (const auto& lc: cl.lits) vars[lc.var_num].score -= cl.weight;
            } else if (cl.sat_count == 2) {
                vars[cl.sat_var].score += cl.weight;
            }
        } else if (sol[v] == !l.sense) {
          // make it unsat
          cl.sat_count--;
          assert(cl.sat_count >= 0);

          // first time touching, and made it unsat
          if (cl.sat_count == 0) {
              unsat_a_clause(l.cl_num);
              for (const auto& lc: cl.lits) vars[lc.var_num].score += cl.weight;
          } else if (cl.sat_count == 1) {
              // Have to update the var that makes the clause satisfied
              for (const auto& lc: cl.lits) {
                  if (sol[lc.var_num] == lc.sense) {
                      vars[lc.var_num].score -= cl.weight;
                      cl.sat_var = lc.var_num;
                      break;
                  }
              }
          }
        }
#ifdef SLOW_DEBUG
        /* cout << "Effect on cl_id: " << l.cl_num << " -- "; print_cl(l.cl_num); */
        check_clause(l.cl_num);
        for(uint32_t i = 0; i < unsat_cls.size(); i++) {
          uint32_t clid = unsat_cls[i];
          assert(cls[clid].sat_count == 0);
          if (idx_in_unsat_cls[clid] != (int)i) {
            cout << "bad clid: " << clid << " i: " << i << endl;
          }
          assert(idx_in_unsat_cls[clid] == (int)i);
        }
#endif
    }
    vars[v].score = -orig_score;
    vars[v].last_flip_step = step;

#ifdef SLOW_DEBUG
    /* cout << "Done flip(). Checking all clauses" << endl; */
    for (uint32_t i = 0; i < cls.size(); i++) check_clause(i);
#endif
}

void OracleLS::sat_a_clause(int cl_id) {
    /* cout << "sat_a_clause: cl_id: " << cl_id << endl; */
    assert(unsat_cls.size() > 0);

    //use the position of the clause to store the last unsat clause in stack
    int last_item = unsat_cls.back();
    unsat_cls.pop_back();
    int index = idx_in_unsat_cls[cl_id];
    if (index < (int)unsat_cls.size()) {
      assert(unsat_cls[index] == cl_id);
      unsat_cls[index] = last_item;
      idx_in_unsat_cls[last_item] = index;
    }

    //update unsat_appear and unsat_vars
    for (const auto& l: cls[cl_id].lits) {
        vars[l.var_num].unsat_appear--;
        if (0 == vars[l.var_num].unsat_appear) {
            last_item = unsat_vars.back();
            unsat_vars.pop_back();
            index = idx_in_unsat_vars[l.var_num];
            if (index < (int)unsat_vars.size()) {
              unsat_vars[index] = last_item;
              idx_in_unsat_vars[last_item] = index;
            }
        }
    }
}

void OracleLS::unsat_a_clause(int cl_id) {
    /* cout << "unsat_a_clause: cl_id: " << cl_id << endl; */
    assert(cls[cl_id].sat_count == 0);
    unsat_cls.push_back(cl_id);
    idx_in_unsat_cls[cl_id] = unsat_cls.size()-1;
    assert(unsat_cls[idx_in_unsat_cls[cl_id]] == cl_id);

    //update unsat_appear and unsat_vars
    for (const auto& l: cls[cl_id].lits) {
        vars[l.var_num].unsat_appear++;
        if (1 == vars[l.var_num].unsat_appear) {
            idx_in_unsat_vars[l.var_num] = unsat_vars.size();
            unsat_vars.push_back(l.var_num);
        }
    }
}

void OracleLS::update_clause_weights() {
    for (int c: unsat_cls) cls[c].weight++;
    for (int v: unsat_vars) vars[v].score += vars[v].unsat_appear;

    delta_tot_cl_weight += unsat_cls.size();
    if (delta_tot_cl_weight >= num_cls) {
        avg_cl_weight += 1;
        delta_tot_cl_weight -= num_cls;
        if (avg_cl_weight > swt_thresh) {
            smooth_clause_weights();
        }
    }
}

void OracleLS::smooth_clause_weights() {
    for (int v = 1; v <= num_vars; v++) vars[v].score = 0;
    int scale_avg = avg_cl_weight * swt_q;
    avg_cl_weight = 0;
    delta_tot_cl_weight = 0;
    mems += num_cls;
    for (int c = 0; c < num_cls; ++c) {
        auto *cp = &(cls[c]);
        cp->weight = cp->weight * swt_p + scale_avg;
        if (cp->weight < 1) cp->weight = 1;
        delta_tot_cl_weight += cp->weight;
        if (delta_tot_cl_weight >= num_cls) {
            avg_cl_weight += 1;
            delta_tot_cl_weight -= num_cls;
        }
        if (0 == cp->sat_count) {
            for (const auto& l: cp->lits) {
                vars[l.var_num].score += cp->weight;
            }
        } else if (1 == cp->sat_count) {
            vars[cp->sat_var].score -= cp->weight;
        }
    }
}

void OracleLS::check_solution() {
    assert(assump_map != nullptr);
    assert((int)assump_map->size() == num_vars+1);
    for(int i = 1; i <= num_vars; i++) {
        if ((*assump_map)[i] != 2) {
            assert(sol[i] == (*assump_map)[i]);
        }
    }

    for (int cid = 0; cid < num_cls; cid++) {
        bool sat_flag = false;
        for (const auto& l: cls[cid].lits) {
            if (sol[l.var_num] == l.sense) {
                sat_flag = true;
                break;
            }
        }
        if (!sat_flag) {
            cout << "c Error: verify error in cl_id : " << cid << " -- "; print_cl(cid);
            exit(-1);
            return;
        }
    }
}

void OracleLS::print_solution() {
    if (0 == get_cost()) cout << "s SATISFIABLE" << endl;
    else cout << "s UNKNOWN" << endl;
    cout << "v";
    for (int v = 1; v <= num_vars; v++) {
        cout << ' ';
        if (sol[v] == 0)
            cout << '-';
        cout << v;
    }
    cout << endl;
}
