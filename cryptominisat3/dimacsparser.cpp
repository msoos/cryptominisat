/*****************************************************************************
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

Original code by MiniSat authors are under an MIT licence.
Modifications for CryptoMiniSat are under GPLv3 licence.
******************************************************************************/

#include "dimacsparser.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <fstream>
#include <complex>
#include "assert.h"

#ifdef VERBOSE_DEBUG
#define DEBUG_COMMENT_PARSING
#endif //VERBOSE_DEBUG

using std::vector;
using std::cout;
using std::endl;

//#define DEBUG_COMMENT_PARSING

DimacsParser::DimacsParser(
    MainSolver* _solver
    , const bool _debugLib
):
    solver(_solver)
    , debugLib(_debugLib)
    , lineNum(0)
{}

/**
@brief Skips all whitespaces
*/
void DimacsParser::skipWhitespace(StreamBuffer& in)
{
    while ((*in >= 9 && *in <= 13 && *in != 10) || *in == 32)
        ++in;
}

/**
@brief Skips until the end of the line
*/
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

/**
@brief Parses in an integer
*/
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
        val = val*10 + (*in - '0'),
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

/**
@brief Reads in a clause and puts it in lit
@p[out] lits
*/
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
        if (solver->get_conf().verbosity >= 1) {
            cout << "c -- header says num vars:   " << std::setw(12) << vars << endl;
            cout << "c -- header says num clauses:" <<  std::setw(12) << clauses << endl;
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

    if (solver->get_conf().verbosity >= 2) {
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

    if (solver->get_conf().verbosity >= 6) {
        cout << "c Parsed Solver::solve()" << endl;
    }
}

void DimacsParser::parseComments(StreamBuffer& in, const std::string str)
{
    if (debugLib && str.substr(0, 13) == "Solver::solve") {
        parseSolveComment(in);
    } else if (debugLib && str == "Solver::newVar()") {
        solver->new_var();

        if (solver->get_conf().verbosity >= 6) {
            cout << "c Parsed Solver::newVar()" << endl;
        }
    } else {
        if (solver->get_conf().verbosity >= 6) {
            cout
            << "didn't understand in CNF file comment line:"
            << "'c " << str << "'"
            << endl;
        }
    }
    skipLine(in);
}

/**
@brief Parses in a clause and its optional attributes
*/
void DimacsParser::readFullClause(StreamBuffer& in)
{
    if ( *in == 'x') {
        cout << "ERROR: Cannot read XOR clause!" << endl;
        std::exit(-1);
    }

    lits.clear();
    readClause(in);
    skipLine(in);
    solver->add_clause(lits);
    numNormClauses++;

    if (*in == 'c') {
        ++in;
        std::string str;
        parseString(in, str);
        parseComments(in, str);
    }
}

/**
@brief The main function: parses in a full DIMACS file
*/
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
        case '\n':
            cout
            << "c WARNING: Empty line at line number " << lineNum+1
            << " -- this is not part of the DIMACS specifications. Ignoring."
            << endl;
            skipLine(in);
            break;
        default:
            readFullClause(in);
            break;
        }
    }
}

template <class T> void DimacsParser::parse_DIMACS(T input_stream)
{
    debugLibPart = 1;
    numNormClauses = 0;
    const uint32_t origNumVars = solver->nVars();

    StreamBuffer in(input_stream);
    parse_DIMACS_main(in);

    if (solver->get_conf().verbosity >= 1) {
        cout << "c -- clauses added: "
        << std::setw(12) << numNormClauses
        << " irredundant"
        << endl;

        cout << "c -- vars added " << std::setw(10) << (solver->nVars() - origNumVars)
        << endl;
    }
}

#ifdef USE_ZLIB
template void DimacsParser::parse_DIMACS(gzFile input_stream);
#else
template void DimacsParser::parse_DIMACS(FILE* input_stream);
#endif
