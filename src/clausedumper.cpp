/******************************************
Copyright (c) 2016, Mate Soos

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

#include "clausedumper.h"
#include "solver.h"
#include "occsimplifier.h"
#include "varreplacer.h"
#include "comphandler.h"

using namespace CMSat;

void ClauseDumper::write_unsat_file()
{
    *outfile
    << "p cnf 0 1\n"
    << "0\n";
}

void ClauseDumper::open_file_and_write_sat(const std::string& fname)
{
    open_dump_file(fname);
    *outfile
    << "p cnf 0 0\n";
    delete outfile;
    outfile = NULL;
}

void ClauseDumper::open_file_and_write_unsat(const std::string& fname)
{
    open_dump_file(fname);
    *outfile
    << "p cnf 0 1\n"
    << "0\n";
    delete outfile;
    outfile = NULL;
}

void ClauseDumper::open_file_and_dump_red_clauses(const string& redDumpFname)
{
    open_dump_file(redDumpFname);
    try {
        if (!solver->okay()) {
            write_unsat_file();
        } else {
            dumpRedClauses();
        }
    } catch (std::ifstream::failure& e) {
        cout
        << "Error writing clause dump to file: " << e.what()
        << endl;
        std::exit(-1);
    }
    delete outfile;
    outfile = NULL;
}

void ClauseDumper::open_file_and_dump_irred_clauses(const string& irredDumpFname)
{
    open_dump_file(irredDumpFname);

    try {
        if (!solver->okay()) {
            write_unsat_file();
        } else {
            dumpIrredClauses();
        }
    } catch (std::ifstream::failure& e) {
        cout
        << "Error writing clause dump to file: " << e.what()
        << endl;
        std::exit(-1);
    }
    delete outfile;
    outfile = NULL;
}

void ClauseDumper::open_file_and_dump_irred_clauses_preprocessor(const string& irredDumpFname)
{
    open_dump_file(irredDumpFname);

    try {
        if (!solver->okay()) {
            write_unsat_file();
        } else {
            size_t num_cls = 0;
            num_cls += solver->longIrredCls.size();
            num_cls += solver->binTri.irredBins;

            *outfile
            << "p cnf " << solver->nVars() << " " << num_cls << "\n";

            dump_irred_cls_for_preprocessor(false);
        }
    } catch (std::ifstream::failure& e) {
        cout
        << "Error writing clause dump to file: " << e.what()
        << endl;
        std::exit(-1);
    }
    delete outfile;
    outfile = NULL;
}

void ClauseDumper::open_dump_file(const std::string& filename)
{
    delete outfile;
    outfile = NULL;
    std::ofstream* f =  new std::ofstream;
    f->open(filename.c_str());
    if (!f->good()) {
        cout
        << "Cannot open file '"
        << filename
        << "' for writing. exiting"
        << endl;
        std::exit(-1);
    }
    f->exceptions(std::ifstream::failbit | std::ifstream::badbit);
    outfile = f;
}



void ClauseDumper::dumpBinClauses(
    const bool dumpRed
    , const bool dumpIrred
    , const bool backnumber
) {
    size_t wsLit = 0;
    for (watch_array::const_iterator
        it = solver->watches.begin(), end = solver->watches.end()
        ; it != end
        ; ++it, wsLit++
    ) {
        Lit lit = Lit::toLit(wsLit);
        watch_subarray_const ws = *it;

        //Each element in the watchlist
        for (const Watched* it2 = ws.begin(), *end2 = ws.end()
            ; it2 != end2
            ; it2++
        ) {
            //Only dump binaries
            if (it2->isBin() && lit < it2->lit2()) {
                bool toDump = false;
                if (it2->red() && dumpRed) toDump = true;
                if (!it2->red() && dumpIrred) toDump = true;

                if (toDump) {
                    tmpCl.clear();
                    tmpCl.push_back(lit);
                    tmpCl.push_back(it2->lit2());
                    if (backnumber) {
                        tmpCl[0] = solver->map_inter_to_outer(tmpCl[0]);
                        tmpCl[1] = solver->map_inter_to_outer(tmpCl[1]);
                        std::sort(tmpCl.begin(), tmpCl.end());
                    }

                    *outfile
                    << tmpCl[0] << " "
                    << tmpCl[1]
                    << " 0\n";
                }
            }
        }
    }
}

void ClauseDumper::dumpEquivalentLits()
{
    *outfile
    << "c " << endl
    << "c ---------------------------------------" << endl
    << "c equivalent literals" << endl
    << "c ---------------------------------------" << endl;

    solver->varReplacer->print_equivalent_literals(outfile);
}

void ClauseDumper::dumpUnitaryClauses()
{
    *outfile
    << "c " << endl
    << "c ---------" << endl
    << "c unitaries" << endl
    << "c ---------" << endl;

    //'trail' cannot be trusted between 0....size()
    vector<Lit> lits = solver->get_zero_assigned_lits();
    for(const Lit lit: lits) {
        *outfile << lit << " 0\n";
    }
}

void ClauseDumper::dumpRedClauses() {
    if (solver->get_num_bva_vars() > 0) {
        std::cerr << "ERROR: cannot make meaningful dump with BVA turned on." << endl;
        exit(-1);
    }

    dumpUnitaryClauses();

    *outfile
    << "c " << endl
    << "c ---------------------------------" << endl
    << "c redundant binary clauses (extracted from watchlists)" << endl
    << "c ---------------------------------" << endl;
    dumpBinClauses(true, false, true);

    dumpEquivalentLits();

    *outfile
    << "c " << endl
    << "c --------------------" << endl
    << "c redundant long clauses locked in the DB" << endl
    << "c --------------------" << endl;
    dump_clauses(solver->longRedCls[0], true);
}

void ClauseDumper::dump_clauses(
    const vector<ClOffset>& cls
    , const bool backnumber
) {
    for(vector<ClOffset>::const_iterator
        it = cls.begin(), end = cls.end()
        ; it != end
        ; ++it
    ) {
        Clause* cl = solver->cl_alloc.ptr(*it);
        if (backnumber) {
            *outfile << sortLits(solver->clauseBackNumbered(*cl)) << " 0\n";
        } else {
            *outfile << *cl << " 0\n";
        }
    }
}

void ClauseDumper::dump_blocked_clauses()
{
    if (solver->conf.perform_occur_based_simp) {
        solver->occsimplifier->dump_blocked_clauses(outfile);
    }
}

void ClauseDumper::dump_component_clauses()
{
    if (solver->compHandler) {
        solver->compHandler->dump_removed_clauses(outfile);
    }
}

void ClauseDumper::dump_irred_cls_for_preprocessor(const bool backnumber)
{
    *outfile
    << "c " << endl
    << "c ---------------" << endl
    << "c binary clauses" << endl
    << "c ---------------" << endl;
    dumpBinClauses(false, true, backnumber);

    *outfile
    << "c " << endl
    << "c ---------------" << endl
    << "c long clauses" << endl
    << "c ---------------" << endl;
    dump_clauses(solver->longIrredCls, backnumber);
}

void ClauseDumper::dumpIrredClauses()
{
    if (solver->get_num_bva_vars() > 0) {
        std::cerr << "ERROR: cannot make meaningful dump with BVA turned on." << endl;
        exit(-1);
    }

    dumpUnitaryClauses();
    dumpEquivalentLits();

    dump_irred_cls_for_preprocessor(true);

    *outfile
    << "c " << endl
    << "c -------------------------------" << endl
    << "c previously eliminated variables" << endl
    << "c -------------------------------" << endl;
    dump_blocked_clauses();

    *outfile
    << "c " << endl
    << "c ---------------" << endl
    << "c clauses in components" << endl
    << "c ---------------" << endl;
    dump_component_clauses();
}
