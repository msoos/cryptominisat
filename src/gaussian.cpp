/******************************************
Copyright (c) 2012  Cheng-Shen Han
Copyright (c) 2012  Jie-Hong Roland Jiang
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

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

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <set>
#include <iterator>

#include "constants.h"
#include "frat.h"
#include "gaussian.h"
#include "clause.h"
#include "clausecleaner.h"
#include "datasync.h"
#include "propby.h"
#include "solver.h"
#include "time_mem.h"
#include "varreplacer.h"

using std::make_pair;

// #define VERBOSE_DEBUG
// #define SLOW_DEBUG

// don't delete gauss watches, but check when propagating and
// lazily delete then
// #define LAZY_DELETE_HACK

using std::cout;
using std::endl;
using std::ostream;
using std::set;

using namespace CMSat;

// if variable is not in Gaussian matrix , assiag unknown column
static const uint32_t unassigned_col = numeric_limits<uint32_t>::max();

EGaussian::EGaussian(
    Solver* _solver,
    const uint32_t _matrix_no,
    const vector<Xor>& _xorclauses) :
xorclauses(_xorclauses),
solver(_solver),
matrix_no(_matrix_no)
{
}

EGaussian::~EGaussian() {
    delete_gauss_watch_this_matrix();
    free_temps();
}

struct ColSorter {
    explicit ColSorter(Solver* _solver) :
        solver(_solver)
    {
        for(auto p: solver->assumptions) {
            p = solver->varReplacer->get_lit_replaced_with_outer(p);
            p = solver->map_outer_to_inter(p);
            if (p.var() < solver->nVars()) {
                assert(solver->seen.size() > p.var());
                solver->seen[p.var()] = 1;
            }
        }
    }

    void finishup() {
        for(Lit p: solver->assumptions) {
            p = solver->varReplacer->get_lit_replaced_with_outer(p);
            p = solver->map_outer_to_inter(p);
            if (p.var() < solver->nVars()) solver->seen[p.var()] = 0;
        }
    }

    bool operator()(const uint32_t a, const uint32_t b)
    {
        assert(solver->seen.size() > a);
        assert(solver->seen.size() > b);
        if (solver->seen[b] && !solver->seen[a]) {
            return true;
        }

        if (!solver->seen[b] && solver->seen[a]) {
            return false;
        }

        return false;
        //return solver->varData[a].level < solver->varData[b].level;
        //return solver->var_act_vsids[a] > solver->var_act_vsids[b];
    }

    Solver* solver;
};

void EGaussian::select_columnorder() {
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
                largest_used_var = std::max(largest_used_var, v);
            }
        }
    }

    if (vars_needed.size() >= numeric_limits<uint32_t>::max() / 2 - 1) {
        cout << "c Matrix has too many rows, exiting select_columnorder" << endl;
        assert(false);
        exit(-1);
    }
    if (xorclauses.size() >= numeric_limits<uint32_t>::max() / 2 - 1) {
        cout << "c Matrix has too many rows, exiting select_columnorder" << endl;
        assert(false);
        exit(-1);
    }
    var_to_col.resize(largest_used_var + 1);


    ColSorter c(solver);
    std::sort(vars_needed.begin(), vars_needed.end(),c);
    c.finishup();

    #ifdef COL_ORDER_DEBUG_VERBOSE_DEBUG
    cout << "col order: " << endl;
    for(auto& x: vars_needed) {
        bool assump = false;
        for(const auto& ass: solver->assumptions) {
            if (solver->map_outer_to_inter(ass.lit_outer).var() == x) {
                assump = true;
            }
        }
        cout << "assump:" << (int)assump
        << " act: " << std::setprecision(2) << std::scientific
        << solver->var_act_vsids[x] << std::fixed
        << " level: " << solver->varData[x].level
        << endl;
    }
    #endif

    col_to_var.clear();
    for (uint32_t v : vars_needed) {
        assert(var_to_col[v] == unassigned_col - 1);
        col_to_var.push_back(v);
        var_to_col[v] = col_to_var.size() - 1;
    }

    // for the ones that were not in the order_heap, but are marked in var_to_col
    for (uint32_t v = 0; v != var_to_col.size(); v++) {
        if (var_to_col[v] == unassigned_col - 1) {
            // assert(false && "order_heap MUST be complete!");
            col_to_var.push_back(v);
            var_to_col[v] = col_to_var.size() - 1;
        }
    }

    #ifdef VERBOSE_DEBUG_MORE
    cout << "(" << matrix_no << ") num_xorclauses: " << num_xorclauses << endl;
    cout << "(" << matrix_no << ") col_to_var: ";
    std::copy(col_to_var.begin(), col_to_var.end(),
              std::ostream_iterator<uint32_t>(cout, ","));
    cout << endl;
    cout << "num_cols:" << num_cols << endl;
    cout << "col is set:" << endl;
    std::copy(col_is_set.begin(), col_is_set.end(),
              std::ostream_iterator<char>(cout, ","));
    #endif
}

void EGaussian::fill_matrix() {
    assert(solver->prop_at_head());
    var_to_col.clear();

    // decide which variable in matrix column and the number of rows
    select_columnorder();
    num_rows = xorclauses.size();
    num_cols = col_to_var.size();
    if (num_rows == 0 || num_cols == 0) {
        return;
    }
    mat.resize(num_rows, num_cols); // initial gaussian matrix

    reason_mat.clear();
    for (uint32_t row = 0; row < num_rows; row++) {
        const Xor& c = xorclauses[row];
        mat[row].set(c, var_to_col, num_cols);
        vector<char> line;
        line.resize(num_rows, 0);
        line[row] = 1;
        reason_mat.push_back(line);
    }
    assert(reason_mat.size() == num_rows);

    // reset
    var_has_resp_row.clear();
    var_has_resp_row.resize(solver->nVars(), 0);
    row_to_var_non_resp.clear();

    delete_gauss_watch_this_matrix();

    //reset satisfied_xor state
    assert(solver->decisionLevel() == 0);
    satisfied_xors.clear();
    satisfied_xors.resize(num_rows, 0);
}

void EGaussian::delete_gauss_watch_this_matrix()
{
    for (size_t i = 0; i < solver->gwatches.size(); i++) clear_gwatches(i);
}

void EGaussian::clear_gwatches(const uint32_t var)
{
    GaussWatched* i = solver->gwatches[var].begin();
    GaussWatched* j = i;
    for(GaussWatched* end = solver->gwatches[var].end(); i != end; i++) {
        if (i->matrix_num != matrix_no) *j++ = *i;
    }
    solver->gwatches[var].shrink(i-j);
}

bool EGaussian::full_init(bool& created) {
    assert(solver->okay());
    assert(solver->decisionLevel() == 0);
    assert(solver->prop_at_head());
    assert(initialized == false);
    frat_func_start();
    created = true;

    uint32_t trail_before;
    while (true) {
        trail_before = solver->trail_size();
        solver->clauseCleaner->clean_xor_clauses(xorclauses, false);
        if (!solver->okay()) return false;

        fill_matrix();
        before_init_density = get_density();
        if (num_rows == 0 || num_cols == 0) {
            created = false;
            return solver->okay();
        }

        eliminate();

        // find some row already true false, and insert watch list
        free_temps(); create_temps();
        delete_reasons(); xor_reasons.resize(num_rows);
        gret ret = init_adjust_matrix();
        free_temps(); create_temps();
        delete_reasons(); xor_reasons.resize(num_rows);

        switch (ret) {
            case gret::confl:
                return false;
                break;
            case gret::prop:
                assert(solver->decisionLevel() == 0);
                assert(solver->okay());
                solver->ok = solver->propagate<false>().isnullptr();
                if (!solver->okay()) {
                    verb_print(5, "eliminate & adjust matrix during init lead to UNSAT");
                    return false;
                }
                break;
            default:
                break;
        }

        assert(solver->prop_at_head());

        //Let's exit if nothing new happened
        if (solver->trail_size() == trail_before) break;
    }
    SLOW_DEBUG_DO(check_watchlist_sanity());
    verb_print(2, "[gauss] initialized matrix " << matrix_no);

    free_temps(); create_temps();
    delete_reasons(); xor_reasons.resize(num_rows);

    after_init_density = get_density();

    initialized = true;
    update_cols_vals_set(true);
    SLOW_DEBUG_DO(check_invariants());

    frat_func_end();
    return solver->okay();
}

void EGaussian::free_temps()
{
    for(auto& x: tofree) delete[] x;
    tofree.clear();

    delete cols_unset;
    cols_unset = nullptr;
    delete cols_vals;
    cols_vals = nullptr;
    delete tmp_col;
    tmp_col = nullptr;
    delete tmp_col2;
    tmp_col2 = nullptr;
}

void EGaussian::create_temps()
{
    assert(tofree.empty());
    uint32_t num_64b = num_cols/64+(bool)(num_cols%64);

    int64_t* x = new int64_t[num_64b+1];
    tofree.push_back(x);
    cols_unset = new PackedRow(num_64b, x);

    x = new int64_t[num_64b+1];
    tofree.push_back(x);
    cols_vals = new PackedRow(num_64b, x);

    x = new int64_t[num_64b+1];
    tofree.push_back(x);
    tmp_col = new PackedRow(num_64b, x);

    x = new int64_t[num_64b+1];
    tofree.push_back(x);
    tmp_col2 = new PackedRow(num_64b, x);

    /* cols_unset->setZero(); */
    cols_unset->rhs() = 0;
    /* cols_vals->setZero(); */
    cols_vals->rhs() = 0;
    /* tmp_col->setZero(); */
    tmp_col->rhs() = 0;
    /* tmp_col2->setZero(); */
    tmp_col2->rhs() = 0;
}

