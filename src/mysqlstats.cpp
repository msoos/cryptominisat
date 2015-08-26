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

#include "mysqlstats.h"
#include "solvertypes.h"
#include "solver.h"
#include "time_mem.h"
#include <sstream>
#include "varreplacer.h"
#include "occsimplifier.h"
#include <string>
#include <time.h>
#include "constants.h"
#include "reducedb.h"

using namespace CMSat;
using std::cout;
using std::cerr;
using std::endl;
using std::string;

MySQLStats::MySQLStats() :
    bindAt(0)
{
}

MySQLStats::~MySQLStats()
{
    if (!setup_ok)
        return;

    //Free all the prepared statements
    my_bool ret = mysql_stmt_close(stmtRst.stmt);
    if (ret) {
        cout << "Error closing prepared statement" << endl;
        std::exit(-1);
    }

    ret = mysql_stmt_close(stmtReduceDB.stmt);
    if (ret) {
        cout << "Error closing prepared statement" << endl;
        std::exit(-1);
    }

    ret = mysql_stmt_close(stmtTimePassed.stmt);
    if (ret) {
        cout << "Error closing prepared statement" << endl;
        std::exit(-1);
    }

    ret = mysql_stmt_close(stmtTimePassedMin.stmt);
    if (ret) {
        cout << "Error closing prepared statement" << endl;
        std::exit(-1);
    }

    ret = mysql_stmt_close(stmtMemUsed.stmt);
    if (ret) {
        cout << "Error closing prepared statement" << endl;
        std::exit(-1);
    }

    //Close clonnection
    mysql_close(serverConn);
}

bool MySQLStats::setup(const Solver* solver)
{
    setup_ok = connectServer(solver);
    if (!setup_ok) {
        return false;
    }

    getID(solver);
    add_tags(solver);
    addStartupData(solver);
    initRestartSTMT();
    initReduceDBSTMT();
    initTimePassedSTMT();
    initTimePassedMinSTMT();
    initMemUsedSTMT();

    return true;
}

bool MySQLStats::connectServer(const Solver* solver)
{
    //Init MySQL library
    serverConn = mysql_init(NULL);
    if (!serverConn) {
        cout
        << "Insufficient memory to allocate server connection"
        << endl;
        return false;
    }

    //Connect to server
    if (!mysql_real_connect(
        serverConn
        , solver->getConf().sqlServer.c_str()
        , solver->getConf().sqlUser.c_str()
        , solver->getConf().sqlPass.c_str()
        , solver->getConf().sqlDatabase.c_str()
        , 0
        , NULL
        , 0)
    ) {
        cout
        << "c ERROR while connecting to MySQL server:"
        << mysql_error(serverConn)
        << endl;

        cout
        << "c If your MySQL server is running then you did not create the database" << endl
        << "c and/or didn't add the correct user. Read cmsat_mysql_setup.txt to fix this issue " << endl
        ;

        return false;
    }

    return true;
}

bool MySQLStats::tryIDInSQL(const Solver* solver)
{
    std::stringstream ss;
    ss
    << "INSERT INTO solverRun (runID, version, time) values ("
    << runID
    << ", \"" << Solver::get_version_sha1() << "\""
    << ", " << time(NULL)
    << ");";

    //Inserting element into solverruns to get unique ID
    if (mysql_query(serverConn, ss.str().c_str())) {

        if (solver->getConf().verbosity >= 6) {
            cerr << "c ERROR Couldn't insert into table 'solverruns'" << endl;
            cerr << "c " << mysql_error(serverConn) << endl;
        }

        return false;
    }

    return true;
}

void MySQLStats::getID(const Solver* solver)
{
    size_t numTries = 0;
    getRandomID();
    while(!tryIDInSQL(solver)) {
        getRandomID();
        numTries++;

        //Check if we have been in this loop for too long
        if (numTries > 10) {
            cerr
            << "ERROR: Something is wrong while adding runID!" << endl
            << " Exiting!"
            << endl;

            cerr
            << "Maybe you didn't create the tables in the database?" << endl
            << "You can fix this by executing: " << endl
            << "$ mysql -u root -p cmsat < cmsat_tablestructure.sql" << endl
            ;

            std::exit(-1);
        }
    }

    if (solver->getConf().verbosity >= 1) {
        cout << "c SQL runID is " << runID << endl;
    }
}

