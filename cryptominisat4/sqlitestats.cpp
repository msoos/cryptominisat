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

#include "sqlitestats.h"
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
#include "sql_tablestructure.h"

using namespace CMSat;
using std::cout;
using std::cerr;
using std::endl;
using std::string;

SQLiteStats::~SQLiteStats()
{
    if (!setup_ok)
        return;

    //Free all the prepared statements
    int ret = sqlite3_finalize(stmtRst);
    if (ret != SQLITE_OK) {
        cout << "Error closing prepared statement" << endl;
        std::exit(-1);
    }

    ret = sqlite3_finalize(stmtReduceDB);
    if (ret != SQLITE_OK) {
        cout << "Error closing prepared statement" << endl;
        std::exit(-1);
    }

    ret = sqlite3_finalize(stmtTimePassed);
    if (ret != SQLITE_OK) {
        cout << "Error closing prepared statement" << endl;
        std::exit(-1);
    }

    ret = sqlite3_finalize(stmtMemUsed);
    if (ret != SQLITE_OK) {
        cout << "Error closing prepared statement" << endl;
        std::exit(-1);
    }

    //Close clonnection
    sqlite3_close(db);
}

bool SQLiteStats::setup(const Solver* solver)
{
    setup_ok = connectServer(solver->conf.sqlite_filename, solver->conf.verbosity);
    if (!setup_ok) {
        return false;
    }

    getID(solver);
    add_tags(solver);
    addStartupData(solver);
    initRestartSTMT();
    initReduceDBSTMT();
    initTimePassedSTMT();
    initMemUsedSTMT();

    return true;
}

bool SQLiteStats::connectServer(const std::string& sqlite_filename
    , const int verbosity
)
{
    int rc = sqlite3_open(sqlite_filename.c_str(), &db);
    if(rc) {
        cout << "c Cannot open sqlite database: " << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        return false;
    }

    if (sqlite3_exec(db, "PRAGMA synchronous = OFF", NULL, NULL, NULL)) {
        cerr << "Problem setting pragma to SQLite DB" << endl;
        std::exit(-1);
    }

    if (verbosity >= 1) {
        cout << "c writing to SQLite file: " << sqlite_filename << endl;
    }

    return true;
}

bool SQLiteStats::tryIDInSQL(const Solver* solver)
{
    std::stringstream ss;
    ss
    << "INSERT INTO solverRun (runID, version, time) values ("
    << runID
    << ", \"" << Solver::get_version_sha1() << "\""
    << ", " << time(NULL)
    << ");";

    //Inserting element into solverruns to get unique ID
    const int rc = sqlite3_exec(db, ss.str().c_str(), NULL, NULL, NULL);
    if (rc) {
        if (solver->getConf().verbosity >= 6) {
            cerr << "c ERROR Couldn't insert into table 'solverruns'" << endl;
            cerr << "c " << sqlite3_errmsg(db) << endl;
        }

        return false;
    }

    return true;
}

void SQLiteStats::getID(const Solver* solver)
{
    size_t numTries = 0;
    getRandomID();
    while(!tryIDInSQL(solver)) {
        getRandomID();
        numTries++;

        //Check if we have been in this loop for too long
        if (numTries > 15) {
            if (sqlite3_exec(db, cmsat_tablestructure_sql, NULL, NULL, NULL)) {
                cerr << "ERROR: Couln't create table structure for SQLite"
                << endl;
                std::exit(-1);
            }
        }
    }

    if (solver->getConf().verbosity >= 1) {
        cout << "c SQL runID is " << runID << endl;
    }
}

void SQLiteStats::add_tags(const Solver* solver)
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
        if (sqlite3_exec(db, ss.str().c_str(), NULL, NULL, NULL)) {
            cerr << "ERROR Couldn't insert into table 'tags'" << endl;
            std::exit(-1);
        }
    }
}

