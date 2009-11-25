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
#include "PackedRow.h"

#ifndef uint
#define uint unsigned int
#endif

using std::vector;


//=================================================================================================
// Clause -- a simple class for representing a clause:

class MatrixFinder;

class Clause
{
public:
    const uint group;
protected:
    /**
    bit-layout of size_etc:
    
    range           type             meaning
    --------------------------------------------
    0th bit         bool            learnt clause
    1st - 2nd bit   2bit int        marking
    3rd bit         bool            inverted xor
    4th-15th bit    12bit int        matrix number
    16th -31st bit  16bit int       size
    */
    uint32_t size_etc; 
    float act;
    Lit     data[0];

public:
    Clause(const PackedRow& row, const vec<lbool>& assigns, const vector<uint>& col_to_var_original, const uint _group) :
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
        return size_etc >> 16;
    }
    void         shrink      (uint i) {
        assert(i <= size());
        size_etc = (((size_etc >> 16) - i) << 16) | (size_etc & ((1 << 16)-1));
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
        printf("Clause   group: %d, size: %d, learnt:%d, lits: ", group, size(), learnt());
        plain_print();
    }
    void plain_print(FILE* to = stdout) const {
        for (uint i = 0; i < size(); i++) {
            if (data[i].sign()) fprintf(to, "-");
            fprintf(to, "%d ", data[i].var() + 1);
        }
        fprintf(to, "0\n");
    }
protected:
    void setSize(uint32_t size) {
        size_etc = (size_etc & ((1 << 16)-1)) + (size << 16);
    }
    void setLearnt(bool learnt) {
        size_etc = (size_etc & ~1) + learnt;
    }
};

class XorClause : public Clause
{
public:
    // NOTE: This constructor cannot be used directly (doesn't allocate enough memory).
    template<class V>
    XorClause(const V& ps, const bool inverted, const uint _group) :
        Clause(ps, _group, false)
    {
        setInverted(inverted);
    }

    // -- use this function instead:
    template<class V>
    friend XorClause* XorClause_new(const V& ps, const bool inverted, const uint group) {
        void* mem = malloc(sizeof(XorClause) + sizeof(Lit)*(ps.size()));
        XorClause* real= new (mem) XorClause(ps, inverted, group);
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
    
    inline uint32_t getMatrix() const
    {
        return ((size_etc >> 4) & ((1 << 12)-1));
    }

    void print() {
        printf("XOR Clause   group: %d, size: %d, learnt:%d, lits:\"", group, size(), learnt());
        plain_print();
    }
    
    void plain_print(FILE* to = stdout) const {
        fprintf(to, "x");
        if (xor_clause_inverted())
            printf("-");
        for (uint i = 0; i < size(); i++) {
            fprintf(to, "%d ", data[i].var() + 1);
        }
        fprintf(to, "0\n");
    }
    
    friend class MatrixFinder;
    
protected:
    inline void setMatrix   (uint32_t toset) {
        assert(toset < (1 << 12));
        size_etc = (size_etc & 15) + (toset << 4) + (size_etc & ~((1 << 16)-1));
    }
    inline void setInverted(bool inverted)
    {
        size_etc = (size_etc & 7) + ((uint32_t)inverted << 3) + (size_etc & ~15);
    }
};

Clause* Clause_new(const vec<Lit>& ps, const uint group, const bool learnt);
Clause* Clause_new(const vector<Lit>& ps, const uint group, const bool learnt);
Clause* Clause_new(const PackedRow& ps, const vec<lbool>& assigns, const vector<uint>& col_to_var_original, const uint group);

#endif
