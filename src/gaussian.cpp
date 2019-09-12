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

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <set>

#include "gaussian.h"
#include "clause.h"
#include "clausecleaner.h"
#include "datasync.h"
#include "propby.h"
#include "solver.h"
#include "time_mem.h"
#include "varreplacer.h"
#include "xorfinder.h"

using std::cout;
using std::endl;
using std::ostream;
using std::set;

//#define VERBOSE_DEBUG

#ifdef VERBOSE_DEBUG
#include <iterator>
#endif

using namespace CMSat;

// if variable is not in Gaussian matrix , assiag unknown column
static const uint32_t unassigned_col = std::numeric_limits<uint32_t>::max();

EGaussian::EGaussian(
    Solver* _solver,
    const GaussConf& _config,
    const uint32_t _matrix_no,
    const vector<Xor>& _xorclauses) :
xorclauses(_xorclauses),
solver(_solver),
config(_config),
matrix_no(_matrix_no)

{
    vector<Xor> xors;
    for (Xor& x : xorclauses) {
        xors.push_back(x);
    }
    for (Xor& x : xors) {
        x.sort();
    }
    std::sort(xors.begin(), xors.end());
}

EGaussian::~EGaussian() {
    delete_gauss_watch_this_matrix();
    for(auto& x: tofree) {
        delete[] x;
    }
    tofree.clear();

    delete cols_unset;
    delete cols_vals;
    delete tmp_col;
    delete tmp_col2;
}

struct ColSorter {
    explicit ColSorter(Solver* _solver) :
        solver(_solver)
    {
        for(const auto& ass: solver->assumptions) {
            Lit p = solver->map_outer_to_inter(ass.lit_outer);
            assert(solver->seen.size() > p.var());
            solver->seen[p.var()] = 1;
        }
    }

