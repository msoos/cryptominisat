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

void ClauseDumper::open_file_and_dump_red_clauses(const string& redDumpFname)
{
    open_dump_file(redDumpFname);
    try {
        if (!solver->okay()) {
            *outfile
            << "p cnf 0 1\n"
            << "0\n";
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
            *outfile
            << "p cnf 0 1\n"
            << "0\n";
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
) {
    if (solver->get_num_bva_vars() > 0) {
        std::cerr << "ERROR: cannot make meaningful dump with BVA turned on." << endl;
        exit(-1);
    }

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
                    tmpCl.push_back(solver->map_inter_to_outer(it2->lit2()));
                    tmpCl.push_back(solver->map_inter_to_outer(lit));
                    std::sort(tmpCl.begin(), tmpCl.end());

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
) {
    if (solver->get_num_bva_vars() > 0) {
        std::cerr
        << "ERROR: cannot make meaningful dump with BVA turned on." << endl;
        exit(-1);
    }

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
                    tmpCl.push_back(solver->map_inter_to_outer(it2->lit2()));
                    tmpCl.push_back(solver->map_inter_to_outer(it2->lit3()));
                    tmpCl.push_back(solver->map_inter_to_outer(lit));
                    std::sort(tmpCl.begin(), tmpCl.end());

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
    if (solver->get_num_bva_vars() > 0) {
        std::cerr << "ERROR: cannot make meaningful dump with BVA turned on." << endl;
        exit(-1);
    }

    *outfile
    << "c " << endl
    << "c ---------------------------------------" << endl
    << "c equivalent literals" << endl
    << "c ---------------------------------------" << endl;

    solver->varReplacer->print_equivalent_literals(outfile);
}


void ClauseDumper::dumpUnitaryClauses()
{
    if (solver->get_num_bva_vars() > 0) {
        std::cerr << "ERROR: cannot make meaningful dump with BVA turned on." << endl;
        exit(-1);
    }

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


void ClauseDumper::dumpRedClauses(const uint32_t maxSize)
{
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
        dumpBinClauses(true, false);
    }

    *outfile
    << "c " << endl
    << "c ---------------------------------" << endl
    << "c redundant tertiary clauses (extracted from watchlists)" << endl
    << "c ---------------------------------" << endl;
    if (maxSize >= 3) {
        dumpTriClauses(true, false);
    }

    if (maxSize >= 2) {
        dumpEquivalentLits();
    }

    *outfile
    << "c " << endl
    << "c --------------------" << endl
    << "c redundant long clauses" << endl
    << "c --------------------" << endl;
    dump_clauses(solver->longRedCls, maxSize);
}

void ClauseDumper::dump_clauses(
    const vector<ClOffset>& cls
    , size_t max_size
) {
    if (solver->get_num_bva_vars() > 0) {
        std::cerr << "ERROR: cannot make meaningful dump with BVA turned on." << endl;
        exit(-1);
    }

    for(vector<ClOffset>::const_iterator
        it = cls.begin(), end = cls.end()
        ; it != end
        ; ++it
    ) {
        Clause* cl = solver->cl_alloc.ptr(*it);
        if (cl->size() <= max_size)
            *outfile << sortLits(solver->clauseBackNumbered(*cl)) << " 0\n";
    }
}

void ClauseDumper::dump_blocked_clauses()
{
    if (solver->get_num_bva_vars() > 0) {
        std::cerr << "ERROR: cannot make meaningful dump with BVA turned on." << endl;
        exit(-1);
    }

    if (solver->conf.perform_occur_based_simp) {
        solver->simplifier->dump_blocked_clauses(outfile);
    }
}

void ClauseDumper::dump_component_clauses()
{
    if (solver->get_num_bva_vars() > 0) {
        std::cerr << "ERROR: cannot make meaningful dump with BVA turned on." << endl;
        exit(-1);
    }

    if (solver->compHandler) {
        solver->compHandler->dump_removed_clauses(outfile);
    }
}

void ClauseDumper::dumpIrredClauses()
{
    if (solver->get_num_bva_vars() > 0) {
        std::cerr << "ERROR: cannot make meaningful dump with BVA turned on." << endl;
        exit(-1);
    }

    dumpUnitaryClauses();
    dumpEquivalentLits();

    *outfile
    << "c " << endl
    << "c ---------------" << endl
    << "c binary clauses" << endl
    << "c ---------------" << endl;
    dumpBinClauses(false, true);

    *outfile
    << "c " << endl
    << "c ---------------" << endl
    << "c tertiary clauses" << endl
    << "c ---------------" << endl;
    dumpTriClauses(false, true);

    *outfile
    << "c " << endl
    << "c ---------------" << endl
    << "c normal clauses" << endl
    << "c ---------------" << endl;
    dump_clauses(solver->longIrredCls);

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
