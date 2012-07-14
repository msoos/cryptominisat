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
    initClauseSizeDistribSTMT(solver);
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

void SQLStats::writeQuestionMarks(
    size_t num
    , std::stringstream& ss
) {
    ss << "(";
    for(size_t i = 0
        ; i < num
        ; i++
    ) {
        if (i < num-1)
            ss << "?,";
        else
            ss << "?";
    }
    ss << ")";
}

void SQLStats::initClauseSizeDistribSTMT(
    const Solver* solver
) {
    const size_t numInserts = solver->conf.dumpClauseDistribMax;
    const size_t numElems = 4;
    stmtClsDistrib.bind.resize(numElems*numInserts);

    std::stringstream ss;
    ss << "insert into `clauseSizeDistrib`"
    << "("
    << " `runID`, `conflicts`, `size`, `num`"
    << ") values ";
    for(size_t i = 0; i < numInserts; i++) {
        writeQuestionMarks(numElems, ss);

        if (i != numInserts-1)
            ss << " , ";
    }
    ss << ";";

    //Get memory for statement
    stmtClsDistrib.STMT = mysql_stmt_init(serverConn);
    if (!stmtClsDistrib.STMT) {
        cout << "Error: mysql_stmt_init() out of memory" << endl;
        exit(1);
    }

    //Prepare the statement
    if (mysql_stmt_prepare(stmtClsDistrib.STMT, ss.str().c_str(), ss.str().length())) {
        cout << "Error in mysql_stmt_prepare(), INSERT failed" << endl
        << mysql_stmt_error(stmtClsDistrib.STMT) << endl;
        exit(0);
    }

    if (solver->conf.verbosity >= 6) {
        cout
        << "prepare INSERT successful"
        << endl;
    }

    //Validate parameter count
    unsigned long param_count = mysql_stmt_param_count(stmtClsDistrib.STMT);
    if (param_count != numElems*numInserts) {
        cout
        << "invalid parameter count returned by MySQL"
        << endl;

        exit(1);
    }

    //Clear mem of bind, get enough mem for vars
    memset(stmtClsDistrib.bind.data(), 0, stmtClsDistrib.bind.size());
    stmtClsDistrib.size.resize(numInserts);
    stmtClsDistrib.num.resize(numInserts);

    //Bind the local variables to the statement
    bindAt = 0;
    for(size_t i = 0; i < numInserts; i++) {
        bindTo(stmtClsDistrib, runID);
        bindTo(stmtClsDistrib, stmtClsDistrib.sumConflicts);
        bindTo(stmtClsDistrib, stmtClsDistrib.size[i]);
        bindTo(stmtClsDistrib, stmtClsDistrib.num[i]);
    }
    assert(bindAt == numElems*numInserts);

    //Bind the buffers
    if (mysql_stmt_bind_param(stmtClsDistrib.STMT, stmtClsDistrib.bind.data())) {
        cout << "mysql_stmt_bind_param() failed" << endl
        << mysql_stmt_error(stmtClsDistrib.STMT) << endl;
        exit(1);
    }
}

