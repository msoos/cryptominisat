/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
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
        glue(0x1fffffff)
        , locked(false)
        , marked_clause(false)
        , ttl(0)
    {}

    //Stored data
    double   activity = 0.0;
    #ifdef STATS_NEEDED
    uint64_t introduced_at_conflict = std::numeric_limits<uint32_t>::max(); ///<At what conflict number the clause  was introduced
    uint32_t conflicts_made = 0; ///<Number of times caused conflict
    uint64_t sum_of_branch_depth_conflict = 0;
    uint32_t propagations_made = 0; ///<Number of times caused propagation
    uint64_t visited_literals = 0; ///<Number of literals visited
    uint64_t clause_looked_at = 0; ///<Number of times the clause has been deferenced during propagation
    uint32_t used_for_uip_creation = 0; ///Number of times the claue was using during 1st UIP conflict generation
    #endif
    uint32_t glue:29;
    uint32_t locked:1;
    uint32_t marked_clause:1;
    uint32_t ttl:1;

    ///Number of resolutions it took to make the clause when it was
    ///originally learnt. Only makes sense for redundant clauses
    ResolutionTypes<uint16_t> resolutions;

    void clear()
    {
        activity = 0;
        #ifdef STATS_NEEDED
        conflicts_made = 0;
        sum_of_branch_depth_conflict = 0;
        propagations_made = 0;;
        visited_literals = 0;
        clause_looked_at = 0;
        used_for_uip_creation = 0;
        #endif
    }

    static ClauseStats combineStats(const ClauseStats& first, const ClauseStats& second)
    {
        //Create to-be-returned data
        ClauseStats ret;

        //Combine stats
        ret.glue = std::min(first.glue, second.glue);
        ret.activity = std::max(first.activity, second.activity);
        #ifdef STATS_NEEDED
        ret.introduced_at_conflict = std::min(first.introduced_at_conflict, second.introduced_at_conflict);
        ret.conflicts_made = first.conflicts_made + second.conflicts_made;
        ret.sum_of_branch_depth_conflict = first.sum_of_branch_depth_conflict  + second.sum_of_branch_depth_conflict;
        ret.propagations_made = first.propagations_made + second.propagations_made;
        ret.visited_literals = first.visited_literals + second.visited_literals;
        ret.clause_looked_at = first.clause_looked_at + second.clause_looked_at;
        ret.used_for_uip_creation = first.used_for_uip_creation + second.used_for_uip_creation;
        #endif
        ret.locked = first.locked | second.locked;

        return ret;
    }
};

inline std::ostream& operator<<(std::ostream& os, const ClauseStats& stats)
{

    os << "glue " << stats.glue << " ";
    #ifdef STATS_NEEDED
    os << "conflIntro " << stats.introduced_at_conflict<< " ";
    os << "numConfl " << stats.conflicts_made<< " ";
    os << "numProp " << stats.propagations_made<< " ";
    os << "numLitVisit " << stats.visited_literals<< " ";
    os << "numLook " << stats.clause_looked_at<< " ";
    os << "used_for_uip_creation" << stats.used_for_uip_creation << " ";
    #endif

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

    uint32_t mySize;
    uint16_t isRed:1; ///<Is the clause a redundant clause?
    uint16_t isRemoved:1; ///<Is this clause queued for removal because of usless binary removal?
    uint16_t isFreed:1; ///<Has this clause been marked as freed by the ClauseAllocator ?
    uint16_t is_distilled:1;
    uint16_t occurLinked:1;
    uint16_t must_recalc_abst:1;


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
    Clause(const V& ps
        , const uint32_t
        #ifdef STATS_NEEDED
        _introduced_at_conflict
        #endif
        )
    {
        //assert(ps.size() > 2);

        #ifdef STATS_NEEDED
        stats.introduced_at_conflict = _introduced_at_conflict;
        #endif
        stats.glue = std::min<uint32_t>(stats.glue, ps.size());
        isFreed = false;
        mySize = ps.size();
        isRed = false;
        isRemoved = false;
        is_distilled = false;
        must_recalc_abst = true;

        for (uint32_t i = 0; i < ps.size(); i++) {
            getData()[i] = ps[i];
        }
    }

    typedef Lit* iterator;
    typedef const Lit* const_iterator;

    uint32_t size() const
    {
        return mySize;
    }

    void shrink(const uint32_t i)
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
        must_recalc_abst = false;
    }

    void setStrenghtened()
    {
        must_recalc_abst = true;
    }

    void recalc_abst_if_needed()
    {
        if (must_recalc_abst) {
            reCalcAbstraction();
        }
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
        setStrenghtened();
    }

    const Lit* begin() const
    {
        #ifdef SLOW_DEBUG
        assert(!freed());
        assert(!getRemoved());
        #endif
        return getData();
    }

    Lit* begin()
    {
        #ifdef SLOW_DEBUG
        assert(!freed());
        assert(!getRemoved());
        #endif
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

    void unset_removed()
    {
        isRemoved = false;
    }

    void setFreed()
    {
        isFreed = true;
    }

    void combineStats(const ClauseStats& other)
    {
        stats = ClauseStats::combineStats(stats, other);
    }

    void set_distilled(bool distilled)
    {
        is_distilled = distilled;
    }

    bool getdistilled() const
    {
        return is_distilled;
    }

    bool getOccurLinked() const
    {
        return occurLinked;
    }

    void set_occur_linked(bool toset)
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
        #ifdef STATS_NEEDED
        cout
        << " Confls: " << std::setw(10) << stats.conflicts_made
        << " Props: " << std::setw(10) << stats.propagations_made
        << " Lit visited: " << std::setw(10)<< stats.visited_literals
        << " Looked at: " << std::setw(10)<< stats.clause_looked_at
        << " UIP used: " << std::setw(10)<< stats.used_for_uip_creation;
        #endif
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

struct BinaryXor
{
    Var vars[2];
    bool rhs;

    BinaryXor(Var var1, Var var2, const bool _rhs) {
        if (var1 > var2) {
            std::swap(var1, var2);
        }
        vars[0] = var1;
        vars[1] = var2;
        rhs = _rhs;
    }

    bool operator<(const BinaryXor& other) const
    {
        if (vars[0] != other.vars[0]) {
            return vars[0] < other.vars[0];
        }

        if (vars[1] != other.vars[1]) {
            return vars[1] < other.vars[1];
        }

        if (rhs != other.rhs) {
            return (int)rhs < (int)other.rhs;
        }
        return false;
    }
};

} //end namespace

#endif //CLAUSE_H
