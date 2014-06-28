#ifndef __CLAUSEUSAGESTATS_H__
#define __CLAUSEUSAGESTATS_H__

#include <cstdint>
#include "clause.h"

namespace CMSat {

struct ClauseUsageStats
{
    uint64_t sumPropAndConfl() const
    {
        return sumProp + sumConfl;
    }

    uint64_t num = 0;
    uint64_t sumProp = 0;
    uint64_t sumConfl = 0;
    uint64_t sumLitVisited = 0;
    uint64_t sumLookedAt = 0;
    uint64_t sumUsedUIP = 0;

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
        sumProp += cl.stats.propagations_made;
        sumConfl += cl.stats.conflicts_made;
        #ifdef STATS_NEEDED
        sumLitVisited += cl.stats.visited_literals;
        sumLookedAt += cl.stats.clause_looked_at;
        #endif
        sumUsedUIP += cl.stats.used_for_uip_creation;
    }
    void print() const;
};

}

#endif //__CLAUSEUSAGESTATS_H__
