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
#include "ccnr_cms.h"

using namespace CMSat;

SLS::SLS(Solver* _solver) :
    solver(_solver)
{}

SLS::~SLS()
{}

lbool SLS::run(const uint32_t num_sls_called)
{
    if (solver->conf.which_sls == "yalsat") {
        return run_yalsat();
    } else if (solver->conf.which_sls == "ccnr") {
        return run_ccnr(num_sls_called);
    } else if (solver->conf.which_sls == "walksat") {
        return run_walksat();
    } else if (solver->conf.which_sls == "ccnr_yalsat") {
        if ((num_sls_called % 2) == 0) {
            return run_ccnr(num_sls_called);
        } else {
            return run_yalsat();
        }
    } else {
        cout << "ERROR: SLS configuration '" << solver->conf.which_sls
        << "' does not exist. Only 'walksat', 'yalsat' and 'ccnr' are acceptable."
        << endl;
        exit(-1);
    }
}

lbool SLS::run_walksat()
{
    WalkSAT walksat(solver);
    double mem_needed_mb = (double)approx_mem_needed()/(1000.0*1000.0);
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
    double mem_needed_mb = (double)approx_mem_needed()/(1000.0*1000.0);
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

lbool SLS::run_ccnr(const uint32_t num_sls_called)
{
    CMS_ccnr ccnr(solver);
    double mem_needed_mb = (double)approx_mem_needed()/(1000.0*1000.0);
    double maxmem = solver->conf.sls_memoutMB*solver->conf.var_and_mem_out_mult;
    if (mem_needed_mb < maxmem) {
        lbool ret = ccnr.main(num_sls_called);
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

uint64_t SLS::approx_mem_needed()
{
    uint32_t numvars = solver->nVars();
    uint32_t numclauses = solver->longIrredCls.size() + solver->binTri.irredBins;
    uint64_t numliterals = solver->litStats.irredLits + solver->binTri.irredBins*2;
    uint64_t needed = 0;

    //LIT storage (all clause data)
    needed += (solver->litStats.irredLits+solver->binTri.irredBins*2)*sizeof(Lit);

    //This is just an estimation of yalsat's memory needs.

    //clause
    needed += sizeof(Lit *) * numclauses;
    //clsize
    needed += sizeof(uint32_t) * numclauses;

    //false_cls
    needed += sizeof(uint32_t) * numclauses;
    //map_cl_to_false_cls
    needed += sizeof(uint32_t) * numclauses;
    //numtruelit
    needed += sizeof(uint32_t) * numclauses;

    //occurrence
    needed += sizeof(uint32_t *) * (2 * numvars);
    //numoccurrence
    needed += sizeof(uint32_t) * (2 * numvars);
    //assigns
    needed += sizeof(lbool) * numvars;
    //breakcount
    needed += sizeof(uint32_t) * numvars;
    //makecount
    needed += sizeof(uint32_t) * numvars;

    //occur_list_alloc
    needed += sizeof(uint32_t) * numliterals;


    return needed;
}