void EGaussian::xor_in_bdd(const uint32_t a, const uint32_t b)
{
    for(uint32_t i = 0; i < reason_mat[a].size(); i ++) {
        reason_mat[a][i] ^= reason_mat[b][i];
    }
}

void EGaussian::eliminate() {
    PackedMatrix::iterator end_row_it = mat.begin() + num_rows;
    PackedMatrix::iterator rowI = mat.begin();
    uint32_t row_i = 0;
    uint32_t col = 0;

    // Gauss-Jordan Elimination
    while (row_i != num_rows && col != num_cols) {
        PackedMatrix::iterator row_with_1_in_col = rowI;
        uint32_t row_with_1_in_col_n = row_i;

        //Find first "1" in column.
        for (; row_with_1_in_col != end_row_it; ++row_with_1_in_col, row_with_1_in_col_n++) {
            if ((*row_with_1_in_col)[col]) {
                break;
            }
        }

        //We have found a "1" in this column
        if (row_with_1_in_col != end_row_it) {
            //cout << "col zeroed:" << col << " var is: " << col_to_var[col] + 1 << endl;
            var_has_resp_row[col_to_var[col]] = 1;

            // swap row row_with_1_in_col and rowIt
            if (row_with_1_in_col != rowI) {
                (*rowI).swapBoth(*row_with_1_in_col);
                std::swap(reason_mat[row_i], reason_mat[row_with_1_in_col_n]);
            }

            // XOR into *all* rows that have a "1" in column COL
            // Since we XOR into *all*, this is Gauss-Jordan (and not just Gauss)
            uint32_t k = 0;
            for (PackedMatrix::iterator k_row = mat.begin()
                ; k_row != end_row_it
                ; ++k_row, k++
            ) {
                // xor rows K and I
                if (k_row != rowI) {
                    if ((*k_row)[col]) {
                        (*k_row).xor_in(*rowI);
                        if (solver->frat->enabled()) xor_in_bdd(k, row_i);
                    }
                }
            }
            row_i++;
            ++rowI;
        }
        col++;
    }
    //print_matrix();
}

vector<Lit>* EGaussian::get_reason(const uint32_t row, int32_t& out_ID) {
    frat_func_start();
    if (!xor_reasons[row].must_recalc) {
        out_ID = xor_reasons[row].ID;
        return &(xor_reasons[row].reason);
    }

    // Clean up previous one
    if (solver->frat->enabled() && xor_reasons[row].ID != 0) {
        *solver->frat << del << xor_reasons[row].ID << xor_reasons[row].reason << fin;
    }

    vector<Lit>& tofill = xor_reasons[row].reason;
    tofill.clear();

    mat[row].get_reason(
        tofill,
        solver->assigns,
        col_to_var,
        *cols_vals,
        *tmp_col2,
        xor_reasons[row].propagated);

    if (solver->frat->enabled()) {
        Xor reason = xor_reason_create(row);
        out_ID = ++solver->clauseID;
        assert(tofill.size() == reason.size());
        *solver->frat << implyclfromx << out_ID << tofill << fratchain << reason.XID << fin;
        *solver->frat << delx << reason << fin;
        VERBOSE_PRINT("ID of asserted get_reason ID: " << out_ID);
    }

    xor_reasons[row].must_recalc = false;
    xor_reasons[row].ID = out_ID;
    frat_func_end();
    return &tofill;
}

