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

#if defined(__GNUC__) && defined(__linux__)

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <fenv.h>
#endif

#include <stdio.h>
#include <ctime>
#include <cstring>
#include <errno.h>
#include <string.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#include <fstream>
#include <sys/stat.h>
#include <string.h>
#include <list>
#include <array>
#include <cmath>

#include "scalmc.h"
#include "time_mem.h"
#include "dimacsparser.h"
#include "cryptominisat5/cryptominisat.h"
#include "signalcode.h"

using std::cout;
using std::cerr;
using std::endl;
using boost::lexical_cast;
using std::list;
using std::map;

string binary(unsigned x, uint32_t length)
{
    uint32_t logSize = (x == 0 ? 1 : log2(x) + 1);
    string s;
    do {
        s.push_back('0' + (x & 1));
    } while (x >>= 1);

    for (uint32_t i = logSize; i < length; i++) {
        s.push_back('0');
    }
    std::reverse(s.begin(), s.end());

    return s;
}

string CUSP::GenerateRandomBits(uint32_t size)
{
    string randomBits;
    std::uniform_int_distribution<unsigned> uid {0, 2147483647U};
    uint32_t i = 0;
    while (i < size) {
        i += 31;
        randomBits += binary(uid(randomEngine), 31);
    }
    return randomBits;
}

void CUSP::add_approxmc_options()
{
    approxMCOptions.add_options()
    ("pivotAC", po::value(&pivotApproxMC)->default_value(pivotApproxMC)
        , "Number of solutions to check for")
    ("mode", po::value(&searchMode)->default_value(searchMode)
        ,"Seach mode. ApproxMX = 0, ScalMC = 1")
    ("tApproxMC", po::value(&tApproxMC)->default_value(tApproxMC)
        , "Number of measurements")
    ("startIteration", po::value(&startIteration)->default_value(startIteration), "")
    ("looptout", po::value(&loopTimeout)->default_value(loopTimeout)
        , "Timeout for one measurement, consisting of finding pivotAC solutions")
    ("cuspLogFile", po::value(&cuspLogFile)->default_value(cuspLogFile),"")
    ("unset", po::value(&unset_vars)->default_value(unset_vars),
     "Try to ask the solver to unset some independent variables, thereby"
     "finding more than one solution at a time")
    ;

    help_options_simple.add(approxMCOptions);
    help_options_complicated.add(approxMCOptions);
}

void CUSP::add_supported_options()
{
    Main::add_supported_options();
    add_approxmc_options();
}

void print_xor(const vector<uint32_t>& vars, const uint32_t rhs)
{
    cout << "Added XOR ";
    for (size_t i = 0; i < vars.size(); i++) {
        cout << vars[i]+1;
        if (i < vars.size()-1) {
            cout << " + ";
        }
    }
    cout << " = " << (rhs ? "True" : "False") << endl;
}

bool CUSP::openLogFile()
{
    cusp_logf.open(cuspLogFile.c_str());
    if (!cusp_logf.is_open()) {
        cout << "Cannot open CUSP log file '" << cuspLogFile
             << "' for writing." << endl;
        exit(1);
    }
    return true;
}

template<class T>
inline T findMedian(vector<T>& numList)
{
    std::sort(numList.begin(), numList.end());
    size_t medIndex = (numList.size() + 1) / 2;
    size_t at = 0;
    if (medIndex >= numList.size()) {
        at += numList.size() - 1;
        return numList[at];
    }
    at += medIndex;
    return numList[at];
}

template<class T>
inline T findMin(vector<T>& numList)
{
    T min = std::numeric_limits<T>::max();
    for (const auto a: numList) {
        if (a < min) {
            min = a;
        }
    }
    return min;
}

bool CUSP::AddHash(uint32_t num_xor_cls, vector<Lit>& assumps)
{
    string randomBits = GenerateRandomBits((independent_vars.size() + 1) * num_xor_cls);
    bool rhs = true;
    vector<uint32_t> vars;

    for (uint32_t i = 0; i < num_xor_cls; i++) {
        //new activation variable
        solver->new_var();
        uint32_t act_var = solver->nVars()-1;
        assumps.push_back(Lit(act_var, true));

        vars.clear();
        vars.push_back(act_var);
        rhs = (randomBits[(independent_vars.size() + 1) * i] == '1');

        for (uint32_t j = 0; j < independent_vars.size(); j++) {
            if (randomBits[(independent_vars.size() + 1) * i + j+1] == '1') {
                vars.push_back(independent_vars[j]);
            }
        }
        solver->add_xor_clause(vars, rhs);
        if (conf.verbosity >= 3) {
            print_xor(vars, rhs);
        }
    }
    return true;
}

