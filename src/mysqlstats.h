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

#include "sqlstats.h"

#ifdef _MSC_VER
#define NOMINMAX
#include <windows.h>
#include <mysql.h>
#else
#include <mysql/mysql.h>
#endif

using namespace CMSat;


class MySQLStats: public SQLStats
{
public:
    MySQLStats();
    ~MySQLStats() override;

    void restart(
        const PropStats& thisPropStats
        , const Searcher::Stats& thisStats
        , const Solver* solver
        , const Searcher* searcher
    ) override;

    void reduceDB(
        const ClauseUsageStats& irredStats
        , const ClauseUsageStats& redStats
        , const CleaningStats& clean
        , const Solver* solver
    ) override;

    void time_passed(
        const Solver* solver
        , const string& name
        , double time_passed
        , bool time_out
        , double percent_time_remain
    ) override;

    void time_passed_min(
        const Solver* solver
        , const string& name
        , double time_passed
    ) override;

    void mem_used(
        const Solver* solver
        , const string& name
        , double given_time
        , uint64_t mem_used_mb
    ) override;

    bool setup(const Solver* solver) override;
    void finishup(lbool status) override;

private:

    bool connectServer(const Solver* solver);
    void getID(const Solver* solver);
    bool tryIDInSQL(const Solver* solver);
    void add_tags(const Solver* solver);

    void addStartupData(const Solver* solver);
    void initRestartSTMT();
    void initTimePassedSTMT();
    void initMemUsedSTMT();
    void initTimePassedMinSTMT();

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

    template<typename T>
    void bindTo(
        T& t
        , char* str
        , unsigned long* str_len
    ) {
        t.bind[bindAt].buffer_type= MYSQL_TYPE_STRING;
        t.bind[bindAt].buffer= str;
        t.bind[bindAt].buffer_length = 200;
        t.bind[bindAt].is_null= 0;
        t.bind[bindAt].length= str_len;

        bindAt++;
    }

    struct StmtReduceDB {
        StmtReduceDB() :
            stmt(NULL)
        {}

        MYSQL_BIND  bind[10 + 14*2];
        MYSQL_STMT  *stmt = NULL;

        //Position
        uint64_t numSimplify;
        uint64_t sumRestarts;
        uint64_t sumConflicts;
        double cpuTime;
        uint64_t reduceDBs;

        //Actual data -- irred
        uint64_t irredClsVisited;
        uint64_t irredLitsVisited;

        //Actual data -- red
        uint64_t redClsVisited;
        uint64_t redLitsVisited;

        //Cleaning stats
        CleaningStats clean;
    };
    StmtReduceDB stmtReduceDB;
    void initReduceDBSTMT();

    struct StmtTimePassed {
        StmtTimePassed() :
            stmt(NULL)
        {}

        MYSQL_BIND  bind[1+7];
        MYSQL_STMT  *stmt = NULL;

        uint64_t numSimplify;
        uint64_t sumConflicts;
        double cpuTime;
        char name[200];
        unsigned long name_len;
        double time_passed;
        uint64_t time_out;
        double percent_time_remain;
    };
    StmtTimePassed stmtTimePassed;

    struct StmtMemUsed {
        StmtMemUsed() :
            stmt(NULL)
        {}

        MYSQL_BIND  bind[1+5];
        MYSQL_STMT  *stmt = NULL;

        uint64_t numSimplify;
        uint64_t sumConflicts;
        double cpuTime;
        char name[200];
        unsigned long name_len;
        uint64_t mem_used_mb;
    };
    StmtMemUsed stmtMemUsed;

    struct StmtTimePassedMin {
        StmtTimePassedMin() :
            stmt(NULL)
        {}

        MYSQL_BIND  bind[1+5];
        MYSQL_STMT  *stmt = NULL;

        uint64_t numSimplify;
        uint64_t sumConflicts;
        double cpuTime;
        char name[200];
        unsigned long name_len;
        double time_passed;
    };
    StmtTimePassedMin stmtTimePassedMin;

    size_t bindAt;
    struct StmtRst {
        StmtRst() :
            stmt(NULL)
        {}

        MYSQL_BIND  bind[70+1]; //+1 == runID
        MYSQL_STMT  *stmt = NULL;

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
        uint64_t glueHistMin;
        uint64_t glueHistMax;

        double conflSizeHist;
        double conflSizeHistSD;
        uint64_t conflSizeHistMin;
        uint64_t conflSizeHistMax;

        double numResolutionsHist;
        double numResolutionsHistSD;
        uint64_t numResolutionsHistMin;
        uint64_t numResolutionsHistMax;

        double conflictAfterConflict;

        //Search stats
        double branchDepthHist;
        double branchDepthHistSD;
        uint64_t branchDepthHistMin;
        uint64_t branchDepthHistMax;

        double branchDepthDeltaHist;
        double branchDepthDeltaHistSD;
        uint64_t branchDepthDeltaHistMin;
        uint64_t branchDepthDeltaHistMax;

        double trailDepthHist;
        double trailDepthHistSD;
        uint64_t trailDepthHistMin;
        uint64_t trailDepthHistMax;

        double trailDepthDeltaHist;
        double trailDepthDeltaHistSD;
        uint64_t trailDepthDeltaHistMin;
        uint64_t trailDepthDeltaHistMax;

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

        //Red
        uint64_t learntUnits;
        uint64_t learntBins;
        uint64_t learntTris;
        uint64_t learntLongs;

        //Resolution stats
        ResolutionTypes<uint64_t> resolv;

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
    bool setup_ok = false;
};
