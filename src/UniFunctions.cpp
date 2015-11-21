
#include<vector>
#include<algorithm>
#include<list>
#include<stdio.h>
#include<string>
#include<iostream>
#include<stdlib.h>
#include<fstream>
#include<iterator>
#include<bitset>
#include<ctime>

#include "time_mem.h"
#include "UniFunctions.h"
#include "solvertypes.h"

using namespace CMSat;
using namespace std;

double findMean(list<int> numList)
{
    double sum = 0;
    for (list<int>::iterator it = numList.begin(); it != numList.end(); it++) {
        sum += *it;
    }
    return (sum * 1.0 / numList.size());
}

double findMedian(list<int> numList)
{
    numList.sort();
    int medIndex = int((numList.size() + 1) / 2);
    list<int>::iterator it = numList.begin();
    if (medIndex >= (int) numList.size()) {
        std::advance(it, numList.size() - 1);
        return double(*it);
    }
    std::advance(it, medIndex);
    return double(*it);
}

int findMin(list<int> numList)
{
    int min = std::numeric_limits<int>::max();
    for (list<int>::iterator it = numList.begin(); it != numList.end(); it++) {
        if ((*it) < min) {
            min = *it;
        }
    }
    return min;
}

std::string binary(int x, uint32_t length)
{
    uint32_t logSize = log2(x) + 1;
    std::string s;
    do {
        s.push_back('0' + (x & 1));
    } while (x >>= 1);
    for (uint32_t i = logSize; i < (uint32_t) length; i++) {
        s.push_back('0');
    }
    std::reverse(s.begin(), s.end());

    return s;

}

bool UniFunctions::GenerateRandomBits(string& randomBits, uint32_t size)
{
    std::uniform_int_distribution<int> uid(0, 2147483647);
    //uint32_t logSize = log2(size)+1;
    uint32_t i = 0;
    while (i < size) {
        i += 31;
        randomBits += binary(uid(rd), 31);
    }
    return true;
}

int UniFunctions::GenerateRandomNum(int maxRange)
{
    std::uniform_int_distribution<int> uid(0, maxRange);
    int value = -1;
    while (value < 0 || value > maxRange) {
        value = uid(rd);
    }
    return value;
}

bool UniFunctions::AddHash(uint32_t numClaus, Solver& solver)
{
    string randomBits;
    GenerateRandomBits(randomBits, (solver.independentSet.size() + 1) * numClaus);
    bool xorEqualFalse = false;
    Var activationVar;

    for (uint32_t i = 0; i < numClaus; i++) {
        lits.clear();
        activationVar = solver.newVar();
        assumptions.push(Lit(activationVar, true));
        lits.push(Lit(activationVar, false));
        xorEqualFalse = (randomBits[(solver.independentSet.size() + 1) * i] == 1);

        for (uint32_t j = 0; j < solver.independentSet.size(); j++) {

            if (randomBits[(solver.independentSet.size() + 1) * i + j] == '1') {
                lits.push(Lit(solver.independentSet[j], true));
            }
        }
        solver.addXorClause(lits, xorEqualFalse);
    }
    return true;
}

void UniFunctions::printResultFunc(Solver& S, vec<lbool> solutionModel, const lbool ret, FILE* res, bool printResult)
{
    if (res != NULL && printResult) {
        if (ret == l_True) {
            fprintf(res, "v ");
            for (Var var = 0; var != S.nOrigVars(); var++)
                if (solutionModel[var] != l_Undef) {
                    fprintf(res, "%s%d ", (S.model[var] == l_True) ? "" : "-", var + 1);
                }
            fprintf(res, "0\n");
            fflush(res);
        }
    } else {

        if (ret == l_True && printResult) {
            std::stringstream toPrint;
            toPrint << "v ";
            for (Var var = 0; var != S.nOrigVars(); var++)
                if (solutionModel[var] != l_Undef) {
                    toPrint << ((solutionModel[var] == l_True) ? "" : "-") << var + 1 << " ";
                }
            toPrint << "0" << std::endl;
            std::cout << toPrint.str();
        }
    }
}

