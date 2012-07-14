#ifndef __SQLSTATS_H__
#define __SQLSTATS_H__

#include <mysql/mysql.h>
#include "Searcher.h"
class Solver;

class SQLStats
{
public:
    SQLStats();
    void printRestartSQL(
        const PropStats& thisPropStats
        , const Searcher::Stats& thisStats
        , const Solver* solver
        , const Searcher* searcher
    );
    void setup(const Solver* solver);

private:
    void connectServer();
    void getID();
    void addFiles(const Solver* solver);
    void initRestartSTMT(uint64_t verbosity);

    void bindTo(uint64_t& data);
    void bindTo(double& data);

    uint64_t runID;
    size_t bindAt;
    struct StmtRst {
        MYSQL_BIND  bind[50];
        MYSQL_STMT  *STMT;

        //Variables
        uint64_t numSimplify;
        uint64_t sumRestarts;
        uint64_t sumConflicts;
        uint64_t cpuTime;

        //Conflict stats
        double glueHist;
        double glueHistSD;
        double conflSizeHist;
        double conflSizeHistSD;
        double numResolutionsHist;
        double numResolutionsHistSD;
        double conflictAfterConflict;
        double conflictAfterConflictSD;

        //Search stats
        double branchDepthHist;
        double branchDepthHistSD;
        double branchDepthDeltaHist;
        double branchDepthDeltaHistSD;
        double trailDepthHist;
        double trailDepthHistSD;
        double trailDepthDeltaHist;
        double trailDepthDeltaHistSD;
        double agilityHist;

        //Prop
        uint64_t propsBinIrred;
        uint64_t propsBinRed;
        uint64_t propsTriIrred;
        uint64_t propsTriRed;
        uint64_t propsLongIrred;
        uint64_t propsLongRed;

        //Confl
        uint64_t conflsBinIrred;
        uint64_t conflsBinRed;
        uint64_t conflsTri;
        uint64_t conflsLongIrred;
        uint64_t conflsLongRed;

        //Learnt
        uint64_t learntUnits;
        uint64_t learntBins;
        uint64_t learntTris;
        uint64_t learntLongs;

        //Misc
        double watchListSizeTraversed;
        double watchListSizeTraversedSD;
        double litPropagatedSomething;
        double litPropagatedSomethingSD;

        //Var stats
        uint64_t propagations;
        uint64_t decisions;
        uint64_t varFlipped;
        uint64_t varSetNeg;
        uint64_t varSetPos;
        uint64_t numFreeVars;
        uint64_t numReplacedVars;
        uint64_t numVarsElimed;
        uint64_t trailSize;

    };
    StmtRst stmtRst;
    MYSQL *serverConn;
};

#endif //__SQLSTATS_H__
