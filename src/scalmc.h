/*
 CUSP and ScalMC

 Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 Copyright (c) 2014, Supratik Chakraborty, Kuldeep S. Meel, Moshe Y. Vardi
 Copyright (c) 2015, Supratik Chakraborty, Daniel J. Fremont,
 Kuldeep S. Meel, Sanjit A. Seshia, Moshe Y. Vardi

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
 */


#ifndef CUSP_H_
#define CUSP_H_

#include "main.h"
#include <fstream>
#include <random>
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
    void SetHash(uint32_t clausNum, std::map<uint64_t,Lit>& hashVars, vector<Lit>& assumps);

    int64_t BoundedSATCount(uint32_t maxSolutions, const vector<Lit>& assumps);
    lbool BoundedSAT(
        uint32_t maxSolutions, uint32_t minSolutions
        , vector<Lit>& assumptions
        , std::map<std::string, uint32_t>& solutionMap
        , uint32_t* solutionCount
    );
    string GenerateRandomBits(uint32_t size);

    //config
    std::string cuspLogFile = "cusp_log.txt";

    double startTime;
    std::map< std::string, std::vector<uint32_t>> globalSolutionMap;
    bool openLogFile();
    std::atomic<bool> must_interrupt;
    void call_after_parse() override;

    uint32_t startIteration = 0;
    uint32_t pivotApproxMC = 52;
    uint32_t tApproxMC = 17;
    uint32_t searchMode = 1;
    double   loopTimeout = 2500;
    int      unset_vars = 0;
    std::ofstream cusp_logf;
    std::mt19937 randomEngine;
};


#endif //CUSP_H_
