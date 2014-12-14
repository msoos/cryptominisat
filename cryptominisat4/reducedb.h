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

    void reset();
    void reset_increment();
    void increment_for_next_reduce();
    const CleaningStats& get_cleaning_stats() const;

    uint64_t get_nbReduceDB() const
    {
        return nbReduceDB;
    }
    uint64_t get_nextCleanLimit() const
    {
        return nextCleanLimit;
    }

private:
    Solver* solver;
    uint64_t nbReduceDB = 0;
    uint64_t nextCleanLimit = 0;
    uint64_t nextCleanLimitInc;
    vector<ClOffset> delayed_clause_free;
    CleaningStats cleaningStats;

    size_t last_reducedb_num_conflicts = 0;
    bool red_cl_too_young(const Clause* cl) const;
    void clear_clauses_stats(vector<ClOffset>& clauseset);

    bool cl_needs_removal(const Clause* cl, const ClOffset offset) const;
    void remove_cl_from_watchlists();
    void remove_cl_from_array_and_count_stats(
        CleaningStats& tmpStats
        , uint64_t sumConflicts
    );

    CleaningStats reduceDB(bool lock_clauses_in);
    void lock_most_UIP_used_clauses();

    void sort_red_cls(ClauseCleaningTypes clean_type);
    void mark_top_N_clauses(const uint64_t keep_num);
    void print_best_red_clauses_if_required() const;
    ClauseUsageStats sumClauseData(
        const vector<ClOffset>& toprint
        , bool red
    ) const;
};

inline const CleaningStats& ReduceDB::get_cleaning_stats() const
{
    return cleaningStats;
}

}

#endif //__REDUCEDB_H__
