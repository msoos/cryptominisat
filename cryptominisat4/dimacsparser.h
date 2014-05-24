/*****************************************************************************
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

File under MIT licence.
******************************************************************************/

#ifndef DIMACSPARSER_H
#define DIMACSPARSER_H

#include <string>
#include "streambuffer.h"
#include "cryptominisat.h"

#ifdef USE_ZLIB
#include <zlib.h>
#endif

using namespace CMSat;

class DimacsParser
{
    public:
        DimacsParser(SATSolver* solver, const bool debugLib, unsigned _verbosity);

        template <class T> void parse_DIMACS(T input_stream);

    private:
        void parse_DIMACS_main(StreamBuffer& in);
        void skipWhitespace(StreamBuffer& in);
        void skipLine(StreamBuffer& in);
        int32_t parseInt(StreamBuffer& in, uint32_t& len);
        void parseString(StreamBuffer& in, std::string& str);
        void readClause(StreamBuffer& in);
        void parse_and_add_clause(StreamBuffer& in);
        void parse_and_add_xor_clause(StreamBuffer& in);
        bool match(StreamBuffer& in, const char* str);
        void printHeader(StreamBuffer& in);
        void parseComments(StreamBuffer& in, const std::string str);
        std::string stringify(uint32_t x);
        void parseSolveComment(StreamBuffer& in);


        SATSolver* solver;
        const bool debugLib;
        unsigned verbosity;

        //Stat
        size_t lineNum;

        uint32_t debugLibPart; ///<printing partial solutions to debugLibPart1..N.output when "debugLib" is set to TRUE
        std::vector<Lit> lits; ///<To reduce temporary creation overhead
        size_t norm_clauses_added = 0;
        size_t xor_clauses_added = 0;
};

#endif //DIMACSPARSER_H
