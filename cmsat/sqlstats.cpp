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
#include <string>
#include <time.h>

using std::cout;
using std::endl;
using std::string;

SQLStats::SQLStats() :
    bindAt(0)
{
}

void SQLStats::setup(const Solver* solver)
{
    connectServer();
    getID();
    addFiles(solver);
    initRestartSTMT(solver->conf.verbosity);
}

void SQLStats::connectServer()
{
    //Connection parameters
    string server = "localhost";
    string user = "cryptomsuser";
    string password = "";
    string database = "cryptoms";

    //Init MySQL library
    serverConn = mysql_init(NULL);
    if (!serverConn) {
        cout
        << "Insufficient memory to allocate server connection"
        << endl;

        exit(-1);
    }

    //Connect to server
    if (!mysql_real_connect(
        serverConn
        , server.c_str()
        , user.c_str()
        , password.c_str()
        , database.c_str()
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

void SQLStats::getID()
{

    std::stringstream ss;
    ss
    << "INSERT INTO solverRun (version, time) values ("
    << "\"" << Solver::getVersion() << "\""
    << ", " << time(NULL)
    << ");";

    //Inserting element into solverruns to get unique ID
    if (mysql_query(serverConn, ss.str().c_str())) {
        cout << "Couldn't insert into table 'solverruns'" << endl;
        exit(1);
    }

    runID = mysql_insert_id(serverConn);
    cout << "This run number is: " << runID << endl;
}

void SQLStats::addFiles(const Solver* solver)
{
    for(vector<string>::const_iterator
        it = solver->fileNamesUsed.begin(), end = solver->fileNamesUsed.end()
        ; it != end
        ; it++
    ) {

        std::stringstream ss;
        ss
        << "INSERT INTO fileNamesUsed (runID, fileName) VALUES"
        <<"(" << runID << ", \"" << *it << "\");";

        //Inserting element into solverruns to get unique ID
        if (mysql_query(serverConn, ss.str().c_str())) {
            cout << "Couldn't insert into table 'solverruns'" << endl;
            exit(1);
        }
    }
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

    //Propagations
    << ", `propBinIrred` , `propBinRed` , `propTriIrred` , `propTriRed`"
    << ", `propLongIrred` , `propLongRed`"

    //Conflicts
    << ", `conflBinIrred`, `conflBinRed`, `conflTri`"
    << ", `conflLongIrred`, `conflLongRed`"

    //Learnts
    << ", `learntUnits`, `learntBins`, `learntTris`, `learntLongs`"

    //Misc
    << ", `watchListSizeTraversed`, `watchListSizeTraversedSD`"
    << ", `litPropagatedSomething`, `litPropagatedSomethingSD`"

    //Var stats
    << ", `propagations`"
    << ", `decisions`"
    << ", `flipped`, `varSetPos`, `varSetNeg`"
    << ", `free`, `replaced`, `eliminated`, `set`"
    << ")"
    << " values (";

    const size_t num = sizeof(stmtRst.bind)/sizeof(MYSQL_BIND);
    for(size_t i = 0
        ; i < num
        ; i++
    ) {
        if (i < num-1)
            ss << "?,";
        else
            ss << "?";
    }
    ss << ");";

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
    if (param_count != sizeof(stmtRst.bind)/sizeof(MYSQL_BIND)) {
        cout
        << "invalid parameter count returned by MySQL"
        << endl;

        exit(1);
    }

    memset(stmtRst.bind, 0, sizeof(stmtRst.bind));


    //Position of solving
    assert(bindAt == 0);
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
    bindTo(stmtRst.varSetPos);
    bindTo(stmtRst.varSetNeg);
    bindTo(stmtRst.numFreeVars);
    bindTo(stmtRst.numReplacedVars);
    bindTo(stmtRst.numVarsElimed);
    bindTo(stmtRst.trailSize);
    assert(bindAt == sizeof(stmtRst.bind)/sizeof(MYSQL_BIND));

    // Bind the buffers
    if (mysql_stmt_bind_param(stmtRst.STMT, stmtRst.bind)) {
        cout << "mysql_stmt_bind_param() failed" << endl
        << mysql_stmt_error(stmtRst.STMT) << endl;
        exit(1);
    }
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
    stmtRst.trailDepthHist          = search->trailDepthHist.getAvgMidLong();
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

    if (mysql_stmt_execute(stmtRst.STMT)) {
        cout
        << "ERROR: while executing restart insertion MySQL prepared statement"
        << endl;

        cout << "Error from mysql: "
        << mysql_stmt_error(stmtRst.STMT)
        << endl;

        exit(-1);
    }
}

