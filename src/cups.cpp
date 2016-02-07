/*
 * CUPS
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

#if defined(__GNUC__) && defined(__linux__)

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <fenv.h>
#endif

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
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <list>
#include <array>
#include <time.h>

#include "cups.h"
#include "time_mem.h"
#include "dimacsparser.h"
#include "cryptominisat4/cryptominisat.h"
#include "signalcode.h"

using std::cout;
using std::cerr;
using std::endl;
using boost::lexical_cast;
using std::list;
using std::map;

timer_t *mytimer;
bool *timerSetFirstTime;
void start_timer(int num) {
    struct sigevent sev;
    sev.sigev_signo = SIGUSR1;
    sev.sigev_notify = SIGEV_SIGNAL;
    //printf("start_timer;thread:%d\n", threadNum);
    struct itimerspec value;
    value.it_value.tv_sec = num; //waits for n seconds before sending timer signal
    value.it_value.tv_nsec = 0;
    value.it_interval.tv_sec = 0; //exipire once
    value.it_interval.tv_nsec = 0;
    if (*timerSetFirstTime){
        timer_create(CLOCK_REALTIME, &sev, mytimer);
        //timer_delete(mytimer);
    }
    *timerSetFirstTime = false;
    timer_settime(*mytimer, 0, &value, NULL);
}
void SIGINT_handler_exit(int) {
  _exit(1);
}

void SIGALARM_handler(int /*sig*/, siginfo_t *si, void */*uc*/) {
    int num = si->si_value.sival_int;
    SATSolver* solver = solverToInterrupt;
    if (!redDumpFname.empty() || !irredDumpFname.empty() || need_clean_exit) {
        solver->interrupt_asap();
    } else {
        if (solver->nVars() > 0) {
            solver->add_in_partial_solving_stats();
            solver->print_stats();
        }
        _exit(1);
    }
}


string binary(unsigned x, uint32_t length)
{
    uint32_t logSize = (x == 0 ? 1 : log2(x) + 1);
    string s;
    do {
        s.push_back('0' + (x & 1));
    } while (x >>= 1);
    for (uint32_t i = logSize; i < (uint32_t) length; i++) {
        s.push_back('0');
    }
    std::reverse(s.begin(), s.end());

    return s;

}

bool CUPS::GenerateRandomBits(string& randomBits, uint32_t size, std::mt19937& randomEngine)
{
    std::uniform_int_distribution<unsigned> uid {0, 2147483647};
    uint32_t i = 0;
    while (i < size) {
        i += 31;
        randomBits += binary(uid(randomEngine), 31);
    }
    return true;
}
int CUPS::GenerateRandomNum(int maxRange, std::mt19937& randomEngine)
{
    std::uniform_int_distribution<int> uid {0, maxRange};
    return uid(randomEngine);
}

/* Number of solutions to return from one invocation of UniGen2 */
uint32_t CUPS::SolutionsToReturn(
    uint32_t minSolutions
) {
    if (conf.multisample) {
        return minSolutions;
    } else {
        return 1;
    }
}
bool CUPS::AddHash(uint32_t numClaus, SATSolver* solver, vector<Lit>& assumptions, std::mt19937& randomEngine)
{
    string randomBits;
    GenerateRandomBits(randomBits, (independent_vars.size() + 1) * numClaus, randomEngine);
    bool rhs = true;
    uint32_t activationVar;
    vector<uint32_t> vars;

    for (uint32_t i = 0; i < numClaus; i++) {
        vars.clear();
        solver->new_var();
        activationVar = solver->nVars()-1;
        assumptions.push_back(Lit(activationVar, true));
        vars.push_back(activationVar);
        rhs = (randomBits[(independent_vars.size() + 1) * i] == 1);

        for (uint32_t j = 0; j < independent_vars.size(); j++) {
            if (randomBits[(independent_vars.size() + 1) * i + j] == '1') {
                vars.push_back(independent_vars[j]);
            }
        }
        solver->add_xor_clause(vars, rhs);
    }
    return true;
}