    void finishup()
    {
        for(const auto& ass: solver->assumptions) {
            Lit p = solver->map_outer_to_inter(ass.lit_outer);
            solver->seen[p.var()] = 0;
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

        //return false;
        return solver->varData[a].level > solver->varData[b].level;
        //return solver->var_act_vsids[a] > solver->var_act_vsids[b];
    }

    Solver* solver;
};

uint32_t EGaussian::select_columnorder() {
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


    ColSorter c(solver);
    std::sort(vars_needed.begin(), vars_needed.end(),c);
    c.finishup();

    #ifdef VERBOSE_DEBUG
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

    return xorclauses.size();
}

void EGaussian::fill_matrix() {
    var_to_col.clear();

    // decide which variable in matrix column and the number of rows
    num_rows = select_columnorder();
    num_cols = col_to_var.size();
    if (num_rows == 0 || num_cols == 0) {
        return;
    }
    mat.resize(num_rows, num_cols); // initial gaussian matrix

    uint32_t matrix_row = 0;
    for (uint32_t i = 0; i != xorclauses.size(); i++) {
        const Xor& c = xorclauses[i];
        mat[matrix_row].set(c, var_to_col, num_cols);
        matrix_row++;
    }
    assert(num_rows == matrix_row);

    // reset
    var_has_resp_row.clear();
    var_has_resp_row.resize(solver->nVars(), 0);
    row_non_resp_for_var.clear();

    delete_gauss_watch_this_matrix();

    //reset satisfied_xor state
    assert(solver->decisionLevel() == 0);
    satisfied_xors.clear();
    satisfied_xors.resize(num_rows, 0);
}

void EGaussian::delete_gauss_watch_this_matrix()
{
    for (size_t ii = 0; ii < solver->gwatches.size(); ii++) {
        clear_gwatches(ii);
    }
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

bool EGaussian::clean_xors()
{
    for(Xor& x: xorclauses) {
        solver->clean_xor_vars_no_prop(x.get_vars(), x.rhs);
    }
    XorFinder f(NULL, solver);
    if (!f.add_new_truths_from_xors(xorclauses))
        return false;

    return true;
}

bool EGaussian::full_init(bool& created) {
    assert(solver->ok);
    assert(solver->decisionLevel() == 0);
    bool do_again_gauss = true;
    created = true;
    if (!clean_xors()) {
        return false;
    }

    while (do_again_gauss) {
        do_again_gauss = false;

        if (!solver->clauseCleaner->clean_xor_clauses(xorclauses)) {
            return false;
        }

        fill_matrix();
        before_init_density = get_density();
        if (num_rows == 0 || num_cols == 0) {
            created = false;
            return solver->okay();
        }

        eliminate();

        // find some row already true false, and insert watch list
        gret ret = adjust_matrix();

        switch (ret) {
            case gret::confl:
                solver->ok = false;
                return false;
                break;
            case gret::prop:
                do_again_gauss = true;

                assert(solver->decisionLevel() == 0);
                solver->ok = (solver->propagate<false>().isNULL());
                if (!solver->ok) {
                    return false;
                }
                break;
            default:
                break;
        }
    }

#ifdef SLOW_DEBUG
    check_watchlist_sanity();
#endif

    if (solver->conf.verbosity >= 2) {
        cout << "c [gauss] initialised matrix " << matrix_no << endl;
    }

    xor_reasons.resize(num_rows);
    uint32_t num_64b = num_cols/64+(bool)(num_cols%64);
    for(auto& x: tofree) {
        delete[] x;
    }
    tofree.clear();
    delete cols_unset;
    delete cols_vals;
    delete tmp_col;
    delete tmp_col2;

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

    cols_vals->rhs() = 0;
    cols_unset->rhs() = 0;
    tmp_col->rhs() = 0;
    tmp_col2->rhs() = 0;
    after_init_density = get_density();
    return true;
}

void EGaussian::check_watchlist_sanity()
{
    for(size_t i = 0; i < solver->nVars(); i++) {
        for(auto w: solver->gwatches[i]) {
            if (w.matrix_num == matrix_no) {
                assert(i < var_to_col.size());
            }
        }
    }
}

void EGaussian::eliminate() {
    uint32_t row = 0;
    uint32_t col = 0;
    PackedMatrix::iterator end_row_it = mat.begin() + num_rows;
    PackedMatrix::iterator row_i = mat.begin();

    // Gauss-Jordan Elimination
    while (row != num_rows && col != num_cols) {
        PackedMatrix::iterator row_with_1_in_col = row_i;

        //Find first "1" in column.
        for (; row_with_1_in_col != end_row_it; ++row_with_1_in_col) {
            if ((*row_with_1_in_col)[col]) {
                break;
            }
        }

        //We have found a "1" in this column
        if (row_with_1_in_col != end_row_it) {
            //cout << "col zeroed:" << col << " var is: " << col_to_var[col] + 1 << endl;
            var_has_resp_row[col_to_var[col]] = 1;

            // swap row row_with_1_in_col and rowIt
            if (row_with_1_in_col != row_i) {
                (*row_i).swapBoth(*row_with_1_in_col);
            }

            // XOR into *all* rows that have a "1" in column COL
            // Since we XOR into *all*, this is Gauss-Jordan (and not just Gauss)
            for (PackedMatrix::iterator k_row = mat.begin()
                ; k_row != end_row_it
                ; ++k_row
            ) {
                // xor rows K and I
                if (k_row != row_i) {
                    if ((*k_row)[col]) {
                        (*k_row).xor_in(*row_i);
                    }
                }
            }
            row++;
            ++row_i;
        }
        col++;
    }
    //print_matrix();
}

gret EGaussian::adjust_matrix() {
    assert(solver->decisionLevel() == 0);
    assert(row_non_resp_for_var.empty());
    assert(satisfied_xors.size() >= num_rows);

    PackedMatrix::iterator end = mat.begin() + num_rows;
    PackedMatrix::iterator rowIt = mat.begin();
    uint32_t row_id = 0;      // row index
    uint32_t adjust_zero = 0; //  elimination row

    while (rowIt != end) {
        uint32_t non_resp_var;
        const uint32_t popcnt = (*rowIt).find_watchVar(
            tmp_clause, col_to_var, var_has_resp_row, non_resp_var);

        switch (popcnt) {

            //Conflict or satisfied
            case 0:
                // printf("%d:Warning: this row is all zero in adjust matrix    n",row_id);
                adjust_zero++;

                // conflict
                if ((*rowIt).rhs()) {
                    // printf("%d:Warring: this row is conflict in adjust matrix!!!",row_id);
                    return gret::confl;
                }
                satisfied_xors[row_id] = 1;
                break;

            //Normal propagation
            case 1:
            {
                // printf("%d:This row only one variable, need to propogation!!!! in adjust matrix
                // n",row_id);

                bool xorEqualFalse = !mat[row_id].rhs();
                tmp_clause[0] = Lit(tmp_clause[0].var(), xorEqualFalse);
                assert(solver->value(tmp_clause[0].var()) == l_Undef);
                solver->enqueue(tmp_clause[0]); // propagation

                //adjusting
                (*rowIt).setZero(); // reset this row all zero
                row_non_resp_for_var.push_back(std::numeric_limits<uint32_t>::max());
                var_has_resp_row[tmp_clause[0].var()] = 0;
                return gret::prop;
            }

            //Binary XOR
            case 2: {
                // printf("%d:This row have two variable!!!! in adjust matrix    n",row_id);
                bool xorEqualFalse = !mat[row_id].rhs();

                tmp_clause[0] = tmp_clause[0].unsign();
                tmp_clause[1] = tmp_clause[1].unsign();
                solver->ok = solver->add_xor_clause_inter(tmp_clause, !xorEqualFalse, true);
                release_assert(solver->ok);

                (*rowIt).setZero(); // reset this row all zero
                row_non_resp_for_var.push_back(std::numeric_limits<uint32_t>::max()); // delete non-basic value in this row
                var_has_resp_row[tmp_clause[0].var()] = 0; // delete basic value in this row
                break;
            }

            default: // need to update watch list
                // printf("%d:need to update watch list    n",row_id);
                assert(non_resp_var != std::numeric_limits<uint32_t>::max());

                // insert watch list
                solver->gwatches[tmp_clause[0].var()].push(
                    GaussWatched(row_id, matrix_no)); // insert basic variable

                solver->gwatches[non_resp_var].push(
                    GaussWatched(row_id, matrix_no)); // insert non-basic variable
                row_non_resp_for_var.push_back(non_resp_var);               // record in this row non-basic variable
                break;
        }
        ++rowIt;
        row_id++;
    }
    // printf("DD:nb_rows:%d %d %d    n",nb_rows.size() ,   row_id - adjust_zero  ,  adjust_zero);
    assert(row_non_resp_for_var.size() == row_id - adjust_zero);

    mat.resizeNumRows(row_id - adjust_zero);
    num_rows = row_id - adjust_zero;

    // printf("DD: adjust number of Row:%d    n",num_row);
    // printf("dd:matrix by EGaussian::adjust_matrix    n");
    // print_matrix(m);
    // printf(" adjust_zero %d    n",adjust_zero);
    // printf("%d    t%d    t",num_rows , num_cols);
    return gret::nothing_satisfied;
}

// Delete this row because we have already add to xor clause, nothing to do anymore
void EGaussian::delete_gausswatch(
    const uint32_t row_n
) {
    // clear nonbasic value watch list
    bool debug_find = false;
    vec<GaussWatched>& ws_t = solver->gwatches[row_non_resp_for_var[row_n]];
    for (int32_t tmpi = ws_t.size() - 1; tmpi >= 0; tmpi--) {
        if (ws_t[tmpi].row_id == row_n
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
    << row_resp_for_var[row_n]+1 << endl;
    #endif
    assert(debug_find);
}

bool EGaussian::find_truths(
    GaussWatched*& i,
    GaussWatched*& j,
    uint32_t p,
    const uint32_t row_n,
    GaussQData& gqd
) {
    assert(gqd.ret != gauss_res::confl);
    // printf("dd Watch variable : %d  ,  Wathch row num %d    n", p , row_n);

    #ifdef VERBOSE_DEBUG
    cout << "row_n:" << row_n << endl;
    cout << "dec lev:" << solver->decisionLevel() << endl;
    cout << "satisfied_xors.size(): " << satisfied_xors.size() << endl;
    #endif
    #ifdef SLOW_DEBUG
    assert(row_n < num_rows);
    assert(satisfied_xors.size() > row_n);
    #endif

    // this XOR is already satisfied
    if (satisfied_xors[row_n]) {
        *j++ = *i;
        find_truth_ret_satisfied_precheck++;
        return true;
    }

    bool p_was_resp_var = false; // check invoked variable is basic or non-basic

    // swap basic and non-basic variable
    if (var_has_resp_row[p] == 1) {
        p_was_resp_var = true;
        var_has_resp_row[row_non_resp_for_var[row_n]] = 1;
        var_has_resp_row[p] = 0;
    }

    uint32_t new_non_resp_var;
    Lit ret_lit_prop;
    const gret ret = mat[row_n].propGause(
        solver->assigns,
        col_to_var,
        var_has_resp_row,
        new_non_resp_var,
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
            #ifdef VERBOSE_DEBUG
            cout
            << "mat[" << matrix_no << "] "
            << "find_truths - Gauss long conf " << endl;
            #endif

            if (p_was_resp_var) { // recover
                var_has_resp_row[row_non_resp_for_var[row_n]] = 0;
                var_has_resp_row[p] = 1;
            }

            return false;
        }

        case gret::prop: {
            find_truth_ret_prop++;
            // printf("%d:This row is propagation : level: %d    n",row_n, solver->level[p]);
            *j++ = *i;


            xor_reasons[row_n].must_recalc = true;
            xor_reasons[row_n].propagated = ret_lit_prop;
            assert(solver->value(ret_lit_prop.var()) == l_Undef);
            solver->enqueue(ret_lit_prop, PropBy(matrix_no, row_n));
            update_cols_vals_set(ret_lit_prop);
            gqd.ret = gauss_res::prop;
            #ifdef VERBOSE_DEBUG
            cout
            << "mat[" << matrix_no << "] "
            << "find_truths - Gauss prop "
            << " tmp_clause.size: " << tmp_clause.size() << endl;
            #endif

            if (p_was_resp_var) { // recover
                var_has_resp_row[row_non_resp_for_var[row_n]] = 0;
                var_has_resp_row[p] = 1;
            }

            satisfied_xors[row_n] = 1;
            return true;
        }

        // find new watch list
        case gret::nothing_fnewwatch:
            find_truth_ret_fnewwatch++;
            // printf("%d:This row is find new watch:%d => orig %d p:%d    n",row_n ,
            // new_resp_var,orig_basic , p);
            if (p_was_resp_var) {
                /// clear watchlist, because only one basic value in watchlist
                assert(new_non_resp_var != p);
                clear_gwatches(new_non_resp_var);
            }
            assert(new_non_resp_var != p);
            solver->gwatches[new_non_resp_var].push(GaussWatched(row_n, matrix_no));

            if (!p_was_resp_var) {
                row_non_resp_for_var[row_n] = new_non_resp_var;
                return true;
            }

            // adjust resp and non-resp vars
            var_has_resp_row[row_non_resp_for_var[row_n]] = 0;
            var_has_resp_row[new_non_resp_var] = 1;

            // store the eliminate variable & row
            gqd.new_resp_var = new_non_resp_var;
            gqd.new_resp_row = row_n;
            if (solver->gmatrices.size() == 1) {
                assert(solver->gwatches[gqd.new_resp_var].size() == 1);
            }
            gqd.do_eliminate = true;
            return true;

        // this row already true
        case gret::nothing_satisfied:
            find_truth_ret_satisfied++;
            // printf("%d:This row is nothing( maybe already true)     n",row_n);
            *j++ = *i;
            if (p_was_resp_var) { // recover
                var_has_resp_row[row_non_resp_for_var[row_n]] = 0;
                var_has_resp_row[p] = 1;
            }
            satisfied_xors[row_n] = 1;
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

void EGaussian::update_cols_vals_set()
{
    //cancelled_since_val_update = true;
    if (cancelled_since_val_update) {
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
        uint32_t var = solver->trail[i].var();
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

void EGaussian::eliminate_col(uint32_t p, GaussQData& gqd) {
    PackedMatrix::iterator new_resp_row = mat.begin() + gqd.new_resp_row;
    PackedMatrix::iterator rowI = mat.begin();
    PackedMatrix::iterator end = mat.end();
    uint32_t new_resp_col = var_to_col[gqd.new_resp_var];
    uint32_t orig_non_resp_var = 0;
    uint32_t orig_non_resp_col = 0;
    uint32_t row_n = 0;

    #ifdef VERBOSE_DEBUG
    cout
    << "mat[" << matrix_no << "] "
    << "** eliminate this var's column: " << gqd.e_var+1
    << " p: " << p+1
    << " col: " << e_col
    << " e-row: " << gqd.e_row_n
    << " ***"
    <<  endl;
    #endif
    elim_called++;

    while (rowI != end) {
        //Row has a '1' in eliminating column, and it's not the row responsible
        if (new_resp_row != rowI && (*rowI)[new_resp_col]) {

            // detect orignal non-basic watch list change or not
            orig_non_resp_var = row_non_resp_for_var[row_n];
            orig_non_resp_col = var_to_col[orig_non_resp_var];
            assert((*rowI)[orig_non_resp_col]);
            #ifdef VERBOSE_DEBUG
            cout
            << "mat[" << matrix_no << "] "
            << "This row " << num_row << " is responsible for var: " << orig_resp_var + 1
            << " i.e. it contains '1' for this var's column"
            << endl;
            #endif

            assert(satisfied_xors[row_n] == 0);
            (*rowI).xor_in(*new_resp_row);
            elim_xored_rows++;

            //NOTE: basic variable cannot be eliminated
            // orignal non-basic value is eliminated
            if (!(*rowI)[orig_non_resp_col]) {

                #ifdef VERBOSE_DEBUG
                cout
                << "mat[" << matrix_no << "] "
                << "-> This row " << num_row << " can no longer be responsible, has no '1', "
                << "fixing up..."<< endl;
                #endif

                // Delelte orignal non-basic value in watch list
                if (orig_non_resp_var != gqd.new_resp_var) {
                    delete_gausswatch(row_n);
                }

                Lit ret_lit_prop;
                uint32_t new_non_resp_var = 0;
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
                        #ifdef VERBOSE_DEBUG
                        cout
                        << "mat[" << matrix_no << "] "
                        << "-> conflict during fixup"<< endl;
                        #endif

                        solver->gwatches[p].push(
                            GaussWatched(row_n, matrix_no));

                        // update in this row non-basic variable
                        row_non_resp_for_var[row_n] = p;

                        xor_reasons[row_n].must_recalc = true;
                        xor_reasons[row_n].propagated = lit_Undef;
                        gqd.confl = PropBy(matrix_no, row_n);
                        gqd.ret = gauss_res::confl;
                        break;
                    }
                    case gret::prop: {
                        elim_ret_prop++;
                        #ifdef VERBOSE_DEBUG
                        cout
                        << "mat[" << matrix_no << "] "
                        << "-> propagation during fixup" << endl;
                        #endif

                        // if conflicted already, just update non-basic variable
                        if (gqd.ret == gauss_res::confl) {
                            solver->gwatches[p].push(GaussWatched(row_n, matrix_no));
                            row_non_resp_for_var[row_n] = p;
                            break;
                        }

                        // update no_basic information
                        solver->gwatches[p].push(GaussWatched(row_n, matrix_no));
                        row_non_resp_for_var[row_n] = p;

                        xor_reasons[row_n].must_recalc = true;
                        xor_reasons[row_n].propagated = ret_lit_prop;
                        assert(solver->value(ret_lit_prop.var()) == l_Undef);
                        solver->enqueue(ret_lit_prop, PropBy(matrix_no, row_n));
                        update_cols_vals_set(ret_lit_prop);

                        #ifdef VERBOSE_DEBUG
                        cout
                        << "mat[" << matrix_no << "] "
                        << "-> Long prop"  << endl;
                        #endif
                        gqd.ret = gauss_res::prop;
                        satisfied_xors[row_n] = 1;
                        break;
                    }

                    // find new watch list
                    case gret::nothing_fnewwatch:
                        elim_ret_fnewwatch++;
                        #ifdef VERBOSE_DEBUG
                        cout
                        << "mat[" << matrix_no << "] "
                        << "-> Nothing, clause NOT already satisfied, pushing in "
                        << new_resp_var+1 << " as responsible var ( "
                        << num_row << " row) "
                        << endl;
                        #endif

                        solver->gwatches[new_non_resp_var].push(GaussWatched(row_n, matrix_no));
                        row_non_resp_for_var[row_n] = new_non_resp_var;
                        break;

                    // this row already satisfied
                    case gret::nothing_satisfied:
                        elim_ret_satisfied++;
                        #ifdef VERBOSE_DEBUG
                        cout
                        << "mat[" << matrix_no << "] "
                        << "-> Nothing to do, already satisfied , pushing in "
                        << p+1 << " as responsible var ( "
                        << num_row << " row) "
                        << endl;
                        #endif

                        // printf("%d:This row is nothing( maybe already true) in eliminate col
                        // n",num_row);

                        solver->gwatches[p].push(GaussWatched(row_n, matrix_no));
                        row_non_resp_for_var[row_n] = p;
                        satisfied_xors[row_n] = 1;
                        break;
                    default:
                        // can not here
                        assert(false);
                        break;
                }
            } else {
                #ifdef VERBOSE_DEBUG
                cout
                << "mat[" << matrix_no << "] "
                << "-> OK, this row " << num_row
                << " still contains '1', can still be responsible" << endl;
                #endif
            }
        }
        ++rowI;
        row_n++;
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

void EGaussian::print_matrix_stats()
{
    std::stringstream ss;
    ss << "c [g " << matrix_no << "] ";
    const std::string pre = ss.str();

    cout << std::left;

    cout << pre << "truth-find satisfied    : "
    << print_value_kilo_mega(find_truth_ret_satisfied_precheck, false) << endl;

    cout << pre << "truth-find prop checks  : "
    << print_value_kilo_mega(find_truth_called_propgause, false) << endl;

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

    cout << std::left;
    cout << pre << "elim called             : "
    << print_value_kilo_mega(elim_called, false) << endl;
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
    cout << std::left;

    cout << pre << "size: "
    << std::setw(5) << num_rows << " x "
    << std::setw(5) << num_cols << endl;

    double density = get_density();

    cout << pre << "density before init: "
    << std::setprecision(4) << std::left << before_init_density
    << endl;
    cout << pre << "density after  init: "
    << std::setprecision(4) << std::left << after_init_density
    << endl;
    cout << pre << "density            : "
    << std::setprecision(4) << std::left << density
    << endl;
    cout << std::setprecision(2);
}

vector<Lit>* EGaussian::get_reason(uint32_t row)
{
    if (!xor_reasons[row].must_recalc) {
        return &(xor_reasons[row].reason);
    }
    vector<Lit>& tofill = xor_reasons[row].reason;
    tofill.clear();

    mat[row].get_reason(
        tofill,
        solver->assigns,
        col_to_var,
        xor_reasons[row].propagated);

    xor_reasons[row].must_recalc = false;
    return &tofill;
}
