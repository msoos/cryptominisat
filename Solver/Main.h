/*****************************************************************************
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

Original code by MiniSat authors are under an MIT licence.
Modifications for CryptoMiniSat are under GPLv3 licence.
******************************************************************************/

#ifndef MAIN_H
#define MAIN_H

#include <string>
#include <vector>
#include <memory>

#ifndef DISABLE_ZLIB
#include <zlib.h>
#endif // DISABLE_ZLIB

#include "SolverTypes.h"
#include "SolverConf.h"

class ThreadControl;

using std::string;

class Main
{
    public:
        Main(int argc, char** argv);

        void parseCommandLine();
        int solve();

    private:

        ThreadControl* control;

        //File reading
        void readInAFile(const std::string& filename);
        void readInStandardInput();
        void parseInAllFiles();

        //Helper functions
        void printResultFunc(const lbool ret);
        void printVersionInfo();
        int correctReturnValue(const lbool ret) const;

        //Config
        SolverConf conf;
        int numThreads;
        bool debugLib;
        bool debugNewVar;
        int printResult;

        //Multi-start solving
        uint32_t max_nr_of_solutions;
        int doBanFoundSolution;

        //Files to read & write
        bool fileNamePresent;
        std::vector<std::string> filesToRead;

        //Command line arguments
        int argc;
        char** argv;
};

#endif //MAIN_H
