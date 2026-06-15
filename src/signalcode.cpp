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

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

using namespace CMSat;

SATSolver* solverToInterrupt;
int need_clean_exit;
double wallclock_time_started = 0.0;
std::string redDumpFname;
std::string irredDumpFname;

using std::cout;
using std::endl;

#ifdef _WIN32
static BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
    if (fdwCtrlType == CTRL_C_EVENT || fdwCtrlType == CTRL_BREAK_EVENT) {
        std::cerr << "*** INTERRUPTED ***" << std::endl;
        if (solverToInterrupt) {
            // Set the atomic interrupt flag; the solver's search loop
            // will detect this, abort cleanly, return l_Undef, and the
            // normal exit path will print stats.
            solverToInterrupt->interrupt_asap();
        } else {
            // No solver yet (e.g. still parsing); just exit.
            exit(1);
        }
        return TRUE;  // signal handled, don't call next handler
    }
    return FALSE;  // pass to next handler (logoff, shutdown, etc.)
}
#endif

void setup_signal_handler()
{
#ifdef _WIN32
    SetConsoleCtrlHandler(CtrlHandler, TRUE);
#else
    signal(SIGINT, SIGINT_handler);
#endif
}

void SIGINT_handler(int)
{
    SATSolver* solver = solverToInterrupt;
    cout << "c " << endl;
    std::cerr << "*** INTERRUPTED ***" << endl;
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
