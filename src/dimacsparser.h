/*****************************************************************************
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
CryptoMiniSat -- Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

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
******************************************************************************/

#pragma once

#include <string.h>
#include "streambuffer.h"
#include "solvertypesmini.h"
#include <cstdlib>
#include <cmath>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <fstream>
#include <complex>
#include <cassert>

using std::vector;
using std::cout;
using std::endl;

namespace CMSat {

template <class C, class S>
class DimacsParser
{
    public:
        DimacsParser(S* solver, const std::string* debugLib, unsigned _verbosity);

        template <class T> bool parse_DIMACS(
            T input_stpeam,
            const bool strict_header,
            uint32_t offset_vars = 0);
        uint64_t max_var = numeric_limits<uint64_t>::max();
        vector<uint32_t> sampling_vars;
        bool sampling_vars_found = false;
        vector<double> weights;
        uint32_t must_mult_exp2 = 0;
        const std::string dimacs_spec = "http://www.satcompetition.org/2009/format-benchmarks2009.html";
        const std::string please_read_dimacs = "\nPlease read DIMACS specification at http://www.satcompetition.org/2009/format-benchmarks2009.html";

    private:
        bool parse_DIMACS_main(C& in);
        bool readClause(C& in);
        bool parse_and_add_clause(C& in);
        bool parse_and_add_xor_clause(C& in);
        #ifdef ENABLE_BNN
        bool parse_and_add_bnn_clause(C& in);
        #endif
        bool match(C& in, const char* str);
        bool parse_header(C& in);
        bool parseComments(C& in, const std::string& str);
        std::string stringify(uint32_t x) const;
        bool check_var(const uint32_t var);
        bool parseWeight(C& in);

        #ifdef DEBUG_DIMACSPARSER_CMS
        bool parse_solve_simp_comment(C& in, const bool solve);
        void write_solution_to_debuglib_file(const lbool ret) const;
        #endif

        bool parseIndependentSet(C& in);
        std::string get_debuglib_fname() const;


        S* solver;
        std::string debugLib;
        unsigned verbosity;

        //Stat
        size_t lineNum;

        //Printing partial solutions to debugLibPart1..N.output when "debugLib" is set to TRUE
        uint32_t debugLibPart = 1;

        //check header strictly
        bool strict_header = false;
        bool header_found = false;
        int num_header_vars = 0;
        int num_header_cls = 0;
        uint32_t offset_vars = 0;

        //Reduce temp overhead
        vector<Lit> lits;
        vector<uint32_t> vars;

