#ifndef __SQLSTATS_H__
#define __SQLSTATS_H__

#include "searcher.h"
#include "clause.h"
#include "cleaningstats.h"
#include "clauseusagestats.h"

#ifdef STATS_NEEDED_EXTRA
#include "boost/multi_array.hpp"
#endif

namespace CMSat {

class Solver;

class SQLStats
{
public:

    virtual ~SQLStats()
    {}

    virtual void restart(
        const PropStats& thisPropStats
        , const Searcher::Stats& thisStats
        , const VariableVariance& varVarStats
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

    #ifdef STATS_NEEDED_EXTRA
    virtual void clauseSizeDistrib(
        uint64_t sumConflicts
        , const vector<uint32_t>& sizes
    ) = 0;

    virtual void clauseGlueDistrib(
        uint64_t sumConflicts
        , const vector<uint32_t>& glues
    ) = 0;

    virtual void clauseSizeGlueScatter(
        uint64_t sumConflicts
        , boost::multi_array<uint32_t, 2>& sizeAndGlue
    ) = 0;

    virtual void varDataDump(
        const Solver* solver
        , const Searcher* search
        , const vector<Var>& varsToDump
        , const vector<VarData>& varData
    ) = 0;
    #endif

    virtual void reduceDB(
        const ClauseUsageStats& irredStats
        , const ClauseUsageStats& redStats
        , const CleaningStats& clean
        , const Solver* solver
    ) = 0;

    virtual bool setup(const Solver* solver) = 0;
    virtual void finishup(lbool status) = 0;
    uint64_t get_runID() const
    {
        return runID;
    }

protected:

    void getRandomID();
    unsigned long runID;
};

} //end namespace

#endif //__SQLSTATS_H__
