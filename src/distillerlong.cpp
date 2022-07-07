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
#include <random>
using namespace CMSat;
using std::cout;
using std::endl;

#ifdef VERBOSE_DEBUG
#define VERBOSE_SUBSUME_NONEXIST
#endif

//#define VERBOSE_DEBUG

//#define VERBOSE_SUBSUME_NONEXIST

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

#ifdef FINAL_PREDICTOR
struct ClauseSorterFirstInSolver
{
    ClauseSorterFirstInSolver(const ClauseAllocator& _cl_alloc, const vector<ClauseStatsExtra>& _extras) :
        cl_alloc(_cl_alloc),
        extras(_extras)
    {}

    const ClauseAllocator& cl_alloc;
    const vector<ClauseStatsExtra>& extras;

    bool operator()(const ClOffset off1, const ClOffset off2) const
    {
        const Clause* cl1 = cl_alloc.ptr(off1);
        const Clause* cl2 = cl_alloc.ptr(off2);

        const auto& extra1 = extras[cl1->stats.extra_pos];
        const auto& extra2 = extras[cl2->stats.extra_pos];

        //Correct order if c1 was introduced earlier goes first
        return extra1.introduced_at_conflict < extra2.introduced_at_conflict;
    }
};
#endif

struct ClauseSorterSmallGlueFirst
{
    ClauseSorterSmallGlueFirst(const ClauseAllocator& _cl_alloc) :
        cl_alloc(_cl_alloc)
    {}

    const ClauseAllocator& cl_alloc;

    bool operator()(const ClOffset off1, const ClOffset off2) const
    {
        const Clause* cl1 = cl_alloc.ptr(off1);
        const Clause* cl2 = cl_alloc.ptr(off2);

        //Correct order if c1's glue is smaller
        return cl1->stats.glue < cl2->stats.glue;
    }
};

#ifdef FINAL_PREDICTOR
struct ClauseSorterBestPredFirst
{
    ClauseSorterBestPredFirst(const ClauseAllocator& _cl_alloc, vector<ClauseStatsExtra>& _extra_data) :
        cl_alloc(_cl_alloc),
        extra_data(_extra_data)
    {}

    const ClauseAllocator& cl_alloc;
    const vector<ClauseStatsExtra>& extra_data;

    bool operator()(const ClOffset off1, const ClOffset off2) const
    {
        const Clause* cl1 = cl_alloc.ptr(off1);
        const Clause* cl2 = cl_alloc.ptr(off2);
        const auto& ext1 = extra_data[cl1->stats.extra_pos];
        const auto& ext2 = extra_data[cl2->stats.extra_pos];

        if (cl1->size() != cl2->size()) {
            return cl1->size() > cl2->size();
        }
        //Correct order if c1's predicted use is larger
        return ext1.pred_short_use  > ext2.pred_short_use;
    }
};
#endif

struct LitCountDescSort
{
    LitCountDescSort(const vector<uint64_t>& _lit_counts) :
        lit_counts(_lit_counts)
    {}

    bool operator()(const Lit& lit1, const Lit& lit2) {
        return lit_counts[lit1.toInt()] > lit_counts[lit2.toInt()];
    }


    const vector<uint64_t>& lit_counts;
};

DistillerLong::DistillerLong(Solver* _solver) :
    solver(_solver)
{}

