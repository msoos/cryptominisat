/*******************************************************************************************[Alg.h]
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef Alg_h
#define Alg_h

#include <iostream>
#include "Vec.h"
#include "../Solver/SolverTypes.h"
#include "../Solver/Watched.h"

#ifdef _MSC_VER
#include <msvc/stdint.h>
#else
#include <stdint.h>
#endif //_MSC_VER

//=================================================================================================
// Useful functions on vectors

template<class V, class T>
static inline void remove(V& ts, const T& t)
{
    uint32_t j = 0;
    for (; j < ts.size() && ts[j] != t; j++);
    assert(j < ts.size());
    for (; j < (uint16_t)(ts.size()-1); j++) ts[j] = ts[j+1];
    ts.pop();
}

template<class V>
static inline const uint32_t removeAll(V& ts, const Var t)
{
    Lit* i = ts.getData();
    Lit* j = i;
    for (Lit *end = ts.getDataEnd(); i != end; i++) {
        if (i->var() != t) {
            *j++ = *i;
        }
    }
    ts.shrink(i-j);

    return (i-j);
}

template<class V, class T>
static inline void removeW(V& ts, const T& t)
{
    uint32_t j = 0;
    for (; j < ts.size() && ts[j].clause != t; j++);
    assert(j < ts.size());
    for (; j < ts.size()-1; j++) ts[j] = ts[j+1];
    ts.pop();
}

template<class V, class T>
static inline bool find(V& ts, const T& t)
{
    uint32_t j = 0;
    for (; j < ts.size() && ts[j] != t; j++);
    return j < ts.size();
}

template<class V, class T>
static inline bool findW(V& ts, const T& t)
{
    uint32_t j = 0;
    for (; j < ts.size() && ts[j].clause != t; j++);
    return j < ts.size();
}


//Normal clause
static bool    findWCl(const vec<Watched>& ws, const ClauseOffset c);
static void    removeWCl(vec<Watched> &ws, const ClauseOffset c);

//Binary clause
static bool    findWBin(const vec<vec<Watched> >& wsFull, const Lit lit1, const Lit impliedLit);
static bool    findWBin(const vec<vec<Watched> >& wsFull, const Lit lit1, const Lit impliedLit, const bool learnt);
static void    removeWBin(vec<Watched> &ws, const Lit impliedLit, const bool learnt);
static void    removeWTri(vec<Watched> &ws, const Lit lit1, Lit lit2);
static const std::pair<uint32_t, uint32_t>  removeWBinAll(vec<Watched> &ws, const Lit impliedLit);
static Watched& findWatchedOfBin(vec<vec<Watched> >& wsFull, const Lit lit1, const Lit lit2, const bool learnt);
static Watched& findWatchedOfBin(vec<vec<Watched> >& wsFull, const Lit lit1, const Lit lit2);

//Xor Clause
static bool    findWXCl(const vec<Watched>& ws, const ClauseOffset c);
static void    removeWXCl(vec<Watched> &ws, const ClauseOffset c);

//////////////////
// NORMAL Clause
//////////////////
static inline bool findWCl(const vec<Watched>& ws, const ClauseOffset c)
{
    uint32_t j = 0;
    for (; j < ws.size() && (!ws[j].isClause() || ws[j].getNormOffset() != c); j++);
    return j < ws.size();
}

static inline void removeWCl(vec<Watched> &ws, const ClauseOffset c)
{
    uint32_t j = 0;
    for (; j < ws.size() && (!ws[j].isClause() || ws[j].getNormOffset() != c); j++);
    assert(j < ws.size());
    for (; j < ws.size()-1; j++) ws[j] = ws[j+1];
    ws.pop();
}

//////////////////
// XOR Clause
//////////////////
static inline bool findWXCl(const vec<Watched>& ws, const ClauseOffset c)
{
    uint32_t j = 0;
    for (; j < ws.size() && (!ws[j].isXorClause() || ws[j].getXorOffset() != c); j++);
    return j < ws.size();
}

static inline void removeWXCl(vec<Watched> &ws, const ClauseOffset c)
{
    uint32_t j = 0;
    for (; j < ws.size() && (!ws[j].isXorClause() || ws[j].getXorOffset() != c); j++);
    assert(j < ws.size());
    for (; j < ws.size()-1; j++) ws[j] = ws[j+1];
    ws.pop();
}

//////////////////
// TRI Clause
//////////////////

static inline const bool findWTri(const vec<Watched> &ws, const Lit lit1, const Lit lit2)
{
    uint32_t j = 0;
    for (; j < ws.size() && (!ws[j].isTriClause() || ws[j].getOtherLit() != lit1 || ws[j].getOtherLit2() != lit2); j++);
    return(j < ws.size());
}

static inline void removeWTri(vec<Watched> &ws, const Lit lit1, const Lit lit2)
{
    uint32_t j = 0;
    for (; j < ws.size() && (!ws[j].isTriClause() || ws[j].getOtherLit() != lit1 || ws[j].getOtherLit2() != lit2); j++);
    assert(j < ws.size());
    for (; j < ws.size()-1; j++) ws[j] = ws[j+1];
    ws.pop();
}

//////////////////
// BINARY Clause
//////////////////
static inline bool findWBin(const vec<vec<Watched> >& wsFull, const Lit lit1, const Lit impliedLit)
{
    uint32_t j = 0;
    const vec<Watched>& ws = wsFull[(~lit1).toInt()];
    for (; j < ws.size() && (!ws[j].isBinary() || ws[j].getOtherLit() != impliedLit); j++);
    return j < ws.size();
}

static inline bool findWBin(const vec<vec<Watched> >& wsFull, const Lit lit1, const Lit impliedLit, const bool learnt)
{
    uint32_t j = 0;
    const vec<Watched>& ws = wsFull[(~lit1).toInt()];
    for (; j < ws.size() && (!ws[j].isBinary() || ws[j].getOtherLit() != impliedLit || ws[j].getLearnt() != learnt); j++);
    return j < ws.size();
}

static inline void removeWBin(vec<Watched> &ws, const Lit impliedLit, const bool learnt)
{
    uint32_t j = 0;
    for (; j < ws.size() && (!ws[j].isBinary() || ws[j].getOtherLit() != impliedLit || ws[j].getLearnt() != learnt); j++);
    assert(j < ws.size());
    for (; j < ws.size()-1; j++) ws[j] = ws[j+1];
    ws.pop();
}

static inline const std::pair<uint32_t, uint32_t>  removeWBinAll(vec<Watched> &ws, const Lit impliedLit)
{
    uint32_t removedLearnt = 0;
    uint32_t removedNonLearnt = 0;

    Watched *i = ws.getData();
    Watched *j = i;
    for (Watched* end = ws.getDataEnd(); i != end; i++) {
        if (!i->isBinary() || i->getOtherLit() != impliedLit)
            *j++ = *i;
        else {
            if (i->getLearnt())
                removedLearnt++;
            else
                removedNonLearnt++;
        }
    }
    ws.shrink_(i-j);

    return std::make_pair(removedLearnt, removedNonLearnt);
}

static inline Watched& findWatchedOfBin(vec<vec<Watched> >& wsFull, const Lit lit1, const Lit lit2, const bool learnt)
{
    vec<Watched>& ws = wsFull[(~lit1).toInt()];
    for (Watched *i = ws.getData(), *end = ws.getDataEnd(); i != end; i++) {
        if (i->isBinary() && i->getOtherLit() == lit2 && i->getLearnt() == learnt)
            return *i;
    }
    assert(false);

    return wsFull[0][0];
}

static inline Watched& findWatchedOfBin(vec<vec<Watched> >& wsFull, const Lit lit1, const Lit lit2)
{
    vec<Watched>& ws = wsFull[(~lit1).toInt()];
    for (Watched *i = ws.getData(), *end = ws.getDataEnd(); i != end; i++) {
        if (i->isBinary() && i->getOtherLit() == lit2)
            return *i;
    }
    assert(false);

    return wsFull[0][0];
}

#endif
