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
#include <stdint.h>

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
    
    PackedRow& operator=(const PackedRow& b)
    {
        #ifdef DEBUG_ROW
        assert(size > 0);
        assert(b.size > 0);
        assert(size == b.size);
        #endif
        
        memcpy(mp-1, b.mp-1, size+1);
        return *this;
    }
    
    PackedRow& operator^=(const PackedRow& b)
    {
        #ifdef DEBUG_ROW
        assert(size > 0);
        assert(b.size > 0);
        assert(b.size == size);
        #endif
        
        for (uint i = 0; i != size; i++) {
            *(mp + i) ^= *(b.mp + i);
        }
        
        is_true_internal ^= b.is_true_internal;
        return *this;
    }
    
    void xorBoth(const PackedRow& b)
    {
        #ifdef DEBUG_ROW
        assert(size > 0);
        assert(b.size > 0);
        assert(b.size == size);
        #endif
        
        for (uint i = 0; i != 2*size+1; i++) {
            *(mp + i) ^= *(b.mp + i);
        }
        
        is_true_internal ^= b.is_true_internal;
    }
    
    
    uint popcnt() const;
    uint popcnt(uint from) const;
    
    bool popcnt_is_one() const
    {
        char popcount = 0;
        for (uint i = 0; i != size; i++) {
            uint64_t tmp = mp[i];
            while(tmp) {
                popcount += tmp & 1;
                tmp >>= 1;
            }
        }
        return popcount == 1;
    }
    
    bool popcnt_is_one(uint from) const
    {
        from++;
        
        uint64_t tmp = mp[from/64];
        tmp >>= from%64;
        if (tmp) return false;
        
        for (uint i = from/64+1; i != size; i++)
            if (mp[i]) return false;
        return true;
    }

    inline const uint64_t& is_true() const
    {
        return is_true_internal;
    }

    inline const bool isZero() const
    {
        for (uint i = 0; i != size; i++) {
            if (mp[i]) return false;
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

    inline void invert_is_true(const bool b = true)
    {
        is_true_internal ^= (uint64_t)b;
    }

    inline void setBit(const uint i)
    {
        mp[i/64] |= ((uint64_t)1 << (i%64));
    }
    
    void swapBoth(PackedRow b)
    {
        #ifdef DEBUG_ROW
        assert(size > 0);
        assert(b.size > 0);
        assert(b.size == size);
        #endif
        
        uint64_t * __restrict mp1 = mp-1;
        uint64_t * __restrict mp2 = b.mp-1;
        
        uint i = 2*(size+1);
        
        while(i != 0) {
            std::swap(*mp1, *mp2);
            mp1++;
            mp2++;
            i--;
        }
    }

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
        for (uint i = 0; i != v.size(); i++) {
            const uint toset_var = var_to_col[v[i].var()];
            assert(toset_var != UINT_MAX);
            
            setBit(toset_var);
        }
        
        is_true_internal = !v.xor_clause_inverted();
    }
    
    void fill(vec<Lit>& tmp_clause, const vec<lbool>& assigns, const vector<Var>& col_to_var_original) const;
    
    inline unsigned long int scan(const unsigned long int var) const
    {
        #ifdef DEBUG_ROW
        assert(size > 0);
        #endif
        
        for(uint i = var; i != size*64; i++)
            if (this->operator[](i)) return i;
        return ULONG_MAX;
    }

    friend std::ostream& operator << (std::ostream& os, const PackedRow& m);

private:
    friend class PackedMatrix;
    PackedRow(const uint _size, uint64_t*  const _mp) :
        size(_size)
        , mp(_mp+1)
        , is_true_internal(*_mp)
    {}
    
    const uint size;
    uint64_t* __restrict const mp;
    uint64_t& is_true_internal;
};

std::ostream& operator << (std::ostream& os, const PackedRow& m);

#endif //PACKEDROW_H

