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

template<class T>
static inline void printClause(T& ps)
{
    for (uint32_t i = 0; i < ps.size(); i++) {
        if (ps[i].sign()) printf("-");
        printf("%d ", ps[i].var() + 1);
    }
    printf("0\n");
}

template<class T>
static inline void printXorClause(T& ps, const bool xorEqualFalse)
{
    std::cout << "x";
    if (xorEqualFalse) std::cout << "-";
    for (uint32_t i = 0; i < ps.size(); i++) {
        std::cout << ps[i].var() + 1 << " ";
    }
    std::cout << "0" << std::endl;
}


template<class V, class T>
static inline void remove(V& ts, const T& t)
{
    uint32_t j = 0;
    for (; j < ts.size() && ts[j] != t; j++);
    assert(j < ts.size());
    for (; j < ts.size()-1; j++) ts[j] = ts[j+1];
    ts.pop();
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
static bool    findWBin(const vec<Watched>& ws, const Lit impliedLit);
static void    removeWBin(vec<Watched> &ws, const Lit impliedLit);
static void    removeWTri(vec<Watched> &ws, const Lit lit1, Lit lit2);
static const  uint32_t  removeWBinAll(vec<Watched> &ws, const Lit impliedLit);
static inline Watched&  findWatchedOfBin(vec<vec<Watched> >& wsFull, const Lit lit1, const Lit lit2);

//Xor Clause
static bool    findWXCl(const vec<Watched>& ws, const ClauseOffset c);
static void    removeWXCl(vec<Watched> &ws, const ClauseOffset c);

//////////////////
// NORMAL Clause
//////////////////
static inline bool findWCl(const vec<Watched>& ws, const ClauseOffset c)
{
    uint32_t j = 0;
    for (; j < ws.size() && (!ws[j].isClause() || ws[j].getOffset() != c); j++);
    return j < ws.size();
}

static inline void removeWCl(vec<Watched> &ws, const ClauseOffset c)
{
    uint32_t j = 0;
    for (; j < ws.size() && (!ws[j].isClause() || ws[j].getOffset() != c); j++);
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
    for (; j < ws.size() && (!ws[j].isXorClause() || ws[j].getOffset() != c); j++);
    return j < ws.size();
}

static inline void removeWXCl(vec<Watched> &ws, const ClauseOffset c)
{
    uint32_t j = 0;
    for (; j < ws.size() && (!ws[j].isXorClause() || ws[j].getOffset() != c); j++);
    assert(j < ws.size());
    for (; j < ws.size()-1; j++) ws[j] = ws[j+1];
    ws.pop();
}

//////////////////
// BINARY Clause
//////////////////
static inline bool findWBin(const vec<Watched>& ws, const Lit impliedLit)
{
    uint32_t j = 0;
    for (; j < ws.size() && (!ws[j].isBinary() || ws[j].getOtherLit() != impliedLit); j++);
    return j < ws.size();
}

static inline void removeWBin(vec<Watched> &ws, const Lit impliedLit)
{
    uint32_t j = 0;
    for (; j < ws.size() && (!ws[j].isBinary() || ws[j].getOtherLit() != impliedLit); j++);
    assert(j < ws.size());
    for (; j < ws.size()-1; j++) ws[j] = ws[j+1];
    ws.pop();
}

static inline void removeWTri(vec<Watched> &ws, const Lit lit1, const Lit lit2)
{
    uint32_t j = 0;
    for (; j < ws.size() && (!ws[j].isTriClause() || ws[j].getOtherLit() != lit1 || ws[j].getOtherLit2() != lit2); j++);
    assert(j < ws.size());
    for (; j < ws.size()-1; j++) ws[j] = ws[j+1];
    ws.pop();
}

static inline const uint32_t removeWBinAll(vec<Watched> &ws, const Lit impliedLit)
{
    uint32_t removed = 0;
    Watched *i = ws.getData();
    Watched *j = i;
    for (Watched* end = ws.getDataEnd(); i != end; i++) {
        if (!i->isBinary() || i->getOtherLit() != impliedLit)
            *j++ = *i;
        else removed++;
    }
    ws.shrink(i-j);

    return removed;
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