void MySQLStats::add_tags(const Solver* solver)
{
    for(vector<std::pair<string, string> >::const_iterator
        it = solver->get_sql_tags().begin(), end = solver->get_sql_tags().end()
        ; it != end
        ; ++it
    ) {

        std::stringstream ss;
        ss
        << "INSERT INTO `tags` (`runID`, `tagname`, `tag`) VALUES("
        << runID
        << ", '" << it->first << "'"
        << ", '" << it->second << "'"
        << ");";

        //Inserting element into solverruns to get unique ID
        if (mysql_query(serverConn, ss.str().c_str())) {
            cerr << "ERROR Couldn't insert into table 'tags'" << endl;
            std::exit(-1);
        }
    }
}

void MySQLStats::addStartupData(const Solver* solver)
{
    std::stringstream ss;
    ss
    << "INSERT INTO `startup` (`runID`, `startTime`, `verbosity`) VALUES ("
    << runID << ","
    << "NOW() , "
    << solver->getConf().verbosity
    << ");";

    if (mysql_query(serverConn, ss.str().c_str())) {
        cerr << "ERROR Couldn't insert into table 'startup'" << endl;
        std::exit(-1);
    }
}

void MySQLStats::finishup(const lbool status)
{
    std::stringstream ss;
    ss
    << "INSERT INTO `finishup` (`runID`, `endTime`, `status`) VALUES ("
    << runID << ","
    << "NOW() , "
    << "'" << status << "'"
    << ");";

    if (mysql_query(serverConn, ss.str().c_str())) {
        cerr << "ERROR Couldn't insert into table 'finishup'" << endl;
        std::exit(-1);
    }
}

