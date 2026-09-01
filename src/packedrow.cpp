/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file
Copyright (c) 2012  Cheng-Shen Han
Copyright (c) 2012  Jie-Hong Roland Jiang

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

#include "packedrow.h"
// #define VERBOSE_DEBUG
// #define SLOW_DEBUG

using namespace CMSat;

///returns popcnt
uint32_t PackedRow::find_watchVar(
    vector<Lit>& tmp_clause,
    const vector<uint32_t>& col_to_var,
    vector<char> &var_has_resp_row,
    uint32_t& non_resp_var
) {
    uint32_t popcnt = 0;
    non_resp_var = numeric_limits<uint32_t>::max();
    tmp_clause.clear();

    for(int i = 0; i < size*64; i++) {
        if (this->operator[](i)){
            popcnt++;
            uint32_t var = col_to_var[i];
            tmp_clause.push_back(Lit(var, false));

            if (!var_has_resp_row[var]) {
                non_resp_var = var;
            } else {
                //What??? WARNING
                //This var already has a responsible for it...
                //How can it be 1???
                std::swap(tmp_clause[0], tmp_clause.back());
            }
        }
    }
    assert(tmp_clause.size() == popcnt);
    assert( popcnt == 0 || var_has_resp_row[ tmp_clause[0].var() ]) ;
    return popcnt;
}

gret PackedRow::propGause(
    const vector<lbool>& assigns,
    const vector<uint32_t>& dcol_to_var,
    const vector<char>& var_has_resp_row,
    const uint32_t resp_var,
    uint32_t& new_resp_var,
    PackedRow& tmp_col,
    PackedRow& cols_vals,
    PackedRow& cols_unset,
    Lit& ret_lit_prop
) const {
    int first_nz;
    const uint32_t pop = tmp_col.set_and_until_popcnt_atleast2(*this, cols_unset, first_nz);
    const lbool resp_val = assigns[resp_var];
    const uint32_t unset = pop + (resp_val == l_Undef);

    //Find new watch. The responsible var is never a candidate -- it isn't in
    //D at all -- so we only ever look at the D bits.
    if (unset >= 2) {
        for (int i = first_nz; i < size; i++) {
            uint64_t tmp = (uint64_t)tmp_col.mp[i];
            while (tmp) {
                const uint32_t dcol = (uint32_t)i*64 + __builtin_ctzll(tmp);
                tmp &= tmp-1;
                const uint32_t var = dcol_to_var[dcol];
                SLOW_DEBUG_DO(assert(assigns[var] == l_Undef));

                // found new non-basic variable, let's watch it
                if (!var_has_resp_row[var]) {
                    new_resp_var = var;
                    return gret::nothing_fnewwatch;
                }
            }
        }
        assert(false && "Should have found a new watch!");
    }

    //Value of the row -- only its parity is ever used
    const bool odd = parity_of_and(cols_vals) ^ (bool)rhs() ^ (resp_val == l_True);

    //Lazy prop
    if (unset == 1) {
        uint32_t var = resp_var;
        if (resp_val != l_Undef) {
            for (int i = first_nz; i < size; i++) if (tmp_col.mp[i]) {
                var = dcol_to_var[(uint32_t)i*64 + __builtin_ctzll((uint64_t)tmp_col.mp[i])];
                break;
            }
        }
        assert(assigns[var] == l_Undef);
        ret_lit_prop = Lit(var, !odd);
        return gret::prop;
    }

    //Only SAT & UNSAT left.
    assert(unset == 0);
    return odd ? gret::confl : gret::nothing_satisfied;
}
