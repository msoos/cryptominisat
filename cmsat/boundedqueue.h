/***************************************************************************
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
2008 - Gilles Audemard, Laurent Simon
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

Original code by MiniSat and glucose authors are under an MIT licence
Modifications also under MIT
****************************************************************************/

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

    //for mid-term history size
    T2  sumOfElemsMidLong;
    double  sumOfElemsMidLongSqare;
    size_t totalNumElemsMidLong;

    T2  sumOfElemsLong;
    size_t totalNumElemsLong;

public:
    bqueue(void) :
        first(0)
        , last(0)
        , maxsize(0)
        , queuesize(0)
        , sumofqueue(0)

        //Mid
        , sumOfElemsMidLong(0)
        , totalNumElemsMidLong(0)

        //Full
        , sumOfElemsLong(0)
        , totalNumElemsLong(0)
    {}

    void push(const T x) {
        if (queuesize==maxsize) {
            assert(last==first); // The queue is full, next value to enter will replace oldest one
            sumofqueue -= elems[last];
            if ((++last) == maxsize) last = 0;
        } else
            queuesize++;
        sumofqueue += x;

        //Update mid
        sumOfElemsMidLong += x;
        sumOfElemsMidLongSqare += (uint64_t)x*(uint64_t)x;
        totalNumElemsMidLong++;

        //Update long
        sumOfElemsLong += x;
        totalNumElemsLong++;

        elems[first] = x;
        if ((++first) == maxsize) first = 0;
    }

    double getAvg() const
    {
        if (queuesize == 0)
            return 0;

        assert(isvalid());
        return (double)sumofqueue/(double)queuesize;
    }

    double getAvgMidLong() const
    {
        if (totalNumElemsMidLong == 0)
            return 0;

        return (double)sumOfElemsMidLong/(double)totalNumElemsMidLong;
    }

    double getVarMidLong() const
    {
        if (totalNumElemsMidLong == 0)
            return 0;

        const double avg = getAvgMidLong();
        return
            (((double)sumOfElemsMidLongSqare
                - 2.0*avg*(double)sumOfElemsMidLong
            ))/(double)totalNumElemsMidLong
             + avg*avg;
    }

    double getAvgLong() const
    {
        if (totalNumElemsLong == 0)
            return 0;

        return (double)sumOfElemsLong/(double)totalNumElemsLong;
    }

    std::string getAvgPrint(size_t prec, size_t w) const
    {
        std::stringstream ss;
        if (isvalid()) {
            ss << std::fixed << std::setprecision(prec) << std::setw(w) << std::right
            << getAvg();
        } else {
            ss << std::setw(5) << "?";
        }

        return ss.str();
    }

    std::string getAvgMidPrint(size_t prec, size_t w) const
    {
        std::stringstream ss;
        if (totalNumElemsMidLong > 0) {
            ss << std::fixed << std::setprecision(prec) << std::setw(w) << std::left
            << getAvgMidLong();
        } else {
            ss << std::setw(5) << "?";
        }

        return ss.str();
    }

    std::string getAvgLongPrint(size_t prec, size_t w) const
    {
        std::stringstream ss;
        if (totalNumElemsLong > 0) {
            ss << std::fixed << std::setprecision(prec) << std::setw(w) << std::left
            << getAvgLong();
        } else {
            ss << std::setw(5) << "?";
        }

        return ss.str();
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

    void fastclear()
    {
        //Discard the queue, but not the SUMs
        first = 0;
        last = 0;
        queuesize=0;
        sumofqueue=0;

        totalNumElemsMidLong = 0;
        sumOfElemsMidLong = 0;
        sumOfElemsMidLongSqare = 0;
    }

    int  size(void)
    {
        return queuesize;
    }

    void clear()
    {
        elems.clear();
        first = 0;
        last = 0;
        maxsize=0;
        queuesize=0;
        sumofqueue=0;

        totalNumElemsMidLong = 0;
        sumOfElemsMidLong = 0;
        sumOfElemsMidLongSqare = 0;

        totalNumElemsLong = 0;
        sumOfElemsLong = 0;
    }
};

#endif //BOUNDEDQUEUE_H