int64_t CUSP::BoundedSATCount(uint32_t maxSolutions, const vector<Lit>& assumps)
{
    cout << "BoundedSATCount looking for " << maxSolutions << " solutions" << endl;

    //Set up things for adding clauses that can later be removed
    solver->new_var();
    uint32_t act_var = solver->nVars()-1;
    vector<Lit> new_assumps(assumps);
    new_assumps.push_back(Lit(act_var, true));

    double start_time = cpuTime();
    uint64_t solutions = 0;
    lbool ret;
    while (solutions < maxSolutions) {
        //solver->set_max_confl(10*1000*1000);
        double this_iter_timeout = loopTimeout-(cpuTime()-start_time);
        solver->set_timeout_all_calls(this_iter_timeout);
        ret = solver->solve(&new_assumps);
        if (ret != l_True)
            break;

        size_t num_undef = 0;
        if (solutions < maxSolutions) {
            vector<Lit> lits;
            lits.push_back(Lit(act_var, false));
            for (const uint32_t var: independent_vars) {
                if (solver->get_model()[var] != l_Undef) {
                    lits.push_back(Lit(var, solver->get_model()[var] == l_True));
                } else {
                    num_undef++;
                }
            }
            solver->add_clause(lits);
        }
        if (num_undef) {
            cout << "WOW Num undef:" << num_undef << endl;
        }

        //Try not to be crazy, 2**30 solutions is enough
        if (num_undef <= 30) {
            solutions += 1U << num_undef;
        } else {
            solutions += 1U << 30;
            cout << "WARNING, in this cut there are > 2**30 solutions indicated by the solver!" << endl;
        }
    }
    if (solutions > maxSolutions) {
        solutions = maxSolutions;
    }

    //Remove clauses added
    vector<Lit> cl_that_removes;
    cl_that_removes.push_back(Lit(act_var, false));
    solver->add_clause(cl_that_removes);

    //Timeout
    if (ret == l_Undef) {
        must_interrupt.store(false, std::memory_order_relaxed);
        return -1;
    }
    return solutions;
}

bool CUSP::ApproxMC(SATCount& count)
{
    count.clear();
    int64_t currentNumSolutions = 0;
    vector<uint64_t> numHashList;
    vector<int64_t> numCountList;
    vector<Lit> assumps;
    for (uint32_t j = 0; j < tApproxMC; j++) {
        uint64_t hashCount;
        uint32_t repeatTry = 0;
        for (hashCount = 0; hashCount < solver->nVars(); hashCount++) {
            cout << "-> Hash Count " << hashCount << endl;
            double myTime = cpuTimeTotal();
            currentNumSolutions = BoundedSATCount(pivotApproxMC + 1, assumps);

            //cout << currentNumSolutions << ", " << pivotApproxMC << endl;
            cusp_logf << "ApproxMC:" << searchMode << ":"
                      << j << ":" << hashCount << ":"
                      << std::fixed << std::setprecision(2) << (cpuTimeTotal() - myTime) << ":"
                      << (int)(currentNumSolutions == (pivotApproxMC + 1)) << ":"
                      << currentNumSolutions << endl;
            //Timeout!
            if (currentNumSolutions < 0) {
                //Remove all hashes
                assumps.clear();

                if (repeatTry < 2) {    /* Retry up to twice more */
                    assert(hashCount > 0);
                    AddHash(hashCount, assumps); //add new set of hashes
                    solver->simplify(&assumps);
                    hashCount --;
                    repeatTry += 1;
                    cout << "Timeout, try again -- " << repeatTry << endl;
                } else {
                    //this set of hashes does not work, go up
                    AddHash(hashCount + 1, assumps);
                    solver->simplify(&assumps);
                    cout << "Timeout, moving up" << endl;
                }
                continue;
            }

            if (currentNumSolutions < pivotApproxMC + 1) {
                //less than pivotApproxMC solutions
                break;
            }

            //Found all solutions needed
            AddHash(1, assumps);
        }
        assumps.clear();
        numHashList.push_back(hashCount);
        numCountList.push_back(currentNumSolutions);
        solver->simplify(&assumps);
    }
    if (numHashList.size() == 0) {
        //UNSAT
        return true;
    }

    auto minHash = findMin(numHashList);
    auto hash_it = numHashList.begin();
    auto cnt_it = numCountList.begin();
    for (; hash_it != numHashList.end() && cnt_it != numCountList.end()
            ; hash_it++, cnt_it++
        ) {
        *cnt_it *= pow(2, (*hash_it) - minHash);
    }
    int medSolCount = findMedian(numCountList);

    count.cellSolCount = medSolCount;
    count.hashCount = minHash;
    return true;
}