void SQLiteStats::addStartupData(const Solver* solver)
{
    std::stringstream ss;
    ss
    << "INSERT INTO `startup` (`runID`, `startTime`, `verbosity`) VALUES ("
    << runID << ","
    << "datetime('now') , "
    << solver->getConf().verbosity
    << ");";

    if (sqlite3_exec(db, ss.str().c_str(), NULL, NULL, NULL)) {
        cerr << "ERROR Couldn't insert into table 'startup' : "
        << sqlite3_errmsg(db) << endl;

        std::exit(-1);
    }
}

void SQLiteStats::finishup(const lbool status)
{
    std::stringstream ss;
    ss
    << "INSERT INTO `finishup` (`runID`, `endTime`, `status`) VALUES ("
    << runID << ","
    << "datetime('now') , "
    << "'" << status << "'"
    << ");";

    if (sqlite3_exec(db, ss.str().c_str(), NULL, NULL, NULL)) {
        cerr << "ERROR Couldn't insert into table 'finishup'" << endl;
        std::exit(-1);
    }
}

void SQLiteStats::writeQuestionMarks(
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


void SQLiteStats::initMemUsedSTMT()
{
    const size_t numElems = 6;

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

    //Prepare the statement
    const int rc = sqlite3_prepare(db, ss.str().c_str(), -1, &stmtMemUsed, NULL);
    if (rc) {
        cerr << "ERROR  in sqlite_stmt_prepare(), INSERT failed"
        << endl
        << sqlite3_errmsg(db)
        << " error code: " << rc
        << endl
        << "Query was: " << ss.str()
        << endl;
        std::exit(-1);
    }
}

void SQLiteStats::mem_used(
    const Solver* solver
    , const string& name
    , double given_time
    , uint64_t mem_used_mb
) {
    int bindAt = 1;
    sqlite3_bind_int64(stmtMemUsed, bindAt++, runID);
    sqlite3_bind_int64(stmtMemUsed, bindAt++, solver->get_solve_stats().numSimplify);
    sqlite3_bind_int64(stmtMemUsed, bindAt++, solver->sumConflicts());
    sqlite3_bind_double(stmtMemUsed, bindAt++, given_time);
    sqlite3_bind_text(stmtMemUsed, bindAt++, name.c_str(), -1, NULL);
    sqlite3_bind_int(stmtMemUsed, bindAt++, mem_used_mb);

    int rc = sqlite3_step(stmtMemUsed);
    if (rc != SQLITE_DONE) {
        cerr << "ERROR while executing mem_used prepared statement"
        << endl
        << "Error from sqlite: "
        << sqlite3_errmsg(db)
        << " error code: " << rc
        << endl;

        std::exit(-1);
    }

    if (sqlite3_reset(stmtMemUsed)) {
        cerr << "Error calling sqlite3_reset on stmtMemUsed" << endl;
        std::exit(-1);
    }
    /*if (sqlite3_clear_bindings(stmtMemUsed)) {
        cerr << "Error calling sqlite3_clear_bindings on stmtMemUsed" << endl;
        std::exit(-1);
    }*/
}

void SQLiteStats::initTimePassedSTMT()
{
    const size_t numElems = 8;

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

    //Prepare the statement
    const int rc = sqlite3_prepare(db, ss.str().c_str(), -1, &stmtTimePassed, NULL);
    if (rc) {
        cerr << "ERROR  in sqlite_stmt_prepare(), INSERT failed"
        << endl
        << sqlite3_errmsg(db)
        << " error code: " << rc
        << endl
        << "Query was: " << ss.str()
        << endl;
        std::exit(-1);
    }
}

void SQLiteStats::time_passed(
    const Solver* solver
    , const string& name
    , double time_passed
    , bool time_out
    , double percent_time_remain
) {

    int bindAt = 1;
    sqlite3_bind_int64(stmtTimePassed, bindAt++, runID);
    sqlite3_bind_int64(stmtTimePassed, bindAt++, solver->get_solve_stats().numSimplify);
    sqlite3_bind_int64(stmtTimePassed, bindAt++, solver->sumConflicts());
    sqlite3_bind_double(stmtTimePassed, bindAt++, cpuTime());
    sqlite3_bind_text(stmtTimePassed, bindAt++, name.c_str(), -1, NULL);
    sqlite3_bind_double(stmtTimePassed, bindAt++, time_passed);
    sqlite3_bind_int(stmtTimePassed, bindAt++, time_out);
    sqlite3_bind_double(stmtTimePassed, bindAt++, percent_time_remain);

    int rc = sqlite3_step(stmtTimePassed);
    if (rc != SQLITE_DONE) {
        cerr << "ERROR while executing time_passed prepared statement"
        << endl
        << "Error from sqlite: "
        << sqlite3_errmsg(db)
        << " error code: " << rc
        << endl;

        std::exit(-1);
    }

    if (sqlite3_reset(stmtTimePassed)) {
        cerr << "Error calling sqlite3_reset on stmtTimePassed" << endl;
        std::exit(-1);
    }
    /*if (sqlite3_clear_bindings(stmtTimePassed)) {
        cerr << "Error calling sqlite3_clear_bindings on stmtTimePassed" << endl;
        std::exit(-1);
    }*/
}

void SQLiteStats::time_passed_min(
    const Solver* solver
    , const string& name
    , double time_passed
) {
    int bindAt = 1;
    sqlite3_bind_int64(stmtTimePassed, bindAt++, runID);
    sqlite3_bind_int64(stmtTimePassed, bindAt++, solver->get_solve_stats().numSimplify);
    sqlite3_bind_int64(stmtTimePassed, bindAt++, solver->sumConflicts());
    sqlite3_bind_double(stmtTimePassed, bindAt++, cpuTime());
    sqlite3_bind_text(stmtTimePassed, bindAt++, name.c_str(), -1, NULL);
    sqlite3_bind_double(stmtTimePassed, bindAt++, time_passed);
    sqlite3_bind_null(stmtTimePassed, bindAt++);
    sqlite3_bind_null(stmtTimePassed, bindAt++);

    int rc = sqlite3_step(stmtTimePassed);
    if (rc != SQLITE_DONE) {
        cerr << "ERROR while executing time_passed prepared statement (time_passed_min function)"
        << endl
        << "Error from sqlite: "
        << sqlite3_errmsg(db)
        << " error code: " << rc
        << endl;

        std::exit(-1);
    }

    if (sqlite3_reset(stmtTimePassed)) {
        cerr << "Error calling sqlite3_reset on stmtTimePassed" << endl;
        std::exit(-1);
    }
    if (sqlite3_clear_bindings(stmtTimePassed)) {
        cerr << "Error calling sqlite3_clear_bindings on stmtTimePassed" << endl;
        std::exit(-1);
    }
}

//Prepare statement for restart
void SQLiteStats::initRestartSTMT()
{
    const size_t numElems = 71;

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

    //Prepare the statement
    if (sqlite3_prepare(db, ss.str().c_str(), -1, &stmtRst, NULL)) {
        cerr << "ERROR in sqlite_stmt_prepare(), INSERT failed"
        << endl
        << sqlite3_errmsg(db)
        << endl
        << "Query was: " << ss.str()
        << endl;
        std::exit(-1);
    }
}

void SQLiteStats::restart(
    const PropStats& thisPropStats
    , const Searcher::Stats& thisStats
    , const Solver* solver
    , const Searcher* search
) {
    const Searcher::Hist& searchHist = search->getHistory();
    const Solver::BinTriStats& binTri = solver->getBinTriStats();

    int bindAt = 1;
    sqlite3_bind_int64(stmtRst, bindAt++, runID);
    sqlite3_bind_int64(stmtRst, bindAt++, solver->get_solve_stats().numSimplify);
    sqlite3_bind_int64(stmtRst, bindAt++, search->sumRestarts());
    sqlite3_bind_int64(stmtRst, bindAt++, solver->sumConflicts());
    sqlite3_bind_double(stmtRst, bindAt++, cpuTime());


    sqlite3_bind_int64(stmtRst, bindAt++, binTri.irredBins);
    sqlite3_bind_int64(stmtRst, bindAt++, binTri.irredTris);
    sqlite3_bind_int64(stmtRst, bindAt++, solver->get_num_long_irred_cls());

    sqlite3_bind_int64(stmtRst, bindAt++, binTri.redBins);
    sqlite3_bind_int64(stmtRst, bindAt++, binTri.redTris);
    sqlite3_bind_int64(stmtRst, bindAt++, solver->get_num_long_red_cls());

    sqlite3_bind_int64(stmtRst, bindAt++, solver->litStats.irredLits);
    sqlite3_bind_int64(stmtRst, bindAt++, solver->litStats.redLits);

    //Conflict stats
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.glueHist.getLongtTerm().avg());
    sqlite3_bind_double(stmtRst, bindAt++, sqrt(searchHist.glueHist.getLongtTerm().var()));
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.glueHist.getLongtTerm().getMin());
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.glueHist.getLongtTerm().getMax());

    sqlite3_bind_double(stmtRst, bindAt++, searchHist.conflSizeHist.avg());
    sqlite3_bind_double(stmtRst, bindAt++, sqrt(searchHist.conflSizeHist.var()));
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.conflSizeHist.getMin());
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.conflSizeHist.getMax());

    sqlite3_bind_double(stmtRst, bindAt++, searchHist.numResolutionsHist.avg());
    sqlite3_bind_double(stmtRst, bindAt++, sqrt(searchHist.numResolutionsHist.var()));
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.numResolutionsHist.getMin());
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.numResolutionsHist.getMax());

    sqlite3_bind_double(stmtRst, bindAt++, searchHist.conflictAfterConflict.avg());

    //Search stats
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.branchDepthHist.avg());
    sqlite3_bind_double(stmtRst, bindAt++, sqrt(searchHist.branchDepthHist.var()));
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.branchDepthHist.getMin());
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.branchDepthHist.getMax());

    sqlite3_bind_double(stmtRst, bindAt++, searchHist.branchDepthDeltaHist.avg());
    sqlite3_bind_double(stmtRst, bindAt++, sqrt(searchHist.branchDepthDeltaHist.var()));
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.branchDepthDeltaHist.getMin());
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.branchDepthDeltaHist.getMax());

    sqlite3_bind_double(stmtRst, bindAt++, searchHist.trailDepthHist.getLongtTerm().avg());
    sqlite3_bind_double(stmtRst, bindAt++, sqrt(searchHist.trailDepthHist.getLongtTerm().var()));
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.trailDepthHist.getLongtTerm().getMin());
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.trailDepthHist.getLongtTerm().getMax());

    sqlite3_bind_double(stmtRst, bindAt++, searchHist.trailDepthDeltaHist.avg());
    sqlite3_bind_double(stmtRst, bindAt++, sqrt(searchHist.trailDepthDeltaHist.var()));
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.trailDepthDeltaHist.getMin());
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.trailDepthDeltaHist.getMax());

    //Prop
    sqlite3_bind_int64(stmtRst, bindAt++, thisPropStats.propsBinIrred);
    sqlite3_bind_int64(stmtRst, bindAt++, thisPropStats.propsBinRed);
    sqlite3_bind_int64(stmtRst, bindAt++, thisPropStats.propsTriIrred);
    sqlite3_bind_int64(stmtRst, bindAt++, thisPropStats.propsTriRed);
    sqlite3_bind_int64(stmtRst, bindAt++, thisPropStats.propsLongIrred);
    sqlite3_bind_int64(stmtRst, bindAt++, thisPropStats.propsLongRed);

    //Confl
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.conflStats.conflsBinIrred);
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.conflStats.conflsBinRed);
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.conflStats.conflsTriIrred);
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.conflStats.conflsTriRed);
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.conflStats.conflsLongIrred);
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.conflStats.conflsLongRed);

    //Red
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.learntUnits);
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.learntBins);
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.learntTris);
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.learntLongs);

    //Resolv stats
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.resolvs.bin);
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.resolvs.tri);
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.resolvs.irredL);
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.resolvs.redL);


    //Var stats
    sqlite3_bind_int64(stmtRst, bindAt++, thisPropStats.propagations);
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.decisions);

    sqlite3_bind_int64(stmtRst, bindAt++, thisPropStats.varFlipped);
    sqlite3_bind_int64(stmtRst, bindAt++, thisPropStats.varSetPos);
    sqlite3_bind_int64(stmtRst, bindAt++, thisPropStats.varSetNeg);
    sqlite3_bind_int64(stmtRst, bindAt++, solver->get_num_free_vars());
    sqlite3_bind_int64(stmtRst, bindAt++, solver->get_num_vars_replaced());
    sqlite3_bind_int64(stmtRst, bindAt++, solver->get_num_vars_elimed());
    sqlite3_bind_int64(stmtRst, bindAt++, search->getTrailSize());

    int rc = sqlite3_step(stmtRst);
    if (rc != SQLITE_DONE) {
        cerr << "ERROR  while executing restart insertion SQLite prepared statement"
        << endl
        << "Error from sqlite: "
        << sqlite3_errmsg(db)
        << endl;

        std::exit(-1);
    }

    if (sqlite3_reset(stmtRst)) {
        cerr << "Error calling sqlite3_reset on stmtRst" << endl;
        std::exit(-1);
    }
    if (sqlite3_clear_bindings(stmtRst)) {
        cerr << "Error calling sqlite3_clear_bindings on stmtRst" << endl;
        std::exit(-1);
    }
}


