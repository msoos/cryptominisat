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

#ifndef __mpz_class__
#define __mpz_class__

#include "gmp.h"
#include <vector>
#include <limits.h>
#include "SolverTypes.h"
#include "Vec.h"
#include "mpzstack.h"
#include <iostream>

#ifndef uint
#define uint unsigned int
#endif

//#define DEBUG_GAUSS

using std::vector;


class mpz_class
{
public:
    mpz_class() :
        mp(mpzstack.pop(true))
    {
        //mpz_init(mp);
        xor_clause_inverted = false;
        popcount = 0;
    }

    ~mpz_class()
    {
        mpzstack.push(mp);
        //mpz_clear(mp);
    }

    mpz_class(const mpz_class& b) :
        mp(mpzstack.pop())
    {
        //mpz_init_set(mp, b.mp);
        mpz_set(mp, b.mp);
        xor_clause_inverted = b.xor_clause_inverted;
        popcount = b.popcount;
    }

    inline bool operator ==(const mpz_class& b) const
    {
        return (xor_clause_inverted == b.xor_clause_inverted && popcount == b.popcount && mpz_cmp(b.mp, mp) == 0);
    }

    inline bool operator !=(const mpz_class& b) const
    {
        return (xor_clause_inverted != b.xor_clause_inverted || popcount != b.popcount || mpz_cmp(b.mp, mp) != 0);
    }

    inline const bool& get_xor_clause_inverted() const
    {
        return xor_clause_inverted;
    }

    inline const bool isZero() const
    {
        //return (mpz_cmp_ui(mp, 0) == 0);
        return popcount == 0;
    }

    inline void setZero()
    {
        mpz_set_ui(mp, 0);
        popcount = 0;
    }

    inline void clearBit(const uint b)
    {
        #ifdef DEBUG_GAUSS
        assert(mpz_tstbit(mp, b));
        #endif
        mpz_clrbit(mp, b);
        popcount--;
    }

    inline void invert_xor_clause_inverted(const bool b = true)
    {
        xor_clause_inverted ^= b;
    }

    inline mpz_class& operator &= (const mpz_class& b)
    {
        assert(false);
        mpz_and(mp, mp, b.mp);
        return *this;
    }

    inline void setBit(const uint v)
    {
        #ifdef DEBUG_GAUSS
        assert(!mpz_tstbit(mp, v));
        #endif
        mpz_setbit(mp, v);
        popcount++;
    }

    void swap(mpz_class& b)
    {
        mpz_swap(mp, b.mp);
        
        const bool tmp(xor_clause_inverted);
        xor_clause_inverted = b.xor_clause_inverted;
        b.xor_clause_inverted = tmp;
        
        const uint tmp2(popcount);
        popcount = b.popcount;
        b.popcount = tmp2;
    }

    inline mpz_class& operator=(const mpz_class& b)
    {
        mpz_set(mp, b.mp);
        xor_clause_inverted = b.xor_clause_inverted;
        popcount = b.popcount;
        return *this;
    }

    inline mpz_class& operator^=(const mpz_class& b)
    {
        mpz_xor(mp, mp, b.mp);
        xor_clause_inverted ^= !b.xor_clause_inverted;
        popcount = mpz_popcount(mp);
        return *this;
    }
    
    inline void noupdate_xor(const mpz_class& b)
    {
        mpz_xor(mp, mp, b.mp);
        xor_clause_inverted ^= !b.xor_clause_inverted;
    }

    inline const bool operator[](const uint& i) const
    {
        return mpz_tstbit(mp, i);
    }

    template<class T>
    void set(const T& v, const vector<uint>& var_to_col)
    {
        mpz_set_ui(mp, 0);
        for (uint i = 0, size = v.size(); i < size; i++) {
            const uint toset_var = var_to_col[v[i].var()];

            assert(toset_var != UINT_MAX);
            mpz_setbit(mp, toset_var);
        }
        
        popcount = v.size();
        xor_clause_inverted = v.xor_clause_inverted();
    }

    inline unsigned long int scan(const unsigned long int var) const
    {
        return mpz_scan1(mp, var);
    }

    /*static inline void setNumBits(const uint num_bits)
    {
        mpzstack.setNumBits(num_bits);
    }
    static inline const uint getNumBits()
    {
        return mpzstack.getNumBits();
    }*/
    
    inline const uint popcnt() const
    {
        return popcount;
    }
    
    inline const uint real_popcnt() const
    {
        return mpz_popcount(mp);
    }

    void fill(Lit* ps, const vec<lbool>& assigns, const vector<uint>& col_to_var_original) const;

    friend std::ostream& operator << (std::ostream& os, const mpz_class& m);

private:
    
    mpz_t& mp;
    uint popcount;
    bool xor_clause_inverted;

    static MpzStack mpzstack;
};

std::ostream& operator << (std::ostream& os, const mpz_class& m);


#endif