Xor EGaussian::xor_reason_create(const uint32_t row_n) {
    bool rhs = mat[row_n].rhs();
    assert(!(rhs == false && mat[row_n].popcnt() == 0)); // trivial clause not supported here
    assert(solver->frat->enabled());
    frat_func_start();

    Xor reason;
    mat[row_n].get_reason_xor(reason, solver->assigns, col_to_var, *cols_vals, *tmp_col2);
    INC_XID(reason);
    *solver->frat << addx << reason << fratchain;
    for(uint32_t i = 0; i < reason_mat[row_n].size(); i++) {
        if (reason_mat[row_n][i]) *solver->frat << xorclauses[i].XID;
    }
    *solver->frat << fin;
    VERBOSE_PRINT("reason XOR: " << reason);

    frat_func_end();
    return reason;
}

gret EGaussian::init_adjust_matrix() {
    assert(solver->decisionLevel() == 0);
    assert(row_to_var_non_resp.empty());
    assert(satisfied_xors.size() >= num_rows);
    delete_reasons(); xor_reasons.resize(num_rows);
    frat_func_start();
    VERBOSE_PRINT("mat[" << matrix_no << "] init adjusting matrix");

    PackedMatrix::iterator end = mat.begin() + num_rows;
    PackedMatrix::iterator rowI = mat.begin(); //row index iterator
    uint32_t row_i = 0;      // row index
    uint32_t adjust_zero = 0; //  elimination row

    while (rowI != end) {
        uint32_t non_resp_var;
        const uint32_t popcnt = (*rowI).find_watchVar(
            tmp_clause, col_to_var, var_has_resp_row, non_resp_var);

        switch (popcnt) {

            //Conflict or satisfied
            case 0:
                VERBOSE_PRINT("Empty XOR during init_adjust_matrix, rhs: " << (*rowI).rhs());
                adjust_zero++;

                // conflict
                if ((*rowI).rhs()) {
                    if (solver->frat->enabled()) {
                        *solver->frat << "init_adjust_matrix conflict\n";
                        const auto reason = xor_reason_create(row_i);
                        const int32_t ID = ++solver->clauseID;
                        *solver->frat << implyclfromx << ID << fratchain << reason.XID << fin;
                        *solver->frat << delx << reason << fin;
                        set_unsat_cl_id(ID);
                    }
                    solver->ok = false;
                    VERBOSE_PRINT("-> empty clause during init_adjust_matrix");
                    VERBOSE_PRINT("-> conflict on row: " << row_i);
                    return gret::confl;
                }
                VERBOSE_PRINT("-> empty on row: " << row_i);
                VERBOSE_PRINT("-> Satisfied XORs set for row: " << row_i);
                satisfied_xors[row_i] = 1;
                break;

            //Unit (i.e. toplevel unit)
            case 1:
            {
                VERBOSE_PRINT("Unit XOR during init_adjust_matrix, vars: " << tmp_clause);
                tmp_clause[0] = Lit(tmp_clause[0].var(), !mat[row_i].rhs());
                assert(solver->value(tmp_clause[0].var()) == l_Undef);
                if (solver->frat->enabled()) {
                    const auto reason = xor_reason_create(row_i);
                    const int32_t ID = ++solver->clauseID;
                    *solver->frat << implyclfromx << ID << tmp_clause[0] << fratchain << reason.XID << fin;
                    *solver->frat << delx << reason << fin;
                    del_unit_cls.push_back(make_pair(ID, tmp_clause[0]));
                }
                solver->enqueue<false>(tmp_clause[0]);

                VERBOSE_PRINT("-> UNIT during adjust: " << tmp_clause[0]);
                VERBOSE_PRINT("-> Satisfied XORs set for row: " << row_i);
                satisfied_xors[row_i] = 1;
                SLOW_DEBUG_DO(assert(check_row_satisfied(row_i)));

                //adjusting
                (*rowI).setZero(); // reset this row all zero
                row_to_var_non_resp.push_back(numeric_limits<uint32_t>::max());
                var_has_resp_row[tmp_clause[0].var()] = 0;
                return gret::prop;
            }

            //Binary XOR (i.e. toplevel binary XOR)
            case 2: {
                VERBOSE_PRINT("Binary XOR during init_adjust_matrix, vars: " << tmp_clause);
                tmp_clause[0] = tmp_clause[0].unsign();
                tmp_clause[1] = tmp_clause[1].unsign();
                // TODO FRAT -- clean this up, no need for duplication
                if (solver->frat->enabled()) {
                    *solver->frat << "binary XOR from init_adjust matrix begin\n";
                    const auto reason = xor_reason_create(row_i);
                    vector<Lit> out = tmp_clause;
                    out[0] ^= !mat[row_i].rhs();
                    int32_t ID = ++solver->clauseID;
                    *solver->frat << implyclfromx << ID << out << fratchain << reason.XID << fin;
                    solver->attach_bin_clause(out[0], out[1], false, ID);
                    VERBOSE_PRINT("ID of bin XOR found (part 1): " << ID);

                    out[0] = out[0]^true; out[1] = out[1]^true;
                    int32_t ID2 = ++solver->clauseID;
                    *solver->frat << implyclfromx << ID2 << out << fratchain << reason.XID << fin;
                    solver->attach_bin_clause(out[0], out[1], false, ID2);
                    VERBOSE_PRINT("ID of bin XOR found (part 2): " << ID2);

                    *solver->frat << delx << reason << fin;
                    *solver->frat << "binary XOR from init_adjust matrix end\n";
                } else {
                    vector<Lit> out = tmp_clause;
                    out[0] ^= !mat[row_i].rhs();
                    int32_t ID = ++solver->clauseID;
                    solver->attach_bin_clause(out[0], out[1], false, ID);
                    out[0] = out[0]^true; out[1] = out[1]^true;
                    int32_t ID2 = ++solver->clauseID;
                    solver->attach_bin_clause(out[0], out[1], false, ID2);
                }
                VERBOSE_PRINT("-> toplevel bin-xor on row: " << row_i << " cl2: " << tmp_clause);

                // reset this row all zero, no need for this row
                (*rowI).rhs() = 0;
                (*rowI).setZero();

                row_to_var_non_resp.push_back(numeric_limits<uint32_t>::max()); // delete non-basic value in this row
                var_has_resp_row[tmp_clause[0].var()] = 0; // delete basic value in this row
                break;
            }

            default: // need to update watch list
                // printf("%d:need to update watch list    n",row_id);
                assert(non_resp_var != numeric_limits<uint32_t>::max());

                // insert watch list
                VERBOSE_PRINT("-> watch 1: resp var " << tmp_clause[0].var()+1 << " for row " << row_i);
                VERBOSE_PRINT("-> watch 2: non-resp var " << non_resp_var+1 << " for row " << row_i);
                solver->gwatches[tmp_clause[0].var()].push(
                    GaussWatched(row_i, matrix_no)); // insert basic variable
                solver->gwatches[non_resp_var].push(
                    GaussWatched(row_i, matrix_no)); // insert non-basic variable
                row_to_var_non_resp.push_back(non_resp_var); // record in this row non-basic variable
                break;
        }
        ++rowI;
        row_i++;
    }
    assert(row_to_var_non_resp.size() == row_i - adjust_zero);

    mat.resizeNumRows(row_i - adjust_zero);
    num_rows = row_i - adjust_zero;

    frat_func_end();
    return gret::nothing_satisfied;
}

