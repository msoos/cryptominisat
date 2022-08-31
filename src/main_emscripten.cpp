/*************************************************************
MiniSat       --- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
CryptoMiniSat --- Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

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

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include "constants.h"

using std::cout;
using std::endl;

#include "main_common.h"

#include "solverconf.h"
#include "cryptominisat.h"
#include "dimacsparser.h"
#include "streambuffer.h"

using namespace CMSat;

SATSolver* solver = NULL;
DLL_PUBLIC void printVersionInfo()
{
    cout << "c CryptoMiniSat version " << solver->get_version() << endl;
    cout << "c CryptoMiniSat SHA revision " << solver->get_version_sha1() << endl;
}

DLL_PUBLIC int start_solve(const char* input)
{
    SolverConf conf;
    conf.max_confl = 500;
    conf.verbosity = 1;
    conf.do_print_times = 0;
    conf.simplify_at_startup = 0;

    delete solver;
    solver = new SATSolver(&conf);

    if (conf.verbosity) {
        printVersionInfo();
    }

    DimacsParser<StreamBuffer<const char*, CH>, SATSolver> parser(solver, NULL, conf.verbosity);
    if (!parser.parse_DIMACS(input, false)) {
        exit(-1);
    }

    solver->set_max_confl(500);
    lbool ret = solver->simplify();

    if (ret == l_True) {
        cout << "c "<< endl << "c "<< endl;
        cout << "s SATISFIABLE" << endl;
        cout << "c conflicts: " << solver->get_sum_conflicts() << endl;
    } else if (ret == l_False) {
        cout << "c "<< endl << "c "<< endl;
        cout << "s UNSATISFIABLE"<< endl;
        cout << "c conflicts: " << solver->get_sum_conflicts() << endl;
    }

    if (ret == l_True) {
        return 0;
    } else if (ret == l_False) {
        return 1;
    } else {
        return 2;
    }
}

DLL_PUBLIC int continue_solve()
{
    solver->set_max_confl(500);
    lbool ret = solver->solve();

    if (ret == l_True) {
        cout << "c "<< endl << "c "<< endl;
        cout << "s SATISFIABLE" << endl;
        cout << "c conflicts: " << solver->get_sum_conflicts() << endl;
    } else if (ret == l_False) {
        cout << "c "<< endl << "c "<< endl;
        cout << "s UNSATISFIABLE"<< endl;
        cout << "c conflicts: " << solver->get_sum_conflicts() << endl;
    }

    if (ret == l_True) {
        MainCommon::print_model(solver, &std::cout);
    }

    if (ret == l_True) {
        return 0;
    } else if (ret == l_False) {
        return 1;
    } else {
        return 2;
    }
}

DLL_PUBLIC int get_num_conflicts()
{
    uint64_t num = solver->get_sum_conflicts();
    return num;
}


extern "C" {

DLL_PUBLIC int cstart_solve(const char *input) {
  return start_solve(input);
}

DLL_PUBLIC int ccontinue_solve() {
  return continue_solve();
}

DLL_PUBLIC int cget_num_conflicts() {
  return get_num_conflicts();
}

}
