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

#include "Gaussian.h"

#include <iostream>
#include <iomanip>
#include "Clause.h"
#include <algorithm>
using std::ostream;
using std::cout;
using std::endl;

#ifdef VERBOSE_DEBUG
#include <iterator>
#endif

ostream& operator << (ostream& os, const vec<Lit>& v)
{
    for (int i = 0; i < v.size(); i++) {
        if (v[i].sign()) os << "-";
        os << v[i].var()+1 << " ";
    }

    return os;
}

Gaussian::Gaussian(Solver& _solver, const GaussianConfig& _config, const uint _matrix_no) :
        solver(_solver)
        , config(_config)
        , matrix_no(_matrix_no)
        , messed_matrix_vars_since_reversal(true)
        , gauss_last_level(0)
        , disabled(false)
        , useful_prop(0)
        , useful_confl(0)
        , called(0)
{
}

Gaussian::~Gaussian()
{
    clear_clauses();
}

void Gaussian::clear_clauses()
{
    std::for_each(matrix_clauses_toclear.begin(), matrix_clauses_toclear.end(), std::ptr_fun(free));
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
        solver.removeSatisfied(solver.xorclauses);
        solver.cleanClauses(solver.xorclauses);
        init();
        Clause* confl;
        gaussian_ret g = gaussian(confl);
        switch (g) {
        case unit_conflict:
        case conflict:
            return l_False;
        case unit_propagation:
        case propagation:
            do_again_gauss=true;
            if (solver.propagate() != NULL) return l_False;
            break;
        case nothing:
            break;
        }
    }

    return l_Nothing;
}

void Gaussian::init(void)
{
    assert(solver.decisionLevel() == 0);

    matrix_sets.clear();
    fill_matrix();
    if (origMat.num_rows == 0) return;
    
    cur_matrixset = origMat;

    gauss_last_level = solver.trail.size();
    messed_matrix_vars_since_reversal = false;
    if (config.decision_from > 0) went_below_decision_from = true;
    else went_below_decision_from = true;

    #ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")Gaussian init finished." << endl;
    #endif
}

uint Gaussian::select_columnorder(vector<uint16_t>& var_to_col)
{
    var_to_col.resize(solver.nVars(), unassigned_col);

    uint largest_used_var = 0;
    uint num_xorclauses  = 0;
    for (int i = 0; i < solver.xorclauses.size(); i++) {
        #ifdef DEBUG_GAUSS
        assert(!solver.satisfied(*solver.xorclauses[i]));
        #endif
        if (solver.xorclauses[i]->getMatrix() == matrix_no) {
            num_xorclauses++;
            XorClause& c = *solver.xorclauses[i];
            for (uint i2 = 0; i2 < c.size(); i2++) {
                assert(solver.assigns[c[i2].var()].isUndef());
                var_to_col[c[i2].var()] = 1;
                largest_used_var = std::max(largest_used_var, c[i2].var());
            }
        }
    }
    var_to_col.resize(largest_used_var + 1);

    origMat.col_to_var.clear();
    for (int i = solver.order_heap.size()-1; i >= 0 ; i--)
        //for inverse order:
        //for (int i = 0; i < order_heap.size() ; i++)
    {
        Var v = solver.order_heap[i];

        if (var_to_col[v] == 1) {
            #ifdef DEBUG_GAUSS
            vector<uint>::iterator it =
                std::find(origMat.col_to_var.begin(), origMat.col_to_var.end(), v);
            assert(it == origMat.col_to_var.end());
            #endif
            
            origMat.col_to_var.push_back(v);
            var_to_col[v] = 2;
        }
    }

    //for the ones that were not in the order_heap, but are marked in var_to_col
    for (uint i = 0; i < var_to_col.size(); i++) {
        if (var_to_col[i] == 1)
            origMat.col_to_var.push_back(i);
    }

    #ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")col_to_var:";
    std::copy(origMat.col_to_var.begin(), origMat.col_to_var.end(), std::ostream_iterator<uint>(cout, ","));
    cout << endl;

    cout << "(" << matrix_no << ")var_to_col:" << endl;
    #endif

    var_is_in.resize(var_to_col.size());
    var_is_in.setZero();
    origMat.var_is_set.resize(var_to_col.size());
    origMat.var_is_set.setZero();
    for (uint i = 0; i < var_to_col.size(); i++) {
        if (var_to_col[i] != unassigned_col) {
            vector<uint>::iterator it = std::find(origMat.col_to_var.begin(), origMat.col_to_var.end(), i);
            assert(it != origMat.col_to_var.end());
            var_to_col[i] = &(*it) - &origMat.col_to_var[0];
            var_is_in.setBit(i);
            #ifdef VERBOSE_DEBUG
            cout << "(" << matrix_no << ")var_to_col[" << i << "]:" << var_to_col[i] << endl;
            #endif
        }
    }

    return num_xorclauses;
}

