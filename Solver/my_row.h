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

#include <deque>
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
#define STD_VECTOR_BOOL_NOT_SPECIAL

using std::deque;
using std::vector;


class my_row
{
public:
    my_row()
    {
        xor_clause_inverted = false;
        popcount = 0;
    }

    inline const bool& get_xor_clause_inverted() const
    {
        return xor_clause_inverted;
    }

    inline const bool isZero() const
    {
        return popcount == 0;
    }

    inline void setZero()
    {
        std::fill(mp.begin(), mp.end(), false);
        popcount = 0;
    }

    inline void clearBit(const uint b)
    {
        #ifdef DEBUG_GAUSS
        assert(mp[b]);
        #endif
        mp[b] = false;
        popcount--;
    }

    inline void invert_xor_clause_inverted(const bool b = true)
    {
        xor_clause_inverted ^= b;
    }

    inline void setBit(const uint v)
    {
        #ifdef DEBUG_GAUSS
        assert(!mp[v]);
        #endif
        mp[v] = true;
        popcount++;
    }

    void swap(my_row& b)
    {
        mp.swap(b.mp);
        
        const bool tmp(xor_clause_inverted);
        xor_clause_inverted = b.xor_clause_inverted;
        b.xor_clause_inverted = tmp;
        
        const uint tmp2(popcount);
        popcount = b.popcount;
        b.popcount = tmp2;
    }

    inline my_row& operator^=(const my_row& b)
    {
        popcount = 0;
        typedef deque<bool>::iterator myit;
        typedef deque<bool>::const_iterator myit2;
        myit it = mp.begin();
        myit2 it2 = b.mp.begin();
        myit2 end = mp.end();
        for(; it != end; it++, it2++) {
            *it ^= *it2;
            popcount += *it;
        }
        xor_clause_inverted ^= !b.xor_clause_inverted;
        return *this;
    }
    
    inline void noupdate_xor(const my_row& b)
    {
        typedef deque<bool>::iterator myit;
        typedef deque<bool>::const_iterator myit2;
        myit it = mp.begin();
        myit2 it2 = b.mp.begin();
        myit2 end = mp.end();
        for(; it != end; it++, it2++) {
            *it ^= *it2;
        }
        xor_clause_inverted ^= !b.xor_clause_inverted;
    }

    inline const bool operator[](const uint& i) const
    {
        return mp[i];
    }

    template<class T>
    void set(const T& v, const vector<uint>& var_to_col)
    {
        mp.resize(var_to_col.size());
        std::fill(mp.begin(), mp.end(), false);
        for (uint i = 0, size = v.size(); i < size; i++) {
            const uint toset_var = var_to_col[v[i].var()];

            assert(toset_var != UINT_MAX);
            mp[toset_var] = true;
        }
        
        popcount = v.size();
        xor_clause_inverted = v.xor_clause_inverted();
    }
    
    inline const uint popcnt() const
    {
        return popcount;
    }
    
    inline const uint real_popcnt()
    {
        popcount = 0;
        typedef deque<bool>::iterator myit;
        for(myit it = mp.begin(), end = mp.end(); it != end; it++) {
            popcount += *it;
        }
        return popcount;
    }
    
    inline unsigned long int scan(const unsigned long int var) const
    {
        for(uint i = var; i < mp.size(); i++)
            if (mp[i]) return i;
        return ULONG_MAX;
    }

    void fill(Lit* ps, const vec<lbool>& assigns, const vector<uint>& col_to_var_original) const;

    friend std::ostream& operator << (std::ostream& os, const my_row& m);

private:
    
    uint popcount;
    bool xor_clause_inverted;
    deque<bool> mp;
};

std::ostream& operator << (std::ostream& os, const my_row& m);

#endif