bool DistillerLong::distill(const bool red, bool only_rem_cl)
{
    assert(solver->ok);
    numCalls_red += (unsigned)red;
    numCalls_irred += (unsigned)!red;
    runStats.clear();
    *solver->frat << __PRETTY_FUNCTION__ << " start\n";

    if (!red) {
        if (!distill_long_cls_all(
            solver->longIrredCls,
            solver->conf.distill_irred_alsoremove_ratio,
            true, //also remove
            only_rem_cl,
            red))
        {
            goto end;
        }
        globalStats += runStats;
        runStats.clear();

        if (!only_rem_cl) {
            if (!distill_long_cls_all(
                solver->longIrredCls,
                solver->conf.distill_irred_noremove_ratio,
                false, //also remove
                only_rem_cl,
                red))
            {
                goto end;
            }
        }
        globalStats += runStats;
        runStats.clear();
    } else {
        //Redundant
        if (!distill_long_cls_all(
            solver->longRedCls[0],
            solver->conf.distill_red_tier0_ratio,
            false, //dont' remove (it's always redundant)
            only_rem_cl,
            red,
            0)) //red lev (only to print)
        {
            goto end;
        }
        globalStats += runStats;
        runStats.clear();

        if (!distill_long_cls_all(
            solver->longRedCls[1],
            solver->conf.distill_red_tier1_ratio,
            false, //dont' remove (it's always redundant)
            only_rem_cl,
            red,
            1))  // //red lev (only to print)
        {
            goto end;
        }
        globalStats += runStats;
        runStats.clear();
    }

end:
    lit_counts.clear();
    lit_counts.shrink_to_fit();
    *solver->frat << __PRETTY_FUNCTION__ << " end\n";

    return solver->okay();
}

