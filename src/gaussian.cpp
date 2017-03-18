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

#include "gaussian.h"

#include <iostream>
#include <iomanip>
#include "clause.h"
#include <algorithm>
#include <iterator>
#include <limits>
#include <array>

#include "clausecleaner.h"
#include "solver.h"
#include "constants.h"

//#define VERBOSE_DEBUG
//#define VERBOSE_DEBUG_MORE
//#define DEBUG_GAUSS
#ifdef SLOW_DEBUG
#define slow_debug_assert(x) assert(x)
#else
#define slow_debug_assert(x)
#endif

using namespace CMSat;
using std::cout;
using std::endl;

static const uint16_t unassigned_col = std::numeric_limits<uint16_t>::max();
static const uint32_t unassigned_var = std::numeric_limits<uint32_t>::max();

Gaussian::Gaussian(
    Solver* _solver
    , const vector<Xor>& _xors
    , const uint32_t _matrix_no
) :
    solver(_solver)
    , seen(_solver->seen)
    , seen2(_solver->seen2)
    , config(solver->conf.gaussconf)
    , matrix_no(_matrix_no)
    , messed_matrix_vars_since_reversal(true)
    , gauss_last_level(0)
    , xors(_xors)
{
}

Gaussian::~Gaussian()
{
    for (uint32_t i = 0; i < clauses_toclear.size(); i++) {
        solver->cl_alloc.clauseFree(clauses_toclear[i].offs);
    }
}

inline void Gaussian::set_matrixset_to_cur()
{
    uint32_t level = solver->decisionLevel() / config.only_nth_gauss_save;
    assert(level <= matrix_sets.size());

    if (level == matrix_sets.size())
        matrix_sets.push_back(cur_matrixset);
    else
        matrix_sets[level] = cur_matrixset;
}

bool Gaussian::init_until_fixedpoint()
{
    assert(solver->ok);
    assert(solver->decisionLevel() == 0);

    if (config.decision_until == 0)
        return solver->ok;

    reset_stats();

    bool do_again_gauss = true;
    while (do_again_gauss) {
        uint32_t last_trail_size = solver->trail.size();
        if (!solver->clauseCleaner->clean_xor_clauses(xors)) {
            return false;
        }
        if (last_trail_size < solver->trail.size()) {
            continue;
        }
        do_again_gauss = false;

        init();
        PropBy confl;
        gaussian_ret g = perform_gauss(confl);
        switch (g) {
            case unit_conflict:
            case conflict:
                #ifdef VERBOSE_DEBUG
                cout << "(" << matrix_no << ") conflict at level 0" << endl;
                #endif
                solver->ok = false;
                return false;
            case unit_propagation:
            case propagation:
                unit_truths += last_trail_size - solver->trail.size();
                do_again_gauss = true;
                solver->ok = (solver->propagate<false>().isNULL());
                if (!solver->ok)
                    return false;
                break;
            case nothing:
                break;
            }
    }

    if (solver->conf.verbosity >= 3) {
        uint32_t mem1 = cur_matrixset.col_is_set.size()*sizeof(char);
        cout << "c [gauss] Mem used for col_is_set: " << (double)mem1/(1024.0) << " KB" << endl;

        uint32_t mem2 = cur_matrixset.col_to_var.size()*sizeof(uint32_t);
        cout << "c [gauss] Mem used for col_to_var: " << (double)mem2/(1024.0) << " KB" << endl;

        uint32_t mem3 = cur_matrixset.last_one_in_col.size()*sizeof(uint16_t);
        cout << "c [gauss] Mem used for last_one_in_col: " << (double)mem3/(1024.0) << " KB" << endl;

        uint32_t mem4 = cur_matrixset.first_one_in_row.size()*sizeof(uint16_t);
        cout << "c [gauss] Mem used for first_one_in_row: " << (double)mem4/(1024.0) << " KB" << endl;

        uint32_t mem5 = cur_matrixset.matrix.used_mem();
        cout << "c [gauss] Mem used for matrix: " << (double)mem5/(1024.0) << " KB" << endl;
    }

    return true;
}

void Gaussian::init()
{
    assert(solver->decisionLevel() == 0);
    #ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ") Gaussian init start ->" << endl;
    #endif

    fill_matrix(cur_matrixset);
    if (cur_matrixset.num_rows == 0
        || cur_matrixset.num_cols == 0
    ) {
        disabled = true;
        badlevel = 0;
        return;
    }

    matrix_sets.clear();
    matrix_sets.push_back(cur_matrixset);
    gauss_last_level = solver->trail.size();
    messed_matrix_vars_since_reversal = false;
    badlevel = std::numeric_limits<uint32_t>::max();

    #ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ") Gaussian init finished." << endl;
    #endif
}

struct HeapSorter
{
    HeapSorter(vector<double>& _activities) :
        activities(_activities)
    {}

    //higher activity first
    bool operator()(uint32_t a, uint32_t b) {
        return activities[a] < activities[b];
    }

    const vector<double>& activities;
};

