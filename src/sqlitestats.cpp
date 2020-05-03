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

#define bind_null_or_double(stmt,bindat,stucture,func) \
{ \
    if (stucture.num_data_elements() == 0) {\
        sqlite3_bind_null(stmt, bindat); \
    } else { \
        sqlite3_bind_double(stmt, bindat, stucture.func()); \
    }\
    bindat++; \
}

#define bind_null_or_int(stmt,bindat,stucture,func) \
{ \
    if (stucture.num_data_elements() == 0) {\
        sqlite3_bind_null(stmt, bindat); \
    } else { \
        sqlite3_bind_int(stmt, bindat, stucture.func()); \
    }\
    bindat++; \
}

#define bind_null_or_int64(stmt,bindat,stucture,func) \
{ \
    if (stucture.num_data_elements() == 0) {\
        sqlite3_bind_null(stmt, bindat); \
    } else { \
        sqlite3_bind_int64(stmt, bindat, stucture.func()); \
    }\
    bindat++; \
}

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using namespace CMSat;

const char* rst_dat_type_to_str(rst_dat_type type) {
    static const char* const norm ="restart_norm";
    static const char* const var ="restart_var";
    static const char* const cl ="restart_cl";
    if (type == rst_dat_type::norm) {
        return norm;
    } else if (type == rst_dat_type::var) {
        return var;
    } else if (type == rst_dat_type::cl) {
        return cl;
    } else {
        assert(false);
    }
    assert(false);
    exit(-1);
}

SQLiteStats::SQLiteStats(std::string _filename) :
        filename(_filename)
{
}

vector<string> SQLiteStats::get_columns(const char* tablename)
{
    vector<string> ret;

    std::stringstream q;
    q << "pragma table_info(" << tablename << ");";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, q.str().c_str(), -1, &stmt, NULL)) {
        cerr << "ERROR: Couln't create table structure for SQLite: "
        << sqlite3_errmsg(db)
        << endl;
        std::exit(-1);
    }

    sqlite3_bind_int(stmt, 1, 16);
    int rc;
    while ( (rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        ret.push_back(string((const char*)sqlite3_column_text(stmt, 1)));
    }
    sqlite3_finalize(stmt);

    return ret;
}

void SQLiteStats::del_prepared_stmt(sqlite3_stmt* stmt)
{
    if (stmt == NULL) {
        return;
    }

    int ret = sqlite3_finalize(stmt);
    if (ret != SQLITE_OK) {
        cout << "Error closing prepared statement" << endl;
        std::exit(-1);
    }
}


SQLiteStats::~SQLiteStats()
{
    if (!setup_ok)
        return;

    //Free all the prepared statements
    del_prepared_stmt(stmtRst);
    del_prepared_stmt(stmtVarRst);
    del_prepared_stmt(stmtClRst);
    del_prepared_stmt(stmtFeat);
    del_prepared_stmt(stmtReduceDB);
    del_prepared_stmt(stmtTimePassed);
    del_prepared_stmt(stmtMemUsed);
    del_prepared_stmt(stmt_clause_stats);
    del_prepared_stmt(stmt_delete_cl);
    del_prepared_stmt(stmt_var_data_picktime);
    del_prepared_stmt(stmt_var_data_fintime);
    del_prepared_stmt(stmt_dec_var_clid);
    del_prepared_stmt(stmt_var_dist);

    //Close clonnection
    sqlite3_close(db);
}

bool SQLiteStats::setup(const Solver* solver)
{
    setup_ok = connectServer(solver);
    if (!setup_ok) {
        return false;
    }

    //TODO check if data is in any table
    if (sqlite3_exec(db, cmsat_tablestructure_sql, NULL, NULL, NULL)) {
        cerr << "ERROR: Couln't create table structure for SQLite: "
        << sqlite3_errmsg(db)
        << endl;
        std::exit(-1);
    }

    add_solverrun(solver);
    addStartupData();
    init("timepassed", &stmtTimePassed);
    init("memused", &stmtMemUsed);
    init("satzilla_features", &stmtFeat);
    init("clause_stats", &stmt_clause_stats);
    init("restart", &stmtRst);
    init("restart_dat_for_var", &stmtVarRst);
    init("restart_dat_for_cl", &stmtClRst);
    init("reduceDB", &stmtReduceDB);
    #ifdef STATS_NEEDED
    init("var_data_fintime", &stmt_var_data_fintime);
    init("var_data_picktime", &stmt_var_data_picktime);
    init("dec_var_clid", &stmt_dec_var_clid);
    init("cl_last_in_solver", &stmt_delete_cl);
    init("var_dist", &stmt_var_dist);
    #endif

    return true;
}


bool file_exists (const std::string& name) {
    std::ifstream f(name.c_str());
    return f.good();
}


