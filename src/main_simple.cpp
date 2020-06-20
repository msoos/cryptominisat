/*************************************************************
MiniSat       --- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
CryptoMiniSat --- Copyright (c) 2014, Mate Soos

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
***************************************************************/

#include <ctime>
#include <cstring>
#include <errno.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>

using std::cout;
using std::endl;

#include <signal.h>
#include "time_mem.h"
#include "main_common.h"

#include "solverconf.h"
#include "cryptominisat5/cryptominisat.h"
#include "dimacsparser.h"

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4706) // Assignment within conditional expression
                              // -- used in parsing args
#endif

using namespace CMSat;

SATSolver* solver;

static void SIGINT_handler(int) {
    cout << "\n*** INTERRUPTED ***\n";
    solver->add_in_partial_solving_stats();
    solver->print_stats();
    cout << "\n*** INTERRUPTED ***\n";
    exit(1);
}

class Main: public MainCommon {
public:

    void printVersionInfo()
    {
        cout << solver->get_text_version_info() << endl;
    }

    void printUsage(const char** argv)
    {
        cout << "Usage: "
        << argv[0] << " [options] <input-file> where input is plain DIMACS.\n";
        cout << "Options:\n";
        cout << "  --verb          = [0...]  Sets verbosity level. Anything higher\n";
        cout << "                            than 2 will give debug log\n";
        cout << "  --drat          = {fname} DRAT dumped to file\n";
        cout << "  --sls           = {walksat,yalsat} Which SLS solver to use\n";
        cout << "  --threads       = [1...]  Sets number of threads\n";
        cout << "\n";
    }

    const char* hasPrefix(const char* str, const char* prefix)
    {
        int len = strlen(prefix);
        if (strncmp(str, prefix, len) == 0)
            return str + len;
        else
            return NULL;
    }

    int main(int argc, const char** argv) {
        conf.verbosity = 1;

        int i, j;
        for (i = j = 0; i < argc; i++){
            const char* value;
            if ((value = hasPrefix(argv[i], "--drat="))){
                dratfilname = std::string(value);
                handle_drat_option();
            }else if ((value = hasPrefix(argv[i], "--verb="))){
                long int verbosity = (int)strtol(value, NULL, 10);
                if (verbosity == 0 && errno == EINVAL){
                    cout << "ERROR! illegal verbosity level" << value << endl;
                    exit(0);
                }
                conf.verbosity = verbosity;
            }else if ((value = hasPrefix(argv[i], "--simdrat="))){
                int drat_sim  = (int)strtol(value, NULL, 10);
                conf.simulate_drat = drat_sim;
            }else if ((value = hasPrefix(argv[i], "--threads="))){
                num_threads  = (int)strtol(value, NULL, 10);
                if (num_threads == 0 && errno == EINVAL){
                    cout << "ERROR! illegal threads " << value << endl;
                    exit(0);
                }
                if (num_threads > 16) {
                    conf.var_and_mem_out_mult *= 0.4;
                }
            }else if ((value = hasPrefix(argv[i], "--bva="))){
                conf.do_bva = (int)strtol(value, NULL, 10);
            }else if ((value = hasPrefix(argv[i], "--sls="))){
                std::string sls;
                sls = argv[i];
                sls = sls.substr(6, 100);
                conf.which_sls = sls;
                cout << "c using SLS: '" << sls << "'" << endl;
            }else if ((value = hasPrefix(argv[i], "--reconf="))){
                long int reconf  = (int)strtol(value, NULL, 10);
                if (reconf == 0 && errno == EINVAL){
                    cout << "ERROR! illegal threads " << value << endl;
                    exit(0);
                }
                conf.reconfigure_val = reconf;
            }else if (strcmp(argv[i], "--zero-exit-status") == 0){
                zero_exit_status = true;
            }else if (strcmp(argv[i], "--version") == 0){
                printVersionInfo();
                exit(0);
            } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "--help") == 0){
                printUsage(argv);
                exit(0);

            }else if (strncmp(argv[i], "-", 1) == 0){
                cout << "ERROR! unknown flag: " << argv[i] << endl;
                exit(0);

            }else
                argv[j++] = argv[i];
        }
        argc = j;

        SATSolver S(&conf);
        solver = &S;
        if (dratf) {
            solver->set_drat(dratf, false);
            if (num_threads > 1) {
                cout << "ERROR: Cannot have DRAT and multiple threads." << endl;
                exit(-1);
            }
        }
        solver->set_num_threads(num_threads);

        if (conf.verbosity) {
            printVersionInfo();
            cout << "c NOTE: this is a SIMPLIFIED executable. For the full experience, you need to compile/obtain/use the 'cryptominisat5' executable. To compile that, you need the boost libraries. Please read the README." << endl;
        }
        double cpu_time = cpuTime();

        solver = &S;
        signal(SIGINT,SIGINT_handler);
        #if !defined (_WIN32)
        signal(SIGHUP,SIGINT_handler);
        #endif

        if (argc == 1) {
            cout << "Reading from standard input... Use '-h' or '--help' for help.\n";
            #ifndef USE_ZLIB
            FILE* in = stdin;
            DimacsParser<StreamBuffer<FILE*, FN>, SATSolver> parser(solver, NULL, conf.verbosity);
            #else
            gzFile in = gzdopen(0, "rb"); //opens stdin, which is 0
            DimacsParser<StreamBuffer<gzFile, GZ>, SATSolver> parser(solver, NULL, conf.verbosity);
            #endif

            if (!parser.parse_DIMACS(in, false)) {
                exit(-1);
            }

            #ifndef USE_ZLIB
            fclose(in);
            #else
            gzclose(in);
            #endif
        } else {
            #ifndef USE_ZLIB
            FILE* in = fopen(argv[1], "rb");
            #else
            gzFile in = gzopen(argv[1], "rb");
            #endif

            if (in == NULL) {
                std::cout << "ERROR! Could not open file: ";
                std::cout << argv[1] << " reason: " << strerror(errno);
                std::cout << std::endl;
                std::exit(1);
            }

            #ifndef USE_ZLIB
            DimacsParser<StreamBuffer<FILE*, FN>, SATSolver> parser(solver, NULL, conf.verbosity);
            #else
            DimacsParser<StreamBuffer<gzFile, GZ>, SATSolver> parser(solver, NULL, conf.verbosity);
            #endif

            if (!parser.parse_DIMACS(in, false)) {
                exit(-1);
            }

            #ifndef USE_ZLIB
            fclose(in);
            #else
            gzclose(in);
            #endif
        }

        double parse_time = cpuTime() - cpu_time;
        if (conf.verbosity) {
            cout << "c  Parsing time: "
            << std::fixed << std::setprecision(2) << parse_time << " s" << endl;
        }

        lbool ret = S.solve();
        if (conf.verbosity) {
            S.print_stats();
        }

        if (ret == l_True) {
            cout << "s SATISFIABLE" << endl;
        } else if (ret == l_False) {
            cout << "s UNSATISFIABLE"<< endl;
        }

        if (ret == l_True) {
            print_model(solver, &std::cout);
        }

        if (dratf) {
            *dratf << std::flush;
            if (dratf != &std::cout) {
                delete dratf;
            }
        }

        if (zero_exit_status)
            return 0;
        else {
            return ret == l_True ? 10 : 20;
        }
    }
};

int main(int argc, const char** argv)
{
    Main m;
    return m.main(argc, argv);
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
