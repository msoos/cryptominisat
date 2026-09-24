/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

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
#include <utility>

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

SQLiteStats::SQLiteStats(const std::string& _filename) :
        filename(_filename)
{
}

vector<string> SQLiteStats::get_columns(const char* tablename)
{
    vector<string> ret;

    std::stringstream q;
    q << "pragma table_info(" << tablename << ");";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, q.str().c_str(), -1, &stmt, nullptr)) {
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
    if (stmt == nullptr) {
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

    dump_id_confl_cache();

    //Free all the prepared statements
    del_prepared_stmt(stmtRst);
    del_prepared_stmt(stmtFeat);
    del_prepared_stmt(stmtReduceDB);
    del_prepared_stmt(stmtReduceDB_common);
    del_prepared_stmt(stmtTimePassed);
    del_prepared_stmt(stmtMemUsed);
    del_prepared_stmt(stmt_clause_stats);
    del_prepared_stmt(stmt_delete_cl);
    del_prepared_stmt(stmt_update_id);
    del_prepared_stmt(stmt_set_id_confl);
    del_prepared_stmt(stmt_set_id_confl_1000);

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
    if (sqlite3_exec(db, cmsat_tablestructure_sql, nullptr, nullptr, nullptr)) {
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
    init("reduceDB", &stmtReduceDB);
    init("reduceDB_common", &stmtReduceDB_common);
    init("set_id_confl", &stmt_set_id_confl_1000, 1000);
    init("set_id_confl", &stmt_set_id_confl);
    #ifdef STATS_NEEDED
    init("cl_last_in_solver", &stmt_delete_cl);
    init("update_id", &stmt_update_id);
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
        cout << solver->conf.prefix << "Cannot open sqlite database: " << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        return false;
    }

    if (sqlite3_exec(db, "PRAGMA synchronous = OFF", nullptr, nullptr, nullptr)) {
        cerr << "ERROR: Problem setting pragma synchronous = OFF to SQLite DB" << endl;
        cerr << "c " << sqlite3_errmsg(db) << endl;
        std::exit(-1);
    }

    if (sqlite3_exec(db, "PRAGMA journal_mode = MEMORY", nullptr, nullptr, nullptr)) {
        cerr << "ERROR: Problem setting pragma journal_mode = MEMORY to SQLite DB" << endl;
        cerr << "c " << sqlite3_errmsg(db) << endl;
        std::exit(-1);
    }


    if (solver->conf.verbosity) {
        cout << solver->conf.prefix << "writing to SQLite file: " << filename << endl;
    }

    return true;
}

void SQLiteStats::begin_transaction()
{
    if (sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr)) {
        cerr << "ERROR: Beginning SQLITE transaction" << endl;
        cerr << "c " << sqlite3_errmsg(db) << endl;
        std::exit(-1);
    }
}

