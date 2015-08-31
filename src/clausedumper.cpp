/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.0 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/

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
}

void ClauseDumper::open_file_and_write_unsat(const std::string& fname)
{
    open_dump_file(fname);
    *outfile
    << "p cnf 0 1\n"
    << "0\n";
}

void ClauseDumper::open_file_and_dump_red_clauses(const string& redDumpFname)
{
    open_dump_file(redDumpFname);
    try {
        if (!solver->okay()) {
            write_unsat_file();
        } else {
            dumpRedClauses(solver->conf.maxDumpRedsSize);
        }
    } catch (std::ifstream::failure& e) {
        cout
        << "Error writing clause dump to file: " << e.what()
        << endl;
        std::exit(-1);
    }
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
            num_cls += solver->binTri.irredTris;

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
}

void ClauseDumper::open_dump_file(const std::string& filename)
{
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
        for (watch_subarray_const::const_iterator
            it2 = ws.begin(), end2 = ws.end()
            ; it2 != end2
            ; it2++
        ) {
            //Only dump binaries
            if (it2->isBinary() && lit < it2->lit2()) {
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

void ClauseDumper::dumpTriClauses(
    const bool alsoRed
    , const bool alsoIrred
    , const bool backnumber
) {
    uint32_t wsLit = 0;
    for (watch_array::const_iterator
        it = solver->watches.begin(), end = solver->watches.end()
        ; it != end
        ; ++it, wsLit++
    ) {
        Lit lit = Lit::toLit(wsLit);
        watch_subarray_const ws = *it;
        for (watch_subarray_const::const_iterator
            it2 = ws.begin(), end2 = ws.end()
            ; it2 != end2
            ; it2++
        ) {
            //Only one instance of tri clause
            if (it2->isTri() && lit < it2->lit2()) {
                bool toDump = false;
                if (it2->red() && alsoRed) toDump = true;
                if (!it2->red() && alsoIrred) toDump = true;

                if (toDump) {
                    tmpCl.clear();
                    tmpCl.push_back(lit);
                    tmpCl.push_back(it2->lit2());
                    tmpCl.push_back(it2->lit3());
                    if (backnumber) {
                        tmpCl[0] = solver->map_inter_to_outer(tmpCl[0]);
                        tmpCl[1] = solver->map_inter_to_outer(tmpCl[1]);
                        tmpCl[2] = solver->map_inter_to_outer(tmpCl[2]);
                        std::sort(tmpCl.begin(), tmpCl.end());
                    }

                    *outfile
                    << tmpCl[0] << " "
                    << tmpCl[1] << " "
                    << tmpCl[2]
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


void ClauseDumper::dumpRedClauses(
    const uint32_t maxSize
) {
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
    if (maxSize >= 2) {
        dumpBinClauses(true, false, true);
    }

    *outfile
    << "c " << endl
    << "c ---------------------------------" << endl
    << "c redundant tertiary clauses (extracted from watchlists)" << endl
    << "c ---------------------------------" << endl;
    if (maxSize >= 3) {
        dumpTriClauses(true, false, true);
    }

    if (maxSize >= 2) {
        dumpEquivalentLits();
    }

    *outfile
    << "c " << endl
    << "c --------------------" << endl
    << "c redundant long clauses" << endl
    << "c --------------------" << endl;
    dump_clauses(solver->longRedCls, maxSize, true);
}

void ClauseDumper::dump_clauses(
    const vector<ClOffset>& cls
    , size_t max_size
    , const bool backnumber
) {
    for(vector<ClOffset>::const_iterator
        it = cls.begin(), end = cls.end()
        ; it != end
        ; ++it
    ) {
        Clause* cl = solver->cl_alloc.ptr(*it);
        if (cl->size() <= max_size) {
            if (backnumber) {
                *outfile << sortLits(solver->clauseBackNumbered(*cl)) << " 0\n";
            } else {
                *outfile << *cl << " 0\n";
            }
        }
    }
}

void ClauseDumper::dump_blocked_clauses()
{
    if (solver->conf.perform_occur_based_simp) {
        solver->simplifier->dump_blocked_clauses(outfile);
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
    << "c tertiary clauses" << endl
    << "c ---------------" << endl;
    dumpTriClauses(false, true, backnumber);

    *outfile
    << "c " << endl
    << "c ---------------" << endl
    << "c long clauses" << endl
    << "c ---------------" << endl;
    dump_clauses(solver->longIrredCls, std::numeric_limits<size_t>::max(), backnumber);
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
