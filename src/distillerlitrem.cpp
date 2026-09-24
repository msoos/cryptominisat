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



DistillerLitRem::DistillerLitRem(Solver* _solver) :
    solver(_solver)
{}

bool DistillerLitRem::distill_lit_rem()
{
    assert(solver->ok);
    num_calls++;
    run_stats.clear();


    if (!solver->remove_and_clean_all()) {
        goto end;
    }
    if (!distill_long_cls_all(solver->long_irred_cls, 1)) {
        goto end;
    }

end:
    global_stats += run_stats;
    if (solver->conf.verbosity) run_stats.print_short(solver);
    run_stats.clear();

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
    double my_time = cpu_time();
    bool time_out = false;
    auto i = cls.begin();
    auto j = i;
    for (auto end = cls.end(); i != end; ++i) {
        //Check if we are in state where we only copy offsets around
        if (time_out || !solver->ok) {
            *j++ = *i;
            continue;
        }

        //if done enough, stop doing it
        if ((int64_t)solver->prop_stats.bogo_props-(int64_t)oldBogoProps >= max_num_props
            || solver->must_interrupt_asap()
        ) {
            run_stats.time_out++;
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

        //Time to dereference
        max_num_props -= 5;
        run_stats.checked_clauses++;
        assert(cl.size() > 2);

        //we will detach the clause no matter what
        max_num_props -= solver->watches[cl[0]].size();
        max_num_props -= solver->watches[cl[1]].size();

        max_num_props -= cl.size();
        if (solver->satisfied(cl)) {
            solver->detachClause(cl);
            solver->free_cl(&cl);
            continue;
        }

        //Try to distill clause
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

    run_stats.time_used += cpu_time() - my_time;
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

    const size_t orig_trail_size = solver->trail_size();

    //Time-limiting
    max_num_props =
        5*1000LL*1000ULL
        *solver->conf.global_timeout_multiplier;

    if (solver->lit_stats.irred_lits + solver->lit_stats.red_lits <
            (500ULL*1000ULL*solver->conf.var_and_mem_out_mult)
    ) {
        max_num_props *=2;
    }
    max_num_props *= time_mult;
    orig_maxNumProps = max_num_props;

    //stats setup
    oldBogoProps = solver->prop_stats.bogo_props;
    run_stats.potential_clauses += offs.size();
    run_stats.numCalled += 1;

    bool time_out = false;
    for(uint32_t i = 0; i < 10 && !time_out; i++) {
        uint32_t prev_cls_tried = run_stats.cls_tried;
        time_out = go_through_clauses(offs, i);
        if (solver->conf.verbosity >= 2) {
            run_stats.print_short(solver);
        }

        //Max clause size reached
        if (run_stats.cls_tried == prev_cls_tried) {
            break;
        }
    }

    const double time_remain = float_div(
        max_num_props - ((int64_t)solver->prop_stats.bogo_props-(int64_t)oldBogoProps),
        orig_maxNumProps);
    if (solver->sql_stats) {
        solver->sql_stats->time_passed(
            solver
            , "distill-litrem"
            , run_stats.time_used
            , time_out
            , time_remain
        );
    }


    //Update stats
    run_stats.zero_depth_assigns += solver->trail_size() - orig_trail_size;

    return solver->okay();
}

ClOffset DistillerLitRem::try_distill_clause_and_return_new(
    ClOffset offset
    , const ClauseStats* const stats
    , const uint32_t at
) {
    assert(solver->prop_at_head());
    assert(solver->decision_level() == 0);
    const size_t orig_trail_size = solver->trail_size();
    run_stats.cls_tried++;

    Clause& cl = *solver->cl_alloc.ptr(offset);
    const bool red = cl.red();

    uint32_t orig_size = cl.size();
    assert(cl.size() > at);
    Lit torem = cl[at];

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
    // distill_use=true disables Gaussian elimination, which is needed for FRAT
    // correctness: XOR-derived conflicts can't be captured in RUP proofs
    PropBy confl = solver->propagate<true, true, true>();
    //strict hints while the trail is intact: units, the original clause
    //(unit-propagating torem), the propagation reasons, the conflict
    vector<int32_t> hints;
    if (solver->frat->enabled() && !confl.isnullptr()) {
        vector<int32_t> rsns;
        solver->collect_trail_seg_hints(
            solver->trail_begin_of_level(0), hints, rsns);
        const int32_t cid = solver->get_confl_id(confl, hints);
        hints.push_back(cl.stats.id);
        hints.insert(hints.end(), rsns.begin(), rsns.end());
        hints.push_back(cid);
    }
    solver->cancel_until<false, true>(0);

     //Couldn't remove literal
    if (confl.isnullptr()) {
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
//     << " confl.isnullptr(): " << confl.isnullptr()
//     << " i: " << i
//     << " at: " << at
//     << " cl before: " << cl
//     << " cl after: " << lits
//     << endl;

    //We can remove the literal
    (*solver->frat) << deldelay << cl << fin;
    solver->detachClause(cl, false);
    run_stats.num_lits_rem += orig_size - lits.size();
    run_stats.num_cl_shorten++;

    // we have to copy because the re-alloc can invalidate the data
    ClauseStats backup_stats(*stats);
    // new clause will inherit this clause's ID
    // so let's set this to 0, this way, when we free() it, it won't be
    // deleted as per cl_last_in_solver
    solver->free_cl(offset, false);
    Clause *cl2 = solver->add_clause_int(lits, red, &backup_stats,
        true, nullptr, true, lit_Undef, false, false,
        solver->frat->enabled() ? &hints : nullptr);
    (*solver->frat) << findelay;
    assert(solver->trail_size() == orig_trail_size);

    if (cl2 != nullptr) {
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
    time_out += other.time_out;
    zero_depth_assigns += other.zero_depth_assigns;
    num_cl_shorten += other.num_cl_shorten;
    num_lits_rem += other.num_lits_rem;
    checked_clauses += other.checked_clauses;
    potential_clauses += other.potential_clauses;
    numCalled += other.numCalled;

    return *this;
}

void DistillerLitRem::Stats::print_short(const Solver* _solver) const
{
    cout
    << "c [distill-litrem]"
    << " useful: "<< num_cl_shorten
    << "/" << checked_clauses << "/" << potential_clauses
    << " lits-rem: " << num_lits_rem
    << " 0-depth-assigns: " << zero_depth_assigns
    << _solver->conf.print_times(time_used, time_out)
    << endl;
}

double DistillerLitRem::mem_used() const
{
    double mem_used = sizeof(DistillerLitRem);
    mem_used += lits.size()*sizeof(Lit);
    return mem_used;
}
