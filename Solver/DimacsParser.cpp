/*****************************************************************************
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

Original code by MiniSat authors are under an MIT licence.
Modifications for CryptoMiniSat are under GPLv3 licence.
******************************************************************************/

#include "DimacsParser.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <fstream>
#include "ThreadControl.h"

#ifdef VERBOSE_DEBUG
#define DEBUG_COMMENT_PARSING
#endif //VERBOSE_DEBUG

using std::vector;
using std::cout;
using std::endl;

//#define DEBUG_COMMENT_PARSING

DimacsParser::DimacsParser(ThreadControl* _control, const bool _debugLib, const bool _debugNewVar):
    control(_control)
    , debugLib(_debugLib)
    , debugNewVar(_debugNewVar)
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
    if      (*in == '-')
        neg = true, ++in;
    else if (*in == '+')
        ++in;

    if (*in < '0' || *in > '9') {
        cout << "PARSE ERROR! Unexpected char: " << *in << endl;
        exit(3);
    }
    while (*in >= '0' && *in <= '9') {
        lenParsed++;
        val = val*10 + (*in - '0'),
              ++in;
    }
    return neg ? -val : val;
}

float DimacsParser::parseFloat(StreamBuffer& in)
{
    uint32_t len;
    uint32_t main = parseInt(in, len);
    if (*in != '.') {
        cout << "PARSE ERROR! Float does not contain a dot! Instead it contains: " << *in << endl;
        exit(3);
    }
    ++in;
    uint32_t sub = parseInt(in, len);

    uint32_t exp = 1;
    for (uint32_t i = 0;i < len; i++) exp *= 10;
    return (float)main + ((float)sub/exp);
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
                cout << "ERROR! Variable requested is far too large: " << var << endl;
                exit(-1);
            }
            while (var >= control->nVars())
                control->newVar();
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
        if (control->getVerbosity() >= 1) {
            cout << "c -- header says num vars:   " << std::setw(12) << vars << endl;
            cout << "c -- header says num clauses:" <<  std::setw(12) << clauses << endl;
        }
    } else {
        cout << "PARSE ERROR! Unexpected char: " << *in << endl;
        exit(3);
    }
}

/**
@brief Parse up comment lines which could contain important information

In CryptoMiniSat we save quite a bit of information in the comment lines.
These need to be parsed up. This function achieves that. Informations that
can be given:
\li "c Solver::newVar() called" -- we execute Solver::newVar()
\li "c Solver::solve() called" -- we execute Solver::solve() and dump the
solution to debugLibPartX.out, where X is a number that starts with 1 and
increases to N, where N is the number of solve() instructions
\li variable names in the form of "c var VARNUM NAME"
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
        if (var <= 0) cout << "PARSE ERROR! Var number must be a positive integer" << endl, exit(3);
        std::string name = untilEnd(in);
        //control->setVariableName(var-1, name.c_str());

        #ifdef DEBUG_COMMENT_PARSING
        cout << "Parsed 'c var'" << endl;
        #endif //DEBUG_COMMENT_PARSING
    } else if (debugLib && str == "Solver::solve()") {
        lbool ret = control->solve();
        std::string s = "debugLibPart" + stringify(debugLibPart) +".output";
        std::ofstream partFile;
        partFile.open(s.c_str());
        if (ret == l_True) {
            partFile << "SAT" << endl;
            for (Var i = 0; i != control->nVars(); i++) {
                if (control->model[i] != l_Undef)
                    partFile
                    << ((control->model[i]==l_True) ? "" : "-")
                    << (i+1) <<  " ";
            }
            partFile << "0" << endl;;
        } else if (ret == l_False) {
            partFile << "UNSAT" << endl;
        } else if (ret == l_Undef) {
            assert(false);
        } else {
            assert(false);
        }
        partFile.close();
        debugLibPart++;

        #ifdef DEBUG_COMMENT_PARSING
        cout << "Parsed Solver::solve()" << endl;
        #endif //DEBUG_COMMENT_PARSING
    } else if (debugNewVar && str == "Solver::newVar()") {
        control->newVar();

        #ifdef DEBUG_COMMENT_PARSING
        cout << "Parsed Solver::newVar()" << endl;
        #endif //DEBUG_COMMENT_PARSING
    } else {
        #ifdef DEBUG_COMMENT_PARSING
        cout << "didn't understand in CNF file: 'c " << str << endl;
        #endif //DEBUG_COMMENT_PARSING
    }
    skipLine(in);
}

/**
@brief Parses clause parameters given as e.g. "c clause learnt yes glue 4 miniSatAct 5.2"
*/
void DimacsParser::parseClauseParameters(
    StreamBuffer& in
    , bool& learnt
    , ClauseStats& stats
) {
    std::string str;
    uint32_t len;

    //Parse in if we are a learnt clause or not
    ++in;
    parseString(in, str);
    if (str != "learnt") goto addTheClause;

    ++in;
    parseString(in, str);
    if (str == "yes") learnt = true;
    else if (str == "no") {
        learnt = false;
        goto addTheClause;
    }
    else {
        cout << "parsed in instead of yes/no: '" << str << "'" << endl;
        goto addTheClause;
    }

    //Parse in Glue value
    ++in;
    parseString(in, str);
    if (str != "glue") goto addTheClause;
    ++in;
    stats.glue = parseInt(in, len);

    addTheClause:
    skipLine(in);
    return;
}

/**
@brief Parses in a clause and its optional attributes

We might have lines like:
\li "c clause learnt yes glue 4 miniSatAct 5.2" which we need to parse up and
make the clause learnt.
\li Furthermore, we need to take care, since comments might mean orders like
"c Solver::newVar() called", which needs to be parsed with parseComments()
-- this, we delegate
*/
void DimacsParser::readFullClause(StreamBuffer& in)
{
    bool xor_clause = false;
    bool learnt = false;
    ClauseStats stats;
    std::string str;
    bool needToParseComments = false;

    //read in the actual clause
    if ( *in == 'x') xor_clause = true, ++in;
    readClause(in, lits);
    skipLine(in);

    //Parse comments or parse clause type (learnt, glue value, etc.)
    if (*in == 'c') {
        ++in;
        parseString(in, str);
        if (str == "clause") {
            parseClauseParameters(in, learnt, stats);
        } else {
            needToParseComments = true;
        }
    }

    if (xor_clause) {
        assert(false && "Cannot read XOR clause!");
    } else {
        if (learnt) {
            control->addLearntClause(lits, stats);
            numLearntClauses++;
        } else {
            control->addClause(lits);
            numNormClauses++;
        }
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

Parses in header, the clauses, and special comment lines that define clause
groups, clause group names, and variable names, plus it parses up special
comments that have to do with debugging Solver::newVar() and Solver::solve()
calls for library-debugging
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
    numLearntClauses = 0;
    numNormClauses = 0;
    const uint32_t origNumVars = control->nVars();

    StreamBuffer in(input_stream);
    parse_DIMACS_main(in);

    if (control->getVerbosity() >= 1) {
        cout << "c -- clauses added: "
        << std::setw(12) << numLearntClauses
        << " learnts, "
        << std::setw(12) << numNormClauses
        << " normals "
        << endl;

        cout << "c -- vars added " << std::setw(10) << (control->nVars() - origNumVars)
        << endl;
    }
}

#ifndef DISABLE_ZLIB
template void DimacsParser::parse_DIMACS(gzFile input_stream);
#else
template void DimacsParser::parse_DIMACS(FILE* input_stream);
#endif