void Gaussian::fill_matrix()
{
    #ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")Filling matrix" << endl;
    #endif

    vector<uint16_t> var_to_col;
    origMat.num_rows = select_columnorder(var_to_col);
    origMat.num_cols = origMat.col_to_var.size();
    col_to_var_original = origMat.col_to_var;
    changed_rows.resize(origMat.num_rows);
    changed_rows.setZero();
    if (origMat.num_rows == 0) return;

    origMat.last_one_in_col.resize(origMat.num_cols);
    std::fill(origMat.last_one_in_col.begin(), origMat.last_one_in_col.end(), origMat.num_rows);
    origMat.past_the_end_last_one_in_col = origMat.num_cols;
    
    origMat.removeable_cols = 0;
    origMat.least_column_changed = -1;
    origMat.matrix.resize(origMat.num_rows, origMat.num_cols);
    origMat.varset.resize(origMat.num_rows, origMat.num_cols);

    #ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")matrix size:" << origMat.num_rows << "," << origMat.num_cols << endl;
    #endif

    uint matrix_row = 0;
    for (int i = 0; i < solver.xorclauses.size(); i++) {
        const XorClause& c = *solver.xorclauses[i];

        if (c.getMatrix() == matrix_no) {
            origMat.varset[matrix_row].set(c, var_to_col, origMat.num_cols);
            origMat.matrix[matrix_row].set(c, var_to_col, origMat.num_cols);
            matrix_row++;
        }
    }
    assert(origMat.num_rows == matrix_row);
}

void Gaussian::update_matrix_col(matrixset& m, const Var var, const uint col)
{
    #ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")Updating matrix var " << var+1 << " (col " << col << ", m.last_one_in_col[col]: " << m.last_one_in_col[col] << ")" << endl;
    cout << "m.num_rows:" << m.num_rows << endl;
    #endif
    
    #ifdef DEBUG_GAUSS
    assert(col < m.num_cols);
    #endif
    
    m.least_column_changed = std::min(m.least_column_changed, (int)col);
    PackedMatrix::iterator this_row = m.matrix.begin();
    uint row_num = 0;

    if (solver.assigns[var].getBool()) {
        for (PackedMatrix::iterator end = this_row + std::min(m.last_one_in_col[col], m.num_rows);  this_row != end; ++this_row, row_num++) {
            PackedRow r = *this_row;
            if (r[col]) {
                changed_rows.setBit(row_num);
                r.invert_xor_clause_inverted();
                r.clearBit(col);
            }
        }
    } else {
        for (PackedMatrix::iterator end = this_row + std::min(m.last_one_in_col[col], m.num_rows);  this_row != end; ++this_row, row_num++) {
            PackedRow r = *this_row;
            if (r[col]) {
                changed_rows.setBit(row_num);
                r.clearBit(col);
            }
        }
    }

    #ifdef DEBUG_GAUSS
    bool c = false;
    for(PackedMatrix::iterator r = m.matrix.begin(), end = r + m.matrix.size(); r != end; ++r)
        c |= (*r)[col];
    assert(!c);
    #endif

    m.removeable_cols++;
    m.col_to_var[col] = unassigned_var;
    m.var_is_set.setBit(var);
}

