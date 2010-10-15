/*****************************************************************************************[Queue.h]
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
2008 - Gilles Audemard, Laurent Simon
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

Original code by MiniSat and glucose authors are under an MIT licence.
Modifications for CryptoMiniSat are under GPLv3 licence.
**************************************************************************************************/

#ifndef BOUNDEDQUEUE_H
#define BOUNDEDQUEUE_H

#ifdef _MSC_VER
#include <msvc/stdint.h>
#else
#include <stdint.h>
#endif //_MSC_VER

#include "Vec.h"

template <class T>
class bqueue {
    vec<T>  elems;
    int     first;
    int     last;
    uint64_t sumofqueue;
    int     maxsize;
    int     queuesize; // Number of current elements (must be < maxsize !)

public:
    bqueue(void) : first(0), last(0), sumofqueue(0), maxsize(0), queuesize(0) { }

    void initSize(int size) {growTo(size);} // Init size of bounded size queue

    void push(T x) {
        if (queuesize==maxsize) {
            assert(last==first); // The queue is full, next value to enter will replace oldest one
            sumofqueue -= elems[last];
            if ((++last) == maxsize) last = 0;
        } else
            queuesize++;
        sumofqueue += x;
        elems[first] = x;
        if ((++first) == maxsize) first = 0;
    }

    T peek() { assert(queuesize>0); return elems[last]; }
    void pop() {sumofqueue-=elems[last]; queuesize--; if ((++last) == maxsize) last = 0;}

    uint64_t getsum() const {return sumofqueue;}
    uint32_t getavg() const {return (uint64_t)sumofqueue/(uint64_t)queuesize;}
    int isvalid() const {return (queuesize==maxsize);}

    void growTo(int size) {
        elems.growTo(size);
        first=0; maxsize=size; queuesize = 0;
        for(int i=0;i<size;i++) elems[i]=0;
    }

    void fastclear() {first = 0; last = 0; queuesize=0; sumofqueue=0;} // to be called after restarts... Discard the queue

    int  size(void)    { return queuesize; }

    void clear(bool dealloc = false)   { elems.clear(dealloc); first = 0; maxsize=0; queuesize=0;sumofqueue=0;}
};

#endif //BOUNDEDQUEUE_H
