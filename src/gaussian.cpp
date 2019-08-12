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

    PackedMatrix::iterator rowIt = clause_state.beginMatrix();
    (*rowIt).setZero(); //forget state
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
    std::sort(vars_needed.begin(), vars_needed.end(),
              ColSorter(solver->varData));

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
    is_basic.clear();                                // reset variable state
    is_basic.growTo(solver->nVars(), 0); // init varaible state
    origMat.row_to_nb_var.clear();                             // clear non-basic

    delete_gauss_watch_this_matrix();
    clause_state.resize(1, origMat.num_rows);
    PackedMatrix::iterator rowIt = clause_state.beginMatrix();
    (*rowIt).setZero(); // reset this row all zero
    // print_matrix(origMat);
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

        fill_matrix(matrix);
        if (matrix.num_rows == 0 || matrix.num_cols == 0) {
            created = false;
            return solver->okay();
        }

        eliminate(matrix); // gauss eliminate algorithm

        // find some row already true false, and insert watch list
        gret ret = adjust_matrix(matrix);

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

void EGaussian::eliminate(matrixset& m) {
    uint32_t i = 0;
    uint32_t j = 0;
    PackedMatrix::iterator end = m.matrix.beginMatrix() + m.num_rows;
    PackedMatrix::iterator rowIt = m.matrix.beginMatrix();

    while (i != m.num_rows && j != m.num_cols) { // Gauss-Jordan Elimination
        PackedMatrix::iterator row_with_1_in_col = rowIt;

        //Find first "1" in column.
        for (; row_with_1_in_col != end; ++row_with_1_in_col) {
            if ((*row_with_1_in_col)[j]) {
                break;
            }
        }

        //We have found a "1" in this column
        if (row_with_1_in_col != end) {
            // swap row row_with_1_in_col and I
            if (row_with_1_in_col != rowIt) {
                (*rowIt).swapBoth(*row_with_1_in_col);
            }

            // XOR into *all* rows that have a "1" in col J
            // Since we XOR into *all*, this is Gauss-Jordan
            for (PackedMatrix::iterator k_row = m.matrix.beginMatrix()
                ; k_row != end
                ; ++k_row
            ) {
                // xor rows K and I
                if (k_row != rowIt) {
                    if ((*k_row)[j]) {
                        (*k_row).xorBoth(*rowIt);
                    }
                }
            }
            i++;
            ++rowIt;
            is_basic[m.col_to_var[j]] = 1; // this column is basic variable
            // printf("basic var:%d    n",m.col_to_var[j] + 1);
        }
        j++;
    }
    // print_matrix(m);
}

