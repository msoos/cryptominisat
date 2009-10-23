/***********************************************************************************
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include "mpz_class.h"

MpzStack mpz_class::mpzstack;

void mpz_class::fill(Lit* ps, const vec<lbool>& assigns, const vector<uint>& col_to_var_original) const
{
    bool final = xor_clause_inverted;

    Lit* ps_first = ps;
    unsigned long int col = 0;
    bool wasundef = false;
    while (true) {
        col = mpz_scan1(mp, col);
        if (col == ULONG_MAX) break;

        const uint& var = col_to_var_original[col];
        assert(var != UINT_MAX);

        const lbool val = assigns[var];
        const bool val_bool = val.getBool();
        *ps = Lit(var, val_bool);
        final ^= val_bool;
        if (val.isUndef()) {
            Lit tmp(*ps_first);
            *ps_first = *ps;
            *ps = tmp;
            wasundef = true;
        }
        col++;
        ps++;
    }
    if (wasundef) {
        *ps_first ^= final;
        //assert(ps != ps_first+1);
    } else
        assert(!final);
}

std::ostream& operator << (std::ostream& os, const mpz_class& m)
{
    unsigned long int var = 0;
    while (true) {
        var = m.scan(var);
        if (var == ULONG_MAX) break;

        os << var+1 << " ";
        var++;
    }
    return os;
}
