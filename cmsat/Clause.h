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
#include "cmsat/ClAbstraction.h"
#include "constants.h"

class ClauseAllocator;

struct ClauseStats
{
    ClauseStats() :
        glue(std::numeric_limits<uint16_t>::max())
        , conflictNumIntroduced(std::numeric_limits<uint32_t>::max())
        , numPropAndConfl(0)
        , numLitVisited(0)
        , numLookedAt(0)
    {}

    //Stored data
    uint16_t glue;    ///<Clause glue
    uint32_t conflictNumIntroduced; ///<At what conflict number the clause  was introduced
    uint32_t numPropAndConfl; ///<Number of times caused propagation or conflict
    uint32_t numLitVisited; ///<Number of literals visited
    uint32_t numLookedAt; ///<Number of times the clause has been deferenced during propagation

    static ClauseStats combineStats(const ClauseStats& first, const ClauseStats& second)
    {
        //Create to-be-returned data
        ClauseStats ret;

        //Combine stats
        ret.glue = std::min(first.glue, second.glue);
        ret.conflictNumIntroduced = std::min(first.conflictNumIntroduced, second.conflictNumIntroduced);
        ret.numPropAndConfl = first.numPropAndConfl + second.numPropAndConfl;
        ret.numLitVisited = first.numLitVisited + second.numLitVisited;
        ret.numLookedAt = first.numLookedAt + second.numLookedAt;

        return ret;
    };
};

inline std::ostream& operator<<(std::ostream& os, const ClauseStats& stats)
{

    os << "glue " << stats.glue << " ";
    os << "conflIntro " << stats.conflictNumIntroduced<< " ";
    os << "numPropConfl " << stats.numPropAndConfl<< " ";
    os << "numLitVisit " << stats.numLitVisited<< " ";
    os << "numLook " << stats.numLookedAt<< " ";

    return os;
}

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

    uint32_t isRemoved:1; ///<Is this clause queued for removal because of usless binary removal?
    uint32_t isFreed:1; ///<Has this clause been marked as freed by the ClauseAllocator ?
    uint16_t mySize; ///<The current size of the clause


    Lit* getData()
    {
        return (Lit*)((char*)this + sizeof(Clause));
    }

    const Lit* getData() const
    {
        return (Lit*)((char*)this + sizeof(Clause));
    }

public:
    char defOfOrGate; //TODO make it into a bitfield above
    CL_ABST_TYPE abst;
    ClauseStats stats;

    template<class V>
    Clause(const V& ps, const uint32_t _conflictNumIntroduced)
    {
        assert(ps.size() > 2);

        stats.conflictNumIntroduced = _conflictNumIntroduced;
        stats.glue = std::min<uint16_t>(stats.glue, ps.size());
        defOfOrGate = false;
        isFreed = false;
        mySize = ps.size();
        isLearnt = false;
        isRemoved = false;

        for (uint32_t i = 0; i < ps.size(); i++)
            getData()[i] = ps[i];

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
        if (i > 0)
            setStrenghtened();
    }

    void resize (const uint32_t i)
    {
        assert(i <= size());
        if (i == size()) return;
        mySize = i;
        setStrenghtened();
    }

    bool learnt() const
    {
        return isLearnt;
    }

    bool freed() const
    {
        return isFreed;
    }

    bool getStrenghtened() const
    {
        return strenghtened;
    }

    void setStrenghtened()
    {
        abst = calcAbstraction(*this);
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

    void makeNonLearnt()
    {
        assert(isLearnt);
        isLearnt = false;
    }

    void makeLearnt(const uint32_t newGlue)
    {
        stats.glue = newGlue;
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

    void combineStats(const ClauseStats& other)
    {
        stats = ClauseStats::combineStats(stats, other);
    }
};

inline std::ostream& operator<<(std::ostream& os, const Clause& cl)
{
    for (uint32_t i = 0; i < cl.size(); i++) {
        os << cl[i] << " ";
    }
    return os;
}

#endif //CLAUSE_H