void Gaussian::update_matrix_by_col_all(matrixset& m)
{
    #ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")Updating matrix." << endl;
    print_matrix(m);
    uint num_updated = 0;
    #endif
    
    #ifdef DEBUG_GAUSS
    assert(config.every_nth_gauss != 1 || nothing_to_propagate(cur_matrixset));
    assert(solver.decisionLevel() == 0 || check_last_one_in_cols(m));
    #endif
    
    changed_rows.setZero();

    uint last = 0;
    uint col = 0;
    for (Var *it = &m.col_to_var[0], *end = it + m.num_cols; it != end; col++, it++) {
        if (*it != unassigned_var && solver.assigns[*it].isDef()) {
            update_matrix_col(m, *it, col);
            last++;
            #ifdef VERBOSE_DEBUG
            num_updated++;
            #endif
        } else
            last = 0;
    }
    m.num_cols -= last;
    m.past_the_end_last_one_in_col -= last;
    
    #ifdef DEBUG_GAUSS
    check_matrix_against_varset(m.matrix, m.varset);
    #endif

    #ifdef VERBOSE_DEBUG
    cout << "Matrix update finished, updated " << num_updated << " cols" << endl;
    print_matrix(m);
    #endif
    
    /*cout << "num_rows:" << m.num_rows;
    cout << " num_rows diff:" << origMat.num_rows - m.num_rows << endl;
    cout << "num_cols:" << col_to_var_original.size();
    cout << " num_cols diff:" << col_to_var_original.size() - m.col_to_var.size() << endl;
    cout << "removeable cols:" << m.removeable_cols << endl;*/
}

Gaussian::gaussian_ret Gaussian::gaussian(Clause*& confl)
{
    if (origMat.num_rows == 0) return nothing;

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
            cur_matrixset = origMat;
        else
            cur_matrixset = matrix_sets[((solver.decisionLevel() - config.decision_from) / config.only_nth_gauss_save)];
        
        update_matrix_by_col_all(cur_matrixset);
    }
    if (!cur_matrixset.num_cols || !cur_matrixset.num_cols)
        return nothing;

    messed_matrix_vars_since_reversal = false;
    gauss_last_level = solver.trail.size();
    went_below_decision_from = false;

    propagatable_rows.clear();
    
    uint conflict_row = UINT_MAX;
    uint last_row = eliminate(cur_matrixset, conflict_row);
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
        ret = handle_matrix_prop_and_confl(cur_matrixset, last_row, confl);
    }
    
    if (ret == nothing
        && (solver.decisionLevel() == 0 || ((solver.decisionLevel() - config.decision_from) % config.only_nth_gauss_save == 0))
       )
        set_matrixset_to_cur();

    #ifdef VERBOSE_DEBUG
    if (ret == nothing)
        cout << "(" << matrix_no << ")Useless. ";
    else
        cout << "(" << matrix_no << ")Useful. ";
    cout << "(" << matrix_no << ")Useful prop in " << ((double)useful_prop/(double)called)*100.0 << "%" << endl;
    cout << "(" << matrix_no << ")Useful confl in " << ((double)useful_confl/(double)called)*100.0 << "%" << endl;
    #endif

    return ret;
}

