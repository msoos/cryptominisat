#include "sqlstats.h"
#include "SolverTypes.h"
#include "Solver.h"
#include "time_mem.h"
#include <sstream>
#include "VarReplacer.h"
#include "Simplifier.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


using std::cout;
using std::endl;

SQLStats::SQLStats(uint64_t verbosity) :
    bindAt(0)
{
    connectServer();

    randomID();
    initRestartSTMT(verbosity);
}

void SQLStats::connectServer()
{
    //Connection parameters
    char *server = "localhost";
    char *user = "root";
    char *password = "";
    char *database = "mysql";

    //Init MySQL library
    serverConn = mysql_init(NULL);

    //Connect to server
    if (!mysql_real_connect(
        serverConn
        , server
        , user
        , password
        , database
        , 0
        , NULL
        , 0)
    ) {
      cout
      << "ERROR while connectin to MySQL server:"
      << mysql_error(serverConn)
      << endl;

      exit(1);
    }
}

void SQLStats::randomID()
{
    //Generate random ID for SQL
    int randomData = open("/dev/urandom", O_RDONLY);
    if (randomData == -1) {
        cout << "Error reading from /dev/urandom !" << endl;
        exit(-1);
    }
    ssize_t ret = read(randomData, &runID, sizeof(runID));
    if (ret != sizeof(runID)) {
        cout << "Couldn't read from /dev/urandom!" << endl;
        exit(-1);
    }
    close(randomData);
    cout << "c runID: " << runID << endl;
}

void SQLStats::getID()
{
    //Inserting element into solverruns to get unique ID
    if (mysql_query(serverConn, "INSERT INTO solverruns VALUES()")) {
        cout << "Couldn't insert into table 'solverruns'" << endl;
        exit(1);
    }

    runID = mysql_insert_id(serverConn);
    cout << "This run number is: " << runID << endl;
}

void SQLStats::bindTo(uint64_t& data)
{
    stmtRst.bind[bindAt].buffer_type= MYSQL_TYPE_LONG;
    stmtRst.bind[bindAt].buffer= (char *)&data;
    stmtRst.bind[bindAt].is_null= 0;
    stmtRst.bind[bindAt].length= 0;

    bindAt++;
}

void SQLStats::bindTo(double& data)
{
    stmtRst.bind[bindAt].buffer_type= MYSQL_TYPE_DOUBLE;
    stmtRst.bind[bindAt].buffer= (char *)&data;
    stmtRst.bind[bindAt].is_null= 0;
    stmtRst.bind[bindAt].length= 0;

    bindAt++;
}

