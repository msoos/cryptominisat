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

#ifndef ENHANCEGAUSSIAN_H
#define ENHANCEGAUSSIAN_H

#include <vector>
#include <limits>
#include <string>
#include <utility>

#include "solvertypes.h"
#include "packedmatrix.h"
#include "bitarray.h"
#include "propby.h"
#include "xor.h"
#include "gausswatched.h"
#include "gqueuedata.h"

//#define VERBOSE_DEBUG
//#define DEBUG_GAUSS

using std::string;
using std::pair;
using std::vector;

namespace CMSat {

class Solver;

class EGaussian {
  public:
    Solver* solver;   // orignal sat solver
    const GaussConf& config;  // gauss some configure
    const uint32_t matrix_no;            // matrix index
    vector<Lit> tmp_clause;  // conflict&propagation handling

    //Is the clause at this ROW satisfied already?
    //satisfied_xors[row] tells me that
    vector<vector<bool>> satisfied_xors;

    // variable state
    // Someone is responsible for this column if TRUE
    // we watch ONE basic + ONE non-basic var
    vector<char> var_has_resp_row;

    vector<uint32_t>  var_to_col;             // variable to column

    ///row_resp_for_var[ROW] gives VAR it's responsible for
    ///it is MAX in case row is responsible for nothing
    ///we always WATCH this variable!
    vector<uint32_t> row_non_resp_for_var;

    // used in orignal matrix
    PackedMatrix mat; // The matrix, updated to reflect variable assignements

    ///col_to_var[COL] tells which variable is at a given column in the matrix. Gives unassigned_var if the COL has been zeroed (i.e. the variable assigned)
    vector<uint32_t> col_to_var;

    uint32_t num_rows; // number of active rows in the matrix. Unactive rows are rows that contain only zeros (and if they are conflicting, then the conflict has been treated)
    uint32_t num_cols; // number of active columns in the matrix. The columns at the end that have all be zeroed are no longer active


    //Cleanup
    bool clean_xors();
    void clear_gwatches(const uint32_t var);
    void delete_gauss_watch_this_matrix();
    void delete_gausswatch(const bool orig_basic,
                           const uint32_t  row_n,
                           uint32_t no_touch_var = var_Undef);

    void new_decision_level();
    void eliminate();
    gret adjust_matrix(); // adjust matrix, include watch, check row is zero, etc.

    inline void propagation_twoclause();
    inline void conflict_twoclause(PropBy& confl);

    void print_matrix();

  public:
    // variable
    vector<Xor> xorclauses;   // xorclauses
    vector<pair<ClOffset, uint32_t> > clauses_toclear; // use to delete propagate clause


    EGaussian(
        Solver* solver,
        const GaussConf& config,
        const uint32_t matrix_no,
        const vector<Xor>& xorclauses
    );
    ~EGaussian();

    // functiion
    uint32_t get_matrix_no() const;
    void check_watchlist_sanity();
    void canceling(const uint32_t sublevel); //functions used throughout the Solver
    bool full_init(bool& created);  // initial arrary. return true is fine , return false means solver already false;
    void fill_matrix(); // Fills the origMat matrix
    uint32_t select_columnorder(); // Fills var_to_col and col_to_var of the origMat matrix.

    ///execute gaussian
    ///return FALSE only in case of unit conflict
    bool  find_truths2(
        GaussWatched*& i,
        GaussWatched*& j,
        uint32_t p,
        const uint32_t row_n,
        GaussQData& gqd
    );

    // when basic variable is touch , eliminate one col
    void eliminate_col2(
        uint32_t p,
        GaussQData& gqd
    );

    uint64_t propg_called_from_find_truth = 0;
    uint64_t propg_called_from_elim = 0;
    uint64_t eliminate_col_called = 0;
    uint64_t propg_called_from_find_truth_ret_fnewwatch = 0;
    uint64_t elim_xored_rows = 0;

    void check_xor_reason_clauses_not_cleared();
};

inline uint32_t EGaussian::get_matrix_no() const
{
    return matrix_no;
}

}


#endif //ENHANCEGAUSSIAN_H
