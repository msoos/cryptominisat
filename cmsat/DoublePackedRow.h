/***************************************************************************************[Solver.cc]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2009, Niklas Sorensson
Copyright (c) 2009-2012, Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef DOUBLEPACKEDROW_H
#define DOUBLEPACKEDROW_H

#include <stdlib.h>
#include "cmsat/constants.h"
#include "cmsat/SolverTypes.h"

namespace CMSat
{
using namespace CMSat;

class DoublePackedRow
{
    private:
        class BitIter {
            public:
                inline void operator=(const lbool toSet)
                {
                    val &= ~((unsigned char)3 << offset);
                    val |= toSet.value << offset;
                }

                inline operator lbool() const
                {
                    return lbool((val >> offset) & 3);
                }

                inline const bool isUndef() const {
                    return ((lbool)*this).isUndef();
                }
                inline const bool isDef() const {
                    return ((lbool)*this).isDef();
                }
                inline const bool getBool() const {
                    return ((lbool)*this).getBool();
                }
                inline const bool operator==(lbool b) const {
                    return ((lbool)*this) == b;
                }
                inline const bool operator!=(lbool b) const {
                    return ((lbool)*this) != b;
                }
                const lbool operator^(const bool b) const {
                    return ((lbool)*this) ^ b;
                }

            private:
                friend class DoublePackedRow;
                inline BitIter(unsigned char& mp, const uint32_t _offset) :
                val(mp)
                , offset(_offset)
                {}

                unsigned char& val;
                const uint32_t offset;
        };

        class BitIterConst {
             public:
                inline operator lbool() const
                {
                    return lbool((val >> offset) & 3);
                }

                inline const bool isUndef() const {
                    return ((lbool)*this).isUndef();
                }
                inline const bool isDef() const {
                    return ((lbool)*this).isDef();
                }
                inline const bool getBool() const {
                    return ((lbool)*this).getBool();
                }
                inline const bool operator==(lbool b) const {
                    return ((lbool)*this) == b;
                }
                inline const bool operator!=(lbool b) const {
                    return ((lbool)*this) != b;
                }
                const lbool operator^(const bool b) const {
                    return ((lbool)*this) ^ b;
                }


            private:
                friend class DoublePackedRow;
                inline BitIterConst(unsigned char& mp, const uint32_t _offset) :
                val(mp)
                , offset(_offset)
                {}

                const unsigned char& val;
                const uint32_t offset;
        };

    public:
        DoublePackedRow() :
            numElems(0)
            , mp(NULL)
        {}

        uint32_t size() const
        {
            return numElems;
        }

        void growTo(const uint32_t newNumElems)
        {
            uint32_t oldSize = numElems/4 + (bool)(numElems % 4);
            uint32_t newSize = newNumElems/4 + (bool)(newNumElems % 4);

            if (oldSize >= newSize) {
                numElems = std::max(newNumElems, numElems);
                return;
            }

            mp = (unsigned char*)realloc(mp, newSize*sizeof(unsigned char));
            numElems = newNumElems;
        }

        inline BitIter operator[](const uint32_t at)
        {
            return BitIter(mp[at/4], (at%4)*2);
        }

        inline const BitIterConst operator[](const uint32_t at) const
        {
            return BitIterConst(mp[at/4], (at%4)*2);
        }

        inline void push(const lbool val)
        {
            growTo(numElems+1);
            (*this)[numElems-1] = val;
        }

        /*void clear(const uint32_t at)
        {
            mp[at/32] &= ~((uint64_t)3 << ((at%32)*2));
        }*/

    private:

        Var numElems;
        unsigned char *mp;
};

}; //NAMESPACE MINISAT

#endif //DOUBLEPACKEDROW_H
