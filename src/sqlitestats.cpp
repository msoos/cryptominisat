/******************************************
Copyright (c) 2016, Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***********************************************/

#include "sqlitestats.h"
#include "solvertypes.h"
#include "solver.h"
#include "time_mem.h"
#include <sstream>
#include "varreplacer.h"
#include "occsimplifier.h"
#include <string>
#include <cmath>
#include <time.h>
#include "constants.h"
#include "reducedb.h"
#include "sql_tablestructure.h"
#include "varreplacer.h"

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

    ret = sqlite3_finalize(stmt_clause_stats);
    if (ret != SQLITE_OK) {
        cout << "Error closing prepared statement" << endl;
        std::exit(-1);
    }

    //Close clonnection
    sqlite3_close(db);
}

bool SQLiteStats::setup(const Solver* solver)
{
    setup_ok = connectServer(solver->conf.verbosity);
    if (!setup_ok) {
        return false;
    }

    getID(solver);
    addStartupData();
    initRestartSTMT();
    initReduceDBSTMT();
    initTimePassedSTMT();
    initMemUsedSTMT();
    init_clause_stats_STMT();

    return true;
}

bool SQLiteStats::connectServer(const int verbosity)
{
    int rc = sqlite3_open(filename.c_str(), &db);
    if(rc) {
        cout << "c Cannot open sqlite database: " << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        return false;
    }

    if (sqlite3_exec(db, "PRAGMA synchronous = OFF", NULL, NULL, NULL)) {
        cerr << "Problem setting pragma to SQLite DB" << endl;
        std::exit(-1);
    }

    if (verbosity) {
        cout << "c writing to SQLite file: " << filename << endl;
    }

    return true;
}

bool SQLiteStats::tryIDInSQL(const Solver* solver)
{
    std::stringstream ss;
    ss
    << "INSERT INTO solverRun (runID, `runtime`) values ("
    << runID
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
    bool created_tablestruct = false;
    size_t numTries = 0;
    getRandomID();
    while(!tryIDInSQL(solver)) {
        getRandomID();
        numTries++;

        //Check if we have been in this loop for too long
        if (numTries > 15) {
            if (created_tablestruct) {
                cerr << "ERROR: Couldn't get random runID. "
                << "Something is probably off with your sqlite database. "
                << "Try deleting it."
                << endl;
                std::exit(-1);
            }
            if (sqlite3_exec(db, cmsat_tablestructure_sql, NULL, NULL, NULL)) {
                cerr << "ERROR: Couln't create table structure for SQLite: "
                << sqlite3_errmsg(db)
                << endl;
                std::exit(-1);
            }
            created_tablestruct = true;
        }
    }

    if (solver->getConf().verbosity) {
        cout << "c SQL runID is " << runID << endl;
    }
}

void SQLiteStats::add_tag(const std::pair<string, string>& tag)
{
    std::stringstream ss;
    ss
    << "INSERT INTO `tags` (`runID`, `tagname`, `tag`) VALUES("
    << runID
    << ", '" << tag.first << "'"
    << ", '" << tag.second << "'"
    << ");";

    //Inserting element into solverruns to get unique ID
    if (sqlite3_exec(db, ss.str().c_str(), NULL, NULL, NULL)) {
        cerr << "SQLite: ERROR Couldn't insert into table 'tags'" << endl;
        assert(false);
        std::exit(-1);
    }
}

