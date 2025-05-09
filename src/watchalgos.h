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

#pragma once

#include "watched.h"
#include "watcharray.h"
#include "gausswatched.h"
#include "clauseallocator.h"

namespace CMSat {

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
    , const int32_t ID
) {
    watch_subarray ws = wsFull[lit1];
    Watched *i = ws.begin(), *end = ws.end();
    for (; i != end && (
        !i->isBin()
        || i->lit2() != lit2
        || i->red() != red
        || i->get_id() != ID
    ); i++);

    assert(i != end);
    Watched *j = i;
    i++;
    for (; i != end; j++, i++) *j = *i;
    ws.shrink_(1);
}

inline void removeWBin_change_order(
    watch_array &wsFull
    , const Lit lit1
    , const Lit lit2
    , const bool red
    , const int32_t ID
) {
    watch_subarray ws = wsFull[lit1];
    Watched *i = ws.begin(), *end = ws.end();
    for (; i != end && (
        !i->isBin()
        || i->lit2() != lit2
        || i->red() != red
        || i->get_id() != ID
    ); i++);

    assert(i != end);
    *i = ws[ws.size()-1];
    ws.shrink_(1);
}

inline bool removeWBin_except_marked(
    watch_array &wsFull
    , const Lit lit1
    , const Lit lit2
    , const bool red
    , const int32_t ID
) {
    watch_subarray ws = wsFull[lit1];
    Watched *i = ws.begin(), *end = ws.end();
    for (; i != end && (
        !i->isBin()
        || i->lit2() != lit2
        || i->red() != red
        || i->get_id() != ID
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
    , const int32_t ID
) {
    watch_subarray_const ws = wsFull[lit1];
    for (const Watched *i = ws.begin(), *end = ws.end(); i != end; i++) {
        if (i->isBin() && i->lit2() == lit2 && i->red() == red && i->get_id() == ID)
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
    , const int32_t ID
) {
    watch_subarray ws = wsFull[lit1];
    for (Watched *i = ws.begin(), *end = ws.end(); i != end; i++) {
        if (i->isBin() && i->lit2() == lit2 && i->red() == red && i->get_id() == ID)
            return *i;
    }

    assert(false);
    return *ws.begin();
}

inline Watched* findWatchedOfBinMaybe(
    watch_array& wsFull
    , const Lit lit1
    , const Lit lit2
    , const bool red
    , const int32_t ID
) {
    watch_subarray ws = wsFull[lit1];
    for (Watched *i = ws.begin(), *end = ws.end(); i != end; i++) {
        if (i->isBin() && i->lit2() == lit2 && i->red() == red && i->get_id() == ID)
            return i;
    }
    return nullptr;
}

static inline bool findWXCl(const vec<GaussWatched>& gws, const uint32_t at) {
    for(const auto& gw: gws) if (gw.matrix_num == 1000 && gw.row_n == at) return true;
    return false;
}

static inline void removeWXCl(vec<vec<GaussWatched>>& wsFull
    , const uint32_t var
    , const uint32_t at
) {
    auto& gws = wsFull[var];
    auto i = gws.begin(), end = gws.end();
    for (; i != end && !(i->matrix_num == 1000 && i->row_n == at); i++);
    assert(i != end);
    auto j = i;
    i++;
    for (; i != end; j++, i++) *j = *i;
    gws.shrink_(1);
}


// Removes BNN *once*
static inline void removeWBNN(watch_array& wsFull
    , const Lit lit
    , const uint32_t bnnIdx
) {
    watch_subarray ws = wsFull[lit];
    Watched *i = ws.begin(), *end = ws.end();
    for (; i != end && (!i->isBNN() || i->get_bnn() != bnnIdx); i++);
    assert(i != end);
    Watched *j = i;
    i++;
    for (; i != end; j++, i++) *j = *i;
    ws.shrink_(1);
}


} //end namespace