int32_t CUPS::BoundedSATCount(uint32_t maxSolutions, SATSolver* solver, vector<Lit>& assumptions)
{
    unsigned long current_nr_of_solutions = 0;
    lbool ret = l_True;
    solver->new_var();
    uint32_t activationVar = solver->nVars()-1;
    vector<Lit> allSATAssumptions(assumptions);
    allSATAssumptions.push_back(Lit(activationVar, true));

    //signal(SIGALRM, SIGALARM_handler);
    start_timer(conf.loopTimeout);
    while (current_nr_of_solutions < maxSolutions && ret == l_True) {
        ret = solver->solve(&allSATAssumptions);
        current_nr_of_solutions++;
        if (ret == l_True && current_nr_of_solutions < maxSolutions) {
            vector<Lit> lits;
            lits.push_back(Lit(activationVar, false));
            for (uint32_t j = 0; j < independent_vars.size(); j++) {
                uint32_t var = independent_vars[j];
                if (solver->get_model()[var] != l_Undef) {
                    lits.push_back(Lit(var, (solver->get_model()[var] == l_True) ? true : false));
                }
            }
            solver->add_clause(lits);
        }
    }
    vector<Lit> cls_that_removes;
    cls_that_removes.push_back(Lit(activationVar, false));
    solver->add_clause(cls_that_removes);
    if (ret == l_Undef) {
        must_interrupt.store(false, std::memory_order_relaxed);
        return -1 * current_nr_of_solutions;
    }
    return current_nr_of_solutions;
}

lbool CUPS::BoundedSAT(
    uint32_t maxSolutions
    , uint32_t minSolutions
    , SATSolver* solver
    , vector<Lit>& assumptions
    , std::mt19937& randomEngine
    , std::map<string, uint32_t>& solutionMap
    , uint32_t* solutionCount
) {
    unsigned long current_nr_of_solutions = 0;
    lbool ret = l_True;
    solver->new_var();
    uint32_t activationVar = solver->nVars()-1;
    vector<Lit> allSATAssumptions(assumptions);
    allSATAssumptions.push_back(Lit(activationVar, true));

    std::vector<vector<lbool>> modelsSet;
    vector<lbool> model;
    //signal(SIGALRM, SIGALARM_handler);
    start_timer(conf.loopTimeout);
    while (current_nr_of_solutions < maxSolutions && ret == l_True) {
        ret = solver->solve(&allSATAssumptions);
        current_nr_of_solutions++;

        if (ret == l_True && current_nr_of_solutions < maxSolutions) {
            vector<Lit> lits;
            lits.push_back(Lit(activationVar, false));
            model.clear();
            model = solver->get_model();
            modelsSet.push_back(model);
            for (uint32_t j = 0; j < independent_vars.size(); j++) {
                uint32_t var = independent_vars[j];
                if (solver->get_model()[var] != l_Undef) {
                    lits.push_back(Lit(var, (solver->get_model()[var] == l_True) ? true : false));
                }
            }
            solver->add_clause(lits);
        }
    }
    *solutionCount = modelsSet.size();
    cout << "current_nr_of_solutions:" << current_nr_of_solutions << endl;
    vector<Lit> cls_that_removes;
    cls_that_removes.push_back(Lit(activationVar, false));
    solver->add_clause(cls_that_removes);
    if (ret == l_Undef) {
        must_interrupt.store(false, std::memory_order_relaxed);
        return ret;
    }

    if (current_nr_of_solutions < maxSolutions && current_nr_of_solutions > minSolutions) {
        std::vector<int> modelIndices;
        for (uint32_t i = 0; i < modelsSet.size(); i++) {
            modelIndices.push_back(i);
        }
        std::shuffle(modelIndices.begin(), modelIndices.end(), randomEngine);
        uint32_t var;
        uint32_t numSolutionsToReturn = SolutionsToReturn(minSolutions);
        for (uint32_t i = 0; i < numSolutionsToReturn; i++) {
            vector<lbool> model = modelsSet.at(modelIndices.at(i));
            string solution ("v");
            for (uint32_t j = 0; j < independent_vars.size(); j++) {
                var = independent_vars[j];
                if (model[var] != l_Undef) {
                    if (model[var] != l_True) {
                        solution += "-";
                    }
                    solution += std::to_string(var + 1);
                    solution += " ";
                }
            }
            solution += "0";

            std::map<string, uint32_t>::iterator it = solutionMap.find(solution);
            if (it == solutionMap.end()) {
                solutionMap[solution] = 0;
            }
            solutionMap[solution] += 1;
        }
        return l_True;

    }

    return l_False;
}

