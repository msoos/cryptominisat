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

#ifndef WATCHED_H
#define WATCHED_H

//#define DEBUG_WATCHED

#include "clabstraction.h"
#include "constants.h"
#include "cloffset.h"
#include "solvertypes.h"

#include <limits>
#include <string.h>


namespace CMSat {

enum class WatchType {
    watch_clause_t = 0
    , watch_binary_t = 1
    , watch_bnn_t = 2
    , watch_idx_t = 3
};

enum BNNPropType {
    bnn_pos_t = 0
    , bnn_neg_t = 1
    , bnn_out_t = 2
};

class Watched {
    public:
        Watched(Watched const&) = default;

        /**
        @brief Constructor for a long (>2) clause
        */
        Watched(const ClOffset offset, Lit blockedLit) :
            data1(blockedLit.toInt())
            , type(static_cast<int>(WatchType::watch_clause_t))
            , data2(offset)
        {
        }

        /**
        @brief Constructor for a long (>2) clause
        */
        Watched(const ClOffset offset, cl_abst_type abst) :
            data1(abst)
            , type(static_cast<int>(WatchType::watch_clause_t))
            , data2(offset)
        {
        }

        Watched(const uint32_t idx, WatchType t):
            data1(idx)
            , type(static_cast<int>(t))
        {
            assert(t == WatchType::watch_idx_t);
        }

        Watched(const uint32_t idx, WatchType t, BNNPropType bnn_p_t):
            data1(idx)
            , type(static_cast<int>(t))
            , data2(bnn_p_t)
        {
            DEBUG_WATCHED_DO(assert(t == watch_bnn_t));
        }

        Watched() :
            data1 (numeric_limits<uint32_t>::max())
            , type(static_cast<int>(WatchType::watch_clause_t)) // initialize type with most generic type of clause
            , data2(numeric_limits<uint32_t>::max() >> 2)
        {}

        /**
        @brief Constructor for a binary clause
        */
        Watched(const Lit lit, const bool red, int32_t ID) :
            data1(lit.toInt())
            , type(static_cast<int>(WatchType::watch_binary_t))
            , data2((int32_t)red | ID<<2) //marking is 2nd bit
        {
            assert(ID < 1LL<< (EFFECTIVELY_USEABLE_BITS-2));
        }

        /**
        @brief To update the blocked literal of a >3-long normal clause
        */
        void setBlockedLit(const Lit blockedLit)
        {
            DEBUG_WATCHED_DO(assert(type == watch_clause_t));
            data1 = blockedLit.toInt();
        }

        WatchType getType() const
        {
            // we rely that WatchType enum is in [0-3] range and fits into type field two bits
            return static_cast<WatchType>(type);
        }

        bool isBin() const
        {
            return (type == static_cast<int>(WatchType::watch_binary_t));
        }

        bool isClause() const
        {
            return (type == static_cast<int>(WatchType::watch_clause_t));
        }

        bool isIdx() const
        {
            return (type == static_cast<int>(WatchType::watch_idx_t));
        }

        bool isBNN() const
        {
            return (type == static_cast<int>(WatchType::watch_bnn_t));
        }

        uint32_t get_idx() const
        {
            DEBUG_WATCHED_DO(assert(type == static_cast<int>(WatchType::watch_idx_t)));
            return data1;
        }

        uint32_t get_bnn() const
        {
            DEBUG_WATCHED_DO(assert(type == static_cast<int>(WatchType::watch_bnn_t)));
            return data1;
        }

        BNNPropType get_bnn_prop_t() const
        {
            DEBUG_WATCHED_DO(assert(type == static_cast<int>(WatchType::watch_bnn_t)));
            return (BNNPropType)data2;
        }

        /**
        @brief Get the sole other lit of the binary clause, or get lit2 of the tertiary clause
        */
        Lit lit2() const
        {
            DEBUG_WATCHED_DO(assert(isBin()));
            return Lit::toLit(data1);
        }

        /**
        @brief Set the sole other lit of the binary clause
        */
        void setLit2(const Lit lit)
        {
            DEBUG_WATCHED_DO(assert(isBin()));
            data1 = lit.toInt();
        }

