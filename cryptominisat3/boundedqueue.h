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
#include "avgcalc.h"
#include "assert.h"
#include <vector>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace CMSat {
using std::vector;

template <class T, class T2 = uint64_t>
class bqueue {
    //Only stores info for N elements
    vector<T>  elems;
    size_t first;
    size_t last;
    size_t maxsize; //max number of history elements
    size_t queuesize; // Number of current elements (must be < maxsize !)
    T2  sumofqueue;
    AvgCalc<T, T2> longTermAvg;

public:
    bqueue(void) :
        first(0)
        , last(0)
        , maxsize(0)
        , queuesize(0)
        , sumofqueue(0)
    {}

    size_t usedMem() const
    {
        return sizeof(size_t)*4 + elems.capacity()*sizeof(T) + sizeof(T2) + sizeof(AvgCalc<T,T2>);
    }

    void push(const T x) {
        if (queuesize == maxsize) {
            // The queue is full, next value to enter will replace oldest one

            assert(last == first);
            sumofqueue -= elems[last];

            last++;
            if (last == maxsize)
                last = 0;

        } else {
            queuesize++;
        }

        sumofqueue += x;

        //Update avg
        longTermAvg.push(x);
        elems[first] = x;

        first++;
        if (first == maxsize)
            first = 0;
    }

    double avg() const
    {
        if (queuesize == 0)
            return 0;

        assert(isvalid());
        return (double)sumofqueue/(double)queuesize;
    }

    const AvgCalc<T,T2>& getLongtTerm() const
    {
        return longTermAvg;
    }

    std::string getAvgPrint(size_t prec, size_t w) const
    {
        std::stringstream ss;
        if (isvalid()) {
            ss
            << std::fixed << std::setprecision(prec) << std::setw(w) << std::right
            << avg();
        } else {
            ss << std::setw(5) << "?";
        }

        return ss.str();
    }

    bool isvalid() const
    {
        return (queuesize == maxsize);
    }

    void clearAndResize(const size_t size)
    {
        clear();
        elems.resize(size);
        maxsize = size;
    }

    void clear()
    {
        first = 0;
        last = 0;
        queuesize = 0;
        sumofqueue = 0;

        longTermAvg.clear();
    }

    size_t get_size() const
    {
        return queuesize;
    }

    size_t get_maxsize() const
    {
        return maxsize;
    }
};

} //end namespace

#endif //BOUNDEDQUEUE_H
