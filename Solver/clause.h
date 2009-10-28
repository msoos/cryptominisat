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
#include "packedRow.h"

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
    /**
    bit-layout of size_etc:
    
    range          type             meaning
    --------------------------------------------
    0th bit        bool            learnt clause
    1st - 2nd bit  2bit int        marking
    3rd bit        bool            inverted xor
    4th-7th bit    4bit int        matrix number
    8th -31st bit  24bit int       size
    */
    uint32_t size_etc; 
    float act;
    Lit     data[0];

public:
    Clause(const packedRow& row, const vec<lbool>& assigns, const vector<uint>& col_to_var_original, const uint _group) :
        group(_group)
    {
        size_etc = 0;
        setSize(row.popcnt());
        setLearnt(false);
        row.fill(data, assigns, col_to_var_original);
    }

    template<class V>
    Clause(const V& ps, const uint _group, const bool learnt) :
            group(_group)
    {
        size_etc = 0;
        setSize(ps.size());
        setLearnt(learnt);
        for (uint i = 0; i < ps.size(); i++) data[i] = ps[i];
        if (learnt) act = 0;
    }

    // -- use this function instead:
    friend Clause* Clause_new(const vec<Lit>& ps, const uint group, const bool learnt = false);
    friend Clause* Clause_new(const vector<Lit>& ps, const uint group, const bool learnt = false);

    uint         size        ()      const {
        return size_etc >> 8;
    }
    void         shrink      (uint i) {
        assert(i <= size());
        size_etc = (((size_etc >> 8) - i) << 8) | (size_etc & 255);
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
            printf("%d ", c[i].var()+1);
        }
        printf("\"\n");
    }
protected:
    void setSize(uint32_t size) {
        size_etc = ((size_etc & (uint32_t)255) + (size << 8));
    }
    bool setLearnt(bool learnt) {
        size_etc = (size_etc & ~((uint32_t)1)) + (uint32_t)learnt;
    }
};

class XorClause : public Clause
{
public:
    // NOTE: This constructor cannot be used directly (doesn't allocate enough memory).
    template<class V>
    XorClause(const V& ps, const bool inverted, const uint _group, const uint matrix_no) :
        Clause(ps, _group, false)
    {
        setMatrix(matrix_no);
        setInverted(inverted);
    }

    // -- use this function instead:
    template<class V>
    friend XorClause* XorClause_new(const V& ps, const bool inverted, const uint group, const uint matrix_no = 15) {
        void* mem = malloc(sizeof(XorClause) + sizeof(Lit)*(ps.size()));
        XorClause* real= new (mem) XorClause(ps, inverted, group, matrix_no);
        return real;
    }

    inline bool xor_clause_inverted() const
    {
        return size_etc & 8;
    }
    inline void invert(bool b)
    {
        size_etc ^= (uint32_t)b << 3;
    }
    inline uint32_t inMatrix() const
    {
        return (((size_etc >> 4) & 15) != 15);
    }
    
    inline uint32_t getMatrix() const
    {
        assert(inMatrix());
        return ((size_etc >> 4) & 15);
    }

    void print() {
        Clause& c = *this;
        printf("group: %d, size: %d, learnt:%d, lits:\"", c.group, c.size(), c.learnt());
        for (uint i = 0; i < c.size();) {
            assert(!c[i].sign());
            printf("%d", c[i].var()+1);
            i++;
            if (i < c.size()) printf(" + ");
        }
        printf("\"\n");
    }
protected:
    inline void setMatrix   (uint32_t toset) {
        assert(toset < 16);
        size_etc = (size_etc & 15) + (toset << 4) + (size_etc & ~255);
    }
    inline void setInverted(bool inverted)
    {
        size_etc = (size_etc & 7) + ((uint32_t)inverted << 3) + ((size_etc >> 4) << 4);
    }
};

Clause* Clause_new(const vec<Lit>& ps, const uint group, const bool learnt);
Clause* Clause_new(const vector<Lit>& ps, const uint group, const bool learnt);
Clause* Clause_new(const packedRow& ps, const vec<lbool>& assigns, const vector<uint>& col_to_var_original, const uint group);

#endif