template<class T>
inline double findMean(T numList)
{
    assert(!numList.empty());

    double sum = 0;
    for (const auto a: numList) {
        sum += a;
    }
    return (sum * 1.0 / (double)numList.size());
}

template<class T>
inline double findMedian(T numList)
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
inline int findMin(T numList)
{
    int min = std::numeric_limits<int>::max();
    for (const auto a: numList) {
        if (a < min) {
            min = a;
        }
    }
    return min;
}

SATCount CUPS::ApproxMC(SATSolver* solver, FILE* resLog, std::mt19937& randomEngine)
{
    int32_t currentNumSolutions = 0;
    vector<int> numHashList;
    vector<int> numCountList;
    vector<Lit> assumptions;
    double elapsedTime = 0;
    int repeatTry = 0;
    for (uint32_t j = 0; j < conf.tApproxMC; j++) {
        uint32_t  hashCount;
        for (hashCount = 0; hashCount < solver->nVars(); hashCount++) {
            elapsedTime = cpuTimeTotal() - startTime;
            if (elapsedTime > conf.totalTimeout - 3000) {
                break;
            }
            double myTime = cpuTimeTotal();
            currentNumSolutions = BoundedSATCount(conf.pivotApproxMC + 1, solver, assumptions);

            myTime = cpuTimeTotal() - myTime;
            //printf("%f\n", myTime);
            //printf("%d %d\n",currentNumSolutions,conf.pivotApproxMC);
            if (conf.verbosity >= 2) {
                fprintf(resLog, "ApproxMC:%d:%d:%f:%d:%d\n", j, hashCount, myTime,
                        (currentNumSolutions == (int32_t)(conf.pivotApproxMC + 1)), currentNumSolutions);
                fflush(resLog);
            }
            if (currentNumSolutions <= 0) {
                assumptions.clear();
                if (repeatTry < 2) {    /* Retry up to twice more */
                    AddHash(hashCount, solver, assumptions, randomEngine);
                    hashCount --;
                    repeatTry += 1;
                } else {
                    AddHash(hashCount + 1, solver, assumptions, randomEngine);
                    repeatTry = 0;
                }
                continue;
            }
            if (currentNumSolutions == conf.pivotApproxMC + 1) {
                AddHash(1, solver, assumptions, randomEngine);
            } else {
                break;
            }

        }
        assumptions.clear();
        if (elapsedTime > conf.totalTimeout - 3000) {
            break;
        }
        numHashList.push_back(hashCount);
        numCountList.push_back(currentNumSolutions);
    }
    if (numHashList.size() == 0) {
        return SATCount();
    }
    int minHash = findMin(numHashList);
    for (auto it1 = numHashList.begin(), it2 = numCountList.begin()
        ;it1 != numHashList.end() && it2 != numCountList.end()
        ;it1++, it2++
    ) {
        (*it2) *= pow(2, (*it1) - minHash);
    }
    int medSolCount = findMedian(numCountList);

    SATCount solCount;
    solCount.cellSolCount = medSolCount;
    solCount.hashCount = minHash;
    return solCount;
}

