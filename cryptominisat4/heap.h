/******************************************************************************************[heap.h]
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

#include "vec.h"
#include "constants.h"
#include <algorithm>

// A heap implementation with support for decrease/increase key.
template<class Comp>
class Heap {
    Comp lt;
    vec<uint32_t> heap;     // heap of ints
    vec<uint32_t> indices;  // int -> index in heap

    // Index "traversal" functions
    static inline uint32_t left  (uint32_t i) { return i*2; }
    static inline uint32_t right (uint32_t i) { return i*2+1; }
    static inline uint32_t parent(uint32_t i) { return i/2; }

    inline void percolateUp(uint32_t i)
    {
        uint32_t x = heap[i];
        while (i > 1 && lt(x, heap[parent(i)])) {
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
        while (left(i) < heap.size()) {
            uint32_t child;
            if ((right(i) < heap.size())
                && lt(heap[right(i)], heap[left(i)])
            ) {
                child = right(i);
            } else {
                child = left(i);
            }
            if (!lt(heap[child], x)) {
                break;
            }
            heap[i]          = heap[child];
            indices[heap[i]] = i;
            i                = child;
        }
        heap   [i] = x;
        indices[x] = i;
    }


    //i: position in heap
    bool heapProperty (uint32_t i) const {
        return i >= heap.size()
            || ( (i == 1 || !lt(heap[i], heap[parent(i)]))
                  && heapProperty( left(i)  )
                  && heapProperty( right(i) )
               );
    }

  public:
    Heap(const Comp& c) :
        lt(c)
    {
        heap.push(std::numeric_limits<uint32_t>::max());
    }

    Heap(const Heap<Comp>& other) : lt(other.lt) {
        heap.growTo(other.heap.size());
        std::copy(other.heap.begin(), other.heap.end(), heap.begin());
        indices.growTo(other.indices.size());
        std::copy(other.indices.begin(), other.indices.end(), indices.begin());
    }

    void operator=(const Heap<Comp>& other)
    {
        if (other.heap.size() > heap.size())
            heap.growTo(other.heap.size());
        else
            heap.shrink(heap.size()-other.heap.size());
        std::copy(other.heap.begin(), other.heap.end(), heap.begin());

        if (other.indices.size() > indices.size())
            indices.growTo(other.indices.size());
        else
            indices.shrink(indices.size() - other.indices.size());
        std::copy(other.indices.begin(), other.indices.end(), indices.begin());
    }

    size_t memUsed() const
    {
        return heap.capacity()*sizeof(uint32_t)
            + indices.capacity()*sizeof(uint32_t);
    }

    uint32_t size() const {
        return heap.size()-1;
    }
    bool empty() const {
        return heap.size() == 1;
    }
    uint32_t operator[](uint32_t index) const {
        return heap[1+index];
    }

    void decrease  (uint32_t n) {
        //assert(inHeap(n));
        percolateUp(indices[n]);
    }

    void insert(uint32_t n)
    {
        indices.growTo(n+1, std::numeric_limits<uint32_t>::max());
        //assert(!inHeap(n));

        indices[n] = heap.size();
        heap.push(n);
        percolateUp(indices[n]);
    }


    uint32_t removeMin()
    {
        uint32_t x       = heap[1];
        heap[1]          = heap.back();
        indices[heap[1]] = 1;
        indices[x]       = std::numeric_limits<uint32_t>::max();
        heap.pop();
        if (heap.size() > 2) {
            percolateDown(1);
        }
        return x;
    }


    void clear(bool dealloc = false)
    {
        for (uint32_t i = 1; i < heap.size(); i++) {
            indices[heap[i]] = std::numeric_limits<uint32_t>::max();
        }

        #ifndef NDEBUG
        for (uint32_t i = 0; i < indices.size(); i++) {
            assert(indices[i] == std::numeric_limits<uint32_t>::max());
        }
        #endif
        heap.clear(dealloc);
        heap.push(std::numeric_limits<uint32_t>::max());
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
    template <class F> void filter(const F& filt) {
        uint32_t i,j;
        for (i = j = 1; i < heap.size(); i++) {
            if (filt(heap[i])) {
                heap[j]          = heap[i];
                indices[heap[i]] = j++;
            } else {
                indices[heap[i]] = std::numeric_limits<uint32_t>::max();
            }
        }
        heap.shrink(i - j);

        for (int k = ((int)heap.size()) / 2 - 1; k >= 1; k--) {
            percolateDown(k);
        }
        assert(heapProperty());
    }

    bool inHeap(uint32_t n) const {
        return n < indices.size()
            && indices[n] != std::numeric_limits<uint32_t>::max();

    }

    // consistency checking
    bool heapProperty() const {
        return heapProperty(1);
    }

};

#endif
