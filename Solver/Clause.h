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

#ifndef CLAUSE_H
#define CLAUSE_H

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
    4th -31st bit  28bit uint       size
    */
    uint32_t size_etc; 
    union { int act; uint32_t abst; } extra;
    Lit     data[0];

public:

    template<class V>
    Clause(const V& ps, const uint _group, const bool learnt) :
            group(_group)
    {
        size_etc = 0;
        setSize(ps.size());
        setLearnt(learnt);
        for (uint i = 0; i < ps.size(); i++) data[i] = ps[i];
        if (learnt) extra.act = 0;
        else calcAbstraction();
    }

    // -- use this function instead:
    template<class T>
    friend Clause* Clause_new(const T& ps, const uint group, const bool learnt = false);

    uint         size        ()      const {
        return size_etc >> 4;
    }
    void         shrink      (uint i) {
        assert(i <= size());
        size_etc = (((size_etc >> 4) - i) << 4) | (size_etc & ((1 << 4)-1));
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

    void         setActivity(int i)  {
        extra.act = i;
    }
    
    const int&   activity   () const {
        return extra.act;
    }
    
    Lit subsumes (const Clause& other) const;
    
    inline void strengthen(const Lit p)
    {
        remove(*this, p);
        calcAbstraction();
    }
    
    void calcAbstraction() {
        uint32_t abstraction = 0;
        for (int i = 0; i < size(); i++)
        abstraction |= 1 << (data[i].var() & 31);
        extra.abst = abstraction;
    }

    const Lit*     getData     () const {
        return data;
    }
    Lit*    getData     () {
        return data;
    }
    void print() {
        printf("Clause   group: %d, size: %d, learnt:%d, lits: ", group, size(), learnt());
        plainPrint();
    }
    void plainPrint(FILE* to = stdout) const {
        for (uint i = 0; i < size(); i++) {
            if (data[i].sign()) fprintf(to, "-");
            fprintf(to, "%d ", data[i].var() + 1);
        }
        fprintf(to, "0\n");
    }
protected:
    void setSize(uint32_t size) {
        size_etc = (size_etc & ((1 << 4)-1)) + (size << 4);
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
    friend XorClause* XorClause_new(const V& ps, const bool inverted, const uint group);

    inline bool xor_clause_inverted() const
    {
        return size_etc & 8;
    }
    inline void invert(bool b)
    {
        size_etc ^= (uint32_t)b << 3;
    }

    void print() {
        printf("XOR Clause   group: %d, size: %d, learnt:%d, lits:\"", group, size(), learnt());
        plainPrint();
    }
    
    void plainPrint(FILE* to = stdout) const {
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
    inline void setInverted(bool inverted)
    {
        size_etc = (size_etc & 7) + ((uint32_t)inverted << 3) + (size_etc & ~15);
    }
};

template<class T>
Clause* Clause_new(const T& ps, const uint group, const bool learnt = false)
{
    void* mem = malloc(sizeof(Clause) + sizeof(Lit)*(ps.size()));
    Clause* real= new (mem) Clause(ps, group, learnt);
    return real;
}

template<class T>
XorClause* XorClause_new(const T& ps, const bool inverted, const uint group)
{
    void* mem = malloc(sizeof(XorClause) + sizeof(Lit)*(ps.size()));
    XorClause* real= new (mem) XorClause(ps, inverted, group);
    return real;
}

/*_________________________________________________________________________________________________
|
|  subsumes : (other : const Clause&)  ->  Lit
|
|  Description:
|       Checks if clause subsumes 'other', and at the same time, if it can be used to simplify 'other'
|       by subsumption resolution.
|
|    Result:
|       lit_Error  - No subsumption or simplification
|       lit_Undef  - Clause subsumes 'other'
|       p          - The literal p can be deleted from 'other'
|________________________________________________________________________________________________@*/
inline Lit Clause::subsumes(const Clause& other) const
{
    if (other.size() < size() || (extra.abst & ~other.extra.abst) != 0)
        return lit_Error;
    
    Lit        ret = lit_Undef;
    const Lit* c  = this->getData();
    const Lit* d  = other.getData();
    
    for (int i = 0; i < size(); i++) {
        // search for c[i] or ~c[i]
        for (int j = 0; j < other.size(); j++)
            if (c[i] == d[j])
                goto ok;
            else if (ret == lit_Undef && c[i] == ~d[j]){
                ret = c[i];
                goto ok;
            }
            
            // did not find it
            return lit_Error;
        ok:;
    }
    
    return ret;
}


class WatchedBin {
    public:
        WatchedBin(Clause *_clause, Lit _impliedLit) : impliedLit(_impliedLit), clause(_clause) {};
        Lit impliedLit;
        Clause *clause;
};

#endif //CLAUSE_H
