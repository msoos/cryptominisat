/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.0 of the License.
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

#include "solverconf.h"
#include "solvertypes.h"
#include "constants.h"
#include "watched.h"
#include "alg.h"
#include "clabstraction.h"
#include "constants.h"

namespace CMSat {

class ClauseAllocator;

template <class T>
struct ResolutionTypes
{
    void clear()
    {
        *this = ResolutionTypes<T>();
    }

    uint64_t sum() const
    {
        return bin + tri + irredL + redL;
    }

    template <class T2>
    ResolutionTypes& operator+=(const ResolutionTypes<T2>& other)
    {
        bin += other.bin;
        tri += other.tri;
        irredL += other.irredL;
        redL += other.redL;

        return *this;
    }

    ResolutionTypes& operator-=(const ResolutionTypes& other)
    {
        bin -= other.bin;
        tri -= other.tri;
        irredL -= other.irredL;
        redL -= other.redL;

        return *this;
    }

    T bin = 0;
    T tri = 0;
    T irredL = 0;
    T redL = 0;
};

struct ClauseStats
{
    ClauseStats() :
        glue(std::numeric_limits<uint32_t>::max())
        , activity(0)
        , introduced_at_conflict(std::numeric_limits<uint32_t>::max())
        , propagations_made(0)
        , conflicts_made(0)
        , sum_of_branch_depth_conflict(0)
        , sum_of_branch_depth_propagation(0)
        #ifdef STATS_NEEDED
        , visited_literals(0)
        , clause_looked_at(0)
        #endif
        , used_for_uip_creation(0)
        , locked(false)
    {}

    uint64_t numPropAndConfl(const uint64_t confl_multiplier) const
    {
        return (uint64_t)propagations_made
        + (uint64_t)conflicts_made*confl_multiplier;
    }

    double confl_usefulness() const
    {
        double useful = 0;
        if (conflicts_made > 0) {
            uint64_t sum_tmp = sum_of_branch_depth_conflict;
            if (sum_tmp == 0) {
                sum_tmp = 1;
            }
            useful += ((double)conflicts_made*(double)conflicts_made)/(double)sum_tmp;
        }
        return useful;
    }

    //Stored data
    uint32_t glue;    ///<Clause glue
    double   activity;
    uint64_t introduced_at_conflict; ///<At what conflict number the clause  was introduced
    uint32_t propagations_made; ///<Number of times caused propagation
    uint32_t conflicts_made; ///<Number of times caused conflict
    uint64_t sum_of_branch_depth_conflict;
    uint64_t sum_of_branch_depth_propagation;
    #ifdef STATS_NEEDED
    uint64_t visited_literals; ///<Number of literals visited
    uint64_t clause_looked_at; ///<Number of times the clause has been deferenced during propagation
    #endif
    uint32_t used_for_uip_creation; ///Number of times the claue was using during 1st UIP conflict generation
    bool locked;

    ///Number of resolutions it took to make the clause when it was
    ///originally learnt. Only makes sense for redundant clauses
    ResolutionTypes<uint16_t> resolutions;

    void clear(const double multiplier)
    {
        activity = 0;
        propagations_made = (double)propagations_made * multiplier;
        conflicts_made = (double)conflicts_made * multiplier;
        sum_of_branch_depth_conflict = (double)sum_of_branch_depth_conflict * multiplier;
        sum_of_branch_depth_propagation = (double)sum_of_branch_depth_propagation * multiplier;
        #ifdef STATS_NEEDED
        visited_literals = 0;
        clause_looked_at = 0;
        #endif
        used_for_uip_creation = (double)used_for_uip_creation*multiplier;
    }

