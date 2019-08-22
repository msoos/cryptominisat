/******************************************
Copyright (c) 2018  Mate Soos
Copyright (c) 2012  Cheng-Shen Han
Copyright (c) 2012  Jie-Hong Roland Jiang

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

#ifndef PACKEDROW_H
#define PACKEDROW_H

//#define DEBUG_ROW

#include <vector>
#include <cstdint>
#include <string.h>
#include <iostream>
#include <algorithm>
#include <limits>

#include "solvertypes.h"
#include "popcnt.h"
#include "Vec.h"

namespace CMSat {

using std::vector;

class PackedMatrix;
class EGaussian;

class PackedRow
{
public:
    PackedRow& operator=(const PackedRow& b)
    {
        #ifdef DEBUG_ROW
        assert(size > 0);
        assert(b.size > 0);
        assert(b.size == size);
        #endif

        for (int i = -1; i != (int)size; i++) {
            *(mp + i) = *(b.mp + i);
        }

        return *this;
    }

    PackedRow& operator^=(const PackedRow& b)
    {
        #ifdef DEBUG_ROW
        assert(size > 0);
        assert(b.size > 0);
        assert(b.size == size);
        #endif

        for (uint32_t i = 0; i != size; i++) {
            *(mp + i) ^= *(b.mp + i);
        }

        rhs_internal ^= b.rhs_internal;
        return *this;
    }

    PackedRow& operator&=(const PackedRow& b)
    {
        #ifdef DEBUG_ROW
        assert(size > 0);
        assert(b.size > 0);
        assert(b.size == size);
        #endif

        for (uint32_t i = 0; i != size; i++) {
            *(mp + i) &= *(b.mp + i);
        }

        return *this;
    }

    void and_inv(const PackedRow& b)
    {
        #ifdef DEBUG_ROW
        assert(size > 0);
        assert(b.size > 0);
        assert(b.size == size);
        #endif

        for (uint32_t i = 0; i != size; i++) {
            *(mp + i) &= ~(*(b.mp + i));
        }
    }

    void xor_in(const PackedRow& b)
    {
        #ifdef DEBUG_ROW
        assert(size > 0);
        assert(b.size > 0);
        assert(b.size == size);
        #endif

        for (uint32_t i = 0; i != size; i++) {
            *(mp + i) ^= *(b.mp + i);
        }

        rhs_internal ^= b.rhs_internal;
    }


    uint32_t popcnt() const;
    bool popcnt_is_one() const
    {
        int ret = 0;
        for (uint32_t i = 0; i != size; i++) {
            ret += my_popcnt(mp[i]&0xffffffff);
            ret += my_popcnt(mp[i]>>32);
            if (ret > 1) return false;
        }
        return ret == 1;
    }

    bool popcnt_is_one(uint32_t from) const
    {
        from++;

        uint64_t tmp = mp[from/64];
        tmp >>= from%64;
        if (tmp) return false;

        for (uint32_t i = from/64+1; i != size; i++)
            if (mp[i]) return false;
        return true;
    }

    inline const uint64_t& rhs() const
    {
        return rhs_internal;
    }

    inline uint64_t& rhs()
    {
        return rhs_internal;
    }

    inline bool isZero() const
    {
        for (uint32_t i = 0; i != size; i++) {
            if (mp[i]) return false;
        }
        return true;
    }

    inline void setZero()
    {
        memset(mp, 0, sizeof(uint64_t)*size);
    }

    inline void clearBit(const uint32_t i)
    {
        mp[i/64] &= ~((uint64_t)1 << (i%64));
    }

    inline void setBit(const uint32_t i)
    {
        mp[i/64] |= ((uint64_t)1 << (i%64));
    }

    inline void invert_rhs(const bool b = true)
    {
        rhs_internal ^= (uint64_t)b;
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

        uint32_t i = size+1;
        while(i != 0) {
            std::swap(*mp1, *mp2);
            mp1++;
            mp2++;
            i--;
        }
    }

    inline bool operator[](const uint32_t& i) const
    {
        #ifdef DEBUG_ROW
        assert(size*64 > i);
        #endif

        return (mp[i/64] >> (i%64)) & 1;
    }

    template<class T>
    void set(const T& v, const vector<uint32_t>& var_to_col, const uint32_t matrix_size)
    {
        //(xorclause, var_to_col, origMat.num_cols)
        assert(size == (matrix_size/64) + ((bool)(matrix_size % 64)));
        //mp = new uint64_t[size];
        setZero();
        for (uint32_t i = 0; i != v.size(); i++) {
            const uint32_t toset_var = var_to_col[v[i]];
            assert(toset_var != std::numeric_limits<uint32_t>::max());

            setBit(toset_var);
        }

        rhs_internal = v.rhs;
    }

    // using find nonbasic and basic value
    uint32_t find_watchVar(
        vector<Lit>& tmp_clause,
        const vector<uint32_t>& col_to_var,
        vector<char> &var_has_resp_row,
        uint32_t& non_resp_var);

    // using find nonbasic value after watch list is enter
    gret propGause(
        vector<Lit>& tmp_clause,
        const vector<lbool>& assigns,
        const vector<uint32_t>& col_to_var,
        vector<char> &var_has_resp_row,
        uint32_t& new_resp_var,
        PackedRow& tmp_col,
        PackedRow& cols_vals,
        PackedRow& cols_set
    );

private:
    friend class PackedMatrix;
    friend class EGaussian;
    friend std::ostream& operator << (std::ostream& os, const PackedRow& m);

    PackedRow(const uint32_t _size, uint64_t*  const _mp) :
        mp(_mp+1)
        , rhs_internal(*_mp)
        , size(_size)
    {}

    uint64_t* __restrict const mp;
    uint64_t& rhs_internal;
    const uint32_t size;
};

inline std::ostream& operator << (std::ostream& os, const PackedRow& m)
{
    for(uint32_t i = 0; i < m.size*64; i++) {
        os << (int)m[i];
    }
    os << " -- rhs: " << m.rhs();
    return os;
}

inline uint32_t PackedRow::popcnt() const
{
    int ret = 0;
    for (uint32_t i = 0; i != size; i++) {
        ret += my_popcnt(mp[i]&0xffffffff);
        ret += my_popcnt(mp[i]>>32);
    }
    return ret;
}

}

#endif //PACKEDROW_H