bool SQLiteStats::connectServer(const Solver* solver)
{
    if (file_exists(filename) && !solver->conf.sql_overwrite_file) {
        cout << "ERROR -- the database already exists: " << filename << endl;
        cout << "ERROR -- We cannot store more than one run in the same database"
        << endl
        << "Exiting." << endl;
        exit(-1);
    }

    int rc = sqlite3_open(filename.c_str(), &db);
    if(rc) {
        cout << "c Cannot open sqlite database: " << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        return false;
    }

    if (sqlite3_exec(db, "PRAGMA synchronous = OFF", NULL, NULL, NULL)) {
        cerr << "ERROR: Problem setting pragma synchronous = OFF to SQLite DB" << endl;
        cerr << "c " << sqlite3_errmsg(db) << endl;
        std::exit(-1);
    }

    if (sqlite3_exec(db, "PRAGMA journal_mode = MEMORY", NULL, NULL, NULL)) {
        cerr << "ERROR: Problem setting pragma journal_mode = MEMORY to SQLite DB" << endl;
        cerr << "c " << sqlite3_errmsg(db) << endl;
        std::exit(-1);
    }


    if (solver->conf.verbosity) {
        cout << "c writing to SQLite file: " << filename << endl;
    }

    return true;
}

void SQLiteStats::begin_transaction()
{
    if (sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL)) {
        cerr << "ERROR: Beginning SQLITE transaction" << endl;
        cerr << "c " << sqlite3_errmsg(db) << endl;
        std::exit(-1);
    }
}

void SQLiteStats::end_transaction()
{
    if (sqlite3_exec(db, "END TRANSACTION", NULL, NULL, NULL)) {
        cerr << "ERROR: Beginning SQLITE transaction" << endl;
        cerr << "c " << sqlite3_errmsg(db) << endl;
        std::exit(-1);
    }
}

