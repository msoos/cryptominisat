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

EGaussian::EGaussian(Solver* _solver, const GaussConf& _config, const uint32_t _matrix_no,
                     const vector<Xor>& _xorclauses)
    : solver(_solver), config(_config), matrix_no(_matrix_no), xorclauses(_xorclauses) {
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
    for (uint32_t i = 0; i < clauses_toclear.size(); i++) {
        solver->free_cl(clauses_toclear[i].first);
    }
}

void EGaussian::canceling(const uint32_t sublevel) {
    uint32_t a = 0;
    for (int32_t i = (int32_t)clauses_toclear.size() - 1; i >= 0 && clauses_toclear[i].second > sublevel; i--) {
        solver->free_cl(clauses_toclear[i].first);
        a++;
    }
    clauses_toclear.resize(clauses_toclear.size() - a);
}

struct ColSorter {
    explicit ColSorter(vector<VarData>& _dats) : dats(_dats) {
    }

    // higher activity first
    bool operator()(uint32_t a, uint32_t b) {
        return dats[a].set > dats[b].set;
    }

    const vector<VarData>& dats;
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

    col_to_var.clear();
    std::sort(vars_needed.begin(), vars_needed.end(),
              ColSorter(solver->varData));

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
        mat.get_row(matrix_row).set(c, var_to_col, num_cols);
        matrix_row++;
    }
    assert(num_rows == matrix_row);

    // reset
    var_has_responsible_row.clear();
    var_has_responsible_row.resize(solver->nVars(), 0);
    row_responsible_for_var.clear();

    delete_gauss_watch_this_matrix();

    //forget clause state
    assert(solver->decisionLevel() == 0);
    if (satisfied_xors.size() < 1) {
        satisfied_xors.resize(1);
        satisfied_xors[0].clear();
        satisfied_xors[0].resize(num_rows, 0);
    }
}

void EGaussian::new_decision_level()
{
    assert(solver->decisionLevel() > 0);
    if (satisfied_xors.size() < solver->decisionLevel()+1) {
        satisfied_xors.resize(solver->decisionLevel()+1);
    }
    satisfied_xors[solver->decisionLevel()] = satisfied_xors[solver->decisionLevel()-1];
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

    while (do_again_gauss) { // need to chekc
        do_again_gauss = false;

        if (!solver->clauseCleaner->clean_xor_clauses(xorclauses)) {
            return false;
        }

        fill_matrix();
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
                solver->sum_gauss_confl++;
                return false;
                break;
            case gret::prop:
            case gret::unit_prop:
                do_again_gauss = true;
                solver->sum_gauss_prop++;

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

    // std::cout << cpuTime() - GaussConstructTime << "    t";
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
    PackedMatrix::iterator end_row_it = mat.beginMatrix() + num_rows;
    PackedMatrix::iterator row_i = mat.beginMatrix();

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
            var_has_responsible_row[col_to_var[col]] = 1;

            // swap row row_with_1_in_col and rowIt
            if (row_with_1_in_col != row_i) {
                (*row_i).swapBoth(*row_with_1_in_col);
            }

            // XOR into *all* rows that have a "1" in column COL
            // Since we XOR into *all*, this is Gauss-Jordan (and not just Gauss)
            for (PackedMatrix::iterator k_row = mat.beginMatrix()
                ; k_row != end_row_it
                ; ++k_row
            ) {
                // xor rows K and I
                if (k_row != row_i) {
                    if ((*k_row)[col]) {
                        (*k_row).xorBoth(*row_i);
                    }
                }
            }
            row++;
            ++row_i;
        }
        col++;
    }
    print_matrix();
}