bool DistillerLong::distill_long_cls_all(
    vector<ClOffset>& offs
    , double time_mult
    , bool also_remove
    , bool only_remove
    , bool red
    , uint32_t red_lev
) {
    assert(solver->ok);
    if (time_mult == 0.0) return solver->okay();
    verb_print(6, "c Doing distillation branch for long clauses");

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
    runStats.numCalled += 1;

    //Shuffle only when it's non-learnt run (i.e. also_remove)
    if (//Don't shuffle when it's very-very large, too expensive
        offs.size() < 100ULL*1000ULL*1000ULL)
    {
        if (solver->conf.distill_sort == 0) {
            //Nothing

        } else if (solver->conf.distill_sort == 1) {
            std::sort(offs.begin(),
                offs.end(),
                ClauseSizeSorterLargestFirst(solver->cl_alloc)
            );
        } else if (solver->conf.distill_sort == 2) {
            std::sort(offs.begin(),
                offs.end(),
                ClauseSorterSmallGlueFirst(solver->cl_alloc)
            );
        } else if (solver->conf.distill_sort == 3) {
            #ifdef FINAL_PREDICTOR
            if (red) {
                //This ensures fixed order. Otherwise, due to reducedb's ordering clauses around, it'd always be very hectic order, effectively random order
                std::sort(offs.begin(),
                    offs.end(),
                    ClauseSorterFirstInSolver(solver->cl_alloc, solver->red_stats_extra)
                );
            }
            #else
            cout << "ERROR: only distill sort 0, 1 and 2 are recognized" << endl;
            exit(-1);
            #endif
        } else if (solver->conf.distill_sort == 4) {
            bool randomly_sort = solver->mtrand.randInt(solver->conf.distill_rand_shuffle_order_every_n) == 0;
            if (randomly_sort) {
                std::mt19937 gen(solver->mtrand.randInt());
                std::shuffle(offs.begin(), offs.end(), gen);
            } else {
                std::sort(offs.begin(),
                    offs.end(),
                    ClauseSizeSorterLargestFirst(solver->cl_alloc)
                );
            }
        }
    }

    //Prioritize
    lit_counts.clear();
    lit_counts.resize(solver->nVars()*2, 0);
    vector<ClOffset> todo;
    todo.reserve(offs.size());
    for(uint32_t prio = 0; prio < (red ? 1: 2); prio ++) {
        uint32_t j = 0;
        for(uint32_t i = 0; i < offs.size(); i ++) {
            Clause* cl = solver->cl_alloc.ptr(offs[i]);
            VERBOSE_PRINT("Clause at " << i << " is:  " << *cl);
            bool ok = false;
            if (!cl->stats.is_ternary_resolvent
                && !solver->satisfied(*cl)
            ) {
                if (also_remove) {
                    if (cl->tried_to_remove == prio) {
                        ok = true;
                    }
                } else {
                    if (cl->distilled == prio) {
                        ok = true;
                    }
                }
            }

            if (ok) {
                for(const auto& l: *cl) lit_counts[l.toInt()]++;
                todo.push_back(offs[i]);
                VERBOSE_PRINT("Adding this one to TODO");
            } else {
                offs[j++] = offs[i];
                continue;
            }
        }
        offs.resize(j);
    }
    const uint32_t orig_todo_size = todo.size();
    runStats.potentialClauses += orig_todo_size;

    assert(runStats.checkedClauses == 0);
    bool time_out = go_through_clauses(todo, also_remove, only_remove);

    //Add back the prioritized clauses
    for(const auto off: todo) offs.push_back(off);

    const double time_used = cpuTime() - myTime;
    const double time_remain = float_div(
        maxNumProps - ((int64_t)solver->propStats.bogoProps-(int64_t)oldBogoProps),
        orig_maxNumProps);
    if (solver->conf.verbosity >= 1) {
        cout << "c [distill-long";
        if (red) {
            cout << "-red" << red_lev << "]";
        } else {
            cout << "-irred]";
        }
        cout
        << " cls"
        << " tried: " << runStats.checkedClauses << "/" << orig_todo_size
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
    runStats.time_used += time_used;
    runStats.zeroDepthAssigns += solver->trail_size() - origTrailSize;

    return solver->okay();
}

bool DistillerLong::go_through_clauses(
    vector<ClOffset>& cls,
    bool also_remove, bool only_remove

) {
    bool time_out = false;
    vector<ClOffset>::iterator i, j;
    i = j = cls.begin();
    for (vector<ClOffset>::iterator end = cls.end()
        ; i != end
        ; ++i
    ) {
        VERBOSE_PRINT("At offset: " << *i);

        //Check if we are in state where we only copy offsets around
        if (time_out || !solver->ok) {
            *j++ = *i;
            continue;
        }

        //Get pointer
        ClOffset offset = *i;
        Clause& cl = *solver->cl_alloc.ptr(offset);

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

        //check XOR
        if (cl.used_in_xor() &&
            solver->conf.force_preserve_xors
        ) {
            *j++ = *i;
            VERBOSE_PRINT("Skipping offset for XOR " << *i);
            continue;
        }

        //Time to dereference
        maxNumProps -= 5;

        //If we already tried this clause, then move to next
        if (cl._xor_is_detached ||

            //If it's a redundant that's not very good, let's not distill it
            (
                #ifdef FINAL_PREDICTOR
                solver->conf.pred_distill_only_smallgue &&
                #else
                false &&
                #endif
                cl.red() &&
                cl.stats.glue > 3) //TODO I don't like this at all for FINAL_PREDICTOR !!!!
        ) {
            *j++ = *i;
            VERBOSE_PRINT("Skipping offset " << *i);
            continue;
        }
        if (also_remove) {
            cl.tried_to_remove = 1;
        } else {
            cl.distilled = 1;
        }
        runStats.checkedClauses++;
        assert(cl.size() > 2);

        //Try to distill clause
        ClOffset offset2 = try_distill_clause_and_return_new(
            offset, &cl.stats
            , also_remove, only_remove
        );

        if (offset2 != CL_OFFSET_MAX) {
            *j++ = offset2;
        }
    }
    cls.resize(cls.size()- (i-j));

    return time_out;
}

ClOffset DistillerLong::try_distill_clause_and_return_new(
    ClOffset offset, const ClauseStats* const stats,
    const bool also_remove, const bool only_remove
) {
    assert(solver->prop_at_head());
    assert(solver->decisionLevel() == 0);
    bool True_confl = false;
    PropBy confl;

    //Disable this clause
    Clause& cl = *solver->cl_alloc.ptr(offset);
    Lit cl_lit1 = cl[0];
    Lit cl_lit2 = cl[1];
    cl.disabled = true;
    *solver->frat << deldelay << cl << fin;
    const bool red = cl.red();
    if (red) assert(!also_remove);
    VERBOSE_PRINT("Trying to distill clause:" << cl);

    uint32_t orig_size = cl.size();
    uint32_t i = 0;
    uint32_t j = 0;
    for (uint32_t sz = cl.size(); i < sz; i++) {
        if (solver->value(cl[i]) == l_True) {
            goto rem;
        }
        if (solver->value(cl[i]) == l_Undef) {
            cl[j++] = cl[i];
        }
    }
    cl.resize(j);
    assert(cl.size() > 1); //this must have already been propagated

    solver->new_decision_level();
    i = 0;
    j = 0;


    // Sort them differently once in a while, so all literals have a chance of
    // being removed
    if (solver->conf.distill_sort == 4 &&
        cl.size() < 500) //Don't sort them if they are too large, it can be really slow
    {
        //Sort them differently once in a while, so all literals have a chance of
        //being removed
        if (offset % 2  == 0) {
            std::sort(cl.begin(), cl.end(), VSIDS_largest_first(solver->var_act_vsids));
        } else {
            std::sort(cl.begin(), cl.end(), LitCountDescSort(lit_counts));
        }
    }

    for (uint32_t sz = cl.size(); i < sz; i++) {
        const Lit lit = cl[i];
        lbool val = solver->value(lit);
        if (val == l_Undef) {
            solver->enqueue<true>(~lit);
            cl[j++] = cl[i];

            maxNumProps -= 5;
            if (!red && also_remove) {
                //ONLY propagate on irred
                confl = solver->propagate<true, false, true>();
            } else {
                //Normal propagation, on all clauses
                confl = solver->propagate<true, true, true>();
            }
            if (!confl.isNULL()) {
                break;
            }
        } else if (val == l_False) {
            // if we don't want to shorten, then don't remove literals
            if (only_remove) cl[j++] = cl[i];
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
    VERBOSE_PRINT("also_remove: " << also_remove
        << "red: " << red
        << "True_confl: " << True_confl
        << "confl.isNULL(): " << confl.isNULL());


    if (also_remove && !red && !True_confl && !confl.isNULL()) {
        VERBOSE_PRINT("CL Removed.");
        rem:
        solver->cancelUntil<false, true>(0);
        solver->detach_modified_clause(cl_lit1, cl_lit2, orig_size, &cl);
        (*solver->frat) << findelay;
        solver->free_cl(offset);
        runStats.clRemoved++;
        return CL_OFFSET_MAX;
    }

    //Couldn't simplify the clause
    if (j == orig_size && !True_confl && confl.isNULL()) {
        #ifdef VERBOSE_DEBUG
        cout << "CL Cannot be simplified." << endl;
        #endif
        cl.disabled = false;
        solver->cancelUntil<false, true>(0);
        std::swap(*std::find(cl.begin(), cl.end(), cl_lit1), cl[0]);
        std::swap(*std::find(cl.begin(), cl.end(), cl_lit2), cl[1]);
        solver->frat->forget_delay();
        return offset;
    }

    #ifdef VERBOSE_DEBUG
    if (j < i) {
        cout
        << "c Distillation branch effective." << endl
        << "c --> shortened cl:" << cl<< endl
        << "c --> orig size:" << orig_size << endl
        << "c --> new size:" << j << endl;
    } else {
        cout
        << "c Distillation branch NOT effective." << endl
        << "c --> orig size:" << orig_size << endl;
    }
    #endif

    bool lits_set = false;
    //TODO BNN removed this, but needs to be fixed.
    /*if (red && j > 1 && (!confl.isNULL() || True_confl)) {
        #ifdef VERBOSE_DEBUG
        cout
        << "c Distillation even more effective." << endl
        << "c --> orig shortened cl:" << cl << endl;
        #endif
        maxNumProps -= 20;
        lits.clear();
        if (True_confl) {
            lits.push_back(cl[cl.size()-1]);
        }
        solver->simple_create_learnt_clause(confl, lits, True_confl);
        if (lits.size() < cl.size()) {
            #ifdef VERBOSE_DEBUG
            cout
            << "c --> more shortened cl:" << lits << endl;
            #endif
            lits_set = true;
        }
    }*/
    solver->cancelUntil<false, true>(0);
    solver->detach_modified_clause(cl_lit1, cl_lit2, orig_size, &cl);
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
    *solver->frat << findelay;

    if (cl2 != NULL) {
        //This new, distilled clause has been distilled now.
        if (also_remove) {
            cl2->tried_to_remove = 1;
        } else {
            cl2->distilled = 1;
        }
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

void DistillerLong::Stats::print(const size_t nVars) const
{
    cout << "c -------- DISTILL-LONG STATS --------" << endl;
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
