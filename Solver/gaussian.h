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

#ifndef GAUSSIAN_H
#define GAUSSIAN_H

#include <vector>
#include "mpz_class.h"
#include "SolverTypes.h"
#include "Solver.h"
#include "gaussianconfig.h"
using std::vector;
using std::cout;
using std::endl;

class Clause;

//#define VERBOSE_DEBUG
//#define DEBUG_GAUSS

class Gaussian
{
public:
    Gaussian(Solver& solver, const uint matrix_no, const GaussianConfig& config);
    ~Gaussian();

    llbool full_init();
    llbool find_truths(vec<Lit>& learnt_clause, int& conflictC);

    //statistics
    void print_stats() const;
    void reset_stats();
    void print_matrix_stats() const;

    //functions used throughout the Solver
    void back_to_level(const uint level);
    void set_decision_until(const uint to);
    void set_decision_from(unsigned int from);
    void canceling(const uint level, const Var var);
    void clear_clauses();

protected:
    Solver& solver;
    const uint matrix_no;
    const GaussianConfig& config;

    enum gaussian_ret {conflict, unit_conflict, propagation, unit_propagation, nothing};
    gaussian_ret gaussian(Clause*& confl);

    vector<uint> col_to_var_original;

    class matrixset
    {
    public:
        vector<mpz_class> matrix; // The matrix, updated to reflect variable assignements
        vector<mpz_class> varset; // The matrix, without variable assignements. The xor-clause is read from here. This matrix only follows the 'matrix' with its row-swap, row-xor, and row-delete operations.
        vector<uint> var_to_col; // var_to_col[VAR] gives the column for that variable. If the variable is not in the matrix, it gives UINT_MAX, if the var WAS inside the matrix, but has been zeroed, it gives UINT_MAX-1
        vector<uint> col_to_var; // col_to_var[COL] tells which variable is at a given column in the matrix. Gives UINT_MAX if the COL has been zeroed (i.e. the variable assigned)
        uint num_rows; // number of active rows in the matrix. Unactive rows are rows that contain only zeros (and if they are conflicting, then the conflict has been treated)
        uint num_cols; // number of active columns in the matrix. The columns at the end that have all be zeroed are no longer active
        int least_column_changed; // when updating the matrix, this value contains the smallest column number that has been updated  (Gauss elim. can start from here instead of from column 0)
        vector<uint> last_one_in_col; //last_one_in_col[COL] tells the last row that has a '1' in that column. Used to reduce the burden of Gauss elim. (it only needs to look until that row)
        uint removeable_cols; // the number of columns that have been zeroed out (i.e. assigned)
    };

    //Saved states
    vector<matrixset> matrix_sets; // The matrixsets for depths 'decision_from' + 0,  'decision_from' + only_nth_gaussian_save, 'decision_from' + 2*only_nth_gaussian_save, ... 'decision_from' + 'decision_until'.
    matrixset original_matrixset; // The matrixset at depth 0 of the search tree
    matrixset cur_matrixset; // The current matrixset, i.e. the one we are working on, or the last one we worked on

    //Varibales to keep Gauss state
    bool messed_matrix_vars_since_reversal;
    int gauss_last_level;
    vector<Clause*> matrix_clauses_toclear;
    bool went_below_decision_from;
    const uint gauss_starts_from; //Gauss elimination starts from this search depth
    bool disable_gauss; // Gauss is disabled (automatically)

    //Statistics
    uint useful_gaussian; //how many times Gauss was useful
    uint called_gaussian; //how many times called the Gauss

    //gauss init functions
    void init(); // Initalise gauss state
    void fill_matrix(matrixset& m); // Fills the matrix
    uint fill_var_to_col(matrixset& m) const; // Fills the var_to_col and col_to_var in the matrix. 

    //Main function
    uint eliminate(matrixset& matrix, vec<uint>& propagatable_rows, uint& conflict_row) const; //does the actual gaussian elimination

    //matrix update functions
    void update_matrix_col(matrixset& matrix, const Var x, const uint col) const; // Update one matrix column
    void update_matrix_by_col_all(matrixset& m) const; // Update all columns, column-by-column (and not row-by-row)
    void set_matrixset_to_cur(); // Save the current matrixset, the cur_matrixset to matrix_sets
    //void update_matrix_by_row(matrixset& matrix) const;
    //void update_matrix_by_col(matrixset& matrix, const uint last_level) const;