bool SQLiteStats::add_solverrun(const Solver* solver)
{
    std::stringstream ss;
    ss
    << "INSERT INTO solverRun (`runtime`, `gitrev`) values ("
    << time(NULL)
    << ", '" << solver->get_version_sha1() << "'"
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

void SQLiteStats::add_tag(const std::pair<string, string>& tag)
{
    std::stringstream ss;
    ss
    << "INSERT INTO `tags` (`name`, `val`) VALUES("
    << "'" << tag.first << "'"
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
    << "INSERT INTO `startup` (`startTime`) VALUES ("
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
    << "INSERT INTO `finishup` (`endTime`, `status`) VALUES ("
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

void SQLiteStats::run_sqlite_step(sqlite3_stmt* stmt, const char* name)
{
    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        cout
        << "ERROR: while executing '" << name << "' SQLite prepared statement"
        << endl;

        cout << "Error from sqlite: "
        << sqlite3_errmsg(db)
        << endl;
        cout << "Error code from sqlite: " << rc << endl;
        std::exit(-1);
    }

    if (sqlite3_reset(stmt)) {
        cerr << "Error calling sqlite3_reset on cl_last_in_solver" << endl;
        std::exit(-1);
    }

    if (sqlite3_clear_bindings(stmt)) {
        cerr << "Error calling sqlite3_clear_bindings on '"
        << name << "'" << endl;
        std::exit(-1);
    }
}


void SQLiteStats::init(const char* name, sqlite3_stmt** stmt)
{
    vector<string> cols = get_columns(name);
    const size_t numElems = cols.size();

    std::stringstream ss;
    ss << "insert into `" << name << "` (";
    for(uint32_t i = 0; i < cols.size(); i++) {
        if (i > 0) {
            ss << ", ";
        }
        ss << "`" << cols[i] << "`";
    }
    ss << ") values ";
    writeQuestionMarks(
        numElems
        , ss
    );
    ss << ";";

    //Prepare the statement
    if (sqlite3_prepare(db, ss.str().c_str(), -1, stmt, NULL)) {
        cerr << "ERROR in sqlite_stmt_prepare(), INSERT failed"
        << endl
        << sqlite3_errmsg(db)
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
    sqlite3_bind_int64(stmtMemUsed, bindAt++, solver->get_solve_stats().num_simplify);
    sqlite3_bind_int64(stmtMemUsed, bindAt++, solver->sumConflicts);
    sqlite3_bind_double(stmtMemUsed, bindAt++, given_time);
    //memory stats
    sqlite3_bind_text(stmtMemUsed, bindAt++, name.c_str(), -1, NULL);
    sqlite3_bind_int(stmtMemUsed, bindAt++, mem_used_mb);

    run_sqlite_step(stmtMemUsed, "memused");
}

void SQLiteStats::time_passed(
    const Solver* solver
    , const string& name
    , double time_passed
    , bool time_out
    , double percent_time_remain
) {

    int bindAt = 1;
    sqlite3_bind_int64(stmtTimePassed, bindAt++, solver->get_solve_stats().num_simplify);
    sqlite3_bind_int64(stmtTimePassed, bindAt++, solver->sumConflicts);
    sqlite3_bind_double(stmtTimePassed, bindAt++, cpuTime());
    sqlite3_bind_text(stmtTimePassed, bindAt++, name.c_str(), -1, NULL);
    sqlite3_bind_double(stmtTimePassed, bindAt++, time_passed);
    sqlite3_bind_int(stmtTimePassed, bindAt++, time_out);
    sqlite3_bind_double(stmtTimePassed, bindAt++, percent_time_remain);

    run_sqlite_step(stmtTimePassed, "timepassed");
}

void SQLiteStats::time_passed_min(
    const Solver* solver
    , const string& name
    , double time_passed
) {
    int bindAt = 1;
    sqlite3_bind_int64(stmtTimePassed, bindAt++, solver->get_solve_stats().num_simplify);
    sqlite3_bind_int64(stmtTimePassed, bindAt++, solver->sumConflicts);
    sqlite3_bind_double(stmtTimePassed, bindAt++, cpuTime());
    sqlite3_bind_text(stmtTimePassed, bindAt++, name.c_str(), -1, NULL);
    sqlite3_bind_double(stmtTimePassed, bindAt++, time_passed);
    sqlite3_bind_null(stmtTimePassed, bindAt++);
    sqlite3_bind_null(stmtTimePassed, bindAt++);

    run_sqlite_step(stmtTimePassed, "time_passed_min");
}

void SQLiteStats::satzilla_features(
    const Solver* solver
    , const Searcher* search
    , const SatZillaFeatures& satzilla_feat
) {
    int bindAt = 1;
    sqlite3_bind_int64(stmtFeat, bindAt++, solver->get_solve_stats().num_simplify);
    sqlite3_bind_int64(stmtFeat, bindAt++, search->sumRestarts());
    sqlite3_bind_int64(stmtFeat, bindAt++, solver->sumConflicts);
    sqlite3_bind_int(stmtFeat, bindAt++, solver->latest_satzilla_feature_calc);

    sqlite3_bind_int64(stmtFeat, bindAt++, (uint64_t)satzilla_feat.numVars);
    sqlite3_bind_int64(stmtFeat, bindAt++, (uint64_t)satzilla_feat.numClauses);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.var_cl_ratio);

    //Clause distribution
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.binary);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.horn);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.horn_mean);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.horn_std);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.horn_min);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.horn_max);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.horn_spread);

    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.vcg_var_mean);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.vcg_var_std);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.vcg_var_min);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.vcg_var_max);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.vcg_var_spread);

    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.vcg_cls_mean);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.vcg_cls_std);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.vcg_cls_min);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.vcg_cls_max);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.vcg_cls_spread);

    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.pnr_var_mean);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.pnr_var_std);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.pnr_var_min);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.pnr_var_max);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.pnr_var_spread);

    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.pnr_cls_mean);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.pnr_cls_std);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.pnr_cls_min);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.pnr_cls_max);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.pnr_cls_spread);

    //Conflict clauses
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.avg_confl_size);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.confl_size_min);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.confl_size_max);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.avg_confl_glue);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.confl_glue_min);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.confl_glue_max);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.avg_num_resolutions);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.num_resolutions_min);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.num_resolutions_max);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.learnt_bins_per_confl);

    //Search
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.avg_branch_depth);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.branch_depth_min);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.branch_depth_max);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.avg_trail_depth_delta);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.trail_depth_delta_min);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.trail_depth_delta_max);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.avg_branch_depth_delta);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.props_per_confl);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.confl_per_restart);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.decisions_per_conflict);

    //red stats
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.red_cl_distrib.glue_distr_mean);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.red_cl_distrib.glue_distr_var);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.red_cl_distrib.size_distr_mean);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.red_cl_distrib.size_distr_var);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.red_cl_distrib.activity_distr_mean);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.red_cl_distrib.activity_distr_var);

    //irred stats
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.irred_cl_distrib.glue_distr_mean);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.irred_cl_distrib.glue_distr_var);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.irred_cl_distrib.size_distr_mean);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.irred_cl_distrib.size_distr_var);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.irred_cl_distrib.activity_distr_mean);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.irred_cl_distrib.activity_distr_var);

    run_sqlite_step(stmtFeat, "satzilla_features");
}