gret EGaussian::adjust_matrix() {
    assert(solver->decisionLevel() == 0);
    assert(row_responsible_for_var.empty());
    assert(satisfied_xors.size() > 0);
    assert(satisfied_xors[0].size() >= num_rows);

    PackedMatrix::iterator end = mat.beginMatrix() + num_rows;
    PackedMatrix::iterator rowIt = mat.beginMatrix();
    uint32_t row_id = 0;      // row index
    uint32_t adjust_zero = 0; //  elimination row

    while (rowIt != end) {
        uint32_t resp_var;
        const uint32_t popcnt = (*rowIt).find_watchVar(
            tmp_clause, col_to_var, var_has_responsible_row, resp_var);

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
                satisfied_xors[0][row_id] = 1;
                break;

            //Normal propagation
            case 1:
            {
                // printf("%d:This row only one variable, need to propogation!!!! in adjust matrix
                // n",row_id);

                bool xorEqualFalse = !mat.get_row(row_id).rhs();
                tmp_clause[0] = Lit(tmp_clause[0].var(), xorEqualFalse);
                assert(solver->value(tmp_clause[0].var()) == l_Undef);
                solver->enqueue(tmp_clause[0]); // propagation

                //adjusting
                (*rowIt).setZero(); // reset this row all zero
                row_responsible_for_var.push_back(std::numeric_limits<uint32_t>::max());
                var_has_responsible_row[tmp_clause[0].var()] = 0;
                return gret::unit_prop;
            }

            //Binary XOR
            case 2: {
                // printf("%d:This row have two variable!!!! in adjust matrix    n",row_id);
                bool xorEqualFalse = !mat.get_row(row_id).rhs();

                tmp_clause[0] = tmp_clause[0].unsign();
                tmp_clause[1] = tmp_clause[1].unsign();
                solver->ok = solver->add_xor_clause_inter(tmp_clause, !xorEqualFalse, true);
                release_assert(solver->ok);

                (*rowIt).setZero(); // reset this row all zero
                row_responsible_for_var.push_back(std::numeric_limits<uint32_t>::max()); // delete non basic value in this row
                var_has_responsible_row[tmp_clause[0].var()] = 0; // delete basic value in this row
                break;
            }

            default: // need to update watch list
                // printf("%d:need to update watch list    n",row_id);
                assert(resp_var != std::numeric_limits<uint32_t>::max());

                // insert watch list
                solver->gwatches[tmp_clause[0].var()].push(
                    GaussWatched(row_id, matrix_no)); // insert basic variable

                solver->gwatches[resp_var].push(
                    GaussWatched(row_id, matrix_no)); // insert non-basic variable
                row_responsible_for_var.push_back(resp_var);               // record in this row non_basic variable
                break;
        }
        ++rowIt;
        row_id++;
    }
    // printf("DD:nb_rows:%d %d %d    n",nb_rows.size() ,   row_id - adjust_zero  ,  adjust_zero);
    assert(row_responsible_for_var.size() == row_id - adjust_zero);

    mat.resizeNumRows(row_id - adjust_zero);
    num_rows = row_id - adjust_zero;

    // printf("DD: adjust number of Row:%d    n",num_row);
    // printf("dd:matrix by EGaussian::adjust_matrix    n");
    // print_matrix(m);
    // printf(" adjust_zero %d    n",adjust_zero);
    // printf("%d    t%d    t",num_rows , num_cols);
    return gret::nothing;
}

inline void EGaussian::propagation_twoclause() {
    // printf("DD:%d %d    n", solver->qhead  ,solver->trail.size());
    // printf("CC %d. %d  %d    n", solver->qhead , solver->trail.size() , solver->decisionLevel());

    Lit lit1 = tmp_clause[0];
    Lit lit2 = tmp_clause[1];
    solver->attach_bin_clause(lit1, lit2, true, false);
    // solver->dataSync->signalNewBinClause(lit1, lit2);

    lit1 = ~lit1;
    lit2 = ~lit2;
    solver->attach_bin_clause(lit1, lit2, true, false);
    // solver->dataSync->signalNewBinClause(lit1, lit2);

    lit1 = ~lit1;
    lit2 = ~lit2;
    solver->enqueue(lit1, PropBy(lit2, true));
}