uint32_t Gaussian::select_columnorder(
    matrixset& origMat
) {
    var_to_col.clear();
    var_to_col.resize(solver->nVars(), unassigned_col);
    vector<uint32_t> vars_needed;
    uint32_t largest_used_var = 0;

    uint32_t num_xorclauses  = 0;
    for (uint32_t i = 0; i < xors.size(); i++) {
        const Xor& x = xors[i];
        num_xorclauses++;

        for (const uint32_t v: x) {
            assert(solver->value(v) == l_Undef);
            if (var_to_col[v] == unassigned_col) {
                vars_needed.push_back(v);
                var_to_col[v] = unassigned_col - 1;;
                largest_used_var = std::max(largest_used_var, v);
            }
        }
    }
    if (vars_needed.size() >= std::numeric_limits<uint16_t>::max()-1) {
        if (solver->conf.verbosity >= 10) {
            cout << "Matrix has too many columns, exiting select_columnorder" << endl;
        }

        return 0;
    }
    if (xors.size() >= std::numeric_limits<uint16_t>::max()-1) {
        if (solver->conf.verbosity >= 10) {
            cout << "Matrix has too many rows, exiting select_columnorder" << endl;
        }
        return 0;
    }

    var_to_col.resize(largest_used_var + 1);
    var_is_in.setZero();
    var_is_in.resize(var_to_col.size(), 0);
    origMat.col_is_set.resize(origMat.num_cols, false);

    origMat.col_to_var.clear();
    std::sort(vars_needed.begin(), vars_needed.end(), HeapSorter(solver->var_act_vsids));

    for(uint32_t v : vars_needed) {
        assert(var_to_col[v] == unassigned_col - 1);
        origMat.col_to_var.push_back(v);
        var_to_col[v] = origMat.col_to_var.size()-1;
        var_is_in.setBit(v);
    }

    //for the ones that were not in the order_heap, but are marked in var_to_col
    for (uint32_t v = 0; v != var_to_col.size(); v++) {
        if (var_to_col[v] == unassigned_col - 1) {
            //assert(false && "order_heap MUST be complete!");
            origMat.col_to_var.push_back(v);
            var_to_col[v] = origMat.col_to_var.size() -1;
            var_is_in.setBit(v);
        }
    }

    #ifdef VERBOSE_DEBUG_MORE
    cout << "(" << matrix_no << ") num_xorclauses: " << num_xorclauses << endl;
    cout << "(" << matrix_no << ") col_to_var: ";
    std::copy(origMat.col_to_var.begin(), origMat.col_to_var.end(),
              std::ostream_iterator<uint32_t>(cout, ","));
    cout << endl;
    #endif

    return num_xorclauses;
}

void Gaussian::fill_matrix(matrixset& origMat)
{
    #ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ") Filling matrix" << endl;
    #endif

    origMat.num_rows = select_columnorder(origMat);
    if (origMat.num_rows == 0) {
        return;
    }
    origMat.num_cols = origMat.col_to_var.size();
    col_to_var_original = origMat.col_to_var;
    assert(changed_rows.empty());

    origMat.last_one_in_col.clear();
    origMat.last_one_in_col.resize(origMat.num_cols, origMat.num_rows);
    origMat.first_one_in_row.resize(origMat.num_rows);

    origMat.removeable_cols = 0;
    origMat.least_column_changed = -1;
    origMat.matrix.resize(origMat.num_rows, origMat.num_cols);

    #ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ") matrix size:" << origMat.num_rows << "," << origMat.num_cols << endl;
    #endif

    uint32_t matrix_row = 0;
    for (const Xor& x: xors) {
        #ifdef VEROBOSE_DEBUG
        //Used with check_gauss.py
        cout << "x " << x << endl;
        #endif
        #ifdef DEBUG_GAUSS
        for(const Xor& x: xors) {
            for(uint32_t v: x) {
                assert(solver->varData[v].removed == Removed::none);
            }
        }
        #endif

        origMat.matrix.getVarsetAt(matrix_row).set(x, var_to_col, origMat.num_cols);
        origMat.matrix.getMatrixAt(matrix_row).set(x, var_to_col, origMat.num_cols);
        matrix_row++;
    }
    assert(origMat.num_rows == matrix_row);
}

void Gaussian::update_matrix_col(matrixset& m, const uint32_t var, const uint32_t col)
{
    #ifdef VERBOSE_DEBUG_MORE
    cout << "(" << matrix_no << ") Updating matrix var " << var+1
    << " (col " << col << ", m.last_one_in_col[col]: " << m.last_one_in_col[col] << ")"
    << endl;
    cout << "m.num_rows:" << m.num_rows << endl;
    #endif

    #ifdef DEBUG_GAUSS
    assert(col < m.num_cols);
    #endif

    m.least_column_changed = std::min(m.least_column_changed, (int)col);
    PackedMatrix::iterator this_row = m.matrix.beginMatrix();
    uint32_t row_num = 0;

    assert(solver->value(var) != l_Undef && "Not sure about this one.. :S");
    if (solver->value(var) == l_True) {
        for (uint32_t end = m.last_one_in_col[col]
            ; row_num != end
            ; ++this_row, row_num++
        ) {
            if ((*this_row)[col]) {
                if (!seen2[row_num]) {
                    seen2[row_num] = true;
                    changed_rows.push_back(row_num);
                }
                (*this_row).invert_is_true();
                (*this_row).clearBit(col);
            }
        }
    } else {
        for (uint32_t end = m.last_one_in_col[col]
            ; row_num != end
            ; ++this_row, row_num++
        ) {
            if ((*this_row)[col]) {
                if (!seen2[row_num]) {
                    seen2[row_num] = true;
                    changed_rows.push_back(row_num);
                }
                (*this_row).clearBit(col);
            }
        }
    }

    #ifdef DEBUG_GAUSS
    bool c = false;
    for(PackedMatrix::iterator r = m.matrix.beginMatrix(), end = r + m.matrix.getSize()
        ; r != end
        ; ++r)
    {
        c |= (*r)[col];
    }
    assert(!c);
    #endif

    m.removeable_cols++;
    m.col_to_var[col] = unassigned_var;
    slow_debug_assert(var_to_col[var] == col);
    m.col_is_set[col] = true;
}