gret EGaussian::adjust_matrix(matrixset& m) {
    assert(solver->decisionLevel() == 0);

    PackedMatrix::iterator end = m.matrix.beginMatrix() + m.num_rows;
    PackedMatrix::iterator rowIt = m.matrix.beginMatrix();
    uint32_t row_id = 0;      // row index
    uint32_t nb_var = 0;      // non-basic variable
    bool xorEqualFalse;       // xor =
    uint32_t adjust_zero = 0; //  elimination row

    while (rowIt != end) {
        const uint32_t popcnt = (*rowIt).find_watchVar(tmp_clause, matrix.col_to_var, is_basic, nb_var);
        switch (popcnt) {

            //Conflict potentially
            case 0:
                // printf("%d:Warring: this row is all zero in adjust matrix    n",row_id);
                adjust_zero++;        // information
                if ((*rowIt).rhs()) { // conflict
                    // printf("%d:Warring: this row is conflic in adjust matrix!!!",row_id);
                    return gret::confl;
                }
                break;

            //Normal propagation
            case 1:
            {
                // printf("%d:This row only one variable, need to propogation!!!! in adjust matrix
                // n",row_id);

                xorEqualFalse = !m.matrix.getMatrixAt(row_id).rhs();
                tmp_clause[0] = Lit(tmp_clause[0].var(), xorEqualFalse);
                assert(solver->value(tmp_clause[0].var()) == l_Undef);
                solver->enqueue(tmp_clause[0]); // propagation

                //adjusting
                (*rowIt).setZero(); // reset this row all zero
                m.row_to_nb_var.push(std::numeric_limits<uint32_t>::max()); // delete non basic value in this row
                is_basic[tmp_clause[0].var()] = 0; // delete basic value in this row
                return gret::unit_prop;
            }

            //Binary XOR
            case 2: {
                // printf("%d:This row have two variable!!!! in adjust matrix    n",row_id);
                xorEqualFalse = !m.matrix.getMatrixAt(row_id).rhs();

                tmp_clause[0] = tmp_clause[0].unsign();
                tmp_clause[1] = tmp_clause[1].unsign();
                solver->ok = solver->add_xor_clause_inter(tmp_clause, !xorEqualFalse, true);
                release_assert(solver->ok);

                (*rowIt).setZero(); // reset this row all zero
                m.row_to_nb_var.push(std::numeric_limits<uint32_t>::max()); // delete non basic value in this row
                is_basic[tmp_clause[0].var()] = 0; // delete basic value in this row
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
                m.row_to_nb_var.push(nb_var);               // record in this row non_basic variable
                break;
        }
        ++rowIt;
        row_id++;
    }
    // printf("DD:nb_rows:%d %d %d    n",m.nb_rows.size() ,   row_id - adjust_zero  ,  adjust_zero);
    assert(m.row_to_nb_var.size() == row_id - adjust_zero);

    m.matrix.resizeNumRows(row_id - adjust_zero);
    m.num_rows = row_id - adjust_zero;

    // printf("DD: adjust number of Row:%d    n",num_row);
    // printf("dd:matrix by EGaussian::adjust_matrix    n");
    // print_matrix(m);
    // printf(" adjust_zero %d    n",adjust_zero);
    // printf("%d    t%d    t",m.num_rows , m.num_cols);
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
        assert(matrix.row_to_nb_var[row_n] != no_touch_var);
        vec<GaussWatched>& ws_t = solver->gwatches[matrix.row_to_nb_var[row_n]];
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
        << matrix.row_to_nb_var[row_n]+1 << endl;
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

    uint32_t nb_var = 0;     // new nobasic variable
    bool orig_basic = false; // check invoked variable is basic or non-basic

    gqd.e_var = std::numeric_limits<uint32_t>::max();
    gqd.e_row_n = std::numeric_limits<uint32_t>::max();
    gqd.do_eliminate = false;

    PackedMatrix::iterator rowIt =
        matrix.matrix.beginMatrix() + row_n; // gaussian watch invoke row
    PackedMatrix::iterator clauseIt = clause_state.beginMatrix();

    //if this clause is already satisfied
    if ((*clauseIt)[row_n]) {
        *j++ = *i;
        return true;
    }

    //swap basic and non_basic variable
    if (is_basic[p] == 1) {
        orig_basic = true;
        is_basic[matrix.row_to_nb_var[row_n]] = 1;
        is_basic[p] = 0;
    }

    const gret ret = (*rowIt).propGause(
        tmp_clause,
        solver->assigns,
        matrix.col_to_var,
        is_basic,
        nb_var,
        var_to_col[p]);

    switch (ret) {
        case gret::confl: {
            // binary conflict
            if (tmp_clause.size() == 2) {
                // printf("%d:This row is conflict two    n",row_n);
                //WARNING !!!! if orig_basic is FALSE, this will delete
                delete_gausswatch(orig_basic, row_n, p);

                is_basic[tmp_clause[0].var()] = 0; // delete value state;
                is_basic[tmp_clause[1].var()] = 0;
                matrix.row_to_nb_var[row_n] =
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
                is_basic[matrix.row_to_nb_var[row_n]] = 0;
                is_basic[p] = 1;
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
                is_basic[matrix.row_to_nb_var[row_n]] = 0;
                is_basic[p] = 1;
            }

            (*clauseIt).setBit(row_n); // this clause arleady sat
            return true;
        }

        // find new watch list
        case gret::nothing_fnewwatch:
            // printf("%d:This row is find new watch:%d => orig %d p:%d    n",row_n ,
            // nb_var,orig_basic , p);
            assert(nb_var != std::numeric_limits<uint32_t>::max());
            if (orig_basic) {
                /// clear watchlist, because only one basic value in watchlist
                assert(nb_var != p);
                clear_gwatches(nb_var);
            }
            assert(nb_var != p);
            solver->gwatches[nb_var].push(GaussWatched(row_n, matrix_no));

            if (!orig_basic) {
                matrix.row_to_nb_var[row_n] = nb_var; // update in this row non_basic variable
                return true;
            }

            // recover non_basic variable
            is_basic[matrix.row_to_nb_var[row_n]] = 0;

            // set basic variable
            is_basic[nb_var] = 1;

            // store the eliminate variable & row
            gqd.e_var = nb_var;
            gqd.e_row_n = row_n;
            break;

        // this row already true
        case gret::nothing:
            // printf("%d:This row is nothing( maybe already true)     n",row_n);
            *j++ = *i;
            if (orig_basic) { // recover
                is_basic[matrix.row_to_nb_var[row_n]] = 0;
                is_basic[p] = 1;
            }
            (*clauseIt).setBit(row_n); // this clause arleady sat
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
    PackedMatrix::iterator this_row = matrix.matrix.beginMatrix() + gqd.e_row_n;
    PackedMatrix::iterator rowI = matrix.matrix.beginMatrix();
    PackedMatrix::iterator end = matrix.matrix.endMatrix();
    uint32_t e_col = var_to_col[gqd.e_var];
    uint32_t ori_nb = 0;
    uint32_t ori_nb_col = 0;
    uint32_t nb_var = 0;
    uint32_t num_row = 0; // row inde
    PackedMatrix::iterator clauseIt = clause_state.beginMatrix();

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

    while (rowI != end) {
        //Row has a '1' in eliminating column, and it's not the row responsible
        if ((*rowI)[e_col] && this_row != rowI) {

            // detect orignal non basic watch list change or not
            ori_nb = matrix.row_to_nb_var[num_row];
            ori_nb_col = var_to_col[ori_nb];
            assert((*rowI)[ori_nb_col]);
            #ifdef VERBOSE_DEBUG
            cout
            << "mat[" << matrix_no << "] "
            << "This row " << num_row << " is non-basic for var: " << ori_nb + 1
            << " i.e. it contains '1' for this var's column"
            << endl;
            #endif

            (*rowI).xorBoth(*this_row);
            if (!(*rowI)[ori_nb_col]) { // orignal non basic value is eliminated
                #ifdef VERBOSE_DEBUG
                cout
                << "mat[" << matrix_no << "] "
                << "-> This row " << num_row << " can no longer be non-basic, has no '1', "
                << "fixing up..."<< endl;
                #endif

                // Delelte orignal non basic value in watch list
                if (ori_nb != gqd.e_var) {
                    delete_gausswatch(true, num_row);
                }

                const gret ret = (*rowI).propGause(
                    tmp_clause,
                    solver->assigns,
                    matrix.col_to_var,
                    is_basic,
                    nb_var,
                    ori_nb_col);

                switch (ret) {
                    case gret::confl: {
                        #ifdef VERBOSE_DEBUG
                        cout
                        << "mat[" << matrix_no << "] "
                        << "-> conflict during fixup"<< endl;
                        #endif

                        if (tmp_clause.size() == 2) {
                            delete_gausswatch(false, num_row);
                            assert(is_basic[tmp_clause[0].var()] == 1);
                            assert(is_basic[tmp_clause[1].var()] == 0);

                            // delete value state
                            is_basic[tmp_clause[0].var()] = 0;

                            // delete non basic value in this row
                            matrix.row_to_nb_var[num_row] = std::numeric_limits<uint32_t>::max();
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
                            matrix.row_to_nb_var[num_row] = p;

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
                            matrix.row_to_nb_var[num_row] = p;
                            break;
                        }

                        // update no_basic information
                        solver->gwatches[p].push(GaussWatched(num_row, matrix_no));
                        matrix.row_to_nb_var[num_row] = p;

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
                        (*clauseIt).setBit(num_row); // this clause arleady sat
                        break;
                    }
                    case gret::nothing_fnewwatch: // find new watch list
                        #ifdef VERBOSE_DEBUG
                        cout
                        << "mat[" << matrix_no << "] "
                        << "-> Nothing, clause NOT already satisfied, pushing in "
                        << nb_var+1 << " as non-basic var ( "
                        << num_row << " row) "
                        << endl;
                        #endif

                        solver->gwatches[nb_var].push(GaussWatched(num_row, matrix_no));
                        matrix.row_to_nb_var[num_row] = nb_var;
                        break;
                    case gret::nothing: // this row already satisfied
                        #ifdef VERBOSE_DEBUG
                        cout
                        << "mat[" << matrix_no << "] "
                        << "-> Nothing to do, already satisfied , pushing in "
                        << p+1 << " as non-basic var ( "
                        << num_row << " row) "
                        << endl;
                        #endif

                        // printf("%d:This row is nothing( maybe already true) in eliminate col
                        // n",num_row);

                        solver->gwatches[p].push(GaussWatched(num_row, matrix_no));
                        matrix.row_to_nb_var[num_row] = p; // update in this row non_basic variable
                        (*clauseIt).setBit(num_row);        // this clause arleady sat
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
                << " still contains '1', can still be non-basic" << endl;
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

void EGaussian::check_xor_reason_clauses_not_cleared() {
    for (int i = clauses_toclear.size() - 1; i >= 0; i--) {
        ClOffset offs = clauses_toclear[i].first;
        Clause* cl = solver->cl_alloc.ptr(offs);
        assert(!cl->freed());
    }
}
