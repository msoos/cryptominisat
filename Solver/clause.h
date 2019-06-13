/***********************************************************************************[SolverTypes.h]
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

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

#ifndef __clause_h__
#define __clause_h__

#include <stdint.h>
#include <cstdio>
#include <vector>
#include <sys/types.h>
#include "Vec.h"
#include "SolverTypes.h"

#ifndef uint
#define uint unsigned int
#endif

using std::vector;


//=================================================================================================
// Clause -- a simple class for representing a clause:


class Clause
{
public:
    const uint group;
protected:
    uint32_t size_etc;
    float act;
    Lit     data[0];

public:
    Clause(const vec<Lit>& ps, const uint _group, const bool learnt) :
            group(_group) {
        size_etc = (ps.size() << 4) | (uint32_t)learnt ;
        for (int i = 0; i < ps.size(); i++) data[i] = ps[i];
        if (learnt) act = 0;
    }

    Clause(const vector<Lit>& ps, const uint _group, const bool learnt) :
            group(_group) {
        size_etc = (ps.size() << 4) | (uint32_t)learnt ;
        for (uint i = 0; i < ps.size(); i++) data[i] = ps[i];
        if (learnt) act = 0;
    }

    // -- use this function instead:
    friend Clause* Clause_new(const vec<Lit>& ps, const uint group, const bool learnt = false);
    friend Clause* Clause_new(const vector<Lit>& ps, const uint group, const bool learnt = false);

    uint         size        ()      const {
        return size_etc >> 4;
    }
    void         shrink      (uint i) {
        assert(i <= size());
        size_etc = (((size_etc >> 4) - i) << 4) | (size_etc & 15);
    }
    void         pop         () {
        shrink(1);
    }
    bool         learnt      ()      const {
        return size_etc & 1;
    }
    uint32_t     mark        ()      const {
        return (size_etc >> 1) & 3;
    }
    void         mark        (uint32_t m) {
        size_etc = (size_etc & ~6) | ((m & 3) << 1);
    }

    Lit&         operator [] (uint32_t i) {
        return data[i];
    }
    const Lit&   operator [] (uint32_t i) const {
        return data[i];
    }

    float&       activity    () {
        return act;
    }

    Lit*	 getData     () {
        return data;
    }
    void print() {
        Clause& c = *this;
        printf("group: %d, size: %d, learnt:%d, lits:\"", c.group, c.size(), c.learnt());
        for (uint i = 0; i < c.size(); i++) {
            if (c[i].sign())  printf("-");
            printf("%d ", c[i].var());
        }
        printf("\"\n");
    }
};

class XorClause : public Clause
{
public:
    // NOTE: This constructor cannot be used directly (doesn't allocate enough memory).
    template<class V>
    XorClause(const V& ps, const bool _xor_clause_inverted, const uint _group, const bool learnt) :
            Clause(ps, _group, learnt) {
        size_etc |= (((uint32_t)_xor_clause_inverted) << 3);
    }

    inline bool	 xor_clause_inverted() const {
        return size_etc & 8;
    }
    inline void	 invert      (bool b) {
        size_etc ^= (uint32_t)b << 3;
    }

    void print() {
        Clause& c = *this;
        printf("group: %d, size: %d, learnt:%d, lits:\"", c.group, c.size(), c.learnt());
        for (uint i = 0; i < c.size();) {
            assert(!c[i].sign());
            printf("%d", c[i].var());
            i++;
            if (i < c.size()) printf(" + ");
        }
        printf("\"\n");
    }
};

template<class V>
    XorClause* XorClause_new(const V& ps, const bool xor_clause_inverted, const uint group, const bool learnt = false) {
        void* mem = malloc(sizeof(XorClause) + sizeof(Lit)*(ps.size()));
        XorClause* real= new (mem) XorClause(ps, xor_clause_inverted, group, learnt);
        return real;
    }

Clause* Clause_new(const vec<Lit>& ps, const uint group, const bool learnt);
Clause* Clause_new(const vector<Lit>& ps, const uint group, const bool learnt);

#endif
