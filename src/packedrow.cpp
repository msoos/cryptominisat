/******************************************
Copyright (c) 2018  Mate Soos
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

using namespace CMSat;

///returns popcnt
uint32_t PackedRow::find_watchVar(
    vector<Lit>& tmp_clause,
    const vector<uint32_t>& col_to_var,
    vector<char> &var_has_resp_row,
    uint32_t& non_resp_var
) {
    uint32_t popcnt = 0;
    non_resp_var = std::numeric_limits<uint32_t>::max();
    tmp_clause.clear();

    for(uint32_t i = 0; i < size*64 && popcnt < 3; i++) {
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
    vector<Lit>& tmp_clause,
    const vector<lbool>& assigns,
    const vector<uint32_t>& col_to_var,
    vector<char> &var_has_resp_row,
    uint32_t& new_resp_var,
    PackedRow& tmp_col,
    PackedRow& cols_vals,
    PackedRow& cols_set
) {
    bool final = !rhs_internal;
    new_resp_var = std::numeric_limits<uint32_t>::max();
    tmp_clause.clear();
    tmp_col = *this;
    tmp_col.and_inv(cols_set);
    uint32_t pop = tmp_col.popcnt();
    tmp_col ^= cols_vals;

    for (uint32_t i = 0; i != size; i++) if (mp[i]) {
        uint64_t tmp = mp[i];
        uint32_t at = i*64;
        for (uint32_t i2 = 0 ; i2 < 64; i2++) {
            if(tmp & 1){
                const uint32_t var = col_to_var[at  + i2];
                const lbool val = assigns[var];

                //TODO: let's put the most UNDEF variables
                //TODO: at the beginning of the matrix

                // found new non-basic variable, let's watch it
                //TODO understand why is !var_has_resp_row[var] here?? whaaat? if it's UNDEF how would it propagate?
                if (val == l_Undef && !var_has_resp_row[var]) {
                    new_resp_var = var;
                    return gret::nothing_fnewwatch;
                }
                const bool val_bool = (val == l_True);
                final ^= val_bool;
                tmp_clause.push_back(Lit(var, val_bool));

                //if this is the basic variable, put it to the 0th position
                if (var_has_resp_row[var]) {
                    std::swap(tmp_clause[0], tmp_clause.back());
                }
            }
            tmp >>= 1;
        }
    }

    #ifdef SLOW_DEBUG
    {
        for (uint32_t i = 0; i != size; i++) if (mp[i]) {
            uint64_t tmp = mp[i];
            uint32_t at = i*64;
            for (uint32_t i2 = 0 ; i2 < 64; i2++) {
                if(tmp & 1){
                    const uint32_t var = col_to_var[at  + i2];
                    const lbool val = assigns[var];
                    if (val == l_Undef && !var_has_resp_row[var]) {
                        assert(false);
                    }
                }
                tmp >>= 1;
            }
        }
    }
    #endif

    if (assigns[tmp_clause[0].var()] == l_Undef) {
        //#ifdef SLOW_DEBUG
        for(uint32_t i = 1; i < tmp_clause.size(); i++) {
            assert(assigns[tmp_clause[i].var()] != l_Undef);
        }
        //#endif
        tmp_clause[0] = tmp_clause[0].unsign()^final;
        assert(pop == 1);
        return gret::prop;
    } else if (!final) {
        assert(pop == 0);
        return gret::confl;
    }
    // this row is already satisfied, all variables are set
    assert(pop == 0);
    return gret::nothing_satisfied;

}




