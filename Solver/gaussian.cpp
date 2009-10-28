/***********************************************************************************
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************************/

#include "gaussian.h"
#include <boost/foreach.hpp>
#include <iostream>
#include <iomanip>
#include "clause.h"
#include <algorithm>
using std::ostream;
using std::cout;
using std::endl;

ostream& operator << (ostream& os, const vec<Lit>& v)
{
    for (int i = 0; i < v.size(); i++) {
        if (v[i].sign()) os << "-";
        os << v[i].var()+1 << " ";
    }

    return os;
}

Gaussian::Gaussian(Solver& _solver, const uint _matrix_no, const GaussianConfig& _config) :
        solver(_solver)
        , matrix_no(_matrix_no)
        , config(_config)
        , messed_matrix_vars_since_reversal(true)
        , gauss_last_level(0)
        , gauss_starts_from(0)
        , useful_gaussian(0)
        , called_gaussian(0)
{
}

Gaussian::~Gaussian()
{
    clear_clauses();
}

void Gaussian::clear_clauses()
{
    BOOST_FOREACH(Clause* c, matrix_clauses_toclear)
        free(c);
    matrix_clauses_toclear.clear();
}

llbool Gaussian::full_init()
{
    assert(config.every_nth_gauss > 0);
    assert(config.only_nth_gauss_save >= config.every_nth_gauss);
    assert(config.only_nth_gauss_save % config.every_nth_gauss == 0); 
    assert(config.decision_from % config.every_nth_gauss == 0);
    assert(config.decision_from % config.only_nth_gauss_save == 0);
    
    if (!should_init()) return l_Nothing;
    
    bool do_again_gauss = true;
    while (do_again_gauss) {
        do_again_gauss = false;
        if (solver.simplify() != l_Undef) return l_False;
        init();
        Clause* confl;
        gaussian_ret g = gaussian(confl);
        switch (g) {
        case unit_conflict:
        case conflict:
            return l_False;
        case unit_propagation:
            do_again_gauss=true;
            break;
        case propagation:
        case nothing:
            break;
        }
    }
    
    if (at_first_init())
        print_matrix_stats();

    return l_Nothing;
}

void Gaussian::init(void)
{
    assert(solver.decisionLevel() == 0);

    fill_matrix(original_matrixset);
    if (original_matrixset.num_rows == 0) return;
    cur_matrixset = original_matrixset;

    gauss_last_level = solver.trail.size();
    messed_matrix_vars_since_reversal = false;
    if (config.decision_from > 0) went_below_decision_from = true;
    else went_below_decision_from = true;
    disable_gauss = false;

#ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")Gaussian init finished." << endl;
#endif
}

