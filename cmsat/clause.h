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

#include "solvertypes.h"
#include "constants.h"
#include "watched.h"
#include "alg.h"
#include "clabstraction.h"
#include "constants.h"

class ClauseAllocator;

template <class T>
struct ResolutionTypes
{
    ResolutionTypes() :
        bin(0)
        , tri(0)
        , irredL(0)
        , redL(0)
    {}

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

    T bin;
    T tri;
    T irredL;
    T redL;
};

struct ClauseStats
{
    ClauseStats() :
        glue(std::numeric_limits<uint16_t>::max())
        , activity(0)
        , conflictNumIntroduced(std::numeric_limits<uint32_t>::max())
        , numProp(0)
        , numConfl(0)
        #ifdef STATS_NEEDED
        , numLitVisited(0)
        , numLookedAt(0)
        #endif
        , numUsedUIP(0)
    {}

    uint32_t numPropAndConfl() const
    {
        return numProp + numConfl;
    }

    //Stored data
    uint16_t glue;    ///<Clause glue
    double   activity;
    uint32_t conflictNumIntroduced; ///<At what conflict number the clause  was introduced
    uint32_t numProp; ///<Number of times caused propagation
    uint32_t numConfl; ///<Number of times caused conflict
    #ifdef STATS_NEEDED
    uint32_t numLitVisited; ///<Number of literals visited
    uint32_t numLookedAt; ///<Number of times the clause has been deferenced during propagation
    #endif
    uint32_t numUsedUIP; ///Number of times the claue was using during 1st UIP conflict generation

    ///Number of resolutions it took to make the clause when it was
    ///originally learnt. Only makes sense for learnt clauses
    ResolutionTypes<uint16_t> resolutions;