        size_t norm_clauses_added = 0;
        size_t xor_clauses_added = 0;
        #ifdef ENABLE_BNN
        size_t bnn_clauses_added = 0;
        #endif
};


template<class C, class S>
DimacsParser<C, S>::DimacsParser(
    S* _solver
    , const std::string* _debugLib
    , unsigned _verbosity
):
    solver(_solver)
    , verbosity(_verbosity)
    , lineNum(0)
{
    if (_debugLib) {
        debugLib = *_debugLib;
    }
}

template<class C, class S>
std::string DimacsParser<C, S>::stringify(uint32_t x) const
{
    std::ostringstream o;
    o << x;
    return o.str();
}

template<class C, class S>
bool DimacsParser<C, S>::check_var(const uint32_t var)
{
    if (var > max_var) {
        std::cerr
        << "ERROR! "
        << "Variable requested is too large for DIMACS parser parameter: "
        << var << endl
        << "--> At line " << lineNum+1
        << please_read_dimacs
        << endl;
        return false;
    }

    if (var >= (1ULL<<28)) {
        std::cerr
        << "ERROR! "
        << "Variable requested is far too large: " << var + 1 << endl
        << "--> At line " << lineNum+1
        << please_read_dimacs
        << endl;
        return false;
    }

    if (strict_header && !header_found) {
        std::cerr
        << "ERROR! "
        << "DIMACS header ('p cnf vars cls') never found!" << endl;
        return false;
    }

    if ((int)var >= num_header_vars && strict_header) {
        std::cerr
        << "ERROR! "
        << "Variable requested is larger than the header told us." << endl
        << " -> var is : " << var + 1 << endl
        << " -> header told us maximum will be : " << num_header_vars << endl
        << " -> At line " << lineNum+1
        << endl;
        return false;
    }

    if (var >= solver->nVars()) {
        assert(!strict_header);
        solver->new_vars(var - solver->nVars() +1);
    }

    return true;
}

template<class C, class S>
bool DimacsParser<C, S>::readClause(C& in)
{
    int32_t parsed_lit;
    uint32_t var;
    for (;;) {
        if (!in.parseInt(parsed_lit, lineNum)) {
            return false;
        }
        if (parsed_lit == 0) {
            break;
        }

        var = std::abs(parsed_lit)-1;
        var += offset_vars;

        if (!check_var(var)) {
            return false;
        }

        lits.push_back( (parsed_lit > 0) ? Lit(var, false) : Lit(var, true) );
        if (*in != ' ') {
            std::cerr
            << "ERROR! "
            << "After last element on the line must be 0" << endl
            << "--> At line " << lineNum+1
            << please_read_dimacs
            << endl
            << endl;
            return false;
        }
    }

    return true;
}

template<class C, class S>
bool DimacsParser<C, S>::match(C& in, const char* str)
{
    for (; *str != 0; ++str, ++in)
        if (*str != *in)
            return false;
    return true;
}

#ifdef DEBUG_DIMACSPARSER_CMS
template<class C, class S>
bool DimacsParser<C, S>::parseWeight(C& in)
{
    if (match(in, "w ")) {
        int32_t slit;
        double weight;
        if (in.parseInt(slit, lineNum)
            && in.parseDouble(weight, lineNum)
        ) {
            if (slit == 0) {
                cout << "ERROR: Cannot define weight of literal 0!" << endl;
                exit(-1);
            }
            uint32_t var = std::abs(slit)-1;
            bool sign = slit < 0;
            Lit lit = Lit(var, sign);
            solver->set_var_weight(lit, weight);
            //cout << "lit: " << lit << " weight: " << std::setprecision(12) << weight << endl;
            if (weight < 0) {
                cout << "ERROR: while definint weight, variable " << var+1 << " has is negative weight: " << weight << " -- line " << lineNum << endl;
                exit(-1);
            }
            return true;
        } else {
            cout << "ERROR: weight is incorrect on line " << lineNum << endl;
            exit(-1);
        }
    } else {
        cout << "ERROR: weight is not given on line " << lineNum << endl;
        exit(-1);
    }
    return true;
}
#endif

template<class C, class S>
bool DimacsParser<C, S>::parse_header(C& in)
{
    ++in;
    in.skipWhitespace();
    std::string str;
    in.parseString(str);
    if (str == "cnf") {
        if (header_found && strict_header) {
            std::cerr << "ERROR: CNF header ('p cnf vars cls') found twice in file! Exiting." << endl;
            exit(-1);
        }
        header_found = true;

        if (!in.parseInt(num_header_vars, lineNum)
            || !in.parseInt(num_header_cls, lineNum)
        ) {
            return false;
        }
        if (verbosity) {
            cout << "c -- header says num vars:   " << std::setw(12) << num_header_vars << endl;
            cout << "c -- header says num clauses:" <<  std::setw(12) << num_header_cls << endl;
        }
        if (num_header_vars < 0) {
            std::cerr << "ERROR: Number of variables in header cannot be less than 0" << endl;
            return false;
        }
        if (num_header_cls < 0) {
            std::cerr << "ERROR: Number of clauses in header cannot be less than 0" << endl;
            return false;
        }
        num_header_vars += offset_vars;

        if (solver->nVars() < (size_t)num_header_vars) {
            solver->new_vars(num_header_vars-solver->nVars());
        }
    } else {
        std::cerr
        << "PARSE ERROR! Unexpected char (hex: " << std::hex
        << std::setw(2)
        << std::setfill('0')
        << "0x" << *in
        << std::setfill(' ')
        << std::dec
        << ")"
        << " At line " << lineNum+1
        << "' in the header"
        << please_read_dimacs
        << endl;
        return false;
    }

    return true;
}

template<class C, class S>
std::string DimacsParser<C, S>::get_debuglib_fname() const
{
    std::string sol_fname = debugLib + "-debugLibPart" + stringify(debugLibPart) +".output";
    return sol_fname;
}

#ifdef DEBUG_DIMACSPARSER_CMS
template<class C, class S>
bool DimacsParser<C, S>::parse_solve_simp_comment(C& in, const bool solve)
{
    vector<Lit> assumps;
    in.skipWhitespace();
    while(*in != ')') {
        int lit;
        if (!in.parseInt(lit, lineNum)) {
            return false;
        }
        assumps.push_back(Lit(std::abs(lit)-1, lit < 0));
        in.skipWhitespace();
    }

    if (verbosity) {
        cout
        << "c -----------> Solver::"
        << (solve ? "solve" : "simplify")
        <<" called (number: "
        << std::setw(3) << debugLibPart << ") with assumps :";
        for(Lit lit: assumps) {
            cout << lit << " ";
        }
        cout << "<-----------" << endl;
    }

    lbool ret;
    if (solve) {
        if (verbosity) {
            cout << "c Solution will be written to: "
            << get_debuglib_fname() << endl;
        }
        ret = solver->solve(&assumps);
        write_solution_to_debuglib_file(ret);
        debugLibPart++;
    } else {
        ret = solver->simplify(&assumps);
    }

    if (verbosity >= 6) {
        cout << "c Parsed Solver::"
        << (solve ? "solve" : "simplify")
        << endl;
    }
    return true;
}

template<class C, class S>
void DimacsParser<C, S>::write_solution_to_debuglib_file(const lbool ret) const
{
    //Open file for writing
    std::string s = get_debuglib_fname();
    std::ofstream partFile;
    partFile.open(s.c_str());
    if (!partFile) {
        std::cerr << "ERROR: Cannot open part file '" << s << "'";
        std::exit(-1);
    }

    //Output to part file the result
    if (ret == l_True) {
        partFile << "s SATISFIABLE\n";
        partFile << "v ";
        for (uint32_t i = 0; i != solver->nVars(); i++) {
            if (solver->get_model()[i] != l_Undef)
                partFile
                << ((solver->get_model()[i]==l_True) ? "" : "-")
                << (i+1) <<  " ";
        }
        partFile << "0\n";
    } else if (ret == l_False) {
        partFile << "conflict ";
        for (Lit lit: solver->get_conflict()) {
            partFile << lit << " ";
        }
        partFile
        << "\ns UNSAT\n";
    } else if (ret == l_Undef) {
        cout << "c timeout, exiting" << endl;
        std::exit(15);
    } else {
        assert(false);
    }
    partFile.close();
}
#endif

template<class C, class S>
bool DimacsParser<C, S>::parseComments(C& in, const std::string& str)
{
    #ifdef DEBUG_DIMACSPARSER_CMS
    if (!debugLib.empty() && str.substr(0, 13) == "Solver::solve") {
        if (!parse_solve_simp_comment(in, true)) {
            return false;
        }
    } else if (!debugLib.empty() && str.substr(0, 16) == "Solver::simplify") {
        if (!parse_solve_simp_comment(in, false)) {
            return false;
        }
    } else
    #endif
    if (str == "red") {
        in.skipWhitespace();
        lits.clear();
        if (!readClause(in)) return false;
        solver->add_red_clause(lits);
    }
    if (str == "MUST") {
        std::string str2;
        in.skipWhitespace();
        in.parseString(str2);
        if (str2 != "MULTIPLY") {
            cout << "ERROR: expected 'MULTIPLY' after 'MUST'" << endl;
            return false;
        }
        in.skipWhitespace();
        in.parseString(str2);
        if (str2 != "BY") {
            cout << "ERROR: expected 'BY' after 'MUST MULTIPLY'" << endl;
            return false;
        }
        in.skipWhitespace();
        assert(*in == '2');++in;
        assert(*in == '*');++in;
        assert(*in == '*');++in;
        if (!in.parseInt(must_mult_exp2, lineNum)) {
            return false;
        }
    }
    if (!debugLib.empty() && str == "Solver::new_var()") {
        solver->new_var();

        if (verbosity >= 6) {
            cout << "c Parsed Solver::new_var()" << endl;
        }
    } else if (!debugLib.empty() && str == "Solver::new_vars(") {
        in.skipWhitespace();
        int n;
        if (!in.parseInt(n, lineNum)) {
            return false;
        }
        solver->new_vars(n);

        if (verbosity >= 6) {
            cout << "c Parsed Solver::new_vars( " << n << " )" << endl;
        }
    } else if (str == "ind") {
        sampling_vars_found = true;
        if (!parseIndependentSet(in)) {
            return false;
        }
    } else if (str == "p") {
        in.skipWhitespace();
        std::string str2;
        in.parseString(str2);
        if (str2 == "show") {
            in.skipWhitespace();
            sampling_vars_found = true;
            if (!parseIndependentSet(in)) { return false; }
        } else {
            cout << "ERROR, 'c p' followed by unknown text: '" << str2 << "'" << endl;
            exit(-1);
        }
    } else {
        if (verbosity >= 6) {
            cout
            << "didn't understand in CNF file comment line:"
            << "'c " << str << "'"
            << endl;
        }
    }
    in.skipLine();
    lineNum++;
    return true;
}

template<class C, class S>
bool DimacsParser<C, S>::parse_and_add_clause(C& in)
{
    lits.clear();
    if (!readClause(in)) {
        return false;
    }
    in.skipWhitespace();
    if (!in.skipEOL(lineNum)) {
        return false;
    }
    lineNum++;
    solver->add_clause(lits);
    norm_clauses_added++;
    return true;
}

#ifdef ENABLE_BNN
// This parses a threshold constraint of the form:
// b lit1..litn 0 cutoff 0 [output_lit]
// where output_lit is optional and if missing is assumed to be TRUE
// basically, lit1+lit2+litN >= cutoff  <=> output_lit=TRUE
// where lit1 is 1 if it's TRUE and 0 otherwise
template<class C, class S>
bool DimacsParser<C, S>::parse_and_add_bnn_clause(C& in)
{
    // Read in inputs to BNN
    lits.clear();
    if (!readClause(in)) {
        return false;
    }
    if (lits.empty()) {
        std::cerr
        << "ERROR! "
        << "BNN constraint has empty set of inputs" << endl
        << "--> At line " << lineNum+1
        << endl;
        return false;
    }

    // Read cutoff
    int32_t cutoff;
    if (!in.parseInt(cutoff, lineNum)) {
        return false;
    }

    in.skipWhitespace();

    Lit out = lit_Undef;
    if (*in != '\n') {
        // Read in output var
        int32_t parsed_lit;
        if (!in.parseInt(parsed_lit, lineNum)) {
            return false;
        }
        assert(parsed_lit != 0);
        uint32_t var = std::abs(parsed_lit)-1;
        var += offset_vars;

        //off-by-one internally.
        if (!check_var(var)) {
            return false;
        }
        out = Lit(var, parsed_lit < 0);
    }

    // Line finished
    in.skipLine();
    lineNum++;

    solver->add_bnn_clause(lits, cutoff, out);
    bnn_clauses_added++;
    return true;
}
#endif

template<class C, class S>
bool DimacsParser<C, S>::parse_and_add_xor_clause(C& in)
{
    lits.clear();
    if (!readClause(in)) {
        return false;
    }
    if (!in.skipEOL(lineNum)) {
        return false;
    }
    lineNum++;
    if (lits.empty())
        return true;

    bool rhs = true;
    vars.clear();
    for(Lit& lit: lits) {
        vars.push_back(lit.var());
        if (lit.sign()) {
            rhs ^= true;
        }
    }
    solver->add_xor_clause(vars, rhs);
    xor_clauses_added++;
    return true;
}

template<class C, class S>
bool DimacsParser<C, S>::parse_DIMACS_main(C& in)
{
    std::string str;

    for (;;) {
        in.skipWhitespace();
        switch (*in) {
        case EOF:
            return true;
        case 'p':
            if (!parse_header(in)) {
                return false;
            }
            in.skipLine();
            lineNum++;
            break;

        #ifdef DEBUG_DIMACSPARSER_CMS
        case 'w':
            if (!parseWeight(in)) {
                return false;
            }
            in.skipLine();
            lineNum++;
            break;
        #endif
        case 'c':
            ++in;
            in.parseString(str);
            if (!parseComments(in, str)) {
                return false;
            }
            break;
        case 'x':
            ++in;
            if (!parse_and_add_xor_clause(in)) {
                return false;
            }
            break;
        case 'b':
            #ifdef ENABLE_BNN
            ++in;
            if (!parse_and_add_bnn_clause(in)) {
                return false;
            }
            #else
            std::cout << "ERROR: BNN encounered but not enabled in parsing. Exiting." << endl;
            exit(-1);
            #endif
            break;
        case '\n':
            if (verbosity) {
                std::cout
                << "c WARNING: Empty line at line number " << lineNum+1
                << " -- this is not part of the DIMACS specifications ("
                << dimacs_spec << "). Ignoring."
                << endl;
            }
            in.skipLine();
            lineNum++;
            break;
        default:
            if (!parse_and_add_clause(in)) {
                return false;
            }
            break;
        }
    }

    return true;
}

template <class C, class S>
template <class T>
bool DimacsParser<C, S>::parse_DIMACS(
    T input_stream,
    const bool _strict_header,
    uint32_t _offset_vars)
{
    debugLibPart = 1;
    strict_header = _strict_header;
    offset_vars = _offset_vars;
    const uint32_t origNumVars = solver->nVars();

    C in(input_stream);
    if ( !parse_DIMACS_main(in)) {
        return false;
    }

    if (verbosity) {
        cout
        << "c -- clauses added: " << norm_clauses_added << endl
        << "c -- xor clauses added: " << xor_clauses_added << endl
        #ifdef ENABLE_BNN
        << "c -- bnn clauses added: " << bnn_clauses_added << endl
        #endif
        << "c -- vars added " << (solver->nVars() - origNumVars)
        << endl;
    }

    return true;
}

template <class C, class S>
bool DimacsParser<C, S>::parseIndependentSet(C& in)
{
    int32_t parsed_lit;
    for (;;) {
        if (!in.parseInt(parsed_lit, lineNum)) {
            return false;
        }
        if (parsed_lit == 0) {
            break;
        }
        uint32_t var = std::abs(parsed_lit) - 1;
        sampling_vars.push_back(var);
    }
    return true;
}

}
