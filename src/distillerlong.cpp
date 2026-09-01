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
#include "constants.h"
#include "time_mem.h"
#include "solver.h"
#include "watchalgos.h"
#include "clauseallocator.h"
#include "reducedb.h"
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

//CaDiCaL's vivify_more_noccs: literal order for the trie effect. Higher
//Jeroslow-Wang-style score first; ties broken as in CaDiCaL.
struct LitScoreDescSort
{
    LitScoreDescSort(const vector<uint64_t>& _lit_scores) :
        lit_scores(_lit_scores)
    {}

    bool operator()(const Lit& a, const Lit& b) const {
        const uint64_t n = lit_scores[a.toInt()];
        const uint64_t m = lit_scores[b.toInt()];
        if (n != m) return n > m;
        if (a.var() == b.var()) return !a.sign(); //positive first
        return a.var() < b.var(); //smaller index first
    }

    const vector<uint64_t>& lit_scores;
};

DistillerLong::DistillerLong(Solver* _solver) :
    solver(_solver)
{}

bool DistillerLong::distill(const bool red, bool only_rem_cl)
{
    frat_func_start();
    assert(solver->ok);
    numCalls_red += (unsigned)red;
    numCalls_irred += (unsigned)!red;
    runStats.clear();

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
            1.0,
            false, //dont' remove (it's always redundant)
            only_rem_cl,
            red,
            0)) //red lev (only to print)
        {
            goto end;
        }
        globalStats += runStats;
        runStats.clear();

        #ifdef FINAL_PREDICTOR //only predictor builds populate longRedCls[1]
        if (!distill_long_cls_all(
            solver->longRedCls[1],
            1.0,
            false, //dont' remove (it's always redundant)
            only_rem_cl,
            red,
            1))  // //red lev (only to print)
        {
            goto end;
        }
        globalStats += runStats;
        runStats.clear();
        #endif
    }

end:
    lit_counts.clear();
    lit_counts.shrink_to_fit();
    frat_func_end();

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
    frat_func_start();

    double my_time = cpu_time();
    const size_t origTrailSize = solver->trail_size();

    //Time-limiting
    if (red) {
        //CaDiCaL's vivify budget for redundant rounds: relative to the
        //propagations done since the last round, clamped, then the
        //'vivifyredeff' 75-per-mille share
        int64_t lim = (int64_t)(solver->propStats.propagations - last_red_props);
        last_red_props = solver->propStats.propagations;
        lim *= 1e-3 * (double)solver->conf.distill_red_releff;
        lim = std::max<int64_t>(lim, 60LL*1000LL);
        lim = std::min<int64_t>(lim, 60LL*1000LL*1000LL);
        maxNumProps = lim * 75LL / 1000LL;
    } else {
        maxNumProps =
            solver->conf.distill_long_cls_time_limitM*1000LL*1000ULL
            *solver->conf.global_timeout_multiplier;

        if (solver->litStats.irredLits + solver->litStats.redLits <
                (500ULL*1000ULL*solver->conf.var_and_mem_out_mult)
        ) {
            maxNumProps *=2;
        }
        maxNumProps *= time_mult;
    }
    orig_maxNumProps = maxNumProps;

    //stats setup
    oldBogoProps = solver->propStats.bogoProps;
    runStats.numCalled += 1;

    //Select candidates. prio 0: not checked since their bit was cleared,
    //as CaDiCaL's 'vivify' bit; prio 1: the rest (irred only)
    vector<ClOffset> todo;
    vector<uint32_t> todo_prio;
    todo.reserve(offs.size());
    for(uint32_t prio = 0; prio < (red ? 1: 2); prio ++) {
        uint32_t j = 0;
        for(uint32_t i = 0; i < offs.size(); i ++) {
            Clause* cl = solver->cl_alloc.ptr(offs[i]);
            VERBOSE_PRINT("Clause at " << i << " is:  " << *cl);
            bool ok = false;
            if (!cl->stats.is_ternary_resolvent
                && (!red || solver->reduceDB->likely_to_be_kept(*cl))
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
                todo.push_back(offs[i]);
                todo_prio.push_back(prio);
                VERBOSE_PRINT("Adding this one to TODO");
            } else {
                offs[j++] = offs[i];
                continue;
            }
        }
        offs.resize(j);
    }

    //CaDiCaL's noccs: capped Jeroslow-Wang score of each literal over the
    //candidates. Kept for the whole round; try_distill re-sorts each clause
    //by it so consecutive clauses share decision prefixes (the trie effect)
    lit_counts.clear();
    lit_counts.resize(solver->nVars()*2, 0);
    for(const ClOffset off: todo) {
        const Clause& cl = *solver->cl_alloc.ptr(off);
        const uint64_t score = cl.size() >= 12 ? 1ULL : 1ULL << (12-cl.size());
        for(const Lit l: cl) lit_counts[l.toInt()] += score;
    }

    //Sort the schedule as CaDiCaL's vivify_clause_later: unchecked first,
    //small glue (red), short, then lexicographically by the literal order
    //so clauses with shared prefixes are tried back-to-back
    {
        const LitScoreDescSort lit_sort(lit_counts);
        vector<vector<Lit>> slits(todo.size());
        vector<uint32_t> order(todo.size());
        for(uint32_t i = 0; i < todo.size(); i++) {
            order[i] = i;
            const Clause& cl = *solver->cl_alloc.ptr(todo[i]);
            slits[i].assign(cl.begin(), cl.end());
            std::sort(slits[i].begin(), slits[i].end(), lit_sort);
        }
        std::stable_sort(order.begin(), order.end(),
            [&](const uint32_t a, const uint32_t b) {
                if (todo_prio[a] != todo_prio[b]) return todo_prio[a] < todo_prio[b];
                const Clause& c = *solver->cl_alloc.ptr(todo[a]);
                const Clause& d = *solver->cl_alloc.ptr(todo[b]);
                if (red && c.stats.glue != d.stats.glue)
                    return c.stats.glue < d.stats.glue;
                if (c.size() != d.size()) return c.size() < d.size();
                for(uint32_t i = 0; i < c.size(); i++) {
                    if (slits[a][i] != slits[b][i])
                        return lit_sort(slits[a][i], slits[b][i]);
                }
                return false;
            });
        vector<ClOffset> todo2(todo.size());
        for(uint32_t i = 0; i < todo.size(); i++) todo2[i] = todo[order[i]];
        todo = std::move(todo2);
    }

    const uint32_t orig_todo_size = todo.size();
    runStats.potentialClauses += orig_todo_size;

    assert(runStats.checkedClauses == 0);
    bool time_out = go_through_clauses(todo, also_remove, only_remove);

    //Add back the prioritized clauses
    for(const auto off: todo) offs.push_back(off);

    const double time_used = cpu_time() - my_time;
    const double time_remain = float_div(
        maxNumProps - ((int64_t)solver->propStats.bogoProps-(int64_t)oldBogoProps),
        orig_maxNumProps);
    if (solver->conf.verbosity >= 1) {
        cout << solver->conf.prefix << "[distill-long";
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

    frat_func_end();
    return solver->okay();
}

