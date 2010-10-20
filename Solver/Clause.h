/*****************************************************************************
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
glucose -- Gilles Audemard, Laurent Simon (2008)
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

Original code by MiniSat and glucose authors are under an MIT licence.
Modifications for CryptoMiniSat are under GPLv3 licence.
******************************************************************************/

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
#include <string.h>

#include "SolverTypes.h"
#include "constants.h"
#include "Watched.h"
#include "Alg.h"
#include "constants.h"

template <class T>
uint32_t calcAbstraction(const T& ps) {
    uint32_t abstraction = 0;
    for (uint32_t i = 0; i != ps.size(); i++)
        abstraction |= 1 << (ps[i].var() & 31);
    return abstraction;
}

//=================================================================================================
// Clause -- a simple class for representing a clause:

class MatrixFinder;
class ClauseAllocator;

/**
@brief Holds a clause. Does not allocate space for literals

Literals are allocated by an external allocator that allocates enough space
for the class that it can hold the literals as well. I.e. it malloc()-s
    sizeof(Clause)+LENGHT*sizeof(Lit)
to hold the clause.
*/
class Clause
{
protected:

    uint32_t isLearnt:1; ///<Is the clause a learnt clause?
    uint32_t strenghtened:1; ///<Has the clause been strenghtened since last SatELite-like work?
    /**
    @brief Is the XOR equal to 1 or 0?

    i.e. "a + b" = TRUE or FALSE? -- we only have variables inside xor clauses,
    so this is important to know

    NOTE: ONLY set if the clause is an xor clause.
    */
    uint32_t isXorEqualFalse:1;
    uint32_t isXorClause:1; ///< Is the clause an XOR clause?
    uint32_t subsume0Done:1; ///Has normal subsumption been done with this clause?
    //uint32_t isRemoved:1; ///<Is this clause queued for removal because of usless binary removal?
    uint32_t isFreed:1; ///<Has this clause been marked as freed by the ClauseAllocator ?
    uint32_t glue:MAX_GLUE_BITS;    ///<Clause glue -- clause activity according to GLUCOSE
    uint32_t mySize:19; ///<The current size of the clause

    float miniSatAct; ///<Clause activity according to MiniSat

    uint32_t abst; //Abstraction of clause

    #ifdef STATS_NEEDED
    uint32_t group;
    #endif

    #ifdef _MSC_VER
    Lit     data[1];
    #else
    /**
    @brief Stores the literals in the clause

    This is the reason why we cannot have an instance of this class as it is:
    it cannot hold any literals in that case! This is a trick so that the
    literals are at the same place as the data of the clause, i.e. its size,
    glue, etc. We allocate therefore the clause manually, taking care that
    there is enough space for data[] to hold the literals
    */
    Lit     data[0];
    #endif //_MSC_VER

#ifdef _MSC_VER
public:
#endif //_MSC_VER
    template<class V>
    Clause(const V& ps, const uint32_t _group, const bool learnt)
    {
        isFreed = false;
        isXorClause = false;
        assert(ps.size() > 2);
        mySize = ps.size();
        isLearnt = learnt;
        //isRemoved = false;
        setGroup(_group);

        memcpy(data, ps.getData(), ps.size()*sizeof(Lit));
        miniSatAct = 0;
        setStrenghtened();
    }

public:
    friend class ClauseAllocator;

    const uint32_t size() const
    {
        return mySize;
    }

    void shrink (const uint32_t i)
    {
        assert(i <= size());
        mySize -= i;
        if (i > 0) setStrenghtened();
    }

    void pop()
    {
        shrink(1);
    }

    const bool isXor()
    {
        return isXorClause;
    }

    const bool learnt() const
    {
        return isLearnt;
    }

    float& getMiniSatAct()
    {
        return miniSatAct;
    }

    void setMiniSatAct(const float newMiniSatAct)
    {
        miniSatAct = newMiniSatAct;
    }

    const float& getMiniSatAct() const
    {
        return miniSatAct;
    }

    const bool getStrenghtened() const
    {
        return strenghtened;
    }

    void setStrenghtened()
    {
        strenghtened = true;
        subsume0Done = false;
        calcAbstractionClause();
    }

    void unsetStrenghtened()
    {
        strenghtened = false;
    }

    void subsume0Finished()
    {
        subsume0Done = 1;
    }

    const bool subsume0IsFinished()
    {
        return subsume0Done;
    }

    Lit& operator [] (uint32_t i)
    {
        return data[i];
    }

    const Lit& operator [] (uint32_t i) const
    {
        return data[i];
    }

    void setGlue(const uint32_t newGlue)
    {
        assert(newGlue <= MAX_THEORETICAL_GLUE);
        glue = newGlue;
    }

    const uint32_t getGlue() const
    {
        return glue;
    }

    void makeNonLearnt()
    {
        assert(isLearnt);
        isLearnt = false;
    }

    void makeLearnt(const uint32_t newGlue, const float newMiniSatAct)
    {
        glue = newGlue;
        miniSatAct = newMiniSatAct;
        isLearnt = true;
    }

    inline void strengthen(const Lit p)
    {
        remove(*this, p);
        setStrenghtened();
    }

    void calcAbstractionClause()
    {
        abst = calcAbstraction(*this);
    }

    uint32_t getAbst()
    {
        return abst;
    }

    const Lit* getData() const
    {
        return data;
    }

    Lit* getData()
    {
        return data;
    }

    const Lit* getDataEnd() const
    {
        return data+size();
    }

    Lit* getDataEnd()
    {
        return data+size();
    }

    void print(FILE* to = stdout) const
    {
        plainPrint(to);
        fprintf(to, "c clause learnt %s glue %d miniSatAct %.3f group %d\n", (learnt() ? "yes" : "no"), getGlue(), getMiniSatAct(), getGroup());
    }

    void plainPrint(FILE* to = stdout) const
    {
        for (uint32_t i = 0; i < size(); i++) {
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
    const uint32_t getGroup() const
    {
        return 0;
    }
    void setGroup(const uint32_t _group)
    {
        return;
    }
    #endif //STATS_NEEDED
    /*void setRemoved()
    {
        isRemoved = true;
    }

    const bool removed() const
    {
        return isRemoved;
    }*/

    void setFreed()
    {
        isFreed = true;
    }

    const bool freed() const
    {
        return isFreed;
    }
};

/**
@brief Holds an xor clause. Similarly to Clause, it cannot be directly used

The space is not allocated for the literals. See Clause for details
*/
class XorClause : public Clause
{
protected:
    // NOTE: This constructor cannot be used directly (doesn't allocate enough memory).
    template<class V>
    XorClause(const V& ps, const bool xorEqualFalse, const uint32_t _group) :
        Clause(ps, _group, false)
    {
        isXorEqualFalse = xorEqualFalse;
        isXorClause = true;
    }

public:
    friend class ClauseAllocator;

    inline const bool xorEqualFalse() const
    {
        return isXorEqualFalse;
    }

    inline void invert(const bool b)
    {
        isXorEqualFalse ^= b;
    }

    void print() const
    {
        printf("XOR Clause   group: %d, size: %d, learnt:%d, lits:\"", getGroup(), size(), learnt());
        plainPrint();
    }

    void plainPrint(FILE* to = stdout) const
    {
        fprintf(to, "x");
        if (xorEqualFalse())
            printf("-");
        for (uint32_t i = 0; i < size(); i++) {
            fprintf(to, "%d ", data[i].var() + 1);
        }
        fprintf(to, "0\n");
    }

    friend class MatrixFinder;
};


#endif //CLAUSE_H
