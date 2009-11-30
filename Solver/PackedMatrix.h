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

#ifndef PACKEDMATRIX_H
#define PACKEDMATRIX_H

#include "PackedRow.h"
#include <algorithm>

//#define DEBUG_MATRIX

#ifndef uint
#define uint unsigned int
#endif

class PackedMatrix
{
public:
    PackedMatrix() :
        numRows(0)
        , numCols(0)
        , mp(NULL)
    {
    }
    
    PackedMatrix(const PackedMatrix& b) :
        numRows(b.numRows)
        , numCols(b.numCols)
    {
        mp = new uint64_t[numRows*(numCols+1)];
        std::copy(b.mp, b.mp+numRows*(numCols+1), mp);
    }
    
    ~PackedMatrix()
    {
        delete[] mp;
    }
    
    void resize(const uint num_rows, uint num_cols)
    {
        num_cols = num_cols / 64 + (bool)(num_cols % 64);
        if (numRows*(numCols+1) < num_rows*(num_cols+1)) {
            delete[] mp;
            mp = new uint64_t[num_rows*(num_cols+1)];
        }
        numRows = num_rows;
        numCols = num_cols;
    }
    
    void resizeNumRows(const uint num_rows)
    {
        #ifdef DEBUG_MATRIX
        assert(num_rows <= numRows);
        #endif
        
        numRows = num_rows;
    }
    
    PackedMatrix& operator=(const PackedMatrix& b)
    {
        if (b.numRows*(b.numCols+1) > numRows*(numCols+1)) {
            delete[] mp;
            mp = new uint64_t[b.numRows*(b.numCols+1)];
        }
        
        numRows = b.numRows;
        numCols = b.numCols;
        std::copy(b.mp, b.mp+numRows*(numCols+1), mp);
        
        return *this;
    }

    inline PackedRow operator[](const uint i)
    {
        #ifdef DEBUG_MATRIX
        assert(i <= numRows);
        #endif
        
        return PackedRow(numCols, *(mp+i*(numCols+1)), mp+i*(numCols+1)+1);
    }
    
    inline const PackedRow operator[](const uint i) const
    {
        #ifdef DEBUG_MATRIX
        assert(i <= numRows);
        #endif
        
        return PackedRow(numCols, *(mp+i*(numCols+1)), mp+i*(numCols+1)+1);
    }
    
    class iterator
    {
    public:
        PackedRow operator*()
        {
            return PackedRow(numCols, *mp, mp+1);
        }
        
        iterator& operator++()
        {
            mp += numCols+1;
            return *this;
        }
        
        iterator operator+(const uint num) const
        {
            iterator ret(*this);
            ret.mp += (numCols+1)*num;
            return ret;
        }
        
        void operator+=(const uint num)
        {
            mp += (numCols+1)*num;
        }
        
        const bool operator!=(const iterator& it) const
        {
            return mp != it.mp;
        }
        
        const bool operator==(const iterator& it) const
        {
            return mp == it.mp;
        }
        
    private:
        friend class PackedMatrix;
        
        iterator(uint64_t* _mp, const uint _numCols) :
            mp(_mp)
            , numCols(_numCols)
        {}
        
        uint64_t* mp;
        const uint numCols;
    };
    
    inline iterator begin()
    {
        return iterator(mp, numCols);
    }
    
    inline iterator end()
    {
        return iterator(mp+numRows*(numCols+1), numCols);
    }
    
    /*class const_iterator
    {
        public:
            const PackedRow operator*()
            {
                return PackedRow(numCols, *mp, mp+1);
            }
            
            const_iterator& operator++()
            {
                mp += numCols+1;
                return *this;
            }
            
            const_iterator operator+(const uint num) const
            {
                const_iterator ret(*this);
                ret.mp += (numCols+1)*num;
                return ret;
            }
            
            void operator+=(const uint num)
            {
                mp += (numCols+1)*num;
            }
            
            const bool operator!=(const const_iterator& it) const
            {
                return mp != it.mp;
            }
            
            const bool operator==(const const_iterator& it) const
            {
                return mp == it.mp;
            }
            
        private:
            friend class PackedMatrix;
            
            const_iterator(uint64_t* _mp, const uint _numCols) :
                mp(_mp)
                , numCols(_numCols)
            {}
            
            const uint64_t* mp;
            const uint numCols;
    };
    inline const_iterator begin() const
    {
        return const_iterator(mp, numCols);
    }

    inline const_iterator end() const
    {
        return const_iterator(mp+numRows*(numCols+1), numCols);
    }*/
    
    inline const uint size() const
    {
        return numRows;
    }

private:
    
    uint numRows;
    uint numCols;
    uint64_t* mp;
};

#endif //PACKEDMATRIX_H

