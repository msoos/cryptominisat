#ifndef __CLAUSEUSAGESTATS_H__
#define __CLAUSEUSAGESTATS_H__

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

void ClauseUsageStats::print() const
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


}

#endif //__CLAUSEUSAGESTATS_H__
