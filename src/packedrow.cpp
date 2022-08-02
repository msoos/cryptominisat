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

#ifdef _MSC_VER
inline int scan_fwd_64b(int64_t value)
{
    unsigned long at;
    unsigned char ret = _BitScanForward64(&at, value);
    at++;
    if (!ret) at = 0;
    return at;
}
#else
inline int scan_fwd_64b(uint64_t value)
{
    return  __builtin_ffsll(value);
}
#endif

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

void PackedRow::get_reason(
    vector<Lit>& tmp_clause,
    [[maybe_unused]] const vector<lbool>& assigns,
    const vector<uint32_t>& col_to_var,
    PackedRow& cols_vals,
    PackedRow& tmp_col2,
    Lit prop
) {
    tmp_col2.set_and(*this, cols_vals);
    for (int i = 0; i < size; i++) if (mp[i]) {
        int64_t tmp = mp[i];
        unsigned long at;
        at = scan_fwd_64b(tmp);
        int extra = 0;
        while (at != 0) {
            uint32_t col = extra + at-1 + i*64;
            #ifdef SLOW_DEBUG
            assert(this->operator[](col) == 1);
            #endif
            const uint32_t var = col_to_var[col];
            if (var == prop.var()) {
                tmp_clause.push_back(prop);
                std::swap(tmp_clause[0], tmp_clause.back());
            } else {
                const bool val_bool = tmp_col2[col];
                tmp_clause.push_back(Lit(var, val_bool));
            }

            extra += at;
            if (extra == 64)
                break;

            tmp >>= at;
            at = scan_fwd_64b(tmp);
        }
    }

    #ifdef SLOW_DEBUG
    for(uint32_t i = 1; i < tmp_clause.size(); i++) {
        assert(assigns[tmp_clause[i].var()] != l_Undef);
    }
    #endif
}

gret PackedRow::propGause(
    const vector<lbool>& assigns,
    const vector<uint32_t>& col_to_var,
    vector<char> &var_has_resp_row,
    uint32_t& new_resp_var,
    PackedRow& tmp_col,
    PackedRow& tmp_col2,
    PackedRow& cols_vals,
    PackedRow& cols_unset,
    Lit& ret_lit_prop
) {
    uint32_t pop = tmp_col.set_and_until_popcnt_atleast2(*this, cols_unset);
    #ifdef VERBOSE_DEBUG
    cout << "POP in GausE: " << pop << " row: " << endl;
    cout << *this << endl;
    cout << " cols_unset: " << endl;
    cout << cols_unset << endl;
    #endif

    //Find new watch
    if (pop >= 2) {
        for (int i = 0; i < size; i++) if (tmp_col.mp[i]) {
            int64_t tmp = tmp_col.mp[i];
            unsigned long at;
            at = scan_fwd_64b(tmp);
            int extra = 0;
            while (at != 0) {
                uint32_t col = extra + at-1 + i*64;
                #ifdef SLOW_DEBUG
                assert(tmp_col[col] == 1);
                #endif
                const uint32_t var = col_to_var[col];

                #ifdef SLOW_DEBUG
                const lbool val = assigns[var];
                assert(val == l_Undef);
                #endif

                // found new non-basic variable, let's watch it
                if (!var_has_resp_row[var]) {
                    new_resp_var = var;
                    return gret::nothing_fnewwatch;
                }

                extra += at;
                if (extra == 64)
                    break;

                tmp >>= at;
                at = scan_fwd_64b(tmp);
            }
        }
        assert(false && "Should have found a new watch!");
    }

    //Calc value of row
    tmp_col2.set_and(*this, cols_vals);
    const uint32_t pop_t = tmp_col2.popcnt() + rhs();

    //Lazy prop
    if (pop == 1) {
        for (int i = 0; i < size; i++) if (tmp_col.mp[i]) {
            int at = scan_fwd_64b(tmp_col.mp[i]);

            // found prop
            uint32_t col = at-1 + i*64;
            #ifdef SLOW_DEBUG
            assert(tmp_col[col] == 1);
            #endif
            const uint32_t var = col_to_var[col];
            assert(assigns[var] == l_Undef);
            ret_lit_prop = Lit(var, !(pop_t % 2));
            return gret::prop;
        }
        assert(false && "Should have found the propagating literal!");
    }

    //Only SAT & UNSAT left.
    assert(pop == 0);

    //Satisfied
    if (pop_t % 2 == 0) {
        return gret::nothing_satisfied;
    }

    //Conflict
    return gret::confl;
}
