/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

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

#include <iomanip>
#include <algorithm>
#include <random>

#include "distillerbin.h"
#include "clausecleaner.h"
#include "time_mem.h"
#include "solver.h"
#include "watchalgos.h"
#include "clauseallocator.h"
#include "sqlstats.h"


using namespace CMSat;
using std::cout;
using std::endl;

#ifdef VERBOSE_DEBUG
#define VERBOSE_SUBSUME_NONEXIST
#endif

//#define VERBOSE_SUBSUME_NONEXIST

DistillerBin::DistillerBin(Solver* _solver) :
    solver(_solver)
{}

bool DistillerBin::distill()
{
    assert(solver->ok);
    numCalls++;
    runStats.clear();

    if (!distill_bin_cls_all(1.0, true, false)) {
        goto end;
    }

end:
    globalStats += runStats;
    if (solver->conf.verbosity) {
        if (solver->conf.verbosity >= 3)
            runStats.print(solver->nVars());
        else
            runStats.print_short(solver);
    }
    runStats.clear();

    return solver->okay();
}


bool DistillerBin::distill_bin_cls_all(
    double time_mult,
    const bool also_remove,
    const bool red
) {
    assert(solver->ok);
    if (time_mult == 0.0) {
        return solver->okay();
    }

    if (solver->conf.verbosity >= 6) {
        cout
        << "c Doing distillation branch for long clauses"
        << endl;
    }

    double myTime = cpuTime();
    const size_t origTrailSize = solver->trail_size();

    //Time-limiting
    maxNumProps =
        solver->conf.distill_long_cls_time_limitM*1000LL*1000ULL
        *solver->conf.global_timeout_multiplier;

    if (solver->litStats.irredLits + solver->litStats.redLits <
            (500ULL*1000ULL*solver->conf.var_and_mem_out_mult)
    ) {
        maxNumProps *=2;
    }
    maxNumProps *= time_mult;
    orig_maxNumProps = maxNumProps;

    //stats setup
    oldBogoProps = solver->propStats.bogoProps;
    uint32_t potential_size = red ? solver->binTri.redBins : solver->binTri.irredBins;
    runStats.potentialClauses += potential_size;
    runStats.numCalled += 1;

    bool time_out;
    vector<Lit> todo;
    for(uint32_t i = 0; i < solver->nVars()*2; i ++) {
        const Lit lit = Lit::toLit(i);
        todo.push_back(lit);
    }
    std::shuffle(todo.begin(), todo.end(), std::default_random_engine(solver->mtrand.randInt()));
    for(const auto& lit: todo) {
        time_out = go_through_bins(lit, also_remove, red);
        if (time_out) {
            break;
        }
    }

    const double time_used = cpuTime() - myTime;
    const double time_remain = float_div(
        maxNumProps - ((int64_t)solver->propStats.bogoProps-(int64_t)oldBogoProps),
        orig_maxNumProps);
    if (solver->conf.verbosity >= 3) {
        cout << "c [distill-bin] cls"
        << " tried: " << runStats.checkedClauses << "/" << potential_size
        << endl;
    }
    if (solver->sqlStats) {
        solver->sqlStats->time_passed(
            solver
            , "distill bin cls"
            , time_used
            , time_out
            , time_remain
        );
    }

    //Update stats
    runStats.time_used += time_used;
    runStats.zeroDepthAssigns += solver->trail_size() - origTrailSize;

    return solver->okay();
}

bool DistillerBin::go_through_bins(
    const Lit lit1,
    bool also_remove,
    const bool red
) {
    bool time_out = false;
    solver->watches[lit1].copyTo(tmp);

    for (const auto& w: tmp) {
        if (time_out || !solver->ok || //Check if we are in state where we only copy offsets around
            !w.isBin() || lit1 > w.lit2() || //check if we are bin and don't do it 2x
            w.red() != red) // only the ones we want
        {
            continue;
        }

        //if done enough, stop doing it
        if ((int64_t)solver->propStats.bogoProps-(int64_t)oldBogoProps >= maxNumProps
            || solver->must_interrupt_asap()
        ) {
            if (solver->conf.verbosity >= 3) {
                cout
                << "c Need to finish distillation -- ran out of prop (=allocated time)"
                << endl;
            }
            runStats.timeOut++;
            time_out = true;
        }
        runStats.checkedClauses++;
        const Lit lit2 = w.lit2();

        //we will detach the clause no matter what
        maxNumProps -= solver->watches[lit1].size();
        maxNumProps -= solver->watches[lit2].size();
        maxNumProps -= 2;

        if (solver->value(lit1) == l_True || solver->value(lit2) == l_True) {
            solver->detach_bin_clause(lit1, lit2, w.red());
            continue;
        }

        //Try to distill clause
        try_distill_bin(lit1, lit2, w.red(), also_remove);
    }

    return time_out;
}