inline void EGaussian::conflict_twoclause(PropBy& confl) {
    // assert(tmp_clause.size() == 2);
    // printf("dd %d:This row is conflict two    n",row_n);
    Lit lit1 = tmp_clause[0];
    Lit lit2 = tmp_clause[1];

#if 0
    cout << "conflict twoclause: " << lit1 << " " << lit2
    << " vals: " << solver->value(lit1) << " " << solver->value(lit2)
    << " levels: " << solver->varData[lit1.var()].level << " " << solver->varData[lit2.var()].level
    << " declevel: " << solver->decisionLevel()
    << endl;
#endif

    solver->attach_bin_clause(lit1, lit2, true, false);
    // solver->dataSync->signalNewBinClause(lit1, lit2);

    lit1 = ~lit1;
    lit2 = ~lit2;
    solver->attach_bin_clause(lit1, lit2, true, false);
    // solver->dataSync->signalNewBinClause(lit1, lit2);

    lit1 = ~lit1;
    lit2 = ~lit2;
    confl = PropBy(lit1, true);
    solver->failBinLit = lit2;
}

// Delete this row because we have already add to xor clause, nothing to do anymore
void EGaussian::delete_gausswatch(
    const bool orig_basic
    , const uint32_t row_n
    , const uint32_t no_touch_var
) {
    if (orig_basic) {
        // clear nonbasic value watch list
        bool debug_find = false;
        assert(row_responsible_for_var[row_n] != no_touch_var);
        vec<GaussWatched>& ws_t = solver->gwatches[row_responsible_for_var[row_n]];
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
        << row_responsible_for_var[row_n]+1 << endl;
        #endif
        assert(debug_find);
    } else {
        clear_gwatches(tmp_clause[0].var());
    }
}