void Gaussian::update_matrix_by_col_all(matrixset& m)
{
    #ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ") Updating matrix." << endl;
    #ifdef VERBOSE_DEBUG_MORE
    print_matrix(m);
    #endif
    uint32_t num_updated = 0;
    #endif

    #ifdef DEBUG_GAUSS
    assert(nothing_to_propagate(cur_matrixset));
    assert(solver->decisionLevel() == 0 || check_last_one_in_cols(m));
    #endif
    assert(changed_rows.empty());

    uint32_t last = 0;
    uint32_t col = 0;
    for (const uint32_t *it = &m.col_to_var[0], *end = it + m.num_cols; it != end; col++, it++) {
        if (*it != unassigned_var && solver->value(*it) != l_Undef) {
            update_matrix_col(m, *it, col);
            last++;
            #ifdef VERBOSE_DEBUG
            num_updated++;
            #endif
        } else
            last = 0;
    }
    m.num_cols -= last;

    #ifdef DEBUG_GAUSS
    check_matrix_against_varset(m.matrix, m);
    #endif

    #ifdef VERBOSE_DEBUG
    cout << "Matrix update finished, updated " << num_updated << " cols" << endl;
    #ifdef VERBOSE_DEBUG_MORE
    print_matrix(m);
    #endif
    #endif

    /*cout << "num_rows:" << m.num_rows;
    cout << " num_rows diff:" << origMat.num_rows - m.num_rows << endl;
    cout << "num_cols:" << col_to_var_original.size();
    cout << " num_cols diff:" << col_to_var_original.size() - m.col_to_var.size() << endl;
    cout << "removeable cols:" << m.removeable_cols << endl;*/
}

inline void Gaussian::update_last_one_in_col(matrixset& m)
{
    for (uint16_t* i = &m.last_one_in_col[0]+m.last_one_in_col.size()-1, *end = &m.last_one_in_col[0]-1
        ; i != end && *i >= m.num_rows
        ; i--
    ) {
        *i = m.num_rows;
    }
}

Gaussian::gaussian_ret Gaussian::perform_gauss(PropBy& confl)
{
    //cout << ">>G-----" << endl;
    if (solver->decisionLevel() >= badlevel) {
        //cout << "Over badlevel" << endl;
        //cout << "<<----G" << endl;
        return nothing;
    }

    const uint32_t level = solver->decisionLevel() / config.only_nth_gauss_save;
    //cout << "level: " << level << " matrix_sets.size(): " << matrix_sets.size() << endl;

    if (messed_matrix_vars_since_reversal) {
        #ifdef VERBOSE_DEBUG
        cout << "(" << matrix_no << ") matrix needs copy before update" << endl;
        #endif

        assert(level < matrix_sets.size());
        cur_matrixset = matrix_sets[level];
    }
    update_last_one_in_col(cur_matrixset);
    update_matrix_by_col_all(cur_matrixset);

    messed_matrix_vars_since_reversal = false;
    gauss_last_level = solver->trail.size();
    badlevel = std::numeric_limits<uint32_t>::max();

    propagatable_rows.clear();
    uint32_t last_row = eliminate(cur_matrixset);
    #ifdef DEBUG_GAUSS
    check_matrix_against_varset(cur_matrixset.matrix, cur_matrixset);
    #endif

    gaussian_ret ret;
    //There is no early abort, so this is unneeded
    /*if (conflict_row != std::numeric_limits<uint32_t>::max()) {
        uint32_t maxlevel = std::numeric_limits<uint32_t>::max();
        uint32_t size = std::numeric_limits<uint32_t>::max();
        uint32_t best_row = std::numeric_limits<uint32_t>::max();
        analyse_confl(cur_matrixset, conflict_row, maxlevel, size, best_row);
        ret = handle_matrix_confl(confl, cur_matrixset, size, maxlevel, best_row);
    } else {*/
        ret = handle_matrix_prop_and_confl(cur_matrixset, last_row, confl);
    //}
    #ifdef DEBUG_GAUSS
    assert(ret == conflict || ret == unit_conflict || ret == unit_propagation || nothing_to_propagate(cur_matrixset));
    #endif

    if (!cur_matrixset.num_cols || !cur_matrixset.num_rows) {
        badlevel = solver->decisionLevel();
        //cout << "Set badlevel to " << badlevel << endl;
        //cout << "<<----G" << endl;
        return ret;
    }

    /*cout << "ret is :";
    if (ret == nothing) {
        cout << "nothing" << endl;
    } else if (ret == conflict) {
        cout << "conflict" << endl;
    } else if (ret == unit_conflict) {
        cout << "unit_conflict";
    } else if (ret == propagation) {
        cout << "propagation";
    } else if (ret == unit_propagation) {
        cout << "unit propagation";
    }*/

    if (ret == nothing &&
        solver->decisionLevel() % config.only_nth_gauss_save == 0
    ) {
        set_matrixset_to_cur();
    }

    #ifdef VERBOSE_DEBUG
    if (ret == nothing)
        cout << "(" << matrix_no << ") Useless. ";
    else
        cout << "(" << matrix_no << ") Useful. ";
    cout << "(" << matrix_no << ") Useful prop in " << float_div(useful_prop, called)*100.0 << "%" << endl;
    cout << "(" << matrix_no << ") Useful confl in " << float_div(useful_confl, called)*100.0 << "%" << endl;
    cout << "(" << matrix_no << ") ------------ Finished perform_gauss -----------------------------." << endl;
    #endif

    //cout << "<<----G" << endl;
    return ret;
}

