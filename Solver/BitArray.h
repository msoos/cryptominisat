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

#ifndef __BitArray__
#define __BitArray__

//#define DEBUG_BITARRAY

#include <stdint.h>
#include <string.h>

#ifndef uint
#define uint unsigned int
#endif

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
        if (b.size != size) {
            delete[] mp;
            mp = new uint64_t[size];
        }
        memcpy(mp, b.mp, sizeof(uint64_t)*size);
    }
    
    void resize(uint _size)
    {
        _size = _size/64 + _size%64;
        if (size != _size) {
            delete[] mp;
            size = _size;
            mp = new uint64_t[size];
        }
    }
    
    ~BitArray()
    {
        delete[] mp;
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
        memset(mp, 0, size*sizeof(uint64_t));
    }

    inline void clearBit(const uint i)
    {
        mp[i/64] &= ~((uint64_t)1 << (i%64));
    }

    inline void setBit(const uint i)
    {
        mp[i/64] |= ((uint64_t)1 << (i%64));
    }

    inline const bool operator[](const uint& i) const
    {
        #ifdef DEBUG_BITARRAY
        assert(size*64 > i);
        #endif
        
        return (mp[i/64] >> (i%64)) & 1;
    }

private:
    
    uint size;
    uint64_t* mp;
};

#endif

