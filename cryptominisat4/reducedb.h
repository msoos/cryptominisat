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
    CleaningStats cleaningStats;

    void reset_for_next_clean_limit();
    void increment_for_next_reduce();

    uint64_t get_nbReduceDB() const
    {
        return nbReduceDB;
    }
    uint64_t get_nextCleanLimit() const
    {
        return nextCleanLimit;
    }
    uint64_t get_nextCleanLimitInc() const
    {
        assert(nextCleanLimitInc >= 1);
        return nextCleanLimitInc;
    }

private:
    Solver* solver;
    uint64_t nbReduceDB = 0;
    uint64_t nextCleanLimit = 0;
    uint64_t nextCleanLimitInc;

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
