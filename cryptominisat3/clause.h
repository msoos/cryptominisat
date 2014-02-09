/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.0 of the License, or (at your option) any later version.
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
        , conflictNumIntroduced(std::numeric_limits<uint32_t>::max())
        , numProp(0)
        , numConfl(0)
        #ifdef STATS_NEEDED
        , numLitVisited(0)
        , numLookedAt(0)
        #endif
        , numUsedUIP(0)
        , locked(false)
    {}

    uint32_t numPropAndConfl() const
    {
        return numProp + numConfl;
    }

    //Stored data
    uint32_t glue;    ///<Clause glue
    double   activity;
    uint32_t conflictNumIntroduced; ///<At what conflict number the clause  was introduced
    uint32_t numProp; ///<Number of times caused propagation
    uint32_t numConfl; ///<Number of times caused conflict
    #ifdef STATS_NEEDED
    uint32_t numLitVisited; ///<Number of literals visited
    uint32_t numLookedAt; ///<Number of times the clause has been deferenced during propagation
    #endif
    uint32_t numUsedUIP; ///Number of times the claue was using during 1st UIP conflict generation
    bool locked;

    ///Number of resolutions it took to make the clause when it was
    ///originally learnt. Only makes sense for redundant clauses
    ResolutionTypes<uint16_t> resolutions;

    void clearAfterReduceDB(const double multiplier)
    {
        activity = 0;
        numProp = (double)numProp * multiplier;
        numConfl = (double)numConfl * multiplier;
        #ifdef STATS_NEEDED
        numLitVisited = 0;
        numLookedAt = 0;
        #endif
        numUsedUIP = (double)numUsedUIP*multiplier;
    }

    static ClauseStats combineStats(const ClauseStats& first, const ClauseStats& second)
    {
        //Create to-be-returned data
        ClauseStats ret;

        //Combine stats
        ret.glue = std::min(first.glue, second.glue);
        ret.activity = std::max(first.activity, second.activity);
        ret.conflictNumIntroduced = std::min(first.conflictNumIntroduced, second.conflictNumIntroduced);
        ret.numProp = first.numProp + second.numProp;
        ret.numConfl = first.numConfl + second.numConfl;
        #ifdef STATS_NEEDED
        ret.numLitVisited = first.numLitVisited + second.numLitVisited;
        ret.numLookedAt = first.numLookedAt + second.numLookedAt;
        #endif
        ret.numUsedUIP = first.numUsedUIP + second.numUsedUIP;
        ret.locked = first.locked | second.locked;

        return ret;
    };
};

