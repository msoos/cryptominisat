#ifndef StackAlloc_h
#define StackAlloc_h

//=================================================================================================


template<class T>
struct Allocator {
    virtual T* alloc(int nwords) = 0;
};


// STACK ALLOCATOR 
//
// 'cap' is capacity, 'lim' is malloc limit (use 'malloc()' for larger sizer 
// than this -- unless enough space left on the current stack).
//
template<class T, int cap=10000, int lim=cap/10>
class StackAlloc : public Allocator<T> {
    T*          data;
    StackAlloc* prev;
    int         index;

    T* alloc_helper(int n);
    StackAlloc(T* d, StackAlloc* p, int i) : data(d), prev(p), index(i) { }

public:
    StackAlloc(void) { data = xmalloc<T>(cap); index = 0; prev = NULL; }

    T* alloc(int n) {
        if (index + n <= cap) { T* tmp = data+index; index += n; return tmp; }
        else                    return alloc_helper(n); }

    void freeAll(void);
};


template<class T, int cap, int lim>
T* StackAlloc<T,cap,lim>::alloc_helper(int n)
{
    if (n > lim) {
        StackAlloc* singleton = new StackAlloc<T, cap, lim>(xmalloc<T>(n), prev, -1);
        prev = singleton;
        return singleton->data; }
    else{
        StackAlloc* copy = new StackAlloc<T, cap, lim>(data, prev, index);
        data = xmalloc<T>(cap); index = n; prev = copy;
        assert(n <= cap);
        return data; }
}


// Call this before allocator is destructed if you do not want to keep the data.
//
template<class T, int cap, int lim>
void StackAlloc<T,cap,lim>::freeAll(void)
{
    StackAlloc* tmp,* ptr;
    for (ptr = this; ptr != NULL; ptr = ptr->prev)
        xfree(ptr->data);
    for (ptr = prev; ptr != NULL;)
        tmp = ptr->prev, delete ptr, ptr = tmp;
}


//=================================================================================================
#endif