// Delete this row because we have already add to xor clause, nothing to do anymore
void EGaussian::delete_gausswatch(
    const uint32_t row_n
) {
    // clear nonbasic value watch list
    bool debug_find = false;
    vec<GaussWatched>& ws_t = solver->gwatches[row_to_var_non_resp[row_n]];

    for (int32_t tmpi = ws_t.size() - 1; tmpi >= 0; tmpi--) {
        if (ws_t[tmpi].row_n == row_n
            && ws_t[tmpi].matrix_num == matrix_no
        ) {
            ws_t[tmpi] = ws_t.last();
            ws_t.shrink(1);
            debug_find = true;
            break;
        }
    }
    #ifdef VERBOSE_DEBUG
    cout
    << "mat[" << matrix_no << "] "
    << "Tried cleaning watch of var: "
    << row_to_var_non_resp[row_n]+1 << endl;
    #endif
    assert(debug_find);
}

uint32_t EGaussian::get_max_level(const GaussQData& gqd, const uint32_t row_n)
{
    int32_t ID;
    auto cl = get_reason(row_n, ID);
    uint32_t nMaxLevel = gqd.currLevel;
    uint32_t nMaxInd = 1;

    for (uint32_t i = 1; i < cl->size(); i++) {
        Lit l = (*cl)[i];
        uint32_t nLevel = solver->varData[l.var()].level;
        if (nLevel > nMaxLevel) {
            nMaxLevel = nLevel;
            nMaxInd = i;
        }
    }

    //should we??
    if (nMaxInd != 1) {
        std::swap((*cl)[1], (*cl)[nMaxInd]);
    }
    return nMaxLevel;
}

bool EGaussian::find_truths(
    GaussWatched*& i,
    GaussWatched*& j,
    const uint32_t var,
    const uint32_t row_n,
    GaussQData& gqd
) {
    assert(gqd.ret != gauss_res::confl);
    assert(initialized);

    #ifdef LAZY_DELETE_HACK
    if (!mat[row_n][var_to_col[var]]) {
        //lazy delete
        return true;
    }
    #endif
    // printf("dd Watch variable : %d  ,  Wathch row num %d    n", p , row_n);

    VERBOSE_PRINT(
        "mat[" << matrix_no << "] find_truths" << endl
        << "-> row: " << row_n << endl
        << "-> var: " << var+1 << endl
        << "-> dec lev:" << solver->decisionLevel());
    SLOW_DEBUG_DO(assert(row_n < num_rows));
    SLOW_DEBUG_DO(assert(satisfied_xors.size() > row_n));

    // this XOR is already satisfied
    if (satisfied_xors[row_n]) {
        VERBOSE_PRINT("-> xor satisfied as per satisfied_xors[row_n]");
        SLOW_DEBUG_DO(assert(check_row_satisfied(row_n)));
        *j++ = *i;
        find_truth_ret_satisfied_precheck++;
        return true;
    }

    // swap resp and non-resp variable
    bool was_resp_var = false;
    if (var_has_resp_row[var] == 1) {
        //var has a responsible row, so THIS row must be it!
        //since if a var has a responsible row, only ONE row can have a 1 there
        was_resp_var = true;
        var_has_resp_row[row_to_var_non_resp[row_n]] = 1;
        var_has_resp_row[var] = 0;
    }

    uint32_t new_resp_var;
    Lit ret_lit_prop;
    SLOW_DEBUG_DO(check_cols_unset_vals());
    const gret ret = mat[row_n].propGause(
        solver->assigns,
        col_to_var,
        var_has_resp_row,
        new_resp_var,
        *tmp_col,
        *tmp_col2,
        *cols_vals,
        *cols_unset,
        ret_lit_prop);
    find_truth_called_propgause++;

    switch (ret) {
        case gret::confl: {
            find_truth_ret_confl++;
            *j++ = *i;

            xor_reasons[row_n].must_recalc = true;
            xor_reasons[row_n].propagated = lit_Undef;
            gqd.confl = PropBy(matrix_no, row_n);
            gqd.ret = gauss_res::confl;
            VERBOSE_PRINT("--> conflict");

            // have to get reason if toplevel (reason will never be asked)
            if (solver->decisionLevel() == 0 && solver->frat->enabled()) {
                VERBOSE_PRINT("-> conflict at toplevel during find_truths");
                int32_t out_ID;
                get_reason(row_n, out_ID);
                *solver->frat << add << ++solver->clauseID << fin;
                set_unsat_cl_id(solver->clauseID);
            }

            if (was_resp_var) { // recover
                var_has_resp_row[row_to_var_non_resp[row_n]] = 0;
                var_has_resp_row[var] = 1;
            }
            return false;
        }

        case gret::prop: {
            find_truth_ret_prop++;
            VERBOSE_PRINT("--> propagation");
            *j++ = *i;

            xor_reasons[row_n].must_recalc = true;
            xor_reasons[row_n].propagated = ret_lit_prop;
            assert(solver->value(ret_lit_prop.var()) == l_Undef);
            prop_lit(gqd, row_n, ret_lit_prop);

            update_cols_vals_set(ret_lit_prop);
            gqd.ret = gauss_res::prop;

            if (was_resp_var) { // recover
                var_has_resp_row[row_to_var_non_resp[row_n]] = 0;
                var_has_resp_row[var] = 1;
            }

            VERBOSE_PRINT("--> Satisfied XORs set for row: " << row_n);
            satisfied_xors[row_n] = 1;
            SLOW_DEBUG_DO(assert(check_row_satisfied(row_n)));
            return true;
        }

        // find new watch list
        case gret::nothing_fnewwatch:
            VERBOSE_PRINT("--> found new watch: " << new_resp_var+1);

            find_truth_ret_fnewwatch++;
            // printf("%d:This row is find new watch:%d => orig %d p:%d    n",row_n ,
            // new_resp_var,orig_basic , p);

            if (was_resp_var) {
                /// clear watchlist, because only one responsible value in watchlist
                assert(new_resp_var != var);
                clear_gwatches(new_resp_var);
                VERBOSE_PRINT("Cleared watchlist for new resp var: " << new_resp_var+1);
                VERBOSE_PRINT("After clear...");
                VERBOSE_DEBUG_DO(print_gwatches(new_resp_var));
            }
            assert(new_resp_var != var);
            //VERBOSE_DEBUG_DO(print_gwatches(new_resp_var));
            SLOW_DEBUG_DO(check_row_not_in_watch(new_resp_var, row_n));
            solver->gwatches[new_resp_var].push(GaussWatched(row_n, matrix_no));

            if (was_resp_var) {
                //it was the responsible one, so the newly watched var
                //is the new column it's responsible for
                //so elimination will be needed

                //clear old one, add new resp
                var_has_resp_row[row_to_var_non_resp[row_n]] = 0;
                var_has_resp_row[new_resp_var] = 1;

                // store the eliminate variable & row
                gqd.new_resp_var = new_resp_var;
                gqd.new_resp_row = row_n;
                gqd.do_eliminate = true;
                return true;
            } else {
                row_to_var_non_resp[row_n] = new_resp_var;
                return true;
            }

        // this row already true
        case gret::nothing_satisfied:
            VERBOSE_PRINT("--> satisfied");

            find_truth_ret_satisfied++;
            // printf("%d:This row is nothing( maybe already true)     n",row_n);
            *j++ = *i;
            if (was_resp_var) { // recover
                var_has_resp_row[row_to_var_non_resp[row_n]] = 0;
                var_has_resp_row[var] = 1;
            }

            VERBOSE_PRINT("--> Satisfied XORs set for row: " << row_n);
            satisfied_xors[row_n] = 1;
            SLOW_DEBUG_DO(assert(check_row_satisfied(row_n)));
            return true;

        //error here
        default:
            assert(false); // cannot be here
            break;
    }

    assert(false);
    return true;
}