void MySQLStats::writeQuestionMarks(
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

void MySQLStats::initTimePassedSTMT()
{
    const size_t numElems = sizeof(stmtTimePassed.bind)/sizeof(MYSQL_BIND);

    std::stringstream ss;
    ss << "insert into `timepassed`"
    << "("
    //Position
    << "  `runID`, `simplifications`, `conflicts`, `time`"

    //Clause stats
    << ", `name`, `elapsed`, `timeout`, `percenttimeremain`"
    << ") values ";
    writeQuestionMarks(
        numElems
        , ss
    );
    ss << ";";

    //Get memory for statement
    stmtTimePassed.stmt = mysql_stmt_init(serverConn);
    if (!stmtTimePassed.stmt) {
        cout << "Error: mysql_stmt_init() out of memory" << endl;
        std::exit(-1);
    }

    //Prepare the statement
    if (mysql_stmt_prepare(stmtTimePassed.stmt, ss.str().c_str(), ss.str().length())) {
        cerr << "ERROR  in mysql_stmt_prepare(), INSERT failed"
        << endl
        << mysql_stmt_error(stmtTimePassed.stmt)
        << endl
        << "Query was: " << ss.str()
        << endl;
        std::exit(-1);
    }

    //Validate parameter count
    unsigned long param_count = mysql_stmt_param_count(stmtTimePassed.stmt);
    if (param_count != numElems) {
        cout
        << "invalid parameter count returned by MySQL"
        << endl;

        std::exit(-1);
    }

    memset(stmtTimePassed.bind, 0, sizeof(stmtTimePassed.bind));


    //Bind the local variables to the statement
    bindAt =0;
    bindTo(stmtTimePassed, runID);
    bindTo(stmtTimePassed, stmtTimePassed.numSimplify);
    bindTo(stmtTimePassed, stmtTimePassed.sumConflicts);
    bindTo(stmtTimePassed, stmtTimePassed.cpuTime);
    bindTo(stmtTimePassed, stmtTimePassed.name, &stmtTimePassed.name_len);
    bindTo(stmtTimePassed, stmtTimePassed.time_passed);
    bindTo(stmtTimePassed, stmtTimePassed.time_out);
    bindTo(stmtTimePassed, stmtTimePassed.percent_time_remain);
    assert(bindAt == numElems);

    // Bind the buffers
    if (mysql_stmt_bind_param(stmtTimePassed.stmt, stmtTimePassed.bind)) {
        cerr << "ERROR mysql_stmt_bind_param() failed" << endl
        << mysql_stmt_error(stmtTimePassed.stmt) << endl;
        std::exit(-1);
    }
}

void MySQLStats::initMemUsedSTMT()
{
    const size_t numElems = sizeof(stmtMemUsed.bind)/sizeof(MYSQL_BIND);

    std::stringstream ss;
    ss << "insert into `memused`"
    << "("
    //Position
    << "  `runID`, `simplifications`, `conflicts`, `time`"

    //Clause stats
    << ", `name`, `MB`"
    << ") values ";
    writeQuestionMarks(
        numElems
        , ss
    );
    ss << ";";

    //Get memory for statement
    stmtMemUsed.stmt = mysql_stmt_init(serverConn);
    if (!stmtMemUsed.stmt) {
        cout << "Error: mysql_stmt_init() out of memory" << endl;
        std::exit(-1);
    }

    //Prepare the statement
    if (mysql_stmt_prepare(stmtMemUsed.stmt, ss.str().c_str(), ss.str().length())) {
        cerr << "ERROR  in mysql_stmt_prepare(), INSERT failed"
        << endl
        << mysql_stmt_error(stmtMemUsed.stmt)
        << endl
        << "Query was: " << ss.str()
        << endl;
        std::exit(-1);
    }

    //Validate parameter count
    unsigned long param_count = mysql_stmt_param_count(stmtMemUsed.stmt);
    if (param_count != numElems) {
        cout
        << "invalid parameter count returned by MySQL"
        << endl;

        std::exit(-1);
    }

    memset(stmtMemUsed.bind, 0, sizeof(stmtMemUsed.bind));


    //Bind the local variables to the statement
    bindAt =0;
    bindTo(stmtMemUsed, runID);
    bindTo(stmtMemUsed, stmtMemUsed.numSimplify);
    bindTo(stmtMemUsed, stmtMemUsed.sumConflicts);
    bindTo(stmtMemUsed, stmtMemUsed.cpuTime);
    bindTo(stmtMemUsed, stmtMemUsed.name, &stmtMemUsed.name_len);
    bindTo(stmtMemUsed, stmtMemUsed.mem_used_mb);
    assert(bindAt == numElems);

    // Bind the buffers
    if (mysql_stmt_bind_param(stmtMemUsed.stmt, stmtMemUsed.bind)) {
        cerr << "ERROR mysql_stmt_bind_param() failed" << endl
        << mysql_stmt_error(stmtMemUsed.stmt) << endl;
        std::exit(-1);
    }
}

void MySQLStats::initTimePassedMinSTMT()
{
    const size_t numElems = sizeof(stmtTimePassedMin.bind)/sizeof(MYSQL_BIND);

    std::stringstream ss;
    ss << "insert into `timepassed`"
    << "("
    //Position
    << "  `runID`, `simplifications`, `conflicts`, `time`"

    //Clause stats
    << ", `name`, `elapsed`"
    << ") values ";
    writeQuestionMarks(
        numElems
        , ss
    );
    ss << ";";

    //Get memory for statement
    stmtTimePassedMin.stmt = mysql_stmt_init(serverConn);
    if (!stmtTimePassedMin.stmt) {
        cerr << "ERROR mysql_stmt_init() out of memory" << endl;
        std::exit(-1);
    }

    //Prepare the statement
    if (mysql_stmt_prepare(stmtTimePassedMin.stmt, ss.str().c_str(), ss.str().length())) {
        cerr << "ERROR in mysql_stmt_prepare(), INSERT failed"
        << endl
        << mysql_stmt_error(stmtTimePassedMin.stmt)
        << endl
        << "Query was: " << ss.str()
        << endl;
        std::exit(-1);
    }

    //Validate parameter count
    unsigned long param_count = mysql_stmt_param_count(stmtTimePassedMin.stmt);
    if (param_count != numElems) {
        cerr << "ERROR invalid parameter count returned by MySQL"
        << endl;

        std::exit(-1);
    }

    memset(stmtTimePassedMin.bind, 0, sizeof(stmtTimePassedMin.bind));


    //Bind the local variables to the statement
    bindAt =0;
    bindTo(stmtTimePassedMin, runID);
    bindTo(stmtTimePassedMin, stmtTimePassedMin.numSimplify);
    bindTo(stmtTimePassedMin, stmtTimePassedMin.sumConflicts);
    bindTo(stmtTimePassedMin, stmtTimePassedMin.cpuTime);
    bindTo(stmtTimePassedMin, stmtTimePassedMin.name, &stmtTimePassedMin.name_len);
    bindTo(stmtTimePassedMin, stmtTimePassedMin.time_passed);
    assert(bindAt == numElems);

    // Bind the buffers
    if (mysql_stmt_bind_param(stmtTimePassedMin.stmt, stmtTimePassedMin.bind)) {
        cerr << "ERROR mysql_stmt_bind_param() failed" << endl
        << mysql_stmt_error(stmtTimePassedMin.stmt) << endl;
        std::exit(-1);
    }
}

//Prepare statement for restart
void MySQLStats::initRestartSTMT()
{
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

    //Propagations
    << ", `propBinIrred` , `propBinRed` "
    << ", `propTriIrred` , `propTriRed`"
    << ", `propLongIrred` , `propLongRed`"

    //Conflicts
    << ", `conflBinIrred`, `conflBinRed`"
    << ", `conflTriIrred`, `conflTriRed`"
    << ", `conflLongIrred`, `conflLongRed`"

    //Reds
    << ", `learntUnits`, `learntBins`, `learntTris`, `learntLongs`"

    //Resolutions
    << ", `resolBin`, `resolTri`, `resolLIrred`, `resolLRed`"

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
        std::exit(-1);
    }

    //Prepare the statement
    if (mysql_stmt_prepare(stmtRst.stmt, ss.str().c_str(), ss.str().length())) {
        cerr << "ERROR in mysql_stmt_prepare(), INSERT failed"
        << endl
        << mysql_stmt_error(stmtRst.stmt)
        << endl
        << "Query was: " << ss.str()
        << endl;
        std::exit(-1);
    }

    //Validate parameter count
    unsigned long param_count = mysql_stmt_param_count(stmtRst.stmt);
    if (param_count != numElems) {
        cerr << "ERROR invalid parameter count returned by MySQL"
        << endl;

        std::exit(-1);
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

    //Red
    bindTo(stmtRst, stmtRst.learntUnits);
    bindTo(stmtRst, stmtRst.learntBins);
    bindTo(stmtRst, stmtRst.learntTris);
    bindTo(stmtRst, stmtRst.learntLongs);

    //Resolutions
    bindTo(stmtRst, stmtRst.resolv.bin);
    bindTo(stmtRst, stmtRst.resolv.tri);
    bindTo(stmtRst, stmtRst.resolv.irredL);
    bindTo(stmtRst, stmtRst.resolv.redL);

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
        cerr << "ERROR mysql_stmt_bind_param() failed" << endl
        << mysql_stmt_error(stmtRst.stmt) << endl;
        std::exit(-1);
    }
}