int CUSP::solve()
{
    conf.reconfigure_at = 0;
    conf.reconfigure_val = 15;
    conf.gaussconf.max_matrix_rows = 3000;
    conf.gaussconf.decision_until = 3000;
    conf.gaussconf.max_num_matrixes = 1;
    conf.gaussconf.min_matrix_rows = 5;
    conf.gaussconf.autodisable = false;

    //set seed
    assert(vm.count("random"));
    unsigned int seed = vm["random"].as<unsigned int>();
    randomEngine.seed(seed);

    openLogFile();
    startTime = cpuTimeTotal();

    solver = new SATSolver((void*)&conf, &must_interrupt);
    solverToInterrupt = solver;
    if (dratf) {
        cout
                << "ERROR: Gauss does NOT work with DRAT and Gauss is needed for CUSP. Exiting."
                << endl;
        exit(-1);
    }
    //check_num_threads_sanity(num_threads);
    //solver->set_num_threads(num_threads);
    if (unset_vars) {
        solver->set_greedy_undef();
    }
    printVersionInfo();
    parseInAllFiles(solver);

    if (startIteration > independent_vars.size()) {
        cout << "ERROR: Manually-specified startIteration"
             "is larger than the size of the independent set.\n" << endl;
        return -1;
    }

    SATCount solCount;
    cout << "Using start iteration " << startIteration << endl;

    bool finished = false;
    if (searchMode == 0) {
        finished = ApproxMC(solCount);
    } else {
        finished = ScalApproxMC(solCount);
    }
    cout << "ApproxMC finished in " << (cpuTimeTotal() - startTime) << " s" << endl;
    if (!finished) {
        cout << " (TIMED OUT)" << endl;
        return 0;
    }

    if (solCount.hashCount == 0 && solCount.cellSolCount == 0) {
        cout << "The input formula is unsatisfiable." << endl;
        return correctReturnValue(l_False);
    }

    if (conf.verbosity) {
        solver->print_stats();
    }

    cout << "Number of solutions is: " << solCount.cellSolCount
         << " x 2^" << solCount.hashCount << endl;

    return correctReturnValue(l_True);
}

int main(int argc, char** argv)
{
    #if defined(__GNUC__) && defined(__linux__)
    feenableexcept(FE_INVALID   |
                   FE_DIVBYZERO |
                   FE_OVERFLOW
                  );
    #endif

    #ifndef USE_GAUSS
    std::cerr << "CUSP only makes any sese to run if you have configured with:" << endl
              << "*** cmake -DUSE_GAUSS=ON (.. or .)  ***" << endl
              << "Refusing to run. Please reconfigure and then re-compile." << endl;
    exit(-1);
    #else

    CUSP main(argc, argv);
    main.conf.verbStats = 1;
    main.parseCommandLine();
    return main.solve();
    #endif
}

void CUSP::call_after_parse()
{
    if (independent_vars.empty()) {
        cout
        << "c WARNING! No independent vars were set using 'c ind var1 [var2 var3 ..] 0'"
        "notation in the CNF." << endl
        << " c ScalMC may work substantially worse!" << endl;
        for (size_t i = 0; i < solver->nVars(); i++) {
            independent_vars.push_back(i);
        }
    }
    solver->set_independent_vars(&independent_vars);
}

//For ScalApproxMC only
void CUSP::SetHash(uint32_t clausNum, std::map<uint64_t,Lit>& hashVars, vector<Lit>& assumps)
{
    if (clausNum < assumps.size()) {
        uint64_t numberToRemove = assumps.size()- clausNum;
        for (uint64_t i = 0; i<numberToRemove; i++) {
            assumps.pop_back();
        }
    } else {
        if (clausNum > assumps.size() && assumps.size() < hashVars.size()) {
            for (uint32_t i = assumps.size(); i< hashVars.size() && i < clausNum; i++) {
                assumps.push_back(hashVars[i]);
            }
        }
        if (clausNum > hashVars.size()) {
            AddHash(clausNum-hashVars.size(),assumps);
            for (uint64_t i = hashVars.size(); i < clausNum; i++) {
                hashVars[i] = assumps[i];
            }
        }
    }
}