uint32_t CUPS::UniGen(
    uint32_t samples
    , SATSolver* solver
    , FILE* resLog
    , uint32_t sampleCounter
    , std::mt19937& randomEngine
    , std::map<string, uint32_t>& solutionMap
    , uint32_t* lastSuccessfulHashOffset
    , double timeReference
) {
    lbool ret = l_False;
    uint32_t i, solutionCount, currentHashCount, lastHashCount, currentHashOffset, hashOffsets[3];
    int hashDelta;
    vector<Lit> assumptions;
    double elapsedTime = 0;
    int repeatTry = 0;
    for (i = 0; i < samples; i++) {
        sampleCounter ++;
        ret = l_False;

        hashOffsets[0] = *lastSuccessfulHashOffset;   /* Start at last successful hash offset */
        if (hashOffsets[0] == 0) {  /* Starting at q-2; go to q-1 then q */
            hashOffsets[1] = 1;
            hashOffsets[2] = 2;
        } else if (hashOffsets[0] == 2) { /* Starting at q; go to q-1 then q-2 */
            hashOffsets[1] = 1;
            hashOffsets[2] = 0;
        }
        repeatTry = 0;
        lastHashCount = 0;
        for (uint32_t j = 0; j < 3; j++) {
            currentHashOffset = hashOffsets[j];
            currentHashCount = currentHashOffset + conf.startIteration;
            hashDelta = currentHashCount - lastHashCount;

            if (hashDelta > 0) { /* Add new hash functions */
                AddHash(hashDelta, solver, assumptions, randomEngine);
            } else if (hashDelta < 0) { /* Remove hash functions */
                assumptions.clear();
                AddHash(currentHashCount, solver, assumptions, randomEngine);
            }
            lastHashCount = currentHashCount;

            double currentTime = cpuTimeTotal();
            elapsedTime = currentTime - startTime;
            if (elapsedTime > conf.totalTimeout - 3000) {
                break;
            }
            uint32_t maxSolutions = (uint32_t) (1.41 * (1 + conf.kappa) * conf.pivotUniGen + 2);
            uint32_t minSolutions = (uint32_t) (conf.pivotUniGen / (1.41 * (1 + conf.kappa)));
            ret = BoundedSAT(maxSolutions + 1, minSolutions, solver, assumptions, randomEngine, solutionMap, &solutionCount);
            if (conf.verbosity >= 2) {
                fprintf(resLog, "UniGen2:%d:%d:%f:%d:%d\n", sampleCounter, currentHashCount, cpuTimeTotal() - timeReference, (ret == l_False ? 1 : (ret == l_True ? 0 : 2)), solutionCount);
                fflush(resLog);
            }
            if (ret == l_Undef) {   /* SATSolver timed out; retry current hash count at most twice more */
                assumptions.clear();    /* Throw out old hash functions */
                if (repeatTry < 2) {    /* Retry current hash count with new hash functions */
                    AddHash(currentHashCount, solver, assumptions, randomEngine);
                    j--;
                    repeatTry += 1;
                } else {     /* Go on to next hash count */
                    lastHashCount = 0;
                    if ((j == 0) && (currentHashOffset == 1)) { /* At q-1, and need to pick next hash count */
                        /* Somewhat arbitrarily pick q-2 first; then q */
                        hashOffsets[1] = 0;
                        hashOffsets[2] = 2;
                    }
                    repeatTry = 0;
                }
                continue;
            }
            if (ret == l_True) {    /* Number of solutions in correct range */
                *lastSuccessfulHashOffset = currentHashOffset;
                break;
            } else { /* Number of solutions too small or too large */
                if ((j == 0) && (currentHashOffset == 1)) { /* At q-1, and need to pick next hash count */
                    if (solutionCount < minSolutions) {
                        /* Go to q-2; next will be q */
                        hashOffsets[1] = 0;
                        hashOffsets[2] = 2;
                    } else {
                        /* Go to q; next will be q-2 */
                        hashOffsets[1] = 2;
                        hashOffsets[2] = 0;
                    }
                }
            }
        }
        if (ret != l_True) {
            i --;
        }
        assumptions.clear();
        if (elapsedTime > conf.totalTimeout - 3000) {
            break;
        }
    }
    return sampleCounter;
}