uint32_t Gaussian::eliminate(matrixset& m)
{
    #ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ") ";
    cout << "Starting elimination" << endl;
    cout << "m.least_column_changed:" << m.least_column_changed << endl;
    #ifdef VERBOSE_DEBUG_MORE
    print_last_one_in_cols(m);
    #endif

    uint32_t number_of_row_additions = 0;
    uint32_t no_exchanged = 0;
    #endif

    if (m.least_column_changed == std::numeric_limits<int>::max()) {
        #ifdef VERBOSE_DEBUG
        cout << "Nothing to eliminate" << endl;
        #endif

        return m.num_rows;
    }


    #ifdef DEBUG_GAUSS
    assert(solver->decisionLevel() == 0 || check_last_one_in_cols(m));
    #endif

    uint32_t i = 0;
    uint32_t j = (config.iterativeReduce) ? m.least_column_changed + 1 : 0;
    PackedMatrix::iterator beginIt = m.matrix.beginMatrix();
    PackedMatrix::iterator rowIt = m.matrix.beginMatrix();

    #ifdef DEBUG_GAUSS
    check_first_one_in_row(m, j);
    #endif

    if (j) {
        uint16_t until = std::min(m.last_one_in_col[m.least_column_changed] - 1, (int)m.num_rows);
        if (j-1 > m.first_one_in_row[m.num_rows-1])
            until = m.num_rows;
        for (;i != until; i++, ++rowIt) {
            if (seen2[i]
                && (*rowIt).popcnt_is_one(m.first_one_in_row[i]))
            {
                propagatable_rows.push_back(i);
            }
        }
    }

    //Clear seen2 & changed_rows
    for(uint32_t r: changed_rows) {
        seen2[r] = false;
    }
    changed_rows.clear();

    #ifdef VERBOSE_DEBUG
    cout << "At while() start: i,j = " << i << ", " << j << endl;
    cout << "num_rows:" << m.num_rows << " num_cols:" << m.num_cols << endl;
    #endif

    if (j > m.num_cols) {
        #ifdef VERBOSE_DEBUG
        cout << "Going straight to finish" << endl;
        #endif
        goto finish;
    }

    #ifdef DEBUG_GAUSS
    assert(i <= m.num_rows && j <= m.num_cols);
    #endif

    while (i < m.num_rows && j < m.num_cols) {
        //Find pivot in column j, starting in row i:

        #ifdef VERBOSE_DEBUG_MORE
        cout << "i: " << i << " j: " << j << endl;
        #endif
        if (m.col_to_var[j] == unassigned_var) {
            j++;
            continue;
        }

        PackedMatrix::iterator this_matrix_row = rowIt;
        PackedMatrix::iterator end = beginIt + m.last_one_in_col[j];
        for (; this_matrix_row != end; ++this_matrix_row) {
            if ((*this_matrix_row)[j]) {
                break;
            }
        }
        //First row with non-zero value at j is this_matrix_row

        if (this_matrix_row != end) {
            //swap rows i and maxi, but do not change the value of i;
            if (this_matrix_row != rowIt) {
                #ifdef VERBOSE_DEBUG
                no_exchanged++;
                #endif

                //Would early abort, but would not find the best conflict (and would be expensive)
                //if (matrix_row_i.rhs() && matrix_row_i.isZero()) {
                //    conflict_row = i;
                //    return 0;
                //}
                (*rowIt).swapBoth(*this_matrix_row);
            }
            #ifdef DEBUG_GAUSS
            assert(m.matrix.getMatrixAt(i).popcnt(j) == m.matrix.getMatrixAt(i).popcnt());
            assert(m.matrix.getMatrixAt(i)[j]);
            #endif

            if ((*rowIt).popcnt_is_one(j)) {
                propagatable_rows.push_back(i);
            }

            //Now A[i,j] will contain the old value of A[maxi,j];
            ++this_matrix_row;
            for (; this_matrix_row != end; ++this_matrix_row) if ((*this_matrix_row)[j]) {
                //subtract row i from row u;
                //Now A[u,j] will be 0, since A[u,j] - A[i,j] = A[u,j] -1 = 0.
                #ifdef VERBOSE_DEBUG
                number_of_row_additions++;
                #endif

                (*this_matrix_row).xorBoth(*rowIt);
                //Would early abort, but would not find the best conflict (and would be expensive)
                //if (it->rhs() &&it->isZero()) {
                //    conflict_row = i2;
                //    return 0;
                //}
            }
            m.first_one_in_row[i] = j;
            i++;
            ++rowIt;
            m.last_one_in_col[j] = i;
        } else {
            m.first_one_in_row[i] = j;
            m.last_one_in_col[j] = i + 1;
        }
        j++;
    }

    finish:

    m.least_column_changed = std::numeric_limits<int>::max();

    #ifdef VERBOSE_DEBUG
    cout << "Finished elimination. Num propagatable rows: " << propagatable_rows.size() << endl;
    cout << "Returning with i,j:" << i << ", " << j << "(" << m.num_rows << ", " << m.num_cols << ") " << endl;
    #ifdef VERBOSE_DEBUG_MORE
    print_matrix(m);
    print_last_one_in_cols(m);
    #endif
    cout << "(" << matrix_no << ") Exchanged:" << no_exchanged
    << " row additions:" << number_of_row_additions << endl;
    #endif

    #ifdef DEBUG_GAUSS
    assert(check_last_one_in_cols(m));
    uint32_t row = 0;
    uint32_t col = 0;
    for (; col < m.num_cols && row < m.num_rows && row < i ; col++) {
        assert(m.matrix.getMatrixAt(row).popcnt() == m.matrix.getMatrixAt(row).popcnt(col));
        assert(!(m.col_to_var[col] == unassigned_var && m.matrix.getMatrixAt(row)[col]));
        if (m.col_to_var[col] == unassigned_var || !m.matrix.getMatrixAt(row)[col]) {
            #ifdef VERBOSE_DEBUG_MORE
            cout << "row:" << row << " col:" << col << " m.last_one_in_col[col]-1: " << m.last_one_in_col[col]-1 << endl;
            #endif
            assert(m.col_to_var[col] == unassigned_var || std::min((uint16_t)(m.last_one_in_col[col]-1), m.num_rows) == row);
            continue;
        }
        row++;
    }
    #endif

    return i;
}