uint Gaussian::eliminate(matrixset& m, uint& conflict_row)
{
    #ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")";
    cout << "Starting elimination" << endl;
    cout << "m.least_column_changed:" << m.least_column_changed << endl;
    print_last_one_in_cols(m);
    
    uint number_of_row_additions = 0;
    uint no_exchanged = 0;
    #endif
    
    if (m.least_column_changed == INT_MAX) {
        #ifdef VERBOSE_DEBUG
        cout << "Nothing to eliminate" << endl;
        #endif
        
        return m.num_rows;
    }
    
    
    #ifdef DEBUG_GAUSS
    assert(solver.decisionLevel() == 0 || check_last_one_in_cols(m));
    #endif

    uint i = 0;
    uint j = m.least_column_changed + 1;

    if (j) {
        uint16_t until = std::min(m.last_one_in_col[m.least_column_changed] - 1, (int)m.num_rows);
        if (j > m.past_the_end_last_one_in_col)
            until = m.num_rows;
        for (;i != until; i++) if (changed_rows[i] && m.matrix[i].popcnt_is_one())
            propagatable_rows.push(i);
    }
    
    if (j > m.past_the_end_last_one_in_col) {
        #ifdef VERBOSE_DEBUG
        cout << "Going straight to finish" << endl;
        #endif
        goto finish;
    }
    
    #ifdef VERBOSE_DEBUG
    cout << "At while() start: i,j = " << i << ", " << j << endl;
    #endif
    
    #ifdef DEBUG_GAUSS
    assert(i <= m.num_rows && j <= m.num_cols);
    #endif

    while (i != m.num_rows && j != m.num_cols) {
        //Find pivot in column j, starting in row i:

        if (m.col_to_var[j] == unassigned_var) {
            j++;
            continue;
        }

        uint best_row = i;
        PackedMatrix::iterator this_matrix_row = m.matrix.begin() + i;
        PackedMatrix::iterator end = m.matrix.begin() + std::min(m.last_one_in_col[j], m.num_rows);
        for (; this_matrix_row != end; ++this_matrix_row, best_row++) {
            if ((*this_matrix_row)[j])
                break;
        }

        if (this_matrix_row != end) {
            PackedRow matrix_row_i = m.matrix[i];
            PackedRow varset_row_i = m.varset[i];
            PackedMatrix::iterator this_varset_row = m.varset.begin() + best_row;

            //swap rows i and maxi, but do not change the value of i;
            if (i != best_row) {
                #ifdef VERBOSE_DEBUG
                no_exchanged++;
                #endif
                
                if (!matrix_row_i.get_xor_clause_inverted() && matrix_row_i.isZero()) {
                    conflict_row = i;
                    return 0;
                }
                matrix_row_i.swap(*this_matrix_row);
                varset_row_i.swap(*this_varset_row);
            }
            #ifdef DEBUG_GAUSS
            assert(m.matrix[i].popcnt(j) == m.matrix[i].popcnt());
            assert(m.matrix[i][j]);
            #endif

            if (matrix_row_i.popcnt_is_one(j))
                propagatable_rows.push(i);

            //Now A[i,j] will contain the old value of A[maxi,j];
            ++this_matrix_row;
            ++this_varset_row;
            for (; this_matrix_row != end; ++this_matrix_row, ++this_varset_row) if ((*this_matrix_row)[j]) {
                //subtract row i from row u;
                //Now A[u,j] will be 0, since A[u,j] - A[i,j] = A[u,j] -1 = 0.
                #ifdef VERBOSE_DEBUG
                number_of_row_additions++;
                #endif
                
                *this_matrix_row ^= matrix_row_i;
                *this_varset_row ^= varset_row_i;
                //Would early abort, but would not find the best conflict:
                //if (!it->get_xor_clause_inverted() &&it->isZero()) {
                //    conflict_row = i2;
                //    return 0;
                //}
            }
            i++;
            m.last_one_in_col[j] = i;
        } else
            m.last_one_in_col[j] = i + 1;
        j++;
    }

    m.past_the_end_last_one_in_col = j;
    
    finish:

    m.least_column_changed = INT_MAX;

    #ifdef VERBOSE_DEBUG
    cout << "Finished elimination" << endl;
    cout << "Returning with i,j:" << i << ", " << j << "(" << m.num_rows << ", " << m.num_cols << ")" << endl;
    print_matrix(m);
    print_last_one_in_cols(m);
    cout << "(" << matrix_no << ")Exchanged:" << no_exchanged << " row additions:" << number_of_row_additions << endl;
    #endif
    
    #ifdef DEBUG_GAUSS
    assert(check_last_one_in_cols(m));
    uint row = 0;
    uint col = 0;
    for (; col < m.num_cols && row < m.num_rows && row < i && col < m.past_the_end_last_one_in_col; col++) {
        assert(m.matrix[row].popcnt() == m.matrix[row].popcnt(col));
        assert(!(m.col_to_var[col] == unassigned_var && m.matrix[row][col]));
        if (m.col_to_var[col] == unassigned_var || !m.matrix[row][col]) {
            #ifdef VERBOSE_DEBUG
            cout << "row:" << row << " col:" << col << " m.last_one_in_col[col]-1: " << m.last_one_in_col[col]-1 << endl;
            #endif
            assert(m.col_to_var[col] == unassigned_var || std::min(m.last_one_in_col[col]-1, (int)m.num_rows) == row);
            continue;
        }
        row++;
    }
    #endif

    return i;
}

