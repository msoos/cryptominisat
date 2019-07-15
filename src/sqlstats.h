/******************************************
Copyright (c) 2016, Mate Soos

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

#ifndef __SQLSTATS_H__
#define __SQLSTATS_H__

#include "clauseusagestats.h"
#include "satzilla_features.h"
#include "searchstats.h"
#include "vardata.h"

namespace CMSat {

class Solver;
class Searcher;
class Clause;

class SQLStats
{
public:

    virtual ~SQLStats()
    {}

    virtual void end_transaction() = 0;
    virtual void begin_transaction() = 0;

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

    virtual void satzilla_features(
        const Solver* solver
        , const Searcher* search
        , const SatZillaFeatures& satzilla_feat
    ) = 0;

    #ifdef STATS_NEEDED
    virtual void restart(
        const std::string& restart_type
        , const PropStats& thisPropStats
        , const SearchStats& thisStats
        , const Solver* solver
        , const Searcher* searcher
        , const rst_dat_type type
    ) = 0;

    virtual void reduceDB(
        const Solver* solver
        , const bool locked
        , const Clause* cl
        , const string& cur_restart_type
        , const uint32_t act_ranking_top_10
        , const uint32_t act_ranking
    ) = 0;

    virtual void var_data(
        const Solver* solver
        , const uint32_t var
        , const VarData& varData
        , const uint32_t cls_below
        , const uint64_t end_clid_notincl
    ) = 0;

    virtual void cl_last_in_solver(
        const Solver* solver
        , const uint64_t clid
    ) = 0;

    virtual void dump_clause_stats(
        const Solver* solver
        , uint64_t clid
        , uint32_t glue
        , uint32_t old_glue
        , uint32_t backtrack_level
        , uint32_t size
        , AtecedentData<uint16_t> resoltypes
        , size_t decision_level
        , size_t trail_depth
        , uint64_t conflicts_this_restart
        , const std::string& rest_type
        , const SearchHist& hist
        , const bool decision_cl
        , const bool ternary_resol_cl
    ) = 0;
    #endif

    virtual bool setup(const Solver* solver) = 0;
    virtual void finishup(lbool status) = 0;
    virtual void add_tag(const std::pair<std::string, std::string>& tag) = 0;
};

} //end namespace

#endif //__SQLSTATS_H__