uint Gaussian::fill_var_to_col(matrixset& m) const
{
    m.var_to_col.resize(solver.nVars());
    uint largest_used_var = 0;
    BOOST_FOREACH(uint& w, m.var_to_col)
        w = UINT_MAX;

    uint num_xorclauses  = 0;
    for (int i = 0; i < solver.xorclauses.size(); i++) {
#ifdef DEBUG_GAUSS
        assert(!solver.satisfied(*solver.xorclauses[i]));
#endif
        if (solver.xorclauses[i]->inMatrix() && solver.xorclauses[i]->getMatrix() == matrix_no) {
            num_xorclauses++;
            XorClause& c = *solver.xorclauses[i];
            for (uint i2 = 0; i2 < c.size(); i2++) {
                assert(solver.assigns[c[i2].var()].isUndef());
                m.var_to_col[c[i2].var()] = 1;
                largest_used_var = std::max(largest_used_var, c[i2].var());
            }
        }
    }
    m.var_to_col.resize(largest_used_var + 1);

    m.col_to_var.clear();
    for (int i = solver.order_heap.size()-1; i >= 0 ; i--)
        //for inverse order:
        //for (int i = 0; i < order_heap.size() ; i++)
    {
        Var v = solver.order_heap[i];

        if (m.var_to_col[v] == 1) {
#ifdef DEBUG_GAUSS
            vector<uint>::iterator it =
                std::find(m.col_to_var.begin(), m.col_to_var.end(), v);
            assert(it == m.col_to_var.end());
#endif
            m.col_to_var.push_back(v);
            m.var_to_col[v] = 2;
        }
    }

    //for the ones that were not in the order_heap, but are marked in var_to_col
    for (uint i = 0; i < m.var_to_col.size(); i++) {
        if (m.var_to_col[i] == 1)
            m.col_to_var.push_back(i);
    }

#ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")col_to_var:";
    BOOST_FOREACH(const Var v, m.col_to_var) {
        cout << v << ",";
    }
    cout << endl;


    cout << "(" << matrix_no << ")var_to_col:" << endl;
#endif

    for (uint i = 0; i < m.var_to_col.size(); i++) {
        if (m.var_to_col[i] < UINT_MAX) {
            vector<uint>::iterator it = std::find(m.col_to_var.begin(), m.col_to_var.end(), i);
            assert(it != m.col_to_var.end());
            m.var_to_col[i] = it - m.col_to_var.begin();
#ifdef VERBOSE_DEBUG
            cout << "(" << matrix_no << ")var_to_col[" << i << "]:" << m.var_to_col[i] << endl;
#endif
        }
    }

    return num_xorclauses;
}

void Gaussian::fill_matrix(matrixset& m)
{
#ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")Filling matrix" << endl;
#endif

    m.num_rows = fill_var_to_col(m);
    m.num_cols = m.col_to_var.size();
    col_to_var_original = m.col_to_var;
    if (m.num_rows == 0) return;

    m.last_one_in_col.resize(m.num_cols);
    BOOST_FOREACH(uint& last, m.last_one_in_col) last = m.num_rows;
    m.removeable_cols = 0;
    m.least_column_changed = -1;
    m.matrix.resize(m.num_rows);
    m.varset.resize(m.num_rows);

#ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")matrix size:" << m.num_rows << "," << m.num_cols << endl;
#endif

    uint matrix_row = 0;
    for (int i = 0; i < solver.xorclauses.size(); i++) {
        const XorClause& c = *solver.xorclauses[i];

        if (c.inMatrix() &&  c.getMatrix() == matrix_no) {
            m.varset[matrix_row].set(c, m.var_to_col, m.col_to_var.size());
            m.matrix[matrix_row].set(c, m.var_to_col, m.col_to_var.size());
            matrix_row++;
        }
    }
    assert(m.num_rows == matrix_row);
}

void Gaussian::update_matrix_col(matrixset& m, const Var var, const uint col) const
{
#ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")Updating matrix var " << var+1 << endl;
#endif
    m.least_column_changed = std::min(m.least_column_changed, (int)col);
    matrix_row* this_row = &m.matrix[0];

    if (solver.assigns[var].getBool()) {
        for (uint i = 0, end = std::min(m.num_rows, m.last_one_in_col[col]+1);  i < end; i++, this_row++) {
            matrix_row& r = *this_row;
            if (r[col]) {
                r.invert_xor_clause_inverted();
                r.clearBit(col);
            }
        }
    } else {
        for (uint i = 0, end = std::min(m.num_rows, m.last_one_in_col[col]+1);  i < end; i++, this_row++) {
            //this_row->clearBit(col);
            matrix_row& r = *this_row;
            if (r[col]) {
                //r.invert_xor_clause_inverted();
                r.clearBit(col);
            }
        }
    }

#ifdef DEBUG_GAUSS
    bool c = false;
    BOOST_FOREACH(matrix_row& r, m.matrix)
        c |= r[col];
    assert(!c);
#endif

    m.removeable_cols++;
    m.col_to_var[col] = UINT_MAX;
    m.var_to_col[var] = UINT_MAX-1;
}

