/*
 * File:   UniFunctions.h
 * Author: kuldeep
 * Created on October 5, 2013, 9:26 PM
 */

#ifndef UNIFUNCTIONS_H
#define UNIFUNCTIONS_H

#include<iostream>
#include<vector>
#include <sstream>
#include<random>
#include "solver.h"
#include "solvertypes.h"
#include "constants.h"
//#include "StreamBuffer.h"
#include "constants.h"
using namespace std;
namespace CMSat {

class Solver;

class UniFunctions {
public:

private:

};

inline double findMean(list<int> numList)
{
    double sum = 0;
    for (list<int>::iterator it = numList.begin(); it != numList.end(); it++) {
        sum += *it;
    }
    return (sum * 1.0 / numList.size());
}

inline double findMedian(list<int> numList)
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

}
#endif  /* UNIFUNCTIONS_H */