inline std::ostream& operator<<(std::ostream& os, const ClauseStats& stats)
{

    os << "glue " << stats.glue << " ";
    os << "conflIntro " << stats.conflictNumIntroduced<< " ";
    os << "numProp " << stats.numProp<< " ";
    os << "numConfl " << stats.numConfl<< " ";
    #ifdef STATS_NEEDED
    os << "numLitVisit " << stats.numLitVisited<< " ";
    os << "numLook " << stats.numLookedAt<< " ";
    #endif
    os << "numUsedUIP" << stats.numUsedUIP << " ";

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
    CL_ABST_TYPE abst;
    ClauseStats stats;

    template<class V>
    Clause(const V& ps, const uint32_t _conflictNumIntroduced)
    {
        //assert(ps.size() > 2);

        stats.conflictNumIntroduced = _conflictNumIntroduced;
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

    friend class ClauseAllocator;

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
        << " Props: " << std::setw(10) << stats.numProp
        << " Confls: " << std::setw(10) << stats.numConfl
        #ifdef STATS_NEEDED
        << " Lit visited: " << std::setw(10)<< stats.numLitVisited
        << " Looked at: " << std::setw(10)<< stats.numLookedAt
        << " Props&confls/Litsvisited*10: ";
        if (stats.numLitVisited > 0) {
            cout
            << std::setw(6) << std::fixed << std::setprecision(4)
            << (10.0*(double)stats.numPropAndConfl()/(double)stats.numLitVisited);
        }
        #endif
        ;
        cout << " UIP used: " << std::setw(10)<< stats.numUsedUIP;
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

struct ClauseUsageStats
{
    ClauseUsageStats() :
        num(0)
        , sumProp(0)
        , sumConfl(0)
        , sumLitVisited(0)
        , sumLookedAt(0)
        , sumUsedUIP(0)
    {}

    uint64_t sumPropAndConfl() const
    {
        return sumProp + sumConfl;
    }

    uint64_t num;
    uint64_t sumProp;
    uint64_t sumConfl;
    uint64_t sumLitVisited;
    uint64_t sumLookedAt;
    uint64_t sumUsedUIP;

    ClauseUsageStats& operator+=(const ClauseUsageStats& other)
    {
        num += other.num;
        sumProp += other.sumProp;
        sumConfl += other.sumConfl;
        sumLitVisited += other.sumLitVisited;
        sumLookedAt += other.sumLookedAt;
        sumUsedUIP += other.sumUsedUIP;

        return *this;
    }

    void addStat(const Clause& cl)
    {
        num++;
        sumProp += cl.stats.numProp;
        sumConfl += cl.stats.numConfl;
        #ifdef STATS_NEEDED
        sumLitVisited += cl.stats.numLitVisited;
        sumLookedAt += cl.stats.numLookedAt;
        #endif
        sumUsedUIP += cl.stats.numUsedUIP;
    }

    void print() const
    {
        cout
        #ifdef STATS_NEEDED
        << " lits visit: "
        << std::setw(8) << sumLitVisited/1000UL
        << "K"

        << " cls visit: "
        << std::setw(7) << sumLookedAt/1000UL
        << "K"
        #endif

        << " prop: "
        << std::setw(5) << sumProp/1000UL
        << "K"

        << " conf: "
        << std::setw(5) << sumConfl/1000UL
        << "K"

        << " UIP used: "
        << std::setw(5) << sumUsedUIP/1000UL
        << "K"
        << endl;
    }
};

struct CleaningStats
{
    struct Data
    {
        uint64_t sumResolutions() const
        {
            return resol.sum();
        }

        Data& operator+=(const Data& other)
        {
            num += other.num;
            lits += other.lits;
            age += other.age;

            glue += other.glue;
            numProp += other.numProp;
            numConfl += other.numConfl;
            numLitVisited += other.numLitVisited;
            numLookedAt += other.numLookedAt;
            numUsedUIP += other.numUsedUIP;
            resol += other.resol;

            act += other.act;

            return *this;
        }

        uint64_t num = 0;
        uint64_t lits = 0;
        uint64_t age = 0;

        uint64_t glue = 0;
        uint64_t numProp = 0;
        uint64_t numConfl = 0;
        uint64_t numLitVisited = 0;
        uint64_t numLookedAt = 0;
        uint64_t numUsedUIP = 0;
        ResolutionTypes<uint64_t> resol;
        double   act = 0.0;

        void incorporate(const Clause* cl)
        {
            num ++;
            lits += cl->size();
            glue += cl->stats.glue;
            act += cl->stats.activity;
            numConfl += cl->stats.numConfl;
            #ifdef STATS_NEEDED
            numLitVisited += cl->stats.numLitVisited;
            numLookedAt += cl->stats.numLookedAt;
            #endif
            numProp += cl->stats.numProp;
            resol += cl->stats.resolutions;
            numUsedUIP += cl->stats.numUsedUIP;
        }


    };
    CleaningStats() :
        cpu_time(0)
        //Before remove
        , origNumClauses(0)
        , origNumLits(0)

        //Type of clean
        , glueBasedClean(0)
        , sizeBasedClean(0)
        , propConflBasedClean(0)
        , actBasedClean(0)
    {}

    CleaningStats& operator+=(const CleaningStats& other)
    {
        //Time
        cpu_time += other.cpu_time;

        //Before remove
        origNumClauses += other.origNumClauses;
        origNumLits += other.origNumLits;

        //Type of clean
        glueBasedClean += other.glueBasedClean;
        sizeBasedClean += other.sizeBasedClean;
        propConflBasedClean += other.propConflBasedClean;
        actBasedClean += other.actBasedClean;

        //Clause Cleaning data
        preRemove += other.preRemove;
        removed += other.removed;
        remain += other.remain;

        return *this;
    }

    void print(const size_t nbReduceDB) const
    {
        cout << "c ------ CLEANING STATS ---------" << endl;
        //Pre-clean
        printStatsLine("c pre-removed"
            , preRemove.num
            , (double)preRemove.num/(double)origNumClauses*100.0
            , "% long redundant clauses"
        );

        printStatsLine("c pre-removed lits"
            , preRemove.lits
            , (double)preRemove.lits/(double)origNumLits*100.0
            , "% long red lits"
        );
        printStatsLine("c pre-removed cl avg size"
            , (double)preRemove.lits/(double)preRemove.num
        );
        printStatsLine("c pre-removed cl avg glue"
            , (double)preRemove.glue/(double)preRemove.num
        );
        printStatsLine("c pre-removed cl avg num resolutions"
            , (double)preRemove.sumResolutions()/(double)preRemove.num
        );

        //Types of clean
        printStatsLine("c clean by glue"
            , glueBasedClean
            , (double)glueBasedClean/(double)nbReduceDB*100.0
            , "% cleans"
        );
        printStatsLine("c clean by size"
            , sizeBasedClean
            , (double)sizeBasedClean/(double)nbReduceDB*100.0
            , "% cleans"
        );
        printStatsLine("c clean by prop&confl"
            , propConflBasedClean
            , (double)propConflBasedClean/(double)nbReduceDB*100.0
            , "% cleans"
        );

        //--- Actual clean --

        //-->CLEAN
        printStatsLine("c cleaned cls"
            , removed.num
            , (double)removed.num/(double)origNumClauses*100.0
            , "% long redundant clauses"
        );
        printStatsLine("c cleaned lits"
            , removed.lits
            , (double)removed.lits/(double)origNumLits*100.0
            , "% long red lits"
        );
        printStatsLine("c cleaned cl avg size"
            , (double)removed.lits/(double)removed.num
        );
        printStatsLine("c cleaned avg glue"
            , (double)removed.glue/(double)removed.num
        );

        //--> REMAIN
        printStatsLine("c remain cls"
            , remain.num
            , (double)remain.num/(double)origNumClauses*100.0
            , "% long redundant clauses"
        );
        printStatsLine("c remain lits"
            , remain.lits
            , (double)remain.lits/(double)origNumLits*100.0
            , "% long red lits"
        );
        printStatsLine("c remain cl avg size"
            , (double)remain.lits/(double)remain.num
        );
        printStatsLine("c remain avg glue"
            , (double)remain.glue/(double)remain.num
        );

        cout << "c ------ CLEANING STATS END ---------" << endl;
    }

    void printShort() const
    {
        //Pre-clean
        cout
        << "c [DBclean]"
        << " Pre-removed: "
        << preRemove.num
        << " clean type will be " << getNameOfCleanType(clauseCleaningType)
        << endl;

        cout
        << "c [DBclean]"
        << " rem " << removed.num

        << " avgGlue " << std::fixed << std::setprecision(2)
        << ((double)removed.glue/(double)removed.num)

        << " avgSize "
        << std::fixed << std::setprecision(2)
        << ((double)removed.lits/(double)removed.num)
        << endl;

        cout
        << "c [DBclean]"
        << " remain " << remain.num

        << " avgGlue " << std::fixed << std::setprecision(2)
        << ((double)remain.glue/(double)remain.num)

        << " avgSize " << std::fixed << std::setprecision(2)
        << ((double)remain.lits/(double)remain.num)

        << " T " << std::fixed << std::setprecision(2)
        << cpu_time
        << endl;
    }

    //Time
    double cpu_time;

    //Before remove
    uint64_t origNumClauses;
    uint64_t origNumLits;

    //Clause Cleaning --pre-remove
    Data preRemove;

    //Clean type
    ClauseCleaningTypes clauseCleaningType;
    size_t glueBasedClean;
    size_t sizeBasedClean;
    size_t propConflBasedClean;
    size_t actBasedClean;

    //Clause Cleaning
    Data removed;
    Data remain;
};

} //end namespace

#endif //CLAUSE_H