uint32_t UniFunctions::BoundedSATCount(uint32_t maxSolutions, Solver& solver, int timeout)
{
    unsigned long current_nr_of_solutions = 0;
    lbool ret = l_True;
    Var activationVar = solver.newVar();
    allSATAssumptions.clear();
    if (!assumptions.empty()) {
        assumptions.copyTo(allSATAssumptions);
    }
    allSATAssumptions.push(Lit(activationVar, true));

    while (current_nr_of_solutions < maxSolutions && ret == l_True) {

        ret = solver.solve(allSATAssumptions);
        current_nr_of_solutions++;
        if (ret == l_True && current_nr_of_solutions < maxSolutions) {
            vec<Lit> lits;
            lits.push(Lit(activationVar, false));
            for (Var var = 0; var != solver.nVars(); var++) {
                if (solver.model[var] != l_Undef) {
                    lits.push(Lit(var, (solver.model[var] == l_True) ? true : false));
                }
            }
            solver.addClause(lits);
        }
    }
    vec<Lit> cls_that_removes;
    cls_that_removes.push(Lit(activationVar, false));
    solver.addClause(cls_that_removes);
    return current_nr_of_solutions;
}

lbool UniFunctions::BoundedSAT(uint32_t maxSolutions, Solver& solver, FILE* res,
                               bool printResult, int timeout)
{
    unsigned long current_nr_of_solutions = 0;
    lbool ret = l_True;
    Var activationVar = solver.newVar();
    allSATAssumptions.clear();
    if (!assumptions.empty()) {

        assumptions.copyTo(allSATAssumptions);
    }
    allSATAssumptions.push(Lit(activationVar, true));

    modelsSet.clear();
    while (current_nr_of_solutions < maxSolutions && ret == l_True) {
        ret = solver.solve(allSATAssumptions);
        current_nr_of_solutions++;
        if (ret == l_True && current_nr_of_solutions < maxSolutions) {
            vec<Lit> lits;
            lits.push(Lit(activationVar, false));
            model.clear();
            solver.model.copyTo(model);
            modelsSet.push_back(model);
            for (Var var = 0; var != solver.nVars(); var++) {
                if (solver.model[var] != l_Undef) {
                    lits.push(Lit(var, (solver.model[var] == l_True) ? true : false));
                }
            }
            solver.addClause(lits);
        }
    }
    vec<Lit> cls_that_removes;
    cls_that_removes.push(Lit(activationVar, false));
    solver.addClause(cls_that_removes);

    if (current_nr_of_solutions < maxSolutions) {
        int randNum = GenerateRandomNum(modelsSet.size() - 1);
        printResultFunc(solver, modelsSet.at(randNum), l_True, res, printResult);
        return l_True;
    }
    return l_False;
}

SATCount UniFunctions::ApproxMC(uint32_t pivot, uint32_t t, Solver& solver,
                                FILE* resLog, bool shouldLog, int timeout)
{
    uint32_t currentNumSolutions = 0, hashCount;
    list<int> numHashList, numCountList;
    SATCount solCount;
    for (uint32_t j = 0; j < t; j++) {
        for (hashCount = 0; hashCount < solver.nVars(); hashCount++) {
            double myTime = cpuTime();
            currentNumSolutions = BoundedSATCount(pivot + 1, solver, timeout);
            myTime = cpuTime() - myTime;
            if (shouldLog) {
                fprintf(resLog, "ApproxMC:%d:%d:%f:%d\n", j, hashCount, myTime, (currentNumSolutions == pivot + 1));
                fflush(resLog);
            }
            if (currentNumSolutions == pivot + 1) {
                AddHash(1, solver);
            } else {
                break;
            }
        }
        numHashList.push_back(hashCount);
        numCountList.push_back(currentNumSolutions);
        assumptions.clear();
    }
    int minHash = findMin(numHashList);
    for (list<int>::iterator it1 = numHashList.begin(), it2 = numCountList.begin();
            it1 != numHashList.end() && it2 != numCountList.end(); it1++, it2++) {
        (*it2) *= pow(2, (*it1) - minHash);
    }

    int medSolCount = findMedian(numCountList);
    solCount.cellSolCount = medSolCount;
    solCount.hashCount = minHash;
    return solCount;
}

/*
 * Returns the number of samples generated
 */
uint32_t UniFunctions::UniGen(uint32_t pivot, uint32_t startIteration, uint32_t samples, Solver& solver, FILE* res, bool printResult, FILE* resLog, bool shouldLog)
{
    lbool ret = l_False;
    uint32_t i;
    if (!assumptions.empty()) {
        assumptions.clear();
    }
    for (i = 0; i < samples; i++) {
        AddHash(startIteration, solver);
        for (uint32_t j = startIteration; j <= startIteration + 3; j++) {
            double myTime = cpuTime();
            ret = BoundedSAT(pivot + 1, solver, res, printResult);
            myTime = cpuTime() - myTime;
            if (shouldLog) {
                fprintf(resLog, "UniGen:%d:%d:%f:%d\n", i, j, myTime, (ret == l_False));
                fflush(resLog);
            }

            if (ret == l_True) {
                break;
            } else {
                AddHash(1, solver);
            }
        }
        assumptions.clear();
    }
    return i;
}



