#ifndef DIMACSPARSER_H
#define DIMACSPARSER_H

#include <string>
#include "SolverTypes.h"
#include "constants.h"
#include "StreamBuffer.h"
#include "Vec.h"

#ifdef _MSC_VER
#include <msvc/stdint.h>
#else
#include <stdint.h>
#endif //_MSC_VER

#ifndef DISABLE_ZLIB
#include <zlib.h>
#endif // DISABLE_ZLIB

#ifdef STATS_NEEDED
#include "Logger.h"
#endif //STATS_NEEDED

class Solver;

class DimacsParser
{
    public:
        DimacsParser(Solver* solver, const bool debugLib, const bool debugNewVar, const bool grouping, const bool addAsLearnt = false);

        #ifdef DISABLE_ZLIB
        void parse_DIMACS(FILE * input_stream);
        #else
        void parse_DIMACS(gzFile input_stream);
        #endif // DISABLE_ZLIB

    private:
        template<class B>
        void parse_DIMACS_main(B& in);

        template<class B>
        void skipWhitespace(B& in);

        template<class B>
        void skipLine(B& in);

        template<class B>
        void untilEnd(B& in, char* ret);

        template<class B>
        int parseInt(B& in);

        std::string stringify(uint32_t x);

        template<class B>
        void parseString(B& in, std::string& str);

        template<class B>
        void readClause(B& in, vec<Lit>& lits);

        template<class B>
        bool match(B& in, const char* str);

        Solver *solver;
        const bool debugLib;
        const bool debugNewVar;
        const bool grouping;
        const bool addAsLearnt;
};

#endif //DIMACSPARSER_H
