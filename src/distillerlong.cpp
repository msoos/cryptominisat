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

#include "distillerlong.h"
#include "clausecleaner.h"
#include "time_mem.h"
#include "solver.h"
#include "watchalgos.h"
#include "clauseallocator.h"
#include "sqlstats.h"

#include <iomanip>
using namespace CMSat;
using std::cout;
using std::endl;

//#define DEBUG_STAMPING

#ifdef VERBOSE_DEBUG
#define VERBOSE_SUBSUME_NONEXIST
#endif

//#define VERBOSE_SUBSUME_NONEXIST

DistillerLong::DistillerLong(Solver* _solver) :
    solver(_solver)
{}

bool DistillerLong::distill(uint32_t _queueByBy)
{
    assert(solver->ok);
    numCalls++;
    queueByBy = _queueByBy;
    Stats other;


    runStats.clear();
    //solver->clauseCleaner->clean_clauses(solver->longIrredCls);
    if (!distill_long_cls_all(solver->longIrredCls)) {
        goto end;
    }
    other = runStats;

    runStats.clear();
    //solver->clauseCleaner->clean_clauses(solver->longRedCls[0]);
    if (!distill_long_cls_all(solver->longRedCls[0])) {
        goto end;
    }

end:
    runStats += other;
    globalStats += runStats;
    if (solver->conf.verbosity) {
        if (solver->conf.verbosity >= 3)
            runStats.print(solver->nVars());
        else
            runStats.print_short(solver);
    }
    runStats.clear();

    return solver->ok;
}

struct ClauseSizeSorterInv
{
    ClauseSizeSorterInv(const ClauseAllocator& _cl_alloc) :
        cl_alloc(_cl_alloc)
    {}

    const ClauseAllocator& cl_alloc;

    bool operator()(const ClOffset off1, const ClOffset off2) const
    {
        const Clause* cl1 = cl_alloc.ptr(off1);
        const Clause* cl2 = cl_alloc.ptr(off2);

        return cl1->size() > cl2->size();
    }
};

