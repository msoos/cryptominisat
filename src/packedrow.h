/******************************************
Copyright (c) 2016, Mate Soos

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

#include "popcnt.h"
#include "constants.h"
#include "solvertypes.h"
#include <string.h>
#include <iostream>
#include <algorithm>
#include <limits>
#include <vector>
using std::vector;

namespace CMSat {

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

        for (uint32_t i = 0; i != size; i++) {
            *(mp + i) ^= *(b.mp + i);
        }

        rhs_internal ^= b.rhs_internal;
        return *this;
    }

    void xorBoth(const PackedRow& b)
    {
        #ifdef DEBUG_ROW
        assert(size > 0);
        assert(b.size > 0);
        assert(b.size == size);
        #endif

        for (uint32_t i = 0; i != 2*size+1; i++) {
            *(mp + i) ^= *(b.mp + i);
        }

        rhs_internal ^= b.rhs_internal;
    }


    uint32_t popcnt() const;

    //Population count INCLUDING from
    uint32_t popcnt(uint32_t from) const;

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

    //popcnt is 1 given that there is a 1 at FROM
    //and there are only zeroes before it
    bool popcnt_is_one(uint32_t from) const
    {
        from++;

        //it's the last bit, there are none after, so it only has 1 bit set
        if (from/64 == size) {
            return true;
        }

        uint64_t tmp = mp[from/64];
        tmp >>= (from%64);
        if (tmp) return false;

        for (uint32_t i = from/64+1; i != size; i++)
            if (mp[i]) return false;
        return true;
    }

    inline const uint64_t& rhs() const
    {
        return rhs_internal;
    }

    bool isZero() const
    {
        for (uint32_t i = 0; i != size; i++) {
            if (mp[i]) return false;
        }
        return true;
    }

    void setZero()
    {
        memset(mp, 0, sizeof(uint64_t)*size);
    }

    void clearBit(const uint32_t i)
    {
        mp[i/64] &= ~((uint64_t)1 << (i%64));
    }

    void invert_is_true(const bool b = true)
    {
        rhs_internal ^= (uint64_t)b;
    }

    void setBit(const uint32_t i)
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

        uint32_t i = 2*(size+1);

        while(i != 0) {
            std::swap(*mp1, *mp2);
            mp1++;
            mp2++;
            i--;
        }
    }

    bool operator[](const uint32_t& i) const
    {
        #ifdef DEBUG_ROW
        assert(size*64 > i);
        #endif

        return (mp[i/64] >> (i%64)) & 1;
    }

    template<class T>
    void set(const T& v, const vector<uint16_t>& var_to_col, const uint32_t num_cols)
    {
        assert(size == (num_cols/64) + ((bool)(num_cols % 64)));
        //mp = new uint64_t[size];
        setZero();
        for (uint32_t i = 0; i != v.size(); i++) {
            const uint32_t toset_var = var_to_col[v[i]];
            assert(toset_var != std::numeric_limits<uint32_t>::max());

            setBit(toset_var);
        }

        rhs_internal = v.rhs;
    }

    bool fill(vector<Lit>& tmp_clause, const vector<lbool>& assigns, const vector<uint32_t>& col_to_var_original) const;

    unsigned long int scan(const unsigned long int var) const
    {
        #ifdef DEBUG_ROW
        assert(size > 0);
        #endif

        for(uint32_t i = var; i != size*64; i++) {
            if (this->operator[](i)) return i;
        }

        return std::numeric_limits<unsigned long int>::max();
    }

    //friend ::std::ostream& operator << (std::ostream& os, const PackedRow& m);
    uint32_t getSize() const
    {
        return size;
    }

private:
    friend class PackedMatrix;
    PackedRow(const uint32_t _size, uint64_t*  const _mp) :
        mp(_mp+1)
        , rhs_internal(*_mp)
        , size(_size)
    {}

    uint64_t* __restrict const mp;
    uint64_t& rhs_internal;
    const uint32_t size;
};

inline std::ostream& operator << (std::ostream& os, const CMSat::PackedRow& m)
{
    for(uint32_t i = 0; i < m.getSize()*64; i++) {
        os << m[i];
    }
    os << " -- rhs: " << m.rhs();
    return os;
}


inline bool PackedRow::operator ==(const PackedRow& b) const
{
    #ifdef DEBUG_ROW
    assert(size > 0);
    assert(b.size > 0);
    assert(size == b.size);
    #endif

    return (std::equal(b.mp-1, b.mp+size, mp-1));
}

inline bool PackedRow::operator !=(const PackedRow& b) const
{
    #ifdef DEBUG_ROW
    assert(size > 0);
    assert(b.size > 0);
    assert(size == b.size);
    #endif

    return (!std::equal(b.mp-1, b.mp+size, mp-1));
}

inline uint32_t PackedRow::popcnt() const
{
    uint32_t popcnt = 0;
    for (uint32_t i = 0; i < size; i++) {
        uint64_t tmp = mp[i];
        while(tmp) {
            popcnt += (tmp & 1);
            tmp >>= 1;
        }
    }
    return popcnt;
}

inline uint32_t PackedRow::popcnt(const uint32_t from) const
{
    uint32_t popcnt = 0;
    for (uint32_t i = from/64; i != size; i++) {
        uint64_t tmp = mp[i];
        if (i == from/64) {
            tmp >>= from%64;
        }
        while (tmp) {
            popcnt += (tmp & 1);
            tmp >>= 1;
        }
    }
    return popcnt;
}

inline bool PackedRow::fill(vector<Lit>& tmp_clause
    , const vector<lbool>& assigns
    , const vector<uint32_t>& col_to_var_original
) const {
    bool final = !rhs_internal;

    tmp_clause.clear();
    uint32_t col = 0;
    bool wasundef = false;
    for (uint32_t i = 0; i < size; i++) for (uint32_t i2 = 0; i2 < 64; i2++) {
        if ((mp[i] >> i2) &1) {
            const uint32_t var = col_to_var_original[col];
            assert(var != std::numeric_limits<uint32_t>::max());

            const lbool val = assigns[var];
            const bool val_bool = val == l_True;
            tmp_clause.push_back(Lit(var, val_bool));
            final ^= val_bool;
            if (val == l_Undef) {
                assert(!wasundef);
                std::swap(tmp_clause[0], tmp_clause.back());
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

} //end namespace

#endif //PACKEDROW_H

