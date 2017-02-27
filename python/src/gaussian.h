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

#ifndef GAUSSIAN_H
#define GAUSSIAN_H

#include <vector>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "solvertypes.h"
#include "gaussconfig.h"
#include "propby.h"
#include "packedmatrix.h"
#include "bitarray.h"
#include "xorfinder.h"

//#define VERBOSE_DEBUG
//#define DEBUG_GAUSS

namespace CMSat {

using std::string;
using std::pair;
using std::vector;

enum gauss_ret {gauss_cont, gauss_confl, gauss_false, gauss_nothing};

class Clause;
class Solver;

struct GaussClauseToClear
{
    GaussClauseToClear() {}
    GaussClauseToClear(ClOffset _offs, uint32_t _sublevel) :
        offs(_offs)
        , sublevel(_sublevel)
    {}

    ClOffset offs;
    uint32_t sublevel;
};

class Gaussian
{
public:
    Gaussian(Solver* solver, const vector<Xor>& xors, const uint32_t matrix_no);
    ~Gaussian();

    bool init_until_fixedpoint();
    gauss_ret find_truths();

    //statistics
    void print_stats() const;
    void print_matrix_stats() const;
    uint32_t get_called() const;
    uint32_t get_useful_prop() const;
    uint32_t get_useful_confl() const;
    bool get_disabled() const;
    uint32_t get_unit_truths() const;
    void set_disabled(const bool toset);

    //functions used throughout the Solver
    void canceling(const uint32_t sublevel);
    vector<GaussClauseToClear> clauses_toclear;
    PropBy found_conflict;

protected:
    Solver* solver;
    vector<uint16_t>& seen;
    vector<uint8_t>& seen2; //for marking changed_rows
    vector<uint16_t> var_to_col;

    //Gauss high-level configuration
    const GaussConf& config;
    const uint32_t matrix_no;

    enum gaussian_ret {conflict, unit_conflict, propagation, unit_propagation, nothing};
    gaussian_ret perform_gauss(PropBy& confl);

    vector<uint32_t> col_to_var_original; //Matches columns to variables
    BitArray var_is_in; //variable is part of the the matrix. var_is_in's size is _minimal_ so you should check whether var_is_in.getSize() < var before issuing var_is_in[var]
    uint32_t badlevel;

    class matrixset
    {
    public:
        PackedMatrix matrix; // The matrix, updated to reflect variable assignements
        uint16_t num_rows = 0; // number of active rows in the matrix. Unactive rows are rows that contain only zeros (and if they are conflicting, then the conflict has been treated)
        uint16_t num_cols = 0; // number of active columns in the matrix. The columns at the end that have all be zeroed are no longer active
        int least_column_changed; // when updating the matrix, this value contains the smallest column number that has been updated  (Gauss elim. can start from here instead of from column 0)
        uint32_t removeable_cols; // the number of columns that have been zeroed out (i.e. assigned)

        vector<char> col_is_set;
        vector<uint32_t> col_to_var; // col_to_var[COL] tells which variable is at a given column in the matrix. Gives unassigned_var if the COL has been zeroed (i.e. the variable assigned)
        vector<uint16_t> last_one_in_col; //last_one_in_col[COL] tells the last row+1 that has a '1' in that column. Used to reduce the burden of Gauss elim. (it only needs to look until that row)


        vector<uint16_t> first_one_in_row; //first columnt with a '1' in [ROW]
    };

    //Saved states
    vector<matrixset> matrix_sets; // The matrixsets for depths 'decision_from' + 0,  'decision_from' + only_nth_gaussian_save, 'decision_from' + 2*only_nth_gaussian_save, ... 'decision_from' + 'decision_until'.
    matrixset cur_matrixset; // The current matrixset, i.e. the one we are working on, or the last one we worked on

    //Varibales to keep Gauss state
    bool messed_matrix_vars_since_reversal;
    int gauss_last_level;
    bool disabled = false; // Gauss is disabled

