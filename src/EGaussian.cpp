/******************************************
Copyright (c) 2012  Cheng-Shen Han
Copyright (c) 2012  Jie-Hong Roland Jiang
Copyright (c) 2018  Mate Soos

For more information, see " When Boolean Satisfiability Meets Gaussian
Elimination in a Simplex Way." by Cheng-Shen Han and Jie-Hong Roland Jiang
in CAV (Computer Aided Verification), 2012: 410-426


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

#include "EGaussian.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <set>

#include "EGaussian.h"
#include "clause.h"
#include "clausecleaner.h"
#include "datasync.h"
#include "propby.h"
#include "solver.h"
#include "time_mem.h"
#include "varreplacer.h"

using std::cout;
using std::endl;
using std::ostream;
using std::set;

#ifdef VERBOSE_DEBUG
#include <iterator>
#endif

using namespace CMSat;

// if variable is not in Gaussian matrix , assiag unknown column
static const uint32_t unassigned_col = std::numeric_limits<uint32_t>::max();

EGaussian::EGaussian(Solver* _solver, const GaussConf& _config, const uint32_t _matrix_no,
                     const vector<Xor>& _xorclauses)
    : solver(_solver), config(_config), matrix_no(_matrix_no), xorclauses(_xorclauses) {
    uint64_t num_unfound = 0;
    vector<Xor> xors;
    for (Xor& x : xorclauses) {
        xors.push_back(x);
    }
    for (Xor& x : xors) {
        x.sort();
    }
    std::sort(xors.begin(), xors.end());

    for (Xor& x : xors) {
        for (uint32_t v : x) {
            if (v > 165) {
                num_unfound++;
                if (solver->conf.verbosity) {
                    cout << "c " << x << endl;
                }
                break;
            }
        }
    }
    cout << "c num_unfound xor: " << num_unfound << endl;

    for (Xor& x : xors) {
        bool must_print = true;
        for (uint32_t v : x) {
            if (v > 165) {
                must_print = false;
                break;
            }
        }
        if (must_print) {
            if (solver->conf.verbosity) {
                cout << "c " << x << endl;
            }
        }
    }
}

EGaussian::~EGaussian() {
    for (uint32_t i = 0; i < clauses_toclear.size(); i++) {
        solver->cl_alloc.clauseFree(clauses_toclear[i].first);
    }
}

void EGaussian::canceling(const uint32_t sublevel) {
    uint32_t a = 0;
    for (int i = clauses_toclear.size() - 1; i >= 0 && clauses_toclear[i].second > sublevel; i--) {
        solver->cl_alloc.clauseFree(clauses_toclear[i].first);
        a++;
    }
    clauses_toclear.resize(clauses_toclear.size() - a);

    PackedMatrix::iterator rowIt = clause_state.beginMatrix();
    (*rowIt).setZero(); // reset this row all zero
}

struct HeapSorter {
    explicit HeapSorter(vector<double>& _activities) : activities(_activities) {
    }

    // higher activity first
    bool operator()(uint32_t a, uint32_t b) {
        return activities[a] < activities[b];
    }

    const vector<double>& activities;
};

uint32_t EGaussian::select_columnorder(matrixset& origMat) {
    var_to_col.clear();
    var_to_col.resize(solver->nVars(), unassigned_col);
    vector<uint32_t> vars_needed;
    uint32_t largest_used_var = 0;

    for (const Xor& x : xorclauses) {
        for (const uint32_t v : x) {
            assert(solver->value(v) == l_Undef);
            if (var_to_col[v] == unassigned_col) {
                vars_needed.push_back(v);
                var_to_col[v] = unassigned_col - 1;
                ;
                largest_used_var = std::max(largest_used_var, v);
            }
        }
    }

    if (vars_needed.size() >= std::numeric_limits<uint32_t>::max() / 2 - 1) {
        if (solver->conf.verbosity) {
            cout << "c Matrix has too many columns, exiting select_columnorder" << endl;
        }

        return 0;
    }
    if (xorclauses.size() >= std::numeric_limits<uint32_t>::max() / 2 - 1) {
        if (solver->conf.verbosity) {
            cout << "c Matrix has too many rows, exiting select_columnorder" << endl;
        }
        return 0;
    }
    var_to_col.resize(largest_used_var + 1);

    origMat.col_to_var.clear();
    std::sort(vars_needed.begin(), vars_needed.end(), HeapSorter(solver->var_act_vsids));

    for (uint32_t v : vars_needed) {
        assert(var_to_col[v] == unassigned_col - 1);
        origMat.col_to_var.push_back(v);
        var_to_col[v] = origMat.col_to_var.size() - 1;
    }

    // for the ones that were not in the order_heap, but are marked in var_to_col
    for (uint32_t v = 0; v != var_to_col.size(); v++) {
        if (var_to_col[v] == unassigned_col - 1) {
            // assert(false && "order_heap MUST be complete!");
            origMat.col_to_var.push_back(v);
            var_to_col[v] = origMat.col_to_var.size() - 1;
        }
    }

#ifdef VERBOSE_DEBUG_MORE
    cout << "(" << matrix_no << ") num_xorclauses: " << num_xorclauses << endl;
    cout << "(" << matrix_no << ") col_to_var: ";
    std::copy(origMat.col_to_var.begin(), origMat.col_to_var.end(),
              std::ostream_iterator<uint32_t>(cout, ","));
    cout << endl;
    cout << "origMat.num_cols:" << origMat.num_cols << endl;
    cout << "col is set:" << endl;
    std::copy(origMat.col_is_set.begin(), origMat.col_is_set.end(),
              std::ostream_iterator<char>(cout, ","));
#endif

    return xorclauses.size();
}

void EGaussian::fill_matrix(matrixset& origMat) {
    var_to_col.clear();

    // decide which variable in matrix column and the number of rows
    origMat.num_rows = select_columnorder(origMat);
    origMat.num_cols = origMat.col_to_var.size();
    if (origMat.num_rows == 0 || origMat.num_cols == 0) {
        return;
    }
    origMat.matrix.resize(origMat.num_rows, origMat.num_cols); // initial gaussian matrix

    uint32_t matrix_row = 0;
    for (uint32_t i = 0; i != xorclauses.size(); i++) {
        const Xor& c = xorclauses[i];
        origMat.matrix.getMatrixAt(matrix_row).set(c, var_to_col, origMat.num_cols);
        matrix_row++;
    }
    assert(origMat.num_rows == matrix_row);

    // reset  gaussian matrixt condition
    GasVar_state.clear();                                // reset variable state
    GasVar_state.growTo(solver->nVars(), non_basic_var); // init varaible state
    origMat.nb_rows.clear();                             // clear non-basic

    // delete gauss watch list for this matrix
    for (size_t ii = 0; ii < solver->gwatches.size(); ii++) {
        clear_gwatches(ii);
    }
    clause_state.resize(1, origMat.num_rows);
    PackedMatrix::iterator rowIt = clause_state.beginMatrix();
    (*rowIt).setZero(); // reset this row all zero
    // print_matrix(origMat);
}

void EGaussian::clear_gwatches(const uint32_t var) {
    GaussWatched* i = solver->gwatches[var].begin();
    GaussWatched* j = i;
    for(GaussWatched* end = solver->gwatches[var].end(); i != end; i++) {
        if (i->matrix_num != matrix_no) {
            *j++ = *i;
        }
    }
    solver->gwatches[var].shrink(i-j);
}

bool EGaussian::full_init(bool& created) {
    assert(solver->ok);
    assert(solver->decisionLevel() == 0);
    gaussian_ret ret; // gaussian elimination result
    bool do_again_gauss = true;
    created = true;

    while (do_again_gauss) { // need to chekc
        do_again_gauss = false;
        solver->sum_initEnGauss++; // to gather statistics

        if (!solver->clauseCleaner->clean_xor_clauses(xorclauses)) {
            return false;
        }

        fill_matrix(cur_matrixset);
        if (cur_matrixset.num_rows == 0 || cur_matrixset.num_cols == 0) {
            created = false;
            return solver->okay();
        }

        eliminate(cur_matrixset); // gauss eliminate algorithm

        // find some row already true false, and insert watch list
        ret = adjust_matrix(cur_matrixset);

        switch (ret) {
            case conflict:
                solver->ok = false;
                solver->sum_Enconflict++;
                return false;
                break;
            case unit_propagation:
                do_again_gauss = true;
                solver->sum_Enpropagate++;

                solver->cancelUntil(0);
                solver->ok = (solver->propagate<true>().isNULL());
                if (!solver->ok) {
                    return false;
                }

                break;
            default:
                break;
        }
    }

    // std::cout << cpuTime() - GaussConstructTime << "    t";
    return true;
}

void EGaussian::eliminate(matrixset& m) {
    uint32_t i = 0;
    uint32_t j = 0;
    PackedMatrix::iterator end = m.matrix.beginMatrix() + m.num_rows;
    PackedMatrix::iterator rowIt = m.matrix.beginMatrix();

    while (i != m.num_rows && j != m.num_cols) { // Gauss-Jordan Elimination
        PackedMatrix::iterator this_matrix_row = rowIt;

        for (; this_matrix_row != end; ++this_matrix_row) {
            if ((*this_matrix_row)[j]) {
                break;
            }
        }
        if (this_matrix_row != end) {
            // swap rows
            if (this_matrix_row != rowIt) {
                (*rowIt).swapBoth(*this_matrix_row);
            }
            // diagnose row
            for (PackedMatrix::iterator k_row = m.matrix.beginMatrix(); k_row != end; ++k_row) {
                // subtract row i from row u;
                if (k_row != rowIt) {
                    if ((*k_row)[j]) {
                        (*k_row).xorBoth(*rowIt);
                    }
                }
            }
            i++;
            ++rowIt;
            GasVar_state[m.col_to_var[j]] = basic_var; // this column is basic variable
            // printf("basic var:%d    n",m.col_to_var[j] + 1);
        }
        j++;
    }
    // print_matrix(m);
}

EGaussian::gaussian_ret EGaussian::adjust_matrix(matrixset& m) {
    PackedMatrix::iterator end = m.matrix.beginMatrix() + m.num_rows;
    PackedMatrix::iterator rowIt = m.matrix.beginMatrix();
    uint32_t row_id = 0;      // row index
    uint32_t nb_var = 0;      // non-basic variable
    bool xorEqualFalse;       // xor =
    uint32_t adjust_zero = 0; //  elimination row

    while (rowIt != end) {
        switch (
            (*rowIt).find_watchVar(tmp_clause, cur_matrixset.col_to_var, GasVar_state, nb_var)) {
            case 0: // this row is all zero
                // printf("%d:Warring: this row is all zero in adjust matrix    n",row_id);
                adjust_zero++;        // information
                if ((*rowIt).rhs()) { // conflic
                    // printf("%d:Warring: this row is conflic in adjust matrix!!!",row_id);
                    return conflict;
                }
                break;
            case 1: { // this row neeed to propogation
                // printf("%d:This row only one variable, need to propogation!!!! in adjust matrix
                // n",row_id);

                xorEqualFalse = !m.matrix.getMatrixAt(row_id).rhs();
                tmp_clause[0] = Lit(tmp_clause[0].var(), xorEqualFalse);
                assert(solver->value(tmp_clause[0].var()) == l_Undef);
                solver->enqueue(tmp_clause[0]); // propagation

                (*rowIt).setZero(); // reset this row all zero
                m.nb_rows.push(
                    std::numeric_limits<uint32_t>::max()); // delete non basic value in this row
                GasVar_state[tmp_clause[0].var()] = non_basic_var; // delete basic value in this row

                solver->sum_initUnit++; // information
                return unit_propagation;
            }
            case 2: { // this row have to variable
                // printf("%d:This row have two variable!!!! in adjust matrix    n",row_id);
                xorEqualFalse = !m.matrix.getMatrixAt(row_id).rhs();
                propagation_twoclause(xorEqualFalse);

                (*rowIt).setZero(); // reset this row all zero
                m.nb_rows.push(
                    std::numeric_limits<uint32_t>::max()); // delete non basic value in this row
                GasVar_state[tmp_clause[0].var()] = non_basic_var; // delete basic value in this row
                solver->sum_initTwo++;                             // information
                break;
            }
            default: // need to update watch list
                // printf("%d:need to update watch list    n",row_id);
                assert(nb_var != std::numeric_limits<uint32_t>::max());

                // insert watch list
                solver->gwatches[tmp_clause[0].var()].push(
                    GaussWatched(row_id, matrix_no)); // insert basic variable
                solver->gwatches[nb_var].push(
                    GaussWatched(row_id, matrix_no)); // insert non-basic variable
                m.nb_rows.push(nb_var);               // record in this row non_basic variable
                break;
        }
        ++rowIt;
        row_id++;
    }
    // printf("DD:nb_rows:%d %d %d    n",m.nb_rows.size() ,   row_id - adjust_zero  ,  adjust_zero);
    assert(m.nb_rows.size() == row_id - adjust_zero);

    m.matrix.resizeNumRows(row_id - adjust_zero);
    m.num_rows = row_id - adjust_zero;

    if (!solver->Is_Gauss_first) {
        solver->sum_initLinear = adjust_zero;
        solver->Is_Gauss_first = true;
    }
    // printf("DD: adjust number of Row:%d    n",num_row);
    // printf("dd:matrix by EGaussian::adjust_matrix    n");
    // print_matrix(m);
    // printf(" adjust_zero %d    n",adjust_zero);
    // printf("%d    t%d    t",m.num_rows , m.num_cols);
    return nothing;
}

inline void EGaussian::propagation_twoclause(const bool xorEqualFalse) {
    // printf("DD:%d %d    n", solver->qhead  ,solver->trail.size());
    // printf("CC %d. %d  %d    n", solver->qhead , solver->trail.size() , solver->decisionLevel());
    solver->cancelUntil(0);
    // printf("DD %d. %d  %d    n", solver->qhead , solver->trail.size() , solver->decisionLevel());

    tmp_clause[0] = tmp_clause[0].unsign();
    tmp_clause[1] = tmp_clause[1].unsign();
    solver->ok = solver->add_xor_clause_inter(tmp_clause, !xorEqualFalse, true);
    release_assert(solver->ok);
    // printf("DD:x%d %d    n",tmp_clause[0].var() , tmp_clause[1].var());
}

inline void EGaussian::conflict_twoclause(PropBy& confl) {
    // assert(tmp_clause.size() == 2);
    // printf("dd %d:This row is conflict two    n",row_n);
    Lit lit1 = tmp_clause[0];
    Lit lit2 = tmp_clause[1];

    solver->attach_bin_clause(tmp_clause[0], tmp_clause[1], true, false);
    // solver->dataSync->signalNewBinClause(lit1, lit2);

    lit1 = ~lit1;
    lit2 = ~lit2;
    solver->attach_bin_clause(tmp_clause[0], tmp_clause[1], true, false);
    // solver->dataSync->signalNewBinClause(lit1, lit2);

    lit1 = ~lit1;
    lit2 = ~lit2;

    confl = PropBy(lit1, true);
    solver->failBinLit = lit2;
}

// Delete this row because we have already add to xor clause, nothing to do anymore
inline void EGaussian::delete_gausswatch(const bool orig_basic, const uint16_t row_n) {
    if (orig_basic) {
        // clear nonbasic value watch list
        bool debug_find = false;
        vec<GaussWatched>& ws_t = solver->gwatches[cur_matrixset.nb_rows[row_n]];
        for (int tmpi = ws_t.size() - 1; tmpi >= 0; tmpi--) {
            if (ws_t[tmpi].row_id == row_n) {
                ws_t[tmpi] = ws_t.last();
                ws_t.shrink(1);
                debug_find = true;
                break;
            }
        }
        assert(debug_find);
    } else {
        clear_gwatches(tmp_clause[0].var());
    }
}

bool EGaussian::find_truths2(const GaussWatched* i, GaussWatched*& j, uint32_t p, PropBy& confl,
                             const uint16_t row_n, bool& do_eliminate, uint32_t& e_var,
                             uint16_t& e_row_n, int& ret_gauss, vector<Lit>& conflict_clause_gauss,
                             uint32_t& conflict_size_gauss, bool& xorEqualFalse_gauss) {
    // printf("dd Watch variable : %d  ,  Wathch row num %d    n", p , row_n);

    uint32_t nb_var = 0;     // new nobasic variable
    int ret;                 // gaussian matrixt condition
    bool xorEqualFalse;      // XOR constrains result
    bool orig_basic = false; // check invoked variable is basic or non-basic
    // init
    e_var = std::numeric_limits<uint32_t>::max();
    e_row_n = std::numeric_limits<uint16_t>::max();
    do_eliminate = false;
    PackedMatrix::iterator rowIt =
        cur_matrixset.matrix.beginMatrix() + row_n; // gaussian watch invoke row
    PackedMatrix::iterator clauseIt = clause_state.beginMatrix();

    // if this clause is alreadt true
    if ((*clauseIt)[row_n]) {
        *j++ = *i; // store watch list
        return true;
    }

    if (GasVar_state[p]) { // swap basic and non_basic variable
        orig_basic = true;
        GasVar_state[cur_matrixset.nb_rows[row_n]] = basic_var;
        GasVar_state[p] = non_basic_var;
    }

    ret = (*rowIt).propGause(tmp_clause, solver->assigns, cur_matrixset.col_to_var, GasVar_state,
                             nb_var, var_to_col[p]);

    switch (ret) {
        // gaussian state     0         1              2            3                4
        // enum gaussian_ret {conflict, unit_conflict, propagation, unit_propagation, nothing};

        // conflict
        case 0: {
            // printf("dd %d:This row is conflict %d    n",row_n , solver->level[p] );
            if (tmp_clause.size() >= conflict_size_gauss) { // choose perfect conflict clause
                *j++ = *i;                                  // we need to leave this
                if (orig_basic) {                           // recover
                    GasVar_state[cur_matrixset.nb_rows[row_n]] = non_basic_var;
                    GasVar_state[p] = basic_var;
                }
                //(*clauseIt).setBit(row_n);  // this clasue already conflict
                return true;
            }

            // binary conflict
            if (tmp_clause.size() == 2) {
                // printf("%d:This row is conflict two    n",row_n);
                delete_gausswatch(orig_basic, row_n);              // delete watch list
                GasVar_state[tmp_clause[0].var()] = non_basic_var; // delete value state;
                GasVar_state[tmp_clause[1].var()] = non_basic_var;
                cur_matrixset.nb_rows[row_n] =
                    std::numeric_limits<uint32_t>::max(); // delete non basic value in this row
                (*rowIt).setZero();                       // reset this row all zero

                conflict_twoclause(confl);            // get two conflict  clause
                solver->qhead = solver->trail.size(); // quick break gaussian elimination
                solver->Gauseqhead = solver->trail.size();

                // for tell outside solver
                ret_gauss = 1; // gaussian matrix is   unit_conflict
                conflict_size_gauss = 2;
                solver->sum_Enunit++;
                return false;
            } else {
                // long conflict clause

                *j++ = *i;
                conflict_clause_gauss = tmp_clause; // choose better conflice clause
                ret_gauss = 0;                      // gaussian matrix is   conflict
                conflict_size_gauss = tmp_clause.size();
                xorEqualFalse_gauss = !cur_matrixset.matrix.getMatrixAt(row_n).rhs();

                if (orig_basic) { // recover
                    GasVar_state[cur_matrixset.nb_rows[row_n]] = non_basic_var;
                    GasVar_state[p] = basic_var;
                }

                //(*clauseIt).setBit(row_n);  // this clasue already conflict
                return true;
            }
        }

        // propagation
        case 2: {
            // printf("%d:This row is propagation : level: %d    n",row_n, solver->level[p]);
            if (ret_gauss == 0) { // Gaussian matrix is already conflict
                *j++ = *i;        // store watch list
                if (orig_basic) { // recover
                    GasVar_state[cur_matrixset.nb_rows[row_n]] = non_basic_var;
                    GasVar_state[p] = basic_var;
                }
                return true;
            }

            xorEqualFalse = !cur_matrixset.matrix.getMatrixAt(row_n).rhs();

            // binary propagation
            if (tmp_clause.size() == 2) {
                // printf("%d:This row is propagation two    n",row_n);

                delete_gausswatch(orig_basic, row_n);              // delete watch list
                GasVar_state[tmp_clause[0].var()] = non_basic_var; // delete value state;
                GasVar_state[tmp_clause[1].var()] = non_basic_var;
                cur_matrixset.nb_rows[row_n] =
                    std::numeric_limits<uint32_t>::max(); // delete non basic value in this row
                (*rowIt).setZero();                       // reset this row all zero

                propagation_twoclause(xorEqualFalse); // propagation two clause

                // for tell outside solver
                ret_gauss = 3;                      // gaussian matrix is   unit_propagation
                solver->Gauseqhead = solver->qhead; // quick break gaussian elimination
                return false;
            }

            // larger-than-2 propagation
            *j++ = *i; // store watch list
            if (solver->decisionLevel() == 0) {
                solver->enqueue(tmp_clause[0]);

                if (orig_basic) { // recover
                    GasVar_state[cur_matrixset.nb_rows[row_n]] = non_basic_var;
                    GasVar_state[p] = basic_var;
                }

                ret_gauss = 3;                      // gaussian matrix is   unit_propagation
                solver->Gauseqhead = solver->qhead; // quick break gaussian elimination
                (*clauseIt).setBit(row_n);          // this clause arleady sat
                return false;

            } else {
                Clause* cla = solver->cl_alloc.Clause_new(tmp_clause, solver->sumConflicts);
                cla->set_gauss_temp_cl();
                const ClOffset offs = solver->cl_alloc.get_offset(cla);

                clauses_toclear.push_back(std::make_pair(offs, solver->trail.size() - 1));
                assert(!cla->freed());
                assert(solver->value((*cla)[0].var()) == l_Undef);
                solver->enqueue((*cla)[0], PropBy(offs));

                ret_gauss = 2; // gaussian matrix is  propagation
            }

            if (orig_basic) { // recover
                GasVar_state[cur_matrixset.nb_rows[row_n]] = non_basic_var;
                GasVar_state[p] = basic_var;
            }

            (*clauseIt).setBit(row_n); // this clause arleady sat
            return true;
        }
        case 5: // find new watch list
            // printf("%d:This row is find new watch:%d => orig %d p:%d    n",row_n ,
            // nb_var,orig_basic , p);
            if (ret_gauss == 0) { // Gaussian matrix is already conflict
                *j++ = *i;        // store watch list
                if (orig_basic) { // recover
                    GasVar_state[cur_matrixset.nb_rows[row_n]] = non_basic_var;
                    GasVar_state[p] = basic_var;
                }
                return true;
            }
            assert(nb_var != std::numeric_limits<uint32_t>::max());
            if (orig_basic) {
                /// clear watchlist, because only one basic value in watchlist
                clear_gwatches(nb_var);
            }
            // update gausWatch list
            solver->gwatches[nb_var].push(GaussWatched(row_n, matrix_no));

            if (!orig_basic) {
                cur_matrixset.nb_rows[row_n] = nb_var; // update in this row non_basic variable
                return true;
            }
            GasVar_state[cur_matrixset.nb_rows[row_n]] =
                non_basic_var;                // recover non_basic variable
            GasVar_state[nb_var] = basic_var; // set basic variable
            e_var = nb_var;                   // store the eliminate valuable
            e_row_n = row_n;
            break;
        case 4: // this row already treu
            // printf("%d:This row is nothing( maybe already true)     n",row_n);
            *j++ = *i;        // store watch list
            if (orig_basic) { // recover
                GasVar_state[cur_matrixset.nb_rows[row_n]] = non_basic_var;
                GasVar_state[p] = basic_var;
            }
            (*clauseIt).setBit(row_n); // this clause arleady sat
            return true;
        default:
            assert(false); // can not here
            break;
    }
    /*     assert(e_var != std::numeric_limits<uint32_t>::max());
        assert(e_row_n != std::numeric_limits<uint16_t>::max());
        assert(orig_basic);
        assert(ret == 5 );
        // assert(solver->gwatches[e_var].size() == 1); <-- definietely wrong, more than one matrix!
         */
    do_eliminate = true;
    return true;
}

