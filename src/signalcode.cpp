/*
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
*/

#include "signalcode.h"
#include "time_mem.h"
#include "cryptominisat.h"
#if !defined (_MSC_VER)
#include <unistd.h>
#endif

using namespace CMSat;

SATSolver* solverToInterrupt;
int need_clean_exit;
double wallclock_time_started = 0.0;
bool g_python_lib = false;
std::string redDumpFname;
std::string irredDumpFname;

using std::cout;
using std::endl;

void SIGINT_handler(int)
{
    // Re-register: on some platforms (MinGW, System V) signal() resets
    // to SIG_DFL after invocation.  Without this, the second Ctrl+C
    // kills the process instead of interrupting cleanly.
    signal(SIGINT, SIGINT_handler);

    SATSolver* solver = solverToInterrupt;
    cout << "c " << endl;
    std::cerr << "*** INTERRUPTED ***" << endl;

    if (g_python_lib) {
        // Library mode (Python): only set the interrupt flag, let the
        // solver return l_Undef cleanly.  Do NOT call _exit() — that
        // would kill the Python process.
        if (solver) {
            solver->interrupt_asap();
        }
        return;
    }

    if (!redDumpFname.empty() || !irredDumpFname.empty() || need_clean_exit) {
        solver->interrupt_asap();
        std::cerr
        << "*** Please wait. We need to interrupt cleanly" << endl
        << "*** This means we might need to finish some calculations"
        << endl;
    } else {
        if (solver->nVars() > 0) {
            //if (conf.verbosity) {
                solver->add_in_partial_solving_stats();
                solver->print_stats(wallclock_time_started);
            //}
        } else {
            cout
            << "No clauses or variables were put into the solver, exiting without stats"
            << endl;
        }
        #if defined (_MSC_VER)
        exit(1);
        #else
        _exit(1);
        #endif
    }
}