//Prepare statement for restart
void SQLStats::initRestartSTMT(
    uint64_t verbosity
) {
    std::stringstream ss;
    ss << "insert into `restart`"
    << "("
    //Position
    << "  `runID`, `simplifications`, `restarts`, `conflicts`, `time`"

    //Conflict stats
    << ", `glue`, `glueSD`, `size`, `sizeSD`"
    << ", `resolutions`, `resolutionsSD`"
    << ", `conflAfterConfl`, `conflAfterConflSD`"

    //Search stats
    << ", `branchDepth`, `branchDepthSD`"
    << ", `branchDepthDelta`, `branchDepthDeltaSD`"
    << ", `trailDepth`, `trailDepthSD`"
    << ", `trailDepthDelta`, `trailDepthDeltaSD`, `agility`"

    //Prop&confl&lerant
    << ", `propBinIrred` , `propBinRed` , `propTriIrred` , `propTriRed`"
    << ", `propLongIrred` , `propLongRed`"
    << ", `conflBinIrred`, `conflBinRed`, `conflTri`"
    << ", `conflLongIrred`, `conflLongRed`"
    << ", `learntUnits`, `learntBins`, `learntTris`, `learntLongs`"

    //Misc
    << ", `watchListSizeTraversed`, `watchListSizeTraversedSD`"
    << ", `litPropagatedSomething`, `litPropagatedSomethingSD`"

    //Var stats
    << ", `propsPerDec`"
    << ", `flippedPercent`, `varSetPos`, `varSetNeg`"
    << ", `free`, `replaced`, `eliminated`, `set`"
    << ")"
    << " values "
    << "(?, ?, );";

    stmtRst.STMT = mysql_stmt_init(serverConn);
    if (!stmtRst.STMT) {
        cout << "Error: mysql_stmt_init() out of memory" << endl;
        exit(1);
    }

    //const char* STMTTxt = "insert into literals(clindex,var,inv) values(?,?,?)";
    if (mysql_stmt_prepare(stmtRst.STMT, ss.str().c_str(), ss.str().length())) {
        cout << "Error in mysql_stmt_prepare(), INSERT failed" << endl
        << mysql_stmt_error(stmtRst.STMT) << endl;
        exit(0);
    }

    if (verbosity >= 6) {
        cout
        << "prepare INSERT successful"
        << endl;
    }

    // validate parameter count
    unsigned long param_count = mysql_stmt_param_count(stmtRst.STMT);
    if (param_count != 3) {
        cout
        << "invalid parameter count returned by MySQL"
        << endl;

        exit(1);
    }

    memset(stmtRst.bind, 0, sizeof(stmtRst.bind));


    //Position of solving
    bindTo(runID);
    bindTo(stmtRst.numSimplify);
    bindTo(stmtRst.sumRestarts);
    bindTo(stmtRst.sumConflicts);
    bindTo(stmtRst.cpuTime);

    //Conflict stats
    bindTo(stmtRst.glueHist);
    bindTo(stmtRst.glueHistSD);
    bindTo(stmtRst.conflSizeHist);
    bindTo(stmtRst.conflSizeHistSD);
    bindTo(stmtRst.numResolutionsHist);
    bindTo(stmtRst.numResolutionsHistSD);
    bindTo(stmtRst.conflictAfterConflict);
    bindTo(stmtRst.conflictAfterConflictSD);

    //Search stats
    bindTo(stmtRst.branchDepthHist);
    bindTo(stmtRst.branchDepthHistSD);
    bindTo(stmtRst.branchDepthDeltaHist);
    bindTo(stmtRst.branchDepthDeltaHistSD);
    bindTo(stmtRst.trailDepthHist);
    bindTo(stmtRst.trailDepthHistSD);
    bindTo(stmtRst.trailDepthDeltaHist);
    bindTo(stmtRst.trailDepthDeltaHistSD);
    bindTo(stmtRst.agilityHist);

    //Prop
    bindTo(stmtRst.propsBinIrred);
    bindTo(stmtRst.propsBinRed);
    bindTo(stmtRst.propsTriIrred);
    bindTo(stmtRst.propsTriRed);
    bindTo(stmtRst.propsLongIrred);
    bindTo(stmtRst.propsLongRed);

    //Confl
    bindTo(stmtRst.conflsBinIrred);
    bindTo(stmtRst.conflsBinRed);
    bindTo(stmtRst.conflsTri);
    bindTo(stmtRst.conflsLongIrred);
    bindTo(stmtRst.conflsLongRed);

    //Learnt
    bindTo(stmtRst.learntUnits);
    bindTo(stmtRst.learntBins);
    bindTo(stmtRst.learntTris);
    bindTo(stmtRst.learntLongs);

    //Misc
    bindTo(stmtRst.watchListSizeTraversed);
    bindTo(stmtRst.watchListSizeTraversedSD);
    bindTo(stmtRst.litPropagatedSomething);
    bindTo(stmtRst.litPropagatedSomethingSD);

    //Var stats
    bindTo(stmtRst.propagations);
    bindTo(stmtRst.decisions);
    bindTo(stmtRst.varFlipped);
    bindTo(stmtRst.varSetNeg);
    bindTo(stmtRst.varSetPos);
    bindTo(stmtRst.numFreeVars);
    bindTo(stmtRst.numReplacedVars);
    bindTo(stmtRst.numVarsElimed);
    bindTo(stmtRst.trailSize);

    // Bind the buffers
    if (mysql_stmt_bind_param(stmtRst.STMT, stmtRst.bind)) {
        cout << "mysql_stmt_bind_param() failed" << endl
        << mysql_stmt_error(stmtRst.STMT) << endl;
        exit(1);
    }


    //ANOTHER
/*
    insSTMTCl.STMT = mysql_stmt_init(conf.serverConn);
    if (!insSTMTCl.STMT) {
        cout << "Error: mysql_stmt_init() out of memory" << endl;
        exit(1);
    }

    const char* STMTTxt2 = "insert into clauses(runno, declevel, traillevel, glue, size, num, learnt) values(?,?,?,?,?,?,?)";
    if (mysql_stmt_prepare(insSTMTCl.STMT, STMTTxt2, strlen(STMTTxt2))) {
        cout << "Error in mysql_stmt_prepare(), INSERT failed" << endl
        << mysql_stmt_error(insSTMTCl.STMT) << endl;
        exit(0);
    }
    cout << "prepare INSERT successful" << endl;

    // Get the parameter count from the statement
    param_count = mysql_stmt_param_count(insSTMTCl.STMT);
    if (param_count != 7) { // validate parameter count
        cout << "invalid parameter count returned by MySQL" << endl;
        exit(1);
    }


    //RUNNO

    // Bind the buffers
    if (mysql_stmt_bind_param(insSTMTCl.STMT, insSTMTCl.bind)) {
        cout << "mysql_stmt_bind_param() failed" << endl
        << mysql_stmt_error(insSTMTCl.STMT) << endl;
        exit(1);
    }
    */
}

