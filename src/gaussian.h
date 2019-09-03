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

struct XorReason
{
    bool must_recalc = true;
    Lit propagated = lit_Undef;
    vector<Lit> reason;
};

class EGaussian {
  public:
      EGaussian(
        Solver* solver,
        const GaussConf& config,
        const uint32_t matrix_no,
        const vector<Xor>& xorclauses
    );
    ~EGaussian();


    ///returns FALSE in case of conflict
    bool  find_truths(
        GaussWatched*& i,
        GaussWatched*& j,
        uint32_t p,
        const uint32_t row_n,
        GaussQData& gqd
    );

    vector<Lit>* get_reason(uint32_t row);

    // when basic variable is touched , eliminate one col
    void eliminate_col(
        uint32_t p,
        GaussQData& gqd
    );
    void new_decision_level();
    void canceling();
    bool full_init(bool& created);
    void update_cols_vals_set();
    void print_matrix_stats();

    vector<Xor> xorclauses;

  private:
    Solver* solver;   // orignal sat solver
    const GaussConf& config;

    //Cleanup
    bool clean_xors();
    void clear_gwatches(const uint32_t var);
    void delete_gauss_watch_this_matrix();
    void delete_gausswatch(const bool orig_basic,
                           const uint32_t  row_n,
                           uint32_t no_touch_var = var_Undef);

    //Reason generation
    vector<XorReason> xor_reasons;

    //Initialisation
    void eliminate();
    void fill_matrix();
    uint32_t select_columnorder();
    gret adjust_matrix(); // adjust matrix, include watch, check row is zero, etc.

    ///////////////
    // Helper during truth finding/elim
    ///////////////
    inline void conflict_twoclause(PropBy& confl);


    ///////////////
    // stats
    ///////////////
    uint64_t find_truth_called_propgause = 0;
    uint64_t find_truth_ret_fnewwatch = 0;
    uint64_t find_truth_ret_confl = 0;
    uint64_t find_truth_ret_satisfied = 0;
    uint64_t find_truth_ret_prop = 0;

    uint64_t elim_called = 0;
    uint64_t elim_xored_rows = 0;
    uint64_t elim_called_propgause = 0;
    uint64_t elim_ret_prop = 0;
    uint64_t elim_ret_confl = 0;
    uint64_t elim_ret_satisfied = 0;
    uint64_t elim_ret_fnewwatch = 0;

    ///////////////
    // Internal data
    ///////////////
    const uint32_t matrix_no;
    bool cancelled_since_val_update = true;
    uint32_t last_val_update = 0;
    vector<Lit> tmp_clause;  // conflict&propagation handling

    //Is the clause at this ROW satisfied already?
    //satisfied_xors[decision_level][row] tells me that
    vector<vector<bool>> satisfied_xors;

    // Someone is responsible for this column if TRUE
    ///we always WATCH this variable
    vector<char> var_has_resp_row;

    ///row_non_resp_for_var[ROW] gives VAR it's NOT responsible for
    ///we always WATCH this variable
    vector<uint32_t> row_non_resp_for_var;


    PackedMatrix mat;
    vector<uint32_t>  var_to_col; ///var->col mapping. Index with VAR
    vector<uint32_t> col_to_var; ///col->var mapping. Index with COL
    uint32_t num_rows;
    uint32_t num_cols;

    //quick lookup
    PackedRow *cols_vals = NULL;
    PackedRow *cols_set = NULL;
    PackedRow *tmp_col = NULL;
    PackedRow *tmp_col2 = NULL;
    void update_cols_vals_set(const Lit lit1);


    ///////////////
    // Debug
    ///////////////
    void print_matrix();
    void check_watchlist_sanity();
};

inline void EGaussian::canceling() {
    cancelled_since_val_update = true;
}

}


#endif //ENHANCEGAUSSIAN_H