inline void EGaussian::update_cols_vals_set(const Lit lit1)
{
    cols_unset->clearBit(var_to_col[lit1.var()]);
    if (!lit1.sign()) {
        cols_vals->setBit(var_to_col[lit1.var()]);
    }
}

void EGaussian::update_cols_vals_set(bool force)
{
    assert(initialized);

    //cancelled_since_val_update = true;
    if (cancelled_since_val_update || force) {
        cols_vals->setZero();
        cols_unset->setOne();

        for(uint32_t col = 0; col < col_to_var.size(); col++) {
            uint32_t var = col_to_var[col];
            if (solver->value(var) != l_Undef) {
                cols_unset->clearBit(col);
                if (solver->value(var) == l_True) {
                    cols_vals->setBit(col);
                }
            }
        }
        last_val_update = solver->trail.size();
        cancelled_since_val_update = false;
        return;
    }

    assert(solver->trail.size() >= last_val_update);
    for(uint32_t i = last_val_update; i < solver->trail.size(); i++) {
        uint32_t var = solver->trail[i].lit.var();
        if (var_to_col.size() <= var) {
            continue;
        }
        uint32_t col = var_to_col[var];
        if (col != unassigned_col) {
            assert (solver->value(var) != l_Undef);
            cols_unset->clearBit(col);
            if (solver->value(var) == l_True) {
                cols_vals->setBit(col);
            }
        }
    }
    last_val_update = solver->trail.size();
}

void EGaussian::prop_lit(
    const GaussQData& gqd, const uint32_t row_i, const Lit ret_lit_prop)
{
    uint32_t lev;
    if (gqd.currLevel == solver->decisionLevel()) lev = gqd.currLevel;
    else lev = get_max_level(gqd, row_i);
    if (lev == 0 && solver->frat->enabled()) {
        //we produce the reason, because we need it immediately, since it's toplevel
        int32_t out_ID;
        VERBOSE_PRINT("--> BDD reason needed in prop due to lev 0 enqueue");
        [[maybe_unused]] auto const x = get_reason(row_i, out_ID);

        #ifdef SLOW_DEBUG
        VERBOSE_PRINT("--> reason clause: " << *x);
        uint32_t num_unset = 0;
        for(auto const& a: *x) {
            assert(solver->value(a) != l_True);
            if (solver->value(a) == l_False) {
                assert(solver->varData[a.var()].level == 0);
                assert(solver->unit_cl_IDs[a.var()] != 0);
            }
            if (solver->value(a) == l_Undef) num_unset ++;
        }
        assert(num_unset == 1);
        #endif
    }
    solver->enqueue<false>(ret_lit_prop, lev, PropBy(matrix_no, row_i));
}