//For ScalApproxMC only
bool CUSP::ScalApproxMC(SATCount& count)
{
    count.clear();
    vector<uint64_t> numHashList;
    vector<int64_t> numCountList;
    vector<Lit> assumps;

    uint64_t hashCount = startIteration;
    uint64_t hashPrev = 0;
    uint64_t mPrev = 0;

    double myTime = cpuTimeTotal();
    if (hashCount == 0) {
        int64_t currentNumSolutions = BoundedSATCount(pivotApproxMC+1,assumps);
        cusp_logf << "ApproxMC:"<< searchMode<<":"<<"0:0:"
                  << std::fixed << std::setprecision(2) << (cpuTimeTotal() - myTime) << ":"
                  << (int)(currentNumSolutions == (pivotApproxMC + 1)) << ":"
                  << currentNumSolutions << endl;

        //Din't find at least pivotApproxMC+1
        if (currentNumSolutions <= pivotApproxMC) {
            count.cellSolCount = currentNumSolutions;
            count.hashCount = 0;
            return true;
        }
        hashCount++;
    }

    for (uint32_t j = 0; j < tApproxMC; j++) {
        map<uint64_t,int64_t> countRecord;
        map<uint64_t,uint32_t> succRecord;
        map<uint64_t,Lit> hashVars; //map assumption var to XOR hash

        uint32_t repeatTry = 0;
        uint64_t numExplored = 1;
        uint64_t lowerFib = 0, upperFib = independent_vars.size();

        while (numExplored < independent_vars.size()) {
            cout << "Num Explored: " << numExplored
                 << " ind set size: " << independent_vars.size() << endl;
            myTime = cpuTimeTotal();
            uint64_t swapVar = hashCount;
            SetHash(hashCount,hashVars,assumps);
            cout << "Number of XOR hashes active: " << hashCount << endl;
            int64_t currentNumSolutions = BoundedSATCount(pivotApproxMC + 1, assumps);

            //cout << currentNumSolutions << ", " << pivotApproxMC << endl;
            cusp_logf << "ApproxMC:" << searchMode<<":"
                      << j << ":" << hashCount << ":"
                      << std::fixed << std::setprecision(2) << (cpuTimeTotal() - myTime) << ":"
                      << (int)(currentNumSolutions == (pivotApproxMC + 1)) << ":"
                      << currentNumSolutions << endl;
            //Timeout!
            if (currentNumSolutions < 0) {
                //Remove all hashes
                assumps.clear();
                hashVars.clear();

                if (repeatTry < 2) {    /* Retry up to twice more */
                    assert(hashCount > 0);
                    SetHash(hashCount,hashVars,assumps);
                    solver->simplify(&assumps);
                    hashCount --;
                    repeatTry += 1;
                    cout << "Timeout, try again -- " << repeatTry << endl;
                } else {
                    //this set of hashes does not work, go up
                    SetHash(hashCount + 1, hashVars, assumps);
                    solver->simplify(&assumps);
                    cout << "Timeout, moving up" << endl;
                }
                hashCount = swapVar;
                continue;
            }

            if (currentNumSolutions < pivotApproxMC + 1) {
                numExplored = lowerFib+independent_vars.size()-hashCount;
                if (succRecord.find(hashCount-1) != succRecord.end()
                    && succRecord[hashCount-1] == 1
                ) {
                    numHashList.push_back(hashCount);
                    numCountList.push_back(currentNumSolutions);
                    mPrev = hashCount;
                    //less than pivotApproxMC solutions
                    break;
                }
                succRecord[hashCount] = 0;
                countRecord[hashCount] = currentNumSolutions;
                if (abs(hashCount-mPrev) <= 2 && mPrev != 0) {
                    upperFib = hashCount;
                    hashCount--;
                } else {
                    if (hashPrev > hashCount) {
                        hashPrev = 0;
                    }
                    upperFib = hashCount;
                    if (hashPrev > lowerFib) {
                        lowerFib = hashPrev;
                    }
                    hashCount = (upperFib+lowerFib)/2;
                }
            } else {
                assert(currentNumSolutions == pivotApproxMC+1);

                numExplored = hashCount + independent_vars.size()-upperFib;
                if (succRecord.find(hashCount+1) != succRecord.end()
                    && succRecord[hashCount+1] == 0
                ) {
                    numHashList.push_back(hashCount+1);
                    numCountList.push_back(countRecord[hashCount+1]);
                    mPrev = hashCount+1;
                    break;
                }
                succRecord[hashCount] = 1;
                if (abs(hashCount - mPrev) < 2 && mPrev!=0) {
                    lowerFib = hashCount;
                    hashCount ++;
                } else if (lowerFib + (hashCount - lowerFib)*2 >= upperFib-1) {
                    lowerFib = hashCount;
                    hashCount = (lowerFib+upperFib)/2;
                } else {
                    //printf("hashPrev:%d hashCount:%d\n",hashPrev, hashCount);
                    hashCount = lowerFib + (hashCount -lowerFib)*2;
                }
            }
            hashPrev = swapVar;
        }
        assumps.clear();
        solver->simplify(&assumps);
        hashCount =mPrev;
    }
    if (numHashList.size() == 0) {
        //UNSAT
        return true;
    }

    auto minHash = findMin(numHashList);
    auto cnt_it = numCountList.begin();
    for (auto hash_it = numHashList.begin()
        ; hash_it != numHashList.end() && cnt_it != numCountList.end()
        ; hash_it++, cnt_it++
    ) {
        *cnt_it *= pow(2, (*hash_it) - minHash);
    }
    int medSolCount = findMedian(numCountList);

    count.cellSolCount = medSolCount;
    count.hashCount = minHash;
    return true;
}
