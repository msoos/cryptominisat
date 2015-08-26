/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.0 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/

#include "reducedb.h"
#include "solver.h"
#include "sqlstats.h"
#include "clausecleaner.h"
#include <functional>

using namespace CMSat;

struct SortRedClsGlue
{
    SortRedClsGlue(ClauseAllocator& _cl_alloc) :
        cl_alloc(_cl_alloc)
    {}
    ClauseAllocator& cl_alloc;

    bool operator () (const ClOffset xOff, const ClOffset yOff) const
    {
        const Clause* x = cl_alloc.ptr(xOff);
        const Clause* y = cl_alloc.ptr(yOff);
        return x->stats.glue < y->stats.glue;
    }
};

struct SortRedClsSize
{
    SortRedClsSize(ClauseAllocator& _cl_alloc) :
        cl_alloc(_cl_alloc)
    {}
    ClauseAllocator& cl_alloc;

    bool operator () (const ClOffset xOff, const ClOffset yOff) const
    {
        const Clause* x = cl_alloc.ptr(xOff);
        const Clause* y = cl_alloc.ptr(yOff);
        return x->size() < y->size();
    }
};

struct SortRedClsAct
{
    SortRedClsAct(ClauseAllocator& _cl_alloc) :
        cl_alloc(_cl_alloc)
    {}
    ClauseAllocator& cl_alloc;

    bool operator () (const ClOffset xOff, const ClOffset yOff) const
    {
        const Clause* x = cl_alloc.ptr(xOff);
        const Clause* y = cl_alloc.ptr(yOff);
        return x->stats.activity > y->stats.activity;
    }
};

ReduceDB::ReduceDB(Solver* _solver) :
    solver(_solver)
{
}

void ReduceDB::sort_red_cls(ClauseClean clean_type)
{
    switch (clean_type) {
        case ClauseClean::glue : {
            std::sort(solver->longRedCls.begin(), solver->longRedCls.end(), SortRedClsGlue(solver->cl_alloc));
            break;
        }

        case ClauseClean::size : {
            std::sort(solver->longRedCls.begin(), solver->longRedCls.end(), SortRedClsSize(solver->cl_alloc));
            break;
        }

        case ClauseClean::activity : {
            std::sort(solver->longRedCls.begin(), solver->longRedCls.end(), SortRedClsAct(solver->cl_alloc));
            break;
        }

        default: {
            cout << "Unknown cleaning type: " << clean_to_int(clean_type) << endl;
            assert(false);
            exit(-1);
        }
    }
}

void ReduceDB::print_best_red_clauses_if_required() const
{
    if (solver->longRedCls.empty()
        || solver->conf.doPrintBestRedClauses == 0
    ) {
        return;
    }

    size_t at = 0;
    for(long i = ((long)solver->longRedCls.size())-1
        ; i > ((long)solver->longRedCls.size())-1-solver->conf.doPrintBestRedClauses && i >= 0
        ; i--
    ) {
        ClOffset offset = solver->longRedCls[i];
        const Clause* cl = solver->cl_alloc.ptr(offset);
        cout
        << "c [best-red-cl] Red " << nbReduceDB
        << " No. " << at << " > "
        << solver->clauseBackNumbered(*cl)
        << endl;

        at++;
    }
}

CleaningStats ReduceDB::reduceDB()
{
    const double myTime = cpuTime();
    nbReduceDB++;
    CleaningStats tmpStats;
    tmpStats.origNumClauses = solver->longRedCls.size();
    tmpStats.origNumLits = solver->litStats.redLits;

    const uint64_t sumConfl = solver->sumConflicts();

    move_to_never_cleaned();
    int64_t num_to_reduce = solver->longRedCls.size();

    //TODO maybe we chould count binary learnt clauses as well into the kept no. of clauses as other solvers do
    for(unsigned keep_type = 0; keep_type < 3; keep_type++) {
        const uint64_t keep_num = (double)num_to_reduce*solver->conf.ratio_keep_clauses[keep_type];
        if (keep_num == 0) {
            continue;
        }
        sort_red_cls(static_cast<ClauseClean>(keep_type));
        print_best_red_clauses_if_required();
        mark_top_N_clauses(keep_num);
    }
    move_from_never_cleaned();
    assert(delayed_clause_free.empty());
    cl_locked = 0;
    cl_marked = 0;
    cl_glue = 0;
    cl_ttl = 0;
    cl_locked_solver = 0;
    remove_cl_from_array_and_count_stats(tmpStats, sumConfl);
    if (solver->conf.verbosity >= 2) {
        cout << "c [DBclean] locked:" << cl_locked
        << " marked: " << cl_marked
        << " glue: " << cl_glue
        << " ttl:" << cl_ttl
        << " locked_solver:" << cl_locked_solver
        << endl;
    }

    solver->clean_occur_from_removed_clauses_only_smudged();
    solver->watches.clear_smudged();
    for(ClOffset offset: delayed_clause_free) {
        solver->cl_alloc.clauseFree(offset);
        solver->num_red_cls_reducedb--;
    }
    delayed_clause_free.clear();
    solver->unmark_all_red_clauses();
    solver->check_no_removed_or_freed_cl_in_watch();

    tmpStats.cpu_time = cpuTime() - myTime;
    if (solver->conf.verbosity >= 4)
        tmpStats.print(0);
    else if (solver->conf.verbosity >= 3) {
        tmpStats.print_short(solver);
    }
    cleaningStats += tmpStats;

    if (solver->sqlStats) {
        solver->sqlStats->time_passed_min(
            solver
            , "dbclean"
            , tmpStats.cpu_time
        );
    }

    last_reducedb_num_conflicts = solver->sumConflicts();
    return tmpStats;
}