Gaussian::gaussian_ret Gaussian::handle_matrix_confl(
    PropBy& confl
    , const matrixset& m
    , const uint32_t maxlevel
    , const uint32_t best_row
) {
    assert(best_row != std::numeric_limits<uint32_t>::max());

    const bool wasUndef = m.matrix.getVarsetAt(best_row).fill(tmp_clause, solver->assigns, col_to_var_original);
    release_assert(!wasUndef);
    /*
     * TODO: try out on a cluster
     *
     * for(Lit l: tmp_clause) {
        solver->bump_vsids_var_act<false>(l.var());
    }*/

    #ifdef VERBOSE_DEBUG
    const bool rhs = m.matrix.getVarsetAt(best_row).rhs();
    //Used with check_gauss.py
    cout << "(" << matrix_no << ") confl clause: "
    << tmp_clause << " , "
    << "rhs:" << rhs << endl;
    #endif

    if (tmp_clause.size() <= 1) {
        if (tmp_clause.size() == 1) {
            confl = PropBy(tmp_clause[0], false);
        } else {
            confl = PropBy();
            solver->ok = false;
        }
        return unit_conflict;
    }

    if (maxlevel != solver->decisionLevel()) {
        solver->cancelUntil(maxlevel);
    }
    const uint32_t curr_dec_level = solver->decisionLevel();
    assert(maxlevel == curr_dec_level);

    uint32_t first_var = std::numeric_limits<uint32_t>::max();
    if (tmp_clause.size() == 2) {
        solver->attach_bin_clause(tmp_clause[0], tmp_clause[1], true, false);
        Lit lit1 = tmp_clause[0];
        Lit lit2 = tmp_clause[1];
        seen[lit1.var()] = 1;
        seen[lit2.var()] = 1;

        for (int i = solver->trail.size()-1; i >= 0; i --) {
            uint32_t v = solver->trail[i].var();
            if (v < seen.size() && seen[v]) {
                first_var = v;
                break;
            }
        }

        if (lit1.var() == first_var) {
            std::swap(lit1, lit2);
        }

        seen[lit1.var()] = 0;
        seen[lit2.var()] = 0;

        confl = PropBy(lit1, false);
        solver->failBinLit = lit2;
    } else {
        //NOTE: No need to put this clause into some special struct
        //it will be immediately freed after calling solver->handle_conflict()
        Clause* cl = (Clause*)solver->cl_alloc.Clause_new(tmp_clause
        , solver->sumConflicts
        #ifdef STATS_NEEDED
        , 1
        #endif
        );
        confl = PropBy(solver->cl_alloc.get_offset(cl));

        uint32_t first_var_at = std::numeric_limits<uint32_t>::max();
        for(Lit l: tmp_clause) {
            if (solver->varData[l.var()].level == curr_dec_level) {
                seen[l.var()] = 1;
            }
        }

        for (int i = solver->trail.size()-1; i >= 0; i --) {
            uint32_t v = solver->trail[i].var();
            if (v < seen.size() && seen[v]) {
                first_var = v;
                break;
            }
        }
        for(size_t i = 0; i < tmp_clause.size(); i++) {
            Lit l = tmp_clause[i];
            if (l.var() == first_var) {
                first_var_at = i;
            }
            if (solver->varData[l.var()].level == curr_dec_level) {
                seen[l.var()] = 0;
            }
        }

        std::swap((*cl)[first_var_at], (*cl)[1]);
        cl->set_gauss_temp_cl();
    }
    messed_matrix_vars_since_reversal = true;

    return conflict;
}

Gaussian::gaussian_ret Gaussian::handle_matrix_prop_and_confl(
    matrixset& m
    , uint32_t last_row
    , PropBy& confl
) {
    uint32_t maxlevel = std::numeric_limits<uint32_t>::max();
    uint32_t size = std::numeric_limits<uint32_t>::max();
    uint32_t best_row = std::numeric_limits<uint32_t>::max();

    for (uint32_t row = last_row; row != m.num_rows; row++) {
        #ifdef DEBUG_GAUSS
        assert(m.matrix.getMatrixAt(row).isZero());
        #endif
        if (m.matrix.getMatrixAt(row).rhs())
            analyse_confl(m, row, maxlevel, size, best_row);
    }

    if (maxlevel != std::numeric_limits<uint32_t>::max())
        return handle_matrix_confl(confl, m, maxlevel, best_row);

    #ifdef DEBUG_GAUSS
    assert(check_no_conflict(m));
    assert(last_row == 0 || !m.matrix.getMatrixAt(last_row-1).isZero());
    #endif

    #ifdef VERBOSE_DEBUG
    cout << "Resizing matrix to num_rows = " << last_row << endl;
    #endif
    m.num_rows = last_row;
    m.matrix.resizeNumRows(m.num_rows);

    gaussian_ret ret = nothing;

    uint32_t num_props = 0;
    uint32_t orig_dec_level = solver->decisionLevel();
    for (uint32_t prop_row : propagatable_rows) {
        //this is a "000..1..0000000X" row. I.e. it indicates a propagation
        ret = handle_matrix_prop(m, prop_row);
        num_props++;
        if (ret == unit_propagation
            && orig_dec_level != 0
        ) {
            #ifdef VERBOSE_DEBUG
            cout << "(" << matrix_no << ") Unit prop! Breaking from prop examination" << endl;
            #endif
            return  unit_propagation;
        }
    }
    #ifdef VERBOSE_DEBUG
    if (num_props > 0) cout << "(" << matrix_no << ") Number of props during gauss:" << num_props << endl;
    #endif

    return ret;
}