void SQLiteStats::end_transaction()
{
    if (sqlite3_exec(db, "END TRANSACTION", nullptr, nullptr, nullptr)) {
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
    << time(nullptr)
    << ", '" << solver->get_version_sha1() << "'"
    << ");";

    //Inserting element into solverruns to get unique ID
    const int rc = sqlite3_exec(db, ss.str().c_str(), nullptr, nullptr, nullptr);
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
    if (sqlite3_exec(db, ss.str().c_str(), nullptr, nullptr, nullptr)) {
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

    if (sqlite3_exec(db, ss.str().c_str(), nullptr, nullptr, nullptr)) {
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

    if (sqlite3_exec(db, ss.str().c_str(), nullptr, nullptr, nullptr)) {
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

void SQLiteStats::run_sqlite_step(
    sqlite3_stmt* stmt,
    const char* name,
    const uint32_t bindAt)
{
    if (name != nullptr) {
        assert(query_to_size.find(name) != query_to_size.end());
        //SQLite numbers them from 1, so it's off-by-one
        assert(query_to_size[name]+1 == bindAt);
    }

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

void SQLiteStats::init(const char* name, sqlite3_stmt** stmt, uint32_t num)
{
    vector<string> cols = get_columns(name);
    query_to_size[string(name)] = cols.size();
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
    for(uint32_t i = 0; i < num; i++) {
        writeQuestionMarks(numElems, ss);
        if (i+1 < num) ss << ",";
    }
    ss << ";";

    //Prepare the statement
    if (sqlite3_prepare(db, ss.str().c_str(), -1, stmt, nullptr)) {
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
    sqlite3_bind_int64(stmtMemUsed, bindAt++, solver->sum_conflicts);
    sqlite3_bind_double(stmtMemUsed, bindAt++, given_time);
    //memory stats
    sqlite3_bind_text(stmtMemUsed, bindAt++, name.c_str(), -1, nullptr);
    sqlite3_bind_int(stmtMemUsed, bindAt++, mem_used_mb);

    run_sqlite_step(stmtMemUsed, "memused", bindAt);
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
    sqlite3_bind_int64(stmtTimePassed, bindAt++, solver->sum_conflicts);
    sqlite3_bind_double(stmtTimePassed, bindAt++, cpu_time());
    sqlite3_bind_text(stmtTimePassed, bindAt++, name.c_str(), -1, nullptr);
    sqlite3_bind_double(stmtTimePassed, bindAt++, time_passed);
    sqlite3_bind_int(stmtTimePassed, bindAt++, time_out);
    sqlite3_bind_double(stmtTimePassed, bindAt++, percent_time_remain);

    run_sqlite_step(stmtTimePassed, "timepassed", bindAt);
}

void SQLiteStats::time_passed_min(
    const Solver* solver
    , const string& name
    , double time_passed
) {
    int bindAt = 1;
    sqlite3_bind_int64(stmtTimePassed, bindAt++, solver->get_solve_stats().num_simplify);
    sqlite3_bind_int64(stmtTimePassed, bindAt++, solver->sum_conflicts);
    sqlite3_bind_double(stmtTimePassed, bindAt++, cpu_time());
    sqlite3_bind_text(stmtTimePassed, bindAt++, name.c_str(), -1, nullptr);
    sqlite3_bind_double(stmtTimePassed, bindAt++, time_passed);
    sqlite3_bind_null(stmtTimePassed, bindAt++);
    sqlite3_bind_null(stmtTimePassed, bindAt++);

    run_sqlite_step(stmtTimePassed, "timepassed", bindAt);
}

void SQLiteStats::dump_id_confl_cache()
{
    if (id_conf_cache.size() == 1000) {
        int bindAt = 1;
        for(auto const& elem: id_conf_cache) {
            sqlite3_bind_int64(stmt_set_id_confl_1000, bindAt++, elem.first);
            sqlite3_bind_int64(stmt_set_id_confl_1000, bindAt++, elem.second);
        }
        run_sqlite_step(stmt_set_id_confl_1000, nullptr, 0);
    } else {
        for(auto const& elem: id_conf_cache) {
            int bindAt = 1;
            sqlite3_bind_int64(stmt_set_id_confl, bindAt++, elem.first);
            sqlite3_bind_int64(stmt_set_id_confl, bindAt++, elem.second);
            run_sqlite_step(stmt_set_id_confl, "set_id_confl", bindAt);
        }
    }
    id_conf_cache.clear();
}

void SQLiteStats::set_id_confl(
        const int32_t id
        , const uint64_t sum_conflicts)
{
    assert(id != 0);
    id_conf_cache.emplace_back(id, sum_conflicts);
    if (id_conf_cache.size() < 1000) return;
    else dump_id_confl_cache();
}

#ifdef STATS_NEEDED
void SQLiteStats::satzilla_features(
    const Solver* solver
    , const Searcher* search
    , const SatZillaFeatures& satzilla_feat
) {
    int bindAt = 1;
    sqlite3_bind_int64(stmtFeat, bindAt++, solver->get_solve_stats().num_simplify);
    sqlite3_bind_int64(stmtFeat, bindAt++, search->sumRestarts());
    sqlite3_bind_int64(stmtFeat, bindAt++, solver->sum_conflicts);
    sqlite3_bind_int(stmtFeat, bindAt++, solver->latest_satzilla_feature_calc);

    sqlite3_bind_int64(stmtFeat, bindAt++, (uint64_t)satzilla_feat.numVars);
    sqlite3_bind_int64(stmtFeat, bindAt++, (uint64_t)satzilla_feat.numClauses);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.var_cl_ratio);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.binary);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.horn);

    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.avg_confl_size);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.avg_confl_glue);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.avg_num_resolutions);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.learnt_bins_per_confl);

    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.avg_branch_depth);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.avg_trail_depth_delta);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.avg_branch_depth_delta);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.props_per_confl);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.confl_per_restart);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.decisions_per_conflict);

    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.red_glue_distr_mean);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.red_glue_distr_var);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.red_size_distr_mean);
    sqlite3_bind_double(stmtFeat, bindAt++, satzilla_feat.red_size_distr_var);

    run_sqlite_step(stmtFeat, "satzilla_features", bindAt);
}

