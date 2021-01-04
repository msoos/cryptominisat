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
    runStats.clear();


    if (!red) {

        if (!distill_long_cls_all(solver->longIrredCls, 1, true)) {
            goto end;
        }
        if (!distill_long_cls_all(solver->longIrredCls, 1, false)) {
            goto end;
        }
    } else {
        if (solver->conf.pred_distill_orig) {
            if (!distill_long_cls_all(solver->longRedCls[0], 10.0)) {
                goto end;
            }
            if (!distill_long_cls_all(solver->longRedCls[1], solver->conf.distill_red_tier1_ratio)) {
                goto end;
            }
        } else {
            if (!distill_long_cls_all(solver->longRedCls[0], 7.0)) {
                goto end;
            }
            if (!distill_long_cls_all(solver->longRedCls[1], 3.0)) {
                goto end;
            }
        }
    }

end:
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

        return cl1->size() > cl2->size();
    }
};

bool DistillerLong::go_through_clauses(
    vector<ClOffset>& cls,
    bool also_remove
) {
    bool time_out = false;
    uint32_t skipped = 0;
    uint32_t tried = 0;
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
        if (cl.getdistilled() || cl._xor_is_detached
            || (!solver->conf.pred_distill_orig &&
                cl.red() &&
                cl.stats.glue > 3)
        ) {
            skipped++;
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
        tried++;
        offset2 = try_distill_clause_and_return_new(
            offset
            , &cl.stats
            , also_remove
        );

        copy:
        if (offset2 != CL_OFFSET_MAX) {
            *j++ = offset2;
        }
    }
    cls.resize(cls.size()- (i-j));
//     cout << "Did: " << tried << endl;
//     cout << "Skipped: " << skipped << endl;

    return time_out;
}

bool DistillerLong::distill_long_cls_all(
    vector<ClOffset>& offs
    , double time_mult
    , bool also_remove
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

    if (also_remove) {
        std::sort(offs.begin()
            , offs.end()
            , ClauseSizeSorterLargestFirst(solver->cl_alloc)
        );
    }

    bool time_out = go_through_clauses(offs, also_remove);

    const double time_used = cpuTime() - myTime;
    const double time_remain = float_div(
        maxNumProps - ((int64_t)solver->propStats.bogoProps-(int64_t)oldBogoProps),
        orig_maxNumProps);
    if (solver->conf.verbosity >= 3) {
        cout << "c [distill] long cls"
        << " tried: " << runStats.checkedClauses << "/" << offs.size()
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
    runStats.time_used += time_used;
    runStats.zeroDepthAssigns += solver->trail_size() - origTrailSize;

    return solver->okay();
}

ClOffset DistillerLong::try_distill_clause_and_return_new(
    ClOffset offset
    , const ClauseStats* const stats
    , const bool also_remove
) {
    assert(solver->prop_at_head());
    #ifdef DRAT_DEBUG
    if (solver->conf.verbosity >= 6) {
        cout << "Trying to distill clause:" << lits << endl;
    }
    #endif

    //Detach this clause
    solver->detachClause(offset, false);
    Clause& cl = *solver->cl_alloc.ptr(offset);
    (*solver->drat) << deldelay << cl << fin;
    const bool red = cl.red();
    if (red) {
        assert(!also_remove);
    }

    uint32_t orig_size = cl.size();
    uint32_t i = 0;
    uint32_t j = 0;
    for (uint32_t sz = cl.size(); i < sz; i++) {
        //When we go into this function, we KNOW the clause is not satisfied
        assert(solver->value(cl[i]) != l_True);
        if (solver->value(cl[i]) == l_Undef) {
            cl[j++] = cl[i];
        }
    }
    cl.resize(j);
    assert(cl.size() > 1); //this must have already been propagated

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
            if (!red && also_remove) {
                //ONLY propagate on irred
                confl = solver->propagate<true, false>();
            } else {
                //Normal propagation, on all clauses
                confl = solver->propagate<true>();
            }
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

    //Actually, we can remove the clause!
    if (also_remove && !red && !True_confl && !confl.isNULL()) {
        //cout << "Removed clause" << endl;
        solver->cancelUntil<false, true>(0);
        (*solver->drat) << findelay;
        solver->free_cl(offset);
        runStats.clRemoved++;
        return CL_OFFSET_MAX;
    }

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

    // we have to copy because the re-alloc can invalidate the data
    ClauseStats backup_stats(*stats);
    // new clause will inherit this clause's ID
    // so let's set this to 0, this way, when we free() it, it won't be
    // deleted as per cl_last_in_solver
    solver->free_cl(offset, false);
    Clause *cl2 = solver->add_clause_int(lits, red, &backup_stats);
    (*solver->drat) << findelay;

    if (cl2 != NULL) {
        cl2->set_distilled(true);
        return solver->cl_alloc.get_offset(cl2);
    } else {
        #ifdef STATS_NEEDED
        solver->stats_del_cl(offset);
        #endif
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
    clRemoved += other.clRemoved;

    return *this;
}

void DistillerLong::Stats::print_short(const Solver* _solver) const
{
    cout
    << "c [distill] long"
    << " useful: "<< numClShorten
    << "/" << checkedClauses << "/" << potentialClauses
    << " lits-rem: " << numLitsRem
    << " cl-rem: " << clRemoved
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