#ifdef STATS_NEEDED
void SQLiteStats::restart(
    const uint32_t restartID
    , const Restart rest_type
    , const PropStats& thisPropStats
    , const SearchStats& thisStats
    , const Solver* solver
    , const Searcher* search
    , const rst_dat_type type
    , const int64_t clauseID
) {
    sqlite3_stmt* stmt;
    if (type == rst_dat_type::norm) {
        stmt = stmtRst;
    } else if (type == rst_dat_type::var) {
        stmt = stmtVarRst;
    } else if (type == rst_dat_type::cl) {
        stmt = stmtClRst;
    } else {
        assert(false);
    }

    const SearchHist& searchHist = search->getHistory();
    const BinTriStats& binTri = solver->getBinTriStats();

    int bindAt = 1;
    sqlite3_bind_int64(stmt, bindAt++, restartID);
    if (clauseID == -1) {
        sqlite3_bind_null(stmt, bindAt++);
    } else {
        sqlite3_bind_int64(stmt, bindAt++, clauseID);
    }
    sqlite3_bind_int64(stmt, bindAt++, solver->get_solve_stats().num_simplify);
    sqlite3_bind_int64(stmt, bindAt++, search->sumRestarts());
    sqlite3_bind_int64(stmt, bindAt++, solver->sumConflicts);
    sqlite3_bind_int(stmt, bindAt++, searchHist.num_conflicts_this_restart);
    sqlite3_bind_int(stmt, bindAt++, solver->latest_satzilla_feature_calc);
    sqlite3_bind_double(stmt, bindAt++, cpuTime());


    sqlite3_bind_int64(stmt, bindAt++, binTri.irredBins);
    sqlite3_bind_int64(stmt, bindAt++, solver->get_num_long_irred_cls());
    sqlite3_bind_int64(stmt, bindAt++, binTri.redBins);
    sqlite3_bind_int64(stmt, bindAt++, solver->get_num_long_red_cls());

    sqlite3_bind_int64(stmt, bindAt++, solver->litStats.irredLits);
    sqlite3_bind_int64(stmt, bindAt++, solver->litStats.redLits);

    //Conflict stats
    bind_null_or_double(stmt, bindAt,   searchHist.glueHist.getLongtTerm(),avg);
    sqlite3_bind_double(stmt, bindAt++, std:: sqrt(searchHist.glueHist.getLongtTerm().var()));
    bind_null_or_double(stmt, bindAt,   searchHist.glueHist.getLongtTerm(),getMin);
    bind_null_or_double(stmt, bindAt,   searchHist.glueHist.getLongtTerm(),getMax);

    bind_null_or_double(stmt, bindAt,   searchHist.conflSizeHist, avg);
    sqlite3_bind_double(stmt, bindAt++, std:: sqrt(searchHist.conflSizeHist.var()));
    bind_null_or_double(stmt, bindAt,   searchHist.conflSizeHist,getMin);
    bind_null_or_double(stmt, bindAt,   searchHist.conflSizeHist,getMax);

    bind_null_or_double(stmt, bindAt,   searchHist.numResolutionsHist, avg);
    sqlite3_bind_double(stmt, bindAt++, std:: sqrt(searchHist.numResolutionsHist.var()));
    bind_null_or_double(stmt, bindAt,   searchHist.numResolutionsHist,getMin);
    bind_null_or_double(stmt, bindAt,   searchHist.numResolutionsHist,getMax);

    //Search stats
    bind_null_or_double(stmt, bindAt,   searchHist.branchDepthHist,avg);
    sqlite3_bind_double(stmt, bindAt++, std:: sqrt(searchHist.branchDepthHist.var()));
    bind_null_or_double(stmt, bindAt, searchHist.branchDepthHist,getMin);
    bind_null_or_double(stmt, bindAt, searchHist.branchDepthHist,getMax);

    bind_null_or_double(stmt, bindAt,   searchHist.branchDepthDeltaHist,avg);
    sqlite3_bind_double(stmt, bindAt++, std:: sqrt(searchHist.branchDepthDeltaHist.var()));
    bind_null_or_double(stmt, bindAt,   searchHist.branchDepthDeltaHist,getMin);
    bind_null_or_double(stmt, bindAt,   searchHist.branchDepthDeltaHist,getMax);

    bind_null_or_double(stmt, bindAt, searchHist.trailDepthHist.getLongtTerm(),avg);
    sqlite3_bind_double(stmt, bindAt++, std:: sqrt(searchHist.trailDepthHist.getLongtTerm().var()));
    bind_null_or_double(stmt, bindAt,   searchHist.trailDepthHist.getLongtTerm(),getMin);
    bind_null_or_double(stmt, bindAt,   searchHist.trailDepthHist.getLongtTerm(),getMax);

    bind_null_or_double(stmt, bindAt,   searchHist.trailDepthDeltaHist,avg);
    sqlite3_bind_double(stmt, bindAt++, std:: sqrt(searchHist.trailDepthDeltaHist.var()));
    bind_null_or_double(stmt, bindAt,   searchHist.trailDepthDeltaHist,getMin);
    bind_null_or_double(stmt, bindAt,   searchHist.trailDepthDeltaHist,getMax);

    //Prop
    sqlite3_bind_int64(stmt, bindAt++, thisPropStats.propsBinIrred);
    sqlite3_bind_int64(stmt, bindAt++, thisPropStats.propsBinRed);
    sqlite3_bind_int64(stmt, bindAt++, thisPropStats.propsLongIrred);
    sqlite3_bind_int64(stmt, bindAt++, thisPropStats.propsLongRed);

    //Confl
    sqlite3_bind_int64(stmt, bindAt++, thisStats.conflStats.conflsBinIrred);
    sqlite3_bind_int64(stmt, bindAt++, thisStats.conflStats.conflsBinRed);
    sqlite3_bind_int64(stmt, bindAt++, thisStats.conflStats.conflsLongIrred);
    sqlite3_bind_int64(stmt, bindAt++, thisStats.conflStats.conflsLongRed);

    //Red
    sqlite3_bind_int64(stmt, bindAt++, thisStats.learntUnits);
    sqlite3_bind_int64(stmt, bindAt++, thisStats.learntBins);
    sqlite3_bind_int64(stmt, bindAt++, thisStats.learntLongs);

    //Resolv stats
    sqlite3_bind_int64(stmt, bindAt++, thisStats.resolvs.binIrred);
    sqlite3_bind_int64(stmt, bindAt++, thisStats.resolvs.binRed);
    sqlite3_bind_int64(stmt, bindAt++, thisStats.resolvs.longIrred);
    sqlite3_bind_int64(stmt, bindAt++, thisStats.resolvs.longRed);


    //Var stats
    sqlite3_bind_int64(stmt, bindAt++, thisPropStats.propagations);
    sqlite3_bind_int64(stmt, bindAt++, thisStats.decisions);

    sqlite3_bind_int64(stmt, bindAt++, thisPropStats.varFlipped);
    sqlite3_bind_int64(stmt, bindAt++, thisPropStats.varSetPos);
    sqlite3_bind_int64(stmt, bindAt++, thisPropStats.varSetNeg);
    sqlite3_bind_int64(stmt, bindAt++, solver->get_num_free_vars());
    sqlite3_bind_int64(stmt, bindAt++, solver->varReplacer->get_num_replaced_vars());
    sqlite3_bind_int64(stmt, bindAt++, solver->get_num_vars_elimed());
    sqlite3_bind_int64(stmt, bindAt++, search->getTrailSize());

    //strategy
    sqlite3_bind_int(stmt, bindAt++, branch_type_to_int(solver->branch_strategy));
    sqlite3_bind_int(stmt, bindAt++, restart_type_to_int(rest_type));

    run_sqlite_step(stmt, rst_dat_type_to_str(type));
}