void SQLiteStats::restart(
    const uint32_t restartID
    , const uint32_t rest_stable
    , const PropStats& thisPropStats
    , const SearchStats& thisStats
    , const Solver* solver
    , const Searcher* search
) {
    sqlite3_stmt* stmt = stmtRst;

    const SearchHist& search_hist = search->getHistory();
    const BinTriStats& bin_tri = solver->getBinTriStats();

    int bindAt = 1;
    sqlite3_bind_int64(stmt, bindAt++, restartID);
    sqlite3_bind_int64(stmt, bindAt++, solver->get_solve_stats().num_simplify);
    sqlite3_bind_int64(stmt, bindAt++, search->sumRestarts());
    sqlite3_bind_int64(stmt, bindAt++, solver->sum_conflicts);
    sqlite3_bind_int  (stmt, bindAt++, search_hist.num_conflicts_this_restart);
    sqlite3_bind_double(stmt, bindAt++, cpu_time());


    sqlite3_bind_int64(stmt, bindAt++, bin_tri.irred_bins);
    sqlite3_bind_int64(stmt, bindAt++, solver->get_num_long_irred_cls());
    sqlite3_bind_int64(stmt, bindAt++, bin_tri.red_bins);
    sqlite3_bind_int64(stmt, bindAt++, solver->get_num_long_red_cls());

    sqlite3_bind_int64(stmt, bindAt++, solver->lit_stats.irred_lits);
    sqlite3_bind_int64(stmt, bindAt++, solver->lit_stats.red_lits);

    //Conflict stats
    bind_null_or_double(stmt, bindAt,   search_hist.glueHist.getLongtTerm(),avg)
    sqlite3_bind_double(stmt, bindAt++, std:: sqrt(search_hist.glueHist.getLongtTerm().var()));
    bind_null_or_double(stmt, bindAt,   search_hist.glueHist.getLongtTerm(),getMin)
    bind_null_or_double(stmt, bindAt,   search_hist.glueHist.getLongtTerm(),getMax)

    bind_null_or_double(stmt, bindAt,   search_hist.conflSizeHist, avg)
    sqlite3_bind_double(stmt, bindAt++, std:: sqrt(search_hist.conflSizeHist.var()));
    bind_null_or_double(stmt, bindAt,   search_hist.conflSizeHist,getMin)
    bind_null_or_double(stmt, bindAt,   search_hist.conflSizeHist,getMax)

    bind_null_or_double(stmt, bindAt,   search_hist.numResolutionsHist, avg)
    sqlite3_bind_double(stmt, bindAt++, std:: sqrt(search_hist.numResolutionsHist.var()));
    bind_null_or_double(stmt, bindAt,   search_hist.numResolutionsHist,getMin)
    bind_null_or_double(stmt, bindAt,   search_hist.numResolutionsHist,getMax)

    //Search stats
    bind_null_or_double(stmt, bindAt,   search_hist.branchDepthHist,avg)
    sqlite3_bind_double(stmt, bindAt++, std:: sqrt(search_hist.branchDepthHist.var()));
    bind_null_or_double(stmt, bindAt, search_hist.branchDepthHist,getMin)
    bind_null_or_double(stmt, bindAt, search_hist.branchDepthHist,getMax)

    bind_null_or_double(stmt, bindAt,   search_hist.branchDepthDeltaHist,avg)
    sqlite3_bind_double(stmt, bindAt++, std:: sqrt(search_hist.branchDepthDeltaHist.var()));
    bind_null_or_double(stmt, bindAt,   search_hist.branchDepthDeltaHist,getMin)
    bind_null_or_double(stmt, bindAt,   search_hist.branchDepthDeltaHist,getMax)

    bind_null_or_double(stmt, bindAt, search_hist.trailDepthHist.getLongtTerm(),avg)
    sqlite3_bind_double(stmt, bindAt++, std:: sqrt(search_hist.trailDepthHist.getLongtTerm().var()));
    bind_null_or_double(stmt, bindAt,   search_hist.trailDepthHist.getLongtTerm(),getMin)
    bind_null_or_double(stmt, bindAt,   search_hist.trailDepthHist.getLongtTerm(),getMax)

    bind_null_or_double(stmt, bindAt,   search_hist.trailDepthDeltaHist,avg)
    sqlite3_bind_double(stmt, bindAt++, std:: sqrt(search_hist.trailDepthDeltaHist.var()));
    bind_null_or_double(stmt, bindAt,   search_hist.trailDepthDeltaHist,getMin)
    bind_null_or_double(stmt, bindAt,   search_hist.trailDepthDeltaHist,getMax)

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
    sqlite3_bind_int64(stmt, bindAt++, solver->var_replacer->get_num_replaced_vars());
    sqlite3_bind_int64(stmt, bindAt++, solver->get_num_vars_elimed());
    sqlite3_bind_int64(stmt, bindAt++, search->getTrailSize());

    //strategy
    sqlite3_bind_int(stmt, bindAt++, (int)solver->branch_strategy);
    sqlite3_bind_int(stmt, bindAt++, (int)rest_stable);

    run_sqlite_step(stmt, "restart", bindAt);
}


