/*****************************************************************************************[Queue.h]
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
2008 - Gilles Audemard, Laurent Simon
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

Original code by MiniSat and glucose authors are under an MIT licence.
Modifications for CryptoMiniSat are under GPLv3 licence.
**************************************************************************************************/

#ifndef BOUNDEDQUEUE_H
#define BOUNDEDQUEUE_H

#include "constants.h"
#include "assert.h"
#include <vector>
#include <cstring>
#include <sstream>
#include <iomanip>
using std::vector;

template <class T, class T2 = uint64_t>
class bqueue {
    vector<T>  elems;
    size_t first;
    size_t last;
    size_t maxsize; //max number of history elements
    size_t queuesize; // Number of current elements (must be < maxsize !)

    T2  sumofqueue;
    T2  sumOfAllElems;
    size_t totalNumElems;

public:
    bqueue(void) :
        first(0)
        , last(0)
        , maxsize(0)
        , queuesize(0)
        , sumofqueue(0)
        , sumOfAllElems(0)
        , totalNumElems(0)
    {}

    void push(const T x) {
        if (queuesize==maxsize) {
            assert(last==first); // The queue is full, next value to enter will replace oldest one
            sumofqueue -= elems[last];
            if ((++last) == maxsize) last = 0;
        } else
            queuesize++;
        sumofqueue += x;
        sumOfAllElems += x;
        totalNumElems++;
        elems[first] = x;
        if ((++first) == maxsize) first = 0;
    }

    /*const T peek() const { assert(queuesize>0); return elems[last]; }
    void pop() {sumofqueue-=elems[last]; queuesize--; if ((++last) == maxsize) last = 0;}*/

    T2 getsum() const
    {
        return sumofqueue;
    }

    double getAvg() const
    {
        assert(isvalid());
        return (double)sumofqueue/(double)queuesize;
    }

    std::string getAvgPrint(size_t prec, size_t w) const
    {
        std::stringstream ss;
        if (isvalid()) {
            ss << std::fixed << std::setprecision(prec) << std::setw(w) << std::left << getAvg();
        } else {
            ss << std::setw(5) << std::left << "?";
        }

        return ss.str();
    }

    std::string getAvgAllPrint(size_t prec, size_t w) const
    {
        std::stringstream ss;
        if (isvalid()) {
            ss << std::fixed << std::setprecision(prec) << std::setw(w) << std::left << getAvgAll();
        } else {
            ss << std::setw(5) << std::left << "?";
        }

        return ss.str();
    }

    double getAvgAll() const
    {
        if (totalNumElems == 0)
            return 0;

        return (double)sumOfAllElems/(double)totalNumElems;
    }

    uint64_t getTotalNumeElems() const
    {
        return totalNumElems;
    }

    bool isvalid() const
    {
        return (queuesize==maxsize);
    }

    void resize(const uint32_t size)
    {
        elems.resize(size);
        first=0; maxsize=size; queuesize = 0;
        for(uint32_t i=0;i<size;i++) elems[i]=0;
    }

    void fastclear() {
        //Discard the queue, but not the SUMs
        first = 0;
        last = 0;
        queuesize=0;
        sumofqueue=0;
    }

    int  size(void)
    {
        return queuesize;

    }

    void clear()   {
        elems.clear();
        first = 0;
        last = 0;
        maxsize=0;
        queuesize=0;
        sumofqueue=0;

        totalNumElems = 0;
        sumOfAllElems = 0;
    }
};

#endif //BOUNDEDQUEUE_H