void SQLiteStats::reduceDB(
    const Solver* solver
    , const bool locked
    , const Clause* cl
    , const string& cur_restart_type
    , const uint32_t act_ranking_top_10
    , const uint32_t act_ranking
    , const uint32_t tot_cls_in_db
) {
    assert(cl->stats.dump_no != std::numeric_limits<uint16_t>::max());

    int bindAt = 1;
    sqlite3_bind_int64(stmtReduceDB, bindAt++, solver->get_solve_stats().num_simplify);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, solver->sumRestarts());
    sqlite3_bind_int64(stmtReduceDB, bindAt++, solver->sumConflicts);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, solver->latest_satzilla_feature_calc);
    sqlite3_bind_text(stmtReduceDB, bindAt++, cur_restart_type.c_str(), -1, NULL);
    sqlite3_bind_double(stmtReduceDB, bindAt++, cpuTime());

    //data
    sqlite3_bind_int64(stmtReduceDB, bindAt++, cl->stats.ID);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, cl->stats.dump_no);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, cl->stats.conflicts_made);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, cl->stats.propagations_made);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, cl->stats.sum_propagations_made);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, cl->stats.clause_looked_at);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, cl->stats.used_for_uip_creation);

    int64_t last_touched_diff = solver->sumConflicts-cl->stats.last_touched;
    sqlite3_bind_int64(stmtReduceDB, bindAt++, last_touched_diff);

    sqlite3_bind_double(stmtReduceDB, bindAt++, (double)cl->stats.activity/(double)solver->get_cla_inc());
    sqlite3_bind_int(stmtReduceDB, bindAt++, locked);
    sqlite3_bind_int(stmtReduceDB, bindAt++, cl->used_in_xor());
    sqlite3_bind_int(stmtReduceDB, bindAt++, cl->stats.glue);
    sqlite3_bind_int(stmtReduceDB, bindAt++, cl->size());
    sqlite3_bind_int(stmtReduceDB, bindAt++, cl->stats.ttl);
    sqlite3_bind_int(stmtReduceDB, bindAt++, cl->is_ternary_resolvent);
    sqlite3_bind_int(stmtReduceDB, bindAt++, act_ranking_top_10);
    sqlite3_bind_int(stmtReduceDB, bindAt++, act_ranking);
    sqlite3_bind_int(stmtReduceDB, bindAt++, tot_cls_in_db);
    sqlite3_bind_int(stmtReduceDB, bindAt++, cl->stats.sum_uip1_used);

    run_sqlite_step(stmtReduceDB, "reduceDB");
}

