/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2014, Mate Soos. All rights reserved.
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

#ifdef STATS_NEEDED
struct SortRedClsPropConfl
{
    SortRedClsPropConfl(
        ClauseAllocator& _cl_alloc
        , double _confl_weight
        , double _prop_weight
    ) :
        cl_alloc(_cl_alloc)
        , confl_weight(_confl_weight)
        , prop_weight(_prop_weight)
    {}
    ClauseAllocator& cl_alloc;
    double confl_weight;
    double prop_weight;

    bool operator () (const ClOffset xOff, const ClOffset yOff) const
    {
        const Clause* x = cl_alloc.ptr(xOff);
        const Clause* y = cl_alloc.ptr(yOff);

        const uint32_t xsize = x->size();
        const uint32_t ysize = y->size();

        //No clause should be less than 3-long: 2&3-long are not removed
        assert(xsize > 2 && ysize > 2);

        const double x_useful = x->stats.weighted_prop_and_confl(prop_weight, confl_weight);
        const double y_useful = y->stats.weighted_prop_and_confl(prop_weight, confl_weight);
        if (x_useful != y_useful) {
            return x_useful > y_useful;
        }

        //Second tie: UIP usage
        if (x->stats.used_for_uip_creation != y->stats.used_for_uip_creation) {
            return x->stats.used_for_uip_creation > y->stats.used_for_uip_creation;
        }

        return x->size() < y->size();
    }
};

struct SortRedClsConflDepth
{
    SortRedClsConflDepth(ClauseAllocator& _cl_alloc) :
        cl_alloc(_cl_alloc)
    {}
    ClauseAllocator& cl_alloc;

    bool operator () (const ClOffset xOff, const ClOffset yOff) const
    {
        const Clause* x = cl_alloc.ptr(xOff);
        const Clause* y = cl_alloc.ptr(yOff);

        const uint32_t xsize = x->size();
        const uint32_t ysize = y->size();

        //No clause should be less than 3-long: 2&3-long are not removed
        assert(xsize > 2 && ysize > 2);

        if (x->stats.weighted_prop_and_confl(1.0, 1.0) == 0 && y->stats.weighted_prop_and_confl(1.0, 1.0) == 0)
            return false;

        if (y->stats.weighted_prop_and_confl(1.0, 1.0) == 0)
            return true;
        if (x->stats.weighted_prop_and_confl(1.0, 1.0) == 0)
            return false;

        const double x_useful = x->stats.calc_usefulness_depth();
        const double y_useful = y->stats.calc_usefulness_depth();
        if (x_useful != y_useful)
            return x_useful > y_useful;

        //Second tie: UIP usage
        if (x->stats.used_for_uip_creation != y->stats.used_for_uip_creation)
            return x->stats.used_for_uip_creation > y->stats.used_for_uip_creation;

        return x->size() < y->size();
    }
};
#endif

ReduceDB::ReduceDB(Solver* _solver) :
    solver(_solver)
{
}

