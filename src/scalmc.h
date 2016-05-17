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
#include <fstream>
#include <map>
struct SATCount {
    void clear()
    {
        SATCount tmp;
        *this = tmp;
    }
    uint32_t hashCount = 0;
    uint32_t cellSolCount = 0;
};

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
    bool ScalApproxMC(SATCount& count);
    bool ApproxMC(SATCount& count);
    bool AddHash(uint32_t num_xor_cls, vector<Lit>& assumps);
    bool SetHash(uint32_t clausNum, std::map<uint64_t,Lit>& hashVars, vector<Lit>& assumps);

    int64_t BoundedSATCount(uint32_t maxSolutions, const vector<Lit>& assumps);
    lbool BoundedSAT(
        uint32_t maxSolutions, uint32_t minSolutions
        , vector<Lit>& assumptions
        , std::map<std::string, uint32_t>& solutionMap
        , uint32_t* solutionCount
    );
    string GenerateRandomBits(uint32_t size);
    uint32_t SolutionsToReturn(uint32_t minSolutions);
    int GenerateRandomNum(int maxRange);
    bool printSolutions();
    void seed_random_engine();

    //config
    int onlyCount = true;
    std::string cuspLogFile = "cusp_log.txt";

    double startTime;
    std::map< std::string, std::vector<uint32_t>> globalSolutionMap;
    bool openLogFile();
    std::atomic<bool> must_interrupt;
    vector<uint32_t> independent_vars;
    void call_after_parse(const vector<uint32_t>& independent_vars) override;
    void set_up_timer();

    uint32_t samples = 1;
    uint32_t callsPerSolver = 0;
    uint32_t startIteration = 0;
    uint32_t pivotApproxMC = 60;
    uint32_t pivotUniGen = 27;
    uint32_t samplesGen  = 1;
    uint32_t tApproxMC = 1;
    uint32_t searchMode = 1;
    int      totalTimeout = 72000;
    int      loopTimeout = 2500;
    double   kappa = 0.638;
    bool     multisample = true;
    bool     aggregateSolutions = true;
    std::ofstream cusp_logf;
    std::mt19937 randomEngine;



    /// SAMPLING/UNIGEN
    void generate_samples();
    uint32_t UniGen(
        uint32_t samples
        , uint32_t sampleCounter
        , std::map< string, uint32_t >& solutionMap
        , uint32_t* lastSuccessfulHashOffset, double timeReference
    );
    int uniGenCall(uint32_t samples
                   , uint32_t sampleCounter
                   , std::map<std::string, uint32_t>& solutionMap
                   , uint32_t* lastSuccessfulHashOffset
                   , double timeReference
                  );
};


#endif //CUSP_H_
