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

#include "distillerlitrem.h"
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

#ifdef VERBOSE_DEBUG
#define VERBOSE_SUBSUME_NONEXIST
#endif

//#define VERBOSE_SUBSUME_NONEXIST

DistillerLitRem::DistillerLitRem(Solver* _solver) :
    solver(_solver)
{}

bool DistillerLitRem::distill_lit_rem()
{
    assert(solver->ok);
    numCalls++;
    runStats.clear();


    if (!solver->remove_and_clean_all()) {
        goto end;
    }
    if (!distill_long_cls_all(solver->longIrredCls, 1)) {
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

struct ClauseSizeSorterLargestFirst
{
    ClauseSizeSorterLargestFirst(const ClauseAllocator& _cl_alloc) :
        cl_alloc(_cl_alloc)
    {}

    const ClauseAllocator& cl_alloc;

    bool operator()(const ClOffset off1, const ClOffset off2) const
    {
        const Clause* cl1 = cl_alloc.ptr(off1);
        const Clause* cl2 = cl_alloc.ptr(off2);

        //Correct order if c1's size is larger
        return cl1->size() > cl2->size();
    }
};

bool DistillerLitRem::go_through_clauses(
    vector<ClOffset>& cls,
    uint32_t at
) {
    double myTime = cpuTime();
    bool time_out = false;
    uint32_t skipped = 0;
    uint32_t tried = 0;
    vector<ClOffset>::iterator i, j;
    i = j = cls.begin();
    for (vector<ClOffset>::iterator end = cls.end()
        ; i != end
        ; ++i
    ) {
        //Check if we are in state where we only copy offsets around
        if (time_out || !solver->ok) {
            *j++ = *i;
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

        //Get pointer
        ClOffset offset = *i;
        ClOffset offset2;
        Clause& cl = *solver->cl_alloc.ptr(offset);
        if (cl.size() <= at) {
            *j++ = *i;
            continue;
        }

        if (cl.used_in_xor() &&
            solver->conf.force_preserve_xors
        ) {
            *j++ = *i;
            continue;
        }

        //Time to dereference
        maxNumProps -= 5;

        if (cl._xor_is_detached

            //If it's a redundant that's not very good, let's not distill it
            || (
#ifdef FINAL_PREDICTOR
                solver->conf.pred_distill_only_smallgue &&
#else
                false &&
#endif
                cl.red() &&
                cl.stats.glue > 3)
        ) {
            skipped++;
            *j++ = *i;
            continue;
        }
        runStats.checkedClauses++;
        assert(cl.size() > 2);

        //we will detach the clause no matter what
        maxNumProps -= solver->watches[cl[0]].size();
        maxNumProps -= solver->watches[cl[1]].size();

        maxNumProps -= cl.size();
        if (solver->satisfied(cl)) {
            solver->detachClause(cl);
            solver->free_cl(&cl);
            continue;
        }

        //Try to distill clause
        tried++;
        offset2 = try_distill_clause_and_return_new(
            offset
            , &cl.stats
            , at
        );

        if (offset2 != CL_OFFSET_MAX) {
            *j++ = offset2;
        }
    }
    cls.resize(cls.size()- (i-j));

    runStats.time_used += cpuTime() - myTime;
    return time_out;
}

bool DistillerLitRem::distill_long_cls_all(
    vector<ClOffset>& offs
    , double time_mult
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

    const size_t origTrailSize = solver->trail_size();

    //Time-limiting
    maxNumProps =
        5*1000LL*1000ULL
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
    runStats.potentialClauses += offs.size();
    runStats.numCalled += 1;

    bool time_out = false;
    for(uint32_t i = 0; i < 10 && !time_out; i++) {
        uint32_t prev_cls_tried = runStats.cls_tried;
        time_out = go_through_clauses(offs, i);
        if (solver->conf.verbosity >= 2) {
            runStats.print_short(solver);
        }

        //Max clause size reached
        if (runStats.cls_tried == prev_cls_tried) {
            break;
        }
    }

    const double time_remain = float_div(
        maxNumProps - ((int64_t)solver->propStats.bogoProps-(int64_t)oldBogoProps),
        orig_maxNumProps);
    if (solver->conf.verbosity >= 3) {
        cout << "c [distill-litrem] "
        << " tried: " << runStats.checkedClauses << "/" << offs.size()
        << endl;
    }
    if (solver->sqlStats) {
        solver->sqlStats->time_passed(
            solver
            , "distill-litrem"
            , runStats.time_used
            , time_out
            , time_remain
        );
    }


    //Update stats
    runStats.zeroDepthAssigns += solver->trail_size() - origTrailSize;

    return solver->okay();
}

ClOffset DistillerLitRem::try_distill_clause_and_return_new(
    ClOffset offset
    , const ClauseStats* const stats
    , const uint32_t at
) {
    assert(solver->prop_at_head());
    assert(solver->decisionLevel() == 0);
    const size_t origTrailSize = solver->trail_size();
    runStats.cls_tried++;

    Clause& cl = *solver->cl_alloc.ptr(offset);
    const bool red = cl.red();

    uint32_t orig_size = cl.size();
    assert(cl.size() > at);
    Lit torem = cl[at];
    //if (solver->conf.verbosity >= 6) {
    //    cout << "Trying to rem lit: " << torem << " from clause:" << cl << endl;
    //}

    solver->new_decision_level();
    for (const auto& l: cl) {
        Lit lit = l;
        if (lit == torem) {
            lit = ~lit;
        }
        //cout << "Enq: " << ~lit << endl;
        solver->enqueue<true>(~lit);
    }
    assert(solver->ok);
    PropBy confl = solver->propagate<true>();
    solver->cancelUntil<false, true>(0);

     //Couldn't remove literal
    if (confl.isNULL()) {
        return offset;
    }

    //Managed to remove literal
    lits.clear();
    for(const auto& l: cl) {
        if (l != torem) {
            lits.push_back(l);
        }
    }
//     cout
//     << "Failed"
//     << " confl.isNULL(): " << confl.isNULL()
//     << " i: " << i
//     << " at: " << at
//     << " cl before: " << cl
//     << " cl after: " << lits
//     << endl;

    //We can remove the literal
    (*solver->frat) << deldelay << cl << fin;
    solver->detachClause(cl, false);
    runStats.numLitsRem += orig_size - lits.size();
    runStats.numClShorten++;

    // we have to copy because the re-alloc can invalidate the data
    ClauseStats backup_stats(*stats);
    // new clause will inherit this clause's ID
    // so let's set this to 0, this way, when we free() it, it won't be
    // deleted as per cl_last_in_solver
    solver->free_cl(offset, false);
    Clause *cl2 = solver->add_clause_int(lits, red, &backup_stats);
    (*solver->frat) << findelay;
    assert(solver->trail_size() == origTrailSize);

    if (cl2 != NULL) {
        return solver->cl_alloc.get_offset(cl2);
    } else {
        #ifdef STATS_NEEDED
        solver->stats_del_cl(offset);
        #endif
        //it became a bin/unit/zero
        return CL_OFFSET_MAX;
    }
}

DistillerLitRem::Stats& DistillerLitRem::Stats::operator+=(const Stats& other)
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

void DistillerLitRem::Stats::print_short(const Solver* _solver) const
{
    cout
    << "c [distill-litrem]"
    << " useful: "<< numClShorten
    << "/" << checkedClauses << "/" << potentialClauses
    << " lits-rem: " << numLitsRem
    << " 0-depth-assigns: " << zeroDepthAssigns
    << _solver->conf.print_times(time_used, timeOut)
    << endl;
}

void DistillerLitRem::Stats::print(const size_t nVars) const
{
    cout << "c -------- DISTILL-LITREM STATS --------" << endl;
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

double DistillerLitRem::mem_used() const
{
    double mem_used = sizeof(DistillerLitRem);
    mem_used += lits.size()*sizeof(Lit);
    return mem_used;
}