void SQLiteStats::dump_clause_stats(
    const Solver* solver
    , uint64_t clid
    , const uint64_t restartID
    , uint32_t orig_glue
    , uint32_t glue_before_minim
    , uint32_t backtrack_level
    , uint32_t size
    , AtecedentData<uint16_t> antec_data
    , size_t decision_level
    , size_t trail_depth
    , uint64_t conflicts_this_restart
    , const std::string& restart_type
    , const SearchHist& hist
    , const bool is_decision
) {
    uint32_t num_overlap_literals = antec_data.sum_size()-(antec_data.num()-1)-size;

    int bindAt = 1;
    sqlite3_bind_int64(stmt_clause_stats, bindAt++, solver->get_solve_stats().num_simplify);
    sqlite3_bind_int64(stmt_clause_stats, bindAt++, solver->sumRestarts());
    if (solver->sumRestarts() == 0) {
        sqlite3_bind_int64(stmt_clause_stats, bindAt++, 0);
    } else {
        sqlite3_bind_int64(stmt_clause_stats, bindAt++, solver->sumRestarts()-1);
    }
    sqlite3_bind_int64 (stmt_clause_stats, bindAt++, solver->sumConflicts);
    sqlite3_bind_int   (stmt_clause_stats, bindAt++, solver->latest_satzilla_feature_calc);
    sqlite3_bind_int64 (stmt_clause_stats, bindAt++, clid);
    sqlite3_bind_int   (stmt_clause_stats, bindAt++, restartID);

    sqlite3_bind_int   (stmt_clause_stats, bindAt++, orig_glue);
    sqlite3_bind_int   (stmt_clause_stats, bindAt++, glue_before_minim);
    sqlite3_bind_int   (stmt_clause_stats, bindAt++, size);
    sqlite3_bind_int64 (stmt_clause_stats, bindAt++, conflicts_this_restart);
    sqlite3_bind_int   (stmt_clause_stats, bindAt++, num_overlap_literals);
    sqlite3_bind_int   (stmt_clause_stats, bindAt++, antec_data.num());
    sqlite3_bind_int   (stmt_clause_stats, bindAt++, antec_data.sum_size());
    sqlite3_bind_int   (stmt_clause_stats, bindAt++, is_decision);

    sqlite3_bind_int   (stmt_clause_stats, bindAt++, backtrack_level);
    sqlite3_bind_int64 (stmt_clause_stats, bindAt++, decision_level);
    sqlite3_bind_int64 (stmt_clause_stats, bindAt++, hist.branchDepthHistQueue.prev(1));
    sqlite3_bind_int64 (stmt_clause_stats, bindAt++, hist.branchDepthHistQueue.prev(2));
    sqlite3_bind_int64 (stmt_clause_stats, bindAt++, trail_depth);
    sqlite3_bind_text  (stmt_clause_stats, bindAt++,  restart_type.c_str(), -1, NULL);

    sqlite3_bind_int   (stmt_clause_stats, bindAt++, antec_data.binIrred);
    sqlite3_bind_int   (stmt_clause_stats, bindAt++, antec_data.binRed);
    sqlite3_bind_int   (stmt_clause_stats, bindAt++, antec_data.longIrred);
    sqlite3_bind_int   (stmt_clause_stats, bindAt++, antec_data.longRed);

    bind_null_or_double(stmt_clause_stats, bindAt, antec_data.glue_long_reds,avg);
    bind_null_or_double(stmt_clause_stats, bindAt, antec_data.glue_long_reds,avg);
    bind_null_or_double(stmt_clause_stats, bindAt, antec_data.glue_long_reds,var);
    bind_null_or_int   (stmt_clause_stats, bindAt, antec_data.glue_long_reds,getMin);
    bind_null_or_int   (stmt_clause_stats, bindAt, antec_data.glue_long_reds,getMax);

    bind_null_or_double(stmt_clause_stats, bindAt, antec_data.age_long_reds,avg);
    bind_null_or_double(stmt_clause_stats, bindAt, antec_data.age_long_reds,var);
    bind_null_or_int64 (stmt_clause_stats, bindAt, antec_data.age_long_reds,getMin);
    bind_null_or_int64 (stmt_clause_stats, bindAt, antec_data.age_long_reds,getMax);

    bind_null_or_double(stmt_clause_stats, bindAt, hist.decisionLevelHistLT,avg);
    bind_null_or_double(stmt_clause_stats, bindAt, hist.backtrackLevelHistLT,avg);
    bind_null_or_double(stmt_clause_stats, bindAt, hist.trailDepthHistLT,avg);
    bind_null_or_double(stmt_clause_stats, bindAt, hist.conflSizeHistLT,avg);
    bind_null_or_double(stmt_clause_stats, bindAt, hist.glueHistLT,avg);
    bind_null_or_double(stmt_clause_stats, bindAt, hist.numResolutionsHistLT,avg);

    bind_null_or_double(stmt_clause_stats, bindAt, hist.antec_data_sum_sizeHistLT,avg);
    bind_null_or_double(stmt_clause_stats, bindAt, hist.overlapHistLT,avg);

    sqlite3_bind_double(stmt_clause_stats, bindAt++, hist.branchDepthHistQueue.avg_nocheck());
    sqlite3_bind_double(stmt_clause_stats, bindAt++, hist.trailDepthHist.avg_nocheck());
    sqlite3_bind_double(stmt_clause_stats, bindAt++, hist.trailDepthHistLonger.avg_nocheck());
    bind_null_or_double(stmt_clause_stats, bindAt,   hist.numResolutionsHist,avg);
    bind_null_or_double(stmt_clause_stats, bindAt,   hist.conflSizeHist,avg);
    bind_null_or_double(stmt_clause_stats, bindAt,   hist.trailDepthDeltaHist,avg);
    sqlite3_bind_double(stmt_clause_stats, bindAt++, hist.backtrackLevelHist.avg_nocheck());
    sqlite3_bind_double(stmt_clause_stats, bindAt++, hist.glueHist.avg_nocheck());
    bind_null_or_double(stmt_clause_stats, bindAt,   hist.glueHist.getLongtTerm(),avg);

    run_sqlite_step(stmt_clause_stats, "dump_clause_stats");
}

