/*****************************************************************************
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

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

#include "dimacsparser.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <fstream>
#include <complex>
#include "assert.h"

using std::vector;
using std::cout;
using std::endl;

DimacsParser::DimacsParser(
    SATSolver* _solver
    , const bool _debugLib
    , unsigned _verbosity
):
    solver(_solver)
    , debugLib(_debugLib)
    , verbosity(_verbosity)
    , lineNum(0)
{}

void DimacsParser::skipWhitespace(StreamBuffer& in)
{
    char c = *in;
    while ((c >= 9 && c <= 13 && c != 10) || c == 32) {
        ++in;
        c = *in;
    }
}

void DimacsParser::skipLine(StreamBuffer& in)
{
    lineNum++;
    for (;;) {
        if (*in == EOF || *in == '\0') return;
        if (*in == '\n') {
            ++in;
            return;
        }
        ++in;
    }
}

int32_t DimacsParser::parseInt(StreamBuffer& in)
{
    int32_t val = 0;
    int32_t mult = 1;
    skipWhitespace(in);
    if (*in == '-') {
        mult = -1;
        ++in;
    } else if (*in == '+') {
        ++in;
    }

    char c = *in;
    if (c < '0' || c > '9') {
        cout
        << "PARSE ERROR! Unexpected char (dec: '" << c << ")"
        << " At line " << lineNum
        << " we expected a number"
        << endl;
        std::exit(3);
    }

    while (c >= '0' && c <= '9') {
        val = val*10 + (c - '0');
        ++in;
        c = *in;
    }
    return mult*val;
}

std::string DimacsParser::stringify(uint32_t x) const
{
    std::ostringstream o;
    o << x;
    return o.str();
}

/**
@brief Parse a continious set of characters from "in" to "str".

\todo EOF is not checked for!!
*/
void DimacsParser::parseString(StreamBuffer& in, std::string& str)
{
    str.clear();
    skipWhitespace(in);
    while (*in != ' ' && *in != '\n' && *in != EOF) {
        str.push_back(*in);
        ++in;
    }
}

void DimacsParser::readClause(StreamBuffer& in)
{
    int32_t parsed_lit;
    uint32_t var;
    for (;;) {
        parsed_lit = parseInt(in);
        if (parsed_lit == 0) break;
        var = abs(parsed_lit)-1;
        if (var >= (1ULL<<28)) {
            std::cerr
            << "ERROR! "
            << "Variable requested is far too large: " << var << endl
            << "--> At line " << lineNum+1
            << endl;
            std::exit(-1);
        }

        while (var >= solver->nVars()) {
            solver->new_var();
        }
        lits.push_back( (parsed_lit > 0) ? Lit(var, false) : Lit(var, true) );
        if (*in != ' ') {
            std::cerr
            << "ERROR! "
            << "After each literal there must be an empty space!"
            << "--> At line " << lineNum+1 << endl
            << endl;
            std::exit(-1);
        }
    }
}

/**
@brief Matches parameter "str" to content in "in"
*/
bool DimacsParser::match(StreamBuffer& in, const char* str)
{
    for (; *str != 0; ++str, ++in)
        if (*str != *in)
            return false;
    return true;
}

void DimacsParser::printHeader(StreamBuffer& in)
{
    if (match(in, "p cnf")) {
        int vars    = parseInt(in);
        int clauses = parseInt(in);
        if (verbosity >= 1) {
            cout << "c -- header says num vars:   " << std::setw(12) << vars << endl;
            cout << "c -- header says num clauses:" <<  std::setw(12) << clauses << endl;
        }
        if (vars < 0) {
            std::cerr << "ERROR: Number of variables in header cannot be less than 0" << endl;
            exit(-1);
        }
        if (clauses < 0) {
            std::cerr << "ERROR: Number of clauses in header cannot be less than 0" << endl;
            exit(-1);
        }

        if (solver->nVars() <= (size_t)vars) {
            solver->new_vars(vars-solver->nVars());
        }
    } else {
        std::cerr
        << "PARSE ERROR! Unexpected char: '" << *in
        << "' in the header, at line " << lineNum+1
        << endl;
        std::exit(3);
    }
}