bool DistillerLong::go_through_clauses(
    vector<ClOffset>& cls
) {
    bool time_out = false;
    vector<ClOffset>::iterator i, j;
    i = j = cls.begin();
    for (vector<ClOffset>::iterator end = cls.end()
        ; i != end
        ; i++
    ) {
        //Check if we are in state where we only copy offsets around
        if (time_out || !solver->ok) {
            *j++ = *i;
            continue;
        }

        //if done enough, stop doing it
        if (solver->propStats.bogoProps-oldBogoProps + extraTime >= maxNumProps
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

        //Get pointer
        ClOffset offset = *i;
        Clause& cl = *solver->cl_alloc.ptr(offset);
        //Time to dereference
        extraTime += 5;

        //If we already tried this clause, then move to next
        if (cl.getdistilled()) {
            *j++ = *i;
            continue;
        };
        cl.set_distilled(true);

        extraTime += cl.size();
        runStats.checkedClauses++;

        //Sanity check
        assert(cl.size() > 2);
        //assert(!cl.red());

        //Copy literals
        lits.resize(cl.size());
        std::copy(cl.begin(), cl.end(), lits.begin());

        //Try to distill clause
        ClOffset offset2 = try_distill_clause_and_return_new(
            offset
            , cl.red()
            , cl.stats
        );

        if (offset2 != CL_OFFSET_MAX) {
            *j++ = offset2;
        }
    }
    cls.resize(cls.size()- (i-j));

    //Didn't time out, so it went through the whole list. Reset distill for all.
//     if (!time_out) {
//         for (vector<ClOffset>::const_iterator
//             it = cls.begin(), end = cls.end()
//             ; it != end
//             ; ++it
//         ) {
//             Clause* cl = solver->cl_alloc.ptr(*it);
//             cl->set_distilled(false);
//         }
//     }

    return time_out;
}

bool DistillerLong::distill_long_cls_all(vector<ClOffset>& offs)
{
    assert(solver->ok);
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

    if (solver->litStats.irredLits + solver->litStats.redLits < (500ULL*1000ULL))
        maxNumProps *=2;

    if (numCalls > 8
        && (solver->litStats.irredLits + solver->litStats.redLits < 4000000)
        //&& (solver->longIrredCls.size() < 50000)
    ) {
        queueByBy = 1;
    }

    //stats setup
    extraTime = 0;
    oldBogoProps = solver->propStats.bogoProps;
    runStats.potentialClauses += offs.size();
    runStats.numCalled += 1;
    uint64_t origLitRem = runStats.numLitsRem;
    uint64_t origClShorten = runStats.numClShorten;

    /*std::sort(offs.begin()
        , offs.end()
        , ClauseSizeSorterInv(solver->cl_alloc)
    );*/

    bool time_out = go_through_clauses(offs);

    const double time_used = cpuTime() - myTime;
    const double time_remain = float_div(solver->propStats.bogoProps-oldBogoProps + extraTime, maxNumProps);
    if (solver->conf.verbosity) {
        cout << "c [distill] long cls"
        << " tried: " << runStats.checkedClauses << "/" << offs.size()
        << " cl-r:" << runStats.numClShorten- origClShorten
        << " lit-r:" << runStats.numLitsRem - origLitRem
        << solver->conf.print_times(time_used, time_out, time_remain)
        << endl;
    }
    if (solver->sqlStats) {
        solver->sqlStats->time_passed(
            solver
            , "distill long cls"
            , time_used
            , time_out
            , time_remain
        );
    }

    //Update stats
    runStats.time_used += cpuTime() - myTime;
    runStats.zeroDepthAssigns += solver->trail_size() - origTrailSize;

    return solver->ok;
}

ClOffset DistillerLong::try_distill_clause_and_return_new(
    ClOffset offset
    , const bool red
    , const ClauseStats& stats
) {
    #ifdef DRAT_DEBUG
    if (solver->conf.verbosity >= 6) {
        cout << "Trying to distill clause:" << lits << endl;
    }
    #endif

    //Try to enqueue the literals in 'queueByBy' amounts and see if we fail
    uint32_t new_sz = 0;
    solver->new_decision_level();
    bool early_abort = false;
    for (size_t i = 0, sz = lits.size()-1; i < sz; i++) {
        const Lit lit = lits[i];
        lbool val = solver->value(lit);
        if (val == l_Undef) {
            solver->enqueue(~lit);
            new_sz++;

            extraTime += 5;
            if (!solver->propagate<true>().isNULL()) {
                early_abort = true;
                break;
            }
        } else if (val == l_False) {
            //Record that there is no use for this literal
            solver->seen[lit.toInt()] = 1;
            //new_sz++;
        } else {
            //val is l_True --> can't do much
            new_sz++;
        }
    }
    solver->cancelUntil<false>(0);
    assert(solver->ok);
    if (!early_abort) {
        new_sz++;
    }

    if (new_sz < lits.size()) {
        runStats.numClShorten++;
        extraTime += 20;
        runStats.numLitsRem += lits.size() - new_sz;
        if (solver->conf.verbosity >= 5) {
            cout
            << "c Distillation branch effective." << endl
            << "c --> orig clause:" << *solver->cl_alloc.ptr(offset) << endl
            << "c --> orig size:" << lits.size() << endl
            << "c --> new size:" << new_sz << endl;
        }

        //Remove useless literals from 'lits'
        vector<Lit>::iterator i = lits.begin();
        vector<Lit>::iterator j = lits.begin();
        for(vector<Lit>::iterator end = lits.end()
            ; i != end
            ; i++
        ) {
            if (solver->seen[i->toInt()] == 0) {
                *j++ = *i;
            } else {
                //zero out 'seen'
                solver->seen[i->toInt()] = 0;
            }
        }
        lits.resize(new_sz);

        //Make new clause
        Clause *cl2 = solver->add_clause_int(lits, red, stats);

        //Detach and free old clause
        solver->detachClause(offset);
        solver->cl_alloc.clauseFree(offset);

        if (cl2 != NULL) {
            cl2->set_distilled(true);
            return solver->cl_alloc.get_offset(cl2);
        } else {
            //it became a bin/unit/zero
            return CL_OFFSET_MAX;
        }
    } else {
        //couldn't simplify the clause
        return offset;
    }
}

DistillerLong::Stats& DistillerLong::Stats::operator+=(const Stats& other)
{
    time_used += other.time_used;
    timeOut += other.timeOut;
    zeroDepthAssigns += other.zeroDepthAssigns;
    numClShorten += other.numClShorten;
    numLitsRem += other.numLitsRem;
    checkedClauses += other.checkedClauses;
    potentialClauses += other.potentialClauses;
    numCalled += other.numCalled;

    return *this;
}

void DistillerLong::Stats::print_short(const Solver* _solver) const
{
    cout
    << "c [distill] long"
    << " useful: "<< numClShorten
    << "/" << checkedClauses << "/" << potentialClauses
    << " lits-rem: " << numLitsRem
    << " 0-depth-assigns: " << zeroDepthAssigns
    << _solver->conf.print_times(time_used, timeOut)
    << endl;
}

void DistillerLong::Stats::print(const size_t nVars) const
{
    cout << "c -------- DISTILL STATS --------" << endl;
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

double DistillerLong::mem_used() const
{
    double mem_used = sizeof(DistillerLong);
    mem_used += lits.size()*sizeof(Lit);
    return mem_used;
}
