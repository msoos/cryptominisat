/******************************************
Copyright (c) 2018, Mate Soos

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

void ClauseDumper::write_unsat(std::ostream *out)
{
    *out
    << "p cnf 0 1\n"
    << "0\n";
}

void ClauseDumper::open_file_and_write_unsat(const std::string& fname)
{
    open_dump_file(fname);
    write_unsat(outfile);
    delete outfile;
    outfile = NULL;
}

void ClauseDumper::dump_irred_clauses(std::ostream *out) {
    if (!solver->okay()) {
        write_unsat(out);
    } else {
        dump_irred_cls(out, true);
    }
}

void ClauseDumper::open_file_and_dump_irred_clauses(const string& irredDumpFname)
{
    open_dump_file(irredDumpFname);
    try {
        dump_irred_clauses(outfile);
    } catch (std::ifstream::failure& e) {
        cout
        << "Error writing clause dump to file: " << e.what()
        << endl;
        std::exit(-1);
    }

    delete outfile;
    outfile = NULL;
}

void ClauseDumper::dump_red_clauses(std::ostream *out) {
        if (!solver->okay()) {
            write_unsat(out);
        } else {
            dump_red_cls(out, true);
        }
}

void ClauseDumper::open_file_and_dump_red_clauses(const string& redDumpFname)
{
    open_dump_file(redDumpFname);
    try {
        dump_red_clauses(outfile);
    } catch (std::ifstream::failure& e) {
        cout
        << "Error writing clause dump to file: " << e.what()
        << endl;
        std::exit(-1);
    }
    delete outfile;
    outfile = NULL;
}

size_t ClauseDumper::get_preprocessor_num_cls(bool outer_numbering) {
    size_t num_cls = 0;
    num_cls += solver->longIrredCls.size();
    num_cls += solver->binTri.irredBins;
    if (!outer_numbering) {
        vector<Lit> units = solver->get_toplevel_units_internal(false);
        num_cls += units.size();
    } else {
        vector<Lit> units = solver->get_zero_assigned_lits();
        num_cls += units.size();
    }
    num_cls += solver->undef_must_set_vars.size();
    num_cls += solver->varReplacer->print_equivalent_literals(outer_numbering)*2;

    return num_cls;
}

void ClauseDumper::dump_irred_clauses_preprocessor(std::ostream *out) {
    if (!solver->okay()) {
        write_unsat(out);
    } else {
        *out
        << "p cnf " << solver->nVars()
        << " " << get_preprocessor_num_cls(false) << "\n";

        if (solver->conf.sampling_vars) {
            *out << "c ind ";
            for(uint32_t outside_var: *solver->conf.sampling_vars) {
                uint32_t outer_var = solver->map_to_with_bva(outside_var);
                outer_var = solver->varReplacer->get_var_replaced_with_outer(outer_var);
                uint32_t int_var = solver->map_outer_to_inter(outer_var);
                if (int_var < solver->nVars()) {
                    *out << int_var+1 << " ";
                }
            }
            *out << " 0\n";
        }

        dump_irred_cls_for_preprocessor(out, false);
    }
}

void ClauseDumper::open_file_and_dump_irred_clauses_preprocessor(const string& irredDumpFname)
{
    open_dump_file(irredDumpFname);
    try {
        dump_irred_clauses_preprocessor(outfile);
    } catch (std::ifstream::failure& e) {
        cout
        << "Error writing clause dump to file: " << e.what()
        << endl;
        std::exit(-1);
    }
    delete outfile;
    outfile = NULL;
}

void ClauseDumper::dump_red_cls(std::ostream *out, bool outer_numbering)
{
    if (solver->get_num_bva_vars() > 0) {
        std::cerr << "ERROR: cannot make meaningful dump with BVA turned on." << endl;
        exit(-1);
    }

    *out << "c --- c red bin clauses" << endl;
    dump_bin_cls(out, true, false, outer_numbering);

    *out << "c ----- red long cls locked in the DB" << endl;
    dump_clauses(out, solver->longRedCls[0], outer_numbering);

    dump_eq_lits(out, outer_numbering);
}

void ClauseDumper::dump_irred_cls(std::ostream *out, bool outer_numbering)
{
    if (solver->get_num_bva_vars() > 0) {
        std::cerr << "ERROR: cannot make meaningful dump with BVA turned on." << endl;
        exit(-1);
    }

    size_t num_cls = get_preprocessor_num_cls(outer_numbering);
    num_cls += dump_blocked_clauses(NULL, outer_numbering);
    num_cls += dump_component_clauses(NULL, outer_numbering);

    *out
    << "p cnf " << solver->nVarsOutside()
    << " " << num_cls << "\n";

    dump_irred_cls_for_preprocessor(out, outer_numbering);

    *out << "c ------------------ previously eliminated variables" << endl;
    dump_blocked_clauses(out, outer_numbering);

    *out << "c ---------- clauses in components" << endl;
    dump_component_clauses(out, outer_numbering);
}

void ClauseDumper::dump_unit_cls(std::ostream *out, bool outer_numbering)
{
    *out << "c --------- unit clauses" << endl;
    if (outer_numbering) {
        //'trail' cannot be trusted between 0....size()
        vector<Lit> lits = solver->get_zero_assigned_lits();
        for(Lit lit: lits) {
            *out << lit << " 0\n";
        }
    } else {
        vector<Lit> units = solver->get_toplevel_units_internal(false);
        for(Lit l: units) {
            *out << l << " 0" << "\n";
        }
    }
}

uint32_t ClauseDumper::dump_blocked_clauses(std::ostream *out, bool outer_numbering) {
    assert(outer_numbering);
    uint32_t num_cls = 0;
    if (solver->conf.perform_occur_based_simp) {
        num_cls = solver->occsimplifier->dump_blocked_clauses(out);
    }
    return num_cls;
}

uint32_t ClauseDumper::dump_component_clauses(std::ostream *out, bool outer_numbering)
{
    assert(outer_numbering);
    uint32_t num_cls = 0;
    if (solver->compHandler) {
        num_cls = solver->compHandler->dump_removed_clauses(out);
    }
    return num_cls;
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

void ClauseDumper::dump_bin_cls(
    std::ostream *out,
    const bool dumpRed
    , const bool dumpIrred
    , const bool outer_number
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
                    if (outer_number) {
                        tmpCl[0] = solver->map_inter_to_outer(tmpCl[0]);
                        tmpCl[1] = solver->map_inter_to_outer(tmpCl[1]);
                    }

                    *out
                    << tmpCl[0] << " "
                    << tmpCl[1]
                    << " 0\n";
                }
            }
        }
    }
}

void ClauseDumper::dump_eq_lits(std::ostream *out, bool outer_numbering)
{
    *out << "c ------------ equivalent literals" << endl;
    solver->varReplacer->print_equivalent_literals(outer_numbering, out);
}

void ClauseDumper::dump_clauses(
    std::ostream *out,
    const vector<ClOffset>& cls
    , const bool outer_numbering
) {
    for(vector<ClOffset>::const_iterator
        it = cls.begin(), end = cls.end()
        ; it != end
        ; ++it
    ) {
        Clause* cl = solver->cl_alloc.ptr(*it);
        if (outer_numbering) {
            *out << solver->clause_outer_numbered(*cl) << " 0\n";
        } else {
            *out << *cl << " 0\n";
        }
    }
}

void ClauseDumper::dump_vars_appearing_inverted(std::ostream *out, bool outer_numbering)
{
    *out << "c ------------ vars appearing inverted in cls" << endl;
    for(size_t i = 0; i < solver->undef_must_set_vars.size(); i++) {
        if (!solver->undef_must_set_vars[i] ||
            solver->map_outer_to_inter(i) >= solver->nVars() ||
            solver->value(solver->map_outer_to_inter(i)) != l_Undef
        ) {
            continue;
        }

        Lit l = Lit(i, false);
        if (!outer_numbering) {
            l = solver->map_outer_to_inter(l);
        }
        *out << l << " " << ~l << " 0" << "\n";
    }
}

void ClauseDumper::dump_irred_cls_for_preprocessor(std::ostream *out, const bool outer_numbering)
{
    dump_unit_cls(out, outer_numbering);

    dump_vars_appearing_inverted(out, outer_numbering);

    *out << "c -------- irred bin cls" << endl;
    dump_bin_cls(out, false, true, outer_numbering);

    *out << "c -------- irred long cls" << endl;
    dump_clauses(out, solver->longIrredCls, outer_numbering);

    dump_eq_lits(out, outer_numbering);
}
