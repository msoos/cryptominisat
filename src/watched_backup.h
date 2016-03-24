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

#ifndef WATCHED_H
#define WATCHED_H

//#define DEBUG_WATCHED

#include "clabstraction.h"
#include "constants.h"
#include "cloffset.h"
#include "solvertypes.h"

#include <limits>

namespace CMSat {

enum WatchType {
    watch_clause_t = 0
    , watch_binary_t = 1
    , watch_tertiary_t = 2
    , watch_idx_t = 3
};

/**
@brief An element in the watchlist. Natively contains 2- and 3-long clauses, others are referenced by pointer

This class contains two 32-bit datapieces. They are either used as:
\li One literal, in the case of binary clauses
\li Two literals, in the case of tertiary clauses
\li One blocking literal (i.e. an example literal from the clause) and a clause
offset (as per ClauseAllocator ), in the case of long clauses
*/
class Watched {
    public:
        /**
        @brief Constructor for a long (>3) clause
        */
        Watched(const ClOffset offset, Lit blockedLit) :
            data1(blockedLit.toInt())
            , data2(offset)
            , type(watch_clause_t)
        {
        }

        /**
        @brief Constructor for a long (>3) clause
        */
        Watched(const ClOffset offset, cl_abst_type abst) :
            data1(abst)
            , data2(offset)
            , type(watch_clause_t)
        {
        }

        Watched(){}

        /**
        @brief Constructor for a binary clause
        */
        Watched(const Lit lit, const bool red) :
            data1(lit.toInt())
            , type(watch_binary_t)
            , _red(red)
        {
        }

        /**
        @brief Constructor for a 3-long clause
        */
        Watched(const Lit lit1, const Lit lit2, const bool red) :
            data1(lit1.toInt())
            , data2(lit2.toInt())
            , type(watch_tertiary_t)
            , _red(red)
        {
        }

        /**
        @brief Constructor for an Index value
        */
        Watched(const uint32_t idx) :
            data1(idx)
            , type(watch_idx_t)
        {
        }

        void setNormOffset(const ClOffset offset)
        {
            #ifdef DEBUG_WATCHED
            assert(type == watch_clause_t);
            #endif
            data2 = offset;
        }

        /**
        @brief To update the blocked literal of a >3-long normal clause
        */
        void setBlockedLit(const Lit blockedLit)
        {
            #ifdef DEBUG_WATCHED
            assert(type == watch_clause_t);
            #endif
            data1 = blockedLit.toInt();
        }

        WatchType getType() const
        {
            // we rely that WatchType enum is in [0-3] range and fits into type field two bits
            return static_cast<WatchType>(type);
        }

        bool isBin() const
        {
            return (type == watch_binary_t);
        }

        bool isClause() const
        {
            return (type == watch_clause_t);
        }

        bool isTri() const
        {
            return (type == watch_tertiary_t);
        }

        bool isIdx() const
        {
            return (type == watch_idx_t);
        }

        uint32_t get_idx() const
        {
            #ifdef DEBUG_WATCHED
            assert(type == watch_idx_t);
            #endif
            return data1;
        }

        /**
        @brief Get the sole other lit of the binary clause, or get lit2 of the tertiary clause
        */
        Lit lit2() const
        {
            #ifdef DEBUG_WATCHED
            assert(isBin() || isTri());
            #endif
            return Lit::toLit(data1);
        }

        /**
        @brief Set the sole other lit of the binary clause
        */
        void setLit2(const Lit lit)
        {
            #ifdef DEBUG_WATCHED
            assert(isBin() || isTri());
            #endif
            data1 = lit.toInt();
        }

        bool red() const
        {
            #ifdef DEBUG_WATCHED
            assert(isBin() || isTri());
            #endif
            return _red;
        }

        void setRed(const bool
        #ifdef DEBUG_WATCHED
        toSet
        #endif
        )
        {
            #ifdef DEBUG_WATCHED
            assert(isBin() || isTri());
            assert(red());
            assert(toSet == false);
            #endif
            _red = true;
        }

        /**
        @brief Get the 3rd literal of a 3-long clause
        */
        Lit lit3() const
        {
            #ifdef DEBUG_WATCHED
            assert(isTri());
            #endif
            return Lit::toLit(data2);
        }

