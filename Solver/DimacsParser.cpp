/******************************************************************************************[Main.C]
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include "DimacsParser.h"
#include <sstream>
#include <iostream>
#include <iomanip>
using std::cout;
using std::endl;

#include "Solver.h"

#define MAX_NAMES_SIZE 1000

DimacsParser::DimacsParser(Solver* _solver, const bool _debugLib, const bool _debugNewVar, const bool _grouping, const bool _addAsLearnt):
    solver(_solver)
    , debugLib(_debugLib)
    , debugNewVar(_debugNewVar)
    , grouping(_grouping)
    , addAsLearnt(_addAsLearnt)
{}

/**
@brief Skips all whitespaces
*/
void DimacsParser::skipWhitespace(StreamBuffer& in)
{
    while ((*in >= 9 && *in <= 13) || *in == 32)
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
@brief Returns line until the end of line into "ret"
*/
void DimacsParser::untilEnd(StreamBuffer& in, char* ret)
{
    uint32_t sizeRead = 0;
    for (;sizeRead < MAX_NAMES_SIZE-1; sizeRead++) {
        if (*in == EOF || *in == '\0') return;
        if (*in == '\n') {
            return;
        }
        *ret = *in;
        ret++;
        *ret = '\0';
        ++in;
    }
}

/**
@brief Parses in an integer
*/
int DimacsParser::parseInt(StreamBuffer& in)
{
    int     val = 0;
    bool    neg = false;
    skipWhitespace(in);
    if      (*in == '-') neg = true, ++in;
    else if (*in == '+') ++in;
    if (*in < '0' || *in > '9') printf("PARSE ERROR! Unexpected char: %c\n", *in), exit(3);
    while (*in >= '0' && *in <= '9')
        val = val*10 + (*in - '0'),
              ++in;
    return neg ? -val : val;
}

std::string DimacsParser::stringify(uint32_t x)
{
    std::ostringstream o;
    o << x;
    return o.str();
}

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
void DimacsParser::readClause(StreamBuffer& in, vec<Lit>& lits)
{
    int     parsed_lit;
    Var     var;
    lits.clear();
    for (;;) {
        parsed_lit = parseInt(in);
        if (parsed_lit == 0) break;
        var = abs(parsed_lit)-1;
        if (!debugNewVar) {
            while (var >= solver->nVars()) solver->newVar();
        }
        lits.push( (parsed_lit > 0) ? Lit(var, false) : Lit(var, true) );
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
    if (match(in, "p cnf")) {
                int vars    = parseInt(in);
                int clauses = parseInt(in);
                if (solver->verbosity >= 1) {
                    printf("c |  Number of variables:  %-12d                                                   |\n", vars);
                    printf("c |  Number of clauses:    %-12d                                                   |\n", clauses);
                }
            } else {
                printf("PARSE ERROR! Unexpected char: %c\n", *in), exit(3);
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
    vec<Lit> lits;
    int group = 0;
    std::string str;
    uint32_t debugLibPart = 1;
    char name[MAX_NAMES_SIZE];
    uint32_t numLearntClause = 0;


    for (;;) {
        skipWhitespace(in);
        switch (*in) {
        case EOF:
            std::cout << "c Added " << std::setw(10) << numLearntClause << " learnt clauses" << std::endl;
            return;
        case 'p':
            printHeader(in);
            break;
        case 'c':
            ++in;
            parseString(in, str);
            if (str == "v" || str == "var") {
                int var = parseInt(in);
                skipWhitespace(in);
                if (var <= 0) cout << "PARSE ERROR! Var number must be a positive integer" << endl, exit(3);
                name[0] = '\0';
                untilEnd(in, name);
                solver->setVariableName(var-1, name);
            } else if (debugLib && str == "Solver::solve()") {
                lbool ret = solver->solve();
                std::string s = "debugLibPart" + stringify(debugLibPart) +".output";
                FILE* res = fopen(s.c_str(), "w");
                if (ret == l_True) {
                    fprintf(res, "SAT\n");
                    for (Var i = 0; i != solver->nVars(); i++)
                        if (solver->model[i] != l_Undef)
                            fprintf(res, "%s%s%d", (i==0)?"":" ", (solver->model[i]==l_True)?"":"-", i+1);
                        fprintf(res, " 0\n");
                } else if (ret == l_False) {
                    fprintf(res, "UNSAT\n");
                } else if (ret == l_Undef) {
                    assert(false);
                } else {
                    assert(false);
                }
                fclose(res);
                debugLibPart++;
            } else if (debugNewVar && str == "Solver::newVar()") {
                solver->newVar();
            } else {
                //printf("didn't understand in CNF file: 'c %s'\n", str.c_str());
                skipLine(in);
            }
            break;
        default:
            bool xor_clause = false;
            if ( *in == 'x') xor_clause = true, ++in;
            readClause(in, lits);
            skipLine(in);

            name[0] = '\0';

            if (!grouping) group++;
            else {
                if (*in != 'c') {
                    cout << "PARSE ERROR! Group must be present after earch clause ('c' missing after clause line)" << endl;
                    exit(3);
                }
                ++in;

                parseString(in, str);
                if (str != "g" && str != "group") {
                    cout << "PARSE ERROR! Group must be present after each clause('group' missing)!" << endl;
                    cout << "Instead of 'group' there was:" << str << endl;
                    exit(3);
                }

                group = parseInt(in);
                if (group <= 0) printf("PARSE ERROR! Group number must be a positive integer\n"), exit(3);

                skipWhitespace(in);
                untilEnd(in, name);
            }

            if (xor_clause) {
                bool xorEqualFalse = false;
                for (uint32_t i = 0; i < lits.size(); i++) {
                    xorEqualFalse ^= lits[i].sign();
                }
                solver->addXorClause(lits, xorEqualFalse, group, name);
                assert(!addAsLearnt);
            } else {
                if (!addAsLearnt) {
                    solver->addClause(lits, group, name);
                } else {
                    solver->addLearntClause(lits, 0, 0);
                    numLearntClause++;
                }
            }
            break;
        }
    }
}

#ifdef DISABLE_ZLIB
void DimacsParser::parse_DIMACS(FILE * input_stream)
#else
void DimacsParser::parse_DIMACS(gzFile input_stream)
#endif // DISABLE_ZLIB
{
    StreamBuffer in(input_stream);
    parse_DIMACS_main(in);
}
