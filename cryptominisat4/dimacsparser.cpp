/*****************************************************************************
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

File under MIT licence
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
    while ((*in >= 9 && *in <= 13 && *in != 10) || *in == 32)
        ++in;
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

/**
@brief Returns line until the end of line
*/
std::string DimacsParser::untilEnd(StreamBuffer& in)
{
    std::string ret;

    while(*in != EOF && *in != '\0' && *in != '\n') {
        ret += *in;
        ++in;
    }

    return ret;
}

int32_t DimacsParser::parseInt(StreamBuffer& in, uint32_t& lenParsed)
{
    lenParsed = 0;
    int32_t val = 0;
    bool    neg = false;
    skipWhitespace(in);
    if (*in == '-') {
        neg = true;
        ++in;
    } else if (*in == '+') {
        ++in;
    }

    if (*in < '0' || *in > '9') {
        cout
        << "PARSE ERROR! Unexpected char (dec: '" << (char)*in << ")"
        << " At line " << lineNum
        << " we expected a number"
        << endl;
        std::exit(3);
    }
    while (*in >= '0' && *in <= '9') {
        lenParsed++;
        val = val*10 + (*in - '0');
        ++in;
    }
    return neg ? -val : val;
}

std::string DimacsParser::stringify(uint32_t x)
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
    while (*in != ' ' && *in != '\n') {
        str += *in;
        ++in;
    }
}

void DimacsParser::readClause(StreamBuffer& in)
{
    int32_t parsed_lit;
    uint32_t var;
    uint32_t len;
    for (;;) {
        parsed_lit = parseInt(in, len);
        if (parsed_lit == 0) break;
        var = abs(parsed_lit)-1;
        if (var >= (1ULL<<28)) {
            cout
            << "ERROR! Variable requested is far too large: "
            << var << endl
            << "--> At line " << lineNum+1
            << endl;
            std::exit(-1);
        }

        while (var >= solver->nVars()) {
            solver->new_var();
        }
        lits.push_back( (parsed_lit > 0) ? Lit(var, false) : Lit(var, true) );
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
    uint32_t len;

    if (match(in, "p cnf")) {
        int vars    = parseInt(in, len);
        int clauses = parseInt(in, len);
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
            solver->new_vars(vars-solver->nVars()+1);
        }
    } else {
        cout
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
        uint32_t len = 0;
        int lit = parseInt(in, len);
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

    //Open file for writing
    std::string s = "debugLibPart" + stringify(debugLibPart) +".output";
    std::ofstream partFile;
    partFile.open(s.c_str());
    if (!partFile) {
        cout << "ERROR: Cannot open part file '" << s << "'";
        std::exit(-1);
    }

    //Output to part file the result
    if (ret == l_True) {
        partFile << "s SATISFIABLE" << endl;
        partFile << "v ";
        for (uint32_t i = 0; i != solver->nVars(); i++) {
            if (solver->get_model()[i] != l_Undef)
                partFile
                << ((solver->get_model()[i]==l_True) ? "" : "-")
                << (i+1) <<  " ";
        }
        partFile << "0" << endl;
    } else if (ret == l_False) {
        partFile << "s UNSAT" << endl;
    } else if (ret == l_Undef) {
        cout << "c timeout, exiting" << endl;
        std::exit(15);
    } else {
        assert(false);
    }
    partFile.close();
    debugLibPart++;

    if (verbosity >= 6) {
        cout << "c Parsed Solver::solve()" << endl;
    }
}

void DimacsParser::parseComments(StreamBuffer& in, const std::string str)
{
    if (debugLib && str.substr(0, 13) == "Solver::solve") {
        parseSolveComment(in);
    } else if (debugLib && str == "Solver::new_var()") {
        solver->new_var();

        if (verbosity >= 6) {
            cout << "c Parsed Solver::new_var()" << endl;
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
    vector<Var> vars;
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
            cout
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
