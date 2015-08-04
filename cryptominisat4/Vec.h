/*******************************************************************************************[Vec.h]
Copyright (c) 2003-2007, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

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

#ifndef Minisat_Vec_h
#define Minisat_Vec_h

#include <assert.h>
#include <limits>
#include <new>
#include <utility>
#include <string.h>
#include "constants.h"

#include "XAlloc.h"

namespace CMSat {

//=================================================================================================
// Automatically resizable arrays
//
// NOTE! Don't use this vector on _datatypes that cannot be re-located in memory (with realloc)

template<class T, class _Size = uint32_t>
class vec {
public:
    typedef _Size Size;
private:
    T*   _data;
    Size sz;
    Size cap;

    // Don't allow copying (error prone):
    vec<T>&  operator=(vec<T>& other);
             vec      (vec<T>& other);

    static inline Size max(Size x, Size y){ return (x > y) ? x : y; }

public:
    // Constructors:
    vec()                        : _data(NULL), sz(0), cap(0)    { }
    explicit vec(Size size)      : _data(NULL), sz(0), cap(0)    { growTo(size); }
    vec(Size size, const T& pad) : _data(NULL), sz(0), cap(0)    { growTo(size, pad); }
   ~vec()                                                       { clear(true); }
   vec(vec&& other)
   #ifndef _MSC_VER
   noexcept
   #endif
   {
       _data = other._data;
       sz = other.sz;
       cap = other.cap;

       other._data = NULL;
       other.sz = 0;
       other.cap = 0;
    }

    void shrink_to_fit()
    {
        T* _data2 = (T*)malloc(sz*sizeof(T));
        if (!_data2) {
            throw OutOfMemoryException();
        }
        memcpy(_data2, _data, sz*sizeof(T));
        free(_data);
        _data = _data2;
        cap = sz;
    }

   void swap(vec& other)
   {
       std::swap(_data, other._data);
       std::swap(sz, other.sz);
       std::swap(cap, other.cap);
   }

    // Pointer to first element:
    T* data       (void)           { return _data; }
    const T* data        (void) const           { return _data; }

    // Size operations:
    Size     size     (void) const   { return sz; }
    void     shrink   (Size nelems)  { shrink_(nelems); }
    void     resize   (Size nelems)  {
        if (nelems > size()) {
            growTo(nelems);
        } else if (nelems < size()) {
            shrink(size()-nelems);
        }
    }
    void     shrink_  (Size nelems)  { assert(nelems <= sz); sz -= nelems; }
    int      capacity (void) const   { return cap; }
    void     capacity (Size min_cap);
    void     growTo   (Size size);
    void     growTo   (Size size, const T& pad);
    void     clear    (bool dealloc = false);

    // Stack interface:
    void     push  (void)              { if (sz == cap) capacity(sz+1); new (&_data[sz]) T(); sz++; }
    //void     push  (const T& elem)     { if (sz == cap) capacity(sz+1); _data[sz++] = elem; }
    void     push  (const T& elem)     { if (sz == cap) capacity(sz+1); new (&_data[sz++]) T(elem); }
    void     push_ (const T& elem)     { assert(sz < cap); _data[sz++] = elem; }
    void     pop   (void)              { assert(sz > 0); sz--, _data[sz].~T(); }
    // NOTE: it seems possible that overflow can happen in the 'sz+1' expression of 'push()', but
    // in fact it can not since it requires that 'cap' is equal to INT_MAX. This in turn can not
    // happen given the way capacities are calculated (below). Essentially, all capacities are
    // even, but INT_MAX is odd.

    const T& last  (void) const        { return _data[sz-1]; }
    T&       last  (void)              { return _data[sz-1]; }

    // Vector interface:
    const T& operator [] (Size index) const { return _data[index]; }
    T&       operator [] (Size index)       { return _data[index]; }
    const T& at (Size index) const { release_assert(index > size()); return _data[index]; }
    T&       at (Size index)       { release_assert(index > size()); return _data[index]; }

    bool empty() const { return size() == 0; }

    // Duplicatation (preferred instead):
    void copyTo(vec<T>& copy) const { copy.clear(); copy.growTo(sz); for (Size i = 0; i < sz; i++) copy[i] = _data[i]; }
    void moveTo(vec<T>& dest) { dest.clear(true); dest._data = _data; dest.sz = sz; dest.cap = cap; _data = NULL; sz = 0; cap = 0; }
};


template<class T, class _Size>
void vec<T,_Size>::capacity(Size min_cap) {
    if (cap >= min_cap) return;
    Size add = max((min_cap - cap + 1) & ~1, ((cap >> 1) + 2) & ~1);   // NOTE: grow by approximately 3/2
    const Size size_max = std::numeric_limits<Size>::max();
    if ( ((size_max <= std::numeric_limits<int>::max()) && (add > size_max - cap))
    ||   (((_data = (T*)::realloc(_data, (cap += add) * sizeof(T))) == NULL) && errno == ENOMEM) )
        throw OutOfMemoryException();
 }


template<class T, class _Size>
void vec<T,_Size>::growTo(Size size, const T& pad) {
    if (sz >= size) return;
    capacity(size);
    for (Size i = sz; i < size; i++) _data[i] = pad;
    sz = size; }


template<class T, class _Size>
void vec<T,_Size>::growTo(Size size) {
    if (sz >= size) return;
    capacity(size);
    sz = size; }


template<class T, class _Size>
void vec<T,_Size>::clear(bool dealloc) {
    if (_data != NULL){
        sz = 0;
        if (dealloc) free(_data), _data = NULL, cap = 0; } }

//=================================================================================================
}

#endif