#ifdef STATS_NEEDED_BRANCH
void SQLiteStats::var_data_fintime(
    const Solver* solver
    , const uint32_t var
    , const VarData& vardata
    , const double rel_activity
) {
    int bindAt = 1;
    sqlite3_bind_int   (stmt_var_data_fintime, bindAt++, var);
    sqlite3_bind_int64 (stmt_var_data_fintime, bindAt++, vardata.sumConflicts_at_picktime);

    sqlite3_bind_double (stmt_var_data_fintime, bindAt++, rel_activity);

    sqlite3_bind_int64 (stmt_var_data_fintime, bindAt++, vardata.inside_conflict_clause);
    sqlite3_bind_int64 (stmt_var_data_fintime, bindAt++, vardata.inside_conflict_clause_antecedents);
    sqlite3_bind_int64 (stmt_var_data_fintime, bindAt++, vardata.inside_conflict_clause_glue);

    sqlite3_bind_int64 (stmt_var_data_fintime, bindAt++, solver->sumDecisions);
    sqlite3_bind_int64 (stmt_var_data_fintime, bindAt++, solver->sumConflicts);
    sqlite3_bind_int64 (stmt_var_data_fintime, bindAt++, solver->sumPropagations);
    sqlite3_bind_int64 (stmt_var_data_fintime, bindAt++, solver->sumAntecedents);
    sqlite3_bind_int64 (stmt_var_data_fintime, bindAt++, solver->sumAntecedentsLits);
    sqlite3_bind_int64 (stmt_var_data_fintime, bindAt++, solver->sumConflictClauseLits);
    sqlite3_bind_int64 (stmt_var_data_fintime, bindAt++, solver->sumDecisionBasedCl);
    sqlite3_bind_int64 (stmt_var_data_fintime, bindAt++, solver->sumClLBD);
    sqlite3_bind_int64 (stmt_var_data_fintime, bindAt++, solver->sumClSize);

    run_sqlite_step(stmt_var_data_fintime, "var_data_fintime");
}