void SQLiteStats::addStartupData()
{
    std::stringstream ss;
    ss
    << "INSERT INTO `startup` (`runID`, `startTime`) VALUES ("
    << runID << ","
    << "datetime('now')"
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
    << "  `runID`, `simplifications`, `conflicts`, `runtime`"

    //memory stats
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
    //Position
    sqlite3_bind_int64(stmtMemUsed, bindAt++, runID);
    sqlite3_bind_int64(stmtMemUsed, bindAt++, solver->get_solve_stats().numSimplify);
    sqlite3_bind_int64(stmtMemUsed, bindAt++, solver->sumConflicts);
    sqlite3_bind_double(stmtMemUsed, bindAt++, given_time);
    //memory stats
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
    << "  `runID`, `simplifications`, `conflicts`, `runtime`"

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
    sqlite3_bind_int64(stmtTimePassed, bindAt++, solver->sumConflicts);
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
    sqlite3_bind_int64(stmtTimePassed, bindAt++, solver->sumConflicts);
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
    const size_t numElems = 65;

    std::stringstream ss;
    ss << "insert into `restart`"
    << "("
    //Position
    << "  `runID`, `simplifications`, `restarts`, `conflicts`, `runtime`"

    //Clause stats
    << ", numIrredBins, numIrredLongs"
    << ", numRedBins, numRedLongs"
    << ", numIrredLits, numRedLits"

    //Conflict stats
    << ", `glue`, `glueSD`, `glueMin`, `glueMax`"
    << ", `size`, `sizeSD`, `sizeMin`, `sizeMax`"
    << ", `resolutions`, `resolutionsSD`, `resolutionsMin`, `resolutionsMax`"

    //Search stats
    << ", `branchDepth`, `branchDepthSD`, `branchDepthMin`, `branchDepthMax`"
    << ", `branchDepthDelta`, `branchDepthDeltaSD`, `branchDepthDeltaMin`, `branchDepthDeltaMax`"
    << ", `trailDepth`, `trailDepthSD`, `trailDepthMin`, `trailDepthMax`"
    << ", `trailDepthDelta`, `trailDepthDeltaSD`, `trailDepthDeltaMin`,`trailDepthDeltaMax`"

    //Propagations
    << ", `propBinIrred` , `propBinRed` "
    << ", `propLongIrred` , `propLongRed`"

    //Conflicts
    << ", `conflBinIrred`, `conflBinRed`"
    << ", `conflLongIrred`, `conflLongRed`"

    //Reds
    << ", `learntUnits`, `learntBins`, `learntLongs`"

    //Resolutions
    << ", `resolBinIrred`, `resolBinRed`, `resolLIrred`, `resolLRed`"

    //Var stats
    << ", `propagations`"
    << ", `decisions`"
    << ", `flipped`, `varSetPos`, `varSetNeg`"
    << ", `free`, `replaced`, `eliminated`, `set`"
    << ", `clauseIDstartInclusive`, `clauseIDendExclusive`"
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
    , const SearchStats& thisStats
    , const Solver* solver
    , const Searcher* search
) {
    const SearchHist& searchHist = search->getHistory();
    const BinTriStats& binTri = solver->getBinTriStats();

    int bindAt = 1;
    sqlite3_bind_int64(stmtRst, bindAt++, runID);
    sqlite3_bind_int64(stmtRst, bindAt++, solver->get_solve_stats().numSimplify);
    sqlite3_bind_int64(stmtRst, bindAt++, search->sumRestarts());
    sqlite3_bind_int64(stmtRst, bindAt++, solver->sumConflicts);
    sqlite3_bind_double(stmtRst, bindAt++, cpuTime());


    sqlite3_bind_int64(stmtRst, bindAt++, binTri.irredBins);
    sqlite3_bind_int64(stmtRst, bindAt++, solver->get_num_long_irred_cls());

    sqlite3_bind_int64(stmtRst, bindAt++, binTri.redBins);
    sqlite3_bind_int64(stmtRst, bindAt++, solver->get_num_long_red_cls());

    sqlite3_bind_int64(stmtRst, bindAt++, solver->litStats.irredLits);
    sqlite3_bind_int64(stmtRst, bindAt++, solver->litStats.redLits);

    //Conflict stats
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.glueHist.getLongtTerm().avg());
    sqlite3_bind_double(stmtRst, bindAt++, std:: sqrt(searchHist.glueHist.getLongtTerm().var()));
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.glueHist.getLongtTerm().getMin());
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.glueHist.getLongtTerm().getMax());

    sqlite3_bind_double(stmtRst, bindAt++, searchHist.conflSizeHist.avg());
    sqlite3_bind_double(stmtRst, bindAt++, std:: sqrt(searchHist.conflSizeHist.var()));
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.conflSizeHist.getMin());
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.conflSizeHist.getMax());

    sqlite3_bind_double(stmtRst, bindAt++, searchHist.numResolutionsHist.avg());
    sqlite3_bind_double(stmtRst, bindAt++, std:: sqrt(searchHist.numResolutionsHist.var()));
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.numResolutionsHist.getMin());
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.numResolutionsHist.getMax());

    //Search stats
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.branchDepthHist.avg());
    sqlite3_bind_double(stmtRst, bindAt++, std:: sqrt(searchHist.branchDepthHist.var()));
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.branchDepthHist.getMin());
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.branchDepthHist.getMax());

    sqlite3_bind_double(stmtRst, bindAt++, searchHist.branchDepthDeltaHist.avg());
    sqlite3_bind_double(stmtRst, bindAt++, std:: sqrt(searchHist.branchDepthDeltaHist.var()));
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.branchDepthDeltaHist.getMin());
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.branchDepthDeltaHist.getMax());

    sqlite3_bind_double(stmtRst, bindAt++, searchHist.trailDepthHist.getLongtTerm().avg());
    sqlite3_bind_double(stmtRst, bindAt++, std:: sqrt(searchHist.trailDepthHist.getLongtTerm().var()));
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.trailDepthHist.getLongtTerm().getMin());
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.trailDepthHist.getLongtTerm().getMax());

    sqlite3_bind_double(stmtRst, bindAt++, searchHist.trailDepthDeltaHist.avg());
    sqlite3_bind_double(stmtRst, bindAt++, std:: sqrt(searchHist.trailDepthDeltaHist.var()));
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.trailDepthDeltaHist.getMin());
    sqlite3_bind_double(stmtRst, bindAt++, searchHist.trailDepthDeltaHist.getMax());

    //Prop
    sqlite3_bind_int64(stmtRst, bindAt++, thisPropStats.propsBinIrred);
    sqlite3_bind_int64(stmtRst, bindAt++, thisPropStats.propsBinRed);
    sqlite3_bind_int64(stmtRst, bindAt++, thisPropStats.propsLongIrred);
    sqlite3_bind_int64(stmtRst, bindAt++, thisPropStats.propsLongRed);

    //Confl
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.conflStats.conflsBinIrred);
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.conflStats.conflsBinRed);
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.conflStats.conflsLongIrred);
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.conflStats.conflsLongRed);

    //Red
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.learntUnits);
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.learntBins);
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.learntLongs);

    //Resolv stats
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.resolvs.binIrred);
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.resolvs.binRed);
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.resolvs.longIrred);
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.resolvs.longRed);


    //Var stats
    sqlite3_bind_int64(stmtRst, bindAt++, thisPropStats.propagations);
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.decisions);

    sqlite3_bind_int64(stmtRst, bindAt++, thisPropStats.varFlipped);
    sqlite3_bind_int64(stmtRst, bindAt++, thisPropStats.varSetPos);
    sqlite3_bind_int64(stmtRst, bindAt++, thisPropStats.varSetNeg);
    sqlite3_bind_int64(stmtRst, bindAt++, solver->get_num_free_vars());
    sqlite3_bind_int64(stmtRst, bindAt++, solver->varReplacer->get_num_replaced_vars());
    sqlite3_bind_int64(stmtRst, bindAt++, solver->get_num_vars_elimed());
    sqlite3_bind_int64(stmtRst, bindAt++, search->getTrailSize());

    //ClauseID
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.clauseID_at_start_inclusive);
    sqlite3_bind_int64(stmtRst, bindAt++, thisStats.clauseID_at_end_exclusive);

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
    const size_t numElems = 8;

    std::stringstream ss;
    ss << "insert into `reduceDB`"
    << "("
    //Position
    << "  `runID`, `simplifications`, `restarts`, `conflicts`, `runtime`"
    << ", `level`"

    //Actual data
    << ", `numReduceDBs`, `numRemoved`"
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
    uint64_t level
    , uint64_t num_cleans
    , uint64_t num_removed
    , const Solver* solver
) {

    int bindAt = 1;
    sqlite3_bind_int64(stmtReduceDB, bindAt++, runID);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, solver->get_solve_stats().numSimplify);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, solver->sumRestarts());
    sqlite3_bind_int64(stmtReduceDB, bindAt++, solver->sumConflicts);
    sqlite3_bind_double(stmtReduceDB, bindAt++, cpuTime());
    sqlite3_bind_int64(stmtReduceDB, bindAt++, level);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, num_cleans);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, num_removed);

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

