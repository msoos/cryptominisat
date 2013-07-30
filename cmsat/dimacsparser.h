/*****************************************************************************
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

Original code by MiniSat authors are under an MIT licence.
Modifications for CryptoMiniSat are under GPLv3 licence.
******************************************************************************/

#ifndef DIMACSPARSER_H
#define DIMACSPARSER_H

#include <string>
#include "solvertypes.h"
#include "constants.h"
#include "streambuffer.h"
#include "vec.h"
#include "constants.h"
#include "clause.h"

#ifdef USE_ZLIB
#include <zlib.h>
#endif

namespace CMSat {

class Solver;

/**
@brief Parses up a DIMACS file that my be zipped
*/
class DimacsParser
{
    public:
        DimacsParser(Solver* solver, const bool debugLib, const bool debugNewVar);

        template <class T> void parse_DIMACS(T input_stream);

    private:
        void parse_DIMACS_main(StreamBuffer& in);
        void skipWhitespace(StreamBuffer& in);
        void skipLine(StreamBuffer& in);
        std::string untilEnd(StreamBuffer& in);
        int32_t parseInt(StreamBuffer& in, uint32_t& len);
        void parseString(StreamBuffer& in, std::string& str);
        void readClause(StreamBuffer& in, std::vector<Lit>& lits);
        void parseClauseParameters(
            StreamBuffer& in
            , bool& red
        );
        void readFullClause(StreamBuffer& in);
        bool match(StreamBuffer& in, const char* str);
        void printHeader(StreamBuffer& in);
        void parseComments(StreamBuffer& in, const std::string str);
        std::string stringify(uint32_t x);


        Solver *solver;
        const bool debugLib;
        const bool debugNewVar;

        //Stat
        size_t lineNum;

        uint32_t debugLibPart; ///<printing partial solutions to debugLibPart1..N.output when "debugLib" is set to TRUE
        std::vector<Lit> lits; ///<To reduce temporary creation overhead
        uint32_t numRedClauses; ///<Number of redundant non-xor clauses added
        uint32_t numNormClauses; ///<Number of irred, non-xor claues added
};

}

#endif //DIMACSPARSER_H