void Gaussian::update_matrix_by_col_all(matrixset& m) const
{
#ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")Updating matrix." << endl;
    uint num_updated = 0;
#endif
#ifdef DEBUG_GAUSS
    //assert(nothing_to_propagate(cur_matrixset));  //NOTE: only holds if gauss is executed at each level
#endif

    uint last = 0;
    uint* col_to_var_it = &m.col_to_var[0];
    for (uint col = 0, end = m.num_cols; col < end; col++, col_to_var_it++) {
        if (*col_to_var_it != UINT_MAX && solver.assigns[*col_to_var_it].isDef()) {
            update_matrix_col(m, *col_to_var_it, col);
            last++;
        } else
            last = 0;
    }
    m.num_cols -= last;
    m.col_to_var.resize(m.num_cols);
    m.last_one_in_col.resize(m.num_cols);
    
#ifdef DEBUG_GAUSS
    check_matrix_against_varset(m.matrix, m.varset);
#endif

#ifdef VERBOSE_DEBUG
    cout << "Matrix update finished, updated " << num_updated << " cols" << endl;
#endif
    
    /*cout << "num_rows:" << m.num_rows;
    cout << " num_rows diff:" << original_matrixset.num_rows - m.num_rows << endl;
    cout << "num_cols:" << col_to_var_original.size();
    cout << " num_cols diff:" << col_to_var_original.size() - m.col_to_var.size() << endl;
    cout << "removeable cols:" << m.removeable_cols << endl;*/
}

Gaussian::gaussian_ret Gaussian::gaussian(Clause*& confl)
{
    if (original_matrixset.num_rows == 0) return nothing;

    if (!messed_matrix_vars_since_reversal) {
#ifdef VERBOSE_DEBUG
        cout << "(" << matrix_no << ")matrix needs only update" << endl;
#endif
        update_matrix_by_col_all(cur_matrixset);
    } else {
#ifdef VERBOSE_DEBUG
        cout << "(" << matrix_no << ")matrix needs copy&update" << endl;
#endif
        if (went_below_decision_from)
            cur_matrixset = original_matrixset;
        else
            cur_matrixset = matrix_sets[((solver.decisionLevel() - config.decision_from) / config.only_nth_gauss_save)];
        update_matrix_by_col_all(cur_matrixset);
    }

    messed_matrix_vars_since_reversal = false;
    gauss_last_level = solver.trail.size();

    propagatable_rows.clear();
    uint conflict_row = UINT_MAX;
    uint row = eliminate(cur_matrixset, propagatable_rows, conflict_row);
#ifdef DEBUG_GAUSS
    check_matrix_against_varset(cur_matrixset.matrix, cur_matrixset.varset);
#endif
    gaussian_ret ret;
    if (conflict_row != UINT_MAX) {
        uint maxlevel = UINT_MAX;
        uint size = UINT_MAX;
        uint best_row = UINT_MAX;
        analyse_confl(cur_matrixset, conflict_row, maxlevel, size, best_row);
        ret = handle_matrix_confl(confl, cur_matrixset, size, maxlevel, best_row);
        
    } else {
        ret = handle_matrix_prop_and_confl(cur_matrixset, row, propagatable_rows, confl);
    }
    
    if (ret == nothing
        && (solver.decisionLevel() == 0 || ((solver.decisionLevel() - config.decision_from) % config.only_nth_gauss_save == 0))
       )
        set_matrixset_to_cur();

    called_gaussian++;
    if (ret != nothing) useful_gaussian++;

#ifdef VERBOSE_DEBUG
    if (ret == nothing) cout << "(" << matrix_no << ")Useless. ";
    else cout << "(" << matrix_no << ")Useful. ";

    cout << "(" << matrix_no << ")Useful in " << ((double)useful_gaussian/(double)called_gaussian)*100.0 << "%" << endl;
#endif

    return ret;
}