void Gaussian::analyse_confl(
    const matrixset& m
    , const uint32_t row
    , uint32_t& maxlevel
    , uint32_t& size
    , uint32_t& best_row
) const {
    assert(row < m.num_rows);

    //this is a "000...00000001" row. I.e. it indicates we are on the wrong branch
    #ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")matrix conflict found!" << endl;
    cout << "(" << matrix_no << ")conflict clause's vars: ";
    #ifdef VERBOSE_DEBUG_MORE
    print_matrix_row_with_assigns(m.matrix.getVarsetAt(row));
    cout << endl;

    cout << "(" << matrix_no << ")corresponding matrix's row (should be empty): ";
    print_matrix_row(m.matrix.getMatrixAt(row));
    cout << endl;
    #endif
    #endif

    uint32_t this_maxlevel = 0;
    uint32_t var = 0;
    uint32_t this_size = 0;
    while (true) {
        var = m.matrix.getVarsetAt(row).scan(var);
        if (var == std::numeric_limits<uint32_t>::max()) break;

        const uint32_t real_var = col_to_var_original[var];
        assert(real_var < solver->nVars());

        if (solver->varData[real_var].level > this_maxlevel)
            this_maxlevel = solver->varData[real_var].level;
        var++;
        this_size++;
    }

    //the maximum of all lit's level must be lower than the max. level of the current best clause (or this clause must be either empty or unit clause)
    if (!(
                (this_maxlevel < maxlevel)
                || (this_maxlevel == maxlevel && this_size < size)
                || (this_size <= 1)
            )) {
        assert(maxlevel != std::numeric_limits<uint32_t>::max());

        #ifdef VERBOSE_DEBUG
        cout << "(" << matrix_no << ")Other found conflict just as good or better.";
        cout << "(" << matrix_no << ") || Old maxlevel:" << maxlevel << " new maxlevel:" << this_maxlevel;
        cout << "(" << matrix_no << ") || Old size:" << size << " new size:" << this_size << endl;
        //assert(!(maxlevel != std::numeric_limits<uint32_t>::max() && maxlevel != this_maxlevel)); //NOTE: only holds if gauss is executed at each level
        #endif

        return;
    }


    #ifdef VERBOSE_DEBUG
    if (maxlevel != std::numeric_limits<uint32_t>::max())
        cout << "(" << matrix_no << ")Better conflict found.";
    else
        cout << "(" << matrix_no << ")Found a possible conflict.";

    cout << "(" << matrix_no << ") || Old maxlevel:" << maxlevel << " new maxlevel:" << this_maxlevel;
    cout << "(" << matrix_no << ") || Old size:" << size << " new size:" << this_size << endl;
    #endif

    maxlevel = this_maxlevel;
    size = this_size;
    best_row = row;
}

Gaussian::gaussian_ret Gaussian::handle_matrix_prop(matrixset& m, const uint32_t row)
{
    #ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ") matrix prop" << endl;
    #ifdef VERBOSE_DEBUG_MORE
    cout << "(" << matrix_no << ") matrix row:" << m.matrix.getMatrixAt(row) << endl;
    #endif
    #endif

    assert(m.matrix.getMatrixAt(row).popcnt() == 1);
    m.matrix.getVarsetAt(row).fill(tmp_clause, solver->assigns, col_to_var_original);
    #ifdef VERBOSE_DEBUG
    //Used with check_gauss.py
    cout << "(" << matrix_no << ") prop clause: "
    << tmp_clause << " , "
    << "rhs:" << m.matrix.getVarsetAt(row).rhs() << endl;
    cout << "varData [0]:" << removed_type_to_string(solver->varData[tmp_clause[0].var()].removed) << endl;
    #endif
    #ifdef DEBUG_GAUSS
    for(Lit l: tmp_clause) {
        assert(solver->varData[l.var()].removed == Removed::none);
    }
    #endif

    switch(tmp_clause.size()) {
        case 0:
            //This would mean nothing, empty = false, always true in xors
            assert(false);
            break;
        case 1:
            solver->cancelUntil(0);
            solver->enqueue(tmp_clause[0]);
            return unit_propagation;
        case 2: {
            solver->attach_bin_clause(tmp_clause[0], tmp_clause[1], true, false);
            solver->attach_bin_clause(~tmp_clause[0], ~tmp_clause[1], true, false);
            solver->enqueue(tmp_clause[0], PropBy(tmp_clause[1], true));
            return propagation;
        }
        default:
            if (solver->decisionLevel() == 0) {
                solver->enqueue(tmp_clause[0]);
                return unit_propagation;
            }
            Clause* x = solver->cl_alloc.Clause_new(tmp_clause
            , solver->sumConflicts
            #ifdef STATS_NEEDED
            , 1
            #endif
            );
            ClOffset offs = solver->cl_alloc.get_offset(x);
            assert(m.matrix.getMatrixAt(row).rhs() == !tmp_clause[0].sign());
            assert(solver->value(tmp_clause[0]) == l_Undef);
            x->set_gauss_temp_cl();

            clauses_toclear.push_back(GaussClauseToClear(offs, solver->trail.size()-1));
            solver->enqueue(tmp_clause[0], PropBy(offs));
            return propagation;
    }

    return propagation;
}

