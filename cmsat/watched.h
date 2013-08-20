/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos and collaborators. All rights reserved.
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

#include "clabstraction.h"
#include "constants.h"
#include "cloffset.h"
#include "solvertypes.h"
#include "vec.h"

#include <limits>

namespace CMSat {

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
        Watched(const ClOffset offset, Lit blockedLit) :
            data1(blockedLit.toInt())
            , type(watch_clause_t)
            , data2(offset)
        {
        }

        /**
        @brief Constructor for a long (>3) clause
        */
        Watched(const ClOffset offset, CL_ABST_TYPE abst) :
            data1(abst)
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
        Watched(const Lit lit, const bool red) :
            data1(lit.toInt())
            , type(watch_binary_t)
            , data2(red)
        {
        }

        /**
        @brief Constructor for a 3-long clause
        */
        Watched(const Lit lit1, const Lit lit2, const bool red) :
            data1(lit1.toInt())
            , type(watch_tertiary_t)
            , data2((lit2.toInt() << 1) | (uint32_t)red)
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
            if (isBinary())
                return watch_binary_t;
            else if (isTri())
                return watch_tertiary_t;
            else
                return watch_clause_t;
        }

        bool isBinary() const
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

        /**
        @brief Get the sole other lit of the binary clause, or get lit2 of the tertiary clause
        */
        Lit lit2() const
        {
            #ifdef DEBUG_WATCHED
            assert(isBinary() || isTri());
            #endif
            return Lit::toLit(data1);
        }

        /**
        @brief Set the sole other lit of the binary clause
        */
        void setLit1(const Lit lit)
        {
            #ifdef DEBUG_WATCHED
            assert(isBinary() || isTri());
            #endif
            data1 = lit.toInt();
        }

        bool red() const
        {
            #ifdef DEBUG_WATCHED
            assert(isBinary() || isTri());
            #endif
            return data2 & 1;
        }

        void setRed(const bool toSet)
        {
            #ifdef DEBUG_WATCHED
            assert(isBinary() || isTri());
            assert(toSet == false);
            assert(red());
            #endif
            if (toSet) {
                data2 |= 1U;
            } else {
                data2 &= (~(1U));
            }
        }

        /**
        @brief Get the 3rd literal of a 3-long clause
        */
        Lit lit3() const
        {
            #ifdef DEBUG_WATCHED
            assert(isTri());
            #endif
            return Lit::toLit(data2>>1);
        }

        void setLit2(const Lit lit2)
        {
            #ifdef DEBUG_WATCHED
            assert(isTri());
            #endif
            data2 = (lit2.toInt()<<1) | (data2&1);
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

        CL_ABST_TYPE getAbst() const
        {
            #ifdef DEBUG_WATCHED
            assert(isClause());
            #endif
            return data1;
        }

        /**
        @brief Get offset of a >3-long normal clause or of an xor clause (which may be 3-long)
        */
        ClOffset getOffset() const
        {
            #ifdef DEBUG_WATCHED
            assert(isClause());
            #endif
            return data2;
        }

        class Iterator
        {
        public:
            //end iterator
            Iterator() :
                at(2)
            {}

            //Begin iterator
            Iterator(Lit lit2, Lit lit3) :
                at(0)
            {
                lits[0] = lit2;
                lits[1] = lit3;
            }

            Lit operator*()
            {
                return lits[at];
            }

            void operator++()
            {
                at++;
            }

            bool operator==(const Iterator& other) const
            {
                return (at == other.at);
            }

            bool operator!=(const Iterator& other) const
            {
                return (at != other.at);
            }

        private:
            unsigned at;
            Lit lits[2];
        };

        Iterator begin() const
        {
            assert(isTri());
            return Iterator(lit2(), lit3());
        }

        Iterator end() const
        {
            assert(isTri());
            return Iterator();
        }

    private:
        uint32_t data1;
        //binary, tertiary or long, as per WatchType
        uint32_t type:2;
        uint32_t data2:30;
};

inline std::ostream& operator<<(std::ostream& os, const Watched& ws)
{

    if (ws.isClause()) {
        os << "Clause, offset: " << ws.getOffset();
    }

    if (ws.isBinary()) {
        os << "Bin: " << ws.lit2() << " (red: " << ws.red() << " )";
    }

    if (ws.isTri()) {
        os << "Tri: "
        << ws.lit2() << ", " << ws.lit3()
        << " (red: " << ws.red() << " )";
    }

    return os;
}

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
    if (y.isTri()) return false;
    if (x.isTri()) return true;

    //from now on, none is binary or tertiary
    //don't bother sorting these
    return false;
}

//////////////////
// NORMAL Clause
//////////////////
static inline bool findWCl(const vec<Watched>& ws, const ClOffset c)
{
    vec<Watched>::const_iterator i = ws.begin(), end = ws.end();
    for (; i != end && (!i->isClause() || i->getOffset() != c); i++);
    return i != end;
}

