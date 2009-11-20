#ifndef __HEAP__
#define __HEAP__

static inline int left (int i)  { return i * 2; }
static inline int right(int i)  { return i * 2 + 1; }
static inline int parent(int i) { return i / 2; }

template<class C>
class Heap {
 public:
    C        comp;
    vec<int> heap;     // heap of ints
    vec<int> indices;  // int -> index in heap

    inline void percolateUp(int i)
    {
        int x = heap[i];
        while (parent(i) != 0 && comp(x,heap[parent(i)])){
            heap[i]          = heap[parent(i)];
            indices[heap[i]] = i;
            i                = parent(i);
        }
        heap[i]     = x;
        indices[x]  = i;
    }

    inline void percolateDown(int i)
    {
        int x = heap[i];
        while (left(i) < heap.size()){
            int child = right(i) < heap.size() && comp(heap[right(i)],heap[left(i)]) ? right(i) : left(i);
            if (comp(x,heap[child])) break;
            heap[i]          = heap[child];
            indices[heap[i]] = i;
            i                = child;
        }
        heap[i]     = x;
        indices[x]  = i;
    }

    bool ok(int n) { return n >= 0 && n < (int)indices.size(); }

 public:
    Heap(C c) : comp(c) { heap.push(-1); }

    void setBounds (int size) { assert(size >= 0); indices.growTo(size,0); }
    bool inHeap    (int n)    { assert(ok(n)); return indices[n] != 0; }
    void increase  (int n)    { assert(ok(n)); assert(inHeap(n)); percolateUp(indices[n]); }
    bool empty     (void)     { return heap.size() == 1; }

    void insert(int n) {
        assert(ok(n));
        indices[n] = heap.size();
        heap.push(n);
        percolateUp(indices[n]);
    }

    int  getmin(void) {
        int r = heap[1];
        int q = heap[1] = heap.last(); heap.pop();
        indices[r] = 0;
        indices[q] = 1;
        if (heap.size() > 1) percolateDown(1);
        return r;
    }

    int  peekmin(void) { return heap[1]; }

    bool heapProperty(void) { return heapProperty(1); }
    bool heapProperty(int i) {
    return (size_t)i >= heap.size() ||
        ((parent(i) == 0 || !comp(heap[i],heap[parent(i)])) &&
         heapProperty(left(i)) && heapProperty(right(i)));
    }
};

#endif