void SQLiteStats::init_clause_stats_STMT()
{
    const size_t numElems = 45;

    std::stringstream ss;
    ss << "insert into `clauseStats`"
    << "("
    << " `runID`,"
    << " `simplifications`,"
    << " `restarts`,"
    << " `conflicts`,"
    << " `clauseID`,"
    << ""
    << " `glue`,"
    << " `size`,"
    << " `conflicts_this_restart`,"
    << " `num_overlap_literals`,"
    << " `num_antecedents`,"
    << " `antecedents_avg_size`,"

    << " `backtrack_level`,"
    << " `decision_level`,"
    << " `trail_depth_level`,"

    << " `atedecents_binIrred`,"
    << " `atedecents_binRed`,"
    << " `atedecents_longIrred`,"
    << " `atedecents_longRed`,"

    << " `vsids_vars_avg`,"
    << " `vsids_vars_var`,"
    << " `vsids_vars_max`,"

    << " `antecedents_glue_long_reds_avg`,"
    << " `antecedents_glue_long_reds_var`,"
    << " `antecedents_glue_long_reds_min`,"
    << " `antecedents_glue_long_reds_max`,"

    << " `antecedents_long_red_age_avg`,"
    << " `antecedents_long_red_age_var`,"
    << " `antecedents_long_red_age_min`,"
    << " `antecedents_long_red_age_max`,"

    << " `vsids_of_resolving_literals_avg`,"
    << " `vsids_of_resolving_literals_var`,"
    << " `vsids_of_resolving_literals_min`,"
    << " `vsids_of_resolving_literals_max`,"

    << " `vsids_of_all_incoming_lits_avg`,"
    << " `vsids_of_all_incoming_lits_var`,"
    << " `vsids_of_all_incoming_lits_min`,"
    << " `vsids_of_all_incoming_lits_max`,"

    << " `antecedents_antecedents_vsids_avg`,"

    << " `decision_level_hist`,"
    << " `backtrack_level_hist`,"
    << " `trail_depth_level_hist`,"
    << " `vsids_vars_hist`,"
    << " `size_hist`,"
    << " `glue_hist`,"
    << " `num_antecedents_hist`"
    << ") values ";
    writeQuestionMarks(
        numElems
        , ss
    );
    ss << ";";

    //Prepare the statement
    int rc = sqlite3_prepare(db, ss.str().c_str(), -1, &stmt_clause_stats, NULL);
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

void SQLiteStats::dump_clause_stats(
    const Solver* solver
    , uint64_t clauseID
    , uint32_t glue
    , uint32_t backtrack_level
    , uint32_t size
    , AtecedentData<uint16_t> antec_data
    , size_t decision_level
    , size_t trail_depth
    , uint64_t conflicts_this_restart
    , const SearchHist& hist
) {
    uint32_t num_overlap_literals = antec_data.sum_size()-(antec_data.num()-1)-size;

    int bindAt = 1;
    sqlite3_bind_int64(stmt_clause_stats, bindAt++, runID);
    sqlite3_bind_int64(stmt_clause_stats, bindAt++, solver->get_solve_stats().numSimplify);
    sqlite3_bind_int64(stmt_clause_stats, bindAt++, solver->sumRestarts());
    sqlite3_bind_int64(stmt_clause_stats, bindAt++, solver->sumConflicts);
    sqlite3_bind_int64(stmt_clause_stats, bindAt++, clauseID);

    sqlite3_bind_int(stmt_clause_stats, bindAt++, glue);
    sqlite3_bind_int(stmt_clause_stats, bindAt++, size);
    sqlite3_bind_int64(stmt_clause_stats, bindAt++, conflicts_this_restart);
    sqlite3_bind_int(stmt_clause_stats, bindAt++, num_overlap_literals);
    sqlite3_bind_int(stmt_clause_stats, bindAt++, antec_data.num());
    sqlite3_bind_double(stmt_clause_stats, bindAt++, (double)antec_data.sum_size()/(double)antec_data.num() );

    sqlite3_bind_int(stmt_clause_stats, bindAt++, backtrack_level);
    sqlite3_bind_int64(stmt_clause_stats, bindAt++, decision_level);
    sqlite3_bind_int64(stmt_clause_stats, bindAt++, trail_depth);

    sqlite3_bind_int(stmt_clause_stats, bindAt++, antec_data.binIrred);
    sqlite3_bind_int(stmt_clause_stats, bindAt++, antec_data.binRed);
    sqlite3_bind_int(stmt_clause_stats, bindAt++, antec_data.longIrred);
    sqlite3_bind_int(stmt_clause_stats, bindAt++, antec_data.longRed);

    sqlite3_bind_double(stmt_clause_stats, bindAt++, antec_data.vsids_vars.avg());
    sqlite3_bind_double(stmt_clause_stats, bindAt++, antec_data.vsids_vars.var());
    sqlite3_bind_double(stmt_clause_stats, bindAt++, antec_data.vsids_vars.getMin());
    sqlite3_bind_double(stmt_clause_stats, bindAt++, antec_data.vsids_vars.getMax());

    sqlite3_bind_double(stmt_clause_stats, bindAt++, antec_data.glue_long_reds.avg());
    sqlite3_bind_double(stmt_clause_stats, bindAt++, antec_data.glue_long_reds.var());
    sqlite3_bind_int(stmt_clause_stats, bindAt++, antec_data.glue_long_reds.getMin());

    sqlite3_bind_double(stmt_clause_stats, bindAt++, antec_data.age_long_reds.avg() );
    sqlite3_bind_double(stmt_clause_stats, bindAt++, antec_data.age_long_reds.var() );
    sqlite3_bind_int64(stmt_clause_stats, bindAt++, antec_data.age_long_reds.getMin() );
    sqlite3_bind_int64(stmt_clause_stats, bindAt++, antec_data.age_long_reds.getMax() );

    sqlite3_bind_double(stmt_clause_stats, bindAt++, antec_data.vsids_of_resolving_literals.avg());
    sqlite3_bind_double(stmt_clause_stats, bindAt++, antec_data.vsids_of_resolving_literals.var());
    sqlite3_bind_double(stmt_clause_stats, bindAt++, antec_data.vsids_of_resolving_literals.getMin());
    sqlite3_bind_double(stmt_clause_stats, bindAt++, antec_data.vsids_of_resolving_literals.getMax());

    sqlite3_bind_double(stmt_clause_stats, bindAt++, antec_data.vsids_all_incoming_vars.avg());
    sqlite3_bind_double(stmt_clause_stats, bindAt++, antec_data.vsids_all_incoming_vars.var());
    sqlite3_bind_double(stmt_clause_stats, bindAt++, antec_data.vsids_all_incoming_vars.getMin());
    sqlite3_bind_double(stmt_clause_stats, bindAt++, antec_data.vsids_all_incoming_vars.getMax());

    sqlite3_bind_double(stmt_clause_stats, bindAt++, antec_data.vsids_of_ants.avg());

    sqlite3_bind_double(stmt_clause_stats, bindAt++, hist.decisionLevelHistLT.avg());
    sqlite3_bind_double(stmt_clause_stats, bindAt++, hist.backtrackLevelHistLT.avg());
    sqlite3_bind_double(stmt_clause_stats, bindAt++, hist.trailDepthHistLT.avg());
    sqlite3_bind_double(stmt_clause_stats, bindAt++, hist.vsidsVarsAvgLT.avg());
    sqlite3_bind_double(stmt_clause_stats, bindAt++, hist.conflSizeHistLT.avg());
    sqlite3_bind_double(stmt_clause_stats, bindAt++, hist.glueHistLT.avg());
    sqlite3_bind_double(stmt_clause_stats, bindAt++, hist.numResolutionsHistLT.avg());

    int rc = sqlite3_step(stmt_clause_stats);
    if (rc != SQLITE_DONE) {
        cout
        << "ERROR: while executing clause DB cleaning SQLite prepared statement"
        << endl;

        cout << "Error from sqlite: "
        << sqlite3_errmsg(db)
        << endl;
        std::exit(-1);
    }

    if (sqlite3_reset(stmt_clause_stats)) {
        cerr << "Error calling sqlite3_reset on stmt_clause_stats" << endl;
        std::exit(-1);
    }

    if (sqlite3_clear_bindings(stmt_clause_stats)) {
        cerr << "Error calling sqlite3_clear_bindings on stmt_clause_stats" << endl;
        std::exit(-1);
    }
}
