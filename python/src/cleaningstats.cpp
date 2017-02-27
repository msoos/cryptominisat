/******************************************
Copyright (c) 2016, Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***********************************************/

#include "cleaningstats.h"
#include "solvertypes.h"

using namespace CMSat;

#include <iostream>
#include "solver.h"
using std::cout;
using std::endl;

void CleaningStats::print(const double total_cpu_time) const
{
    cout << "c ------ REDUCEDB STATS ---------" << endl;

    if (total_cpu_time != 0.0) {
        print_stats_line("c reduceDB time"
            , cpu_time
            , stats_line_percent(cpu_time, total_cpu_time)
            , "% time"
        );
    } else {
        print_stats_line("c reduceDB time"
            , cpu_time
        );
    }

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
        , ratio_for_stat(removed.lits, removed.num)
    );
    print_stats_line("c cleaned avg glue"
        , ratio_for_stat(removed.glue, removed.num)
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
        , ratio_for_stat(remain.lits, remain.num)
    );
    print_stats_line("c remain avg glue"
        , ratio_for_stat(remain.glue, remain.num)
    );

    cout << "c ------ REDUCEDB STATS END ---------" << endl;
}

void CleaningStats::print_short(const Solver* solver) const
{
    cout
    << "c [DBclean]"
    << " remv'd "; print_value_kilo_mega(removed.num);

    cout
    << " avgGlue " << std::fixed << std::setprecision(2)
    << float_div(removed.glue, removed.num)

    << " avgSize "
    << std::fixed << std::setprecision(2)
    << float_div(removed.lits, removed.num)
    << endl;

    cout
    << "c [DBclean]"
    << " remain "; print_value_kilo_mega(remain.num);

    cout
    << " avgGlue " << std::fixed << std::setprecision(2)
    << float_div(remain.glue, remain.num)

    << " avgSize " << std::fixed << std::setprecision(2)
    << float_div(remain.lits, remain.num)

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