uint Gaussian::eliminate(matrixset& m, vec<uint>& propagatable_rows, uint& conflict_row) const
{
    if (m.least_column_changed == INT_MAX)
        return m.num_rows;

#ifdef VERBOSE_DEBUG
    uint number_of_row_additions = 0;
    uint no_exchanged = 0;
#endif

    uint i = 0;
    uint j = 0;

    if (m.least_column_changed > -1) {
        const uint until = m.last_one_in_col[m.least_column_changed];
        while (i < until) {
            const bool propagatable = m.matrix[i].popcnt_is_one();
            if (propagatable)
                propagatable_rows.push(i);
            i++;
        }

        j = m.least_column_changed + 1;
    }

    while (i < m.num_rows && j < m.num_cols) {
        //Find pivot in column j, starting in row i:

        if (m.col_to_var[j] == UINT_MAX) {
            j++;
            continue;
        }

        uint best_row = UINT_MAX;
        //uint best_row_popcnt = UINT_MAX;
        uint end_investigate = std::min(m.last_one_in_col[j] + 1, m.num_rows);
        rows_with_one.clear();

        matrix_row* this_matrix_row = &m.matrix[0] + i;
        for (uint i2 = i; i2 < end_investigate; i2++, this_matrix_row++) {
            //intelligent pivoting
            if ((*this_matrix_row)[j]) {
                //uint popcnt = this_matrix_row->popcnt();
                //if (popcnt < best_row_popcnt) {
                //    best_row = i2;
                //    best_row_popcnt = popcnt;
                //}
                best_row = i2;
                rows_with_one.push(i2);
            }
        }

        if (best_row < m.num_rows) {
            matrix_row& matrix_row_i = m.matrix[i];
            matrix_row& varset_row_i = m.varset[i];

            //swap rows i and maxi, but do not change the value of i;
            if (i != best_row) {
#ifdef VERBOSE_DEBUG
                no_exchanged++;
#endif
                if (matrix_row_i.isZero() && !matrix_row_i.get_xor_clause_inverted()) {
                    conflict_row = i;
                    return 0;
                }
                matrix_row_i.swap(m.matrix[best_row]);
                varset_row_i.swap(m.varset[best_row]);
            }
#ifdef DEBUG_GAUSS
            matrix_row backup = m.matrix[i];
            assert(m.matrix[i][j]);
#endif

            if (m.matrix[i].popcnt_is_one(j))
                propagatable_rows.push(i);

            //Now A[i,j] will contain the old value of A[maxi,j];
            uint* real_it = rows_with_one.getData();
            bool original_was_good = false;
            if (*real_it == i) {
                real_it ++;
                original_was_good = true;
            }

            for (const uint* end = rows_with_one.getData() + rows_with_one.size(); real_it != end; real_it++) {
                const uint& u = *real_it;
                if (original_was_good || u != best_row) {
#ifdef DEBUG_GAUSS
                    assert( u != i );
                    assert(m.matrix[u][j]);
#endif
                    //subtract row i from row u;
                    //Now A[u,j] will be 0, since A[u,j] - A[i,j] = A[u,j] -1 = 0.
#ifdef VERBOSE_DEBUG
                    number_of_row_additions++;
#endif
                    m.matrix[u] ^= matrix_row_i;
                    m.varset[u] ^= varset_row_i;
                    //Would early abort, but would not find the best conflict:
                    //if (!m.matrix[u].get_xor_clause_inverted() && m.matrix[u].isZero()) {
                    //    conflict_row = u;
                    //    return 0;
                    //}
                }
            }
#ifdef DEBUG_GAUSS
            assert(m.matrix[i] == backup);
#endif

            m.last_one_in_col[j] = i;
            i++;
        } else
            m.last_one_in_col[j] = i;
        j++;
    }

    while (j < m.num_cols) {
        m.last_one_in_col[j] = i;
        j++;
    }

    m.least_column_changed = INT_MAX;

#ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")last one in col:";
    BOOST_FOREACH(const uint& r, m.last_one_in_col)
        cout << r << ",";
    cout << endl;
    cout << "(" << matrix_no << ")Exchanged:" << no_exchanged << " row additions:" << number_of_row_additions << endl;
#endif

    return i;
}

