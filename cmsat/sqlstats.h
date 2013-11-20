#ifndef __SQLSTATS_H__
#define __SQLSTATS_H__

#include "searcher.h"
#include "clause.h"
#include "boost/multi_array.hpp"

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

    #ifdef STATS_NEEDED_EXTRA
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

    virtual void setup(const Solver* solver) = 0;

protected:

    void getRandomID();
    uint64_t runID;
};

} //end namespace

#endif //__SQLSTATS_H__
