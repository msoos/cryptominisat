/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

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

#ifndef PROPBY_H
#define PROPBY_H

#include "constants.h"
#include "solvertypes.h"
#include "clause.h"

//#define DEBUG_PROPAGATEFROM

#include "cloffset.h"

namespace CMSat {

enum PropByType {
    null_clause_t = 0, clause_t = 1, binary_t = 2,
    xor_t = 3, bnn_t = 4
};

class PropBy
{
    enum bitFieldSizes {
        bitsize_red_step = 1,
        bitsize_data1 = 31,
        bitsize_type = 3,
        bitsize_data2 = 29
    };

    private:
        uint32_t red_step:bitsize_red_step;
        uint32_t data1:bitsize_data1;
        uint32_t type:bitsize_type;
        //0: clause, NULL
        //1: clause, non-null
        //2: binary
        //3: xor
        //4: bnn
        uint32_t data2:bitsize_data2;
        int32_t ID;

    public:
        PropBy() :
            red_step(0)
            , data1(0)
            , type(null_clause_t)
            , data2(0)
        {}

#ifndef LARGE_OFFSETS
        //Normal clause prop
        explicit PropBy(const ClOffset offset) :
            red_step(0)
            , data1(offset)
            , type(clause_t)
            , data2(0)
        {
            //No roll-over
            /*#ifdef DEBUG_PROPAGATEFROM
            assert(offset == get_offset());
            #endif*/
        }
#else
        //Normal clause prop
        explicit PropBy(const ClOffset offset) :
            red_step(0)
            , type(clause_t)
        {
            //No roll-over
            data1 = offset & ((((ClOffset)1) << bitsize_data1) - 1);
            data2 = offset >> bitsize_data1;
            /*#ifdef DEBUG_PROPAGATEFROM
            assert(offset == get_offset());
            #endif*/
        }
#endif

        //XOR
        PropBy(const uint32_t matrix_num, const uint32_t row_num):
            data1(matrix_num)
            , type(xor_t)
            , data2(row_num)
        {
        }

        //BNN prop
        PropBy(uint32_t bnn_idx, void*):
            data1(0xfffffff)
            , type(bnn_t)
            , data2(bnn_idx)
        {
        }

        //Binary prop
        PropBy(const Lit lit, const bool redStep, int32_t _ID) :
            red_step(redStep)
            , data1(lit.toInt())
            , type(binary_t)
            , data2(0)
            , ID(_ID)
        {
        }

        //For hyper-bin, etc.
        PropBy(
            const Lit lit
            , bool redStep //Step that lead here from ancestor is redundant
            , bool hyperBin //It's a hyper-binary clause
            , bool hyperBinNotAdded //It's a hyper-binary clause, but was never added because all the rest was zero-level
            , int32_t _ID
        ) :
            red_step(redStep)
            , data1(lit.toInt())
            , type(binary_t)
            , data2(0)
            , ID(_ID)
        {
            //HACK: if we are doing seamless hyper-bin and transitive reduction
            //then if we are at toplevel, .getAncestor()
            //must work, and return lit_Undef, but at the same time, .isNULL()
            //must also work, for conflict generation. So this is a hack to
            //achieve that. What an awful hack.
            if (lit == ~lit_Undef)
                type = null_clause_t;

            data2 = ((uint32_t)hyperBin) << 1
                | ((uint32_t)hyperBinNotAdded) << 2;
        }

        void set_bnn_reason(uint32_t idx)
        {
            assert(isBNN());
            data1 = idx;
        }

        bool bnn_reason_set() const
        {
            assert(isBNN());
            return data1 != 0xfffffff;
        }

        uint32_t get_bnn_reason() const
        {
            assert(bnn_reason_set());
            return data1;
        }

        uint32_t isBNN() const
        {
            return type == bnn_t;
        }

        uint32_t getBNNidx() const
        {
            assert(isBNN());
            return data2;
        }

        bool isRedStep() const
        {
            return red_step;
        }

        int32_t getID() const
        {
            return ID;
        }

        bool getHyperbin() const
        {
            return data2 & 2U;
        }

        void setHyperbin(bool toSet)
        {
            data2 &= ~2U;
            data2 |= (uint32_t)toSet << 1;
        }

        bool getHyperbinNotAdded() const
        {
            return data2 & 4U;
        }

        void setHyperbinNotAdded(bool toSet)
        {
            data2 &= ~4U;
            data2 |= (uint32_t)toSet << 2;
        }

        Lit getAncestor() const
        {
            #ifdef DEBUG_PROPAGATEFROM
            assert(type == null_clause_t || type == binary_t);
            #endif
            return ~Lit::toLit(data1);
        }

        bool isClause() const
        {
            return type == clause_t;
        }

        PropByType getType() const
        {
            return (PropByType)type;
        }

        Lit lit2() const
        {
            #ifdef DEBUG_PROPAGATEFROM
            assert(type == binary_t);
            #endif
            return Lit::toLit(data1);
        }

        uint32_t get_matrix_num() const
        {
            #ifdef DEBUG_PROPAGATEFROM
            assert(type == xor_t);
            #endif
            return data1;
        }

        uint32_t get_row_num() const
        {
            #ifdef DEBUG_PROPAGATEFROM
            assert(type == xor_t);
            #endif
            return data2;
        }

        ClOffset get_offset() const
        {
            #ifdef DEBUG_PROPAGATEFROM
            assert(isClause());
            #endif
#ifndef LARGE_OFFSETS
            return data1;
#else
            ClOffset offset = data2;
            offset <<= bitsize_data1;
            offset |= data1;
            return offset;
#endif
        }

        bool isNULL() const
        {
            return type == null_clause_t;
        }

        bool operator==(const PropBy other) const
        {
            return (type == other.type
                    && red_step == other.red_step
                    && data1 == other.data1
                    && data2 == other.data2
                   );
        }

        bool operator!=(const PropBy other) const
        {
            return !(*this == other);
        }
};

inline std::ostream& operator<<(std::ostream& os, const PropBy& pb)
{
    switch (pb.getType()) {
        case binary_t:
            os << " binary, other lit= " << pb.lit2();
            break;

        case clause_t:
            os << " clause, num= " << pb.get_offset();
            break;

        case null_clause_t:
            os << " NULL";
            break;

        case bnn_t:
            os << " BNN reason, bnn idx: " << pb.get_bnn_reason();
            break;

        case xor_t:
            os << " xor reason, matrix= " << pb.get_matrix_num() << " row: " << pb.get_row_num();
            break;

        default:
            assert(false);
            break;
    }
    return os;
}

} //end namespace

#endif //PROPBY_H
