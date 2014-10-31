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

    //-->CLEAN
    print_stats_line("c cleaned cls"
        , removed.num
        , stats_line_percent(removed.num, origNumClauses)
        , "% long redundant clauses"
    );
    print_stats_line("c cleaned lits"
        , removed.lits
        , stats_line_percent(removed.lits, origNumLits)
        , "% long red lits"
    );
    print_stats_line("c cleaned cl avg size"
        , (double)removed.lits/(double)removed.num
    );
    print_stats_line("c cleaned avg glue"
        , (double)removed.glue/(double)removed.num
    );

    //--> REMAIN
    print_stats_line("c remain cls"
        , remain.num
        , stats_line_percent(remain.num, origNumClauses)
        , "% long redundant clauses"
    );
    print_stats_line("c remain lits"
        , remain.lits
        , stats_line_percent(remain.lits, origNumLits)
        , "% long red lits"
    );
    print_stats_line("c remain cl avg size"
        , (double)remain.lits/(double)remain.num
    );
    print_stats_line("c remain avg glue"
        , (double)remain.glue/(double)remain.num
    );

    cout << "c ------ CLEANING STATS END ---------" << endl;
}

void CleaningStats::print_short(const Solver* solver) const
{
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

    //Clause Cleaning data
    removed += other.removed;
    remain += other.remain;

    return *this;
}