        bool red() const
        {
            DEBUG_WATCHED_DO(assert(isBin()));
            return data2 & 1;
        }

        int32_t get_ID() const
        {
            DEBUG_WATCHED_DO(assert(isBin()));
            return data2 >> 2;
        }

        void setRed(const bool toSet)
        {
            DEBUG_WATCHED_DO(assert(isBin()));
            DEBUG_WATCHED_DO(assert(red()));
            assert(toSet == false);
            data2 &= (~(1U));
        }

        void mark_bin_cl()
        {
            DEBUG_WATCHED_DO(assert(isBin()));
            data2 |= 2;
        }

        void unmark_bin_cl()
        {
            DEBUG_WATCHED_DO(assert(isBin()));
            data2 &= (~(2ULL));
        }

        bool bin_cl_marked() const
        {
            DEBUG_WATCHED_DO(assert(isBin()));
            return data2&2;
        }

        /**
        @brief Get example literal (blocked lit) of a normal >3-long clause
        */
        Lit getBlockedLit() const
        {
            DEBUG_WATCHED_DO(assert(isClause()));
            return Lit::toLit(data1);
        }

        cl_abst_type getAbst() const
        {
            DEBUG_WATCHED_DO(assert(isClause()));
            return data1;
        }

        /**
        @brief Get offset of a >3-long normal clause or of an xor clause (which may be 3-long)
        */
        ClOffset get_offset() const
        {
            DEBUG_WATCHED_DO(assert(isClause()));
            return data2;
        }

        bool operator==(const Watched& other) const
        {
            return data1 == other.data1 && data2 == other.data2 && type == other.type;
        }

        bool operator!=(const Watched& other) const
        {
            return !(*this == other);
        }

    private:
        uint32_t data1;
        ClOffset type:2;
        ClOffset data2:EFFECTIVELY_USEABLE_BITS;
};

inline std::ostream& operator<<(std::ostream& os, const Watched& ws)
{

    if (ws.isClause()) {
        os << "Clause offset " << ws.get_offset();
    }

    if (ws.isBin()) {
        os << "Bin lit " << ws.lit2() << " (red: " << ws.red() << " )";
    }

    return os;
}

struct OccurClause {
    OccurClause(const Lit& _lit, const Watched& _ws) :
        lit(_lit)
        , ws(_ws)
    {}

    OccurClause() :
        lit(lit_Undef)
    {}

    Lit lit;
    Watched ws;

    // will be equal even if one is removing a literal, and the other is subsuming the whole clause
    bool operator==(const OccurClause& other) const {
        if (ws.getType() != other.ws.getType()) return false;
        if (ws.isBin()) return ws.get_ID() == other.ws.get_ID();
        if (ws.isBNN()) return ws.get_bnn() == other.ws.get_bnn();
        if (ws.isClause()) return ws.get_offset() == other.ws.get_offset();
        release_assert(false);
        return false;
    }

    bool operator<(const OccurClause& other) const {
        if (ws.isBin() && !other.ws.isBin()) {
            return true;
        }
        if (!ws.isBin() && other.ws.isBin()) {
            return false;
        }

        if (ws.isBin()) {
            assert(other.ws.isBin());
            return ws.get_ID() < other.ws.get_ID();
        }

        assert(!ws.isBNN()); // no idea how this would work
        assert(!other.ws.isBNN()); // no idea how this would work
        return ws.get_offset() < other.ws.get_offset();
    }
};

struct WatchSorterBinTriLong {
        bool operator()(const Watched& a, const Watched& b)
        {
            assert(!a.isIdx());
            assert(!b.isIdx());

            //Anything but clause!
            if (a.isClause() || a.isBNN()) {
                //A is definitely not better than B
                return false;
            }
            if (b.isClause() || b.isBNN()) {
                //B is clause, A is NOT a clause. So A is better than B.
                return true;
            }

            //Both are BIN
            assert(a.isBin());
            assert(b.isBin());

            if (a.lit2() != b.lit2()) {
                return a.lit2() < b.lit2();
            }

            if (a.red() != b.red()) {
                return !a.red();
            }

            return (a.get_ID() < b.get_ID());
        }
    };


} //end namespace

#endif //WATCHED_H
