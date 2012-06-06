/*
This file is part of CryptoMiniSat2.

CryptoMiniSat2 is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

CryptoMiniSat2 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CryptoMiniSat2.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MAIN_H
#define MAIN_H

#include <string>
#include <vector>
#ifndef DISABLE_ZLIB
#include <zlib.h>
#endif // DISABLE_ZLIB

#include "Solver.h"
#include "SharedData.h"

namespace CMSat {

using std::string;

class Main
{
    public:
        Main(int argc, char** argv);

        void parseCommandLine();

        int singleThreadSolve();
        int oneThreadSolve();
        int multiThreadSolve();

        int numThreads;

    private:

        void printUsage(char** argv);
        const char* hasPrefix(const char* str, const char* prefix);
        void printResultFunc(const Solver& S, const lbool ret, FILE* res);

        //File reading
        void readInAFile(const std::string& filename, Solver& solver);
        void readInStandardInput(Solver& solver);
        void parseInAllFiles(Solver& solver);
        FILE* openOutputFile();

        void setDoublePrecision(const uint32_t verbosity);
        void printVersionInfo(const uint32_t verbosity);
        int correctReturnValue(const lbool ret) const;

        SolverConf conf;
        GaussConf gaussconfig;

        bool grouping;
        bool debugLib;
        bool debugNewVar;
        bool printResult;
        uint32_t max_nr_of_solutions;
        bool fileNamePresent;
        bool twoFileNamesPresent;
        std::vector<std::string> filesToRead;

        SharedData sharedData;

        int argc;
        char** argv;
};

}

#endif //MAIN_H
