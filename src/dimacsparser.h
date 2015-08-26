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

#ifndef DIMACSPARSER_H
#define DIMACSPARSER_H

#include <string>
#include "streambuffer.h"
#include "cryptominisat4/cryptominisat.h"

#ifdef USE_ZLIB
#include <zlib.h>
#endif

using namespace CMSat;
using std::vector;

class DimacsParser
{
    public:
        DimacsParser(SATSolver* solver, const bool debugLib, unsigned _verbosity);

        template <class T> void parse_DIMACS(T input_stream);

    private:
        void parse_DIMACS_main(StreamBuffer& in);
        void skipWhitespace(StreamBuffer& in);
        void skipLine(StreamBuffer& in);
        int32_t parseInt(StreamBuffer& in);
        void parseString(StreamBuffer& in, std::string& str);
        void readClause(StreamBuffer& in);
        void parse_and_add_clause(StreamBuffer& in);
        void parse_and_add_xor_clause(StreamBuffer& in);
        bool match(StreamBuffer& in, const char* str);
        void printHeader(StreamBuffer& in);
        void parseComments(StreamBuffer& in, const std::string& str);
        std::string stringify(uint32_t x) const;
        void parseSolveComment(StreamBuffer& in);
        void write_solution_to_debuglib_file(const lbool ret) const;


        SATSolver* solver;
        const bool debugLib;
        unsigned verbosity;

        //Stat
        size_t lineNum;

        //Printing partial solutions to debugLibPart1..N.output when "debugLib" is set to TRUE
        uint32_t debugLibPart = 1;

        //Reduce temp overhead
        vector<Lit> lits;
        vector<Var> vars;

        size_t norm_clauses_added = 0;
        size_t xor_clauses_added = 0;
};

#endif //DIMACSPARSER_H