void SQLiteStats::reduceDB_common(
    const Solver* solver,
    const uint32_t reduceDB_called,
    const uint32_t tot_cls_in_db,
    const uint32_t cur_rst_type,
    const MedianCommonDataRDB& median_data,
    const AverageCommonDataRDB& avg_data)
{
    int bindAt = 1;

    sqlite3_bind_int(stmtReduceDB_common, bindAt++, reduceDB_called);

    sqlite3_bind_int64 (stmtReduceDB_common, bindAt++, solver->get_solve_stats().num_simplify);
    sqlite3_bind_int64 (stmtReduceDB_common, bindAt++, solver->sumRestarts());
    sqlite3_bind_int64 (stmtReduceDB_common, bindAt++, solver->sum_conflicts);
    sqlite3_bind_int   (stmtReduceDB_common, bindAt++, cur_rst_type);
    sqlite3_bind_int   (stmtReduceDB_common, bindAt++, tot_cls_in_db);

    sqlite3_bind_double(stmtReduceDB_common, bindAt++, (double)median_data.median_act);
    sqlite3_bind_int   (stmtReduceDB_common, bindAt++, median_data.median_uip1_used);
    sqlite3_bind_int   (stmtReduceDB_common, bindAt++, median_data.median_props);
    sqlite3_bind_double(stmtReduceDB_common, bindAt++, median_data.median_sum_uip1_per_time);
    sqlite3_bind_double(stmtReduceDB_common, bindAt++, median_data.median_sum_props_per_time);

    //sqlite3_bind_double(stmtReduceDB_common, bindAt++, avg_data.avg_glue);
    sqlite3_bind_double(stmtReduceDB_common, bindAt++, avg_data.avg_props);
    sqlite3_bind_double(stmtReduceDB_common, bindAt++, avg_data.avg_uip1_used);
    sqlite3_bind_double(stmtReduceDB_common, bindAt++, avg_data.avg_sum_uip1_per_time);
    sqlite3_bind_double(stmtReduceDB_common, bindAt++, avg_data.avg_sum_props_per_time);


    sqlite3_bind_int   (stmtReduceDB_common, bindAt++, solver->nVars());
    sqlite3_bind_int   (stmtReduceDB_common, bindAt++, solver->long_irred_cls.size());
    sqlite3_bind_int   (stmtReduceDB_common, bindAt++, solver->lit_stats.irred_lits);
    uint32_t total_long_red_cls = 0;
    for(const auto& cls: solver->long_red_cls) {
        total_long_red_cls += cls.size();
    }
    sqlite3_bind_int(stmtReduceDB_common, bindAt++, total_long_red_cls);
    sqlite3_bind_int(stmtReduceDB_common, bindAt++, solver->lit_stats.red_lits);
    sqlite3_bind_int(stmtReduceDB_common, bindAt++, solver->bin_tri.irred_bins);
    sqlite3_bind_int(stmtReduceDB_common, bindAt++, solver->bin_tri.red_bins);

    sqlite3_bind_double(stmtReduceDB_common, bindAt++, solver->hist.trailDepthHistLT.avg());
    sqlite3_bind_double(stmtReduceDB_common, bindAt++, solver->hist.backtrackLevelHistLT.avg());
    sqlite3_bind_double(stmtReduceDB_common, bindAt++, solver->hist.conflSizeHistLT.avg());
    sqlite3_bind_double(stmtReduceDB_common, bindAt++, solver->hist.numResolutionsHistLT.avg());
    sqlite3_bind_double(stmtReduceDB_common, bindAt++, solver->hist.glueHistLT.avg());
    sqlite3_bind_double(stmtReduceDB_common, bindAt++, solver->hist.antec_data_sum_sizeHistLT.avg());
    sqlite3_bind_double(stmtReduceDB_common, bindAt++, solver->hist.overlapHistLT.avg());

    run_sqlite_step(stmtReduceDB_common, "reduceDB_common", bindAt);
}