bool EGaussian::find_truths2(
    GaussWatched*& i,
    GaussWatched*& j,
    uint32_t p,
    const uint32_t row_n,
    GaussQData& gqd
) {
    assert(gqd.ret != gauss_res::long_confl &&
    gqd.ret != gauss_res::bin_confl);

    // printf("dd Watch variable : %d  ,  Wathch row num %d    n", p , row_n);

    uint32_t new_resp_var = 0;
    bool orig_basic = false; // check invoked variable is basic or non-basic

    gqd.e_var = std::numeric_limits<uint32_t>::max();
    gqd.e_row_n = std::numeric_limits<uint32_t>::max();
    gqd.do_eliminate = false;

    PackedMatrix::iterator rowIt = mat.beginMatrix() + row_n;

    //if this clause is already satisfied
    if (satisfied_xors[solver->decisionLevel()][row_n]) {
        *j++ = *i;
        return true;
    }

    //TODO if the two watched things didn't get knocked out and they are both UNDEF
    //then we can return here.
    /*uint32_t var2 = row_responsible_for_var[row_n];
    if ((*rowIt)[var_to_col[var2]] &&
        solver->value(var2) == l_Undef
    ) {
        *j++ = *i;
        return true;
    }*/

    //swap basic and non_basic variable
    if (var_has_responsible_row[p] == 1) {
        orig_basic = true;
        var_has_responsible_row[row_responsible_for_var[row_n]] = 1;
        var_has_responsible_row[p] = 0;
    }

    const gret ret = (*rowIt).propGause(
        tmp_clause,
        solver->assigns,
        col_to_var,
        var_has_responsible_row,
        new_resp_var,
        0);
    propg_called_from_find_truth++;

    switch (ret) {
        case gret::confl: {
            // binary conflict
            if (tmp_clause.size() == 2) {
                // printf("%d:This row is conflict two    n",row_n);
                //WARNING !!!! if orig_basic is FALSE, this will delete
                delete_gausswatch(orig_basic, row_n, p);

                var_has_responsible_row[tmp_clause[0].var()] = 0; // delete value state;
                var_has_responsible_row[tmp_clause[1].var()] = 0;
                row_responsible_for_var[row_n] =
                    std::numeric_limits<uint32_t>::max(); // delete non basic value in this row
                (*rowIt).setZero();

                conflict_twoclause(gqd.confl);

                gqd.ret = gauss_res::bin_confl;
                #ifdef VERBOSE_DEBUG
                cout
                << "mat[" << matrix_no << "] "
                << "find_truths2 - Gauss binary conf " << endl;
                #endif
                return false;
            }

            // long conflict clause
            *j++ = *i;
            gqd.conflict_clause_gauss = tmp_clause;
            gqd.ret = gauss_res::long_confl;
            #ifdef VERBOSE_DEBUG
            cout
            << "mat[" << matrix_no << "] "
            << "find_truths2 - Gauss long conf " << endl;
            #endif

            if (orig_basic) { // recover
                var_has_responsible_row[row_responsible_for_var[row_n]] = 0;
                var_has_responsible_row[p] = 1;
            }

            return false;
        }

        case gret::prop: {
            // printf("%d:This row is propagation : level: %d    n",row_n, solver->level[p]);
            *j++ = *i;

            if (tmp_clause.size() == 2) {
                propagation_twoclause();
            } else {
                Clause* cla = solver->cl_alloc.Clause_new(
                    tmp_clause,
                    solver->sumConflicts
                    #ifdef STATS_NEEDED
                    , solver->clauseID++
                    #endif
                );
                cla->set_gauss_temp_cl();
                const ClOffset offs = solver->cl_alloc.get_offset(cla);
                clauses_toclear.push_back(std::make_pair(offs, solver->trail.size() - 1));
                assert(solver->value((*cla)[0].var()) == l_Undef);
                solver->enqueue((*cla)[0], PropBy(offs));
            }
            gqd.ret = gauss_res::prop;
            #ifdef VERBOSE_DEBUG
            cout
            << "mat[" << matrix_no << "] "
            << "find_truths2 - Gauss prop "
            << " tmp_clause.size: " << tmp_clause.size() << endl;
            #endif

            if (orig_basic) { // recover
                var_has_responsible_row[row_responsible_for_var[row_n]] = 0;
                var_has_responsible_row[p] = 1;
            }

            satisfied_xors[solver->decisionLevel()][row_n] = 1;
            return true;
        }

        // find new watch list
        case gret::nothing_fnewwatch:
            propg_called_from_find_truth_ret_fnewwatch++;
            // printf("%d:This row is find new watch:%d => orig %d p:%d    n",row_n ,
            // new_resp_var,orig_basic , p);
            assert(new_resp_var != std::numeric_limits<uint32_t>::max());
            if (orig_basic) {
                /// clear watchlist, because only one basic value in watchlist
                assert(new_resp_var != p);
                clear_gwatches(new_resp_var);
            }
            assert(new_resp_var != p);
            solver->gwatches[new_resp_var].push(GaussWatched(row_n, matrix_no));

            if (!orig_basic) {
                row_responsible_for_var[row_n] = new_resp_var;
                return true;
            }

            // recover non_basic variable
            var_has_responsible_row[row_responsible_for_var[row_n]] = 0;

            // set basic variable
            var_has_responsible_row[new_resp_var] = 1;

            // store the eliminate variable & row
            gqd.e_var = new_resp_var;
            gqd.e_row_n = row_n;
            break;

        // this row already true
        case gret::nothing:
            // printf("%d:This row is nothing( maybe already true)     n",row_n);
            *j++ = *i;
            if (orig_basic) { // recover
                var_has_responsible_row[row_responsible_for_var[row_n]] = 0;
                var_has_responsible_row[p] = 1;
            }
            satisfied_xors[solver->decisionLevel()][row_n] = 1;
            return true;

        //error here
        default:
            assert(false); // cannot be here
            break;
    }
    /*     assert(e_var != std::numeric_limits<uint32_t>::max());
        assert(e_row_n != std::numeric_limits<uint32_t>::max());
        assert(orig_basic);
        assert(ret == 5 );*/
    if (solver->gmatrices.size() == 1) {
        assert(solver->gwatches[gqd.e_var].size() == 1 && "Not sure about this assert");
    }
    gqd.do_eliminate = true;
    return true;
}

