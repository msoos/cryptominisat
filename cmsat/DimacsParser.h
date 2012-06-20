/***************************************************************************************[Solver.cc]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2009, Niklas Sorensson
Copyright (c) 2009-2012, Mate Soos

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

#ifndef DIMACSPARSER_H
#define DIMACSPARSER_H

#include <stdexcept>
#include <string>
#include "cmsat/SolverTypes.h"
#include "cmsat/constants.h"
#include "cmsat/StreamBuffer.h"
#include "cmsat/Vec.h"
#include "cmsat/constants.h"

#ifndef DISABLE_ZLIB
#include <zlib.h>
#endif // DISABLE_ZLIB

namespace CMSat {

class Solver;

class DimacsParseError : public std::runtime_error
{
    public:
        explicit DimacsParseError(const std::string& arg);
        virtual ~DimacsParseError() throw();
};

/**
@brief Parses up a DIMACS file that my be zipped
*/
class DimacsParser
{
    public:
        DimacsParser(Solver* solver, const bool debugLib, const bool debugNewVar, const bool grouping, const bool addAsLearnt = false);

        template <class T>
        void parse_DIMACS(T input_stream);

    private:
        void parse_DIMACS_main(StreamBuffer& in);
        void skipWhitespace(StreamBuffer& in);
        void skipLine(StreamBuffer& in);
        std::string untilEnd(StreamBuffer& in);
        int32_t parseInt(StreamBuffer& in, uint32_t& len) throw (DimacsParseError);
        float parseFloat(StreamBuffer& in) throw (DimacsParseError);
        void parseString(StreamBuffer& in, std::string& str);
        void readClause(StreamBuffer& in, vec<Lit>& lits) throw (DimacsParseError);
        void parseClauseParameters(StreamBuffer& in, bool& learnt, uint32_t& glue, float& miniSatAct);
        void readFullClause(StreamBuffer& in) throw (DimacsParseError);
        void readBranchingOrder(StreamBuffer& in);
        bool match(StreamBuffer& in, const char* str);
        void printHeader(StreamBuffer& in) throw (DimacsParseError);
        void parseComments(StreamBuffer& in, const std::string str) throw (DimacsParseError);
        std::string stringify(uint32_t x);


        Solver *solver;
        const bool debugLib;
        const bool debugNewVar;
        const bool grouping;
        const bool addAsLearnt;

        uint32_t debugLibPart; ///<printing partial solutions to debugLibPart1..N.output when "debugLib" is set to TRUE
        vec<Lit> lits; ///<To reduce temporary creation overhead
        uint32_t numLearntClauses; ///<Number of learnt non-xor clauses added
        uint32_t numNormClauses; ///<Number of non-learnt, non-xor claues added
        uint32_t numXorClauses; ///<Number of non-learnt xor clauses added
};

}

#endif //DIMACSPARSER_H