bool DistillerLong::go_through_clauses(vector<ClOffset>& cls, bool also_remove, bool only_remove) {
    frat_func_start();
    bool time_out = false;
    size_t kept = 0;
    for (size_t at = 0; at < cls.size(); at++) {
        const ClOffset offset = cls[at];
        VERBOSE_PRINT("At offset: " << offset);

        //Check if we are in state where we only copy offsets around
        if (time_out || !solver->ok) {
            cls[kept++] = offset;
            continue;
        }

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

        //Time to dereference
        maxNumProps -= 5;

        if (also_remove) cl.tried_to_remove = 1;
        else cl.distilled = 1;
        runStats.checkedClauses++;
        assert(cl.size() > 2);

        //Try to distill clause
        const ClOffset offset2 = try_distill_clause_and_return_new(
            offset, &cl.stats, also_remove, only_remove);

        if (offset2 != CL_OFFSET_MAX) cls[kept++] = offset2;
    }
    cls.resize(kept);

    //decisions may have been left in place for reuse between candidates
    solver->cancelUntil<false, true>(0);

    frat_func_end();
    return time_out;
}

ClOffset DistillerLong::try_distill_clause_and_return_new(
    ClOffset offset, const ClauseStats* const stats,
    const bool also_remove, const bool only_remove
) {
    frat_func_start();
    assert(solver->prop_at_head());
    bool True_confl = false;
    PropBy confl;

    //Disable this clause
    Clause& cl = *solver->cl_alloc.ptr(offset);
    const Lit cl_lit1 = cl[0];
    const Lit cl_lit2 = cl[1];
    const uint32_t orig_size = cl.size();
    cl.disabled = true;
    *solver->frat << deldelay << cl << fin;
    const bool red = cl.red();
    if (red) assert(!also_remove);
    VERBOSE_PRINT("Trying to distill clause:" << cl);

    const auto remove_cl = [&]() {
        solver->cancelUntil<false, true>(0);
        solver->detach_modified_clause(cl_lit1, cl_lit2, orig_size, &cl);
        *solver->frat << findelay;
        solver->free_cl(offset);
        runStats.clRemoved++;
        frat_func_end();
        return CL_OFFSET_MAX;
    };

    //Copy the non-fixed literals to 'sorted', as CaDiCaL's vivify_clause.
    //Fixed-true: clause is satisfied; fixed-false: drop with a unit hint
    hint_units.clear();
    sorted.clear();
    for (const Lit l: cl) {
        const lbool val = solver->value(l);
        if (val != l_Undef && solver->varData[l.var()].level == 0) {
            if (val == l_True) return remove_cl();
            if (solver->frat->enabled())
                hint_units.push_back(solver->unit_cl_IDs[l.var()]);
        } else {
            sorted.push_back(l);
        }
    }
    assert(sorted.size() > 1); //fixed-false lits must have been propagated

    //Sort by the global literal order so the decision prefix lines up
    //with the previous candidate's
    std::sort(sorted.begin(), sorted.end(), LitScoreDescSort(lit_counts));

    //If this clause forced one of its literals on the reused trail, it may
    //not be used to prove itself redundant: backtrack below that level
    if (solver->decisionLevel() > 0 && solver->clause_locked(cl, offset)) {
        assert(solver->varData[cl[0].var()].level > 0);
        solver->cancelUntil<false, true>(solver->varData[cl[0].var()].level - 1);
    }

    //Reuse the decisions of the previous candidate as long as they match
    //our sorted literals, as CaDiCaL does (the trie effect)
    if (solver->decisionLevel() > 0) {
        uint32_t match = 0;
        for (const Lit l: sorted) {
            if (match >= solver->decisionLevel()) break;
            const Lit dec = solver->trail_at(solver->trail_begin_of_level(match));
            if (dec == ~l) match++;
            else break;
        }
        if (match < solver->decisionLevel()) solver->cancelUntil<false, true>(match);
    }

    const uint32_t seg_start =
        solver->decisionLevel() > 0 ? solver->trail_begin_of_level(0) : solver->getTrailSize();
    uint32_t num_dropped = orig_size - sorted.size(); //fixed-false lits
    kept_lits.clear();
    for (const Lit lit: sorted) {
        const lbool val = solver->value(lit);
        if (val == l_Undef) {
            solver->new_decision_level();
            solver->enqueue<true>(~lit);
            kept_lits.push_back(lit);

            maxNumProps -= 5;
            if (!red && also_remove) {
                //ONLY propagate on irred
                confl = solver->propagate<true, false, true>();
            } else {
                //Normal propagation, on all clauses
                confl = solver->propagate<true, true, true>();
            }
            if (!confl.isnullptr()) break;
        } else if (val == l_False) {
            if (solver->varData[lit.var()].reason.isnullptr()) {
                //one of our own (possibly reused) decisions
                kept_lits.push_back(lit);
            } else if (only_remove) {
                // if we don't want to shorten, then don't remove literals
                kept_lits.push_back(lit);
            } else {
                num_dropped++;
            }
        } else {
            assert(val == l_True);
            kept_lits.push_back(lit);
            True_confl = true;
            confl = solver->varData[lit.var()].reason;
            break;
        }
    }
    assert(solver->ok);

    VERBOSE_PRINT("also_remove: " << also_remove
        << "red: " << red
        << "True_confl: " << True_confl
        << "confl.isnullptr(): " << confl.isnullptr());

    //Actually, we can remove the clause!
    if (also_remove && !red && !True_confl && !confl.isnullptr()) {
        VERBOSE_PRINT("CL Removed.");
        return remove_cl();
    }

    //Couldn't simplify the clause. Keep the trail for the next candidate
    if (num_dropped == 0 && kept_lits.size() == orig_size
        && !True_confl && confl.isnullptr()
    ) {
        VERBOSE_PRINT("CL Cannot be simplified.");
        cl.disabled = false;
        solver->frat->forget_delay();
        frat_func_end();
        return offset;
    }

    //strict hints while the trail is still intact: units, propagation
    //reasons in trail order, closing clause (conflict/true-lit reason/orig)
    hints.clear();
    if (solver->frat->enabled()) {
        vector<int32_t> rsns;
        solver->collect_trail_seg_hints(seg_start, hint_units, rsns);
        int32_t final_id;
        if (True_confl) final_id = solver->get_reason_id(confl, hint_units);
        else if (!confl.isnullptr()) final_id = solver->get_confl_id(confl, hint_units);
        else final_id = stats->id;
        hints = hint_units;
        hints.insert(hints.end(), rsns.begin(), rsns.end());
        hints.push_back(final_id);
    }

    solver->cancelUntil<false, true>(0);
    solver->detach_modified_clause(cl_lit1, cl_lit2, orig_size, &cl);
    runStats.numLitsRem += orig_size - kept_lits.size();
    runStats.numClShorten++;

    //Make new clause
    lits.resize(kept_lits.size());
    std::copy(kept_lits.begin(), kept_lits.end(), lits.begin());

    // we have to copy because the re-alloc can invalidate the data
    ClauseStats backup_stats(*stats);
    // new clause will inherit this clause's ID
    // so let's set this to 0, this way, when we free() it, it won't be
    // deleted as per cl_last_in_solver
#ifdef DEBUG_FRAT
    {
        std::stringstream ss2;
        ss2 << lits;
        *solver->frat << " new smaller cl: " << ss2.str().c_str() << "\n";
    }
#endif
    solver->free_cl(offset, false);
    Clause *cl2 = solver->add_clause_int(lits, red, &backup_stats,
        true, nullptr, true, lit_Undef, false, false, &hints);
    *solver->frat << findelay;

    if (cl2 != nullptr) {
        //This new, distilled clause has been distilled now.
        if (also_remove) cl2->tried_to_remove = 1;
        else cl2->distilled = 1;
        frat_func_end();
        return solver->cl_alloc.get_offset(cl2);
    } else {
        STATS_DO(solver->stats_del_cl(offset));
        //it became a bin/unit/zero
        frat_func_end();
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