    static ClauseStats combineStats(const ClauseStats& first, const ClauseStats& second)
    {
        //Create to-be-returned data
        ClauseStats ret;

        //Combine stats
        ret.glue = std::min(first.glue, second.glue);
        ret.activity = std::max(first.activity, second.activity);
        ret.introduced_at_conflict = std::min(first.introduced_at_conflict, second.introduced_at_conflict);
        ret.propagations_made = first.propagations_made + second.propagations_made;
        ret.conflicts_made = first.conflicts_made + second.conflicts_made;
        ret.sum_of_branch_depth_conflict = first.sum_of_branch_depth_conflict  + second.sum_of_branch_depth_conflict;
        ret.sum_of_branch_depth_propagation = first.sum_of_branch_depth_propagation * second.sum_of_branch_depth_propagation;
        #ifdef STATS_NEEDED
        ret.visited_literals = first.visited_literals + second.visited_literals;
        ret.clause_looked_at = first.clause_looked_at + second.clause_looked_at;
        #endif
        ret.used_for_uip_creation = first.used_for_uip_creation + second.used_for_uip_creation;
        ret.locked = first.locked | second.locked;

        return ret;
    }
};

inline std::ostream& operator<<(std::ostream& os, const ClauseStats& stats)
{

    os << "glue " << stats.glue << " ";
    os << "conflIntro " << stats.introduced_at_conflict<< " ";
    os << "numProp " << stats.propagations_made<< " ";
    os << "numConfl " << stats.conflicts_made<< " ";
    #ifdef STATS_NEEDED
    os << "numLitVisit " << stats.visited_literals<< " ";
    os << "numLook " << stats.clause_looked_at<< " ";
    #endif
    os << "used_for_uip_creation" << stats.used_for_uip_creation << " ";

    return os;
}

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

    uint16_t isRed:1; ///<Is the clause a redundant clause?
    uint16_t isRemoved:1; ///<Is this clause queued for removal because of usless binary removal?
    uint16_t isFreed:1; ///<Has this clause been marked as freed by the ClauseAllocator ?
    uint16_t isAsymmed:1;
    uint16_t occurLinked:1;
    uint32_t mySize;


    Lit* getData()
    {
        return (Lit*)((char*)this + sizeof(Clause));
    }

    const Lit* getData() const
    {
        return (Lit*)((char*)this + sizeof(Clause));
    }

public:
    cl_abst_type abst;
    ClauseStats stats;

    template<class V>
    Clause(const V& ps, const uint32_t _introduced_at_conflict)
    {
        //assert(ps.size() > 2);

        stats.introduced_at_conflict = _introduced_at_conflict;
        stats.glue = std::min<uint32_t>(stats.glue, ps.size());
        isFreed = false;
        mySize = ps.size();
        isRed = false;
        isRemoved = false;
        isAsymmed = false;

        for (uint32_t i = 0; i < ps.size(); i++)
            getData()[i] = ps[i];

        setChanged();
    }

    typedef Lit* iterator;
    typedef const Lit* const_iterator;

    uint32_t size() const
    {
        return mySize;
    }

    void setChanged()
    {
        setStrenghtened();
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

    bool red() const
    {
        return isRed;
    }

    bool freed() const
    {
        return isFreed;
    }

    void reCalcAbstraction()
    {
        abst = calcAbstraction(*this);
    }

    void setStrenghtened()
    {
        abst = calcAbstraction(*this);
    }

    Lit& operator [] (const uint32_t i)
    {
        return *(getData() + i);
    }

    const Lit& operator [] (const uint32_t i) const
    {
        return *(getData() + i);
    }

    void makeIrred()
    {
        assert(isRed);
        isRed = false;
    }

    void makeRed(const uint32_t newGlue)
    {
        stats.glue = newGlue;
        isRed = true;
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

    void setAsymmed(bool asymmed)
    {
        isAsymmed = asymmed;
    }

    bool getAsymmed() const
    {
        return isAsymmed;
    }

    bool getOccurLinked() const
    {
        return occurLinked;
    }

    void setOccurLinked(bool toset)
    {
        occurLinked = toset;
    }

    void print_extra_stats() const
    {
        cout
        << "Clause size " << std::setw(4) << size();
        if (red()) {
            cout << " glue : " << std::setw(4) << stats.glue;
        }
        cout
        << " Props: " << std::setw(10) << stats.propagations_made
        << " Confls: " << std::setw(10) << stats.conflicts_made
        #ifdef STATS_NEEDED
        << " Lit visited: " << std::setw(10)<< stats.visited_literals
        << " Looked at: " << std::setw(10)<< stats.clause_looked_at
        << " Props&confls/Litsvisited*10: ";
        if (stats.visited_literals > 0) {
            cout
            << std::setw(6) << std::fixed << std::setprecision(4)
            << (10.0*(double)stats.numPropAndConfl(1)/(double)stats.visited_literals);
        }
        #endif
        ;
        cout << " UIP used: " << std::setw(10)<< stats.used_for_uip_creation;
        cout << endl;
    }
};

inline std::ostream& operator<<(std::ostream& os, const Clause& cl)
{
    for (uint32_t i = 0; i < cl.size(); i++) {
        os << cl[i];

        if (i+1 != cl.size())
            os << " ";
    }

    return os;
}

} //end namespace

#endif //CLAUSE_H