int CUPS::singleThreadUniGenCall(
    uint32_t samples
    , FILE* resLog
    , uint32_t sampleCounter
    , std::map<string, uint32_t>& solutionMap
    , std::mt19937& randomEngine
    , uint32_t* lastSuccessfulHashOffset
    , double timeReference
) {
    SATSolver solver2(&conf);
    solverToInterrupt = &solver2;

    parseInAllFiles(&solver2);
    sampleCounter = UniGen(
        samples
        , &solver2
        , resLog
        , sampleCounter
        , randomEngine
        , solutionMap
        , lastSuccessfulHashOffset
        , timeReference
    );
    return sampleCounter;
}

void CUPS::SeedEngine(std::mt19937& randomEngine)
{
    /* Initialize PRNG with seed from random_device */
    std::random_device rd {};
    std::array<int, 10> seedArray;
    std::generate_n(seedArray.data(), seedArray.size(), std::ref(rd));
    std::seed_seq seed(std::begin(seedArray), std::end(seedArray));
    randomEngine.seed(seed);
}

bool CUPS::openLogFile(FILE*& res)
{
    if (false) {
        return false;
    }

    string suffix, logFileName;
    for (int i = 0; i < 1; i++) {
        suffix = "_";
        suffix.append(std::to_string(i).append(".txt"));
        logFileName = "mylog";
        res = fopen(logFileName.append(suffix).c_str(), "wb");
        if (res == NULL) {
            int backup_errno = errno;
            printf("Cannot open %s for writing. Problem: %s\n", logFileName.append(suffix).c_str(), strerror(backup_errno));
            exit(1);
        }
    }
    return true;
}

