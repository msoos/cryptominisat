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

#ifndef MAIN_H
#define MAIN_H

#include <string>
#include <vector>
#include <memory>
#include <fstream>

#include "solverconf.h"
#include "cryptominisat4/cryptominisat.h"

using std::string;
using std::vector;

#include <boost/program_options.hpp>
namespace po = boost::program_options;
using namespace CMSat;

struct SATCount {
    uint32_t hashCount = 0;
    uint32_t cellSolCount = 0;
};

class Main
{
    public:
        Main(int argc, char** argv);
        ~Main()
        {
            if (dratf) {
                *dratf << std::flush;
                if (dratf != &std::cout) {
                    delete dratf;
                }
            }

            delete solver;
        }

        void parseCommandLine();
        virtual int solve();

    private:
        //arguments
        int argc;
        char** argv;
        string var_elim_strategy;
        string dratfilname;
        void add_supported_options();
        void check_options_correctness();
        void manually_parse_some_options();
        void parse_var_elim_strategy();
        void handle_drat_option();
        void parse_restart_type();
        void parse_polarity_type();
        void dumpIfNeeded() const;
        void check_num_threads_sanity(const unsigned thread_num) const;

        po::positional_options_description p;
        po::variables_map vm;
        po::options_description all_options;
        po::options_description help_options_simple;
        po::options_description help_options_complicated;

    protected:
        SATSolver* solver = NULL;
        SolverConf conf;

        //File reading
        void readInAFile(SATSolver* solver2, const string& filename);
        void readInStandardInput(SATSolver* solver2);
        void parseInAllFiles(SATSolver* solver2);

        //Helper functions
        void printResultFunc(
            std::ostream* os
            , const bool toFile
            , const lbool ret
        );
        void printVersionInfo();
        int correctReturnValue(const lbool ret) const;
        lbool multi_solutions();
        std::ofstream* resultfile = NULL;

        //Config
        bool zero_exit_status = false;
        std::string resultFilename;
        std::string debugLib;
        int printResult = true;
        string commandLine;
        unsigned num_threads = 1;
        uint32_t max_nr_of_solutions = 1;
        int sql = 0;
        string sqlite_filename;
        string sqlServer;
        string sqlUser;
        string sqlPass;
        string sqlDatabase;

        //Files to read & write
        bool fileNamePresent;
        vector<string> filesToRead;

        //Drat checker
        std::ostream* dratf = NULL;
        bool dratDebug = false;
        bool clause_ID_needed = false;

        vector<uint32_t> independent_vars;
};

#endif //MAIN_H