Gaussian::gaussian_ret Gaussian::handle_matrix_confl(Clause*& confl, const matrixset& m, const uint size, const uint maxlevel, const uint best_row)
{
    assert(best_row != UINT_MAX);

    confl = Clause_new(m.varset[best_row], solver.assigns, col_to_var_original, solver.learnt_clause_group++);
    Clause& cla = *confl;
    if (cla.size() <= 1)
        return unit_conflict;

    assert(cla.size() >= 2);
#ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")Found conflict:";
    solver.printClause(cla);
#endif

    if (maxlevel != solver.decisionLevel()) solver.cancelUntil(maxlevel);
    const uint curr_dec_level = solver.decisionLevel();
    assert(maxlevel == curr_dec_level);
    
    uint maxsublevel = 0;
    uint maxsublevel_at = UINT_MAX;
    for (uint i = 0, size = cla.size(); i < size; i++) if (solver.level[cla[i].var()] == curr_dec_level) {
        uint tmp = find_sublevel(cla[i].var());
        if (tmp >= maxsublevel) {
            maxsublevel = tmp;
            maxsublevel_at = i;
        }
    }
#ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ") || Sublevel of confl: " << maxsublevel << " (due to var:" << cla[maxsublevel_at].var()-1 << ")" << endl;
#endif
    
    Lit tmp(cla[maxsublevel_at]);
    cla[maxsublevel_at] = cla[1];
    cla[1] = tmp;

    cancel_until_sublevel(maxsublevel+1);
    messed_matrix_vars_since_reversal = true;
    return conflict;
}

Gaussian::gaussian_ret Gaussian::handle_matrix_prop_and_confl(matrixset& m, uint row, const vec<uint>& propagatable_rows, Clause*& confl)
{
    uint maxlevel = UINT_MAX;
    uint size = UINT_MAX;
    uint best_row = UINT_MAX;

    const uint end_interesting_rows = row;
    while (row < m.num_rows) {
#ifdef DEBUG_GAUSS
        assert(m.matrix[row].isZero());
#endif
        if (!m.matrix[row].get_xor_clause_inverted())
            analyse_confl(m, row, maxlevel, size, best_row);
        row++;
    }

    if (maxlevel != UINT_MAX)
        return handle_matrix_confl(confl, m, size, maxlevel, best_row);
#ifdef DEBUG_GAUSS
    assert(check_no_conflict(m));
#endif
    m.num_rows = end_interesting_rows;
    m.matrix.resize(m.num_rows);
    m.varset.resize(m.num_rows);

    gaussian_ret ret = nothing;

    uint num_props = 0;
    const uint* prop_row = propagatable_rows.getData();
    const uint* end = prop_row + propagatable_rows.size();
    for (; prop_row != end; prop_row++ ) {
        //this is a "000..1..0000000X" row. I.e. it indicates a propagation
        ret = handle_matrix_prop(m, *prop_row);
        num_props++;
        if (ret == unit_propagation) {
#ifdef VERBOSE_DEBUG
            cout << "(" << matrix_no << ")Unit prop! Breaking from prop examination" << endl;
#endif
            return  unit_propagation;
        }
    }
#ifdef VERBOSE_DEBUG
    if (num_props > 0) cout << "(" << matrix_no << ")Number of props during gauss:" << num_props << endl;
#endif

    return ret;
}

uint Gaussian::find_sublevel(const Var v) const
{
    for (int i = solver.trail.size()-1; i >= 0; i --)
        if (solver.trail[i].var() == v) return i;
#ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")Oooops! Var " << v+1 << " does not have a sublevel!! (so it must be undefined)" << endl;
#endif
    assert(false);
    return 0;
}