void EGaussian::eliminate_col(uint32_t p, GaussQData& gqd)
{
    const uint32_t new_resp_row_n = gqd.new_resp_row;
    PackedMatrix::iterator rowI = mat.begin();
    PackedMatrix::iterator end = mat.end();
    const uint32_t new_resp_col = var_to_col[gqd.new_resp_var];
    uint32_t row_i = 0;
    bool unsat_set = false;

    #ifdef VERBOSE_DEBUG
    cout
    << "mat[" << matrix_no << "] "
    << "** eliminating this column: " << new_resp_col << endl
    << "-> row that will be the SOLE one having a 1: " << gqd.new_resp_row << endl
    << "-> var associated with col: " << gqd.new_resp_var+1
    <<  endl;
    #endif
    elim_called++;

    while (rowI != end) {
        //Row has a '1' in eliminating column, and it's not the row responsible
        if (new_resp_row_n != row_i && (*rowI)[new_resp_col]) {

            // detect orignal non-basic watch list change or not
            uint32_t orig_non_resp_var = row_to_var_non_resp[row_i];
            uint32_t orig_non_resp_col = var_to_col[orig_non_resp_var];
            assert((*rowI)[orig_non_resp_col]);
            VERBOSE_PRINT("--> This row " << row_i
                << " is being watched on var: " << orig_non_resp_var + 1
                << " i.e. it must contain '1' for this var's column");

            assert(satisfied_xors[row_i] == 0);
            (*rowI).xor_in(*(mat.begin() + new_resp_row_n));
            if (solver->frat->enabled()) xor_in_bdd(row_i, new_resp_row_n);

            elim_xored_rows++;

            //NOTE: responsible variable cannot be eliminated of course
            //      (it's the only '1' in that column).
            //      But non-responsible can be eliminated. So let's check that
            //      and then deal with it if we have to
            if (!(*rowI)[orig_non_resp_col]) {

                #ifdef VERBOSE_DEBUG
                cout
                << "--> This row " << row_i
                << " can no longer be watched (non-responsible), it has no '1' at col " << orig_non_resp_col
                << " (var " << col_to_var[orig_non_resp_col]+1 << ")"
                << " fixing up..."<< endl;
                #endif

                // Delete orignal non-responsible var from watch list
                if (orig_non_resp_var != gqd.new_resp_var) {
                    #ifndef LAZY_DELETE_HACK
                    delete_gausswatch(row_i);
                    #endif
                } else {
                     //this does not need a delete, because during
                     //find_truths, we already did clear_gwatches of the
                     //orig_non_resp_var, so there is nothing to delete here
                 }

                Lit ret_lit_prop;
                uint32_t new_non_resp_var = 0;
                #ifdef SLOW_DEBUG
                check_cols_unset_vals();
                #endif
                const gret ret = (*rowI).propGause(
                    solver->assigns,
                    col_to_var,
                    var_has_resp_row,
                    new_non_resp_var,
                    *tmp_col,
                    *tmp_col2,
                    *cols_vals,
                    *cols_unset,
                    ret_lit_prop
                );
                elim_called_propgause++;

                switch (ret) {
                    case gret::confl: {
                        elim_ret_confl++;
                        VERBOSE_PRINT("---> conflict during eliminate_col's fixup");
                        solver->gwatches[p].push(GaussWatched(row_i, matrix_no));

                        // update in this row non-basic variable
                        row_to_var_non_resp[row_i] = p;

                        xor_reasons[row_i].must_recalc = true;
                        xor_reasons[row_i].propagated = lit_Undef;
                        gqd.confl = PropBy(matrix_no, row_i);
                        gqd.ret = gauss_res::confl;

                        // have to get reason if toplevel (reason will never be asked)
                        if (solver->decisionLevel() == 0 && solver->frat->enabled() && !unsat_set) {
                            VERBOSE_PRINT("-> conflict at toplevel during eliminate_col");
                            int32_t ID;
                            get_reason(row_i, ID); // needed to make below step valid
                                                   // but we don't really need the reason
                            int32_t fin_ID = ++solver->clauseID;
                            *solver->frat << add << fin_ID << fin;
                            set_unsat_cl_id(fin_ID);
                            unsat_set = true;
                        }

                        break;
                    }
                    case gret::prop: {
                        elim_ret_prop++;
                        VERBOSE_PRINT("---> propagation during eliminate_col's fixup");

                        // if conflicted already, just update non-basic variable
                        if (gqd.ret == gauss_res::confl) {
                            SLOW_DEBUG_DO(check_row_not_in_watch(p, row_i));
                            solver->gwatches[p].push(GaussWatched(row_i, matrix_no));
                            row_to_var_non_resp[row_i] = p;
                            break;
                        }

                        // update no_basic information
                        SLOW_DEBUG_DO(check_row_not_in_watch(p, row_i));
                        solver->gwatches[p].push(GaussWatched(row_i, matrix_no));
                        row_to_var_non_resp[row_i] = p;

                        xor_reasons[row_i].must_recalc = true;
                        xor_reasons[row_i].propagated = ret_lit_prop;
                        assert(solver->value(ret_lit_prop.var()) == l_Undef);
                        prop_lit(gqd, row_i, ret_lit_prop);

                        update_cols_vals_set(ret_lit_prop);
                        gqd.ret = gauss_res::prop;

                        VERBOSE_PRINT("---> Satisfied XORs set for row: " << row_i);
                        satisfied_xors[row_i] = 1;
                        SLOW_DEBUG_DO(assert(check_row_satisfied(row_i)));
                        break;
                    }

                    // find new watch list
                    case gret::nothing_fnewwatch:
                        elim_ret_fnewwatch++;
                        #ifdef VERBOSE_DEBUG
                        cout
                        << "---> Nothing, clause NOT already satisfied, pushing in "
                        << new_non_resp_var+1 << " as non-responsible var ( "
                        << row_i << " row) "
                        << endl;
                        #endif

                        SLOW_DEBUG_DO(check_row_not_in_watch(new_non_resp_var, row_i));
                        solver->gwatches[new_non_resp_var].push(GaussWatched(row_i, matrix_no));
                        row_to_var_non_resp[row_i] = new_non_resp_var;
                        break;

                    // this row already satisfied
                    case gret::nothing_satisfied:
                        elim_ret_satisfied++;
                        VERBOSE_PRINT("---> Nothing to do, already satisfied , pushing in "
                        << p+1 << " as non-responsible var ( "
                        << row_i << " row) ");

                        // printf("%d:This row is nothing( maybe already true) in eliminate col
                        // n",num_row);

                        SLOW_DEBUG_DO(check_row_not_in_watch(p, row_i));
                        solver->gwatches[p].push(GaussWatched(row_i, matrix_no));
                        row_to_var_non_resp[row_i] = p;

                        VERBOSE_PRINT("---> Satisfied XORs set for row: " << row_i);
                        satisfied_xors[row_i] = 1;
                        SLOW_DEBUG_DO(assert(check_row_satisfied(row_i)));
                        break;
                    default:
                        // can not here
                        assert(false);
                        break;
                }
            } else {
                VERBOSE_PRINT("--> OK, this row " << row_i << " still contains '1', can still be responsible");
            }
        }
        ++rowI;
        row_i++;
    }

    // Debug_funtion();
    #ifdef VERBOSE_DEBUG
    cout
    << "mat[" << matrix_no << "] "
    << "eliminate_col - exiting. " << endl;
    #endif
}

