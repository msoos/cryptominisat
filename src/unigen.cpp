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

///////////////////////////////////////////////////



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
    vector<Lit> assumps;
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
                AddHash(hashDelta, assumps);
            } else if (hashDelta < 0) { // Remove hash functions
                assumps.clear();
                AddHash(currentHashCount, assumps);
            }
            lastHashCount = currentHashCount;

            double currentTime = cpuTimeTotal();
            elapsedTime = currentTime - startTime;
            if (elapsedTime > totalTimeout - 3000) {
                break;
            }
            uint32_t maxSolutions = (uint32_t) (1.41 * (1 + kappa) * pivotUniGen + 2);
            uint32_t minSolutions = (uint32_t) (pivotUniGen / (1.41 * (1 + kappa)));
            ret = BoundedSAT(maxSolutions + 1, minSolutions, assumps, solutionMap, &solutionCount);


            cusp_logf << "UniGen2:"
            << sampleCounter << ":" << currentHashCount << ":"
            << std::fixed << std::setprecision(2) << (cpuTimeTotal() - timeReference) << ":"
            << (int)(ret == l_False ? 1 : (ret == l_True ? 0 : 2)) << ":"
            << solutionCount << endl;

            if (ret == l_Undef) {   // SATSolver timed out; retry current hash count at most twice more
                assumps.clear();    // Throw out old hash functions
                if (repeatTry < 2) {    // Retry current hash count with new hash functions
                    AddHash(currentHashCount, assumps);
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
        assumps.clear();
        if (elapsedTime > totalTimeout - 3000) {
            break;
        }
    }
    return sampleCounter;
}

lbool CUSP::BoundedSAT(
    uint32_t maxSolutions
    , uint32_t minSolutions
    , vector<Lit>& assumps
    , std::map<string, uint32_t>& solutionMap
    , uint32_t* solutionCount
)
{
    unsigned long solutions = 0;
    lbool ret = l_True;
    solver->new_var();
    uint32_t act_var = solver->nVars()-1;
    vector<Lit> allSATAssumptions(assumps);
    allSATAssumptions.push_back(Lit(act_var, true));

    std::vector<vector<lbool>> modelsSet;
    vector<lbool> model;

    start_timer(loopTimeout);
    while (solutions < maxSolutions && ret == l_True) {
        cout << "BoundedSAT solve!" << endl;
        ret = solver->solve(&allSATAssumptions);
        solutions++;

        if (ret == l_True && solutions < maxSolutions) {
            vector<Lit> lits;
            lits.push_back(Lit(act_var, false));
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
    cout << "solutions:" << solutions << endl;
    vector<Lit> cls_that_removes;
    cls_that_removes.push_back(Lit(act_var, false));
    solver->add_clause(cls_that_removes);
    if (ret == l_Undef) {
        must_interrupt.store(false, std::memory_order_relaxed);
        return ret;
    }

    if (solutions < maxSolutions && solutions > minSolutions) {
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