void Gaussian::cancel_until_sublevel(const uint sublevel)
{
#ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")Canceling until sublevel " << sublevel << endl;
#endif

    for (int level = solver.trail.size()-1; level >= sublevel; level--) {
        Var     var  = solver.trail[level].var();
#ifdef VERBOSE_DEBUG
        cout << "(" << matrix_no << ")Canceling var " << var+1 << endl;
#endif

        solver.assigns[var] = l_Undef;
        solver.insertVarOrder(var);
        BOOST_FOREACH(Gaussian* gauss, solver.gauss_matrixes) {
            if (gauss != this) gauss->canceling(level, var);
        }
    }
    solver.trail.shrink(solver.trail.size() - sublevel);
#ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")Canceling sublevel finished." << endl;
#endif
}

void Gaussian::analyse_confl(const matrixset& m, const uint row, uint& maxlevel, uint& size, uint& best_row) const
{
    assert(row < m.num_rows);

    //this is a "000...00000001" row. I.e. it indicates we are on the wrong branch
#ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")matrix conflict found!" << endl;
    cout << "(" << matrix_no << ")conflict clause's vars: ";
    print_matrix_row_with_assigns(m.varset[row]);
    cout << endl;
    
    cout << "(" << matrix_no << ")corresponding matrix's row (should be empty): ";
    print_matrix_row(m.matrix[row]);
    cout << endl;
    
#endif

    uint this_maxlevel = 0;
    unsigned long int var = 0;
    uint this_size = 0;
    while (true) {
        var = m.varset[row].scan(var);
        if (var == ULONG_MAX) break;

        const uint real_var = col_to_var_original[var];
        assert(real_var < solver.nVars());

        if (solver.level[real_var] > this_maxlevel)
            this_maxlevel = solver.level[real_var];
        var++;
        this_size++;
    }

    //the maximum of all lit's level must be lower than the max. level of the current best clause (or this clause must be either empty or unit clause)
    if (!(
                (this_maxlevel < maxlevel)
                || (this_maxlevel == maxlevel && this_size < size)
                || (this_size <= 1)
            )) {
        assert(maxlevel != UINT_MAX);
#ifdef VERBOSE_DEBUG
        cout << "(" << matrix_no << ")Other found conflict just as good or better.";
        cout << "(" << matrix_no << ") || Old maxlevel:" << maxlevel << " new maxlevel:" << this_maxlevel;
        cout << "(" << matrix_no << ") || Old size:" << size << " new size:" << this_size << endl;
        //assert(!(maxlevel != UINT_MAX && maxlevel != this_maxlevel)); //NOTE: only holds if gauss is executed at each level
#endif
        return;
    }


#ifdef VERBOSE_DEBUG
    if (maxlevel != UINT_MAX)
        cout << "(" << matrix_no << ")Better conflict found.";
    else
        cout << "(" << matrix_no << ")Found a possible conflict.";

    cout << "(" << matrix_no << ") || Old maxlevel:" << maxlevel << " new maxlevel:" << this_maxlevel;
    cout << "(" << matrix_no << ") || Old size:" << size << " new size:" << this_size << endl;
    //assert(!(maxlevel != UINT_MAX && maxlevel != this_maxlevel)); //NOTE: only holds if gauss is executed at each level
#endif

    maxlevel = this_maxlevel;
    size = this_size;
    best_row = row;
}

Gaussian::gaussian_ret Gaussian::handle_matrix_prop(matrixset& m, const uint row)
{
#ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")matrix prop found!" << endl;
    cout << "(" << matrix_no << ")matrix row:";
    print_matrix_row(m.matrix[row]);
    cout << endl;
#endif

    Clause& cla = *Clause_new(m.varset[row], solver.assigns, col_to_var_original, solver.learnt_clause_group++);
#ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")matrix prop clause: ";
    solver.printClause(cla);
    cout << endl;
#endif
    assert(m.matrix[row][m.var_to_col[cla[0].var()]]);
    assert(!m.matrix[row].get_xor_clause_inverted() == !cla[0].sign());
    assert(solver.assigns[cla[0].var()].isUndef());
    if (cla.size() == 1) {
        const Lit lit = cla[0];
        solver.cancelUntil(0);
        solver.uncheckedEnqueue(lit);
        free(&cla);
        return unit_propagation;
    }

    matrix_clauses_toclear.push_back(&cla);
    solver.uncheckedEnqueue(cla[0], &cla);
    if (solver.dynamic_behaviour_analysis)
        solver.logger.propagation(cla[0], Logger::gauss_propagation_type, cla.group);

    return propagation;
}

