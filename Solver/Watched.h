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

#include "ClauseAllocator.h"

//#define DEBUG_WATCHED

class Watched {
    public:
        Watched(const ClauseOffset offset, Lit blockedLit) //for normal clause
        {
            data1 = (uint32_t)1 + (blockedLit.toInt() << 2);
            data2 = (uint32_t)offset;
        }

        Watched(const ClauseOffset offset) //for xor-clause
        {
            data1 = (uint32_t)2;
            data2 = (uint32_t)offset;
        }

        Watched(const Lit lit) //for binary clause
        {
            data1 = (uint32_t)0 + (lit.toInt() << 2);
        }

        Watched(const Lit lit1, const Lit lit2) //for binary clause
        {
            data1 = (uint32_t)3 + (lit1.toInt() << 2);
            data2 = lit2.toInt();
        }

        void setOffset(const ClauseOffset offset)
        {
            data2 = (uint32_t)offset;
        }

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

        const Lit getOtherLit() const
        {
            #ifdef DEBUG_WATCHED
            assert(isBinary() || isTriClause());
            #endif
            return data1AsLit();
        }

        const Lit getOtherLit2() const
        {
            #ifdef DEBUG_WATCHED
            assert(isTriClause());
            #endif
            return data2AsLit();
        }

        const Lit getBlockedLit() const
        {
            #ifdef DEBUG_WATCHED
            assert(isClause());
            #endif
            return data1AsLit();
        }

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

        uint32_t data1; //blocked lit
        uint32_t data2; //offset (if normal/xor Clause)
};

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

class WatchedBin {
    public:
        WatchedBin(Lit _impliedLit) : impliedLit(_impliedLit) {};
        Lit impliedLit;
};

#endif //WATCHED_H
