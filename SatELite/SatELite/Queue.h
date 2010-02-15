/**************************************************************************************************

Queue.h -- (C) Niklas Een, 2003

Simple implementation of a queue (intended to queue propagations).

**************************************************************************************************/

#ifndef Queue_h
#define Queue_h


//=================================================================================================


template <class T>
class Queue {
    vec<T>  elems;
    int     first;

public:
    Queue(void) : first(0) { }

    void insert(T x)   { elems.push(x); }
    T    dequeue(void) { return elems[first++]; }
    void clear(void)   { elems.clear(); first = 0; }
    int  size(void)    { return elems.size() - first; }

    bool has(T x) { for (int i = first; i < elems.size(); i++) if (elems[i] == x) return true; return false; }
};


//=================================================================================================
#endif
