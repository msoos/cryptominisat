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

#ifdef VERBOSE_DEBUG
#define VERBOSE_SUBSUME_NONEXIST
#endif

//#define VERBOSE_SUBSUME_NONEXIST

DistillerLong::DistillerLong(Solver* _solver) :
    solver(_solver)
{}

bool DistillerLong::distill(const bool red, bool fullstats)
{
    assert(solver->ok);
    numCalls++;
    Stats other;


    if (!red) {
        runStats.clear();
        if (!distill_long_cls_all(solver->longIrredCls, 1)) {
            goto end;
        }
        other = runStats;
    } else {
        runStats.clear();
        if (!distill_long_cls_all(solver->longRedCls[0], 10.0)) {
            goto end;
        }
        runStats.clear();
        if (!distill_long_cls_all(solver->longRedCls[1], solver->conf.distill_red_tier1_ratio)) {
            goto end;
        }
    }

end:
    runStats += other;
    globalStats += runStats;
    if (solver->conf.verbosity && fullstats) {
        if (solver->conf.verbosity >= 3)
            runStats.print(solver->nVars());
        else
            runStats.print_short(solver);
    }
    runStats.clear();

    return solver->okay();
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
        if (cl.used_in_xor() &&
            solver->conf.force_preserve_xors
        ) {
            offset2 = offset;
            goto copy;
        }

        //Time to dereference
        maxNumProps -= 5;

        //If we already tried this clause, then move to next
        if (cl.getdistilled() || cl._xor_is_detached) {
            *j++ = *i;
            continue;
        }
        cl.set_distilled(true);
        runStats.checkedClauses++;
        assert(cl.size() > 2);

        //we will detach the clause no matter what
        maxNumProps -= solver->watches[cl[0]].size();
        maxNumProps -= solver->watches[cl[1]].size();

        maxNumProps -= cl.size();
        if (solver->satisfied_cl(cl)) {
            solver->detachClause(cl);
            solver->free_cl(&cl);
            continue;
        }

        //Try to distill clause
        offset2 = try_distill_clause_and_return_new(
            offset
            , cl.red()
            , cl.stats
        );

        copy:
        if (offset2 != CL_OFFSET_MAX) {
            *j++ = offset2;
        }
    }
    cls.resize(cls.size()- (i-j));

    return time_out;
}

bool DistillerLong::distill_long_cls_all(
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
    runStats.potentialClauses += offs.size();
    runStats.numCalled += 1;

    /*std::sort(offs.begin()
        , offs.end()
        , ClauseSizeSorterInv(solver->cl_alloc)
    );*/

    bool time_out = go_through_clauses(offs);

    const double time_used = cpuTime() - myTime;
    const double time_remain = float_div(
        maxNumProps - ((int64_t)solver->propStats.bogoProps-(int64_t)oldBogoProps),
        orig_maxNumProps);
    if (solver->conf.verbosity >= 2) {
        cout << "c [distill] long cls"
        << " tried: " << runStats.checkedClauses << "/" << offs.size()
        << " cl-short:" << runStats.numClShorten
        << " lit-r:" << runStats.numLitsRem
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

    return solver->okay();
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
    solver->detachClause(offset, false);
    Clause& cl = *solver->cl_alloc.ptr(offset);
    (*solver->drat) << deldelay << cl << fin;

    uint32_t orig_size = cl.size();
    uint32_t i = 0;
    uint32_t j = 0;
    for (uint32_t sz = cl.size(); i < sz; i++) {
        if (solver->value(cl[i]) != l_False) {
            cl[j++] = cl[i];
        }
    }
    cl.resize(j);

    solver->new_decision_level();
    bool True_confl = false;
    PropBy confl;
    i = 0;
    j = 0;
    for (uint32_t sz = cl.size(); i < sz; i++) {
        const Lit lit = cl[i];
        lbool val = solver->value(lit);
        if (val == l_Undef) {
            solver->enqueue(~lit);
            cl[j++] = cl[i];

            maxNumProps -= 5;
            confl = solver->propagate<true>();
            if (!confl.isNULL()) {
                break;
            }
        } else if (val == l_False) {
            // skip
        } else {
            assert(val == l_True);
            cl[j++] = cl[i];
            True_confl = true;
            confl = solver->varData[cl[i].var()].reason;
            break;
        }
    }
    assert(solver->ok);
    cl.resize(j);

    //Couldn't simplify the clause
    if (j == orig_size && !True_confl && confl.isNULL()) {
        solver->cancelUntil<false, true>(0);
        solver->attachClause(cl);
        solver->drat->forget_delay();
        return offset;
    }

    #ifdef VERBOSE_DEBUG
    if (j < i && solver->conf.verbosity >= 5) {
        cout
        << "c Distillation branch effective." << endl
        << "c --> shortened cl:" << cl<< endl
        << "c --> orig size:" << orig_size << endl
        << "c --> new size:" << j << endl;
    }
    #endif

    bool lits_set = false;
    if (red && j > 1 && (!confl.isNULL() || True_confl)) {
        #ifdef VERBOSE_DEBUG
        if (solver->conf.verbosity >= 5) {
            cout
            << "c Distillation even more effective." << endl
            << "c --> orig shortened cl:" << cl << endl;
        }
        #endif
        maxNumProps -= 20;
        lits.clear();
        if (True_confl) {
            lits.push_back(cl[cl.size()-1]);
        }
        solver->simple_create_learnt_clause(confl, lits, True_confl);
        if (lits.size() < cl.size()) {
            #ifdef VERBOSE_DEBUG
            if (solver->conf.verbosity >= 5) {
                cout
                << "c --> more shortened cl:" << lits << endl;
            }
            #endif
            lits_set = true;
        }
    }
    solver->cancelUntil<false, true>(0);
    runStats.numLitsRem += orig_size - cl.size();
    runStats.numClShorten++;

    //Make new clause
    if (!lits_set) {
        lits.resize(cl.size());
        std::copy(cl.begin(), cl.end(), lits.begin());
    }
    solver->free_cl(offset);
    Clause *cl2 = solver->add_clause_int(lits, red, stats);
    (*solver->drat) << findelay;

    if (cl2 != NULL) {
        cl2->set_distilled(true);
        return solver->cl_alloc.get_offset(cl2);
    } else {
        //it became a bin/unit/zero
        return CL_OFFSET_MAX;
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
