/******************************************************************************************[Heap.h]
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef Heap_h
#define Heap_h

#include "Vec.h"
#include "string.h"
#include <stdint.h>
#define UINT32_MAX ((uint32_t)-1)

//=================================================================================================
// A heap implementation with support for decrease/increase key.


template<class Comp>
class Heap {
    Comp     lt;
    vec<uint32_t> heap;     // heap of ints
    vec<uint32_t> indices;  // int -> index in heap

    // Index "traversal" functions
    static inline uint32_t left  (uint32_t i) { return i*2+1; }
    static inline uint32_t right (uint32_t i) { return (i+1)*2; }
    static inline uint32_t parent(uint32_t i) { return (i-1) >> 1; }


    inline void percolateUp(uint32_t i)
    {
        uint32_t x = heap[i];
        while (i != 0 && lt(x, heap[parent(i)])){
            heap[i]          = heap[parent(i)];
            indices[heap[i]] = i;
            i                = parent(i);
        }
        heap   [i] = x;
        indices[x] = i;
    }


    inline void percolateDown(uint32_t i)
    {
        uint32_t x = heap[i];
        while (left(i) < heap.size()){
            uint32_t child = right(i) < heap.size() && lt(heap[right(i)], heap[left(i)]) ? right(i) : left(i);
            if (!lt(heap[child], x)) break;
            heap[i]          = heap[child];
            indices[heap[i]] = i;
            i                = child;
        }
        heap   [i] = x;
        indices[x] = i;
    }


    bool heapProperty (uint32_t i) const {
        return i >= heap.size()
            || ((i == 0 || !lt(heap[i], heap[parent(i)])) && heapProperty(left(i)) && heapProperty(right(i))); }


  public:
    Heap(const Comp& c) : lt(c) { }
    Heap(const Heap<Comp>& other) : lt(other.lt) {
        heap.growTo(other.heap.size());
        memcpy(heap.getData(), other.heap.getData(), sizeof(uint32_t)*other.heap.size());
        indices.growTo(other.indices.size());
        memcpy(indices.getData(), other.indices.getData(), sizeof(uint32_t)*other.indices.size());
    }

    uint32_t  size      ()          const { return heap.size(); }
    bool empty     ()          const { return heap.size() == 0; }
    bool inHeap    (uint32_t n)     const { return n < indices.size() && indices[n] != UINT32_MAX; }
    uint32_t  operator[](uint32_t index) const { assert(index < heap.size()); return heap[index]; }

    void decrease  (uint32_t n) { assert(inHeap(n)); percolateUp(indices[n]); }

    // RENAME WHEN THE DEPRECATED INCREASE IS REMOVED.
    void increase_ (uint32_t n) { assert(inHeap(n)); percolateDown(indices[n]); }


    void insert(uint32_t n)
    {
        indices.growTo(n+1, UINT32_MAX);
        assert(!inHeap(n));

        indices[n] = heap.size();
        heap.push(n);
        percolateUp(indices[n]); 
    }


    uint32_t  removeMin()
    {
        uint32_t x            = heap[0];
        heap[0]          = heap.last();
        indices[heap[0]] = 0;
        indices[x]       = UINT32_MAX;
        heap.pop();
        if (heap.size() > 1) percolateDown(0);
        return x; 
    }


    void clear(bool dealloc = false) 
    { 
        for (uint32_t i = 0; i != heap.size(); i++)
            indices[heap[i]] = UINT32_MAX;
#ifndef NDEBUG
        for (uint32_t i = 0; i != indices.size(); i++)
            assert(indices[i] == UINT32_MAX);
#endif
        heap.clear(dealloc); 
    }


    // Fool proof variant of insert/decrease/increase
    void update (uint32_t n)
    {
        if (!inHeap(n))
            insert(n);
        else {
            percolateUp(indices[n]);
            percolateDown(indices[n]);
        }
    }


    // Delete elements from the heap using a given filter function (-object).
    // *** this could probaly be replaced with a more general "buildHeap(vec<int>&)" method ***
    template <class F>
    void filter(const F& filt) {
        uint32_t i,j;
        for (i = j = 0; i != heap.size(); i++)
            if (filt(heap[i])){
                heap[j]          = heap[i];
                indices[heap[i]] = j++;
            }else
                indices[heap[i]] = UINT32_MAX;

        heap.shrink(i - j);
        for (int i = heap.size() / 2 - 1; i >= 0; i--)
            percolateDown(i);

        assert(heapProperty());
    }


    // DEBUG: consistency checking
    bool heapProperty() const {
        return heapProperty(1); }


    // COMPAT: should be removed
    void setBounds (uint32_t n) { }
    void increase  (uint32_t n) { decrease(n); }
    uint32_t  getmin    ()      { return removeMin(); }

};


//=================================================================================================
#endif
