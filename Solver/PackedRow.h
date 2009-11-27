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

#ifndef PACKEDROW_H
#define PACKEDROW_H

//#define DEBUG_ROW

#include <vector>
#include <limits.h>
#include "SolverTypes.h"
#include "Vec.h"
#include <string.h>
#include <iostream>
#include <algorithm>

#ifndef uint
#define uint unsigned int
#endif

using std::vector;


class PackedMatrix;

class PackedRow
{
public:
    bool operator ==(const PackedRow& b) const;
    bool operator !=(const PackedRow& b) const;
    PackedRow& operator=(const PackedRow& b);
    uint popcnt() const;
    uint popcnt(uint from) const;
    bool popcnt_is_one() const;
    bool popcnt_is_one(uint from) const;

    inline const uint64_t& get_xor_clause_inverted() const
    {
        return xor_clause_inverted;
    }

    inline const bool isZero() const
    {
        const uint64_t*  mp2 = (const uint64_t*)mp;
        
        for (uint i = 0; i < size; i++) {
            if (mp2[i]) return false;
        }
        return true;
    }

    inline void setZero()
    {
        memset(mp, 0, sizeof(uint64_t)*size);
    }

    inline void clearBit(const uint i)
    {
        mp[i/64] &= ~((uint64_t)1 << (i%64));
    }

    inline void invert_xor_clause_inverted(const bool b = true)
    {
        xor_clause_inverted ^= b;
    }

    inline void setBit(const uint i)
    {
        mp[i/64] |= ((uint64_t)1 << (i%64));
    }
    
    void swap(PackedRow b)
    {
        #ifdef DEBUG_ROW
        assert(size > 0);
        assert(b.size > 0);
        assert(b.size == size);
        #endif
        
        memcpy(tmp_row, b.mp-1, sizeof(uint64_t)*(size+1));
        memcpy(b.mp-1, mp-1, sizeof(uint64_t)*(size+1));
        memcpy(mp-1, tmp_row, sizeof(uint64_t)*(size+1));
    }

    PackedRow& operator^=(const PackedRow& b);

    inline const bool operator[](const uint& i) const
    {
        #ifdef DEBUG_ROW
        assert(size*64 > i);
        #endif
        
        return (mp[i/64] >> (i%64)) & 1;
    }

    template<class T>
    void set(const T& v, const vector<uint16_t>& var_to_col, const uint matrix_size)
    {
        assert(size == (matrix_size/64) + ((bool)(matrix_size % 64)));
        //mp = new uint64_t[size];
        setZero();
        for (uint i = 0; i < v.size(); i++) {
            const uint toset_var = var_to_col[v[i].var()];
            assert(toset_var != UINT_MAX);
            
            setBit(toset_var);
        }
        
        xor_clause_inverted = v.xor_clause_inverted();
    }
    
    void fill(Lit* ps, const vec<lbool>& assigns, const vector<Var>& col_to_var_original) const;
    
    inline unsigned long int scan(const unsigned long int var) const
    {
        #ifdef DEBUG_ROW
        assert(size > 0);
        #endif
        
        for(uint i = var; i < size*64; i++)
            if (this->operator[](i)) return i;
        return ULONG_MAX;
    }

    friend std::ostream& operator << (std::ostream& os, const PackedRow& m);
    
    static __thread uint64_t *tmp_row;

private:
    friend class PackedMatrix;
    PackedRow(const uint _size, uint64_t& _xor_clause_inverted, uint64_t*  const _mp) :
        size(_size)
        , xor_clause_inverted(_xor_clause_inverted)
        , mp(_mp)
    {}
    
    const uint size;
    uint64_t* const mp;
    uint64_t& xor_clause_inverted;
};

std::ostream& operator << (std::ostream& os, const PackedRow& m);

#endif //PACKEDROW_H

