#ifndef __SQLSTATS_H__
#define __SQLSTATS_H__

#include <mysql/mysql.h>
#include "searcher.h"
#include "clause.h"
#include "boost/multi_array.hpp"
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
    void clauseGlueDistrib(
        uint64_t sumConflicts
        , const vector<uint32_t>& glues
    );
    void clauseSizeGlueScatter(
        uint64_t sumConflicts
        , boost::multi_array<uint32_t, 2>& sizeAndGlue
    );

    void reduceDB(
        const ClauseUsageStats& irredStats
        , const ClauseUsageStats& redStats
        , const CleaningStats& clean
        , const Solver* solver
    );
    void setup(const Solver* solver);

private:

    struct StmtClsDistrib {
        StmtClsDistrib() :
            stmt(NULL)
        {}

        vector<MYSQL_BIND>  bind;
        MYSQL_STMT  *stmt;

        //Variables
        uint64_t sumConflicts;
        vector<uint64_t> value;
        vector<uint64_t> num;
    };

    void connectServer();
    void getID(const Solver* solver);
    bool tryIDInSQL(const Solver* solver);
    void getRandomID();

    void addFiles(const Solver* solver);
    void addStartupData(const Solver* solver);
    void initRestartSTMT(uint64_t verbosity);
    void initClauseDistribSTMT(
        const Solver* solver
        , StmtClsDistrib& stmt
        , const string& tableName
        , const string& valueName
        , const size_t numInserts
    );
    void initSizeGlueScatterSTMT(
        const Solver* solver
        , const size_t numInserts
    );
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

    struct StmtReduceDB {
        StmtReduceDB() :
            stmt(NULL)
        {};

        MYSQL_BIND  bind[16 + 5*3];
        MYSQL_STMT  *stmt;

        //Position
        uint64_t numSimplify;
        uint64_t sumRestarts;
        uint64_t sumConflicts;
        double cpuTime;
        uint64_t reduceDBs;

        //Actual data -- irred
        uint64_t irredClsVisited;
        uint64_t irredLitsVisited;
        uint64_t irredProps;
        uint64_t irredConfls;
        uint64_t irredUIP;

        //Actual data -- red
        uint64_t redClsVisited;
        uint64_t redLitsVisited;
        uint64_t redProps;
        uint64_t redConfls;
        uint64_t redUIP;

        //Cleaning stats
        CleaningStats clean;
    };
    StmtReduceDB stmtReduceDB;
    void initReduceDBSTMT(uint64_t verbosity);

    size_t bindAt;
    struct StmtRst {
        StmtRst() :
            stmt(NULL)
        {};

        MYSQL_BIND  bind[59];
        MYSQL_STMT  *stmt;

        //Position
        uint64_t numSimplify;
        uint64_t sumRestarts;
        uint64_t sumConflicts;
        double cpuTime;

        //Clause stats
        uint64_t numIrredBins;
        uint64_t numIrredTris;
        uint64_t numIrredLongs;
        uint64_t numIrredLits;
        uint64_t numRedBins;
        uint64_t numRedTris;
        uint64_t numRedLongs;
        uint64_t numRedLits;

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

    struct StmtSizeGlueScatter {
        StmtSizeGlueScatter() :
            stmt(NULL)
        {}
        vector<MYSQL_BIND>  bind;
        MYSQL_STMT  *stmt;

        //Variables
        uint64_t sumConflicts;
        vector<uint64_t> size;
        vector<uint64_t> glue;
        vector<uint64_t> num;
    };

    StmtRst stmtRst;
    StmtSizeGlueScatter stmtSizeGlueScatter;
    StmtClsDistrib stmtClsDistribSize;
    StmtClsDistrib stmtClsDistribGlue;

    MYSQL *serverConn;
};

#endif //__SQLSTATS_H__
