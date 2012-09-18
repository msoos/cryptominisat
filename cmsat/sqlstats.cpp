#include "sqlstats.h"
#include "solvertypes.h"
#include "solver.h"
#include "time_mem.h"
#include <sstream>
#include "varreplacer.h"
#include "simplifier.h"
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
    , clsSizeDistrib(NULL)
    , clsGlueDistrib(NULL)
{
}

void SQLStats::setup(const Solver* solver)
{
    connectServer();
    getID(solver);
    addFiles(solver);
    initRestartSTMT(solver->conf.verbosity);
    initReduceDBSTMT(solver->conf.verbosity);
    initClauseDistribSTMT(
        solver
        , clsSizeDistrib
        , "clauseSizeDistrib"   //table name
        , "size"                //property name
        , solver->conf.dumpClauseDistribMaxSize //num inserts
    );

    initClauseDistribSTMT(
        solver
        , clsGlueDistrib
        , "clauseGlueDistrib"   //table name
        , "glue"                //property name
        , solver->conf.dumpClauseDistribMaxGlue //num inserts
    );
    initSizeGlueScatterSTMT(
        solver
        , solver->conf.dumpClauseDistribMaxSize
            * solver->conf.dumpClauseDistribMaxGlue //num inserts
    );
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

bool SQLStats::tryIDInSQL(const Solver* solver)
{
    std::stringstream ss;
    ss
    << "INSERT INTO solverRun (runID, version, time) values ("
    << runID
    << ", \"" << Solver::getVersion() << "\""
    << ", " << time(NULL)
    << ");";

    //Inserting element into solverruns to get unique ID
    if (mysql_query(serverConn, ss.str().c_str())) {

        if (solver->conf.verbosity >= 6) {
            cout << "c Couldn't insert into table 'solverruns'" << endl;
            cout << "c " << mysql_error(serverConn) << endl;
        }

        return false;
    }

    return true;
}

void SQLStats::getID(const Solver* solver)
{
    size_t numTries = 0;
    getRandomID();
    while(!tryIDInSQL(solver)) {
        getRandomID();
        numTries++;

        //Check if we have been in this loop for too long
        if (numTries > 10) {
            cout
            << "Database is full?"
            << " Something is wrong while adding runID!" << endl
            << " Exiting!"
            << endl;

            exit(-1);
        }
    }

    if (solver->conf.verbosity >= 1) {
        cout << "c SQL runID is " << runID << endl;
    }
}

void SQLStats::getRandomID()
{
    //Generate random ID for SQL
    int randomData = open("/dev/urandom", O_RDONLY);
    if (randomData == -1) {
        cout << "Error reading from /dev/urandom !" << endl;
        exit(-1);
    }
    ssize_t ret = read(randomData, &runID, sizeof(runID));

    //Can only be <8 bytes long, some PHP-related limit
    //Make it 6-byte long then (good chance to collide after 2^24 entries)
    runID &= 0xffffffffULL;

    if (ret != sizeof(runID)) {
        cout << "Couldn't read from /dev/urandom!" << endl;
        exit(-1);
    }
    close(randomData);
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

void SQLStats::initClauseDistribSTMT(
    const Solver* solver
    , MYSQL_STMT*& stmt
    , const string& tableName
    , const string& valueName
    , const size_t numInserts
) {
    const size_t numElems = 4;
    stmtClsDistrib.bind.resize(numElems*numInserts);

    std::stringstream ss;
    ss << "insert into `" << tableName << "`"
    << "("
    << " `runID`, `conflicts`, `" << valueName << "`, `num`"
    << ") values ";
    for(size_t i = 0; i < numInserts; i++) {
        writeQuestionMarks(numElems, ss);

        if (i != numInserts-1)
            ss << " , ";
    }
    ss << ";";

    //Get memory for statement
    stmt = mysql_stmt_init(serverConn);
    if (!stmt) {
        cout << "Error: mysql_stmt_init() out of memory" << endl;
        exit(1);
    }

    //Prepare the statement
    if (mysql_stmt_prepare(stmt, ss.str().c_str(), ss.str().length())) {
        cout << "Error in mysql_stmt_prepare(), INSERT failed" << endl
        << mysql_stmt_error(stmt) << endl;
        exit(0);
    }

    if (solver->conf.verbosity >= 6) {
        cout
        << "prepare INSERT successful"
        << endl;
    }

    //Validate parameter count
    unsigned long param_count = mysql_stmt_param_count(stmt);
    if (param_count != numElems*numInserts) {
        cout
        << "invalid parameter count returned by MySQL"
        << endl;

        exit(1);
    }

    //Clear mem of bind, get enough mem for vars
    memset(stmtClsDistrib.bind.data(), 0, stmtClsDistrib.bind.size());
    stmtClsDistrib.value.resize(numInserts);
    stmtClsDistrib.num.resize(numInserts);

    //Bind the local variables to the statement
    bindAt = 0;
    for(size_t i = 0; i < numInserts; i++) {
        bindTo(stmtClsDistrib, runID);
        bindTo(stmtClsDistrib, stmtClsDistrib.sumConflicts);
        bindTo(stmtClsDistrib, stmtClsDistrib.value[i]);
        bindTo(stmtClsDistrib, stmtClsDistrib.num[i]);
    }
    assert(bindAt == numElems*numInserts);

    //Bind the buffers
    if (mysql_stmt_bind_param(stmt, stmtClsDistrib.bind.data())) {
        cout << "mysql_stmt_bind_param() failed" << endl
        << mysql_stmt_error(stmt) << endl;
        exit(1);
    }
}

void SQLStats::initSizeGlueScatterSTMT(
    const Solver* solver
    , size_t numInserts
) {
    const size_t numElems = 3;
    stmtSizeGlueScatter.bind.resize(numElems*numInserts);

    std::stringstream ss;
    ss << "insert into `sizeGlue`"
    << "("
    << " `runID`, `conflicts`, `size`, `glue`, `num`"
    << ") values ";
    for(size_t i = 0; i < numInserts; i++) {
        writeQuestionMarks(numElems, ss);

        if (i != numInserts-1)
            ss << " , ";
    }
    ss << ";";

    //Get memory for statement
    stmtSizeGlueScatter.STMT = mysql_stmt_init(serverConn);
    if (!stmtSizeGlueScatter.STMT) {
        cout << "Error: mysql_stmt_init() out of memory" << endl;
        exit(1);
    }

    //Prepare the statement
    if (mysql_stmt_prepare(stmtSizeGlueScatter.STMT, ss.str().c_str(), ss.str().length())) {
        cout << "Error in mysql_stmt_prepare(), INSERT failed" << endl
        << mysql_stmt_error(stmtSizeGlueScatter.STMT) << endl;
        exit(0);
    }

    if (solver->conf.verbosity >= 6) {
        cout
        << "prepare INSERT successful"
        << endl;
    }

    //Validate parameter count
    unsigned long param_count = mysql_stmt_param_count(stmtSizeGlueScatter.STMT);
    if (param_count != numElems*numInserts) {
        cout
        << "invalid parameter count returned by MySQL"
        << endl;

        exit(1);
    }

    //Clear mem of bind, get enough mem for vars
    memset(stmtSizeGlueScatter.bind.data(), 0, stmtSizeGlueScatter.bind.size());
    stmtSizeGlueScatter.size.resize(numInserts);
    stmtSizeGlueScatter.glue.resize(numInserts);
    stmtSizeGlueScatter.num.resize(numInserts);

    //Bind the local variables to the statement
    bindAt = 0;
    for(size_t i = 0; i < numInserts; i++) {
        bindTo(stmtSizeGlueScatter, runID);
        bindTo(stmtSizeGlueScatter, stmtSizeGlueScatter.sumConflicts);
        bindTo(stmtSizeGlueScatter, stmtSizeGlueScatter.size[i]);
        bindTo(stmtSizeGlueScatter, stmtSizeGlueScatter.glue[i]);
        bindTo(stmtSizeGlueScatter, stmtSizeGlueScatter.num[i]);
    }
    assert(bindAt == numElems*numInserts);

    //Bind the buffers
    if (mysql_stmt_bind_param(stmtSizeGlueScatter.STMT, stmtSizeGlueScatter.bind.data())) {
        cout << "mysql_stmt_bind_param() failed" << endl
        << mysql_stmt_error(stmtSizeGlueScatter.STMT) << endl;
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

    //Clause stats
    << ", numIrredBins, numIrredTris, numIrredLongs, numIrredLits"
    << ", numRedBins, numRedTris, numRedLongs, numRedLits"

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
    << ", `conflBinIrred`, `conflBinRed`"
    << ", `conflTriIrred`, `conflTriRed`"
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

    //Clause stats
    bindTo(stmtRst, stmtRst.numIrredBins);
    bindTo(stmtRst, stmtRst.numIrredTris);
    bindTo(stmtRst, stmtRst.numIrredLongs);
    bindTo(stmtRst, stmtRst.numIrredLits);
    bindTo(stmtRst, stmtRst.numRedBins);
    bindTo(stmtRst, stmtRst.numRedTris);
    bindTo(stmtRst, stmtRst.numRedLongs);
    bindTo(stmtRst, stmtRst.numRedLits);

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
    bindTo(stmtRst, stmtRst.conflsTriIrred);
    bindTo(stmtRst, stmtRst.conflsTriRed);
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

//Prepare statement for restart
void SQLStats::initReduceDBSTMT(
    uint64_t verbosity
) {
    const size_t numElems = sizeof(stmtReduceDB.bind)/sizeof(MYSQL_BIND);

    std::stringstream ss;
    ss << "insert into `reduceDB`"
    << "("
    //Position
    << "  `runID`, `simplifications`, `restarts`, `conflicts`, `time`"
    << ", `reduceDBs`"

    //Actual data
    << ", `irredClsVisited`, `irredLitsVisited`, `irredProps`, `irredConfls`, `irredUIP`"
    << ", `redClsVisited`, `redLitsVisited`, `redProps`, `redConfls`, `redUIP`"
    << ") values ";
    writeQuestionMarks(
        numElems
        , ss
    );
    ss << ";";

    //Get memory for statement
    stmtReduceDB.STMT = mysql_stmt_init(serverConn);
    if (!stmtReduceDB.STMT) {
        cout << "Error: mysql_stmt_init() out of memory" << endl;
        exit(1);
    }

    //Prepare the statement
    if (mysql_stmt_prepare(stmtReduceDB.STMT, ss.str().c_str(), ss.str().length())) {
        cout << "Error in mysql_stmt_prepare() of reduceDB, INSERT failed" << endl
        << mysql_stmt_error(stmtReduceDB.STMT) << endl;
        exit(0);
    }

    if (verbosity >= 6) {
        cout
        << "prepare INSERT successful"
        << endl;
    }

    //Validate parameter count
    unsigned long param_count = mysql_stmt_param_count(stmtReduceDB.STMT);
    if (param_count != numElems) {
        cout
        << "invalid parameter count returned by MySQL"
        << endl;

        exit(1);
    }

    memset(stmtReduceDB.bind, 0, sizeof(stmtReduceDB.bind));


    //Bind the local variables to the statement
    bindAt =0;
    bindTo(stmtReduceDB, runID);
    bindTo(stmtReduceDB, stmtReduceDB.numSimplify);
    bindTo(stmtReduceDB, stmtReduceDB.sumRestarts);
    bindTo(stmtReduceDB, stmtReduceDB.sumConflicts);
    bindTo(stmtReduceDB, stmtReduceDB.cpuTime);
    bindTo(stmtReduceDB, stmtReduceDB.reduceDBs);

    //Clause stats -- irred
    bindTo(stmtReduceDB, stmtReduceDB.irredClsVisited);
    bindTo(stmtReduceDB, stmtReduceDB.irredLitsVisited);
    bindTo(stmtReduceDB, stmtReduceDB.irredProps);
    bindTo(stmtReduceDB, stmtReduceDB.irredConfls);
    bindTo(stmtReduceDB, stmtReduceDB.irredUIP);

    //Clause stats -- red
    bindTo(stmtReduceDB, stmtReduceDB.redClsVisited);
    bindTo(stmtReduceDB, stmtReduceDB.redLitsVisited);
    bindTo(stmtReduceDB, stmtReduceDB.redProps);
    bindTo(stmtReduceDB, stmtReduceDB.redConfls);
    bindTo(stmtReduceDB, stmtReduceDB.redUIP);
    assert(bindAt == numElems);

    // Bind the buffers
    if (mysql_stmt_bind_param(stmtReduceDB.STMT, stmtReduceDB.bind)) {
        cout << "mysql_stmt_bind_param() failed" << endl
        << mysql_stmt_error(stmtReduceDB.STMT) << endl;
        exit(1);
    }
}

void SQLStats::clauseSizeDistrib(
    uint64_t sumConflicts
    , const vector<uint32_t>& sizes
) {
    assert(sizes.size() == stmtClsDistrib.value.size());
    assert(sizes.size() == stmtClsDistrib.num.size());

    stmtClsDistrib.sumConflicts = sumConflicts;
    for(size_t i = 0; i < sizes.size(); i++) {
        stmtClsDistrib.value[i] = i;
        stmtClsDistrib.num[i]  = sizes[i];
    }

    if (mysql_stmt_execute(clsSizeDistrib)) {
        cout
        << "ERROR: while executing restart insertion MySQL prepared statement"
        << endl;

        cout << "Error from mysql: "
        << mysql_stmt_error(clsSizeDistrib)
        << endl;

        exit(-1);
    }
}

void SQLStats::clauseGlueDistrib(
    uint64_t sumConflicts
    , const vector<uint32_t>& glues
) {
    assert(glues.size() == stmtClsDistrib.value.size());
    assert(glues.size() == stmtClsDistrib.num.size());

    stmtClsDistrib.sumConflicts = sumConflicts;
    for(size_t i = 0; i < glues.size(); i++) {
        stmtClsDistrib.value[i] = i;
        stmtClsDistrib.num[i]  = glues[i];
    }

    if (mysql_stmt_execute(clsGlueDistrib)) {
        cout
        << "ERROR: while executing restart insertion MySQL prepared statement"
        << endl;

        cout << "Error from mysql: "
        << mysql_stmt_error(clsGlueDistrib)
        << endl;

        exit(-1);
    }
}

void SQLStats::clauseSizeGlueScatter(
    uint64_t sumConflicts
    , boost::multi_array<uint32_t, 2>& sizeAndGlue
) {
    //assert(glues.size() == stmtClsDistrib.value.size());
    //assert(glues.size() == stmtClsDistrib.num.size());

    stmtSizeGlueScatter.sumConflicts = sumConflicts;

    size_t at = 0;
    for(size_t i = 0; i < sizeAndGlue.shape()[0]; i++) {
        for(size_t i2 = 0; i2 < sizeAndGlue.shape()[1]; i2++) {
            stmtSizeGlueScatter.size[at] = i;
            stmtSizeGlueScatter.glue[at] = i2;
            stmtSizeGlueScatter.num[at]  = sizeAndGlue[i][i2];
            at++;
        }
    }
    if (mysql_stmt_execute(clsGlueDistrib)) {
        cout
        << "ERROR: while executing restart insertion MySQL prepared statement"
        << endl;

        cout << "Error from mysql: "
        << mysql_stmt_error(clsGlueDistrib)
        << endl;

        exit(-1);
    }
    assert(at == stmtSizeGlueScatter.size.size());
}

void SQLStats::reduceDB(
    const ClauseUsageStats& irredStats
    , const ClauseUsageStats& redStats
    , const Solver* solver
) {
    //Position of solving
    stmtReduceDB.numSimplify     = solver->getSolveStats().numSimplify;
    stmtReduceDB.sumRestarts     = solver->sumRestarts();
    stmtReduceDB.sumConflicts    = solver->sumConflicts();
    stmtReduceDB.cpuTime         = cpuTime();
    stmtReduceDB.reduceDBs       = solver->solveStats.nbReduceDB;

    //Data
    stmtReduceDB.irredLitsVisited   = irredStats.sumLitVisited;
    stmtReduceDB.irredClsVisited    = irredStats.sumLookedAt;
    stmtReduceDB.irredProps         = irredStats.sumProp;
    stmtReduceDB.irredConfls        = irredStats.sumConfl;
    stmtReduceDB.irredUIP           = irredStats.sumUsedUIP;

    //Data
    stmtReduceDB.redLitsVisited     = redStats.sumLitVisited;
    stmtReduceDB.redClsVisited      = redStats.sumLookedAt;
    stmtReduceDB.redProps           = redStats.sumProp;
    stmtReduceDB.redConfls          = redStats.sumConfl;
    stmtReduceDB.redUIP             = redStats.sumUsedUIP;

    if (mysql_stmt_execute(stmtReduceDB.STMT)) {
        cout
        << "ERROR: while executing clause DB cleaning MySQL prepared statement"
        << endl;

        cout << "Error from mysql: "
        << mysql_stmt_error(stmtReduceDB.STMT)
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

    //Clause stats
    stmtRst.numIrredBins  = solver->irredBins;
    stmtRst.numIrredTris  = solver->irredTris;
    stmtRst.numIrredLongs = solver->longIrredCls.size();
    stmtRst.numIrredLits  = solver->irredLits;
    stmtRst.numRedBins    = solver->redBins;
    stmtRst.numRedTris    = solver->redTris;
    stmtRst.numRedLongs   = solver->longRedCls.size();
    stmtRst.numRedLits    = solver->redLits;

    //Conflict stats
    stmtRst.glueHist        = search->glueHist.getAvgMidLong();
    stmtRst.glueHistSD      = sqrt(search->glueHist.getVarMidLong());
    stmtRst.conflSizeHist   = search->conflSizeHist.getAvgMidLong();
    stmtRst.conflSizeHistSD = sqrt(search->conflSizeHist.getVarMidLong());
    stmtRst.numResolutionsHist =
        search->numResolutionsHist.avg();
    stmtRst.numResolutionsHistSD =
        sqrt(search->numResolutionsHist.var());
    stmtRst.conflictAfterConflict =
        search->conflictAfterConflict.avg()*100.0;
    stmtRst.conflictAfterConflictSD =
        sqrt(search->conflictAfterConflict.var())*100.0;

    //Search stats
    stmtRst.branchDepthHist         = search->branchDepthHist.avg();
    stmtRst.branchDepthHistSD       = sqrt(search->branchDepthHist.var());
    stmtRst.branchDepthDeltaHist    = search->branchDepthDeltaHist.getAvgMidLong();
    stmtRst.branchDepthDeltaHistSD  = sqrt(search->branchDepthDeltaHist.getVarMidLong());
    stmtRst.trailDepthHist          = search->trailDepthHist.avg();
    stmtRst.trailDepthHistSD        = sqrt(search->trailDepthHist.var());
    stmtRst.trailDepthDeltaHist     = search->trailDepthDeltaHist.avg();
    stmtRst.trailDepthDeltaHistSD   = sqrt(search->trailDepthDeltaHist.var());
    stmtRst.agilityHist             = search->agilityHist.avg();

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
    stmtRst.conflsTriIrred  = thisStats.conflStats.conflsTriIrred;
    stmtRst.conflsTriRed    = thisStats.conflStats.conflsTriRed;
    stmtRst.conflsLongIrred = thisStats.conflStats.conflsLongIrred;
    stmtRst.conflsLongRed   = thisStats.conflStats.conflsLongRed;

    //Learnt
    stmtRst.learntUnits = thisStats.learntUnits;
    stmtRst.learntBins  = thisStats.learntBins;
    stmtRst.learntTris  = thisStats.learntTris;
    stmtRst.learntLongs = thisStats.learntLongs;

    //Misc
    stmtRst.watchListSizeTraversed   = search->watchListSizeTraversed.avg();
    stmtRst.watchListSizeTraversedSD = sqrt(search->watchListSizeTraversed.var());
    stmtRst.litPropagatedSomething   = search->litPropagatedSomething.avg()*100.0;
    stmtRst.litPropagatedSomethingSD = sqrt(search->litPropagatedSomething.var())*100.0;

    //Var stats
    stmtRst.decisions       = thisStats.decisions;
    stmtRst.propagations    = thisPropStats.propagations;
    stmtRst.varFlipped      = thisPropStats.varFlipped;
    stmtRst.varSetPos       = thisPropStats.varSetPos;
    stmtRst.varSetNeg       = thisPropStats.varSetNeg;
    stmtRst.numFreeVars     = solver->getNumFreeVars();
    stmtRst.numReplacedVars = solver->varReplacer->getNumReplacedVars();
    stmtRst.numVarsElimed   = solver->simplifier->getStats().numVarsElimed;
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