//Prepare statement for restart
void MySQLStats::initReduceDBSTMT()
{
    const size_t numElems = sizeof(stmtReduceDB.bind)/sizeof(MYSQL_BIND);

    std::stringstream ss;
    ss << "insert into `reduceDB`"
    << "("
    //Position
    << "  `runID`, `simplifications`, `restarts`, `conflicts`, `time`"
    << ", `reduceDBs`"

    //Actual data
    << ", `irredClsVisited`, `irredLitsVisited`"
    << ", `redClsVisited`, `redLitsVisited`"

    //Clean data
    << ", removedNum, removedLits, removedGlue"
    << ", removedResolBin, removedResolTri, removedResolLIrred, removedResolLRed"
    << ", removedAge, removedAct"
    << ", removedLitVisited, removedProp, removedConfl"
    << ", removedLookedAt, removedUsedUIP"

    << ", remainNum, remainLits, remainGlue"
    << ", remainResolBin, remainResolTri, remainResolLIrred, remainResolLRed"
    << ", remainAge, remainAct"
    << ", remainLitVisited, remainProp, remainConfl"
    << ", remainLookedAt, remainUsedUIP"
    << ") values ";
    writeQuestionMarks(
        numElems
        , ss
    );
    ss << ";";

    //Get memory for statement
    stmtReduceDB.stmt = mysql_stmt_init(serverConn);
    if (!stmtReduceDB.stmt) {
        cerr << "ERROR  mysql_stmt_init() out of memory" << endl;
        std::exit(-1);
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
        std::exit(-1);
    }

    //Validate parameter count
    unsigned long param_count = mysql_stmt_param_count(stmtReduceDB.stmt);
    if (param_count != numElems) {
        cout
        << "invalid parameter count returned by MySQL"
        << endl;

        std::exit(-1);
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

    //Clause stats -- red
    bindTo(stmtReduceDB, stmtReduceDB.redClsVisited);
    bindTo(stmtReduceDB, stmtReduceDB.redLitsVisited);

    bindTo(stmtReduceDB, stmtReduceDB.clean.removed.num);
    bindTo(stmtReduceDB, stmtReduceDB.clean.removed.lits);
    bindTo(stmtReduceDB, stmtReduceDB.clean.removed.glue);
    bindTo(stmtReduceDB, stmtReduceDB.clean.removed.resol.bin);
    bindTo(stmtReduceDB, stmtReduceDB.clean.removed.resol.tri);
    bindTo(stmtReduceDB, stmtReduceDB.clean.removed.resol.irredL);
    bindTo(stmtReduceDB, stmtReduceDB.clean.removed.resol.redL);
    bindTo(stmtReduceDB, stmtReduceDB.clean.removed.age);
    bindTo(stmtReduceDB, stmtReduceDB.clean.removed.act);
    bindTo(stmtReduceDB, stmtReduceDB.clean.removed.numLitVisited);
    bindTo(stmtReduceDB, stmtReduceDB.clean.removed.numProp);
    bindTo(stmtReduceDB, stmtReduceDB.clean.removed.numConfl);
    bindTo(stmtReduceDB, stmtReduceDB.clean.removed.numLookedAt);
    bindTo(stmtReduceDB, stmtReduceDB.clean.removed.used_for_uip_creation);

    bindTo(stmtReduceDB, stmtReduceDB.clean.remain.num);
    bindTo(stmtReduceDB, stmtReduceDB.clean.remain.lits);
    bindTo(stmtReduceDB, stmtReduceDB.clean.remain.glue);
    bindTo(stmtReduceDB, stmtReduceDB.clean.remain.resol.bin);
    bindTo(stmtReduceDB, stmtReduceDB.clean.remain.resol.tri);
    bindTo(stmtReduceDB, stmtReduceDB.clean.remain.resol.irredL);
    bindTo(stmtReduceDB, stmtReduceDB.clean.remain.resol.redL);
    bindTo(stmtReduceDB, stmtReduceDB.clean.remain.age);
    bindTo(stmtReduceDB, stmtReduceDB.clean.remain.act);
    bindTo(stmtReduceDB, stmtReduceDB.clean.remain.numLitVisited);
    bindTo(stmtReduceDB, stmtReduceDB.clean.remain.numProp);
    bindTo(stmtReduceDB, stmtReduceDB.clean.remain.numConfl);
    bindTo(stmtReduceDB, stmtReduceDB.clean.remain.numLookedAt);
    bindTo(stmtReduceDB, stmtReduceDB.clean.remain.used_for_uip_creation);

    assert(bindAt == numElems);

    // Bind the buffers
    if (mysql_stmt_bind_param(stmtReduceDB.stmt, stmtReduceDB.bind)) {
        cerr << "ERROR mysql_stmt_bind_param() failed" << endl
        << mysql_stmt_error(stmtReduceDB.stmt) << endl;
        std::exit(-1);
    }
}

void MySQLStats::reduceDB(
    const ClauseUsageStats& irredStats
    , const ClauseUsageStats& redStats
    , const CleaningStats& clean

    , const Solver* solver
) {
    //Position of solving
    stmtReduceDB.numSimplify     = solver->get_solve_stats().numSimplify;
    stmtReduceDB.sumRestarts     = solver->sumRestarts();
    stmtReduceDB.sumConflicts    = solver->sumConflicts();
    stmtReduceDB.cpuTime         = cpuTime();
    stmtReduceDB.reduceDBs       = solver->reduceDB->get_nbReduceDB();

    //Clause data for IRRED
    stmtReduceDB.irredClsVisited    = irredStats.sumLookedAt;
    stmtReduceDB.irredLitsVisited   = irredStats.sumLitVisited;

    //Clause data for RED
    stmtReduceDB.redClsVisited      = redStats.sumLookedAt;
    stmtReduceDB.redLitsVisited     = redStats.sumLitVisited;

    //Clean data
    stmtReduceDB.clean              = clean;

    if (mysql_stmt_execute(stmtReduceDB.stmt)) {
        cout
        << "ERROR: while executing clause DB cleaning MySQL prepared statement"
        << endl;

        cout << "Error from mysql: "
        << mysql_stmt_error(stmtReduceDB.stmt)
        << endl;

        std::exit(-1);
    }
}

void MySQLStats::time_passed(
    const Solver* solver
    , const string& name
    , double time_passed
    , bool time_out
    , double percent_time_remain
) {
    stmtTimePassed.numSimplify     = solver->get_solve_stats().numSimplify;
    stmtTimePassed.sumConflicts    = solver->sumConflicts();
    stmtTimePassed.cpuTime         = cpuTime();
    release_assert(name.size() < sizeof(stmtTimePassed.name)-1);
    strncpy(stmtTimePassed.name, name.c_str(), sizeof(stmtTimePassed.name));
    stmtTimePassed.name[sizeof(stmtTimePassed.name)-1] = '\0';
    stmtTimePassed.name_len = strlen(stmtTimePassed.name);
    stmtTimePassed.time_passed = time_passed;
    stmtTimePassed.time_out = time_out;
    stmtTimePassed.percent_time_remain = percent_time_remain;

    if (mysql_stmt_execute(stmtTimePassed.stmt)) {
        cerr << "ERROR while executing clause DB cleaning MySQL prepared statement"
        << endl
        << "Error from mysql: "
        << mysql_stmt_error(stmtTimePassed.stmt)
        << endl;

        std::exit(-1);
    }
}

void MySQLStats::time_passed_min(
    const Solver* solver
    , const string& name
    , double time_passed
) {
    stmtTimePassedMin.numSimplify     = solver->get_solve_stats().numSimplify;
    stmtTimePassedMin.sumConflicts    = solver->sumConflicts();
    stmtTimePassedMin.cpuTime         = cpuTime();
    release_assert(name.size() < sizeof(stmtTimePassedMin.name)-1);
    strncpy(stmtTimePassedMin.name, name.c_str(), sizeof(stmtTimePassedMin.name));
    stmtTimePassedMin.name[sizeof(stmtTimePassedMin.name)-1] = '\0';
    stmtTimePassedMin.name_len = strlen(stmtTimePassedMin.name);
    stmtTimePassedMin.time_passed = time_passed;

    if (mysql_stmt_execute(stmtTimePassedMin.stmt)) {
        cerr << "ERROR while executing clause DB cleaning MySQL prepared statement"
        << endl
        << "Error from mysql: "
        << mysql_stmt_error(stmtTimePassedMin.stmt)
        << endl;

        std::exit(-1);
    }
}

void MySQLStats::mem_used(
    const Solver* solver
    , const string& name
    , double given_time
    , uint64_t mem_used_mb
) {
    stmtMemUsed.numSimplify     = solver->get_solve_stats().numSimplify;
    stmtMemUsed.sumConflicts    = solver->sumConflicts();
    stmtMemUsed.cpuTime         = given_time;
    release_assert(name.size() < sizeof(stmtMemUsed.name)-1);
    strncpy(stmtMemUsed.name, name.c_str(), sizeof(stmtMemUsed.name));
    stmtMemUsed.name[sizeof(stmtMemUsed.name)-1] = '\0';
    stmtMemUsed.name_len = strlen(stmtMemUsed.name);
    stmtMemUsed.mem_used_mb = mem_used_mb;

    if (mysql_stmt_execute(stmtMemUsed.stmt)) {
        cerr << "ERROR while executing clause DB cleaning MySQL prepared statement"
        << endl
        << "Error from mysql: "
        << mysql_stmt_error(stmtMemUsed.stmt)
        << endl;

        std::exit(-1);
    }
}

void MySQLStats::restart(
    const PropStats& thisPropStats
    , const Searcher::Stats& thisStats
    , const Solver* solver
    , const Searcher* search
) {
    const Searcher::Hist& searchHist = search->getHistory();
    const Solver::BinTriStats& binTri = solver->getBinTriStats();

    //Position of solving
    stmtRst.numSimplify     = solver->get_solve_stats().numSimplify;
    stmtRst.sumRestarts     = search->sumRestarts();
    stmtRst.sumConflicts    = search->sumConflicts();
    stmtRst.cpuTime         = cpuTime();

    //Clause stats
    stmtRst.numIrredBins  = binTri.irredBins;
    stmtRst.numIrredTris  = binTri.irredTris;
    stmtRst.numIrredLongs = solver->get_num_long_irred_cls();
    stmtRst.numIrredLits  = solver->litStats.irredLits;
    stmtRst.numRedBins    = binTri.redBins;
    stmtRst.numRedTris    = binTri.redTris;
    stmtRst.numRedLongs   = solver->get_num_long_red_cls();
    stmtRst.numRedLits    = solver->litStats.redLits;

    //Conflict stats
    stmtRst.glueHist        = searchHist.glueHist.getLongtTerm().avg();
    stmtRst.glueHistSD      = sqrt(searchHist.glueHist.getLongtTerm().var());
    stmtRst.glueHistMin      = searchHist.glueHist.getLongtTerm().getMin();
    stmtRst.glueHistMax      = searchHist.glueHist.getLongtTerm().getMax();

    stmtRst.conflSizeHist   = searchHist.conflSizeHist.avg();
    stmtRst.conflSizeHistSD = sqrt(searchHist.conflSizeHist.var());
    stmtRst.conflSizeHistMin = searchHist.conflSizeHist.getMin();
    stmtRst.conflSizeHistMax = searchHist.conflSizeHist.getMax();

    stmtRst.numResolutionsHist =
        searchHist.numResolutionsHist.avg();
    stmtRst.numResolutionsHistSD =
        sqrt(searchHist.numResolutionsHist.var());
    stmtRst.numResolutionsHistMin =
        searchHist.numResolutionsHist.getMin();
    stmtRst.numResolutionsHistMax =
        searchHist.numResolutionsHist.getMax();

    stmtRst.conflictAfterConflict =
        searchHist.conflictAfterConflict.avg();

    //Search stats
    stmtRst.branchDepthHist         = searchHist.branchDepthHist.avg();
    stmtRst.branchDepthHistSD       = sqrt(searchHist.branchDepthHist.var());
    stmtRst.branchDepthHistMin      = searchHist.branchDepthHist.getMin();
    stmtRst.branchDepthHistMax      = searchHist.branchDepthHist.getMax();


    stmtRst.branchDepthDeltaHist    = searchHist.branchDepthDeltaHist.avg();
    stmtRst.branchDepthDeltaHistSD  = sqrt(searchHist.branchDepthDeltaHist.var());
    stmtRst.branchDepthDeltaHistMin  = searchHist.branchDepthDeltaHist.getMin();
    stmtRst.branchDepthDeltaHistMax  = searchHist.branchDepthDeltaHist.getMax();

    stmtRst.trailDepthHist          = searchHist.trailDepthHist.getLongtTerm().avg();
    stmtRst.trailDepthHistSD        = sqrt(searchHist.trailDepthHist.getLongtTerm().var());
    stmtRst.trailDepthHistMin       = searchHist.trailDepthHist.getLongtTerm().getMin();
    stmtRst.trailDepthHistMax       = searchHist.trailDepthHist.getLongtTerm().getMax();

    stmtRst.trailDepthDeltaHist     = searchHist.trailDepthDeltaHist.avg();
    stmtRst.trailDepthDeltaHistSD   = sqrt(searchHist.trailDepthDeltaHist.var());
    stmtRst.trailDepthDeltaHistMin  = searchHist.trailDepthDeltaHist.getMin();
    stmtRst.trailDepthDeltaHistMax  = searchHist.trailDepthDeltaHist.getMax();

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

    //Red
    stmtRst.learntUnits = thisStats.learntUnits;
    stmtRst.learntBins  = thisStats.learntBins;
    stmtRst.learntTris  = thisStats.learntTris;
    stmtRst.learntLongs = thisStats.learntLongs;

    //Resolv stats
    stmtRst.resolv          = thisStats.resolvs;

    //Var stats
    stmtRst.propagations    = thisPropStats.propagations;
    stmtRst.decisions       = thisStats.decisions;

    stmtRst.varFlipped      = thisPropStats.varFlipped;
    stmtRst.varSetPos       = thisPropStats.varSetPos;
    stmtRst.varSetNeg       = thisPropStats.varSetNeg;
    stmtRst.numFreeVars     = solver->get_num_free_vars();
    stmtRst.numReplacedVars = solver->get_num_vars_replaced();
    stmtRst.numVarsElimed   = solver->get_num_vars_elimed();
    stmtRst.trailSize       = search->getTrailSize();

    if (mysql_stmt_execute(stmtRst.stmt)) {
        cerr << "ERROR  while executing restart insertion MySQL prepared statement"
        << endl
        << "Error from mysql: "
        << mysql_stmt_error(stmtRst.stmt)
        << endl;

        std::exit(-1);
    }
}

