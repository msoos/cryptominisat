#ifndef __CLEANINGSTATS_H__
#define __CLEANINGSTATS_H__

#include <cstdint>
#include "clause.h"

namespace CMSat {

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
            used_for_uip_creation += other.used_for_uip_creation;
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
        uint64_t used_for_uip_creation = 0;
        ResolutionTypes<uint64_t> resol;
        double   act = 0.0;

        void incorporate(const Clause* cl)
        {
            num ++;
            lits += cl->size();
            glue += cl->stats.glue;
            act += cl->stats.activity;
            numConfl += cl->stats.conflicts_made;
            #ifdef STATS_NEEDED
            numLitVisited += cl->stats.visited_literals;
            numLookedAt += cl->stats.clause_looked_at;
            #endif
            numProp += cl->stats.propagations_made;
            resol += cl->stats.resolutions;
            used_for_uip_creation += cl->stats.used_for_uip_creation;
        }


    };

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
        propConflDepthBasedClean + other.propConflDepthBasedClean;

        //Clause Cleaning data
        removed += other.removed;
        remain += other.remain;

        return *this;
    }

    void print(const size_t nbReduceDB) const;
    void printShort() const;

    double cpu_time = 0;

    //Before remove
    uint64_t origNumClauses = 0;
    uint64_t origNumLits = 0;

    //Clean type
    ClauseCleaningTypes clauseCleaningType = clean_none;
    size_t glueBasedClean = 0;
    size_t sizeBasedClean = 0;
    size_t propConflBasedClean = 0;
    size_t actBasedClean = 0;
    size_t propConflDepthBasedClean = 0;

    //Clause Cleaning
    Data removed;
    Data remain;
};

}

#endif //__CLEANINGSTATS_H__