//Prepare statement for restart
void SQLiteStats::initReduceDBSTMT()
{
    const size_t numElems = 38;

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

    //Prepare the statement
    int rc = sqlite3_prepare(db, ss.str().c_str(), -1, &stmtReduceDB, NULL);
    if (rc) {
        cout
        << "Error in sqlite_prepare(), INSERT failed"
        << endl
        << sqlite3_errmsg(db)
        << endl
        << "Query was: " << ss.str()
        << endl;
        std::exit(-1);
    }
}

void SQLiteStats::reduceDB(
    const ClauseUsageStats& irredStats
    , const ClauseUsageStats& redStats
    , const CleaningStats& clean
    , const Solver* solver
) {

     int bindAt = 1;
    sqlite3_bind_int64(stmtReduceDB, bindAt++, runID);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, solver->get_solve_stats().numSimplify);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, solver->sumRestarts());
    sqlite3_bind_int64(stmtReduceDB, bindAt++, solver->sumConflicts());
    sqlite3_bind_double(stmtReduceDB, bindAt++, cpuTime());
    sqlite3_bind_int64(stmtReduceDB, bindAt++, solver->reduceDB->get_nbReduceDB());

    //Clause data for IRRED
    sqlite3_bind_int64(stmtReduceDB, bindAt++, irredStats.sumLookedAt);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, irredStats.sumLitVisited);

    //Clause data for RED
    sqlite3_bind_int64(stmtReduceDB, bindAt++, redStats.sumLookedAt);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, redStats.sumLitVisited);

    //removed
    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.removed.num);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.removed.lits);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.removed.glue);

    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.removed.resol.bin);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.removed.resol.tri);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.removed.resol.irredL);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.removed.resol.redL);

    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.removed.age);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.removed.act);

    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.removed.numLitVisited);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.removed.numProp);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.removed.numConfl);

    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.removed.numLookedAt);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.removed.used_for_uip_creation);

    //remain
    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.remain.num);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.remain.lits);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.remain.glue);

    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.remain.resol.bin);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.remain.resol.tri);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.remain.resol.irredL);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.remain.resol.redL);

    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.remain.age);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.remain.act);

    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.remain.numLitVisited);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.remain.numProp);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.remain.numConfl);

    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.remain.numLookedAt);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, clean.remain.used_for_uip_creation);

    int rc = sqlite3_step(stmtReduceDB);
    if (rc != SQLITE_DONE) {
        cout
        << "ERROR: while executing clause DB cleaning SQLite prepared statement"
        << endl;

        cout << "Error from sqlite: "
        << sqlite3_errmsg(db)
        << endl;
        std::exit(-1);
    }

    if (sqlite3_reset(stmtReduceDB)) {
        cerr << "Error calling sqlite3_reset on stmtReduceDB" << endl;
        std::exit(-1);
    }

    if (sqlite3_clear_bindings(stmtReduceDB)) {
        cerr << "Error calling sqlite3_clear_bindings on stmtReduceDB" << endl;
        std::exit(-1);
    }
}

