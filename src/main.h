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

class Main
{
    public:
        Main(int argc, char** argv);

        void parseCommandLine();
        int solve();

    private:
        string typeclean;
        string var_elim_strategy;
        string drupfilname;
        int drupExistsCheck = 1;
        void add_supported_options();
        void check_options_correctness();
        void manually_parse_some_options();
        void parse_var_elim_strategy();
        void handle_drup_option();
        void parse_restart_type();
        void parse_polarity_type();
        void dumpIfNeeded() const;
        void check_num_threads_sanity(const unsigned thread_num) const;

        po::positional_options_description p;
        po::variables_map vm;
        po::options_description cmdline_options;

        SATSolver* solver = NULL;

        //File reading
        void readInAFile(const string& filename);
        void readInStandardInput();
        void parseInAllFiles();

        //Helper functions
        void printResultFunc(
            std::ostream* os
            , const bool toFile
            , const lbool ret
            , const bool firstSolut
        );
        void printVersionInfo();
        int correctReturnValue(const lbool ret) const;

        //Config
        SolverConf conf;
        bool needResultFile = false;
        bool zero_exit_status = false;
        bool satcomp = false;
        std::string resultFilename;

        bool debugLib = false;
        int printResult = true;
        string commandLine;
        unsigned num_threads = 1;

        //Multi-start solving
        uint32_t max_nr_of_solutions = 1;

        //Files to read & write
        bool fileNamePresent;
        vector<string> filesToRead;

        //Command line arguments
        int argc;
        char** argv;

        //Drup checker
        std::ostream* drupf = NULL;
        bool drupDebug = false;
};

#endif //MAIN_H
