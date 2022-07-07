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
    *solver->frat << __PRETTY_FUNCTION__ << " start\n";

    if (!distill_bin_cls_all(1.0)) {
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
    *solver->frat << __PRETTY_FUNCTION__ << " end\n";

    return solver->okay();
}


bool DistillerBin::distill_bin_cls_all(
    double time_mult
) {
    assert(solver->ok);
    if (time_mult == 0.0) return solver->okay();
    verb_print(6, "Doing distillation branch for long clauses");

    double myTime = cpuTime();
    const size_t origTrailSize = solver->trail_size();
    *solver->frat << __PRETTY_FUNCTION__ << " start\n";

    //Time-limiting
    maxNumProps =
        solver->conf.distill_long_cls_time_limitM*200LL*1000ULL
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
    uint32_t potential_size = solver->binTri.irredBins;
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
        time_out = go_through_bins(lit);
        if (time_out || !solver->okay()) {
            break;
        }
    }

    const double time_used = cpuTime() - myTime;
    const double time_remain = float_div(
        maxNumProps - ((int64_t)solver->propStats.bogoProps-(int64_t)oldBogoProps),
        orig_maxNumProps);
    if (solver->conf.verbosity >= 2) {
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
    *solver->frat << __PRETTY_FUNCTION__ << " end\n";

    //Update stats
    runStats.time_used += time_used;
    runStats.zeroDepthAssigns += solver->trail_size() - origTrailSize;

    return solver->okay();
}

bool DistillerBin::go_through_bins(
    const Lit lit1
) {
    solver->watches[lit1].copyTo(tmp);

    for (const auto& w: tmp) {
        if (!w.isBin() || //check if we are bin
            lit1 > w.lit2() || // don't do it 2x
            w.red()) // only irred
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
            return true;
        }
        runStats.checkedClauses++;
        const Lit lit2 = w.lit2();

        //we will detach the clause no matter what
        maxNumProps -= solver->watches[lit1].size();
        maxNumProps -= solver->watches[lit2].size();
        maxNumProps -= 2;

        if (solver->value(lit1) == l_True || solver->value(lit2) == l_True) {
            solver->detach_bin_clause(lit1, lit2, w.red(), w.get_ID());
            (*solver->frat) << del << w.get_ID() << lit1 << lit2 << fin;
            continue;
        }

        //Try to distill clause
        if (!try_distill_bin(lit1, lit2, w)) {
            //UNSAT
            return false;
        }
    }

    return false;
}

bool DistillerBin::try_distill_bin(
    Lit lit1,
    Lit lit2,
    const Watched& w
) {
    assert(solver->okay());
    assert(solver->prop_at_head());
    assert(solver->decisionLevel() == 0);
    #ifdef FRAT_DEBUG
    if (solver->conf.verbosity >= 6) {
        cout << "Trying to distill clause:" << lits << endl;
    }
    #endif

    //Try different ordering
    if (solver->mtrand.randInt(1) == 1) {
        std::swap(lit1, lit2);
    }

    //Disable this clause
    findWatchedOfBin(solver->watches, lit1, lit2, false, w.get_ID()).mark_bin_cl();
    findWatchedOfBin(solver->watches, lit2, lit1, false, w.get_ID()).mark_bin_cl();

    solver->new_decision_level();
    PropBy confl;
    solver->enqueue<true>(~lit1);
    confl = solver->propagate<true, false, true>();

    if (confl.isNULL()) {
        if (solver->value(lit2) == l_True) {
            //clause can be removed
            confl = PropBy(ClOffset(0));
        } else if (solver->value(lit2) == l_False) {
            //Unit derived
            solver->cancelUntil<false, true>(0);
            vector<Lit> x(1);
            x[0] = lit1;
            solver->add_clause_int(x);
            solver->detach_bin_clause(lit1, lit2, false, w.get_ID());
            (*solver->frat) << del << w.get_ID() << lit1 << lit2 << fin;
            runStats.numClShorten++;
            return solver->okay();
        } else if (solver->value(lit2) == l_Undef) {
            solver->enqueue<true>(~lit2);
            confl = solver->propagate<true, false, true>();
        }
    }

    if (!confl.isNULL()) {
        solver->cancelUntil<false, true>(0);
        solver->detach_bin_clause(lit1, lit2, false, w.get_ID());
        (*solver->frat) << del << w.get_ID() << lit1 << lit2 << fin;
        runStats.clRemoved++;
        return true;
    }

    //Nothing happened
    solver->cancelUntil<false, true>(0);
    auto &w1 = findWatchedOfBin(solver->watches, lit1, lit2, false, w.get_ID());
    assert(w1.bin_cl_marked());
    w1.unmark_bin_cl();

    auto &w2 = findWatchedOfBin(solver->watches, lit2, lit1, false, w.get_ID());
    assert(w2.bin_cl_marked());
    w2.unmark_bin_cl();

    return true;
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
