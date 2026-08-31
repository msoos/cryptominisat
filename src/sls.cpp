/******************************************
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
***********************************************/

#include "sls.h"
#include "solver.h"
#include "yalsat_cms.h"
#include "sqlstats.h"
#include "time_mem.h"

using namespace CMSat;

SLS::SLS(Solver* _solver) : solver(_solver) {}

//CaDiCaL's 'walk': effort relative to the search propagations done so far
int64_t SLS::effort() const
{
    int64_t limit = (double)solver->sumPropagations * 1e-3 * solver->conf.walkreleff;
    limit = std::max<int64_t>(limit, solver->conf.walkmineff);
    limit = std::min<int64_t>(limit, solver->conf.walkmaxeff);
    return limit;
}

void SLS::run_during_search() { run(effort()); }

//CaDiCaL's 'local_search': effort grows quadratically with the round number
void SLS::run_initially()
{
    for(uint32_t i = 1; i <= solver->conf.walkinitially; i++) {
        run((int64_t)solver->conf.walkmineff * i * i);
        if (solver->sls_minimum == 0) break;
    }
}

void SLS::run(const int64_t mems)
{
    assert(solver->decisionLevel() == 0);

    //Local search does not do well on tiny formulas
    if (solver->nVars() < 50 ||
        solver->binTri.irredBins + solver->longIrredCls.size() < 10)
    {
        verb_print(1, "[sls] too few variables & clauses");
        return;
    }
    if (!enough_mem()) return;

    const double my_time = cpu_time();
    CMS_yalsat sls(solver);
    if (!sls.init_problem()) {
        //it's actually l_False under assumptions, let the main solver deal with it
        verb_print(1, "[sls] UNSAT under assumptions, returning to main solver");
        return;
    }
    if (sls.get_num_cls() == 0) return;

    //CaDiCaL's 'walk_round' starts off the CDCL phases. yalsat picks (and on its
    //own restarts re-picks) the initial assignment itself, and forcing anything
    //on it -- the CDCL phases or the previous round's outcome -- keeps it in the
    //same basin and costs an order of magnitude here. Hence this is off, and the
    //rounds of run_initially() are independent draws.
    if (solver->conf.walkseedphase)
        for(uint32_t v = 0; v < solver->nVars(); v++)
            if (sls.has_value(v)) sls.set_phase(v, solver->decide_phase(v, true));

    const int64_t minimum = sls.run(mems, solver->conf.origSeed + solver->num_sls_called);
    solver->num_sls_called++;

    const bool improved = minimum >= 0 && minimum < solver->sls_minimum;
    if (improved) {
        solver->sls_minimum = minimum;
        for(uint32_t v = 0; v < solver->nVars(); v++)
            if (sls.has_value(v)) solver->varData[v].saved_polarity = sls.value(v);
    }

    const double time_used = cpu_time()-my_time;
    verb_print(1, "[sls] " << sls.get_num_cls() << " cls, "
        << (minimum < 0 ? "no assignment" : "minimum " + std::to_string(minimum))
        << (improved ? " (new global minimum)" : "")
        << solver->conf.print_times(time_used));
    if (minimum >= 0) {
        int64_t cnf, xr;
        const int64_t check = sls.count_unsat(cnf, xr);
        assert(check == minimum); (void)check;
        verb_print(2, "[sls] unsat: " << cnf << " CNF cls, " << xr << " XORs");
    }
    if (solver->sqlStats) solver->sqlStats->time_passed_min(solver, "sls", time_used);
}

vector<vector<uint8_t>> SLS::run_alter(const int64_t mems, uint32_t num)
{
    vector<vector<uint8_t>> sols;
    for(uint32_t i = 0; i < num; i++) {
        CMS_yalsat sls(solver);
        if (!sls.init_problem() || sls.get_num_cls() == 0) break;
        if (sls.run(mems, solver->conf.origSeed + i) != 0) continue;

        vector<uint8_t> sol(solver->nVars(), 0);
        for(uint32_t v = 0; v < solver->nVars(); v++) {
            if (sls.has_value(v)) sol[v] = sls.value(v);
            else if (solver->value(v) != l_Undef) sol[v] = solver->value(v) == l_True;
        }
        sols.push_back(sol);
    }
    return sols;
}

bool SLS::enough_mem() const
{
    const double needed_mb = (double)approx_mem_needed()/(1000.0*1000.0);
    const double maxmem = solver->conf.sls_memoutMB*solver->conf.var_and_mem_out_mult;
    if (needed_mb < maxmem) return true;

    verb_print(1, "[sls] would need "
        << std::setprecision(2) << std::fixed << needed_mb
        << " MB but that's over limit of " << std::fixed << maxmem
        << " MB -- skipping");
    return false;
}

uint64_t SLS::approx_mem_needed() const
{
    const uint32_t numvars = solver->nVars();
    uint32_t numclauses = solver->longIrredCls.size() + solver->binTri.irredBins;
    uint64_t numliterals = solver->litStats.irredLits + solver->binTri.irredBins*2;
    for(const auto& x: solver->xorclauses) { numclauses++; numliterals += x.size(); }

    uint64_t needed = 0;
    needed += numliterals*sizeof(int);       //literal storage, twice: clauses and occurrences
    needed += numliterals*sizeof(int);
    needed += numclauses*sizeof(int)*4;      //per-clause bookkeeping
    needed += numvars*sizeof(int)*4;         //per-variable bookkeeping
    needed += 2*numvars*(sizeof(int*)+sizeof(int));
    return needed;
}