//Prepare statement for restart
void SQLStats::initRestartSTMT(
    uint64_t verbosity
) {
    const size_t numElems = sizeof(stmtRst.bind)/sizeof(MYSQL_BIND);

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
    << ") values ";
    writeQuestionMarks(
        numElems
        , ss
    );
    ss << ";";

    //Get memory for statement
    stmtRst.STMT = mysql_stmt_init(serverConn);
    if (!stmtRst.STMT) {
        cout << "Error: mysql_stmt_init() out of memory" << endl;
        exit(1);
    }

    //Prepare the statement
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

    //Validate parameter count
    unsigned long param_count = mysql_stmt_param_count(stmtRst.STMT);
    if (param_count != numElems) {
        cout
        << "invalid parameter count returned by MySQL"
        << endl;

        exit(1);
    }

    memset(stmtRst.bind, 0, sizeof(stmtRst.bind));


    //Bind the local variables to the statement
    bindAt =0;
    bindTo(stmtRst, runID);
    bindTo(stmtRst, stmtRst.numSimplify);
    bindTo(stmtRst, stmtRst.sumRestarts);
    bindTo(stmtRst, stmtRst.sumConflicts);
    bindTo(stmtRst, stmtRst.cpuTime);

    //Conflict stats
    bindTo(stmtRst, stmtRst.glueHist);
    bindTo(stmtRst, stmtRst.glueHistSD);
    bindTo(stmtRst, stmtRst.conflSizeHist);
    bindTo(stmtRst, stmtRst.conflSizeHistSD);
    bindTo(stmtRst, stmtRst.numResolutionsHist);
    bindTo(stmtRst, stmtRst.numResolutionsHistSD);
    bindTo(stmtRst, stmtRst.conflictAfterConflict);
    bindTo(stmtRst, stmtRst.conflictAfterConflictSD);

    //Search stats
    bindTo(stmtRst, stmtRst.branchDepthHist);
    bindTo(stmtRst, stmtRst.branchDepthHistSD);
    bindTo(stmtRst, stmtRst.branchDepthDeltaHist);
    bindTo(stmtRst, stmtRst.branchDepthDeltaHistSD);
    bindTo(stmtRst, stmtRst.trailDepthHist);
    bindTo(stmtRst, stmtRst.trailDepthHistSD);
    bindTo(stmtRst, stmtRst.trailDepthDeltaHist);
    bindTo(stmtRst, stmtRst.trailDepthDeltaHistSD);
    bindTo(stmtRst, stmtRst.agilityHist);

    //Prop
    bindTo(stmtRst, stmtRst.propsBinIrred);
    bindTo(stmtRst, stmtRst.propsBinRed);
    bindTo(stmtRst, stmtRst.propsTriIrred);
    bindTo(stmtRst, stmtRst.propsTriRed);
    bindTo(stmtRst, stmtRst.propsLongIrred);
    bindTo(stmtRst, stmtRst.propsLongRed);

    //Confl
    bindTo(stmtRst, stmtRst.conflsBinIrred);
    bindTo(stmtRst, stmtRst.conflsBinRed);
    bindTo(stmtRst, stmtRst.conflsTri);
    bindTo(stmtRst, stmtRst.conflsLongIrred);
    bindTo(stmtRst, stmtRst.conflsLongRed);

    //Learnt
    bindTo(stmtRst, stmtRst.learntUnits);
    bindTo(stmtRst, stmtRst.learntBins);
    bindTo(stmtRst, stmtRst.learntTris);
    bindTo(stmtRst, stmtRst.learntLongs);

    //Misc
    bindTo(stmtRst, stmtRst.watchListSizeTraversed);
    bindTo(stmtRst, stmtRst.watchListSizeTraversedSD);
    bindTo(stmtRst, stmtRst.litPropagatedSomething);
    bindTo(stmtRst, stmtRst.litPropagatedSomethingSD);

    //Var stats
    bindTo(stmtRst, stmtRst.propagations);
    bindTo(stmtRst, stmtRst.decisions);
    bindTo(stmtRst, stmtRst.varFlipped);
    bindTo(stmtRst, stmtRst.varSetPos);
    bindTo(stmtRst, stmtRst.varSetNeg);
    bindTo(stmtRst, stmtRst.numFreeVars);
    bindTo(stmtRst, stmtRst.numReplacedVars);
    bindTo(stmtRst, stmtRst.numVarsElimed);
    bindTo(stmtRst, stmtRst.trailSize);
    assert(bindAt == numElems);

    // Bind the buffers
    if (mysql_stmt_bind_param(stmtRst.STMT, stmtRst.bind)) {
        cout << "mysql_stmt_bind_param() failed" << endl
        << mysql_stmt_error(stmtRst.STMT) << endl;
        exit(1);
    }
}

void SQLStats::clauseSizeDistrib(
    uint64_t sumConflicts
    , const vector<uint32_t>& sizes
) {
    assert(sizes.size() == stmtClsDistrib.size.size());
    assert(sizes.size() == stmtClsDistrib.num.size());

    stmtClsDistrib.sumConflicts = sumConflicts;
    for(size_t i = 0; i < sizes.size(); i++) {
        stmtClsDistrib.size[i] = i;
        stmtClsDistrib.num[i]  = sizes[i];
    }

    if (mysql_stmt_execute(stmtClsDistrib.STMT)) {
        cout
        << "ERROR: while executing restart insertion MySQL prepared statement"
        << endl;

        cout << "Error from mysql: "
        << mysql_stmt_error(stmtClsDistrib.STMT)
        << endl;

        exit(-1);
    }
}

void SQLStats::restart(
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

