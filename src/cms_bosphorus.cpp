/******************************************
Copyright (c) 2020, Mate Soos

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

#include "cms_bosphorus.h"
#include "solver.h"
#include "sqlstats.h"
#include "bosphorus/bosphorus.hpp"
#include "time_mem.h"
#include "bosphorus/solvertypesmini.hpp"

using namespace CMSat;


CMSBosphorus::CMSBosphorus(Solver* _solver) :
    solver(_solver)
{}

CMSBosphorus::~CMSBosphorus()
{
    delete bosph;
}

void CMSBosphorus::add_clauses()
{
    //Add long clauses
    for(const auto& offs: solver->longIrredCls) {
        Clause & cl = *solver->cl_alloc.ptr(offs);
        bosph->add_dimacs_cl(dimacs, (Bosph::Lit*)cl.getData(), cl.size());
    }

    //Add binary clauses
    Lit lits[2];
    for(uint32_t i = 0; i < solver->nVars()*2; i++) {
        Lit l = Lit::toLit(i);
        lits[0] = l;
        for(const auto& w: solver->watches[l]) {
            if (w.isBin() && l < w.lit2() && !w.red()) {
                lits[1] = w.lit2();
                bosph->add_dimacs_cl(dimacs, (Bosph::Lit*)lits, 2);
            }
        }
    }
}


bool CMSBosphorus::doit()
{
    double myTime = cpuTime();
    uint32_t maxiters = 1;

    bosph = new Bosph::Bosphorus;
    dimacs = bosph->new_dimacs();
    add_clauses();

    auto anf = bosph->chunk_dimacs(dimacs);
    auto orig_anf = bosph->copy_anf_no_replacer(anf);

    //simplify, etc.
    bosph->simplify(anf, NULL, maxiters);
    bosph->add_trivial_learnt_from_anf_to_learnt(anf, orig_anf);
    bosph->delete_anf(orig_anf);
    bosph->deduplicate();

    auto cnf = bosph->cnf_from_anf_and_cnf(NULL, anf);
    auto cls = bosph->get_clauses(cnf);
    vector<Lit> lits;
    for(auto x: cls) {
        bool use = true;
        for(uint32_t i = 0; i < x.size(); i++) {
            Bosph::Lit l = x.lits[i];
            if (l.var() >= solver->nVars()) {
                use = false;
                break;
            }
        }
        if (use) {
            lits.clear();
            for(uint32_t i = 0; i < x.size(); i++) {
                Bosph::Lit* l = x.lits.data()+i;
                lits.push_back(*((Lit*)l));
            }
            cout << "c adding bosph cl: " << lits << endl;
            Clause* cl = solver->add_clause_int(
                lits //Literals in new clause
                , true //Is the new clause redundant?
            );
            if (cl) {
                cl->stats.glue = 2;
                auto cloffset = solver->cl_alloc.get_offset(cl);
                solver->longRedCls[0].push_back(cloffset);
            }
            if (!solver->okay()) {
                break;
            }
        }
    }
    bosph->delete_dimacs(dimacs);

    // Finish up
    double time_used = cpuTime() - myTime;
    if (solver->conf.verbosity) {
        cout << "c [bosph] finished "
        << solver->conf.print_times(time_used)
        << endl;
    }

    if (solver->sqlStats) {
        solver->sqlStats->time_passed_min(
            solver
            , "bosph"
            , time_used
        );
    }

    return solver->okay();
}