void SQLStats::printRestartSQL(
    const PropStats& thisPropStats
    , const Searcher::Stats& thisStats
    , const Solver* solver
    , const Searcher* search
) {
    //Position of solving
    stmtRst.numSimplify     = solver->getSolveStats().numSimplify;
    stmtRst.sumRestarts     = search->sumRestarts();
    stmtRst.sumConflicts    = search->sumConflicts();
    stmtRst.cpuTime         = cpuTime();

    //Conflict stats
    stmtRst.glueHist        = search->glueHist.getAvgMidLong();
    stmtRst.glueHistSD      = sqrt(search->glueHist.getVarMidLong());
    stmtRst.conflSizeHist   = search->conflSizeHist.getAvgMidLong();
    stmtRst.conflSizeHistSD = sqrt(search->conflSizeHist.getVarMidLong());
    stmtRst.numResolutionsHist =
        search->numResolutionsHist.getAvgMidLong();
    stmtRst.numResolutionsHistSD =
        sqrt(search->numResolutionsHist.getVarMidLong());
    stmtRst.conflictAfterConflict =
        search->conflictAfterConflict.getAvgMidLong()*100.0;
    stmtRst.conflictAfterConflictSD =
        sqrt(search->conflictAfterConflict.getVarMidLong())*100.0;

    //Search stats
    stmtRst.branchDepthHist         = search->branchDepthHist.getAvgMidLong();
    stmtRst.branchDepthHistSD       = sqrt(search->branchDepthHist.getVarMidLong());
    stmtRst.branchDepthDeltaHist    = search->branchDepthDeltaHist.getAvgMidLong();
    stmtRst.branchDepthDeltaHistSD  = sqrt(search->branchDepthDeltaHist.getVarMidLong());
    stmtRst.trailDepthHist          =  search->trailDepthHist.getAvgMidLong();
    stmtRst.trailDepthHistSD        = sqrt(search->trailDepthHist.getVarMidLong());
    stmtRst.trailDepthDeltaHist     = search->trailDepthDeltaHist.getAvgMidLong();
    stmtRst.trailDepthDeltaHistSD   = sqrt(search->trailDepthDeltaHist.getVarMidLong());
    stmtRst.agilityHist             = search->agilityHist.getAvgMidLong();

    //Prop
    stmtRst.propsBinIrred    = thisPropStats.propsBinIrred;
    stmtRst.propsBinRed      = thisPropStats.propsBinRed;
    stmtRst.propsTriIrred    = thisPropStats.propsTriIrred;
    stmtRst.propsTriRed      = thisPropStats.propsTriRed;
    stmtRst.propsLongIrred   = thisPropStats.propsLongIrred;
    stmtRst.propsLongRed     = thisPropStats.propsLongRed;

    //Confl
    stmtRst.conflsBinIrred  =  thisStats.conflStats.conflsBinIrred;
    stmtRst.conflsBinRed    = thisStats.conflStats.conflsBinRed;
    stmtRst.conflsTri       = thisStats.conflStats.conflsTri;
    stmtRst.conflsLongIrred = thisStats.conflStats.conflsLongIrred;
    stmtRst.conflsLongRed   = thisStats.conflStats.conflsLongRed;

    //Learnt
    stmtRst.learntUnits = thisStats.learntUnits;
    stmtRst.learntBins  = thisStats.learntBins;
    stmtRst.learntTris  = thisStats.learntTris;
    stmtRst.learntLongs = thisStats.learntLongs;

    //Misc
    stmtRst.watchListSizeTraversed   = search->watchListSizeTraversed.getAvgMidLong();
    stmtRst.watchListSizeTraversedSD = sqrt(search->watchListSizeTraversed.getVarMidLong());
    stmtRst.litPropagatedSomething   = search->litPropagatedSomething.getAvgMidLong()*100.0;
    stmtRst.litPropagatedSomethingSD = sqrt(search->litPropagatedSomething.getVarMidLong())*100.0;

    //Var stats
    stmtRst.decisions       = thisStats.decisions;
    stmtRst.propagations    = thisPropStats.propagations;
    stmtRst.varFlipped      = thisPropStats.varFlipped;
    stmtRst.varSetPos       = thisPropStats.varSetPos;
    stmtRst.varSetNeg       = thisPropStats.varSetNeg;
    stmtRst.numFreeVars     = solver->getNumFreeVarsAdv(search->trail.size());
    stmtRst.numReplacedVars = solver->varReplacer->getNumReplacedVars();
    stmtRst.numVarsElimed   = solver->subsumer->getStats().numVarsElimed;
    stmtRst.trailSize       = search->trail.size();
}