        void setLit3(const Lit lit2)
        {
            #ifdef DEBUG_WATCHED
            assert(isTri());
            #endif
            data2 = lit2.toInt();
        }

        void mark_bin_cl()
        {
            #ifdef DEBUG_WATCHED
            assert(isBin());
            #endif
            marked = 1;
        }

        void unmark_bin_cl()
        {
            #ifdef DEBUG_WATCHED
            assert(isBin());
            #endif
            marked= 0;
        }

        bool bin_cl_marked() const
        {
            #ifdef DEBUG_WATCHED
            assert(isBin());
            #endif
            return marked;
        }

        /**
        @brief Get example literal (blocked lit) of a normal >3-long clause
        */
        Lit getBlockedLit() const
        {
            #ifdef DEBUG_WATCHED
            assert(isClause());
            #endif
            return Lit::toLit(data1);
        }

        cl_abst_type getAbst() const
        {
            #ifdef DEBUG_WATCHED
            assert(isClause());
            #endif
            return data1;
        }

        /**
        @brief Get offset of a >3-long normal clause or of an xor clause (which may be 3-long)
        */
        ClOffset get_offset() const
        {
            #ifdef DEBUG_WATCHED
            assert(isClause());
            #endif
            return data2;
        }

        bool operator==(const Watched& other) const
        {
            return data1 == other.data1 && data2 == other.data2 && type == other.type
                    && _red == other._red;
        }

        bool operator!=(const Watched& other) const
        {
            return !(*this == other);
        }

        Watched(Watched&& o) noexcept {
            data1 = o.data1;
            data2 = o.data2;
            type = o.type;
            _red = o._red;
            marked = o.marked;
        }

        Watched(const Watched& o) noexcept {
            data1 = o.data1;
            data2 = o.data2;
            type = o.type;
            _red = o._red;
            marked = o.marked;
        }

        Watched& operator=(const Watched& o) noexcept {
            data1 = o.data1;
            data2 = o.data2;
            type = o.type;
            _red = o._red;
            marked = o.marked;
            return *this;
        }

    private:
        uint32_t data1;
        // binary, tertiary or long, as per WatchType
        // currently WatchType is enum with range [0..3] and fits in type
        // in case if WatchType extended type size won't be enough.
        uint32_t data2;
        char type;
        char _red;
        char marked;
};

inline std::ostream& operator<<(std::ostream& os, const Watched& ws)
{

    if (ws.isClause()) {
        os << "Clause offset " << ws.get_offset();
    }

    if (ws.isBin()) {
        os << "Bin lit " << ws.lit2() << " (red: " << ws.red() << " )";
    }

    if (ws.isTri()) {
        os << "Tri lits "
        << ws.lit2() << ", " << ws.lit3()
        << " (red: " << ws.red() << " )";
    }

    return os;
}

struct OccurClause {
    OccurClause(const Lit _lit, const Watched _ws) :
        lit(_lit)
        , ws(_ws)
    {}

    OccurClause() :
        lit(lit_Undef)
    {}

    bool operator==(const OccurClause& other) const
    {
        return lit == other.lit && ws == other.ws;
    }

    Lit lit;
    Watched ws;
};

struct WatchSorterBinTriLong {
        bool operator()(const Watched& a, const Watched& b)
        {
            assert(!a.isIdx());
            assert(!b.isIdx());

            //Anything but clause!
            if (a.isClause()) {
                //A is definitely not better than B
                return false;
            }
            if (b.isClause()) {
                //B is clause, A is NOT a clause. So A is better than B.
                return true;
            }
            //Now nothing is clause

            if (a.lit2() != b.lit2()) {
                return a.lit2() < b.lit2();
            }
            if (a.isBin() && b.isTri()) return true;
            if (a.isTri() && b.isBin()) return false;
            //At this point either both are BIN or both are TRI

            //Both are BIN
            if (a.isBin()) {
                assert(b.isBin());
                if (a.red() != b.red()) {
                    return !a.red();
                }
                return false;
            }

            //Both are Tri
            assert(a.isTri() && b.isTri());
            if (a.lit3() != b.lit3()) {
                return a.lit3() < b.lit3();
            }
            if (a.red() != b.red()) {
                return !a.red();
            }
            return false;
        }
    };


} //end namespace

#endif //WATCHED_H