void Gaussian::canceling(const uint32_t sublevel)
{
    uint32_t rem = 0;
    for (int i = (int)clauses_toclear.size()-1
        ; i >= 0 && clauses_toclear[i].sublevel >= sublevel
        ; i--
    ) {
        solver->cl_alloc.clauseFree(clauses_toclear[i].offs);
        rem++;
    }
    clauses_toclear.resize(clauses_toclear.size()-rem);

    //We cannot check for 'disabled' above this point --we must cancel
    //the clauses at the right time
    if (disabled) {
        return;
    }

    //Check if nothing has been messed with, i.e. if matrix is still intact
    if (messed_matrix_vars_since_reversal)
        return;

    int c = std::min((int)gauss_last_level, (int)(solver->trail.size())-1);
    for (; c >= (int)sublevel; c--) {
        uint32_t var  = solver->trail[c].var();
        if (var < var_is_in.getSize()
            && var_is_in[var]
            && cur_matrixset.col_is_set[var_to_col[var]]
        ) {
            messed_matrix_vars_since_reversal = true;
            return;
        }
    }
}

void Gaussian::disable_if_necessary()
{
    if (!disabled
        && config.autodisable
        && called > 1000 //TODO MAGIC constant
        && useful_confl*2+useful_prop < (uint32_t)((double)called*0.05) )
    {
        //NOTE: we cannot call "cancelling(0);" here or we will have
        //dangling PropBy values.
        disabled = true;
    }
}

gauss_ret Gaussian::find_truths()
{
    disable_if_necessary();
    if (!should_check_gauss(solver->decisionLevel())) {
        return gauss_nothing;
    }

    PropBy confl;
    gaussian_ret g = perform_gauss(confl);
    called++;

    switch (g) {
        case conflict:
            useful_confl++;
            if (confl.isClause()) {
                clauses_toclear.push_back(GaussClauseToClear(confl.get_offset(), solver->trail.size()-1));
            }
            found_conflict = confl;
            return gauss_confl;

        case unit_propagation:
            unit_truths++;
            useful_prop++;
            return gauss_cont;

        case propagation:
            useful_prop++;
            return gauss_cont;

        case unit_conflict: {
            unit_truths++;
            useful_confl++;
            if (confl.isNULL()) {
                #ifdef VERBOSE_DEBUG
                cout << "(" << matrix_no << ")zero-length conflict. UNSAT" << endl;
                #endif
                solver->ok = false;
                return gauss_false;
            }

            Lit lit = confl.lit2();
            solver->cancelUntil(0);

            #ifdef VERBOSE_DEBUG
            cout << "(" << matrix_no << ")one-length conflict" << endl;
            #endif

            if (solver->value(lit) != l_Undef) {
                assert(solver->value(lit) == l_False);
                #ifdef VERBOSE_DEBUG
                cout << "(" << matrix_no << ") -> UNSAT" << endl;
                #endif
                solver->ok = false;
                return gauss_false;
            }

            #ifdef VERBOSE_DEBUG
            cout << "(" << matrix_no << ") -> setting to correct value" << endl;
            #endif
            solver->enqueue(lit);
            return gauss_cont;
        }

        case nothing:
            break;
    }

    return gauss_nothing;
}

template<class T>
void Gaussian::print_matrix_row(const T& row) const
{
    uint32_t var = 0;
    while (true) {
        var = row.scan(var);
        if (var == std::numeric_limits<uint32_t>::max())
            break;

        else cout << col_to_var_original[var]+1 << ", ";
        var++;
    }
    cout << "final:" << row.rhs() << endl;;
}

template<class T>
void Gaussian::print_matrix_row_with_assigns(const T& row) const
{
    uint32_t col = 0;
    while (true) {
        col = row.scan(col);
        if (col == std::numeric_limits<uint32_t>::max()) break;

        else {
            uint32_t var = col_to_var_original[col];
            cout << var+1 << "(" << lbool_to_string(solver->assigns[var]) << ")";
            cout << ", ";
        }
        col++;
    }
    if (!row.rhs()) cout << "xorEqualFalse";
}

string Gaussian::lbool_to_string(const lbool toprint)
{
    if (toprint == l_True)
            return "true";
    if (toprint == l_False)
            return "false";
    if (toprint == l_Undef)
            return "undef";

    assert(false);
    return "";
}


void Gaussian::print_stats() const
{
    if (called > 0) {
        cout << std::fixed
        << "c Gauss(" << matrix_no << ") useful"
        << " prop: "
        << std::setprecision(2) << std::setw(5) << float_div(useful_prop, called)*100.0 << "% "
        << " confl: "
        << std::setprecision(2) << std::setw(5) << float_div(useful_confl, called)*100.0 << "% ";
        if (disabled) {
            cout << "disabled";
        }
        cout << endl;
    } else
        cout << "c Gauss(" << matrix_no << ") not called" << endl;
}

void Gaussian::print_matrix_stats() const
{
    cout << "matrix size: " << cur_matrixset.num_rows << "  x " << cur_matrixset.num_cols << endl;
}


void Gaussian::reset_stats()
{
    useful_prop = 0;
    useful_confl = 0;
    called = 0;
    disabled = false;
}

bool Gaussian::check_no_conflict(matrixset& m) const
{
    uint32_t row = 0;
    for(PackedMatrix::iterator r = m.matrix.beginMatrix(), end = m.matrix.endMatrix(); r != end; ++r, ++row) {
        if ((*r).rhs() && (*r).isZero()) {
            cout << "Conflict at row " << row << endl;
            return false;
        }
    }
    return true;
}

