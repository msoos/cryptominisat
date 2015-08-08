/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.0 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/


#ifndef BITARRAY_H
#define BITARRAY_H

//#define DEBUG_BITARRAY

#include <string.h>
#include <assert.h>
#include "constants.h"
#include <stdlib.h>


namespace CMSat {

class BitArray
{
public:
    BitArray() :
        size(0)
        , mp(NULL)
    {
    }

    BitArray(const BitArray& b) :
        size(b.size)
    {
        mp = (uint64_t*)malloc(size*sizeof(uint64_t));
        assert(mp != NULL);
        memcpy(mp, b.mp, sizeof(uint64_t)*size);
    }

    ~BitArray()
    {
        free(mp);
    }

    BitArray& operator=(const BitArray& b)
    {
        if (size != b.size) {
            mp = (uint64_t*)realloc(mp, b.size*sizeof(uint64_t));
            assert(mp != NULL);
            size = b.size;
        }
        memcpy(mp, b.mp, size*sizeof(uint64_t));

        return *this;
    }

    BitArray& operator&=(const BitArray& b)
    {
        assert(size == b.size);
        uint64_t* t1 = mp;
        uint64_t* t2 = b.mp;
        for (uint64_t i = 0; i < size; ++i) {
            *t1 &= *t2;
            ++t1;
            ++t2;
        }

        return *this;
    }

    BitArray& removeThese(const BitArray& b)
    {
        assert(size == b.size);
        uint64_t* t1 = mp;
        uint64_t* t2 = b.mp;
        for (uint64_t i = 0; i < size; ++i) {
            *t1 &= ~(*t2);
            ++t1;
            ++t2;
        }

        return *this;
    }

    template<class T>
    BitArray& removeThese(const T& rem)
    {
        for (size_t i = 0; i < rem.size(); ++i) {
            clearBit(rem[i]);
        }

        return *this;
    }

    template<class T>
    BitArray& removeTheseLit(const T& rem)
    {
        for (uint32_t i = 0; i < rem.size(); i++) {
            clearBit(rem[i].var());
        }

        return *this;
    }

    void resize(uint32_t _size, const bool fill)
    {
        _size = _size/64 + (bool)(_size%64);
        if (size != _size) {
            mp = (uint64_t*)realloc(mp, _size*sizeof(uint64_t));
            assert(mp != NULL);
            size = _size;
        }
        if (fill) setOne();
        else setZero();
    }

    inline bool isZero() const
    {
        const uint64_t*  mp2 = (const uint64_t*)mp;

        for (uint32_t i = 0; i < size; i++) {
            if (mp2[i]) return false;
        }
        return true;
    }

    inline void setZero()
    {
        memset(mp, 0, size*sizeof(uint64_t));
    }

    inline void setOne()
    {
        memset(mp, 0xff, size*sizeof(uint64_t));
    }

    inline void clearBit(const uint32_t i)
    {
        #ifdef DEBUG_BITARRAY
        assert(size*64 > i);
        #endif

        mp[i/64] &= ~((uint64_t)1 << (i%64));
    }

    inline void setBit(const uint32_t i)
    {
        #ifdef DEBUG_BITARRAY
        assert(size*64 > i);
        #endif

        mp[i/64] |= ((uint64_t)1 << (i%64));
    }

    inline bool operator[](const uint32_t& i) const
    {
        #ifdef DEBUG_BITARRAY
        assert(size*64 > i);
        #endif

        return (mp[i/64] >> (i%64)) & 1;
    }

    inline uint32_t getSize() const
    {
        return size*64;
    }

private:

    uint32_t size;
    uint64_t* mp;
};

} //end namespace

#endif //BITARRAY_H

