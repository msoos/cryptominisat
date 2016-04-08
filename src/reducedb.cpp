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
            std::sort(solver->longRedCls[1].begin(), solver->longRedCls[1].end(), SortRedClsGlue(solver->cl_alloc));
            break;
        }

        case ClauseClean::size : {
            std::sort(solver->longRedCls[1].begin(), solver->longRedCls[1].end(), SortRedClsSize(solver->cl_alloc));
            break;
        }

        case ClauseClean::activity : {
            std::sort(solver->longRedCls[1].begin(), solver->longRedCls[1].end(), SortRedClsAct(solver->cl_alloc));
            break;
        }

        default: {
            assert(false && "Unknown cleaning type");
        }
    }
}

CleaningStats ReduceDB::reduceDB()
{
    const double myTime = cpuTime();
    assert(solver->watches.get_smudged_list().empty());
    nbReduceDB++;
    CleaningStats tmpStats;
    tmpStats.origNumClauses = solver->longRedCls[1].size();
    tmpStats.origNumLits = solver->litStats.redLits;

    const uint64_t sumConfl = solver->sumConflicts();

    //move_to_longRedCls0();
    int64_t num_to_reduce = solver->longRedCls[1].size();

    //TODO maybe we chould count binary learnt clauses as well into the kept no. of clauses as other solvers do
    for(unsigned keep_type = 0; keep_type < 3; keep_type++) {
        const uint64_t keep_num = (double)num_to_reduce*solver->conf.ratio_keep_clauses[keep_type];
        if (keep_num == 0) {
            continue;
        }
        sort_red_cls(static_cast<ClauseClean>(keep_type));
        mark_top_N_clauses(keep_num);
    }
    assert(delayed_clause_free.empty());
    cl_marked = 0;
    cl_ttl = 0;
    cl_locked_solver = 0;
    remove_cl_from_array_and_count_stats(tmpStats, sumConfl);

    solver->clean_occur_from_removed_clauses_only_smudged();
    for(ClOffset offset: delayed_clause_free) {
        solver->cl_alloc.clauseFree(offset);
    }
    delayed_clause_free.clear();

    #ifdef SLOW_DEBUG
    solver->check_no_removed_or_freed_cl_in_watch();
    #endif

    tmpStats.cpu_time = cpuTime() - myTime;
    if (solver->conf.verbosity >= 4)
        tmpStats.print(0);
    else if (solver->conf.verbosity >= 3) {
        tmpStats.print_short(solver);
    } else if (solver->conf.verbosity >= 2) {
        cout << "c [DBclean]"
        << " marked: " << cl_marked
        << " ttl:" << cl_ttl
        << " locked_solver:" << cl_locked_solver
        << solver->conf.print_times(tmpStats.cpu_time)
        << endl;
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

void ReduceDB::mark_top_N_clauses(const uint64_t keep_num)
{
    size_t marked = 0;
    for(size_t i = 0
        ; i < solver->longRedCls[1].size() && marked < keep_num
        ; i++
    ) {
        const ClOffset offset = solver->longRedCls[1][i];
        Clause* cl = solver->cl_alloc.ptr(offset);
        if (cl->used_in_xor()
            || cl->stats.ttl > 0
            || solver->clause_locked(*cl, offset)
            || cl->stats.glue <= solver->conf.glue_must_keep_clause_if_below_or_eq
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

bool ReduceDB::cl_needs_removal(const Clause* cl, const ClOffset offset) const
{
    assert(cl->red());
    return !cl->used_in_xor()
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
        ; i < solver->longRedCls[1].size()
        ; i++
    ) {
        ClOffset offset = solver->longRedCls[1][i];
        Clause* cl = solver->cl_alloc.ptr(offset);
        assert(cl->size() > 3);

        if (cl->stats.marked_clause) {
            cl_marked++;
        } else if (cl->stats.ttl != 0) {
            cl_ttl++;
        } else if (solver->clause_locked(*cl, offset)) {
            cl_locked_solver++;
        }

        if (cl->stats.glue <= solver->conf.glue_must_keep_clause_if_below_or_eq) {
            solver->longRedCls[0].push_back(offset);
            continue;
        }

        if (!cl_needs_removal(cl, offset)) {
            cl->stats.ttl = 0;
            solver->longRedCls[1][j++] = offset;
            tmpStats.remain.incorporate(cl, sumConfl);
            cl->stats.marked_clause = 0;
            continue;
        }

        //Stats Update
        cl->setRemoved();
        solver->watches.smudge((*cl)[0]);
        solver->watches.smudge((*cl)[1]);
        tmpStats.removed.incorporate(cl, sumConfl);
        solver->litStats.redLits -= cl->size();

        *solver->drat << del << *cl << fin;
        delayed_clause_free.push_back(offset);
    }
    solver->longRedCls[1].resize(j);
}

void ReduceDB::reduce_db_and_update_reset_stats()
{
    solver->dump_memory_stats_to_sql();
    //ClauseUsageStats irred_cl_usage_stats = sumClauseData(solver->longIrredCls);
    //ClauseUsageStats red_cl_usage_stats;
    //red_cl_usage_stats += sumClauseData(solver->longRedCls[1]);

    /*ClauseUsageStats sum_cl_usage_stats;
    sum_cl_usage_stats += irred_cl_usage_stats;
    sum_cl_usage_stats += red_cl_usage_stats;
    if (solver->conf.verbosity >= 4) {
        cout << "c irred";
        irred_cl_usage_stats.print();

        cout << "c red  ";
        red_cl_usage_stats.print();

        cout << "c sum  ";
        sum_cl_usage_stats.print();
    }*/

    CleaningStats iterCleanStat = reduceDB();
    ClauseUsageStats stats0;
    ClauseUsageStats stats1;

    if (solver->sqlStats) {
        solver->sqlStats->reduceDB(stats0, stats1, iterCleanStat, solver);
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