void SQLiteStats::reduceDB(
    const Solver* solver
    , const bool locked
    , const Clause* cl
    , const uint32_t reduceDB_called
) {
    const ClauseStatsExtra& stats_extra = solver->red_stats_extra[cl->stats.extra_pos];
    assert(stats_extra.dump_no != numeric_limits<uint16_t>::max());

    int bindAt = 1;

    //Global data ("conflicts" is needed because otherwise
    //       code is complicated in data sampler), even though this data
    //       is available in reduceDB_common
    sqlite3_bind_int(stmtReduceDB, bindAt++, reduceDB_called);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, solver->sum_conflicts);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, stats_extra.introduced_at_conflict);

    //data
    sqlite3_bind_int64(stmtReduceDB, bindAt++, stats_extra.orig_ID);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, stats_extra.dump_no);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, stats_extra.conflicts_made);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, cl->stats.props_made);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, stats_extra.sum_props_made);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, cl->stats.uip1_used);
    sqlite3_bind_int64(stmtReduceDB, bindAt++, stats_extra.sum_uip1_used);

    assert(cl->stats.last_touched_any <= solver->sum_conflicts);
    int64_t last_touched_any_diff = solver->sum_conflicts - cl->stats.last_touched_any;
    sqlite3_bind_int64(stmtReduceDB, bindAt++, last_touched_any_diff);
    sqlite3_bind_double(stmtReduceDB, bindAt++, (double)cl->stats.activity/(double)solver->get_cla_inc());
    sqlite3_bind_int(stmtReduceDB, bindAt++, locked);
    //eager subsume sets glue to CL_MAX_GLUE as a 'delete next' marker
    if (cl->stats.is_ternary_resolvent || cl->stats.glue == CL_MAX_GLUE) {
        sqlite3_bind_null(stmtReduceDB, bindAt++);
    } else {
        sqlite3_bind_int(stmtReduceDB, bindAt++, cl->stats.glue);
    }
    sqlite3_bind_int(stmtReduceDB, bindAt++, cl->size());
    sqlite3_bind_int(stmtReduceDB, bindAt++, cl->stats.used);
    sqlite3_bind_int(stmtReduceDB, bindAt++, cl->stats.is_ternary_resolvent);
    sqlite3_bind_int(stmtReduceDB, bindAt++, cl->stats.is_decision);
    sqlite3_bind_int(stmtReduceDB, bindAt++, cl->distilled);

    //Ranking
    sqlite3_bind_int(stmtReduceDB, bindAt++, stats_extra.act_ranking);
    sqlite3_bind_int(stmtReduceDB, bindAt++, stats_extra.prop_ranking);
    sqlite3_bind_int(stmtReduceDB, bindAt++, stats_extra.uip1_ranking);
    sqlite3_bind_int(stmtReduceDB, bindAt++, stats_extra.sum_uip1_per_time_ranking);
    sqlite3_bind_int(stmtReduceDB, bindAt++, stats_extra.sum_props_per_time_ranking);

    //Discounted
    sqlite3_bind_double(stmtReduceDB, bindAt++, (double)stats_extra.discounted_uip1_used);
    sqlite3_bind_double(stmtReduceDB, bindAt++, (double)stats_extra.discounted_props_made);
    sqlite3_bind_double(stmtReduceDB, bindAt++, (double)stats_extra.discounted_uip1_used2);
    sqlite3_bind_double(stmtReduceDB, bindAt++, (double)stats_extra.discounted_props_made2);
    sqlite3_bind_double(stmtReduceDB, bindAt++, (double)stats_extra.discounted_uip1_used3);
    sqlite3_bind_double(stmtReduceDB, bindAt++, (double)stats_extra.discounted_props_made3);

    run_sqlite_step(stmtReduceDB, "reduceDB", bindAt);
}

