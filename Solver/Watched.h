/***************************************************************************
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
*****************************************************************************/

#ifndef WATCHED_H
#define WATCHED_H

#ifdef _MSC_VER
#include <msvc/stdint.h>
#else
#include <stdint.h>
#endif //_MSC_VER

//#define DEBUG_WATCHED

#include "ClauseOffset.h"
#include "SolverTypes.h"

/**
@brief An element in the watchlist. Natively contains 2- and 3-long clauses, others are referenced by pointer

This class contains two 32-bit datapieces. They are either used as:
\li One literal, in the case of binary clauses
\li Two literals, in the case of tertiary clauses
\li One blocking literal (i.e. an example literal from the clause) and a clause
offset (as per ClauseAllocator ), in the case of normal clauses
\li A clause offset (as per ClauseAllocator) for xor clauses
*/
class Watched {
    public:
        /**
        @brief Constructor for a >3-long normal clause
        */
        Watched(const ClauseOffset offset, Lit blockedLit)
        {
            data1 = (uint32_t)1 + (blockedLit.toInt() << 2);
            data2 = (uint32_t)offset;
        }

        /**
        @brief Constructor for an xor-clause
        */
        Watched(const ClauseOffset offset)
        {
            data1 = (uint32_t)2;
            data2 = (uint32_t)offset;
        }

        /**
        @brief Constructor for a binary clause
        */
        Watched(const Lit lit)
        {
            data1 = (uint32_t)0 + (lit.toInt() << 2);
        }

        /**
        @brief Constructor for a 3-long, non-xor clause
        */
        Watched(const Lit lit1, const Lit lit2)
        {
            data1 = (uint32_t)3 + (lit1.toInt() << 2);
            data2 = lit2.toInt();
        }

        void setOffset(const ClauseOffset offset)
        {
            data2 = (uint32_t)offset;
        }

        /**
        @brief To update the example literal (blocked literal) of a >3-long normal clause
        */
        void setBlockedLit(const Lit lit)
        {
            #ifdef DEBUG_WATCHED
            assert(isClause());
            #endif
            data1 = (uint32_t)1 + (lit.toInt() << 2);
        }

        void setClause()
        {
            data1 = 1;
        }

        const bool isBinary() const
        {
            return ((data1&3) == 0);
        }

        const bool isClause() const
        {
            return ((data1&3) == 1);
        }

        const bool isXorClause() const
        {
            return ((data1&3) == 2);
        }

        const bool isTriClause() const
        {
            return ((data1&3) == 3);
        }

        /**
        @brief Get the sole other lit of the binary clause, or get lit2 of the tertiary clause
        */
        const Lit getOtherLit() const
        {
            #ifdef DEBUG_WATCHED
            assert(isBinary() || isTriClause());
            #endif
            return data1AsLit();
        }

        /**
        @brief Get the 3rd literal of a 3-long clause
        */
        const Lit getOtherLit2() const
        {
            #ifdef DEBUG_WATCHED
            assert(isTriClause());
            #endif
            return data2AsLit();
        }

        /**
        @brief Get example literal (blocked lit) of a normal >3-long clause
        */
        const Lit getBlockedLit() const
        {
            #ifdef DEBUG_WATCHED
            assert(isClause());
            #endif
            return data1AsLit();
        }

        /**
        @brief Get offset of a >3-long normal clause or of an xor clause (which may be 3-long)
        */
        const ClauseOffset getOffset() const
        {
            #ifdef DEBUG_WATCHED
            assert(isClause() || isXorClause());
            #endif
            return (ClauseOffset)(data2);
        }

    private:
        const Lit data1AsLit() const
        {
            return (Lit::toLit(data1>>2));
        }

        const Lit data2AsLit() const
        {
            return (Lit::toLit(data2));
        }

        uint32_t data1; ///<Either the other lit (for bin clauses) or the blocked lit it stored here
        uint32_t data2; ///<Either the offset (for >3-long normal, or xor clauses) or the 3rd literal (for 3-long clauses) is stored here
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
