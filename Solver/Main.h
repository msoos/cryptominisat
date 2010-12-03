/*****************************************************************************
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

Original code by MiniSat authors are under an MIT licence.
Modifications for CryptoMiniSat are under GPLv3 licence.
******************************************************************************/

#ifndef MAIN_H
#define MAIN_H

#include <string>
using std::string;
#include <vector>
#ifndef DISABLE_ZLIB
#include <zlib.h>
#endif // DISABLE_ZLIB

#include "MTSolver.h"
#include "SharedData.h"

class Main
{
    public:
        Main(int argc, char** argv);

        void parseCommandLine();

        const int solve();

    private:

        void printUsage(char** argv);
        const char* hasPrefix(const char* str, const char* prefix);
        void printResultFunc(const MTSolver& S, const lbool ret, FILE* res);

        //File reading
        void readInAFile(const std::string& filename, MTSolver& solver);
        void readInStandardInput(MTSolver& solver);
        void parseInAllFiles(MTSolver& solver);
        FILE* openOutputFile();

        void setDoublePrecision(const uint32_t verbosity);
        void printVersionInfo(const uint32_t verbosity);
        int correctReturnValue(const lbool ret) const;

        SolverConf conf;
        GaussConf gaussconfig;
        int numThreads;

        bool grouping;
        bool debugLib;
        bool debugNewVar;
        bool printResult;
        uint32_t max_nr_of_solutions;
        bool fileNamePresent;
        bool twoFileNamesPresent;
        std::vector<std::string> filesToRead;

        int argc;
        char** argv;
};

#endif //MAIN_H