void SQLiteStats::clause_stats(
    const Solver* solver
    , uint64_t clid
    , const uint64_t restartID
    , uint32_t glue
    , uint32_t glue_before_minim
    , uint32_t size
    , uint32_t size_before_minim
    , uint32_t backtrack_level
    , AtecedentData<uint16_t> antec_data
    , size_t decision_level
    , size_t trail_depth
    , const uint32_t restart_type
    , const SearchHist& hist
    , const bool is_decision
) {
    uint32_t num_overlap_literals = antec_data.sum_size()-(antec_data.num()-1)-size;

    int bindAt = 1;
    sqlite3_bind_int64(stmt_clause_stats, bindAt++, solver->get_solve_stats().num_simplify);
    sqlite3_bind_int64(stmt_clause_stats, bindAt++, solver->sumRestarts());
    sqlite3_bind_int64 (stmt_clause_stats, bindAt++, solver->sum_conflicts);
    sqlite3_bind_int64 (stmt_clause_stats, bindAt++, clid);
    sqlite3_bind_int   (stmt_clause_stats, bindAt++, restartID);

    sqlite3_bind_int   (stmt_clause_stats, bindAt++, glue);
    sqlite3_bind_int   (stmt_clause_stats, bindAt++, glue_before_minim);
    sqlite3_bind_int   (stmt_clause_stats, bindAt++, size);
    sqlite3_bind_int   (stmt_clause_stats, bindAt++, size_before_minim);
    sqlite3_bind_int   (stmt_clause_stats, bindAt++, num_overlap_literals);
    sqlite3_bind_int   (stmt_clause_stats, bindAt++, antec_data.num());
    sqlite3_bind_int   (stmt_clause_stats, bindAt++, antec_data.sum_size());
    sqlite3_bind_int   (stmt_clause_stats, bindAt++, is_decision);

    sqlite3_bind_int   (stmt_clause_stats, bindAt++, backtrack_level);
    sqlite3_bind_int64 (stmt_clause_stats, bindAt++, decision_level);
    sqlite3_bind_int64 (stmt_clause_stats, bindAt++, trail_depth);
    sqlite3_bind_int   (stmt_clause_stats, bindAt++, restart_type);

    sqlite3_bind_int   (stmt_clause_stats, bindAt++, antec_data.binIrred);
    sqlite3_bind_int   (stmt_clause_stats, bindAt++, antec_data.binRed);
    sqlite3_bind_int   (stmt_clause_stats, bindAt++, antec_data.longIrred);
    sqlite3_bind_int   (stmt_clause_stats, bindAt++, antec_data.longRed);

    bind_null_or_double(stmt_clause_stats, bindAt, hist.trailDepthHistLT,avg)
    bind_null_or_double(stmt_clause_stats, bindAt, hist.conflSizeHistLT,avg)
    bind_null_or_double(stmt_clause_stats, bindAt, hist.glueHistLT,avg)
    bind_null_or_double(stmt_clause_stats, bindAt, hist.numResolutionsHistLT,avg)

    bind_null_or_double(stmt_clause_stats, bindAt, hist.antec_data_sum_sizeHistLT,avg)
    bind_null_or_double(stmt_clause_stats, bindAt, hist.overlapHistLT,avg)

    bind_null_or_double(stmt_clause_stats, bindAt, hist.branchDepthHistQueue,avg_nocheck)
    bind_null_or_double(stmt_clause_stats, bindAt, hist.trailDepthHist,avg_nocheck)
    bind_null_or_double(stmt_clause_stats, bindAt, hist.conflSizeHist,avg)
    bind_null_or_double(stmt_clause_stats, bindAt, hist.glueHist,avg_nocheck)
    bind_null_or_double(stmt_clause_stats, bindAt, hist.glueHist.getLongtTerm(),avg)

    run_sqlite_step(stmt_clause_stats, "clause_stats", bindAt);
}


void SQLiteStats::cl_last_in_solver(
    const Solver* solver
    , const uint64_t clid)
{
    assert(clid != 0);

    int bindAt = 1;
    sqlite3_bind_int64(stmt_delete_cl, bindAt++, solver->sum_conflicts);
    sqlite3_bind_int64(stmt_delete_cl, bindAt++, clid);

    run_sqlite_step(stmt_delete_cl, "cl_last_in_solver", bindAt);
}

void SQLiteStats::update_id(
    const uint32_t old_id,
    const uint32_t new_id)
{
    assert(old_id != 0);
    assert(new_id != 0);
    assert((new_id == old_id || new_id > old_id) && "not neccessary, but I think we have this always");

    int bindAt = 1;
    sqlite3_bind_int64(stmt_update_id, bindAt++, old_id);
    sqlite3_bind_int64(stmt_update_id, bindAt++, new_id);

    run_sqlite_step(stmt_update_id, "update_id", bindAt);
}


#endif