void EGaussian::print_matrix() {
    uint32_t row = 0;
    for (PackedMatrix::iterator it = mat.begin(); it != mat.end();
         ++it, row++) {
        cout << *it << " -- row:" << row;
        if (row >= num_rows) {
            cout << " (considered past the end)";
        }
        cout << endl;
    }
}

void EGaussian::print_matrix_stats(uint32_t verbosity)
{
    std::stringstream ss;
    ss << "c [g " << matrix_no << "] ";
    const std::string pre = ss.str();

    cout << std::left;

    if (verbosity >= 2) {
        cout << pre << "truth-find satisfied    : "
        << print_value_kilo_mega(find_truth_ret_satisfied_precheck, false) << endl;
    }

    if (verbosity >= 1) {
        cout << pre << "truth-find prop checks  : "
        << print_value_kilo_mega(find_truth_called_propgause, false) << endl;
    }


    if (verbosity >= 2) {
        cout << pre << "-> of which fnnewat     : "
        << std::setw(5) << std::setprecision(2) << std::right
        << stats_line_percent(find_truth_ret_fnewwatch, find_truth_called_propgause)
        << " %"
        << endl;
        cout << pre << "-> of which sat         : "
        << std::setw(5) << std::setprecision(2) << std::right
        << stats_line_percent(find_truth_ret_satisfied, find_truth_called_propgause)
        << " %"
        << endl;
    }

    if (verbosity >= 1) {
        cout << pre << "-> of which prop        : "
        << std::setw(5) << std::setprecision(2) << std::right
        << stats_line_percent(find_truth_ret_prop, find_truth_called_propgause)
        << " %"
        << endl;
        cout << pre << "-> of which confl       : "
        << std::setw(5) << std::setprecision(2) << std::right
        << stats_line_percent(find_truth_ret_confl, find_truth_called_propgause)
        << " %"
        << endl;
    }

    cout << std::left;
    cout << pre << "elim called             : "
    << print_value_kilo_mega(elim_called, false) << endl;

    if (verbosity >= 2) {
        cout << pre << "-> lead to xor rows     : "
        << print_value_kilo_mega(elim_xored_rows, false) << endl;

        cout << pre << "--> lead to prop checks : "
        << print_value_kilo_mega(elim_called_propgause, false) << endl;

        cout << pre << "---> of which satsified : "
        << std::setw(5) << std::setprecision(2) << std::right
        << stats_line_percent(elim_ret_satisfied, elim_called_propgause)
        << " %"
        << endl;

        cout << pre << "---> of which prop      : "
        << std::setw(5) << std::setprecision(2) << std::right
        << stats_line_percent(elim_ret_prop, elim_called_propgause)
        << " %"
        << endl;

        cout << pre << "---> of which fnnewat   : "
        << std::setw(5) << std::setprecision(2) << std::right
        << stats_line_percent(elim_ret_fnewwatch, elim_called_propgause)
        << " %"
        << endl;

        cout << pre << "---> of which confl     : "
        << std::setw(5) << std::setprecision(2) << std::right
        << stats_line_percent(elim_ret_confl, elim_called_propgause)
        << " %"
        << endl;
    }

    if (verbosity == 1) {
        cout << pre << "---> which lead to prop : "
        << std::setw(5) << std::setprecision(2) << std::right
        << stats_line_percent(elim_ret_prop, elim_called)
        << " %"
        << endl;

        cout << pre << "---> which lead to confl: "
        << std::setw(5) << std::setprecision(2) << std::right
        << stats_line_percent(elim_ret_confl, elim_called)
        << " %"
        << endl;
    }

    cout << std::left;
    cout << pre << "size: "
    << std::setw(5) << num_rows << " x "
    << std::setw(5) << num_cols << endl;

    double density = get_density();

    if (verbosity >= 2) {
        cout << pre << "density before init: "
        << std::setprecision(4) << std::left << before_init_density
        << endl;
        cout << pre << "density after  init: "
        << std::setprecision(4) << std::left << after_init_density
        << endl;
        cout << pre << "density            : "
        << std::setprecision(4) << std::left << density
        << endl;
    }
    cout << std::setprecision(2);
}

//////////////////
// Checking functions below
//////////////////

void EGaussian::check_row_not_in_watch(const uint32_t v, const uint32_t row_num) const
{
    for(const auto& x: solver->gwatches[v]) {
        if (x.matrix_num == matrix_no && x.row_n == row_num) {
            cout << "OOOps, row ID " << row_num << " already in watch for var: " << v+1 << endl;
            assert(false);
        }
    }
}

void EGaussian::print_gwatches(const uint32_t var) const
{
    vec<GaussWatched> mycopy;
    for(const auto& x: solver->gwatches[var]) {
        mycopy.push(x);
    }

    std::sort(mycopy.begin(), mycopy.end());
    cout << "Watch for var " << var+1 << ": ";
    for(const auto& x: mycopy) {
        cout
        << "(Mat num: " << x.matrix_num
        << " row_n: " << x.row_n << ") ";
    }
    cout << endl;
}


void EGaussian::check_no_prop_or_unsat_rows()
{
    VERBOSE_PRINT("mat[" << matrix_no << "] checking invariants...");

    for(uint32_t row = 0; row < num_rows; row++) {
        uint32_t bits_unset = 0;
        bool val = mat[row].rhs();
        for(uint32_t col = 0; col < num_cols; col++) {
            if (mat[row][col]) {
                uint32_t var = col_to_var[col];
                if (solver->value(var) == l_Undef) {
                    bits_unset++;
                } else {
                    val ^= (solver->value(var) == l_True);
                }
            }
        }

        bool error = false;
        if (bits_unset == 1) {
            cout << "ERROR: row " << row << " is PROP but did not propagate!!!" << endl;
            error = true;
        }
        if (bits_unset == 0 && val != false) {
            cout << "ERROR: row " << row << " is UNSAT but did not conflict!" << endl;
            error = true;
        }
        if (error) {
            for(uint32_t var = 0; var < solver->nVars(); var++) {
                const auto& ws = solver->gwatches[var];
                for(const auto& w: ws) {
                    if (w.matrix_num == matrix_no && w.row_n == row) {
                        cout << "       gauss watched at var: " << var+1
                        << " val: " << solver->value(var) << endl;
                    }
                }
            }

            cout << "       matrix no: " << matrix_no << endl;
            cout << "       row: " << row << endl;
            uint32_t var = row_to_var_non_resp[row];
            cout << "       non-resp var: " << var+1 << endl;
            cout << "       dec level: " << solver->decisionLevel() << endl;
        }
        assert(bits_unset > 1 || (bits_unset == 0 && val == 0));
    }
}

