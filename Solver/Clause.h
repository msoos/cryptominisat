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

#ifndef CLAUSE_H
#define CLAUSE_H

#include <cstdio>
#include <vector>
#include <sys/types.h>
#include <string.h>
#include <limits>

#include "SolverTypes.h"
#include "constants.h"
#include "Watched.h"
#include "Alg.h"
#include "constants.h"
#include "ClauseAllocator.h"

class ClauseAllocator;

/**
@brief Holds a clause. Does not allocate space for literals

Literals are allocated by an external allocator that allocates enough space
for the class that it can hold the literals as well. I.e. it malloc()-s
    sizeof(Clause)+LENGHT*sizeof(Lit)
to hold the clause.
*/
struct Clause
{
protected:

    uint32_t isLearnt:1; ///<Is the clause a learnt clause?
    uint32_t strenghtened:1; ///<Has the clause been strenghtened since last SatELite-like work?
    uint32_t changed:1; ///<Var inside clause has been changed

    uint16_t isRemoved:1; ///<Is this clause queued for removal because of usless binary removal?
    uint16_t isFreed:1; ///<Has this clause been marked as freed by the ClauseAllocator ?
    uint16_t glue:MAX_GLUE_BITS;    ///<Clause glue -- clause activity according to GLUCOSE
    uint16_t mySize; ///<The current size of the clause

    uint32_t num;

    Lit* getData()
    {
        return (Lit*)((char*)this + sizeof(Clause));
    }

    const Lit* getData() const
    {
        return (Lit*)((char*)this + sizeof(Clause));
    }

public:
    template<class V>
    Clause(const V& ps, const uint32_t clauseNum)
    {
        num = clauseNum;
        isFreed = false;
        assert(ps.size() > 2);
        mySize = ps.size();
        isLearnt = false;
        isRemoved = false;

        for (uint32_t i = 0; i < ps.size(); i++) getData()[i] = ps[i];
        glue = MAX_THEORETICAL_GLUE;
        setChanged();
    }

    typedef Lit* iterator;
    typedef const Lit* const_iterator;

    friend class ClauseAllocator;

    uint16_t size() const
    {
        return mySize;
    }

    bool getChanged() const
    {
        return changed;
    }

    void setChanged()
    {
        setStrenghtened();
        changed = 1;
    }

    void unsetChanged()
    {
        changed = 0;
    }

    void shrink (const uint32_t i)
    {
        assert(i <= size());
        mySize -= i;
        if (i > 0) setStrenghtened();
    }

    void resize (const uint32_t i)
    {
        assert(i <= size());
        if (i == size()) return;
        mySize = i;
        setStrenghtened();
    }

    void pop()
    {
        shrink(1);
    }

    bool learnt() const
    {
        return isLearnt;
    }

    bool getStrenghtened() const
    {
        return strenghtened;
    }

    void setStrenghtened()
    {
        strenghtened = true;
    }

    void unsetStrenghtened()
    {
        strenghtened = false;
    }

    Lit& operator [] (const uint32_t i)
    {
        return *(getData() + i);
    }

    const Lit& operator [] (const uint32_t i) const
    {
        return *(getData() + i);
    }

    void setGlue(const uint32_t newGlue)
    {
        assert(newGlue <= MAX_THEORETICAL_GLUE);
        glue = newGlue;
    }

    uint32_t getGlue() const
    {
        return glue;
    }

    void makeNonLearnt()
    {
        assert(isLearnt);
        isLearnt = false;
    }

    void makeLearnt(const uint32_t newGlue)
    {
        glue = newGlue;
        isLearnt = true;
    }

    void strengthen(const Lit p)
    {
        remove(*this, p);
        setStrenghtened();
    }

    void add(const Lit p)
    {
        mySize++;
        getData()[mySize-1] = p;
        setChanged();
    }

    const Lit* begin() const
    {
        return getData();
    }

    Lit* begin()
    {
        return getData();
    }

    const Lit* end() const
    {
        return getData()+size();
    }

    Lit* end()
    {
        return getData()+size();
    }

    void setRemoved()
    {
        isRemoved = true;
    }

    bool getRemoved() const
    {
        return isRemoved;
    }

    void setFreed()
    {
        isFreed = true;
    }

    bool getFreed() const
    {
        return isFreed;
    }

    void takeMaxOfStats(Clause& other)
    {
        if (other.getGlue() < getGlue())
            setGlue(other.getGlue());
    }

    uint32_t getNum() const
    {
        return num;
    }

    void setNum(const uint32_t newNum)
    {
        num = newNum;
    }
};

inline std::ostream& operator<<(std::ostream& os, const Clause& cl)
{
    for (uint32_t i = 0; i < cl.size(); i++) {
        os << cl[i] << " ";
    }
    return os;
}

struct ClauseData
{
    ClauseData()
    {
        litPos[0] = std::numeric_limits<uint16_t>::max();
        litPos[1] = std::numeric_limits<uint16_t>::max();
    };
    ClauseData(const uint16_t lit1Pos, const uint16_t lit2Pos)
    {
        litPos[0] = lit1Pos;
        litPos[1] = lit2Pos;
    };

    uint16_t operator[](const bool which) const
    {
        return litPos[which];
    }

    uint16_t& operator[](const bool which)
    {
        return litPos[which];
    }

    void operator=(const ClauseData& other)
    {
        litPos[0] = other.litPos[0];
        litPos[1] = other.litPos[1];
    }

    uint16_t litPos[2];
};

#endif //CLAUSE_H