void ReduceDB::sort_red_cls(ClauseCleaningTypes clean_type)
{
    switch (clean_type) {
        case ClauseCleaningTypes::clean_glue_based : {
            std::sort(solver->longRedCls.begin(), solver->longRedCls.end(), SortRedClsGlue(solver->cl_alloc));
            break;
        }

        case ClauseCleaningTypes::clean_size_based : {
            std::sort(solver->longRedCls.begin(), solver->longRedCls.end(), SortRedClsSize(solver->cl_alloc));
            break;
        }

        case ClauseCleaningTypes::clean_sum_activity_based : {
            std::sort(solver->longRedCls.begin(), solver->longRedCls.end(), SortRedClsAct(solver->cl_alloc));
            break;
        }

        #ifdef STATS_NEEDED
        case ClauseCleaningTypes::clean_sum_prop_confl_based : {
            std::sort(solver->longRedCls.begin()
                , solver->longRedCls.end()
                , SortRedClsPropConfl(solver->cl_alloc
                    , solver->conf.clean_confl_multiplier
                    , solver->conf.clean_prop_multiplier
                )
            );
            break;
        }

        case ClauseCleaningTypes::clean_sum_confl_depth_based : {
            std::sort(solver->longRedCls.begin(), solver->longRedCls.end(), SortRedClsConflDepth(solver->cl_alloc));
            break;
        }
        #endif

        default: {
            cout << "Unknown cleaning type: " << clean_type << endl;
            assert(false);
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

CleaningStats ReduceDB::reduceDB(bool lock_clauses_in)
{
    const double myTime = cpuTime();
    nbReduceDB++;
    CleaningStats tmpStats;
    tmpStats.origNumClauses = solver->longRedCls.size();
    tmpStats.origNumLits = solver->litStats.redLits;

    const uint64_t sumConfl = solver->sumConflicts();

    if (lock_clauses_in) {
        lock_most_UIP_used_clauses();
    }

    move_to_never_cleaned();
    int64_t num_to_reduce = solver->longRedCls.size();

    //TODO maybe we chould count binary learnt clauses as well into the kept no. of clauses as other solvers do
    for(unsigned keep_type = 0; keep_type < 3; keep_type++) {
        const uint64_t keep_num = (double)num_to_reduce*solver->conf.ratio_keep_clauses[keep_type];
        if (keep_num == 0) {
            continue;
        }
        sort_red_cls(static_cast<ClauseCleaningTypes>(keep_type));
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
        tmpStats.print();
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

void ReduceDB::lock_most_UIP_used_clauses()
{
    if (solver->conf.lock_uip_per_dbclean == 0)
        return;

    std::function<bool (const ClOffset, const ClOffset)> uipsort
        = [&] (const ClOffset a, const ClOffset b) -> bool {
            const Clause& a_cl = *solver->cl_alloc.ptr(a);
            const Clause& b_cl = *solver->cl_alloc.ptr(b);

            return a_cl.stats.used_for_uip_creation > b_cl.stats.used_for_uip_creation;
    };
    std::sort(solver->longRedCls.begin(), solver->longRedCls.end(), uipsort);

    size_t locked = 0;
    size_t skipped = 0;
    for(size_t i = 0
        ; i < solver->longRedCls.size() && locked < solver->conf.lock_uip_per_dbclean
        ; i++
    ) {
        const ClOffset offs = solver->longRedCls[i];
        Clause& cl = *solver->cl_alloc.ptr(offs);
        if (!cl.stats.locked
            && cl.stats.glue > solver->conf.glue_must_keep_clause_if_below_or_eq
        ) {
            cl.stats.locked = true;
            locked++;
            //std::cout << "Locked: " << cl << endl;
        } else {
            skipped++;
            //std::cout << "skipped: " << cl << endl;
        }
    }

    if (solver->conf.verbosity >= 2) {
        cout << "c [DBclean] UIP"
        << " Locked: " << locked << " skipped: " << skipped << endl;
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
            tmpStats.remain.incorporate(cl);
            #ifdef STATS_NEEDED
            tmpStats.remain.age += sumConfl - cl->stats.introduced_at_conflict;
            #endif
            continue;
        }

        //Stats Update
        cl->setRemoved();
        solver->watches.smudge((*cl)[0]);
        solver->watches.smudge((*cl)[1]);
        tmpStats.removed.incorporate(cl);
        #ifdef STATS_NEEDED
        tmpStats.removed.age += sumConfl - cl->stats.introduced_at_conflict;
        #endif
        solver->litStats.redLits -= cl->size();

        *solver->drup << del << *cl << fin;
        delayed_clause_free.push_back(offset);
    }
    solver->longRedCls.resize(solver->longRedCls.size() - (i - j));
}

void ReduceDB::reduce_db_and_update_reset_stats(bool lock_clauses_in)
{
    ClauseUsageStats irred_cl_usage_stats = sumClauseData(solver->longIrredCls, false);
    ClauseUsageStats red_cl_usage_stats = sumClauseData(solver->longRedCls, true);
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

    CleaningStats iterCleanStat = reduceDB(lock_clauses_in);

    if (solver->sqlStats) {
        solver->sqlStats->reduceDB(irred_cl_usage_stats, red_cl_usage_stats, iterCleanStat, solver);
    }

    if (solver->conf.doClearStatEveryClauseCleaning) {
        solver->clear_clauses_stats();
    }
}

ClauseUsageStats ReduceDB::sumClauseData(
    const vector<ClOffset>& toprint
    , const bool red
) const {
    vector<ClauseUsageStats> perSizeStats;
    vector<ClauseUsageStats> perGlueStats;

    //Reset stats
    ClauseUsageStats stats;

    for(ClOffset offset: toprint) {
        Clause& cl = *solver->cl_alloc.ptr(offset);
        const uint32_t clause_size = cl.size();

        //We have stats on this clause
        if (cl.size() == 3)
            continue;

        stats.addStat(cl);

        //Update size statistics
        if (perSizeStats.size() < cl.size() + 1U)
            perSizeStats.resize(cl.size()+1);

        perSizeStats[clause_size].addStat(cl);

        //If redundant, sum up GLUE-based stats
        if (red) {
            const size_t glue = cl.stats.glue;
            assert(glue != std::numeric_limits<uint32_t>::max());
            if (perSizeStats.size() < glue + 1) {
                perSizeStats.resize(glue + 1);
            }

            perSizeStats[glue].addStat(cl);
        }

        if (solver->conf.verbosity >= 6)
            cl.print_extra_stats();
    }

    //Print more stats
    /*if (solver->conf.verbosity >= 4) {
        solver->print_prop_confl_stats("clause-len", perSizeStats);

        if (red) {
            solver->print_prop_confl_stats("clause-glue", perGlueStats);
        }
    }*/

    return stats;
}
