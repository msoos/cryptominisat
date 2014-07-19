#include "sqlstats.h"
#include <sqlite3.h>

using namespace CMSat;


class SQLiteStats: public SQLStats
{
public:
    ~SQLiteStats() override;

    void restart(
        const PropStats& thisPropStats
        , const Searcher::Stats& thisStats
        , const VariableVariance& varVarStats
        , const Solver* solver
        , const Searcher* searcher
    ) override;

    void reduceDB(
        const ClauseUsageStats& irredStats
        , const ClauseUsageStats& redStats
        , const CleaningStats& clean
        , const Solver* solver
    ) override;

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

    bool setup(const Solver* solver) override;
    void finishup(lbool status) override;

private:

    bool connectServer();
    void getID(const Solver* solver);
    bool tryIDInSQL(const Solver* solver);
    void add_tags(const Solver* solver);

    void addStartupData(const Solver* solver);
    void initRestartSTMT(uint64_t verbosity);
    void initTimePassedSTMT();

    void writeQuestionMarks(size_t num, std::stringstream& ss);
    void initReduceDBSTMT(uint64_t verbosity);

    sqlite3_stmt *stmtTimePassed = NULL;
    sqlite3_stmt *stmtReduceDB = NULL;
    sqlite3_stmt *stmtRst = NULL;

    sqlite3 *db = NULL;
    bool setup_ok = false;
};
