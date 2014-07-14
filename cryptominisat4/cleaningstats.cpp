#include "cleaningstats.h"
#include "solvertypes.h"

using namespace CMSat;

#include <iostream>
#include "solver.h"
using std::cout;
using std::endl;

void CleaningStats::print(const size_t nbReduceDB) const
{
    cout << "c ------ CLEANING STATS ---------" << endl;

    //Types of clean
    printStatsLine("c clean by glue"
        , glueBasedClean
        , stats_line_percent(glueBasedClean, nbReduceDB)
        , "% cleans"
    );
    printStatsLine("c clean by size"
        , sizeBasedClean
        , stats_line_percent(sizeBasedClean, nbReduceDB)
        , "% cleans"
    );
    printStatsLine("c clean by prop&confl"
        , propConflBasedClean
        , stats_line_percent(propConflBasedClean,nbReduceDB)
        , "% cleans"
    );
    printStatsLine("c clean by activities"
        , actBasedClean
        , stats_line_percent(actBasedClean,nbReduceDB)
        , "% cleans"
    );
    printStatsLine("c clean by prop&confl / avg. depth"
        , propConflDepthBasedClean
        , stats_line_percent(propConflDepthBasedClean,nbReduceDB)
        , "% cleans"
    );

    //--- Actual clean --

    //-->CLEAN
    printStatsLine("c cleaned cls"
        , removed.num
        , stats_line_percent(removed.num, origNumClauses)
        , "% long redundant clauses"
    );
    printStatsLine("c cleaned lits"
        , removed.lits
        , stats_line_percent(removed.lits, origNumLits)
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
        , stats_line_percent(remain.num, origNumClauses)
        , "% long redundant clauses"
    );
    printStatsLine("c remain lits"
        , remain.lits
        , stats_line_percent(remain.lits, origNumLits)
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

void CleaningStats::printShort(const Solver* solver) const
{
    //Pre-clean
    cout
    << "c [DBclean]"
    << " clean type is " << getNameOfCleanType(clauseCleaningType)
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

    << solver->conf.print_times(cpu_time)
    << endl;
}

CleaningStats& CleaningStats::operator+=(const CleaningStats& other)
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
    propConflDepthBasedClean += other.propConflDepthBasedClean;

    //Clause Cleaning data
    removed += other.removed;
    remain += other.remain;

    return *this;
}