Gaussian::gaussian_ret Gaussian::handle_matrix_confl(Clause*& confl, const matrixset& m, const uint size, const uint maxlevel, const uint best_row)
{
    assert(best_row != UINT_MAX);

    m.varset[best_row].fill(tmp_clause, solver.assigns, col_to_var_original);
    confl = Clause_new(tmp_clause, solver.learnt_clause_group++, false);
    Clause& cla = *confl;
    if (solver.dynamic_behaviour_analysis)
        solver.logger.set_group_name(confl->group, "learnt gauss clause");
    
    if (cla.size() <= 1)
        return unit_conflict;

    assert(cla.size() >= 2);
    #ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")Found conflict:";
    solver.printClause(cla);
    #endif

    if (maxlevel != solver.decisionLevel()) {
        if (solver.dynamic_behaviour_analysis)
            solver.logger.conflict(Logger::gauss_confl_type, maxlevel, confl->group, *confl);
        solver.cancelUntil(maxlevel);
    }
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

Gaussian::gaussian_ret Gaussian::handle_matrix_prop_and_confl(matrixset& m, uint last_row, Clause*& confl)
{
    uint maxlevel = UINT_MAX;
    uint size = UINT_MAX;
    uint best_row = UINT_MAX;

    for (uint row = last_row; row != m.num_rows; row++) {
        #ifdef DEBUG_GAUSS
        assert(m.matrix[row].isZero());
        #endif
        if (!m.matrix[row].get_xor_clause_inverted())
            analyse_confl(m, row, maxlevel, size, best_row);
    }

    if (maxlevel != UINT_MAX)
        return handle_matrix_confl(confl, m, size, maxlevel, best_row);

    #ifdef DEBUG_GAUSS
    assert(check_no_conflict(m));
    #endif
    m.num_rows = last_row;
    m.matrix.resizeNumRows(m.num_rows);
    m.varset.resizeNumRows(m.num_rows);

    gaussian_ret ret = nothing;

    uint num_props = 0;
    for (const uint* prop_row = propagatable_rows.getData(), *end = prop_row + propagatable_rows.size(); prop_row != end; prop_row++ ) {
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
        for (Gaussian **gauss = &(solver.gauss_matrixes[0]), **end= gauss + solver.gauss_matrixes.size(); gauss != end; gauss++)
            if (*gauss != this) (*gauss)->canceling(level, var);
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

        const Var real_var = col_to_var_original[var];
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
    #endif

    maxlevel = this_maxlevel;
    size = this_size;
    best_row = row;
}

Gaussian::gaussian_ret Gaussian::handle_matrix_prop(matrixset& m, const uint row)
{
    #ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")matrix prop found!" << endl;
    cout << m.matrix[row] << endl;
    cout << "(" << matrix_no << ")matrix row:";
    print_matrix_row(m.matrix[row]);
    cout << endl;
    #endif

    m.varset[row].fill(tmp_clause, solver.assigns, col_to_var_original);
    Clause& cla = *Clause_new(tmp_clause, solver.learnt_clause_group++, false);
    #ifdef VERBOSE_DEBUG
    cout << "(" << matrix_no << ")matrix prop clause: ";
    solver.printClause(cla);
    cout << endl;
    #endif
    
    assert(!m.matrix[row].get_xor_clause_inverted() == !cla[0].sign());
    assert(solver.assigns[cla[0].var()].isUndef());
    if (cla.size() == 1) {
        const Lit lit = cla[0];
        if (solver.dynamic_behaviour_analysis) {
            solver.logger.set_group_name(cla.group, "unitary learnt clause");
            solver.logger.conflict(Logger::gauss_confl_type, 0, cla.group, cla);
        }
        
        solver.cancelUntil(0);
        solver.uncheckedEnqueue(lit);
        solver.unitary_learnts.push(&cla);
        if (solver.dynamic_behaviour_analysis)
            solver.logger.propagation(cla[0], Logger::gauss_propagation_type, cla.group);
        return unit_propagation;
    }

    matrix_clauses_toclear.push_back(&cla);
    solver.uncheckedEnqueue(cla[0], &cla);
    if (solver.dynamic_behaviour_analysis) {
        solver.logger.set_group_name(cla.group, "gauss prop clause");
        solver.logger.propagation(cla[0], Logger::gauss_propagation_type, cla.group);
    }

    return propagation;
}

void Gaussian::disable_if_necessary()
{
    if (//nof_conflicts >= 0
        //&& conflictC >= nof_conflicts/8
        /*&&*/ called > 100
        && (double)useful_confl/(double)called < 0.1
        && (double)useful_prop/(double)called < 0.3 )
            disabled = true;
}

llbool Gaussian::find_truths(vec<Lit>& learnt_clause, int& conflictC)
{
    Clause* confl;

    disable_if_necessary();
    if (should_check_gauss(solver.decisionLevel(), solver.starts)) {
        called++;
        gaussian_ret g = gaussian(confl);
        
        switch (g) {
        case conflict: {
            useful_confl++;
            llbool ret = solver.handle_conflict(learnt_clause, confl, conflictC);
            free(confl);
            
            if (ret != l_Nothing) return ret;
            return l_Continue;
        }
        case propagation:
        case unit_propagation:
            useful_prop++;
            return l_Continue;
        case unit_conflict: {
            useful_confl++;
            if (confl->size() == 0) {
                free(confl);
                return l_False;
            }

            Lit lit = (*confl)[0];
            if (solver.dynamic_behaviour_analysis)
                solver.logger.conflict(Logger::gauss_confl_type, 0, confl->group, *confl);
            
            solver.cancelUntil(0);
            
            if (solver.assigns[lit.var()].isDef()) {
                if (solver.dynamic_behaviour_analysis)
                    solver.logger.empty_clause(confl->group);
                
                free(confl);
                return l_False;
            }
            
            solver.uncheckedEnqueue(lit);
            if (solver.dynamic_behaviour_analysis)
                solver.logger.propagation(lit, Logger::gauss_propagation_type, confl->group);
            
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
    if (called > 0) {
        cout.setf(std::ios::fixed);
        std::cout << " Gauss(" << matrix_no << ") useful";
        cout << " prop: " << std::setprecision(2) << std::setw(5) << ((double)useful_prop/(double)called)*100.0 << "% ";
        cout << " confl: " << std::setprecision(2) << std::setw(5) << ((double)useful_confl/(double)called)*100.0 << "% ";
        if (disabled) std::cout << "disabled";
    } else
        std::cout << " Gauss(" << matrix_no << ") not called.";
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
    uint row = 0;
    for(PackedMatrix::iterator r = m.matrix.begin(), end = m.matrix.end(); r != end; ++r, ++row) {
        if (!(*r).get_xor_clause_inverted() && (*r).isZero()) {
            cout << "Conflict at row " << row << endl;
            return false;
        }
    }
    return true;
}

void Gaussian::print_matrix(matrixset& m) const
{
    uint row = 0;
    for (PackedMatrix::iterator it = m.matrix.begin(); it != m.matrix.end(); ++it, row++) {
        cout << *it << " -- row:" << row;
        if (row >= m.num_rows)
            cout << " (considered past the end)";
        cout << endl;
    }
}

void Gaussian::print_last_one_in_cols(matrixset& m) const
{
    for (uint i = 0; i < m.num_cols; i++) {
        cout << "last_one_in_col[" << i << "]-1 = " << m.last_one_in_col[i]-1 << endl;
    }
    cout << "m.past_the_end_last_one_in_col:" <<  m.past_the_end_last_one_in_col << endl;
}

const bool Gaussian::nothing_to_propagate(matrixset& m) const
{
    for(PackedMatrix::iterator r = m.matrix.begin(), end = m.matrix.end(); r != end; ++r) {
        if ((*r).popcnt_is_one()
            && solver.assigns[m.col_to_var[(*r).scan(0)]].isUndef())
            return false;
    }
    for(PackedMatrix::iterator r = m.matrix.begin(), end = m.matrix.end(); r != end; ++r) {
        if ((*r).isZero() && !(*r).get_xor_clause_inverted())
            return false;
    }
    return true;
}

const bool Gaussian::check_last_one_in_cols(matrixset& m) const
{
    for(uint i = 0; i < m.num_cols; i++) {
        const uint last = std::min(m.last_one_in_col[i] - 1, (int)m.num_rows);
        uint real_last = 0;
        uint i2 = 0;
        for (PackedMatrix::iterator it = m.matrix.begin(); it != m.matrix.end(); ++it, i2++) {
            if ((*it)[i])
                real_last = i2;
        }
        if (real_last > last)
            return false;
    }
    
    return true;
}

const bool Gaussian::check_matrix_against_varset(PackedMatrix& matrix, PackedMatrix& varset) const
{
    assert(matrix.size() == varset.size());
    
    for (uint i = 0; i < matrix.size(); i++) {
        const PackedRow mat_row = matrix[i];
        const PackedRow var_row = varset[i];
        
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
        if ((final^mat_row.get_xor_clause_inverted()) != var_row.get_xor_clause_inverted()) {
            cout << "problem with row:"; print_matrix_row_with_assigns(var_row); cout << endl;
            assert(false);
        }
    }
}

const uint Gaussian::get_called() const
{
    return called;
}

const uint Gaussian::get_useful_prop() const
{
    return useful_prop;
}

const uint Gaussian::get_useful_confl() const
{
    return useful_confl;
}

const bool Gaussian::get_disabled() const
{
    return disabled;
}

void Gaussian::set_disabled(const bool toset)
{
    disabled = toset;
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