    //conflict&propagation handling
    gaussian_ret handle_matrix_prop_and_confl(matrixset& m, uint row, const vec<uint>& propagatable_rows, Clause*& confl);
    void analyse_confl(const matrixset& m, const uint row, uint& maxlevel, uint& size, uint& best_row) const; // analyse conflcit to find the best conflict. Gets & returns the best one in 'maxlevel', 'size' and 'best row' (these are all UINT_MAX when calling this function first, i.e. when there is no other possible conflict to compare to the new in 'row')
    gaussian_ret handle_matrix_confl(Clause*& confl, const matrixset& m, const uint size, const uint maxlevel, const uint best_row);
    gaussian_ret handle_matrix_prop(matrixset& m, const uint row); // Handle matrix propagation at row 'row'

    //propagation&conflict handling
    void cancel_until_sublevel(const uint sublevel); // cancels until sublevel 'sublevel'. The var 'sublevel' must NOT go over the current level. I.e. this function is ONLY for moving inside the current level
    uint find_sublevel(const Var v) const; // find the sublevel (i.e. trail[X]) of a given variable

    //helper functions
    bool at_first_init() const;
    bool should_init() const;
    bool should_check_gauss(const uint decisionlevel, const uint starts) const;

    
    
private:
    //temporary internal data to avoid constantre-allocation
    mutable vec<uint> rows_with_one;
    mutable vec<uint> propagatable_rows; //used in gaussian()
    
    //debug functions
    bool check_no_conflict(const matrixset& m) const; // Are there any conflicts that the matrixset 'm' causes?
    const bool nothing_to_propagate(const matrixset& m) const; // Are there any conflicts of propagations that matrixset 'm' clauses?
    void print_matrix_row(const mpz_class& row) const; // Print matrix row 'row'
    void print_matrix_row_with_assigns(const mpz_class& row) const;
    const bool check_matrix_against_varset(const vector<mpz_class>& matrix, const vector<mpz_class>& varset) const;
    static const string lbool_to_string(const lbool toprint);
};

inline void Gaussian::back_to_level(const uint level)
{
    if (level <= config.decision_from) went_below_decision_from = true;
}

inline bool Gaussian::should_init() const
{
    return (solver.starts >= gauss_starts_from && config.decision_until > 0);
}

inline bool Gaussian::at_first_init() const
{
    return (solver.starts == gauss_starts_from);
}

inline bool Gaussian::should_check_gauss(const uint decisionlevel, const uint starts) const
{
    return (!disable_gauss
            && starts >= gauss_starts_from
            && decisionlevel < config.decision_until
            && decisionlevel >= config.decision_from
            && decisionlevel % config.every_nth_gauss == 0);
}

inline void Gaussian::canceling(const uint level, const Var var)
{
    if (!messed_matrix_vars_since_reversal
            && level <= gauss_last_level
            && var < cur_matrixset.var_to_col.size()
            && cur_matrixset.var_to_col[var] == UINT_MAX-1
       )
        messed_matrix_vars_since_reversal = true;
}

inline void Gaussian::print_matrix_stats() const
{
    cout << "matrix size: " << cur_matrixset.num_rows << "  x " << cur_matrixset.num_cols << endl;
}

inline void Gaussian::set_matrixset_to_cur()
{
    /*cout << solver.decisionLevel() << endl;
    cout << decision_from << endl;
    cout << matrix_sets.size() << endl;*/
    
    if (solver.decisionLevel() == 0) {
        original_matrixset = cur_matrixset;
    }
    
    if (solver.decisionLevel() >= config.decision_from) {
        uint level = ((solver.decisionLevel() - config.decision_from) / config.only_nth_gauss_save);
        
        assert(level <= matrix_sets.size()); //TODO check if we need this, or HOW we need this in a multi-matrix setting
        if (level == matrix_sets.size())
            matrix_sets.push_back(cur_matrixset);
        else
            matrix_sets[level] = cur_matrixset;
    }
}

std::ostream& operator << (std::ostream& os, const vec<Lit>& v);

#endif //GAUSSIAN_H