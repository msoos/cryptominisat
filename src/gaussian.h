/******************************************
Copyright (c) 2012  Cheng-Shen Han
Copyright (c) 2012  Jie-Hong Roland Jiang
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

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
    int32_t ID = 0;
    vector<Lit> reason;
    #ifdef USE_TBUDDY
    tbdd::xor_constraint* constr = NULL;
    ilist list = NULL;
    #endif
};

class EGaussian {
  public:
      EGaussian(
        Solver* solver,
        const uint32_t matrix_no,
        const vector<Xor>& xorclauses
    );
    ~EGaussian();
    bool is_initialized() const;

    ///returns FALSE in case of conflict
    bool  find_truths(
        GaussWatched*& i,
        GaussWatched*& j,
        const uint32_t var,
        const uint32_t row_n,
        GaussQData& gqd
    );

    vector<Lit>* get_reason(const uint32_t row, int32_t& out_ID);

    // when basic variable is touched , eliminate one col
    void eliminate_col(
        uint32_t p,
        GaussQData& gqd
    );
    void canceling();
    bool full_init(bool& created);
    void update_cols_vals_set(bool force = false);
    void print_matrix_stats(uint32_t verbosity);
    bool must_disable(GaussQData& gqd);
    void check_invariants();
    void update_matrix_no(uint32_t n);
    void check_watchlist_sanity();
    uint32_t get_matrix_no();
    void finalize_frat();
    void move_back_xor_clauses();

    vector<Xor> xorclauses;

  private:
    Solver* solver;   // orignal sat solver

    //Cleanup
    void clear_gwatches(const uint32_t var);
    void delete_gauss_watch_this_matrix();
    void delete_gausswatch(const uint32_t  row_n);

    //Invariant checks, debug
    void check_no_prop_or_unsat_rows();
    void check_tracked_cols_only_one_set();
    bool check_row_satisfied(const uint32_t row);
    void print_gwatches(const uint32_t var) const;
    void check_row_not_in_watch(
        const uint32_t v, const uint32_t row_num) const;

    //Reason generation
    vector<XorReason> xor_reasons;
    vector<Lit> tmp_clause;
    uint32_t get_max_level(const GaussQData& gqd, const uint32_t row_n);

    //Initialisation
    void eliminate();
    void fill_matrix();
    void select_columnorder();
    gret init_adjust_matrix(); // adjust matrix, include watch, check row is zero, etc.
    double get_density();

    //Helper functions
    void prop_lit(
        const GaussQData& gqd, const uint32_t row_i, const Lit ret_lit_prop);

    #ifdef USE_TBUDDY
    struct BDDCl {
        ilist cl;
        int32_t ID;
    };
    void xor_in_bdd(const uint32_t a, const uint32_t b);
    tbdd::xor_constraint* bdd_create(const uint32_t row_n, const uint32_t expected_size);
    ilist one_len_ilist = NULL;
    ilist ilist_tmp = NULL;
    void create_unit_bdd_reason(const uint32_t row_n);
    vector<BDDCl> frat_ids;
    tbdd::xor_constraint* unsat_bdd = NULL; //set if UNSAT is from GJ
    #endif


    ///////////////
    // stats
    ///////////////
    uint64_t find_truth_ret_satisfied_precheck = 0;
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
    double before_init_density = 0;
    double after_init_density = 0;

    ///////////////
    // Internal data
    ///////////////
    uint32_t matrix_no;
    bool initialized = false;
    bool cancelled_since_val_update = true;
    uint32_t last_val_update = 0;

    //Is the clause at this ROW satisfied already?
    //satisfied_xors[decision_level][row] tells me that
    vector<char> satisfied_xors;

    // Someone is responsible for this column if TRUE
    ///we always WATCH this variable
    vector<char> var_has_resp_row;

    ///row_to_var_non_resp[ROW] gives VAR it's NOT responsible for
    ///we always WATCH this variable
    vector<uint32_t> row_to_var_non_resp;


    PackedMatrix mat;
    vector<vector<char>> bdd_matrix;
    vector<uint32_t>  var_to_col; ///var->col mapping. Index with VAR
    vector<uint32_t> col_to_var; ///col->var mapping. Index with COL
    uint32_t num_rows = 0;
    uint32_t num_cols = 0;

    //quick lookup
    PackedRow *cols_vals = NULL;
    PackedRow *cols_unset = NULL;
    PackedRow *tmp_col = NULL;
    PackedRow *tmp_col2 = NULL;
    void update_cols_vals_set(const Lit lit1);

    //Data to free (with delete[] x)
    vector<int64_t*> tofree;


    ///////////////
    // Debug
    ///////////////
    void print_matrix();
    void check_cols_unset_vals();
};

inline void EGaussian::canceling() {
    cancelled_since_val_update = true;

    //TODO this is an overstatement, coudl be improved
    memset(satisfied_xors.data(), 0, satisfied_xors.size());
}

inline double EGaussian::get_density()
{
    if (num_rows*num_cols == 0) {
        return 0;
    }

    uint32_t pop = 0;
    for (const auto& row: mat) {
        pop += row.popcnt();
    }
    return (double)pop/(double)(num_rows*num_cols);
}

inline void EGaussian::update_matrix_no(uint32_t n)
{
    matrix_no = n;
}

inline uint32_t EGaussian::get_matrix_no()
{
    return matrix_no;
}

inline bool EGaussian::is_initialized() const
{
    return initialized;
}


}

#endif //ENHANCEGAUSSIAN_H
