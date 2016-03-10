/*
 * CUSP
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 * Copyright (c) 2014, Supratik Chakraborty, Kuldeep S. Meel, Moshe Y. Vardi
 * Copyright (c) 2015, Supratik Chakraborty, Daniel J. Fremont,
 * Kuldeep S. Meel, Sanjit A. Seshia, Moshe Y. Vardi
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.0 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/

#ifndef CUSP_H_
#define CUSP_H_

#include "main.h"

class CUSP: public Main {
public:
    CUSP(int argc, char** argv):
        Main(argc, argv)
        , approxMCOptions("ApproxMC options")
    {
        must_interrupt.store(false, std::memory_order_relaxed);
    }
    int solve() override;
    void add_supported_options() override;

    po::options_description approxMCOptions;

private:
    void add_approxmc_options();

    SATCount ApproxMC(
        SATSolver* solver
        , FILE* resLog
        , std::mt19937& randomEngine
    );

    uint32_t UniGen(
        uint32_t samples
        , SATSolver* solver
        , FILE* resLog, uint32_t sampleCounter
        , std::mt19937& randomEngine
        , std::map< string, uint32_t >& solutionMap
        , uint32_t* lastSuccessfulHashOffset, double timeReference
    );

    bool AddHash(uint32_t clausNum, SATSolver* s, vector<Lit>& assumptions
                 , std::mt19937& randomEngine);
    int32_t BoundedSATCount(uint32_t maxSolutions, SATSolver* solver
                            , vector<Lit>& assumptions
                           );
    lbool BoundedSAT(
        uint32_t maxSolutions, uint32_t minSolutions, SATSolver* solver
        , vector<Lit>& assumptions, std::mt19937& randomEngine
        , std::map<std::string, uint32_t>& solutionMap
        , uint32_t* solutionCount
    );
    bool GenerateRandomBits(string& randomBits
                            , uint32_t size
                            , std::mt19937& randomEngine
                           );
    uint32_t SolutionsToReturn(uint32_t minSolutions);
    int GenerateRandomNum(int maxRange, std::mt19937& randomEngine);
    bool printSolutions(FILE* res);
    void SeedEngine(std::mt19937& randomEngine);
    int singleThreadUniGenCall(uint32_t samples
                               , FILE* resLog, uint32_t sampleCounter
                               , std::map<std::string, uint32_t>& solutionMap
                               , std::mt19937& randomEngine
                               , uint32_t* lastSuccessfulHashOffset
                               , double timeReference
                              );

    //config
    bool onlyCount = true;
    std::string cuspLogFile = "mylog.txt";

    double startTime;
    std::map< std::string, std::vector<uint32_t>> globalSolutionMap;
    bool openLogFile(FILE*& res);
    std::atomic<bool> must_interrupt;
};


#endif //CUSP_H_