void DimacsParser::parseSolveComment(StreamBuffer& in)
{
    vector<Lit> assumps;
    skipWhitespace(in);
    while(*in != ')') {
        int lit = parseInt(in);
        assumps.push_back(Lit(std::abs(lit)-1, lit < 0));
        skipWhitespace(in);
    }

    if (verbosity >= 2) {
        cout
        << "c -----------> Solver::solve() called (number: "
        << std::setw(3) << debugLibPart << ") with assumps :";
        for(Lit lit: assumps) {
            cout << lit << " ";
        }
        cout
        << "<-----------"
        << endl;
    }

    lbool ret = solver->solve(&assumps);

    write_solution_to_debuglib_file(ret);
    debugLibPart++;

    if (verbosity >= 6) {
        cout << "c Parsed Solver::solve()" << endl;
    }
}

void DimacsParser::write_solution_to_debuglib_file(const lbool ret) const
{
    //Open file for writing
    std::string s = "debugLibPart" + stringify(debugLibPart) +".output";
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

void DimacsParser::parseComments(StreamBuffer& in, const std::string& str)
{
    if (debugLib && str.substr(0, 13) == "Solver::solve") {
        parseSolveComment(in);
    } else if (debugLib && str == "Solver::new_var()") {
        solver->new_var();

        if (verbosity >= 6) {
            cout << "c Parsed Solver::new_var()" << endl;
        }
    } else if (debugLib && str == "Solver::new_vars(") {
        skipWhitespace(in);
        int n = parseInt(in);
        solver->new_vars(n);

        if (verbosity >= 6) {
            cout << "c Parsed Solver::new_vars( " << n << " )" << endl;
        }
    } else {
        if (verbosity >= 6) {
            cout
            << "didn't understand in CNF file comment line:"
            << "'c " << str << "'"
            << endl;
        }
    }
    skipLine(in);
}

void DimacsParser::parse_and_add_clause(StreamBuffer& in)
{
    lits.clear();
    readClause(in);
    skipLine(in);
    solver->add_clause(lits);
    norm_clauses_added++;
}

void DimacsParser::parse_and_add_xor_clause(StreamBuffer& in)
{
    lits.clear();
    readClause(in);
    skipLine(in);
    if (lits.empty())
        return;

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
}

void DimacsParser::parse_DIMACS_main(StreamBuffer& in)
{
    std::string str;

    for (;;) {
        skipWhitespace(in);
        switch (*in) {
        case EOF:
            return;
        case 'p':
            printHeader(in);
            skipLine(in);
            break;
        case 'c':
            ++in;
            parseString(in, str);
            parseComments(in, str);
            break;
        case 'x':
            ++in;
            parse_and_add_xor_clause(in);
            break;
        case '\n':
            std::cerr
            << "c WARNING: Empty line at line number " << lineNum+1
            << " -- this is not part of the DIMACS specifications. Ignoring."
            << endl;
            skipLine(in);
            break;
        default:
            parse_and_add_clause(in);
            break;
        }
    }
}

template <class T> void DimacsParser::parse_DIMACS(T input_stream)
{
    debugLibPart = 1;
    const uint32_t origNumVars = solver->nVars();

    StreamBuffer in(input_stream);
    parse_DIMACS_main(in);

    if (verbosity >= 1) {
        cout
        << "c -- clauses added: " << norm_clauses_added << endl
        << "c -- xor clauses added: " << xor_clauses_added << endl
        << "c -- vars added " << (solver->nVars() - origNumVars)
        << endl;
    }
}

#ifdef USE_ZLIB
template void DimacsParser::parse_DIMACS(gzFile input_stream);
#else
template void DimacsParser::parse_DIMACS(FILE* input_stream);
#endif
