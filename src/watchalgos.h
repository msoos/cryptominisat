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

#ifndef __WATCHALGOS_H__
#define __WATCHALGOS_H__

#include "watched.h"
#include "watcharray.h"
#include "clauseallocator.h"

namespace CMSat {
using namespace CMSat;

//////////////////
// NORMAL Clause
//////////////////
static inline bool findWCl(watch_subarray_const ws, const ClOffset c)
{
    const Watched* i = ws.begin(), *end = ws.end();
    for (; i != end && (!i->isClause() || i->get_offset() != c); i++);
    return i != end;
}

static inline void removeWCl(watch_subarray ws, const ClOffset c)
{
    Watched* i = ws.begin(), *end = ws.end();
    for (; i != end && (!i->isClause() || i->get_offset() != c); i++);
    assert(i != end);
    Watched* j = i;
    i++;
    for (; i != end; j++, i++) *j = *i;
    ws.shrink_(1);
}

//////////////////
// BINARY Clause
//////////////////

inline void removeWBin(
    watch_array &wsFull
    , const Lit lit1
    , const Lit lit2
    , const bool red
) {
    watch_subarray ws = wsFull[lit1];
    Watched *i = ws.begin(), *end = ws.end();
    for (; i != end && (
        !i->isBin()
        || i->lit2() != lit2
        || i->red() != red
    ); i++);

    assert(i != end);
    Watched *j = i;
    i++;
    for (; i != end; j++, i++) *j = *i;
    ws.shrink_(1);
}

inline bool removeWBin_except_marked(
    watch_array &wsFull
    , const Lit lit1
    , const Lit lit2
    , const bool red
) {
    watch_subarray ws = wsFull[lit1];
    Watched *i = ws.begin(), *end = ws.end();
    for (; i != end && (
        !i->isBin()
        || i->lit2() != lit2
        || i->red() != red
    ); i++);
    assert(i != end);

    if (i->bin_cl_marked()) {
        return false;
    }

    Watched *j = i;
    i++;
    for (; i != end; j++, i++) *j = *i;
    ws.shrink_(1);

    return true;
}

inline const Watched& findWatchedOfBin(
    const watch_array& wsFull
    , const Lit lit1
    , const Lit lit2
    , const bool red
) {
    watch_subarray_const ws = wsFull[lit1];
    for (const Watched *i = ws.begin(), *end = ws.end(); i != end; i++) {
        if (i->isBin() && i->lit2() == lit2 && i->red() == red)
            return *i;
    }

    assert(false);
    return *ws.begin();
}

inline Watched& findWatchedOfBin(
    watch_array& wsFull
    , const Lit lit1
    , const Lit lit2
    , const bool red
) {
    watch_subarray ws = wsFull[lit1];
    for (Watched *i = ws.begin(), *end = ws.end(); i != end; i++) {
        if (i->isBin() && i->lit2() == lit2 && i->red() == red)
            return *i;
    }

    assert(false);
    return *ws.begin();
}

static inline void removeWXCl(watch_array& wsFull
    , const Lit lit
    , const ClOffset offs
) {
    watch_subarray ws = wsFull[lit];
    Watched *i = ws.begin(), *end = ws.end();
    for (; i != end && (!i->isClause() || i->get_offset() != offs); i++);
    assert(i != end);
    Watched *j = i;
    i++;
    for (; i != end; j++, i++) *j = *i;
    ws.shrink_(1);
}

} //end namespace


#endif //__WATCHALGOS_H__
