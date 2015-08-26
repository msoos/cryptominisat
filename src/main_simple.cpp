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
#include <errno.h>

using std::cout;
using std::endl;

#include <signal.h>
#include "time_mem.h"
#include "main_common.h"

#include "solverconf.h"
#include "cryptominisat4/cryptominisat.h"
#include "dimacsparser.h"
using namespace CMSat;
std::ostream* drupf;

SATSolver* solver;
bool zero_exit_status = false;
static void SIGINT_handler(int) {
    printf("\n"); printf("*** INTERRUPTED ***\n");
    solver->print_stats();
    printf("\n"); printf("*** INTERRUPTED ***\n");
    exit(1);
}

void printVersionInfo()
{
    cout << "c CryptoMiniSat version " << solver->get_version() << endl;
    #ifdef __GNUC__
    cout << "c compiled with gcc version " << __VERSION__ << endl;
    #else
    cout << "c compiled with non-gcc compiler" << endl;
    #endif
}

void drup_stuff(SolverConf& conf)
{
    drupf = &std::cout;

    if (!conf.otfHyperbin) {
        if (conf.verbosity >= 2) {
            cout
            << "c OTF hyper-bin is needed for BProp in DRUP, turning it back"
            << endl;
        }
        conf.otfHyperbin = true;
    }

    if (conf.doFindXors) {
        if (conf.verbosity >= 2) {
            cout
            << "c XOR manipulation is not supported in DRUP, turning it off"
            << endl;
        }
        conf.doFindXors = false;
    }

    if (conf.doRenumberVars) {
        if (conf.verbosity >= 2) {
            cout
            << "c Variable renumbering is not supported during DRUP, turning it off"
            << endl;
        }
        conf.doRenumberVars = false;
    }

    if (conf.doCompHandler) {
        if (conf.verbosity >= 2) {
            cout
            << "c Component finding & solving is not supported during DRUP, turning it off"
            << endl;
        }
        conf.doCompHandler = false;
    }
}

void printUsage(char** argv)
{
    printf("USAGE: %s [options] <input-file> \n\n  where input is plain DIMACS.\n\n", argv[0]);
    printf("OPTIONS:\n\n");
    printf("  --verb          = [0...] Sets verbosity level. Anything higher\n");
    printf("                           than 2 will give debug log\n");
    printf("  --drup          = {0,1}  Sets whether DRUP should be dumped to\n");
    printf("                           the console as per SAT COMPETITION'14 guidelines\n");
    printf("  --threads       = [1...] Sets number of threads\n");
    printf("\n");
}

const char* hasPrefix(const char* str, const char* prefix)
{
    int len = strlen(prefix);
    if (strncmp(str, prefix, len) == 0)
        return str + len;
    else
        return NULL;
}

int main(int argc, char** argv)
{
    SolverConf conf;
    conf.verbosity = 2;
    drupf = NULL;

    int i, j;
    int num_threads = 1;
    const char* value;
    for (i = j = 0; i < argc; i++){
        if ((value = hasPrefix(argv[i], "--drup="))){
            int drup = (int)strtol(value, NULL, 10);
            if (drup == 0 && errno == EINVAL){
                printf("ERROR! illegal drup level %s\n", value);
                exit(0);
            }
            if (drup > 0) {
                drup_stuff(conf);
            }
        }else if ((value = hasPrefix(argv[i], "--verb="))){
            int verbosity = (int)strtol(value, NULL, 10);
            if (verbosity == 0 && errno == EINVAL){
                printf("ERROR! illegal verbosity level %s\n", value);
                exit(0);
            }
            conf.verbosity = verbosity;
        }else if ((value = hasPrefix(argv[i], "--threads="))){
            num_threads  = (int)strtol(value, NULL, 10);
            if (num_threads == 0 && errno == EINVAL){
                printf("ERROR! illegal threads %s\n", value);
                exit(0);
            }
        }else if (strcmp(argv[i], "--zero-exit-status") == 0){
            zero_exit_status = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "--help") == 0){
            printUsage(argv);
            exit(0);

        }else if (strncmp(argv[i], "-", 1) == 0){
            printf("ERROR! unknown flag %s\n", argv[i]);
            exit(0);

        }else
            argv[j++] = argv[i];
    }
    argc = j;

    SATSolver S(&conf);
    solver = &S;
    if (drupf) {
        solver->set_drup(drupf);
        if (num_threads > 1) {
            cout << "ERROR: Cannot have DRUP and multiple threads." << endl;
            exit(-1);
        }
    }
    solver->set_num_threads(num_threads);

    if (conf.verbosity >= 1) {
        printVersionInfo();
    }
    double cpu_time = cpuTime();

    solver = &S;
    signal(SIGINT,SIGINT_handler);
    signal(SIGHUP,SIGINT_handler);

    if (argc == 1) {
        printf("Reading from standard input... Use '-h' or '--help' for help.\n");
        #ifndef USE_ZLIB
        FILE* in = stdin;
        #else
        gzFile in = gzdopen(fileno(stdin), "rb");
        #endif

        DimacsParser parser(solver, false, conf.verbosity);
        parser.parse_DIMACS(in);

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
            printf("ERROR! Could not open file: %s\n", argc == 1 ? "<stdin>" : argv[1]);
            std::exit(1);
        }
        DimacsParser parser(solver, false, conf.verbosity);
        parser.parse_DIMACS(in);

        #ifndef USE_ZLIB
        fclose(in);
        #else
        gzclose(in);
        #endif
    }

    double parse_time = cpuTime() - cpu_time;
    if (conf.verbosity >= 1) {
        printf("c  Parsing time: %-12.2f s\n", parse_time);
    }

    lbool ret = S.solve();
    if (conf.verbosity >= 1) {
        S.print_stats();
    }
    printf(ret == l_True ? "s SATISFIABLE\n" : "s UNSATISFIABLE\n");
    if (ret == l_True) {
        print_model(&std::cout, solver);
    }

    if (zero_exit_status)
        return 0;
    else {
        return ret == l_True ? 10 : 20;
    }
}
