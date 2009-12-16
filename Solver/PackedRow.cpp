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

#include "PackedRow.h"

std::ostream& operator << (std::ostream& os, const PackedRow& m)
{
    for(uint i = 0; i < m.size*64; i++) {
        os << m[i];
    }
    os << " -- xor: " << !m.is_true();
    return os;
}

bool PackedRow::operator ==(const PackedRow& b) const
{
    #ifdef DEBUG_ROW
    assert(size > 0);
    assert(b.size > 0);
    assert(size == b.size);
    #endif
    
    return (std::equal(b.mp-1, b.mp+size, mp-1));
}

bool PackedRow::operator !=(const PackedRow& b) const
{
    #ifdef DEBUG_ROW
    assert(size > 0);
    assert(b.size > 0);
    assert(size == b.size);
    #endif
    
    return (!std::equal(b.mp-1, b.mp+size, mp-1));
}

uint PackedRow::popcnt() const
{
    uint popcnt = 0;
    for (uint i = 0; i < size; i++) if (mp[i]) {
        uint64_t tmp = mp[i];
        for (uint i2 = 0; i2 < 64; i2++) {
            popcnt += (tmp & 1);
            tmp >>= 1;
        }
    }
    return popcnt;
}

uint PackedRow::popcnt(const uint from) const
{
    uint popcnt = 0;
    for (uint i = from/64; i != size; i++) if (mp[i]) {
        uint64_t tmp = mp[i];
        uint i2;
        if (i == from/64) {
            i2 = from%64;
            tmp >>= i2;
        } else
            i2 = 0;
        for (; i2 < 64; i2++) {
            popcnt += (tmp & 1);
            tmp >>= 1;
        }
    }
    return popcnt;
}

void PackedRow::fill(vec<Lit>& tmp_clause, const vec<lbool>& assigns, const vector<Var>& col_to_var_original) const
{
    bool final = !is_true_internal;
    
    tmp_clause.clear();
    uint col = 0;
    bool wasundef = false;
    for (uint i = 0; i < size; i++) for (uint i2 = 0; i2 < 64; i2++) {
        if ((mp[i] >> i2) &1) {
            const uint& var = col_to_var_original[col];
            assert(var != UINT_MAX);
            
            const lbool& val = assigns[var];
            const bool val_bool = val.getBool();
            tmp_clause.push(Lit(var, val_bool));
            final ^= val_bool;
            if (val.isUndef()) {
                assert(!wasundef);
                Lit tmp(tmp_clause[0]);
                tmp_clause[0] = tmp_clause.last();
                tmp_clause.last() = tmp;
                wasundef = true;
            }
        }
        col++;
    }
    if (wasundef) {
        tmp_clause[0] ^= final;
        //assert(ps != ps_first+1);
    } else
        assert(!final);
}

