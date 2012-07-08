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

#ifndef Alg_h
#define Alg_h

#include <iostream>
#include "Vec.h"
#include "cmsat/SolverTypes.h"
#include "cmsat/Watched.h"
#include "cmsat/constants.h"

template<class V, class T>
static inline void remove(V& ts, const T& t)
{
    uint32_t j = 0;
    for (; j < ts.size() && ts[j] != t; j++);
    assert(j < ts.size());
    for (; j < (uint16_t)(ts.size()-1); j++) ts[j] = ts[j+1];
    ts.resize(ts.size() -1);
}

template<class V, class T>
static inline void removeW(V& ts, const T& t)
{
    uint32_t j = 0;
    for (; j < ts.size() && ts[j] != t; j++);
    assert(j < ts.size());
    for (; j < ts.size()-1; j++) ts[j] = ts[j+1];
    ts.pop_back();
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

static inline bool findWTri(
    vector<vec<Watched> >& wsFull
    , const Lit lit1
    , const Lit lit2
    , const Lit lit3
    , const bool learnt
) {
    vec<Watched>& ws = wsFull[lit1.toInt()];
    vec<Watched>::const_iterator i = ws.begin(), end = ws.end();
    for (; i != end && (
        !i->isTri()
        || i->lit1() != lit2
        || i->lit2() != lit3
        || i->learnt() != learnt
    ); i++);
    return i != end;
}

static inline void removeWTri(
    vector<vec<Watched> >& wsFull
    , const Lit lit1
    , const Lit lit2
    , const Lit lit3
    , const bool learnt
) {
    assert(lit2 < lit3);

    vec<Watched>& ws = wsFull[lit1.toInt()];
    vec<Watched>::iterator i = ws.begin(), end = ws.end();
    for (; i != end && (
        !i->isTri()
        || i->lit1() != lit2
        || i->lit2() != lit3
        || i->learnt() != learnt
    ); i++);

    assert(i != end);
    vec<Watched>::iterator j = i;
    i++;
    for (; i != end; j++, i++) *j = *i;
    ws.shrink_(1);
}

//////////////////
// BINARY Clause
//////////////////
static inline bool findWBin(
    const vector<vec<Watched> >& wsFull
    , const Lit lit1
    , const Lit lit2
) {
    vec<Watched>::const_iterator i = wsFull[lit1.toInt()].begin();
    vec<Watched>::const_iterator end = wsFull[lit1.toInt()].end();
    for (; i != end && (!i->isBinary() || i->lit1() != lit2); i++);
    return i != end;
}

static inline bool findWBin(
    const vector<vec<Watched> >& wsFull
    , const Lit lit1
    , const Lit lit2
    , const bool learnt
) {
    vec<Watched>::const_iterator i = wsFull[lit1.toInt()].begin();
    vec<Watched>::const_iterator end = wsFull[lit1.toInt()].end();
    for (; i != end && (
        !i->isBinary()
        || i->lit1() != lit2
        || i->learnt() != learnt
    ); i++);

    return i != end;
}

static inline void removeWBin(
    vector<vec<Watched> > &wsFull
    , const Lit lit1
    , const Lit lit2
    , const bool learnt
) {
    vec<Watched>& ws = wsFull[lit1.toInt()];
    vec<Watched>::iterator i = ws.begin(), end = ws.end();
    for (; i != end && (
        !i->isBinary()
        || i->lit1() != lit2
        || i->learnt() != learnt
    ); i++);

    assert(i != end);
    vec<Watched>::iterator j = i;
    i++;
    for (; i != end; j++, i++) *j = *i;
    ws.shrink_(1);
}

static inline Watched& findWatchedOfBin(
    vector<vec<Watched> >& wsFull
    , const Lit lit1
    , const Lit lit2
    , const bool learnt
) {
    vec<Watched>& ws = wsFull[lit1.toInt()];
    for (vec<Watched>::iterator i = ws.begin(), end = ws.end(); i != end; i++) {
        if (i->isBinary() && i->lit1() == lit2 && i->learnt() == learnt)
            return *i;
    }

    assert(false);
    return *ws.begin();
}

static inline Watched& findWatchedOfBin(
    vector<vec<Watched> >& wsFull
    , const Lit lit1
    , const Lit lit2
) {
    vec<Watched>& ws = wsFull[lit1.toInt()];
    for (vec<Watched>::iterator i = ws.begin(), end = ws.end(); i != end; i++) {
        if (i->isBinary() && i->lit1() == lit2)
            return *i;
    }

    assert(false);
    return *ws.begin();
}

#endif
