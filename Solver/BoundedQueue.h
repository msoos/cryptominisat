/*
This file is part of CryptoMiniSat2.

CryptoMiniSat2 is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

CryptoMiniSat2 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CryptoMiniSat2.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef BOUNDEDQUEUE_H
#define BOUNDEDQUEUE_H

#ifdef _MSC_VER
#include <msvc/stdint.h>
#else
#include <stdint.h>
#endif //_MSC_VER

#include "Vec.h"

namespace CMSat {

template <class T>
class bqueue {
    vec<T>  elems;
    uint32_t first;
    uint32_t last;
    int64_t  sumofqueue;
    int64_t  sumOfAllElems;
    uint64_t totalNumElems;
    uint32_t maxsize;
    uint32_t queuesize; // Number of current elements (must be < maxsize !)

public:
    bqueue(void) :
        first(0)
        , last(0)
        , sumofqueue(0)
        , sumOfAllElems(0)
        , totalNumElems(0)
        , maxsize(0)
        , queuesize(0)
    {}

    void initSize(const uint32_t size) {growTo(size);} // Init size of bounded size queue

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

    const T peek() const { assert(queuesize>0); return elems[last]; }
    void pop() {sumofqueue-=elems[last]; queuesize--; if ((++last) == maxsize) last = 0;}

    int64_t getsum() const {return sumofqueue;}
    uint32_t getAvgUInt() const {return (uint64_t)sumofqueue/(uint64_t)queuesize;}
    double getAvgDouble() const {return (double)sumofqueue/(double)queuesize;}
    double getAvgAllDouble() const {return (double)sumOfAllElems/(double)totalNumElems;}
    uint64_t getTotalNumeElems() const {return totalNumElems;}
    int isvalid() const {return (queuesize==maxsize);}

    void growTo(const uint32_t size) {
        elems.growTo(size);
        first=0; maxsize=size; queuesize = 0;
        for(uint32_t i=0;i<size;i++) elems[i]=0;
    }

    void fastclear() {first = 0; last = 0; queuesize=0; sumofqueue=0;} // to be called after restarts... Discard the queue

    int  size(void)    { return queuesize; }

    void clear(bool dealloc = false)   {
        elems.clear(dealloc);
        first = 0;
        last = 0;
        maxsize=0;
        queuesize=0;
        sumofqueue=0;

        totalNumElems = 0;
        sumOfAllElems = 0;
    }
};

}

#endif //BOUNDEDQUEUE_H