    void clearAfterReduceDB()
    {
        activity = 0;
        numProp = 0;
        numConfl = 0;
        #ifdef STATS_NEEDED
        numLitVisited = 0;
        numLookedAt = 0;
        #endif
        numUsedUIP = 0;
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
struct Clause
{
protected:

    uint32_t isLearnt:1; ///<Is the clause a learnt clause?
    uint32_t strenghtened:1; ///<Has the clause been strenghtened since last simplification?
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

    void reCalcAbstraction()
    {
        abst = calcAbstraction(*this);
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
};

enum clauseCleaningTypes {
    CLEAN_CLAUSES_GLUE_BASED
    , CLEAN_CLAUSES_SIZE_BASED
    , CLEAN_CLAUSES_PROPCONFL_BASED
    ,  CLEAN_CLAUSES_ACTIVITY_BASED
};

inline std::string getNameOfCleanType(clauseCleaningTypes clauseCleaningType)
{
    switch(clauseCleaningType) {
        case CLEAN_CLAUSES_GLUE_BASED :
            return "glue";

        case CLEAN_CLAUSES_SIZE_BASED:
            return "size";

        case CLEAN_CLAUSES_PROPCONFL_BASED:
            return "propconfl";

        case CLEAN_CLAUSES_ACTIVITY_BASED:
            return "activity";

        default:
            assert(false && "Unknown clause cleaning type?");
    };

    return "";
}

struct CleaningStats
{
    struct Data
    {
        Data() :
            num(0)
            , lits(0)
            , age(0)

            , glue(0)
            , numProp(0)
            , numConfl(0)
            , numLitVisited(0)
            , numLookedAt(0)
            , numUsedUIP(0)

            , act(0)
        {}

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

        uint64_t num;
        uint64_t lits;
        uint64_t age;

        uint64_t glue;
        uint64_t numProp;
        uint64_t numConfl;
        uint64_t numLitVisited;
        uint64_t numLookedAt;
        uint64_t numUsedUIP;
        ResolutionTypes<uint64_t> resol;
        double   act;

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
            , "% long learnt clauses"
        );

        printStatsLine("c pre-removed lits"
            , preRemove.lits
            , (double)preRemove.lits/(double)origNumLits*100.0
            , "% long learnt lits"
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
            , "% long learnt clauses"
        );
        printStatsLine("c cleaned lits"
            , removed.lits
            , (double)removed.lits/(double)origNumLits*100.0
            , "% long learnt lits"
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
            , "% long learnt clauses"
        );
        printStatsLine("c remain lits"
            , remain.lits
            , (double)remain.lits/(double)origNumLits*100.0
            , "% long learnt lits"
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
        << " next by " << getNameOfCleanType(clauseCleaningType)
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
    clauseCleaningTypes clauseCleaningType;
    size_t glueBasedClean;
    size_t sizeBasedClean;
    size_t propConflBasedClean;
    size_t actBasedClean;

    //Clause Cleaning
    Data removed;
    Data remain;
};

enum StampType {
    STAMP_IRRED = 0
    , STAMP_RED = 1
};

struct Timestamp
{
    Timestamp()
    {
        start[STAMP_IRRED] = 0;
        start[STAMP_RED] = 0;

        end[STAMP_IRRED] = 0;
        end[STAMP_RED] = 0;

        dominator[STAMP_IRRED] = lit_Undef;
        dominator[STAMP_RED] = lit_Undef;

        numDom[STAMP_IRRED] = 0;
        numDom[STAMP_RED] = 0;
    }

    Timestamp& operator=(const Timestamp& other)
    {
        memcpy(this, &other, sizeof(Timestamp));

        return *this;
    }

    uint64_t start[2];
    uint64_t end[2];

    Lit dominator[2];
    uint64_t numDom[2];
};

struct StampSorter
{
    StampSorter(
        const vector<Timestamp>& _timestamp
        , const StampType _stampType
        , const bool _rev
    ) :
        timestamp(_timestamp)
        , stampType(_stampType)
        , rev(_rev)
    {}

    const vector<Timestamp>& timestamp;
    const StampType stampType;
    const bool rev;

    bool operator()(const Lit lit1, const Lit lit2) const
    {
        if (!rev) {
            return timestamp[lit1.toInt()].start[stampType]
                    < timestamp[lit2.toInt()].start[stampType];
        } else {
            return timestamp[lit1.toInt()].start[stampType]
                    > timestamp[lit2.toInt()].start[stampType];
        }
    }
};

struct StampSorterInv
{
    StampSorterInv(
        const vector<Timestamp>& _timestamp
        , const StampType _stampType
        , const bool _rev
    ) :
        timestamp(_timestamp)
        , stampType(_stampType)
        , rev(_rev)
    {}

    const vector<Timestamp>& timestamp;
    const StampType stampType;
    const bool rev;

    bool operator()(const Lit lit1, const Lit lit2) const
    {
        if (!rev) {
            return timestamp[(~lit1).toInt()].start[stampType]
                < timestamp[(~lit2).toInt()].start[stampType];
        } else {
            return timestamp[(~lit1).toInt()].start[stampType]
                > timestamp[(~lit2).toInt()].start[stampType];
        }
    }
};

inline bool stampBasedClRem(
    const vector<Lit>& lits
    , const vector<Timestamp>& stamp
    , vector<Lit>& stampNorm
    , vector<Lit>& stampInv
) {
    StampSorter sortNorm(stamp, STAMP_IRRED, false);
    StampSorterInv sortInv(stamp, STAMP_IRRED, false);

    stampNorm = lits;
    stampInv = lits;

    std::sort(stampNorm.begin(), stampNorm.end(), sortNorm);
    std::sort(stampInv.begin(), stampInv.end(), sortInv);

    assert(lits.size() > 0);
    vector<Lit>::const_iterator lpos = stampNorm.begin();
    vector<Lit>::const_iterator lneg = stampInv.begin();

    while(true) {
        if (stamp[(~*lneg).toInt()].start[STAMP_IRRED]
            >= stamp[lpos->toInt()].start[STAMP_IRRED]
        ) {
            lpos++;

            if (lpos == stampNorm.end())
                return false;
        } else if (stamp[(~*lneg).toInt()].end[STAMP_IRRED]
            <= stamp[lpos->toInt()].end[STAMP_IRRED]
        ) {
            lneg++;

            if (lneg == stampInv.end())
                return false;
        } else {
            return true;
        }
    }

    return false;
}

inline std::pair<size_t, size_t> stampBasedLitRem(
    vector<Lit>& lits
    , const vector<Timestamp>& timestamp
    , const StampType stampType
) {
    size_t remLitTimeStamp = 0;
    StampSorter sorter(timestamp, stampType, true);
    std::sort(lits.begin(), lits.end(), sorter);

    #ifdef DEBUG_STAMPING
    cout << "Timestamps: ";
    for(size_t i = 0; i < lits.size(); i++) {
        cout
        << " " << timestamp[lits[i].toInt()].start[stampType]
        << "," << timestamp[lits[i].toInt()].end[stampType];
    }
    cout << endl;
    cout << "Ori clause: " << lits << endl;
    #endif

    assert(!lits.empty());
    Lit lastLit = lits[0];
    for(size_t i = 1; i < lits.size(); i++) {
        if (timestamp[lastLit.toInt()].end[stampType]
            < timestamp[lits[i].toInt()].end[stampType]
        ) {
            lits[i] = lit_Undef;
            remLitTimeStamp++;
        } else {
            lastLit = lits[i];
        }
    }

    if (remLitTimeStamp) {
        //First literal cannot be removed
        assert(lits.front() != lit_Undef);

        //At least 1 literal must remain
        assert(remLitTimeStamp < lits.size());

        //Remove lit_Undef-s
        size_t at = 0;
        for(size_t i = 0; i < lits.size(); i++) {
            if (lits[i] != lit_Undef) {
                lits[at++] = lits[i];
            }
        }
        lits.resize(lits.size()-remLitTimeStamp);

        #ifdef DEBUG_STAMPING
        cout << "New clause: " << lits << endl;
        #endif
    }

    size_t remLitTimeStampInv = 0;
    StampSorterInv sorterInv(timestamp, stampType, false);
    std::sort(lits.begin(), lits.end(), sorterInv);
    assert(!lits.empty());
    lastLit = lits[0];

    for(size_t i = 1; i < lits.size(); i++) {
        if (timestamp[(~lastLit).toInt()].end[stampType]
            > timestamp[(~lits[i]).toInt()].end[stampType]
        ) {
            lits[i] = lit_Undef;
            remLitTimeStampInv++;
        } else {
            lastLit = lits[i];
        }
    }

    if (remLitTimeStampInv) {
        //First literal cannot be removed
        assert(lits.front() != lit_Undef);

        //At least 1 literal must remain
        assert(remLitTimeStampInv < lits.size());

        //Remove lit_Undef-s
        size_t at = 0;
        for(size_t i = 0; i < lits.size(); i++) {
            if (lits[i] != lit_Undef) {
                lits[at++] = lits[i];
            }
        }
        lits.resize(lits.size()-remLitTimeStampInv);

        #ifdef DEBUG_STAMPING
        cout << "New clause: " << lits << endl;
        #endif
    }


    return std::make_pair(remLitTimeStamp, remLitTimeStampInv);
}

#endif //CLAUSE_H