void ReduceDB::move_to_never_cleaned()
{
    assert(never_cleaned.empty());

    size_t i = 0;
    size_t j = 0;
    for(size_t size = solver->longRedCls.size(); i < size; i++) {
        const ClOffset offset = solver->longRedCls[i];
        const Clause* cl = solver->cl_alloc.ptr(offset);
        assert(!cl->stats.marked_clause);

        if (!cl_needs_removal(cl, offset)) {
            never_cleaned.push_back(solver->longRedCls[i]);
        } else {
            solver->longRedCls[j++] = solver->longRedCls[i];
        }
    }
    solver->longRedCls.resize(j);
}

void ReduceDB::move_from_never_cleaned()
{
    for(ClOffset off: never_cleaned) {
        solver->longRedCls.push_back(off);
    }
    never_cleaned.clear();
}

void ReduceDB::mark_top_N_clauses(const uint64_t keep_num)
{
    size_t marked = 0;
    for(size_t i = 0
        ; i < solver->longRedCls.size() && marked < keep_num
        ; i++
    ) {
        const ClOffset offset = solver->longRedCls[i];
        Clause* cl = solver->cl_alloc.ptr(offset);
        if ( cl->stats.locked
            || cl->stats.marked_clause
            || cl->stats.glue <= solver->conf.glue_must_keep_clause_if_below_or_eq
            || cl->stats.ttl > 0
            || solver->clause_locked(*cl, offset)
        ) {
            //no need to mark, skip
            continue;
        }

        if (!cl->stats.marked_clause) {
            marked++;
            cl->stats.marked_clause = true;
        }
    }
}

#ifdef STATS_NEEDED
bool ReduceDB::red_cl_too_young(const Clause* cl) const
{
    return cl->stats.introduced_at_conflict + solver->conf.min_time_in_db_before_eligible_for_cleaning
            >= solver->sumConflicts();
}
#endif

bool ReduceDB::cl_needs_removal(const Clause* cl, const ClOffset offset) const
{
    assert(cl->red());
    return
         !cl->stats.locked
         #ifdef STATS_NEEDED
         && !red_cl_too_young(cl)
        #endif
         && !cl->stats.marked_clause
         && cl->stats.ttl == 0
         && cl->stats.glue > solver->conf.glue_must_keep_clause_if_below_or_eq
         && !solver->clause_locked(*cl, offset);
}

void ReduceDB::remove_cl_from_array_and_count_stats(
    CleaningStats& tmpStats
    , uint64_t sumConfl
) {
    size_t i, j;
    for (i = j = 0
        ; i < solver->longRedCls.size()
        ; i++
    ) {
        ClOffset offset = solver->longRedCls[i];
        Clause* cl = solver->cl_alloc.ptr(offset);
        assert(cl->size() > 3);

        if (cl->stats.locked) {
            cl_locked++;
        } else if (cl->stats.marked_clause) {
            cl_marked++;
        } else if (cl->stats.ttl != 0) {
            cl_ttl++;
        } else if (cl->stats.glue <= solver->conf.glue_must_keep_clause_if_below_or_eq) {
            cl_glue++;
        } else if (solver->clause_locked(*cl, offset)) {
            cl_locked_solver++;
        }

        if (!cl_needs_removal(cl, offset)) {
            if (cl->stats.ttl > 0) {
                cl->stats.ttl = 0;
            }
            solver->longRedCls[j++] = offset;
            tmpStats.remain.incorporate(cl, sumConfl);
            continue;
        }

        //Stats Update
        cl->setRemoved();
        solver->watches.smudge((*cl)[0]);
        solver->watches.smudge((*cl)[1]);
        tmpStats.removed.incorporate(cl, sumConfl);
        solver->litStats.redLits -= cl->size();

        *solver->drup << del << *cl << fin;
        delayed_clause_free.push_back(offset);
    }
    solver->longRedCls.resize(solver->longRedCls.size() - (i - j));
}

void ReduceDB::reduce_db_and_update_reset_stats()
{
    ClauseUsageStats irred_cl_usage_stats = sumClauseData(solver->longIrredCls);
    ClauseUsageStats red_cl_usage_stats = sumClauseData(solver->longRedCls);
    ClauseUsageStats sum_cl_usage_stats;
    sum_cl_usage_stats += irred_cl_usage_stats;
    sum_cl_usage_stats += red_cl_usage_stats;
    if (solver->conf.verbosity >= 4) {
        cout << "c irred";
        irred_cl_usage_stats.print();

        cout << "c red  ";
        red_cl_usage_stats.print();

        cout << "c sum  ";
        sum_cl_usage_stats.print();
    }

    CleaningStats iterCleanStat = reduceDB();

    if (solver->sqlStats) {
        solver->sqlStats->reduceDB(irred_cl_usage_stats, red_cl_usage_stats, iterCleanStat, solver);
    }

    if (solver->conf.doClearStatEveryClauseCleaning) {
        solver->clear_clauses_stats();
    }
}

ClauseUsageStats ReduceDB::sumClauseData(
    const vector<ClOffset>& toprint
) const {
    ClauseUsageStats stats;

    for(ClOffset offset: toprint) {
        const Clause& cl = *solver->cl_alloc.ptr(offset);
        stats.addStat(cl);

        if (solver->conf.verbosity >= 6)
            cl.print_extra_stats();
    }

    return stats;
}
