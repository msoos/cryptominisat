#include "clauseusagestats.h"

#include <iostream>
#include <iomanip>

using std::cout;
using std::endl;

using namespace CMSat;

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