static inline void removeWCl(vec<Watched> &ws, const ClOffset c)
{
    vec<Watched>::iterator i = ws.begin(), end = ws.end();
    for (; i != end && (!i->isClause() || i->getOffset() != c); i++);
    assert(i != end);
    vec<Watched>::iterator j = i;
    i++;
    for (; i != end; j++, i++) *j = *i;
    ws.shrink_(1);
}

//////////////////
// TRI Clause
//////////////////

static inline Watched& findWatchedOfTri(
    vector<vec<Watched> >& wsFull
    , const Lit lit1
    , const Lit lit2
    , const Lit lit3
    , const bool red
) {
    vec<Watched>& ws = wsFull[lit1.toInt()];
    for (vec<Watched>::iterator i = ws.begin(), end = ws.end(); i != end; i++) {
        if (i->isTri()
            && i->lit2() == lit2
            && i->lit3() == lit3
            && i->red() == red
        ) {
            return *i;
        }
    }

    assert(false);
    return *ws.begin();
}

static inline const Watched& findWatchedOfTri(
    const vector<vec<Watched> >& wsFull
    , const Lit lit1
    , const Lit lit2
    , const Lit lit3
    , const bool red
) {
    const vec<Watched>& ws = wsFull[lit1.toInt()];
    for (vec<Watched>::const_iterator
        it = ws.begin(), end = ws.end()
        ; it != end
        ; it++
    ) {
        if (it->isTri()
            && it->lit2() == lit2
            && it->lit3() == lit3
            && it->red() == red
        ) {
            return *it;
        }
    }

    assert(false);
    return *ws.begin();
}

static inline void removeWTri(
    vector<vec<Watched> >& wsFull
    , const Lit lit1
    , const Lit lit2
    , const Lit lit3
    , const bool red
) {
    assert(lit2 < lit3);

    vec<Watched>& ws = wsFull[lit1.toInt()];
    vec<Watched>::iterator i = ws.begin(), end = ws.end();
    for (; i != end && (
        !i->isTri()
        || i->lit2() != lit2
        || i->lit3() != lit3
        || i->red() != red
    ); i++);

    assert(i != end);
    vec<Watched>::iterator j = i;
    i++;
    for (; i != end; j++, i++) *j = *i;
    ws.shrink_(1);
}

inline void removeTriAllButOne(
    vector<vec<Watched> >& wsFull
    , const Lit lit
    , const Lit* lits
    , const bool red
) {
    if (lit != lits[0])
        removeWTri(wsFull, lits[0], lits[1], lits[2], red);
    if (lit != lits[1])
        removeWTri(wsFull, lits[1], lits[0], lits[2], red);
    if (lit != lits[2])
        removeWTri(wsFull, lits[2], lits[0], lits[1], red);
}

//////////////////
// BINARY Clause
//////////////////

inline bool findWBin(
    const vector<vec<Watched> >& wsFull
    , const Lit lit1
    , const Lit lit2
) {
    vec<Watched>::const_iterator i = wsFull[lit1.toInt()].begin();
    vec<Watched>::const_iterator end = wsFull[lit1.toInt()].end();
    for (; i != end && (!i->isBinary() || i->lit2() != lit2); i++);
    return i != end;
}

inline bool findWBin(
    const vector<vec<Watched> >& wsFull
    , const Lit lit1
    , const Lit lit2
    , const bool red
) {
    vec<Watched>::const_iterator i = wsFull[lit1.toInt()].begin();
    vec<Watched>::const_iterator end = wsFull[lit1.toInt()].end();
    for (; i != end && (
        !i->isBinary()
        || i->lit2() != lit2
        || i->red() != red
    ); i++);

    return i != end;
}

inline void removeWBin(
    vector<vec<Watched> > &wsFull
    , const Lit lit1
    , const Lit lit2
    , const bool red
) {
    vec<Watched>& ws = wsFull[lit1.toInt()];
    vec<Watched>::iterator i = ws.begin(), end = ws.end();
    for (; i != end && (
        !i->isBinary()
        || i->lit2() != lit2
        || i->red() != red
    ); i++);

    assert(i != end);
    vec<Watched>::iterator j = i;
    i++;
    for (; i != end; j++, i++) *j = *i;
    ws.shrink_(1);
}

inline Watched& findWatchedOfBin(
    vector<vec<Watched> >& wsFull
    , const Lit lit1
    , const Lit lit2
    , const bool red
) {
    vec<Watched>& ws = wsFull[lit1.toInt()];
    for (vec<Watched>::iterator i = ws.begin(), end = ws.end(); i != end; i++) {
        if (i->isBinary() && i->lit2() == lit2 && i->red() == red)
            return *i;
    }

    assert(false);
    return *ws.begin();
}

} //end namespace

#endif //WATCHED_H
