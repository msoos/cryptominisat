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

#include "packedrow.h"

using namespace CMSat;

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

uint32_t PackedRow::popcnt() const
{
    uint32_t popcnt = 0;
    for (uint32_t i = 0; i < size; i++) if (mp[i]) {
        uint64_t tmp = mp[i];
        for (uint32_t i2 = 0; i2 < 64; i2++) {
            popcnt += (tmp & 1);
            tmp >>= 1;
        }
    }
    return popcnt;
}

uint32_t PackedRow::popcnt(const uint32_t from) const
{
    uint32_t popcnt = 0;
    for (uint32_t i = from/64; i != size; i++) if (mp[i]) {
        uint64_t tmp = mp[i];
        uint32_t i2;
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

bool PackedRow::fill(
    vec<Lit>& tmp_clause,
    const vec<lbool>& assigns,
    const vector<uint32_t>& col_to_var_original
) const
{
    bool final = !is_true_internal;

    tmp_clause.clear();
    uint32_t col = 0;
    bool wasundef = false;
    for (uint32_t i = 0; i < size; i++) for (uint32_t i2 = 0; i2 < 64; i2++) {
        if ((mp[i] >> i2) &1) {
            const uint32_t& var = col_to_var_original[col];
            assert(var != std::numeric_limits<uint32_t>::max());

            const lbool val = assigns[var];
            const bool val_bool = val == l_True;
            tmp_clause.push(Lit(var, val_bool));
            final ^= val_bool;
            if (val == l_Undef) {
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

    return wasundef;
}



// add by hankf4
uint32_t PackedRow::find_watchVar(
    vector<Lit>& tmp_clause,
    const vector<uint32_t>& col_to_var,
    vec<bool> &GasVar_state,
    uint32_t& nb_var
) {
    uint32_t  tmp_var = 0;
    uint32_t popcnt = 0;
    nb_var = std::numeric_limits<uint32_t>::max();
    uint32_t i;
    tmp_clause.clear();


    for(i = 0; i < size*64; i++) {
        if (this->operator[](i)){
            popcnt++;
            tmp_var = col_to_var[i];
            tmp_clause.push_back(Lit(tmp_var, false));
            if( !GasVar_state[tmp_var] ){  //nobasic
                nb_var = tmp_var;
                break;
            }else{  // basic
                Lit tmp(tmp_clause[0]);
                tmp_clause[0] = tmp_clause.back();
                tmp_clause.back() = tmp;
            }
        }
    }

    for( i = i + 1 ; i <  size*64; i++) {
        if (this->operator[](i)){
            popcnt++;
            tmp_var = col_to_var[i];
            tmp_clause.push_back(Lit(tmp_var, false));
            if( GasVar_state[tmp_var] ){  //basic
                Lit tmp(tmp_clause[0]);
                tmp_clause[0] = tmp_clause.back();
                tmp_clause.back() = tmp;
            }
        }
    }
    assert(tmp_clause.size() == popcnt);
    assert( popcnt == 0 || GasVar_state[ tmp_clause[0].var() ]) ;
    return popcnt;

}

int PackedRow::propGause(
    vector<Lit>& tmp_clause,
    const vector<lbool>& assigns,
    const vector<uint32_t>& col_to_var,
    vec<bool> &GasVar_state,
    uint32_t& nb_var,
    uint32_t start
) {

    bool final = !is_true_internal;
    nb_var = std::numeric_limits<uint32_t>::max();
    tmp_clause.clear();

    for ( uint32_t i = start/64; i != size; i++) if (mp[i]) {
        uint64_t tmp = mp[i];
        uint32_t i2;
        for (i2 = 0 ; i2 < 64; i2++) {
            if(tmp & 1){
                const uint32_t& var = col_to_var[ i * 64  + i2];
                const lbool& val= assigns[var];
                if (val == l_Undef &&  !GasVar_state[var] ){  // find non basic value
                    nb_var = var;
                    return 5;   // nothing
                }
                const bool val_bool = val == l_True;
                final ^= val_bool;
                tmp_clause.push_back(Lit(var, val_bool));
                if ( GasVar_state[var] ) {
                    Lit tmp_lit(tmp_clause[0]);
                    tmp_clause[0] = tmp_clause.back();
                    tmp_clause.back() = tmp_lit;
                }
            }
            tmp >>= 1;
        }
    }
    for ( uint32_t i =0; i != start/64; i++) if (mp[i]) {
        uint64_t tmp = mp[i]; 
        uint32_t i2;
        for (i2 = 0 ; i2 < 64; i2++) {
            if(tmp & 1){
                const uint32_t& var = col_to_var[ i * 64  + i2];
                const lbool& val= assigns[var];
                if (val == l_Undef &&  !GasVar_state[var] ){  // find non basic value
                    nb_var = var;
                    return 5;   // nothing
                }
                const bool val_bool = val == l_True;
                final ^= val_bool;
                tmp_clause.push_back(Lit(var, val_bool));
                if ( GasVar_state[var] ) {
                    Lit tmp_lit(tmp_clause[0]);
                    tmp_clause[0] = tmp_clause.back();
                    tmp_clause.back() = tmp_lit;
                }
            }
            tmp >>= 1;
        }
    }

    if (assigns[tmp_clause[0].var()] == l_Undef) {    // propogate
        tmp_clause[0] = tmp_clause[0].unsign()^final;
        return 2;  // propogate
    } else if (!final) {
        return 0;  // conflict
    }
    // this row already true
    return 4;  // nothing

}




