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

#ifndef __SQLSTATS_H__
#define __SQLSTATS_H__

#include "searcher.h"
#include "clause.h"
#include "cleaningstats.h"
#include "clauseusagestats.h"

namespace CMSat {

class Solver;

class SQLStats
{
public:

    virtual ~SQLStats()
    {}

    virtual void restart(
        const PropStats& thisPropStats
        , const SearchStats& thisStats
        , const Solver* solver
        , const Searcher* searcher
    ) = 0;

    virtual void time_passed(
        const Solver* solver
        , const string& name
        , double time_passed
        , bool time_out
        , double percent_time_remain
    ) = 0;

    virtual void time_passed_min(
        const Solver* solver
        , const string& name
        , double time_passed
    ) = 0;

     virtual void mem_used(
        const Solver* solver
        , const string& name
        , const double given_time
        , uint64_t mem_used_mb
    ) = 0;

    virtual void reduceDB(
        const ClauseUsageStats& irredStats
        , const ClauseUsageStats& redStats
        , const CleaningStats& clean
        , const Solver* solver
    ) = 0;

    virtual void dump_clause_stats(
        const Solver* solver
        , uint64_t clauseID
        , uint32_t glue
        , uint32_t backtrack_level
        , uint32_t size
        , AtecedentData<uint16_t> resoltypes
        , size_t decision_level
        , size_t propagation_level
        , double sum_vsids_vars
        , uint64_t conflicts_this_restart
    ) = 0;

    virtual bool setup(const Solver* solver) = 0;
    virtual void finishup(lbool status) = 0;
    uint64_t get_runID() const
    {
        return runID;
    }
    virtual void add_tag(const std::pair<std::string, std::string>& tag) = 0;

protected:

    void getRandomID();
    unsigned long runID;
};

} //end namespace

#endif //__SQLSTATS_H__
