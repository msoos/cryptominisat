/************************************************************************
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
********************************************************************/

#ifndef Heap_h
#define Heap_h

#include "constants.h"
#include <assert.h>
#include <algorithm>
#include <vector>
using std::vector;

// A heap implementation with support for decrease/increase key.
template<class Comp>
class Heap {
    Comp lt;
    vector<uint32_t> heap;     // heap of ints
    vector<uint32_t> indices;  // int -> index in heap

    // Index "traversal" functions
    static inline uint32_t left  (uint32_t i) { return i*2; }
    static inline uint32_t right (uint32_t i) { return i*2+1; }
    static inline uint32_t parent(uint32_t i) { return i/2; }

    inline void percolate_up(uint32_t i)
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

    inline void percolate_down(uint32_t i)
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
    bool heap_property (uint32_t i) const {
        return i >= heap.size()
            || ( (i == 1 || !lt(heap[i], heap[parent(i)]))
                  && heap_property( left(i)  )
                  && heap_property( right(i) )
               );
    }

  public:
    Heap(const Comp& c) :
        lt(c)
    {
        heap.push_back(std::numeric_limits<uint32_t>::max());
    }

    Heap(const Heap<Comp>& other) : lt(other.lt) {
        heap.resize(other.heap.size());
        std::copy(other.heap.begin(), other.heap.end(), heap.begin());
        indices.resize(other.indices.size());
        std::copy(other.indices.begin(), other.indices.end(), indices.begin());
    }

    template<class T>
    uint32_t random_element(T& rand) const
    {
        assert(!empty());
        size_t at = rand.randInt(heap.size()-2);
        return heap[at+1];
    }

    void operator=(const Heap<Comp>& other)
    {
        heap.resize(other.heap.size());
        std::copy(other.heap.begin(), other.heap.end(), heap.begin());

        indices.resize(other.indices.size());
        std::copy(other.indices.begin(), other.indices.end(), indices.begin());
        return *this;
    }

    size_t mem_used() const
    {
        size_t mem = 0;
        mem += heap.capacity()*sizeof(uint32_t);
        mem += indices.capacity()*sizeof(uint32_t);
        return mem;
    }

    bool empty() const {
        return heap.size() == 1;
    }

    void decrease  (uint32_t n) {
        //assert(in_heap(n));
        percolate_up(indices[n]);
    }

    void insert(uint32_t n)
    {
        if (indices.size() <= n) {
            indices.resize(n+1, std::numeric_limits<uint32_t>::max());
        }
        //assert(!in_heap(n));

        indices[n] = heap.size();
        heap.push_back(n);
        percolate_up(indices[n]);
    }


    uint32_t remove_min()
    {
        uint32_t x       = heap[1];
        heap[1]          = heap.back();
        indices[heap[1]] = 1;
        indices[x]       = std::numeric_limits<uint32_t>::max();
        heap.pop_back();
        if (heap.size() > 2) {
            percolate_down(1);
        }
        return x;
    }


    void clear(bool dealloc = false)
    {
        indices.clear();
        if (dealloc) {
            indices.shrink_to_fit();
        }

        heap.clear();
        heap.push_back(std::numeric_limits<uint32_t>::max());
        if (dealloc) {
            heap.shrink_to_fit();
        }
    }

    // Fool proof variant of insert/decrease/increase
    void update_if_inside(uint32_t n)
    {
        if (in_heap(n)) {
            percolate_up(indices[n]);
            percolate_down(indices[n]);
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
        heap.resize(heap.size()-(i - j));

        for (long k = ((long)heap.size()) / 2; k >= 1; k--) {
            percolate_down(k);
        }
        assert(heap_property());
    }

    bool in_heap(uint32_t n) const {
        return n < indices.size()
            && indices[n] != std::numeric_limits<uint32_t>::max();

    }

    // consistency checking
    bool heap_property() const {
        return heap_property(1);
    }

};

#endif