    //State of current elimnation
    vector<uint32_t> propagatable_rows; //used to store which rows were deemed propagatable during elimination
    vector<uint32_t> changed_rows; //used to store which rows were deemed propagatable during elimination

    //Statistics
    uint32_t useful_prop = 0; //how many times Gauss gave propagation as a result
    uint32_t useful_confl = 0; //how many times Gauss gave conflict as a result
    uint32_t called = 0; //how many times called the Gauss
    uint32_t unit_truths = 0; //how many unitary (i.e. decisionLevel 0) truths have been found

    //gauss init functions
    void init(); // Initalise gauss state
    void fill_matrix(matrixset& origMat); // Fills the origMat matrix
    uint32_t select_columnorder(matrixset& origMat); // Fills var_to_col and col_to_var of the origMat matrix.

    //Main function
    uint32_t eliminate(matrixset& matrix); //does the actual gaussian elimination

    //matrix update functions
    void update_matrix_col(matrixset& matrix, const uint32_t x, const uint32_t col); // Update one matrix column
    void update_matrix_by_col_all(matrixset& m); // Update all columns, column-by-column (and not row-by-row)
    void set_matrixset_to_cur(); // Save the current matrixset, the cur_matrixset to matrix_sets
    //void update_matrix_by_row(matrixset& matrix) const;
    //void update_matrix_by_col(matrixset& matrix, const uint32_t last_level) const;

    //conflict&propagation handling
    gaussian_ret handle_matrix_prop_and_confl(matrixset& m, uint32_t row, PropBy& confl);
    void analyse_confl(const matrixset& m, const uint32_t row, uint32_t& maxlevel, uint32_t& size, uint32_t& best_row) const; // analyse conflcit to find the best conflict. Gets & returns the best one in 'maxlevel', 'size' and 'best row' (these are all std::numeric_limits<uint32_t>::max() when calling this function first, i.e. when there is no other possible conflict to compare to the new in 'row')
    gaussian_ret handle_matrix_confl(PropBy& confl, const matrixset& m, const uint32_t maxlevel, const uint32_t best_row);
    gaussian_ret handle_matrix_prop(matrixset& m, const uint32_t row); // Handle matrix propagation at row 'row'
    vector<Lit> tmp_clause;

    //helper functions
    bool at_first_init() const;
    bool should_check_gauss(const uint32_t decisionlevel) const;
    void disable_if_necessary();
    void reset_stats();
    void update_last_one_in_col(matrixset& m);

private:
    //debug functions
    bool check_no_conflict(matrixset& m) const; // Are there any conflicts that the matrixset 'm' causes?
    bool nothing_to_propagate(matrixset& m) const; // Are there any conflicts of propagations that matrixset 'm' clauses?
    template<class T> void print_matrix_row(const T& row) const;
    template<class T>
    void print_matrix_row_with_assigns(const T& row) const;
    void check_matrix_against_varset(PackedMatrix& matrix,const matrixset& m) const;
    bool check_last_one_in_cols(matrixset& m) const;
    void check_first_one_in_row(matrixset& m, const uint32_t j);
    void print_matrix(matrixset& m) const;
    void print_last_one_in_cols(matrixset& m) const;
    static string lbool_to_string(const lbool toprint);
    vector<Xor> xors;
};

inline bool Gaussian::should_check_gauss(const uint32_t decisionlevel) const
{
    return (!disabled
            && decisionlevel < config.decision_until);
}

inline uint32_t Gaussian::get_unit_truths() const
{
    return unit_truths;
}

inline uint32_t Gaussian::get_called() const
{
    return called;
}

inline uint32_t Gaussian::get_useful_prop() const
{
    return useful_prop;
}

inline uint32_t Gaussian::get_useful_confl() const
{
    return useful_confl;
}

inline bool Gaussian::get_disabled() const
{
    return disabled;
}

inline void Gaussian::set_disabled(const bool toset)
{
    disabled = toset;
}

//std::ostream& operator << (std::ostream& os, const vector<Lit>& v);

}

#endif //GAUSSIAN_H