void EGaussian::check_watchlist_sanity()
{
    for(size_t i = 0; i < solver->nVars(); i++) {
        for(auto w: solver->gwatches[i]) {
            if (w.matrix_num == matrix_no) assert(i < var_to_col.size());
        }
    }
}

void EGaussian::check_tracked_cols_only_one_set()
{
    vector<uint32_t> row_resp_for_var(num_rows, var_Undef);
    for(uint32_t col = 0; col < num_cols; col++) {
        uint32_t var = col_to_var[col];
        if (var_has_resp_row[var]) {
            uint32_t num_ones = 0;
            uint32_t found_row = var_Undef;
            for(uint32_t row = 0; row < num_rows; row++) {
                if (mat[row][col]) {
                    num_ones++;
                    found_row = row;
                }
            }
            if (num_ones == 0) {
                cout
                << "mat[" << matrix_no << "] "
                << "WARNING: Tracked col " << col
                << " var: " << var+1
                << " has 0 rows' bit set to 1..."
                << endl;
            }
            if (num_ones > 1) {
                cout
                << "mat[" << matrix_no << "] "
                << "ERROR: Tracked col " << col
                << " var: " << var+1
                << " has " << num_ones << " rows' bit set to 1!!"
                << endl;
                assert(num_ones <= 1);
            }
            if (num_ones == 1) {
                if (row_resp_for_var[found_row] != var_Undef) {
                    cout << "ERROR One row can only be responsible for one col"
                    << " but row " << found_row << " is responsible for"
                    << " var: " << row_resp_for_var[found_row]+1
                    << " and var: " << var+1
                    << endl;
                    assert(false);
                }
                row_resp_for_var[found_row] = var;
            }
        }
    }
}

void CMSat::EGaussian::check_invariants()
{
    if (!initialized) return;
    check_tracked_cols_only_one_set();
    check_no_prop_or_unsat_rows();
    VERBOSE_PRINT("mat[" << matrix_no << "] "
    << "Checked invariants. Dec level: " << solver->decisionLevel());
}

bool EGaussian::check_row_satisfied(const uint32_t row)
{
    bool ret = true;
    bool fin = mat[row].rhs();
    for(uint32_t i = 0; i < num_cols; i++) {
        if (mat[row][i]) {
            uint32_t var = col_to_var[i];
            auto val = solver->value(var);
            if (val == l_Undef) {
                cout << "Var " << var+1 << " col: " << i << " is undef!" << endl;
                ret = false;
            }
            fin ^= val == l_True;
        }
    }
    return ret && fin == false;
}

void EGaussian::check_cols_unset_vals()
{
    for(uint32_t i = 0; i < num_cols; i ++) {
        uint32_t var = col_to_var[i];
        if (solver->value(var) == l_Undef) {
            assert((*cols_unset)[i] == 1);
        } else {
            assert((*cols_unset)[i] == 0);
        }

        if (solver->value(var) == l_True) {
            assert((*cols_vals)[i] == 1);
        } else {
            assert((*cols_vals)[i] == 0);
        }
    }
}

bool EGaussian::must_disable(GaussQData& gqd)
{
    assert(initialized);
    gqd.engaus_disable_checks++;
    if ((gqd.engaus_disable_checks & 0x3ff) == 0x3ff //only check once in a while
    ) {
        uint64_t egcalled = elim_called + find_truth_ret_satisfied_precheck+find_truth_called_propgause;
        uint32_t limit = (double)egcalled*solver->conf.gaussconf.min_usefulness_cutoff;
        uint32_t useful = find_truth_ret_prop+find_truth_ret_confl+elim_ret_prop+elim_ret_confl;
        //cout << "CHECKING - limit: " << limit << " useful:" << useful << endl;
        if (egcalled > 200 && useful < limit) {
            if (solver->conf.verbosity) {
                const double perc =
                    stats_line_percent(useful, egcalled);
                cout << "c [g  <" <<  matrix_no <<  "] Disabling GJ-elim in this round. "
                " Usefulness was: "
                << std::setprecision(4) << std::fixed << perc
                <<  "%"
                << std::setprecision(2)
                << "  over " << egcalled << " calls"
                << endl;
            }
            return true;
        }
    }

    return false;
}

void CMSat::EGaussian::move_back_xor_clauses() {
    bool ok = solver->remove_and_clean_detached_xors(xorclauses);
    for(const auto& x: xorclauses) {
        solver->xorclauses.push_back(std::move(x));
        if (ok) solver->attach_xor_clause(solver->xorclauses.size()-1);
    }
}


void CMSat::EGaussian::delete_reasons() {
    frat_func_start();
    if (!solver->frat->enabled()) return;
    for(auto& c: xor_reasons) {
        if (c.ID != 0) *solver->frat << del << c.ID << c.reason << fin;
        c.ID = 0;
    }

    for(const auto& c: del_unit_cls) {
        *solver->frat << del << c.first << c.second << fin;
    }
    del_unit_cls.clear();
    frat_func_end();
}

void CMSat::EGaussian::finalize_frat() {
    assert(solver->frat->enabled());
    frat_func_start();
    for(const auto& c: del_unit_cls) {
        *solver->frat << finalcl << c.first << c.second << fin;
    }
    del_unit_cls.clear();

    for(auto& c: xor_reasons) {
        if (c.ID != 0) *solver->frat << finalcl << c.ID << c.reason << fin;
        c.ID = 0;
    }

    for(auto& x: xorclauses) {
        del_xor_reason(x);
        *solver->frat << finalx << x << fin;
        x.XID = 0;
    }
    frat_func_end();
}
