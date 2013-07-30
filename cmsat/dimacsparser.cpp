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
#include "solver.h"

#ifdef VERBOSE_DEBUG
#define DEBUG_COMMENT_PARSING
#endif //VERBOSE_DEBUG

using namespace CMSat;
using std::vector;
using std::cout;
using std::endl;

//#define DEBUG_COMMENT_PARSING

DimacsParser::DimacsParser(
    Solver* _solver
    , const bool _debugLib
    , const bool _debugNewVar
):
    solver(_solver)
    , debugLib(_debugLib)
    , debugNewVar(_debugNewVar)
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
        exit(3);
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
void DimacsParser::readClause(StreamBuffer& in, vector<Lit>& lits)
{
    int32_t parsed_lit;
    Var     var;
    uint32_t len;
    lits.clear();
    for (;;) {
        parsed_lit = parseInt(in, len);
        if (parsed_lit == 0) break;
        var = abs(parsed_lit)-1;
        if (!debugNewVar) {
            if (var >= ((uint32_t)1)<<25) {
                cout
                << "ERROR! Variable requested is far too large: "
                << var << endl
                << "--> At line " << lineNum
                << endl;
                exit(-1);
            }

            while (var >= solver->nVars())
                solver->newVar();
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

/**
@brief Prints the data in "p cnf VARS CLAUSES" header in DIMACS

We don't actually do \b anything with these. It's just printed for user
happyness. However, I think it's useless to print it, since it might mislead
users to think that their headers are correct, even though a lot of headers are
completely wrong, thanks to MiniSat printing the header, but not checking it.
Not checking it is \b not a problem. The problem is printing it such that
people believe it's validated
*/
void DimacsParser::printHeader(StreamBuffer& in)
{
    uint32_t len;

    if (match(in, "p cnf")) {
        int vars    = parseInt(in, len);
        int clauses = parseInt(in, len);
        if (solver->getVerbosity() >= 1) {
            cout << "c -- header says num vars:   " << std::setw(12) << vars << endl;
            cout << "c -- header says num clauses:" <<  std::setw(12) << clauses << endl;
        }
    } else {
        cout
        << "PARSE ERROR! Unexpected char: '" << *in
        << "' in the header, at line " << lineNum
        << endl;
        exit(3);
    }
}

/**
@brief Parse up comment lines which could contain important information
*/
void DimacsParser::parseComments(StreamBuffer& in, const std::string str)
{
    uint32_t len;
    #ifdef DEBUG_COMMENT_PARSING
    cout << "Parsing comments" << endl;
    #endif //DEBUG_COMMENT_PARSING

    if (str == "v" || str == "var") {
        int var = parseInt(in, len);
        skipWhitespace(in);
        if (var <= 0) {
            cout
            << "PARSE ERROR! Var number (after 'c v' or 'c var'"
            << " must be a positive integer"
            << endl;

            exit(3);
        }
        std::string name = untilEnd(in);
        //solver->setVariableName(var-1, name.c_str());

        if (solver->getConf().verbosity >= 6) {
            cout << "c Parsed 'c var'" << endl;
        }
    } else if (debugLib && str == "Solver::solve()") {
        if (solver->getVerbosity()>= 2) {
            cout
            << "c -----------> Solver::solve() called (number: "
            << std::setw(3) << debugLibPart << ")"
            << "<-----------"
            << endl;
        }
        lbool ret = solver->solve();

        //Open file for writing
        std::string s = "debugLibPart" + stringify(debugLibPart) +".output";
        std::ofstream partFile;
        partFile.open(s.c_str());
        if (!partFile) {
            cout << "ERROR: Cannot open part file '" << s << "'";
            exit(-1);
        }

        //Output to part file the result
        if (ret == l_True) {
            partFile << "s SATISFIABLE" << endl;
            partFile << "v ";
            for (Var i = 0; i != solver->nVarsReal(); i++) {
                if (solver->model[i] != l_Undef)
                    partFile
                    << ((solver->model[i]==l_True) ? "" : "-")
                    << (i+1) <<  " ";
            }
            partFile << "0" << endl;;
        } else if (ret == l_False) {
            partFile << "s UNSAT" << endl;
        } else if (ret == l_Undef) {
            assert(false);
        } else {
            assert(false);
        }
        partFile.close();
        debugLibPart++;

        if (solver->getConf().verbosity >= 6) {
            cout << "c Parsed Solver::solve()" << endl;
        }
    } else if (debugNewVar && str == "Solver::newVar()") {
        solver->newVar();

        if (solver->getConf().verbosity >= 6) {
            cout << "c Parsed Solver::newVar()" << endl;
        }
    } else {
        if (solver->getConf().verbosity >= 6) {
            cout
            << "didn't understand in CNF file comment line:"
            << "'c " << str << "'"
            << endl;
        }
    }
    skipLine(in);
}

/**
@brief Parses clause parameters given as e.g. "c clause red yes"
*/
void DimacsParser::parseClauseParameters(
    StreamBuffer& in
    , bool& red
) {
    std::string str;

    //Parse in if we are a redundant clause or not
    ++in;
    parseString(in, str);
    if (str != "red") goto addTheClause;

    ++in;
    parseString(in, str);
    if (str == "yes") red = true;
    else if (str == "no") {
        red = false;
        goto addTheClause;
    } else {
        cout
        << "c WARNING parsed for 'red' instead of yes/no: '"
        << str << "'"
        << endl;

        goto addTheClause;
    }

    addTheClause:
    skipLine(in);
    return;
}

/**
@brief Parses in a clause and its optional attributes
*/
void DimacsParser::readFullClause(StreamBuffer& in)
{
    bool red = false;
    ClauseStats stats;
    stats.conflictNumIntroduced = 0;
    std::string str;
    bool needToParseComments = false;

    //Is it an XOR clause?
    if ( *in == 'x') {
        cout << "ERROR: Cannot read XOR clause!" << endl;
        exit(-1);
    }

    //read in the actual clause
    readClause(in, lits);
    skipLine(in);

    //Parse comments or parse clause type (redundant, glue value, etc.)
    if (*in == 'c') {
        ++in;
        parseString(in, str);
        if (str == "clause") {
            parseClauseParameters(in, red);
        } else {
            needToParseComments = true;
        }
    }

    if (red) {
        solver->addRedClause(lits, stats);
        numRedClauses++;
    } else {
        solver->addClause(lits);
        numNormClauses++;
    }

    if (needToParseComments) {
        #ifdef DEBUG_COMMENT_PARSING
        cout << "Need to parse comments:" << str << endl;
        #endif //DEBUG_COMMENT_PARSING
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
            //Skipping empty line, even though empty lines are kind of out-of-spec
            ++in;
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
    numRedClauses = 0;
    numNormClauses = 0;
    const uint32_t origNumVars = solver->nVars();

    StreamBuffer in(input_stream);
    parse_DIMACS_main(in);

    if (solver->getVerbosity() >= 1) {
        cout << "c -- clauses added: "
        << std::setw(12) << numRedClauses
        << " redundant "
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
