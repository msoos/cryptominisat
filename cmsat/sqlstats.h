#ifndef __SQLSTATS_H__
#define __SQLSTATS_H__

#include <mysql/mysql.h>
#include "Searcher.h"
class Solver;

class SQLStats
{
public:
    SQLStats();
    void restart(
        const PropStats& thisPropStats
        , const Searcher::Stats& thisStats
        , const Solver* solver
        , const Searcher* searcher
    );
    void clauseSizeDistrib(
        uint64_t sumConflicts
        , const vector<uint32_t>& sizes
    );
    void setup(const Solver* solver);

private:
    void connectServer();
    void getID();
    void addFiles(const Solver* solver);
    void initRestartSTMT(uint64_t verbosity);
    void initClauseSizeDistribSTMT(const Solver* solver);
    void writeQuestionMarks(size_t num, std::stringstream& ss);

    template<typename T>
    void bindTo(
        T& t
        , uint64_t& data
    ) {
        t.bind[bindAt].buffer_type= MYSQL_TYPE_LONG;
        t.bind[bindAt].buffer= (char *)&data;
        t.bind[bindAt].is_null= 0;
        t.bind[bindAt].length= 0;

        bindAt++;
    }

    template<typename T>
    void bindTo(
        T& t
        , double& data
    ) {
        t.bind[bindAt].buffer_type= MYSQL_TYPE_DOUBLE;
        t.bind[bindAt].buffer= (char *)&data;
        t.bind[bindAt].is_null= 0;
        t.bind[bindAt].length= 0;

        bindAt++;
    }

    uint64_t runID;
    size_t bindAt;
    struct StmtRst {
        MYSQL_BIND  bind[51];
        MYSQL_STMT  *STMT;

        //Variables
        uint64_t numSimplify;
        uint64_t sumRestarts;
        uint64_t sumConflicts;
        double cpuTime;

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
        uint64_t conflsTriIrred;
        uint64_t conflsTriRed;
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


    struct StmtClsDistrib {
        vector<MYSQL_BIND>  bind;
        MYSQL_STMT  *STMT;

        //Variables
        uint64_t sumConflicts;
        vector<uint64_t> size;
        vector<uint64_t> num;
    };
    StmtClsDistrib stmtClsDistrib;

    MYSQL *serverConn;
};

#endif //__SQLSTATS_H__