void EGaussian::eliminate_col2(uint32_t e_var, uint16_t e_row_n, uint32_t p, PropBy& confl,
                               int& ret_gauss, vector<Lit>& conflict_clause_gauss,
                               uint32_t& conflict_size_gauss, bool& xorEqualFalse_gauss) {
    // cout << "eliminate this column :" << e_var  << " " << p << " " << e_row_n <<  endl;
    PackedMatrix::iterator this_row = cur_matrixset.matrix.beginMatrix() + e_row_n;
    PackedMatrix::iterator rowI = cur_matrixset.matrix.beginMatrix();
    PackedMatrix::iterator end = cur_matrixset.matrix.endMatrix();
    int ret;
    uint32_t e_col = var_to_col[e_var];
    uint32_t ori_nb = 0, ori_nb_col = 0;
    uint32_t nb_var = 0;
    uint32_t num_row = 0; // row inde
    bool xorEqualFalse = false;
    // uint32_t conflict_size = UINT_MAX;
    PackedMatrix::iterator clauseIt = clause_state.beginMatrix();

    // assert(ret_gauss == 4);  // check this matrix is nothing
    // assert(solver->qhead ==  solver->trail.size() ) ;

    while (rowI != end) {
        if ((*rowI)[e_col] && this_row != rowI) {
            // detect orignal non basic watch list change or not
            ori_nb = cur_matrixset.nb_rows[num_row];
            ori_nb_col = var_to_col[ori_nb];
            assert((*rowI)[ori_nb_col]);

            (*rowI).xorBoth(*this_row); // xor eliminate

            if (!(*rowI)[ori_nb_col]) { // orignal non basic value is eliminate
                if (ori_nb != e_var) {  // delelte orignal non basic value in wathc list
                    delete_gausswatch(true, num_row);
                }

                ret = (*rowI).propGause(tmp_clause, solver->assigns, cur_matrixset.col_to_var,
                                        GasVar_state, nb_var, ori_nb_col);

                switch (ret) {
                    case 0: { // conflict
                        // printf("%d:This row is conflict in eliminate col    n",num_row);
                        if (tmp_clause.size() >= conflict_size_gauss || ret_gauss == 3) {
                            solver->gwatches[p].push(GaussWatched(num_row, matrix_no));

                            // update in this row non_basic variable
                            cur_matrixset.nb_rows[num_row] = p;
                            break;
                        }
                        conflict_size_gauss = tmp_clause.size();
                        if (conflict_size_gauss == 2) {
                            // printf("%d:This row is conflict two in eliminate col    n",num_row);
                            delete_gausswatch(false, num_row); // delete gauss matrix
                            assert(GasVar_state[tmp_clause[0].var()] == basic_var);
                            assert(GasVar_state[tmp_clause[1].var()] == non_basic_var);

                            // delete value state;
                            GasVar_state[tmp_clause[0].var()] = non_basic_var;

                            // delete non basic value in this row
                            cur_matrixset.nb_rows[num_row] = std::numeric_limits<uint32_t>::max();
                            (*rowI).setZero();

                            conflict_twoclause(confl);

                            // quick break gaussian elimination
                            solver->qhead = solver->trail.size();
                            solver->Gauseqhead = solver->trail.size();

                            // unit_conflict
                            ret_gauss = 1;
                            solver->sum_Enunit++;

                        } else {
                            solver->gwatches[p].push(
                                GaussWatched(num_row, matrix_no)); // update gausWatch list
                            cur_matrixset.nb_rows[num_row] =
                                p; // // update in this row non_basic variable

                            // for tell outside solver
                            conflict_clause_gauss = tmp_clause; // choose better conflice clause
                            ret_gauss = 0;                      // gaussian matrix is   conflict
                            conflict_size_gauss = tmp_clause.size();
                            xorEqualFalse_gauss = !cur_matrixset.matrix.getMatrixAt(num_row).rhs();

                            // If conflict is happened in eliminaiton conflict, then we only return
                            // immediately
                            solver->qhead = solver->trail.size();
                            solver->Gauseqhead = solver->trail.size();
                        }
                        break;
                    }
                    case 2: {
                        // printf("%d:This row is propagation in eliminate col    n",num_row);
                        if (ret_gauss == 1 || ret_gauss == 0 ||
                            ret_gauss == 3) { // update no_basic_value
                            solver->gwatches[p].push(GaussWatched(num_row, matrix_no));
                            cur_matrixset.nb_rows[num_row] = p;
                            break;
                        }
                        xorEqualFalse = !cur_matrixset.matrix.getMatrixAt(num_row).rhs();
                        if (tmp_clause.size() == 2) {
                            // printf("%d:This row is propagation two in eliminate col n",num_row);
                            solver->sum_Enunit++;
                            delete_gausswatch(false, num_row); // delete watch list
                            assert(GasVar_state[tmp_clause[0].var()] == basic_var);
                            assert(GasVar_state[tmp_clause[1].var()] == non_basic_var);

                            // delete value state;
                            GasVar_state[tmp_clause[0].var()] = non_basic_var;

                            // delete non basic value in this row
                            cur_matrixset.nb_rows[num_row] = std::numeric_limits<uint32_t>::max();

                            // reset this row all zero
                            (*rowI).setZero();
                            propagation_twoclause(xorEqualFalse); // propagation two clause
                            ret_gauss = 3;                        // unit_propagation
                        } else {
                            // update no_basic information
                            solver->gwatches[p].push(GaussWatched(num_row, matrix_no));
                            cur_matrixset.nb_rows[num_row] = p;

                            if (solver->decisionLevel() == 0) {
                                solver->enqueue(tmp_clause[0]);
                                ret_gauss = 3; // unit_propagation
                            } else {
                                Clause* cla =
                                    solver->cl_alloc.Clause_new(tmp_clause, solver->sumConflicts);
                                cla->set_gauss_temp_cl();
                                const ClOffset offs = solver->cl_alloc.get_offset(cla);
                                clauses_toclear.push_back(
                                    std::make_pair(offs, solver->trail.size() - 1));
                                assert(!cla->freed());
                                assert(solver->value((*cla)[0].var()) == l_Undef);
                                solver->enqueue((*cla)[0], PropBy(offs));
                                ret_gauss = 2;
                            }

                            (*clauseIt).setBit(num_row); // this clause arleady sat
                        }
                        break;
                    }
                    case 5: // find new watch list
                        // printf("%d::This row find new watch list :%d in eliminate col
                        // n",num_row,nb_var);

                        solver->gwatches[nb_var].push(GaussWatched(num_row, matrix_no));
                        cur_matrixset.nb_rows[num_row] = nb_var;
                        break;
                    case 4: // this row already tre
                        // printf("%d:This row is nothing( maybe already true) in eliminate col
                        // n",num_row);

                        solver->gwatches[p].push(GaussWatched(num_row, matrix_no));
                        cur_matrixset.nb_rows[num_row] = p; // update in this row non_basic variable
                        (*clauseIt).setBit(num_row);        // this clause arleady sat
                        break;
                    default:
                        // can not here
                        assert(false);
                        break;
                }
            }
        }
        ++rowI;
        num_row++;
    }

    // Debug_funtion();
}

void EGaussian::print_matrix(matrixset& m) const {
    uint32_t row = 0;
    for (PackedMatrix::iterator it = m.matrix.beginMatrix(); it != m.matrix.endMatrix();
         ++it, row++) {
        cout << *it << " -- row:" << row;
        if (row >= m.num_rows) {
            cout << " (considered past the end)";
        }
        cout << endl;
    }
}

void EGaussian::Debug_funtion() {
    for (int i = clauses_toclear.size() - 1; i >= 0; i--) {
        ClOffset offs = clauses_toclear[i].first;
        Clause* cl = solver->cl_alloc.ptr(offs);
        assert(!cl->freed());
    }
}
