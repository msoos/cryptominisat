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
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <list>
#include <array>
#include <time.h>

#include "cusp.h"
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

timer_t* mytimer;
bool* timerSetFirstTime;
void start_timer(int num)
{
    struct sigevent sev;
    sev.sigev_signo = SIGUSR1;
    sev.sigev_notify = SIGEV_SIGNAL;
    struct itimerspec value;
    value.it_value.tv_sec = num; //waits for n seconds before sending timer signal
    value.it_value.tv_nsec = 0;
    value.it_interval.tv_sec = 0; //exipire once
    value.it_interval.tv_nsec = 0;
    if (*timerSetFirstTime) {
        timer_create(CLOCK_REALTIME, &sev, mytimer);
        //timer_delete(mytimer);
    }
    *timerSetFirstTime = false;
    timer_settime(*mytimer, 0, &value, NULL);
}

void SIGALARM_handler(int /*sig*/, siginfo_t* si, void* /*uc*/)
{
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

bool CUSP::GenerateRandomBits(string& randomBits, uint32_t size)
{
    std::uniform_int_distribution<unsigned> uid {0, 2147483647};
    uint32_t i = 0;
    while (i < size) {
        i += 31;
        randomBits += binary(uid(randomEngine), 31);
    }
    return true;
}

void CUSP::add_approxmc_options()
{
    approxMCOptions.add_options()
    ("samples", po::value(&samples)->default_value(samples), "")
    ("callsPerSolver", po::value(&callsPerSolver)->default_value(callsPerSolver), "")
    ("pivotAC", po::value(&pivotApproxMC)->default_value(pivotApproxMC), "")
    ("pivotUniGen", po::value(&pivotUniGen)->default_value(pivotUniGen), "")
    ("kappa", po::value(&kappa)->default_value(kappa), "")
    ("tApproxMC", po::value(&tApproxMC)->default_value(tApproxMC), "")
    ("startIteration", po::value(&startIteration)->default_value(startIteration), "")
    ("multisample", po::value(&multisample)->default_value(multisample), "")
    ("aggregation", po::value(&aggregateSolutions)->default_value(aggregateSolutions), "")
    ("looptout", po::value(&loopTimeout)->default_value(loopTimeout), "")
    ("cuspLogFile", po::value(&cuspLogFile)->default_value(cuspLogFile),"")
    ("onlyCount", po::value(&onlyCount)->default_value(onlyCount)
        ,"Only counting, the default. Otherwise, UNIGEN is used")
    ;

    help_options_simple.add(approxMCOptions);
    help_options_complicated.add(approxMCOptions);
}

void CUSP::add_supported_options()
{
    Main::add_supported_options();
    add_approxmc_options();
}

int CUSP::GenerateRandomNum(int maxRange)
{
    std::uniform_int_distribution<int> uid {0, maxRange};
    return uid(randomEngine);
}

/* Number of solutions to return from one invocation of UniGen2 */
uint32_t CUSP::SolutionsToReturn(
    uint32_t minSolutions
)
{
    if (multisample) {
        return minSolutions;
    } else {
        return 1;
    }
}

void print_xor(const vector<uint32_t>&vars, const uint32_t rhs)
{
    cout << "Added XOR ";
    for(size_t i = 0; i < vars.size(); i++) {
        cout << vars[i]+1;
        if (i < vars.size()-1) {
            cout << " + ";
        }
    }
    cout << " = " << (rhs ? "True" : "False") << endl;
}

void CUSP::seed_random_engine()
{
    /* Initialize PRNG with seed from random_device */
    std::random_device rd {};
    std::array<int, 10> seedArray;
    std::generate_n(seedArray.data(), seedArray.size(), std::ref(rd));
    std::seed_seq seed(std::begin(seedArray), std::end(seedArray));
    randomEngine.seed(seed);
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

void CUSP::set_up_timer()
{
    mytimer = new timer_t;
    timerSetFirstTime = new bool;
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = SIGALARM_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);
    *timerSetFirstTime = true;
    need_clean_exit = true;
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

bool CUSP::AddHash(uint32_t num_xor_cls, vector<Lit>& assumptions)
{
    string randomBits;
    GenerateRandomBits(randomBits, (independent_vars.size() + 1) * num_xor_cls);
    bool rhs = true;
    uint32_t activationVar;
    vector<uint32_t> vars;

    for (uint32_t i = 0; i < num_xor_cls; i++) {
        //new activation variable
        solver->new_var();
        activationVar = solver->nVars()-1;
        assumptions.push_back(Lit(activationVar, true));

        vars.clear();
        vars.push_back(activationVar);
        rhs = (randomBits[(independent_vars.size() + 1) * i] == 1);

        for (uint32_t j = 0; j < independent_vars.size(); j++) {
            if (randomBits[(independent_vars.size() + 1) * i + j] == '1') {
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
    //Set up things for adding clauses that can later be removed
    lbool ret = l_True;
    solver->new_var();
    uint32_t activationVar = solver->nVars()-1;
    vector<Lit> new_assumps(assumps);
    new_assumps.push_back(Lit(activationVar, true));

    //signal(SIGALRM, SIGALARM_handler);
    start_timer(loopTimeout);
    cout << "BoundedSATCount looking for " << maxSolutions << " solutions" << endl;
    uint64_t current_nr_of_solutions = 0;
    while (current_nr_of_solutions < maxSolutions && ret == l_True) {
        ret = solver->solve(&new_assumps);
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

    //Remove clauses added
    vector<Lit> cl_that_removes;
    cl_that_removes.push_back(Lit(activationVar, false));
    solver->add_clause(cl_that_removes);

    //Timeout
    if (ret == l_Undef) {
        must_interrupt.store(false, std::memory_order_relaxed);
        return -1 * current_nr_of_solutions;
    }
    return current_nr_of_solutions;
}

lbool CUSP::BoundedSAT(
    uint32_t maxSolutions
    , uint32_t minSolutions
    , vector<Lit>& assumptions
    , std::map<string, uint32_t>& solutionMap
    , uint32_t* solutionCount
)
{
    unsigned long current_nr_of_solutions = 0;
    lbool ret = l_True;
    solver->new_var();
    uint32_t activationVar = solver->nVars()-1;
    vector<Lit> allSATAssumptions(assumptions);
    allSATAssumptions.push_back(Lit(activationVar, true));

    std::vector<vector<lbool>> modelsSet;
    vector<lbool> model;
    //signal(SIGALRM, SIGALARM_handler);
    start_timer(loopTimeout);
    while (current_nr_of_solutions < maxSolutions && ret == l_True) {
        cout << "BoundedSAT solve!" << endl;
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

bool CUSP::ApproxMC(SATCount& count)
{
    count.clear();
    int64_t currentNumSolutions = 0;
    vector<uint64_t> numHashList;
    vector<int64_t> numCountList;
    vector<Lit> assumptions;
    uint32_t repeatTry = 0;
    for (uint32_t j = 0; j < tApproxMC; j++) {
        uint64_t hashCount;
        for (hashCount = 0; hashCount < solver->nVars(); hashCount++) {
            if (cpuTimeTotal() - startTime > totalTimeout - 3000) {
                cout << "Timeout in ApproxMC()" << endl;
                return false;
            }
            double myTime = cpuTimeTotal();
            currentNumSolutions = BoundedSATCount(pivotApproxMC + 1, assumptions);

            //cout << currentNumSolutions << ", " << pivotApproxMC << endl;
            cusp_logf << "ApproxMC:"
            << j << ":" << hashCount << ":"
            << std::fixed << std::setprecision(2) << (cpuTimeTotal() - myTime) << ":"
            << (int)(currentNumSolutions == (pivotApproxMC + 1)) << ":"
            << currentNumSolutions << endl;

            //Timeout!
            if (currentNumSolutions < 0) {
                //Remove all hashes
                assumptions.clear();

                if (repeatTry < 2) {    /* Retry up to twice more */
                    AddHash(hashCount, assumptions); //add new set of hashes
                    assert(hashCount > 0);
                    hashCount --;
                    repeatTry += 1;
                    cout << "Timeout, try again -- " << repeatTry << endl;
                } else {
                    //this set of hashes does not work, go up
                    AddHash(hashCount + 1, assumptions);
                    repeatTry = 0;
                    cout << "Timeout, moving up" << endl;
                }
                continue;
            }

            if (currentNumSolutions == pivotApproxMC + 1) {
                //Found all solutions needed
                AddHash(1, assumptions);
            } else {
                //less than pivotApproxMC solutions
                break;
            }
        }
        assumptions.clear();
        numHashList.push_back(hashCount);
        numCountList.push_back(currentNumSolutions);
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
    seed_random_engine();
    conf.reconfigure_at = 0;
    conf.reconfigure_val = 15;
    conf.gaussconf.autodisable = false;

    openLogFile();
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

    set_up_timer();

    if (startIteration > independent_vars.size()) {
        cout << "ERROR: Manually-specified startIteration"
             "is larger than the size of the independent set.\n" << endl;
        return -1;
    }

    SATCount solCount;
    if (startIteration == 0) {
        cout << "Computing startIteration using ApproxMC" << endl;

        bool finished = ApproxMC(solCount);
        double elapsedTime = cpuTimeTotal() - startTime;
        cout << "Completed ApproxMC at " << elapsedTime << " s" <<endl;
        if (!finished) {
            cout << " (TIMED OUT)" << endl;
            return 0;
        }

        if (solCount.hashCount == 0 && solCount.cellSolCount == 0) {
            cout << "The input formula is unsatisfiable." << endl;
            return correctReturnValue(l_False);
        }
        startIteration = round(solCount.hashCount + log2(solCount.cellSolCount) +
                                    log2(1.8) - log2(pivotUniGen)) - 2;
    } else {
        cout << "Using manually-specified startIteration" << endl;
    }

    //Either onlycount or unigen
    if (onlyCount) {
        cout << "Number of solutions is: " << solCount.cellSolCount
        << " x 2^" << solCount.hashCount << endl;
    } else {
        assert(false);
        generate_samples();
    }

    if (conf.verbosity >= 1) {
        solver->print_stats();
    }

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
    #endif

    CUSP main(argc, argv);
    main.parseCommandLine();

    return main.solve();
}

void CUSP::call_after_parse(const vector<uint32_t>& _independent_vars)
{
    independent_vars = _independent_vars;
    if (independent_vars.empty()) {
        for(size_t i = 0; i < solver->nVars(); i++) {
            independent_vars.push_back(i);
        }
    }
}



////// UNIGEN
void CUSP::generate_samples()
{
    uint32_t maxSolutions = (uint32_t) (1.41 * (1 + kappa) * pivotUniGen + 2);
    uint32_t minSolutions = (uint32_t) (pivotUniGen / (1.41 * (1 + kappa)));
    uint32_t samplesPerCall = SolutionsToReturn(minSolutions);
    uint32_t callsNeeded = (samples + samplesPerCall - 1) / samplesPerCall;
    cout << "loThresh " << minSolutions
    << ", hiThresh " << maxSolutions
    << ", startIteration " << startIteration << endl;;

    printf("Outputting %d solutions from each UniGen2 call\n", samplesPerCall);
    uint32_t numCallsInOneLoop = 0;
    if (callsPerSolver == 0) {
        numCallsInOneLoop = std::min(solver->nVars() / (startIteration * 14), callsNeeded);
        if (numCallsInOneLoop == 0) {
            numCallsInOneLoop = 1;
        }
    } else {
        numCallsInOneLoop = callsPerSolver;
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
    uint32_t lastSuccessfulHashOffset = 0;
    ///Perform extra UniGen calls that don't fit into the loops
    if (remainingCalls > 0) {
        sampleCounter = uniGenCall(
                            remainingCalls, sampleCounter
                            , threadSolutionMap
                            , &lastSuccessfulHashOffset, threadStartTime);
    }

    // Perform main UniGen call loops
    for (uint32_t i = 0; i < numCallLoops; i++) {
        if (!timedOut) {
            sampleCounter = uniGenCall(
                                numCallsInOneLoop, sampleCounter, threadSolutionMap
                                , &lastSuccessfulHashOffset, threadStartTime
                            );

            if ((cpuTimeTotal() - threadStartTime) > totalTimeout - 3000) {
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
    << "Total time for UniGen2: " << timeTaken << " s"
    << (timedOut ? " (TIMED OUT)" : "")
    << endl;

    cout << "Total time for all UniGen2 calls: " << allThreadsTime << " s" << endl;
    cout << "Samples generated: " << allThreadsSampleCount << endl;
}

uint32_t CUSP::UniGen(
    uint32_t samples
    , uint32_t sampleCounter
    , std::map<string, uint32_t>& solutionMap
    , uint32_t* lastSuccessfulHashOffset
    , double timeReference
)
{
    lbool ret = l_False;
    uint32_t i, solutionCount, currentHashCount, lastHashCount, currentHashOffset, hashOffsets[3];
    int hashDelta;
    vector<Lit> assumptions;
    double elapsedTime = 0;
    int repeatTry = 0;
    for (i = 0; i < samples; i++) {
        sampleCounter ++;
        ret = l_False;

        hashOffsets[0] = *lastSuccessfulHashOffset;   // Start at last successful hash offset
        if (hashOffsets[0] == 0) { // Starting at q-2; go to q-1 then q
            hashOffsets[1] = 1;
            hashOffsets[2] = 2;
        } else if (hashOffsets[0] == 2) { // Starting at q; go to q-1 then q-26
            hashOffsets[1] = 1;
            hashOffsets[2] = 0;
        }
        repeatTry = 0;
        lastHashCount = 0;
        for (uint32_t j = 0; j < 3; j++) {
            currentHashOffset = hashOffsets[j];
            currentHashCount = currentHashOffset + startIteration;
            hashDelta = currentHashCount - lastHashCount;

            if (hashDelta > 0) { // Add new hash functions
                AddHash(hashDelta, assumptions);
            } else if (hashDelta < 0) { // Remove hash functions
                assumptions.clear();
                AddHash(currentHashCount, assumptions);
            }
            lastHashCount = currentHashCount;

            double currentTime = cpuTimeTotal();
            elapsedTime = currentTime - startTime;
            if (elapsedTime > totalTimeout - 3000) {
                break;
            }
            uint32_t maxSolutions = (uint32_t) (1.41 * (1 + kappa) * pivotUniGen + 2);
            uint32_t minSolutions = (uint32_t) (pivotUniGen / (1.41 * (1 + kappa)));
            ret = BoundedSAT(maxSolutions + 1, minSolutions, assumptions, solutionMap, &solutionCount);


            cusp_logf << "UniGen2:"
            << sampleCounter << ":" << currentHashCount << ":"
            << std::fixed << std::setprecision(2) << (cpuTimeTotal() - timeReference) << ":"
            << (int)(ret == l_False ? 1 : (ret == l_True ? 0 : 2)) << ":"
            << solutionCount << endl;

            if (ret == l_Undef) {   // SATSolver timed out; retry current hash count at most twice more
                assumptions.clear();    // Throw out old hash functions
                if (repeatTry < 2) {    // Retry current hash count with new hash functions
                    AddHash(currentHashCount, assumptions);
                    j--;
                    repeatTry += 1;
                } else {     // Go on to next hash count
                    lastHashCount = 0;
                    if ((j == 0) && (currentHashOffset == 1)) { // At q-1, and need to pick next hash count
                        // Somewhat arbitrarily pick q-2 first; then q
                        hashOffsets[1] = 0;
                        hashOffsets[2] = 2;
                    }
                    repeatTry = 0;
                }
                continue;
            }
            if (ret == l_True) {    // Number of solutions in correct range
                *lastSuccessfulHashOffset = currentHashOffset;
                break;
            } else { // Number of solutions too small or too large
                if ((j == 0) && (currentHashOffset == 1)) { // At q-1, and need to pick next hash count
                    if (solutionCount < minSolutions) {
                        // Go to q-2; next will be q
                        hashOffsets[1] = 0;
                        hashOffsets[2] = 2;
                    } else {
                        // Go to q; next will be q-2
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
        if (elapsedTime > totalTimeout - 3000) {
            break;
        }
    }
    return sampleCounter;
}

int CUSP::uniGenCall(
    uint32_t samples
    , uint32_t sampleCounter
    , std::map<string, uint32_t>& solutionMap
    , uint32_t* lastSuccessfulHashOffset
    , double timeReference
)
{
    delete solver;
    solver = new SATSolver(&conf, &must_interrupt);
    solverToInterrupt = solver;

    parseInAllFiles(solver);
    sampleCounter = UniGen(
                        samples
                        , sampleCounter
                        , solutionMap
                        , lastSuccessfulHashOffset
                        , timeReference
                    );
    return sampleCounter;
}
