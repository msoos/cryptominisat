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
{
}

void SQLStats::setup(const Solver* solver)
{
    connectServer();
    getID(solver);
    addFiles(solver);
    addStartupData(solver);
    initRestartSTMT(solver->conf.verbosity);
    initReduceDBSTMT(solver->conf.verbosity);
    initVarSTMT(solver);
    initClauseDistribSTMT(
        solver
        , stmtClsDistribSize
        , "clauseSizeDistrib"   //table name
        , "size"                //property name
        , solver->conf.dumpClauseDistribMaxSize //num inserts
    );

    initClauseDistribSTMT(
        solver
        , stmtClsDistribGlue
        , "clauseGlueDistrib"   //table name
        , "glue"                //property name
        , solver->conf.dumpClauseDistribMaxGlue //num inserts
    );
    initSizeGlueScatterSTMT(
        solver
        , solver->conf.preparedDumpSize
    );
    initVarSTMT(solver);
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
    runID &= 0xffffffULL;

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

void SQLStats::addStartupData(const Solver* solver)
{
    std::stringstream ss;
    ss
    << "INSERT INTO startup (runID, startTime, verbosity) VALUES ("
    << runID << ","
    << "NOW() , "
    << solver->conf.verbosity
    << ");";

    //Inserting element into solverruns to get unique ID
    if (mysql_query(serverConn, ss.str().c_str())) {
        cout << "Couldn't insert into table 'solverruns'" << endl;
        exit(1);
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
    , StmtClsDistrib& mystruct
    , const string& tableName
    , const string& valueName
    , const size_t numInserts
) {
    const size_t numElems = 4;
    mystruct.bind.resize(numElems*numInserts);

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
    mystruct.stmt = mysql_stmt_init(serverConn);
    if (!mystruct.stmt) {
        cout << "Error: mysql_stmt_init() out of memory" << endl;
        exit(1);
    }

    //Prepare the statement
    if (mysql_stmt_prepare(mystruct.stmt, ss.str().c_str(), ss.str().length())) {
        cout
        << "Error in mysql_stmt_prepare(), INSERT failed"
        << endl
        << mysql_stmt_error(mystruct.stmt)
        << endl
        << "Query was: " << ss.str()
        << endl;
        exit(0);
    }

    if (solver->conf.verbosity >= 6) {
        cout
        << "prepare INSERT successful"
        << endl;
    }

    //Validate parameter count
    unsigned long param_count = mysql_stmt_param_count(mystruct.stmt);
    if (param_count != numElems*numInserts) {
        cout
        << "invalid parameter count returned by MySQL"
        << endl;

        exit(1);
    }

    //Clear mem of bind, get enough mem for vars
    memset(mystruct.bind.data(), 0, mystruct.bind.size()*sizeof(MYSQL_BIND));
    mystruct.value.resize(numInserts);
    mystruct.num.resize(numInserts);

    //Bind the local variables to the statement
    bindAt = 0;
    for(size_t i = 0; i < numInserts; i++) {
        bindTo(mystruct, runID);
        bindTo(mystruct, mystruct.sumConflicts);
        bindTo(mystruct, mystruct.value[i]);
        bindTo(mystruct, mystruct.num[i]);
    }
    assert(bindAt == numElems*numInserts);

    //Bind the buffers
    if (mysql_stmt_bind_param(mystruct.stmt, mystruct.bind.data())) {
        cout << "mysql_stmt_bind_param() failed" << endl
        << mysql_stmt_error(mystruct.stmt) << endl;
        exit(1);
    }
}

void SQLStats::initSizeGlueScatterSTMT(
    const Solver* solver
    , const size_t numInserts
) {
    const size_t numElems = 5;
    cout << "Trying to pass " << numInserts << " inserts" << endl;
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
    stmtSizeGlueScatter.stmt = mysql_stmt_init(serverConn);
    if (!stmtSizeGlueScatter.stmt) {
        cout << "Error: mysql_stmt_init() out of memory" << endl;
        exit(1);
    }

    //Prepare the statement
    if (mysql_stmt_prepare(stmtSizeGlueScatter.stmt, ss.str().c_str(), ss.str().length())) {
        cout
        << "Error in mysql_stmt_prepare(), INSERT failed"
        << endl
        << mysql_stmt_error(stmtSizeGlueScatter.stmt)
        << endl
        << "Query was: " << ss.str()
        << endl;
        exit(0);
    }

    if (solver->conf.verbosity >= 6) {
        cout
        << "prepare INSERT successful"
        << endl;
    }

    //Validate parameter count
    unsigned long param_count = mysql_stmt_param_count(stmtSizeGlueScatter.stmt);
    if (param_count != numElems*numInserts) {
        cout
        << "invalid parameter count returned by MySQL"
        << endl;

        exit(1);
    }

    //Clear mem of bind, get enough mem for vars
    memset(stmtSizeGlueScatter.bind.data(), 0, stmtSizeGlueScatter.bind.size()*sizeof(MYSQL_BIND));
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
    if (mysql_stmt_bind_param(stmtSizeGlueScatter.stmt, stmtSizeGlueScatter.bind.data())) {
        cout << "mysql_stmt_bind_param() failed" << endl
        << mysql_stmt_error(stmtSizeGlueScatter.stmt) << endl;
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
    << ", numIrredBins, numIrredTris, numIrredLongs"
    << ", numRedBins, numRedTris, numRedLongs"
    << ", numIrredLits, numRedLits"

    //Conflict stats
    << ", `glue`, `glueSD`, `glueMin`, `glueMax`"
    << ", `size`, `sizeSD`, `sizeMin`, `sizeMax`"
    << ", `resolutions`, `resolutionsSD`, `resolutionsMin`, `resolutionsMax`"
    << ", `conflAfterConfl`"

    //Search stats
    << ", `branchDepth`, `branchDepthSD`, `branchDepthMin`, `branchDepthMax`"
    << ", `branchDepthDelta`, `branchDepthDeltaSD`, `branchDepthDeltaMin`, `branchDepthDeltaMax`"
    << ", `trailDepth`, `trailDepthSD`, `trailDepthMin`, `trailDepthMax`"
    << ", `trailDepthDelta`, `trailDepthDeltaSD`, `trailDepthDeltaMin`,`trailDepthDeltaMax`"
    << ", `agility`"

    //Propagations
    << ", `propBinIrred` , `propBinRed` "
    << ", `propTriIrred` , `propTriRed`"
    << ", `propLongIrred` , `propLongRed`"

    //Conflicts
    << ", `conflBinIrred`, `conflBinRed`"
    << ", `conflTriIrred`, `conflTriRed`"
    << ", `conflLongIrred`, `conflLongRed`"

    //Learnts
    << ", `learntUnits`, `learntBins`, `learntTris`, `learntLongs`"

    //Misc
    << ", `watchListSizeTraversed`, `watchListSizeTraversedSD`"
    << ", `watchListSizeTraversedMin`, `watchListSizeTraversedMax`"
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
    stmtRst.stmt = mysql_stmt_init(serverConn);
    if (!stmtRst.stmt) {
        cout << "Error: mysql_stmt_init() out of memory" << endl;
        exit(1);
    }

    //Prepare the statement
    if (mysql_stmt_prepare(stmtRst.stmt, ss.str().c_str(), ss.str().length())) {
        cout
        << "Error in mysql_stmt_prepare(), INSERT failed"
        << endl
        << mysql_stmt_error(stmtRst.stmt)
        << endl
        << "Query was: " << ss.str()
        << endl;
        exit(0);
    }

    if (verbosity >= 6) {
        cout
        << "prepare INSERT successful"
        << endl;
    }

    //Validate parameter count
    unsigned long param_count = mysql_stmt_param_count(stmtRst.stmt);
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
    bindTo(stmtRst, stmtRst.numRedBins);
    bindTo(stmtRst, stmtRst.numRedTris);
    bindTo(stmtRst, stmtRst.numRedLongs);

    bindTo(stmtRst, stmtRst.numIrredLits);
    bindTo(stmtRst, stmtRst.numRedLits);

    //Conflict stats
    bindTo(stmtRst, stmtRst.glueHist);
    bindTo(stmtRst, stmtRst.glueHistSD);
    bindTo(stmtRst, stmtRst.glueHistMin);
    bindTo(stmtRst, stmtRst.glueHistMax);

    bindTo(stmtRst, stmtRst.conflSizeHist);
    bindTo(stmtRst, stmtRst.conflSizeHistSD);
    bindTo(stmtRst, stmtRst.conflSizeHistMin);
    bindTo(stmtRst, stmtRst.conflSizeHistMax);

    bindTo(stmtRst, stmtRst.numResolutionsHist);
    bindTo(stmtRst, stmtRst.numResolutionsHistSD);
    bindTo(stmtRst, stmtRst.numResolutionsHistMin);
    bindTo(stmtRst, stmtRst.numResolutionsHistMax);

    bindTo(stmtRst, stmtRst.conflictAfterConflict);

    //Search stats
    bindTo(stmtRst, stmtRst.branchDepthHist);
    bindTo(stmtRst, stmtRst.branchDepthHistSD);
    bindTo(stmtRst, stmtRst.branchDepthHistMin);
    bindTo(stmtRst, stmtRst.branchDepthHistMax);

    bindTo(stmtRst, stmtRst.branchDepthDeltaHist);
    bindTo(stmtRst, stmtRst.branchDepthDeltaHistSD);
    bindTo(stmtRst, stmtRst.branchDepthDeltaHistMin);
    bindTo(stmtRst, stmtRst.branchDepthDeltaHistMax);

    bindTo(stmtRst, stmtRst.trailDepthHist);
    bindTo(stmtRst, stmtRst.trailDepthHistSD);
    bindTo(stmtRst, stmtRst.trailDepthHistMin);
    bindTo(stmtRst, stmtRst.trailDepthHistMax);

    bindTo(stmtRst, stmtRst.trailDepthDeltaHist);
    bindTo(stmtRst, stmtRst.trailDepthDeltaHistSD);
    bindTo(stmtRst, stmtRst.trailDepthDeltaHistMin);
    bindTo(stmtRst, stmtRst.trailDepthDeltaHistMax);

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
    bindTo(stmtRst, stmtRst.watchListSizeTraversedMin);
    bindTo(stmtRst, stmtRst.watchListSizeTraversedMax);

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
    if (mysql_stmt_bind_param(stmtRst.stmt, stmtRst.bind)) {
        cout << "mysql_stmt_bind_param() failed" << endl
        << mysql_stmt_error(stmtRst.stmt) << endl;
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

    //Clean data
    << ", preRemovedNum, preRemovedLits, preRemovedGlue"
    << ", preRemovedResol, preRemovedAge, preRemovedAct"
    << ", preRemovedLitVisited, preRemovedProp, preRemovedConfl"
    << ", preRemovedLookedAt"

    << ", removedNum, removedLits, removedGlue"
    << ", removedResol, removedAge, removedAct"
    << ", removedLitVisited, removedProp, removedConfl"
    << ", removedLookedAt"

    << ", remainNum, remainLits, remainGlue"
    << ", remainResol, remainAge, remainAct"
    << ", remainLitVisited, remainProp, remainConfl"
    << ", remainLookedAt"
    << ") values ";
    writeQuestionMarks(
        numElems
        , ss
    );
    ss << ";";

    //Get memory for statement
    stmtReduceDB.stmt = mysql_stmt_init(serverConn);
    if (!stmtReduceDB.stmt) {
        cout << "Error: mysql_stmt_init() out of memory" << endl;
        exit(1);
    }

    //Prepare the statement
    if (mysql_stmt_prepare(stmtReduceDB.stmt, ss.str().c_str(), ss.str().length())) {
        cout
        << "Error in mysql_stmt_prepare(), INSERT failed"
        << endl
        << mysql_stmt_error(stmtReduceDB.stmt)
        << endl
        << "Query was: " << ss.str()
        << endl;
        exit(0);
    }

    if (verbosity >= 6) {
        cout
        << "prepare INSERT successful"
        << endl;
    }

    //Validate parameter count
    unsigned long param_count = mysql_stmt_param_count(stmtReduceDB.stmt);
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

    bindTo(stmtReduceDB, stmtReduceDB.clean.preRemove.num);
    bindTo(stmtReduceDB, stmtReduceDB.clean.preRemove.lits);
    bindTo(stmtReduceDB, stmtReduceDB.clean.preRemove.glue);
    bindTo(stmtReduceDB, stmtReduceDB.clean.preRemove.numResolutions);
    bindTo(stmtReduceDB, stmtReduceDB.clean.preRemove.age);
    bindTo(stmtReduceDB, stmtReduceDB.clean.preRemove.act);
    bindTo(stmtReduceDB, stmtReduceDB.clean.preRemove.numLitVisited);
    bindTo(stmtReduceDB, stmtReduceDB.clean.preRemove.numProp);
    bindTo(stmtReduceDB, stmtReduceDB.clean.preRemove.numConfl);
    bindTo(stmtReduceDB, stmtReduceDB.clean.preRemove.numLookedAt);

    bindTo(stmtReduceDB, stmtReduceDB.clean.removed.num);
    bindTo(stmtReduceDB, stmtReduceDB.clean.removed.lits);
    bindTo(stmtReduceDB, stmtReduceDB.clean.removed.glue);
    bindTo(stmtReduceDB, stmtReduceDB.clean.removed.numResolutions);
    bindTo(stmtReduceDB, stmtReduceDB.clean.removed.age);
    bindTo(stmtReduceDB, stmtReduceDB.clean.removed.act);
    bindTo(stmtReduceDB, stmtReduceDB.clean.removed.numLitVisited);
    bindTo(stmtReduceDB, stmtReduceDB.clean.removed.numProp);
    bindTo(stmtReduceDB, stmtReduceDB.clean.removed.numConfl);
    bindTo(stmtReduceDB, stmtReduceDB.clean.removed.numLookedAt);

    bindTo(stmtReduceDB, stmtReduceDB.clean.remain.num);
    bindTo(stmtReduceDB, stmtReduceDB.clean.remain.lits);
    bindTo(stmtReduceDB, stmtReduceDB.clean.remain.glue);
    bindTo(stmtReduceDB, stmtReduceDB.clean.remain.numResolutions);
    bindTo(stmtReduceDB, stmtReduceDB.clean.remain.age);
    bindTo(stmtReduceDB, stmtReduceDB.clean.remain.act);
    bindTo(stmtReduceDB, stmtReduceDB.clean.remain.numLitVisited);
    bindTo(stmtReduceDB, stmtReduceDB.clean.remain.numProp);
    bindTo(stmtReduceDB, stmtReduceDB.clean.remain.numConfl);
    bindTo(stmtReduceDB, stmtReduceDB.clean.remain.numLookedAt);

    assert(bindAt == numElems);

    // Bind the buffers
    if (mysql_stmt_bind_param(stmtReduceDB.stmt, stmtReduceDB.bind)) {
        cout << "mysql_stmt_bind_param() failed" << endl
        << mysql_stmt_error(stmtReduceDB.stmt) << endl;
        exit(1);
    }
}

void SQLStats::initVarSTMT(
    const Solver* solver
) {
    stmtVar.data.resize(100);
    const size_t num = 13;
    const size_t numElems = num*stmtVar.data.size();
    stmtVar.bind.resize(numElems);

    std::stringstream ss;
    ss << "insert into `vars`"
    << "("

    //Position
    << " `varInitID`, `var`"

    //Actual data
    << ", `posPolarSet`, `negPolarSet`, `flippedPolarity`"

    //Dec level history stats
    << ", `decLevelAvg`, `decLevelSD`, `decLevelMin`, `decLevelMax`"

    //Trail level history stats
    << ", `trailLevelAvg`, `trailLevelSD`, `trailLevelMin`, `trailLevelMax`"
    << ") values ";
    for(size_t i = 0; i < stmtVar.data.size(); i++) {
        writeQuestionMarks(num, ss);

        if (i != stmtVar.data.size()-1)
            ss << " , ";
    }
    ss << ";";

    //Get memory for statement
    stmtVar.stmt = mysql_stmt_init(serverConn);
    if (!stmtVar.stmt) {
        cout << "Error: mysql_stmt_init() out of memory" << endl;
        exit(1);
    }

    //Prepare the statement
    if (mysql_stmt_prepare(stmtVar.stmt, ss.str().c_str(), ss.str().length())) {
        cout
        << "Error in mysql_stmt_prepare(), INSERT failed"
        << endl
        << mysql_stmt_error(stmtVar.stmt)
        << endl
        << "Query was: " << ss.str()
        << endl;
        exit(0);
    }

    if (solver->conf.verbosity >= 6) {
        cout
        << "prepare INSERT successful"
        << endl;
    }

    //Validate parameter count
    unsigned long param_count = mysql_stmt_param_count(stmtVar.stmt);
    if (param_count != numElems) {
        cout
        << "invalid parameter count returned by MySQL"
        << endl;

        exit(1);
    }

    //Clear bind
    memset(stmtVar.bind.data(), 0, numElems*sizeof(MYSQL_BIND));

    //Bind the local variables to the statement
    bindAt =0;
    for(size_t i = 0; i < stmtVar.data.size(); i++) {
        bindTo(stmtVar, stmtVar.varInitID);
        bindTo(stmtVar, stmtVar.data[i].var);
        bindTo(stmtVar, stmtVar.data[i].posPolarSet);
        bindTo(stmtVar, stmtVar.data[i].negPolarSet);
        bindTo(stmtVar, stmtVar.data[i].flippedPolarity);

        bindTo(stmtVar, stmtVar.data[i].decLevelAvg);
        bindTo(stmtVar, stmtVar.data[i].decLevelSD);
        bindTo(stmtVar, stmtVar.data[i].decLevelMin);
        bindTo(stmtVar, stmtVar.data[i].decLevelMax);

        bindTo(stmtVar, stmtVar.data[i].trailLevelAvg);
        bindTo(stmtVar, stmtVar.data[i].trailLevelSD);
        bindTo(stmtVar, stmtVar.data[i].trailLevelMin);
        bindTo(stmtVar, stmtVar.data[i].trailLevelMax);
    }
    assert(bindAt == numElems);

    // Bind the buffers
    if (mysql_stmt_bind_param(stmtVar.stmt, stmtVar.bind.data())) {
        cout << "mysql_stmt_bind_param() failed" << endl
        << mysql_stmt_error(stmtVar.stmt) << endl;
        exit(1);
    }
}

void SQLStats::varDataDump(
    const Solver* solver
    , const Searcher* search
    , const vector<VarData>& varData
) {
    double myTime = cpuTime();

    std::stringstream ss;
    ss
    << "INSERT INTO varDataInit (`runID`, `simplifications`, `restarts`, `conflicts`, `time`)"
    << " values ("
    << runID
    << ", " << solver->getSolveStats().numSimplify
    << ", " << search->sumRestarts()
    << ", " << search->sumConflicts()
    << ", " << cpuTime()
    << ");";

    if (mysql_query(serverConn, ss.str().c_str())) {
        if (solver->conf.verbosity >= 6) {
            cout << "c Couldn't insert into table `varDataInit`" << endl;
            cout << "c " << mysql_error(serverConn) << endl;
        }
        exit(-1);
    }

    stmtVar.varInitID = mysql_insert_id(serverConn);

    size_t at = 0;
    for(size_t i = 0; i < solver->nVars(); i++) {
        stmtVar.data[at].var = i;

        //Overall stats
        stmtVar.data[at].posPolarSet = varData[i].posPolarSet;
        stmtVar.data[at].negPolarSet = varData[i].negPolarSet;
        stmtVar.data[at].flippedPolarity  = varData[i].flippedPolarity;

        //Dec level history stats
        stmtVar.data[at].decLevelAvg  = varData[i].decLevelHist.avg();
        stmtVar.data[at].decLevelSD   = sqrt(varData[i].decLevelHist.var());
        stmtVar.data[at].decLevelMin  = varData[i].decLevelHist.getMin();
        stmtVar.data[at].decLevelMax  = varData[i].decLevelHist.getMax();

        //Trail level history stats
        stmtVar.data[at].trailLevelAvg  = varData[i].trailLevelHist.avg();
        stmtVar.data[at].trailLevelSD   = sqrt(varData[i].trailLevelHist.var());
        stmtVar.data[at].trailLevelMin  = varData[i].trailLevelHist.getMin();
        stmtVar.data[at].trailLevelMax  = varData[i].trailLevelHist.getMax();
        at++;

        if (at == stmtVar.data.size()) {
            if (mysql_stmt_execute(stmtVar.stmt)) {
                cout
                << "ERROR: while executing restart insertion MySQL prepared statement"
                << endl;

                cout << "Error from mysql: "
                << mysql_stmt_error(stmtVar.stmt)
                << endl;

                exit(-1);
            }
            at = 0;
        }
    }
    assert(at == 0 && "numInserts must be divisible");
}

void SQLStats::clauseSizeDistrib(
    uint64_t sumConflicts
    , const vector<uint32_t>& sizes
) {
    assert(sizes.size() == stmtClsDistribSize.value.size());
    assert(sizes.size() == stmtClsDistribSize.num.size());

    stmtClsDistribSize.sumConflicts = sumConflicts;
    for(size_t i = 0; i < sizes.size(); i++) {
        stmtClsDistribSize.value[i] = i;
        stmtClsDistribSize.num[i]  = sizes[i];
    }

    if (mysql_stmt_execute(stmtClsDistribSize.stmt)) {
        cout
        << "ERROR: while executing restart insertion MySQL prepared statement"
        << endl;

        cout << "Error from mysql: "
        << mysql_stmt_error(stmtClsDistribSize.stmt)
        << endl;

        exit(-1);
    }
}

void SQLStats::clauseGlueDistrib(
    uint64_t sumConflicts
    , const vector<uint32_t>& glues
) {
    assert(glues.size() == stmtClsDistribGlue.value.size());
    assert(glues.size() == stmtClsDistribGlue.num.size());

    stmtClsDistribGlue.sumConflicts = sumConflicts;
    for(size_t i = 0; i < glues.size(); i++) {
        stmtClsDistribGlue.value[i] = i;
        stmtClsDistribGlue.num[i]  = glues[i];
    }

    if (mysql_stmt_execute(stmtClsDistribGlue.stmt)) {
        cout
        << "ERROR: while executing restart insertion MySQL prepared statement"
        << endl;

        cout << "Error from mysql: "
        << mysql_stmt_error(stmtClsDistribGlue.stmt)
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

    const size_t numInserts = stmtSizeGlueScatter.size.size();

    stmtSizeGlueScatter.sumConflicts = sumConflicts;

    size_t at = 0;
    for(size_t i = 0; i < sizeAndGlue.shape()[0]; i++) {
        for(size_t i2 = 0; i2 < sizeAndGlue.shape()[1]; i2++) {
            stmtSizeGlueScatter.size[at] = i;
            stmtSizeGlueScatter.glue[at] = i2;
            stmtSizeGlueScatter.num[at]  = sizeAndGlue[i][i2];
            at++;

            if (at == numInserts) {
                if (mysql_stmt_execute(stmtSizeGlueScatter.stmt)) {
                    cout
                    << "ERROR: while executing restart insertion MySQL prepared statement"
                    << endl;

                    cout << "Error from mysql: "
                    << mysql_stmt_error(stmtSizeGlueScatter.stmt)
                    << endl;

                    exit(-1);
                }
                at = 0;
            }
        }
    }
    assert(at == 0 && "numInserts must be divisible");
}

void SQLStats::reduceDB(
    const ClauseUsageStats& irredStats
    , const ClauseUsageStats& redStats
    , const CleaningStats& clean

    , const Solver* solver
) {
    //Position of solving
    stmtReduceDB.numSimplify     = solver->getSolveStats().numSimplify;
    stmtReduceDB.sumRestarts     = solver->sumRestarts();
    stmtReduceDB.sumConflicts    = solver->sumConflicts();
    stmtReduceDB.cpuTime         = cpuTime();
    stmtReduceDB.reduceDBs       = solver->solveStats.nbReduceDB;

    //Clause data for IRRED
    stmtReduceDB.irredLitsVisited   = irredStats.sumLitVisited;
    stmtReduceDB.irredClsVisited    = irredStats.sumLookedAt;
    stmtReduceDB.irredProps         = irredStats.sumProp;
    stmtReduceDB.irredConfls        = irredStats.sumConfl;
    stmtReduceDB.irredUIP           = irredStats.sumUsedUIP;

    //Clause data for RED
    stmtReduceDB.redLitsVisited     = redStats.sumLitVisited;
    stmtReduceDB.redClsVisited      = redStats.sumLookedAt;
    stmtReduceDB.redProps           = redStats.sumProp;
    stmtReduceDB.redConfls          = redStats.sumConfl;
    stmtReduceDB.redUIP             = redStats.sumUsedUIP;

    //Clean data
    stmtReduceDB.clean              = clean;

    if (mysql_stmt_execute(stmtReduceDB.stmt)) {
        cout
        << "ERROR: while executing clause DB cleaning MySQL prepared statement"
        << endl;

        cout << "Error from mysql: "
        << mysql_stmt_error(stmtReduceDB.stmt)
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
    stmtRst.glueHist        = search->hist.glueHist.getAvgMidLong();
    stmtRst.glueHistSD      = sqrt(search->hist.glueHist.getVarMidLong());
    stmtRst.glueHistMin      = search->hist.glueHist.getMinMidLong();
    stmtRst.glueHistMax      = search->hist.glueHist.getMaxMidLong();

    stmtRst.conflSizeHist   = search->hist.conflSizeHist.getAvgMidLong();
    stmtRst.conflSizeHistSD = sqrt(search->hist.conflSizeHist.getVarMidLong());
    stmtRst.conflSizeHistMin = search->hist.conflSizeHist.getMinMidLong();
    stmtRst.conflSizeHistMax = search->hist.conflSizeHist.getMaxMidLong();

    stmtRst.numResolutionsHist =
        search->hist.numResolutionsHist.avg();
    stmtRst.numResolutionsHistSD =
        sqrt(search->hist.numResolutionsHist.var());
    stmtRst.numResolutionsHistMin =
        search->hist.numResolutionsHist.getMin();
    stmtRst.numResolutionsHistMax =
        search->hist.numResolutionsHist.getMax();

    stmtRst.conflictAfterConflict =
        search->hist.conflictAfterConflict.avg()*100.0;

    //Search stats
    stmtRst.branchDepthHist         = search->hist.branchDepthHist.avg();
    stmtRst.branchDepthHistSD       = sqrt(search->hist.branchDepthHist.var());
    stmtRst.branchDepthHistMin      = search->hist.branchDepthHist.getMin();
    stmtRst.branchDepthHistMax      = search->hist.branchDepthHist.getMax();


    stmtRst.branchDepthDeltaHist    = search->hist.branchDepthDeltaHist.getAvgMidLong();
    stmtRst.branchDepthDeltaHistSD  = sqrt(search->hist.branchDepthDeltaHist.getVarMidLong());
    stmtRst.branchDepthDeltaHistMin  = search->hist.branchDepthDeltaHist.getMinMidLong();
    stmtRst.branchDepthDeltaHistMax  = search->hist.branchDepthDeltaHist.getMaxMidLong();

    stmtRst.trailDepthHist          = search->hist.trailDepthHist.avg();
    stmtRst.trailDepthHistSD        = sqrt(search->hist.trailDepthHist.var());
    stmtRst.trailDepthHistMin       = search->hist.trailDepthHist.getMin();
    stmtRst.trailDepthHistMax       = search->hist.trailDepthHist.getMax();

    stmtRst.trailDepthDeltaHist     = search->hist.trailDepthDeltaHist.avg();
    stmtRst.trailDepthDeltaHistSD   = sqrt(search->hist.trailDepthDeltaHist.var());
    stmtRst.trailDepthDeltaHistMin  = search->hist.trailDepthDeltaHist.getMin();
    stmtRst.trailDepthDeltaHistMax  = search->hist.trailDepthDeltaHist.getMax();

    stmtRst.agilityHist             = search->hist.agilityHist.avg();

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
    stmtRst.watchListSizeTraversed   = search->hist.watchListSizeTraversed.avg();
    stmtRst.watchListSizeTraversedSD = sqrt(search->hist.watchListSizeTraversed.var());
    stmtRst.watchListSizeTraversedMin= search->hist.watchListSizeTraversed.getMin();
    stmtRst.watchListSizeTraversedMax= search->hist.watchListSizeTraversed.getMax();


    stmtRst.litPropagatedSomething   = search->hist.litPropagatedSomething.avg()*100.0;
    stmtRst.litPropagatedSomethingSD = sqrt(search->hist.litPropagatedSomething.var())*100.0;

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

    if (mysql_stmt_execute(stmtRst.stmt)) {
        cout
        << "ERROR: while executing restart insertion MySQL prepared statement"
        << endl;

        cout << "Error from mysql: "
        << mysql_stmt_error(stmtRst.stmt)
        << endl;

        exit(-1);
    }
}