/*void Gaussian::check_to_disable()
{
    if (nof_conflicts >= 0
    && conflictC >= nof_conflicts/5
    && called_gaussian > 0
    && (double)useful_gaussian/(double)called_gaussian < 0.05)
    disable_gauss = true;
}*/

llbool Gaussian::find_truths(vec<Lit>& learnt_clause, int& conflictC)
{
    Clause* confl;

    if (should_check_gauss(solver.decisionLevel(), solver.starts)) {
        //check_to_disable();
        gaussian_ret g = gaussian(confl);
        switch (g) {
        case conflict: {
            llbool ret = solver.handle_conflict(learnt_clause, confl, conflictC);
            free(confl);
            if (ret != l_Nothing) return ret;
            return l_Continue;
        }
        case propagation:
        case unit_propagation:
            return l_Continue;
        case unit_conflict: {
            if (confl->size() == 0) {
                free(confl);
                return l_False;
            }

            Lit lit = (*confl)[0];
            solver.cancelUntil(0);
            if (solver.assigns[lit.var()].isDef()) {
                free(confl);
                return l_False;
            }
            solver.uncheckedEnqueue(lit);
            free(confl);
            return l_Continue;
        }
        case nothing:
            break;
        }
    }

    return l_Nothing;
}

template<class T>
void Gaussian::print_matrix_row(const T& row) const
{
    unsigned long int var = 0;
    while (true) {
        var = row.scan(var);
        if (var == ULONG_MAX) break;

        else cout << col_to_var_original[var]+1 << ", ";
        var++;
    }
    if (row.get_xor_clause_inverted()) cout << "xor_clause_inverted";
}

template<class T>
void Gaussian::print_matrix_row_with_assigns(const T& row) const
{
    unsigned long int col = 0;
    while (true) {
        col = row.scan(col);
        if (col == ULONG_MAX) break;
        
        else {
            Var var = col_to_var_original[col];
            cout << var+1 << "(" << lbool_to_string(solver.assigns[var]) << ")";
            cout << ", ";
        }
        col++;
    }
    if (row.get_xor_clause_inverted()) cout << "xor_clause_inverted";
}

const string Gaussian::lbool_to_string(const lbool toprint)
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
    if (called_gaussian > 0) {
        cout.setf(std::ios::fixed);
        std::cout << " Gauss(" << matrix_no << ") useful " << std::setprecision(2) << std::setw(5) << ((double)useful_gaussian/(double)called_gaussian)*100.0 << "% ";
        if (disable_gauss) std::cout << "disabled";
    } else
        std::cout << " Gauss(" << matrix_no << ") not called.";
}

void Gaussian::reset_stats()
{
    useful_gaussian = 0;
    called_gaussian = 0;
}

bool Gaussian::check_no_conflict(const matrixset& m) const
{
    BOOST_FOREACH(const matrix_row& r, m.matrix) {
        if (!r.get_xor_clause_inverted() && r.isZero())
            return false;
    }
    return true;
}

const bool Gaussian::nothing_to_propagate(const matrixset& m) const
{
    BOOST_FOREACH(const matrix_row& r, m.matrix) {
        if (r.popcnt_is_one()
                && solver.assigns[m.col_to_var[r.scan(0)]].isUndef())
            return false;
    }
    BOOST_FOREACH(const matrix_row& r, m.matrix) {
        if (r.isZero()
                && !r.get_xor_clause_inverted())
            return false;
    }
    return true;
}

