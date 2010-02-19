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

#ifdef _MSC_VER
#include <msvc/stdint.h>
#else
#include <stdint.h>
#endif //_MSC_VER
#include <cstdio>
#include <vector>
#include <sys/types.h>
#include "Vec.h"
#include "SolverTypes.h"
#include "PackedRow.h"
#include "constants.h"
#include "SmallPtr.h"
//#include "pool.hpp"
#ifndef uint
#define uint unsigned int
#endif

using std::vector;

//=================================================================================================
// Clause -- a simple class for representing a clause:

class MatrixFinder;

class Clause
{
protected:
    
    #ifdef STATS_NEEDED
    uint group;
    #endif
    
    uint32_t isLearnt:1;
    uint32_t marking:2;
    uint32_t invertedXor:1;
    uint32_t isXorClause:1;
    uint32_t mySize:20;
    
    int act;
    #ifdef _MSC_VER
    Lit     data[1];
    #else
    Lit     data[0];
    #endif //_MSC_VER

#ifdef _MSC_VER
public:
#endif //_MSC_VER
    template<class V>
    Clause(const V& ps, const uint _group, const bool learnt)
    {
        isXorClause = false;
        mySize = ps.size();
        isLearnt = learnt;
        setGroup(_group);
        for (uint i = 0; i < ps.size(); i++) data[i] = ps[i];
        if (learnt) act = 0;
        //else calcAbstraction();
    }

public:
    #ifndef _MSC_VER
    // -- use this function instead:
    template<class T>
    friend Clause* Clause_new(const T& ps, const uint group, const bool learnt = false);
    #endif //_MSC_VER

    const uint   size        ()      const {
        return mySize;
    }
    void         resize      (const uint size) {
        mySize = size;
    }
    void         shrink      (const uint i) {
        assert(i <= size());
        mySize -= i;
    }
    void         pop         () {
        shrink(1);
    }
    const bool   isXor       () {
        return isXorClause;
    }
    const bool   learnt      ()      const {
        return isLearnt;
    }
    const uint32_t mark        ()      const {
        return marking;
    }
    void         mark        (uint32_t m) {
        marking = m;
    }

    Lit&         operator [] (uint32_t i) {
        return data[i];
    }
    const Lit&   operator [] (uint32_t i) const {
        return data[i];
    }

    void         setActivity(int i)  {
        act = i;
    }
    
    const int&   activity   () const {
        return act;
    }
    
    Lit subsumes (const Clause& other) const;
    
    inline void  strengthen(const Lit p)
    {
        remove(*this, p);
        //calcAbstraction();
    }
    
    /*void calcAbstraction() {
        uint32_t abstraction = 0;
        for (uint32_t i = 0; i != size(); i++)
            abstraction |= 1 << (data[i].var() & 31);
        extra.abst = abstraction;
    }*/

    const Lit*     getData     () const {
        return data;
    }
    Lit*    getData     () {
        return data;
    }
    void print() {
        printf("Clause   group: %d, size: %d, learnt:%d, lits: ", getGroup(), size(), learnt());
        plainPrint();
    }
    void plainPrint(FILE* to = stdout) const {
        for (uint i = 0; i < size(); i++) {
            if (data[i].sign()) fprintf(to, "-");
            fprintf(to, "%d ", data[i].var() + 1);
        }
        fprintf(to, "0\n");
    }
    #ifdef STATS_NEEDED
    const uint32_t getGroup() const
    {
        return group;
    }
    void setGroup(const uint32_t _group)
    {
        group = _group;
    }
    #else
    const uint getGroup() const
    {
        return 0;
    }
    void setGroup(const uint32_t _group)
    {
        return;
    }
    #endif //STATS_NEEDED
};

class XorClause : public Clause
{
    
#ifdef _MSC_VER
public:
#else //_MSC_VER
protected:
#endif //_MSC_VER

    // NOTE: This constructor cannot be used directly (doesn't allocate enough memory).
    template<class V>
    XorClause(const V& ps, const bool inverted, const uint _group) :
        Clause(ps, _group, false)
    {
        invertedXor = inverted;
        isXorClause = true;
    }

public:
    #ifndef _MSC_VER
    // -- use this function instead:
    template<class V>
    friend XorClause* XorClause_new(const V& ps, const bool inverted, const uint group);
    #endif //_MSC_VER

    inline bool xor_clause_inverted() const
    {
        return invertedXor;
    }
    inline void invert(bool b)
    {
        invertedXor ^= b;
    }

    void print() {
        printf("XOR Clause   group: %d, size: %d, learnt:%d, lits:\"", getGroup(), size(), learnt());
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
};

//extern boost::pool<> binaryClausePool;

template<class T>
Clause* Clause_new(const T& ps, const uint group, const bool learnt = false)
{
    void* mem;
    //if (ps.size() != 2)
        mem = malloc(sizeof(Clause) + sizeof(Lit)*(ps.size()));
    //else
    //    mem = binaryClausePool.malloc();
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

inline void clauseFree(Clause* c)
{
    //if (binaryClausePool.is_from(c)) binaryClausePool.free(c);
    //else 
    free(c);
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
/*inline Lit Clause::subsumes(const Clause& other) const
{
    if (other.size() < size() || (extra.abst & ~other.extra.abst) != 0)
        return lit_Error;
    
    Lit        ret = lit_Undef;
    const Lit* c  = this->getData();
    const Lit* d  = other.getData();
    
    for (uint32_t i = 0; i != size(); i++) {
        // search for c[i] or ~c[i]
        for (uint32_t j = 0; j != other.size(); j++)
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
}*/

typedef sptr<Clause> ClausePtr;
typedef sptr<XorClause> XorClausePtr;
//typedef Clause* ClausePtr;
//typedef XorClause* XorClausePtr;

#pragma pack(push)
#pragma pack(1)
class WatchedBin {
    public:
        WatchedBin(Clause *_clause, Lit _impliedLit) : clause(_clause), impliedLit(_impliedLit) {};
        ClausePtr clause;
        Lit impliedLit;
};

class Watched {
    public:
        Watched(Clause *_clause, Lit _blockedLit) : clause(_clause), blockedLit(_blockedLit) {};
        ClausePtr clause;
        Lit blockedLit;
};
#pragma pack(pop)

#endif //CLAUSE_H
