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

#ifndef __my_row__
#define __my_row__

//#define DEBUG_ROW

#include <vector>
#include <limits.h>
#include "SolverTypes.h"
#include "Vec.h"
#include <iostream>
#include <algorithm>
#include <boost/foreach.hpp>

#ifndef uint
#define uint unsigned int
#endif

//#define DEBUG_GAUSS

using std::vector;


class my_row
{
public:
    my_row() :
        size(0)
        , mp(NULL)
    {
        xor_clause_inverted = false;
    }
    
    my_row(const my_row& b) :
        size(b.size)
        , xor_clause_inverted(b.xor_clause_inverted)
    {
        mp = new uint64_t[size];
        std::copy(b.mp, b.mp+size, mp);
    }
    
    ~my_row()
    {
        delete[] mp;
    }
    
    bool operator ==(const my_row& b) const;
    
    bool operator !=(const my_row& b) const;
    
    my_row& operator=(const my_row& b);
    
    bool popcnt_is_one() const;
    bool popcnt_is_one(uint from) const;

    inline const bool& get_xor_clause_inverted() const
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
        std::fill(mp, mp+size, 0);
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

    void swap(my_row& b)
    {
        #ifdef DEBUG_ROW
        assert(size > 0);
        assert(b.size > 0);
        assert(b.size == size);
        #endif
        
        uint64_t* tmp3 = mp;
        mp = b.mp;
        b.mp = tmp3;
        
        const bool tmp(xor_clause_inverted);
        xor_clause_inverted = b.xor_clause_inverted;
        b.xor_clause_inverted = tmp;
    }

    my_row& operator^=(const my_row& b);

    inline const bool operator[](const uint& i) const
    {
        #ifdef DEBUG_ROW
        assert(size*64 > i);
        #endif
        
        return (mp[i/64] >> (i%64)) & 1;
    }

    template<class T>
    void set(const T& v, const vector<uint>& var_to_col, const uint matrix_size)
    {
        size = (matrix_size/64) + ((bool)(matrix_size % 64));
        mp = new uint64_t[size];
        setZero();
        for (uint i = 0; i < v.size(); i++) {
            const uint toset_var = var_to_col[v[i].var()];
            assert(toset_var != UINT_MAX);
            
            setBit(toset_var);
        }
        
        xor_clause_inverted = v.xor_clause_inverted();
    }
    
    inline unsigned long int scan(const unsigned long int var) const
    {
        #ifdef DEBUG_ROW
        assert(size > 0);
        #endif
        
        for(uint i = var; i < size*64; i++)
            if (this->operator[](i)) return i;
        return ULONG_MAX;
    }

    friend std::ostream& operator << (std::ostream& os, const my_row& m);

private:
    
    uint size;
    uint64_t* mp;
    bool xor_clause_inverted;
};

std::ostream& operator << (std::ostream& os, const my_row& m);

#endif

