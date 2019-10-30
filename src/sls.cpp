/******************************************
Copyright (c) 2019, Mate Soos

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

#include "sls.h"
#include "solver.h"
#include "yalsat.h"
#include "walksat.h"

using namespace CMSat;

SLS::SLS(Solver* _solver) :
    solver(_solver)
{}

SLS::~SLS()
{}

lbool SLS::run()
{
    if (solver->conf.which_sls == "yalsat") {
        return run_yalsat();
    } else if (solver->conf.which_sls == "walksat") {
        return run_walksat();
    } else {
        cout << "ERROR: SLS configuration '" << solver->conf.which_sls
        << "' does not exist. Only 'walksat' and 'yalsat' are acceptable."
        << endl;
        exit(-1);
    }
}

lbool SLS::run_walksat()
{
    WalkSAT walksat(solver);
    double mem_needed_mb = (double)walksat.mem_needed()/(1000.0*1000.0);
    double maxmem = solver->conf.sls_memoutMB*solver->conf.var_and_mem_out_mult;
    if (mem_needed_mb < maxmem) {
        lbool ret = walksat.main();
        return ret;
    }

    if (solver->conf.verbosity) {
        cout << "c [sls] would need "
        << std::setprecision(2) << std::fixed << mem_needed_mb
        << " MB but that's over limit of " << std::fixed << maxmem
        << " MB -- skipping" << endl;
    }

    return l_Undef;
}

lbool SLS::run_yalsat()
{
    Yalsat yalsat(solver);
    double mem_needed_mb = (double)yalsat.mem_needed()/(1000.0*1000.0);
    double maxmem = solver->conf.sls_memoutMB*solver->conf.var_and_mem_out_mult;
    if (mem_needed_mb < maxmem) {
        lbool ret = yalsat.main();
        return ret;
    }

    if (solver->conf.verbosity) {
        cout << "c [sls] would need "
        << std::setprecision(2) << std::fixed << mem_needed_mb
        << " MB but that's over limit of " << std::fixed << maxmem
        << " MB -- skipping" << endl;
    }

    return l_Undef;
}