void EGaussian::eliminate_col2(uint32_t p, GaussQData& gqd) {
    PackedMatrix::iterator this_row = mat.beginMatrix() + gqd.e_row_n;
    PackedMatrix::iterator rowI = mat.beginMatrix();
    PackedMatrix::iterator end = mat.endMatrix();
    uint32_t e_col = var_to_col[gqd.e_var];
    uint32_t orig_resp_var = 0;
    uint32_t orig_resp_col = 0;
    uint32_t new_resp_var = 0;
    uint32_t num_row = 0; // row inde

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
    eliminate_col_called++;

    while (rowI != end) {
        //Row has a '1' in eliminating column, and it's not the row responsible
        if ((*rowI)[e_col] && this_row != rowI) {

            // detect orignal non basic watch list change or not
            orig_resp_var = row_responsible_for_var[num_row];
            orig_resp_col = var_to_col[orig_resp_var];
            assert((*rowI)[orig_resp_col]);
            #ifdef VERBOSE_DEBUG
            cout
            << "mat[" << matrix_no << "] "
            << "This row " << num_row << " is responsible for var: " << orig_resp_var + 1
            << " i.e. it contains '1' for this var's column"
            << endl;
            #endif

            assert(satisfied_xors[solver->decisionLevel()][num_row] == 0);
            (*rowI).xorBoth(*this_row);
            elim_xored_rows++;
            if (!(*rowI)[orig_resp_col]) { // orignal non basic value is eliminated
                #ifdef VERBOSE_DEBUG
                cout
                << "mat[" << matrix_no << "] "
                << "-> This row " << num_row << " can no longer be responsible, has no '1', "
                << "fixing up..."<< endl;
                #endif

                // Delelte orignal non basic value in watch list
                if (orig_resp_var != gqd.e_var) {
                    delete_gausswatch(true, num_row);
                }

                const gret ret = (*rowI).propGause(
                    tmp_clause,
                    solver->assigns,
                    col_to_var,
                    var_has_responsible_row,
                    new_resp_var,
                    0); //std::min(e_col, orig_resp_col));
                propg_called_from_elim++;

                switch (ret) {
                    case gret::confl: {
                        #ifdef VERBOSE_DEBUG
                        cout
                        << "mat[" << matrix_no << "] "
                        << "-> conflict during fixup"<< endl;
                        #endif

                        if (tmp_clause.size() == 2) {
                            delete_gausswatch(false, num_row);
                            assert(var_has_responsible_row[tmp_clause[0].var()] == 1);
                            assert(var_has_responsible_row[tmp_clause[1].var()] == 0);

                            // delete value state
                            var_has_responsible_row[tmp_clause[0].var()] = 0;

                            // delete non basic value in this row
                            row_responsible_for_var[num_row] = std::numeric_limits<uint32_t>::max();
                            (*rowI).setZero();

                            conflict_twoclause(gqd.confl);

                            gqd.ret = gauss_res::bin_confl;
                            #ifdef VERBOSE_DEBUG
                            cout
                            << "mat[" << matrix_no << "] "
                            << "-> Gauss bin confl matrix " << matrix_no
                            << endl;
                            #endif

                            break;

                        } else {
                            solver->gwatches[p].push(
                                GaussWatched(num_row, matrix_no));

                            // update in this row non_basic variable
                            row_responsible_for_var[num_row] = p;

                            gqd.conflict_clause_gauss = tmp_clause;
                            gqd.ret = gauss_res::long_confl;
                            #ifdef VERBOSE_DEBUG
                            cout
                            << "mat[" << matrix_no << "] "
                            << "-> Gauss long confl"
                            << endl;
                            #endif

                            break;
                        }
                        break;
                    }
                    case gret::prop: {
                        #ifdef VERBOSE_DEBUG
                        cout
                        << "mat[" << matrix_no << "] "
                        << "-> propagation during fixup" << endl;
                        #endif

                        // if conflicted already, just update non_basic variable
                        if (gqd.ret == gauss_res::bin_confl ||
                            gqd.ret == gauss_res::long_confl
                        ) {
                            solver->gwatches[p].push(GaussWatched(num_row, matrix_no));
                            row_responsible_for_var[num_row] = p;
                            break;
                        }

                        // update no_basic information
                        solver->gwatches[p].push(GaussWatched(num_row, matrix_no));
                        row_responsible_for_var[num_row] = p;

                        if (tmp_clause.size() == 2) {
                            propagation_twoclause();
                            #ifdef VERBOSE_DEBUG
                            cout
                            << "mat[" << matrix_no << "] "
                            << "-> Binary prop" << endl;
                            #endif
                        } else {
                            Clause* cla = solver->cl_alloc.Clause_new(
                                tmp_clause,
                                solver->sumConflicts
                                #ifdef STATS_NEEDED
                                , solver->clauseID++
                                #endif
                            );
                            cla->set_gauss_temp_cl();
                            const ClOffset offs = solver->cl_alloc.get_offset(cla);
                            clauses_toclear.push_back(std::make_pair(offs, solver->trail.size() - 1));
                            assert(solver->value((*cla)[0].var()) == l_Undef);
                            solver->enqueue((*cla)[0], PropBy(offs));
                            #ifdef VERBOSE_DEBUG
                            cout
                            << "mat[" << matrix_no << "] "
                            << "-> Long prop"  << endl;
                            #endif
                        }
                        gqd.ret = gauss_res::prop;
                        satisfied_xors[solver->decisionLevel()][num_row] = 1;
                        break;
                    }
                    case gret::nothing_fnewwatch: // find new watch list
                        #ifdef VERBOSE_DEBUG
                        cout
                        << "mat[" << matrix_no << "] "
                        << "-> Nothing, clause NOT already satisfied, pushing in "
                        << new_resp_var+1 << " as responsible var ( "
                        << num_row << " row) "
                        << endl;
                        #endif

                        solver->gwatches[new_resp_var].push(GaussWatched(num_row, matrix_no));
                        row_responsible_for_var[num_row] = new_resp_var;
                        break;
                    case gret::nothing: // this row already satisfied
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

                        solver->gwatches[p].push(GaussWatched(num_row, matrix_no));
                        row_responsible_for_var[num_row] = p; // update in this row non_basic variable
                        satisfied_xors[solver->decisionLevel()][num_row] = 1;
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
        num_row++;
    }

    // check_xor_reason_clauses_not_cleared();
    // Debug_funtion();
    #ifdef VERBOSE_DEBUG
    cout
    << "mat[" << matrix_no << "] "
    << "eliminate_col2 - exiting. " << endl;
    #endif
}

void EGaussian::print_matrix() {
    uint32_t row = 0;
    for (PackedMatrix::iterator it = mat.beginMatrix(); it != mat.endMatrix();
         ++it, row++) {
        cout << *it << " -- row:" << row;
        if (row >= num_rows) {
            cout << " (considered past the end)";
        }
        cout << endl;
    }
}

void EGaussian::check_xor_reason_clauses_not_cleared() {
    for (int i = clauses_toclear.size() - 1; i >= 0; i--) {
        ClOffset offs = clauses_toclear[i].first;
        Clause* cl = solver->cl_alloc.ptr(offs);
        assert(!cl->freed());
    }
}