void Gaussian::print_matrix(matrixset& m) const
{
    uint32_t row = 0;
    for (PackedMatrix::iterator it = m.matrix.beginMatrix(); it != m.matrix.endMatrix(); ++it, row++) {
        cout << *it << " -- row:" << row;
        if (row >= m.num_rows)
            cout << " (considered past the end)";
        cout << endl;
    }
}

void Gaussian::print_last_one_in_cols(matrixset& m) const
{
    for (uint32_t i = 0; i < m.num_cols; i++) {
        cout << "last_one_in_col[" << i << "]-1 = " << m.last_one_in_col[i]-1 << endl;
    }
}

bool Gaussian::nothing_to_propagate(matrixset& m) const
{
    for(PackedMatrix::iterator
        r = m.matrix.beginMatrix(), end = m.matrix.endMatrix()
        ; r != end
        ; ++r
    ) {
        if ((*r).popcnt_is_one()
            && solver->value(m.col_to_var[(*r).scan(0)]) == l_Undef
        ) {
            #ifdef VERBOSE_DEBUG
            cout << "row " << (*r) << " is a propagation, but we didn't catch it" << endl;
            #endif
            return false;
        }
    }
    for(PackedMatrix::iterator
        r = m.matrix.beginMatrix(), end = m.matrix.endMatrix()
        ; r != end
        ; ++r
    ) {
        if ((*r).isZero() && (*r).rhs()) {
            #ifdef VERBOSE_DEBUG
            cout << "row " << (*r) << " is a conflict, but we didn't catch it" << endl;
            #endif
            return false;
        }
    }
    return true;
}

bool Gaussian::check_last_one_in_cols(matrixset& m) const
{
    for(uint32_t i = 0; i < m.num_cols; i++) {
        const uint32_t last = std::min(m.last_one_in_col[i] - 1, (int)m.num_rows);
        uint32_t real_last = 0;
        uint32_t i2 = 0;
        for (PackedMatrix::iterator it = m.matrix.beginMatrix(); it != m.matrix.endMatrix(); ++it, i2++) {
            if ((*it)[i])
                real_last = i2;
        }
        if (real_last > last)
            return false;
    }

    return true;
}

void Gaussian::check_matrix_against_varset(PackedMatrix& matrix, const matrixset& m) const
{
    for (uint32_t i = 0; i < matrix.getSize(); i++) {
        const PackedRow mat_row = matrix.getMatrixAt(i);
        const PackedRow var_row = matrix.getVarsetAt(i);

        uint32_t col = 0;
        bool final = false;
        while (true) {
            col = var_row.scan(col);
            if (col == std::numeric_limits<uint32_t>::max()) break;

            const uint32_t var = col_to_var_original[col];
            assert(var < solver->nVars());

            slow_debug_assert(var_to_col[var] == col);
            if (solver->value(var) == l_True) {
                assert(!mat_row[col]);
                assert(m.col_to_var[col] == unassigned_var);
                assert(m.col_is_set[col]);
                final = !final;
            } else if (solver->value(var) == l_False) {
                assert(!mat_row[col]);
                assert(m.col_to_var[col] == unassigned_var);
                assert(m.col_is_set[col]);
            } else if (solver->value(var) == l_Undef) {
                assert(m.col_to_var[col] != unassigned_var);
                assert(!m.col_is_set[col]);
                assert(mat_row[col]);
            } else {
                assert(false);
            }

            col++;
        }
        if ((final^!mat_row.rhs()) != !var_row.rhs()) {
            cout << "problem with row:"; print_matrix_row_with_assigns(var_row); cout << endl;
            assert(false);
        }
    }
}

void Gaussian::check_first_one_in_row(matrixset& m, const uint32_t j)
{
    if (j) {
        uint16_t until2 = std::min(m.last_one_in_col[m.least_column_changed] - 1, (int)m.num_rows);
        if (j-1 > m.first_one_in_row[m.num_rows-1]) {
            until2 = m.num_rows;
            #ifdef VERBOSE_DEBUG
            cout << "j-1 > m.first_one_in_row[m.num_rows-1]" << "j:" << j
            << " m.first_one_in_row[m.num_rows-1]:" << m.first_one_in_row[m.num_rows-1] << endl;
            #endif
        }
        for (uint32_t i2 = 0; i2 != until2; i2++) {
            #ifdef VERBOSE_DEBUG
            cout << endl << "row " << i2 << " (num rows:" << m.num_rows << ")" << endl;
            cout << m.matrix.getMatrixAt(i2) << endl;
            cout << " m.first_one_in_row[m.num_rows-1]:" << m.first_one_in_row[m.num_rows-1] << endl;
            cout << "first_one_in_row:" << m.first_one_in_row[i2] << endl;
            cout << "num_cols:" << m.num_cols << endl;
            cout << "popcnt:" << m.matrix.getMatrixAt(i2).popcnt() << endl;
            cout << "popcnt_is_one():" << m.matrix.getMatrixAt(i2).popcnt_is_one() << endl;
            cout << "popcnt_is_one("<< m.first_one_in_row[i2] <<"): "
            << m.matrix.getMatrixAt(i2).popcnt_is_one(m.first_one_in_row[i2]) << endl;
            #endif

            for (uint32_t i3 = 0; i3 < m.first_one_in_row[i2]; i3++) {
                assert(m.matrix.getMatrixAt(i2)[i3] == 0);
            }
            assert(m.matrix.getMatrixAt(i2)[m.first_one_in_row[i2]]);
            assert(m.matrix.getMatrixAt(i2).popcnt_is_one() ==
            m.matrix.getMatrixAt(i2).popcnt_is_one(m.first_one_in_row[i2]));
        }
    }
}