inline
void DistillerBin::myprop(
    const bool red,
    const bool also_remove,
    PropBy& confl)
{
    if (!red && also_remove) {
        //ONLY propagate on irred
        confl = solver->propagate<true, false, true>();
    } else {
        //Normal propagation, on all clauses
        confl = solver->propagate<true, true, true>();
    }
}


void DistillerBin::try_distill_bin(
    const Lit lit1,
    const Lit lit2,
    const bool red,
    bool also_remove
) {
    assert(solver->prop_at_head());
    assert(solver->decisionLevel() == 0);
    #ifdef DRAT_DEBUG
    if (solver->conf.verbosity >= 6) {
        cout << "Trying to distill clause:" << lits << endl;
    }
    #endif

    //Disable this clause
    findWatchedOfBin(solver->watches, lit1, lit2, red).mark_bin_cl();
    findWatchedOfBin(solver->watches, lit2, lit1, red).mark_bin_cl();
    if (red) {
        assert(!also_remove);
    }

    solver->new_decision_level();
    PropBy confl;
    solver->enqueue<true>(~lit1);
    myprop(red, also_remove, confl);

    if (confl.isNULL()) {
        if (solver->value(lit2) == l_True) {
            //clause can be removed
            confl = PropBy(Lit(0, false), false);
        }

        if (solver->value(lit2) == l_False) {
            //Unit derived
            solver->cancelUntil<false, true>(0);
            vector<Lit> x(1);
            x[0] = lit1;
            solver->add_clause_int(x);
            solver->detach_bin_clause(lit1, lit2, red);
            runStats.numClShorten++;
            return;
        }

        if (solver->value(lit2) == l_Undef) {
            solver->enqueue<true>(~lit2);
            myprop(red, also_remove, confl);
        }
    }

    if (also_remove && !red && !confl.isNULL()) {
        solver->cancelUntil<false, true>(0);
        solver->detach_bin_clause(lit1, lit2, red);
        runStats.clRemoved++;
        return;
    }

    //Nothing happened
    solver->cancelUntil<false, true>(0);
    auto &w1 = findWatchedOfBin(solver->watches, lit1, lit2, red);
    assert(w1.bin_cl_marked());
    w1.unmark_bin_cl();

    auto &w2 = findWatchedOfBin(solver->watches, lit2, lit1, red);
    assert(w2.bin_cl_marked());
    w2.unmark_bin_cl();
}

DistillerBin::Stats& DistillerBin::Stats::operator+=(const Stats& other)
{
    time_used += other.time_used;
    timeOut += other.timeOut;
    zeroDepthAssigns += other.zeroDepthAssigns;
    numClShorten += other.numClShorten;
    numLitsRem += other.numLitsRem;
    checkedClauses += other.checkedClauses;
    potentialClauses += other.potentialClauses;
    numCalled += other.numCalled;
    clRemoved += other.clRemoved;

    return *this;
}

void DistillerBin::Stats::print_short(const Solver* _solver) const
{
    cout
    << "c [distill-bin]"
    << " useful: "<< numClShorten+clRemoved
    << "/" << checkedClauses << "/" << potentialClauses
    << " lits-rem: " << numLitsRem
    << " cl-rem: " << clRemoved
    << " 0-depth-assigns: " << zeroDepthAssigns
    << _solver->conf.print_times(time_used, timeOut)
    << endl;
}

void DistillerBin::Stats::print(const size_t nVars) const
{
    cout << "c -------- DISTILL-BIN STATS --------" << endl;
    print_stats_line("c time"
        , time_used
        , ratio_for_stat(time_used, numCalled)
        , "per call"
    );

    print_stats_line("c timed out"
        , timeOut
        , stats_line_percent(timeOut, numCalled)
        , "% of calls"
    );

    print_stats_line("c distill/checked/potential"
        , numClShorten
        , checkedClauses
        , potentialClauses
    );

    print_stats_line("c lits-rem",
        numLitsRem
    );
    print_stats_line("c 0-depth-assigns",
        zeroDepthAssigns
        , stats_line_percent(zeroDepthAssigns, nVars)
        , "% of vars"
    );
    cout << "c -------- DISTILL STATS END --------" << endl;
}

double DistillerBin::mem_used() const
{
    double mem_used = sizeof(DistillerBin);
    mem_used += lits.size()*sizeof(Lit);
    return mem_used;
}
