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

#include "sqlstats.h"
#include "satzilla_features.h"
#include <sqlite3.h>

namespace CMSat {

class SQLiteStats: public SQLStats
{
public:
    virtual ~SQLiteStats() override;
    explicit SQLiteStats(std::string _filename);

    void end_transaction() override;
    void begin_transaction() override;

    void time_passed(
        const Solver* solver
        , const string& name
        , double time_passed
        , bool time_out
        , double percent_time_remain
    ) override;

    void time_passed_min(
        const Solver* solver
        , const string& name
        , double time_passed
    ) override;

    void mem_used(
        const Solver* solver
        , const string& name
        , double given_time
        , uint64_t mem_used_mb
    ) override;

    void satzilla_features(
        const Solver* solver
        , const Searcher* search
        , const SatZillaFeatures& satzilla_feat
    ) override;

    #ifdef STATS_NEEDED
    void restart(
        const uint32_t restartID
        , const Restart rest_type
        , const PropStats& thisPropStats
        , const SearchStats& thisStats
        , const Solver* solver
        , const Searcher* searcher
        , const rst_dat_type type
        , const int64_t clauseID
    ) override;

    void reduceDB(
        const Solver* solver
        , const bool locked
        , const Clause* cl
        , const string& cur_restart_type
        , const uint32_t act_ranking_top_10
        , const uint32_t act_ranking
        , const uint32_t tot_cls_in_db
    ) override;

    virtual void cl_last_in_solver(
        const Solver* solver
        , const uint64_t clid
    ) override;

    void dump_clause_stats(
        const Solver* solver
        , uint64_t clid
        , const uint64_t restartID
        , uint32_t orig_glue
        , uint32_t glue_before_minim
        , const uint32_t backtrack_level
        , uint32_t size
        , AtecedentData<uint16_t> resoltypes
        , size_t decision_level
        , size_t trail_depth
        , uint64_t conflicts_this_restart
        , const std::string& rest_type
        , const SearchHist& hist
        , const bool is_decision
    ) override;

    #ifdef STATS_NEEDED_BRANCH
    void var_data_picktime(
        const Solver* solver
        , const uint32_t var
        , const VarData& vardata
        , const double rel_activity
    ) override;

    void var_data_fintime(
        const Solver* solver
        , const uint32_t var
        , const VarData& vardata
        , const double rel_activity
    ) override;

    void dec_var_clid(
        const uint32_t var
        , const uint64_t sumConflicts_at_picktime
        , const uint64_t clid
    ) override;

    void var_dist(
        const uint32_t var
        , const VarData2& data
        , const Solver* solver
    ) override;
    #endif
    #endif

    bool setup(const Solver* solver) override;
    void finishup(lbool status) override;
    void add_tag(const std::pair<std::string, std::string>& tag) override;

private:

    bool connectServer(const Solver* solver);
    bool add_solverrun(const Solver* solver);
    void init(const char* name, sqlite3_stmt** stmt);
    vector<string> get_columns(const char* tablename);

    void addStartupData();
    void del_prepared_stmt(sqlite3_stmt* stmt);
    void initRestartSTMT(const char* tablename, sqlite3_stmt** stmt);
    void initTimePassedSTMT();
    void init_cl_last_in_solver_STMT();
    void initMemUsedSTMT();
    void init_clause_stats_STMT();
    void init_var_data_picktime_STMT();
    void init_var_data_fintime_STMT();
    void init_dec_var_clid_STMT();
    void run_sqlite_step(sqlite3_stmt* stmt, const char* name);

    void writeQuestionMarks(size_t num, std::stringstream& ss);
    void initReduceDBSTMT();

    sqlite3_stmt *stmtTimePassed = NULL;
    sqlite3_stmt *stmtMemUsed = NULL;
    sqlite3_stmt *stmtReduceDB = NULL;
    sqlite3_stmt *stmtRst = NULL;
    sqlite3_stmt *stmtVarRst = NULL;
    sqlite3_stmt *stmtClRst = NULL;
    sqlite3_stmt *stmtFeat = NULL;
    sqlite3_stmt *stmt_clause_stats = NULL;
    sqlite3_stmt *stmt_delete_cl = NULL;
    sqlite3_stmt *stmt_var_data_fintime = NULL;
    sqlite3_stmt *stmt_var_data_picktime = NULL;
    sqlite3_stmt *stmt_dec_var_clid = NULL;
    sqlite3_stmt *stmt_var_dist = NULL;

    sqlite3 *db = NULL;
    bool setup_ok = false;
    const string filename;
};

}