const bool Gaussian::check_matrix_against_varset(const vector<matrix_row>& matrix, const vector<matrix_row>& varset) const
{
    assert(matrix.size() == varset.size());
    
    for (uint i = 0; i < matrix.size(); i++) {
        const matrix_row& mat_row = matrix[i];
        const matrix_row& var_row = varset[i];
        
        unsigned long int col = 0;
        bool final = false;
        while (true) {
            col = var_row.scan(col);
            if (col == ULONG_MAX) break;
            
            const Var var = col_to_var_original[col];
            assert(var < solver.nVars());
            
            if (solver.assigns[var] == l_True) {
                assert(!mat_row[col]);
                final = !final;
            } else if (solver.assigns[var] == l_False) {
                assert(!mat_row[col]);
            } else if (solver.assigns[var] == l_Undef) {
                assert(mat_row[col]);
            } else assert(false);
            
            col++;
        }
        if (final^mat_row.get_xor_clause_inverted() != var_row.get_xor_clause_inverted()) {
            cout << "problem with row:"; print_matrix_row_with_assigns(var_row); cout << endl;
            assert(false);
        }
    }
}

//old functions

/*void Gaussian::update_matrix_by_row(matrixset& m) const
{
#ifdef VERBOSE_DEBUG
    cout << "Updating matrix." << endl;
    uint num_updated = 0;
#endif
#ifdef DEBUG_GAUSS
    assert(nothing_to_propagate(cur_matrixset));
#endif

    mpz_class toclear, tocount;
    uint last_col = 0;

    for (uint col = 0; col < m.num_cols; col ++) {
        Var var = m.col_to_var[col];

        if (var != UINT_MAX && !solver.assigns[var].isUndef()) {
            toclear.setBit(col);
            if (solver.assigns[var].getBool()) tocount.setBit(col);

#ifdef DEBUG_GAUSS
            assert(m.var_to_col[var] < UINT_MAX-1);
#endif
            last_col = col;
            m.least_column_changed = std::min(m.least_column_changed, (int)col);

            m.removeable_cols++;
            m.col_to_var[col] = UINT_MAX;
            m.var_to_col[var] = UINT_MAX-1;
#ifdef VERBOSE_DEBUG
            num_updated++;
#endif
        }
    }

    toclear.invert();
    mpz_class tmp;
    mpz_class* this_row = &m.matrix[0];
    for(uint i = 0, until = std::min(m.num_rows, m.last_one_in_col[last_col]+1); i < until; i++, this_row++) {
        mpz_class& r = *this_row;
        mpz_and(tmp.get_mp(), tocount.get_mp(), r.get_mp());
        r.invert_xor_clause_inverted(tmp.popcnt() % 2);
        r &= toclear;
}

#ifdef VERBOSE_DEBUG
    cout << "Updated " << num_updated << " matrix cols. Could remove " << m.removeable_cols << " cols " <<endl;
#endif
}*/

/*void Gaussian::update_matrix_by_col(matrixset& m, const uint last_level) const
{
#ifdef VERBOSE_DEBUG
    cout << "Updating matrix." << endl;
    uint num_updated = 0;
#endif
#ifdef DEBUG_GAUSS
    assert(nothing_to_propagate(cur_matrixset));
#endif

    for (int level = solver.trail.size()-1; level >= last_level; level--){
        Var var = solver.trail[level].var();
        const uint col = m.var_to_col[var];
        if ( col < UINT_MAX-1) {
            update_matrix_col(m, var, col);
#ifdef VERBOSE_DEBUG
            num_updated++;
#endif
        }
    }

#ifdef VERBOSE_DEBUG
    cout << "Updated " << num_updated << " matrix cols. Could remove " << m.removeable_cols << " cols (out of " << m.num_cols << " )" <<endl;
#endif
}*/