//My stuff from OneThreadSolve
int CUPS::solve()
{
    conf.reconfigure_at = 0;
    conf.reconfigure_val = 7;

    FILE* resLog;
    openLogFile(resLog);
    startTime = cpuTimeTotal();

    solver = new SATSolver((void*)&conf, &must_interrupt);
    solverToInterrupt = solver;
    if (dratf) {
        solver->set_drat(dratf, false);
    }
    //check_num_threads_sanity(num_threads);
    //solver->set_num_threads(num_threads);
    printVersionInfo();
    parseInAllFiles(solver);

    int numThreads = 1;
    mytimer = new timer_t;
    timerSetFirstTime = new bool;
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = SIGALARM_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);
    *timerSetFirstTime = true;
    need_clean_exit = true;

    if (conf.startIteration > independent_vars.size()) {
        cout << "ERROR: Manually-specified startIteration"
        "is larger than the size of the independent set.\n" << endl;
        return -1;
    }
    if (conf.startIteration == 0) {
        cout << "Computing startIteration using ApproxMC" << endl;

        SATCount solCount;
        std::mt19937 randomEngine {};
        SeedEngine(randomEngine);
        solCount = ApproxMC(solver, resLog, randomEngine);
        double elapsedTime = cpuTimeTotal() - startTime;
        cout << "Completed ApproxMC at " << elapsedTime << " s";
        if (elapsedTime > conf.totalTimeout - 3000) {
            cout << " (TIMED OUT)" << endl;
            return 0;
        }
        cout << endl;
        //printf("Solution count estimate is %d * 2^%d\n", solCount.cellSolCount, solCount.hashCount);
        if (solCount.hashCount == 0 && solCount.cellSolCount == 0) {
            cout << "The input formula is unsatisfiable." << endl;
            return 0;
        }
        conf.startIteration = round(solCount.hashCount + log2(solCount.cellSolCount) +
                                    log2(1.8) - log2(conf.pivotUniGen)) - 2;
    } else {
        cout << "Using manually-specified startIteration" << endl;
    }

    uint32_t maxSolutions = (uint32_t) (1.41 * (1 + conf.kappa) * conf.pivotUniGen + 2);
    uint32_t minSolutions = (uint32_t) (conf.pivotUniGen / (1.41 * (1 + conf.kappa)));
    uint32_t samplesPerCall = SolutionsToReturn(minSolutions);
    uint32_t callsNeeded = (conf.samples + samplesPerCall - 1) / samplesPerCall;
    printf("loThresh %d, hiThresh %d, startIteration %d\n", minSolutions, maxSolutions, conf.startIteration);
    printf("Outputting %d solutions from each UniGen2 call\n", samplesPerCall);
    uint32_t numCallsInOneLoop = 0;
    if (conf.callsPerSolver == 0) {
        numCallsInOneLoop = std::min(solver->nVars() / (conf.startIteration * 14), callsNeeded);
        if (numCallsInOneLoop == 0) {
            numCallsInOneLoop = 1;
        }
    } else {
        numCallsInOneLoop = conf.callsPerSolver;
        cout << "Using manually-specified callsPerSolver" << endl;
    }

    uint32_t numCallLoops = callsNeeded / numCallsInOneLoop;
    uint32_t remainingCalls = callsNeeded % numCallsInOneLoop;

    cout << "Making " << numCallLoops << " loops."
    << " calls per loop: " << numCallsInOneLoop
    << " remaining: " << remainingCalls << endl;
    bool timedOut = false;
    uint32_t sampleCounter = 0;
    std::map<string, uint32_t> threadSolutionMap;
    double allThreadsTime = 0;
    uint32_t allThreadsSampleCount = 0;
    double threadStartTime = cpuTimeTotal();

    std::mt19937 randomEngine {};
    SeedEngine(randomEngine);

    uint32_t lastSuccessfulHashOffset = 0;
    lbool ret = l_True;

    /* Perform extra UniGen calls that don't fit into the loops */
    if (remainingCalls > 0) {
        sampleCounter = singleThreadUniGenCall(
            remainingCalls, resLog, sampleCounter
            , threadSolutionMap, randomEngine
            , &lastSuccessfulHashOffset, threadStartTime);
    }

    /* Perform main UniGen call loops */
    for (uint32_t i = 0; i < numCallLoops; i++) {
        if (!timedOut) {
            sampleCounter = singleThreadUniGenCall(
                numCallsInOneLoop, resLog, sampleCounter, threadSolutionMap
                , randomEngine, &lastSuccessfulHashOffset, threadStartTime
            );

            if ((cpuTimeTotal() - threadStartTime) > conf.totalTimeout - 3000) {
                timedOut = true;
            }
        }
    }

    for (map<string, uint32_t>::iterator itt = threadSolutionMap.begin()
        ; itt != threadSolutionMap.end()
        ; itt++
    ) {
        string solution = itt->first;
        map<string, std::vector<uint32_t>>::iterator itg = globalSolutionMap.find(solution);
        if (itg == globalSolutionMap.end()) {
            globalSolutionMap[solution] = std::vector<uint32_t>(1, 0);
        }
        globalSolutionMap[solution][0] += itt->second;
        allThreadsSampleCount += itt->second;
    }

    double timeTaken = cpuTimeTotal() - threadStartTime;
    allThreadsTime += timeTaken;
    cout
    << "Total time for UniGen2 thread " << 1
    << ": " << timeTaken << " s"
    << (timedOut ? " (TIMED OUT)" : "")
    << endl;

    cout << "Total time for all UniGen2 calls: " << allThreadsTime << " s" << endl;
    cout << "Samples generated: " << allThreadsSampleCount << endl;

    if (conf.verbosity >= 1) {
        solver->print_stats();
    }

    return correctReturnValue(ret);
}

int main(int argc, char** argv)
{
    #if defined(__GNUC__) && defined(__linux__)
    feenableexcept(FE_INVALID   |
                   FE_DIVBYZERO |
                   FE_OVERFLOW
    );
    #endif

    CUPS main(argc, argv);
    main.parseCommandLine();

    signal(SIGINT, SIGINT_handler_exit);
    return main.solve();
}
