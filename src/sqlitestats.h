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

    void mem_used(
        const Solver* solver
        , const string& name
        , double given_time
        , uint64_t mem_used_mb
    ) override;

    bool setup(const Solver* solver) override;
    void finishup(lbool status) override;

private:

    bool connectServer(const std::string& sqlite_filename
        , const int verbosity
    );
    void getID(const Solver* solver);
    bool tryIDInSQL(const Solver* solver);
    void add_tags(const Solver* solver);

    void addStartupData(const Solver* solver);
    void initRestartSTMT();
    void initTimePassedSTMT();
    void initMemUsedSTMT();

    void writeQuestionMarks(size_t num, std::stringstream& ss);
    void initReduceDBSTMT();

    sqlite3_stmt *stmtTimePassed = NULL;
    sqlite3_stmt *stmtMemUsed = NULL;
    sqlite3_stmt *stmtReduceDB = NULL;
    sqlite3_stmt *stmtRst = NULL;

    sqlite3 *db = NULL;
    bool setup_ok = false;
};
