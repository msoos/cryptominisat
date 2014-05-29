#ifndef __REDUCEDB_H__
#define __REDUCEDB_H__

#include "cleaningstats.h"
#include "clauseallocator.h"
#include "clauseusagestats.h"

namespace CMSat {

class Solver;

class ReduceDB
{
public:
    ReduceDB(Solver* solver);
    void reduce_db_and_update_reset_stats(bool lock_clauses_in = true);

private:
    Solver* solver;

    size_t last_reducedb_num_conflicts = 0;
    bool red_cl_too_young(const Clause* cl);
    bool red_cl_introduced_since_last_reducedb(const Clause* cl);
    void clear_clauses_stats(vector<ClOffset>& clauseset);
    CleaningStats reduceDB(bool lock_clauses_in);
    void lock_most_UIP_used_clauses();
    void lock_in_top_N_uncleaned();

    void real_clean_clause_db(
        CleaningStats& tmpStats
        , uint64_t sumConflicts
        , uint64_t removeNum
    );
    uint64_t calc_how_many_to_remove();
    void sort_red_cls(CleaningStats& tmpStats, ClauseCleaningTypes clean_type);
    void print_best_red_clauses_if_required() const;
    ClauseUsageStats sumClauseData(
        const vector<ClOffset>& toprint
        , bool red
    ) const;
};

}

#endif //__REDUCEDB_H__