void SQLiteStats::var_data_picktime(
    const Solver* solver
    , const uint32_t var
    , const VarData& vardata
    , const double rel_activity
) {
    int bindAt = 1;
    sqlite3_bind_int   (stmt_var_data_picktime, bindAt++, var);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, vardata.level);
    sqlite3_bind_double(stmt_var_data_picktime, bindAt++, rel_activity);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, solver->latest_vardist_feature_calc);

    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, vardata.inside_conflict_clause);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, vardata.inside_conflict_clause_antecedents);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, vardata.inside_conflict_clause_glue);

    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, vardata.inside_conflict_clause_during);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, vardata.inside_conflict_clause_antecedents_during);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, vardata.inside_conflict_clause_glue_during);


    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, vardata.num_decided);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, vardata.num_decided_pos);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, vardata.num_propagated);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, vardata.num_propagated_pos);

    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, solver->sumConflicts-vardata.last_seen_in_1uip);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, solver->sumConflicts-vardata.last_decided_on);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, solver->sumConflicts-vardata.last_propagated);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, solver->sumConflicts-vardata.last_canceled);


    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, solver->sumDecisions);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, solver->sumConflicts);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, solver->sumPropagations);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, solver->sumAntecedents);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, solver->sumAntecedentsLits);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, solver->sumConflictClauseLits);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, solver->sumDecisionBasedCl);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, solver->sumClLBD);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, solver->sumClSize);

    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, vardata.sumConflicts_below_during);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, vardata.sumDecisions_below_during);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, vardata.sumPropagations_below_during);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, vardata.sumAntecedents_below_during);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, vardata.sumAntecedentsLits_below_during);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, vardata.sumConflictClauseLits_below_during);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, vardata.sumDecisionBasedCl_below_during);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, vardata.sumClLBD_below_during);
    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, vardata.sumClSize_below_during);

    sqlite3_bind_int64 (stmt_var_data_picktime, bindAt++, solver->sumConflicts-vardata.last_flipped);

    run_sqlite_step(stmt_var_data_picktime, "var_data_picktime");
}

void SQLiteStats::var_dist(
    const uint32_t var
    , const VarData2& data
    , const Solver* solver
) {
    int bindAt = 1;
    sqlite3_bind_int(stmt_var_dist, bindAt++, var);
    sqlite3_bind_int64(stmt_var_dist, bindAt++, solver->latest_vardist_feature_calc);
    sqlite3_bind_int64(stmt_var_dist, bindAt++, solver->sumConflicts);

    sqlite3_bind_int64(stmt_var_dist, bindAt++, solver->longIrredCls.size());
    uint32_t num = 0;
    for(auto& x: solver->longRedCls) {
        num+=x.size();
    }
    sqlite3_bind_int64(stmt_var_dist, bindAt++, num);
    sqlite3_bind_int64(stmt_var_dist, bindAt++, solver->binTri.irredBins);
    sqlite3_bind_int64(stmt_var_dist, bindAt++, solver->binTri.redBins);


    sqlite3_bind_int64(stmt_var_dist, bindAt++, data.red.num_times_in_bin_clause);
    sqlite3_bind_int64(stmt_var_dist, bindAt++, data.red.num_times_in_long_clause);
    sqlite3_bind_int64(stmt_var_dist, bindAt++, data.red.satisfies_cl);
    sqlite3_bind_int64(stmt_var_dist, bindAt++, data.red.falsifies_cl);
    sqlite3_bind_int64(stmt_var_dist, bindAt++, data.red.tot_num_lit_of_bin_it_appears_in);
    sqlite3_bind_int64(stmt_var_dist, bindAt++, data.red.tot_num_lit_of_long_cls_it_appears_in);
    sqlite3_bind_double(stmt_var_dist, bindAt++, data.red.sum_var_act_of_cls);

    sqlite3_bind_int64(stmt_var_dist, bindAt++, data.irred.num_times_in_bin_clause);
    sqlite3_bind_int64(stmt_var_dist, bindAt++, data.irred.num_times_in_long_clause);
    sqlite3_bind_int64(stmt_var_dist, bindAt++, data.irred.satisfies_cl);
    sqlite3_bind_int64(stmt_var_dist, bindAt++, data.irred.falsifies_cl);
    sqlite3_bind_int64(stmt_var_dist, bindAt++, data.irred.tot_num_lit_of_bin_it_appears_in);
    sqlite3_bind_int64(stmt_var_dist, bindAt++, data.irred.tot_num_lit_of_long_cls_it_appears_in);
    sqlite3_bind_double(stmt_var_dist, bindAt++, data.irred.sum_var_act_of_cls);

    sqlite3_bind_double(stmt_var_dist, bindAt++, data.tot_act_long_red_cls);

    run_sqlite_step(stmt_var_dist, "var_dist");
}

void SQLiteStats::dec_var_clid(
    const uint32_t var
    , const uint64_t sumConflicts_at_picktime
    , const uint64_t clid
) {
    assert(clid != 0);

    int bindAt = 1;
    sqlite3_bind_int(stmt_dec_var_clid, bindAt++, var);
    sqlite3_bind_int64(stmt_dec_var_clid, bindAt++, sumConflicts_at_picktime);
    sqlite3_bind_int64(stmt_dec_var_clid, bindAt++, clid);

    run_sqlite_step(stmt_dec_var_clid, "dec_var_clid");
}
#endif

void SQLiteStats::cl_last_in_solver(
    const Solver* solver
    , const uint64_t clid)
{
    assert(clid != 0);

    int bindAt = 1;
    sqlite3_bind_int64(stmt_delete_cl, bindAt++, solver->sumConflicts);
    sqlite3_bind_int64(stmt_delete_cl, bindAt++, clid);

    run_sqlite_step(stmt_delete_cl, "cl_last_in_solver");
}

#endif
