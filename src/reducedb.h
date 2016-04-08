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
    void reduce_db_and_update_reset_stats();
    const CleaningStats& get_stats() const;

    uint64_t get_nbReduceDB() const
    {
        return nbReduceDB;
    }
    uint64_t nbReduceDB = 0;

private:
    Solver* solver;
    vector<ClOffset> delayed_clause_free;
    CleaningStats cleaningStats;

    unsigned cl_marked;
    unsigned cl_ttl;
    unsigned cl_locked_solver;

    size_t last_reducedb_num_conflicts = 0;
    bool red_cl_too_young(const Clause* cl) const;
    void clear_clauses_stats(vector<ClOffset>& clauseset);

    bool cl_needs_removal(const Clause* cl, const ClOffset offset) const;
    void remove_cl_from_array_and_count_stats(
        CleaningStats& tmpStats
        , uint64_t sumConflicts
    );

    CleaningStats reduceDB();
    void lock_most_UIP_used_clauses();

    void sort_red_cls(ClauseClean clean_type);
    void mark_top_N_clauses(const uint64_t keep_num);
    ClauseUsageStats sumClauseData(
        const vector<ClOffset>& toprint
    ) const;
};

inline const CleaningStats& ReduceDB::get_stats() const
{
    return cleaningStats;
}

}

#endif //__REDUCEDB_H__
