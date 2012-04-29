/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2011, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
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

#include "constants.h"
#include "ClauseOffset.h"
#include "SolverTypes.h"
#include <limits>

enum WatchType {
    watch_clause_t = 0
    , watch_binary_t = 1
    , watch_tertiary_t = 2
};

/**
@brief An element in the watchlist. Natively contains 2- and 3-long clauses, others are referenced by pointer

This class contains two 32-bit datapieces. They are either used as:
\li One literal, in the case of binary clauses
\li Two literals, in the case of tertiary clauses
\li One blocking literal (i.e. an example literal from the clause) and a clause
offset (as per ClauseAllocator ), in the case of normal clauses
*/
class Watched {
    public:
        /**
        @brief Constructor for a long (>3) clause
        */
        Watched(const ClauseOffset offset, Lit blockedLit) :
            data1(blockedLit.toInt())
            , type(watch_clause_t)
            , data2(offset)
        {
        }

        Watched() :
            data1 (std::numeric_limits<uint32_t>::max())
            , data2(std::numeric_limits<uint32_t>::max())
        {}

        /**
        @brief Constructor for a binary clause
        */
        Watched(const Lit lit, const bool learnt) :
            data1(lit.toInt())
            , type(watch_binary_t)
            , data2(learnt)
        {
        }

        /**
        @brief Constructor for a 3-long clause
        */
        Watched(const Lit lit1, const Lit lit2) :
            data1(lit1.toInt())
            , type(watch_tertiary_t)
            , data2(lit2.toInt())
        {
        }

        void setNormOffset(const ClauseOffset offset)
        {
            #ifdef DEBUG_WATCHED
            assert(type == watch_clause_t);
            #endif
            data2 = offset;
        }

        /**
        @brief To update the example literal (blocked literal) of a >3-long normal clause
        */
        void setBlockedLit(const Lit blockedLit)
        {
            #ifdef DEBUG_WATCHED
            assert(type == watch_clause_t);
            #endif
            data1 = blockedLit.toInt();
        }

        bool isBinary() const
        {
            return (type == watch_binary_t);
        }

        bool isNonLearntBinary() const
        {
            return (type == watch_binary_t && !data2);
        }

        bool isClause() const
        {
            return (type == watch_clause_t);
        }

        bool isTriClause() const
        {
            return (type == watch_tertiary_t);
        }

        /**
        @brief Get the sole other lit of the binary clause, or get lit2 of the tertiary clause
        */
        Lit getOtherLit() const
        {
            #ifdef DEBUG_WATCHED
            assert(isBinary() || isTriClause());
            #endif
            return Lit::toLit(data1);
        }

        /**
        @brief Set the sole other lit of the binary clause
        */
        void setOtherLit(const Lit lit)
        {
            #ifdef DEBUG_WATCHED
            assert(isBinary() || isTriClause());
            #endif
            data1 = lit.toInt();
        }

        bool getLearnt() const
        {
            #ifdef DEBUG_WATCHED
            assert(isBinary());
            #endif
            return data2;
        }

        void setLearnt(const bool learnt)
        {
            #ifdef DEBUG_WATCHED
            assert(isBinary());
            assert(learnt == false);
            #endif
            data2 = learnt;
        }

        /**
        @brief Get the 3rd literal of a 3-long clause
        */
        Lit getOtherLit2() const
        {
            #ifdef DEBUG_WATCHED
            assert(isTriClause());
            #endif
            return Lit::toLit(data2);
        }

        void setOtherLit2(const Lit lit2)
        {
            #ifdef DEBUG_WATCHED
            assert(isTriClause());
            #endif
            data2 = lit2.toInt();
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

        /**
        @brief Get offset of a >3-long normal clause or of an xor clause (which may be 3-long)
        */
        ClauseOffset getNormOffset() const
        {
            #ifdef DEBUG_WATCHED
            assert(isClause());
            #endif
            return data2;
        }

    private:
        uint32_t data1;
        //binary, tertiary or long, as per WatchType
        uint32_t type:2;
        uint32_t data2:30;
};

/**
@brief Orders the watchlists such that the order is binary, tertiary, normal, xor
*/
struct WatchedSorter
{
    bool operator () (const Watched& x, const Watched& y);
};

inline bool  WatchedSorter::operator () (const Watched& x, const Watched& y)
{
    if (y.isBinary()) return false;
    //y is not binary, but x is, so x must be first
    if (x.isBinary()) return true;

    //from now on, none is binary.
    if (y.isTriClause()) return false;
    if (x.isTriClause()) return true;

    //from now on, none is binary or tertiary
    //don't bother sorting these
    return false;
}

#endif //WATCHED_H
