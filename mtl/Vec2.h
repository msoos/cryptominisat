/*******************************************************************************************[Vec.h]
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

#ifndef Vec2_h
#define Vec2_h

#include <cstdlib>
#include <cassert>
#include <iostream>
#include <new>
#ifdef _MSC_VER
#include <msvc/stdint.h>
#else
#include <stdint.h>
#endif //_MSC_VER
#include <string.h>

//=================================================================================================
// Automatically resizable arrays
//
// NOTE! Don't use this vector on datatypes that cannot be re-located in memory (with realloc)


#define CACHE_NUM 20

template<class T>
class vec2 {
    uint32_t sz;
    uint32_t cap;
    T localData[CACHE_NUM];
    T*  data;

    void     grow(uint32_t min_cap);

    // Don't allow copying (error prone):
    vec2<T>&  operator = (vec2<T>& other) { assert(0); return *this; }
    vec2      (vec2<T>& other) { assert(0); }

    static inline uint32_t imax(int x, int y) {
        int mask = (y-x) >> (sizeof(int)*8-1);
        return (x&mask) + (y&(~mask)); }

public:
    class iterator : public std::iterator<std::bidirectional_iterator_tag, T> {
        public:
            explicit iterator(vec2<T>& a, uint32_t _at = 0) :
                realData(a)
                , at(_at)
            {}

            //post-increment
            iterator operator++(int a) {
                iterator tmp(*this);
                at++;
                return tmp;
            }

            iterator operator++() {
                at++;
                return *this;
            }

            iterator operator=(const iterator& other) {
                at = other.at;
                return *this;
            }

            iterator operator+(int a) const {
                return iterator(realData, at+a);
            }

            iterator operator--(int a) {
                at-=a;
                return *this;
            }

            const bool operator<(const iterator& other) const
            {
                return (at<other.at);
            }

            const bool operator==(const iterator& other) const
            {
                return (at == other.at);
            }

            const bool operator!=(const iterator& other) const {
                return (at != other.at);
            }

            const bool operator<=(const iterator& other) const
            {
                return (at<=other.at);
            }

            iterator operator--() {
                at--;
                return *this;
            }

            const int32_t operator-(const iterator& other) const {
                return at-other.at;
            }

            iterator operator-(int a) const {
                return iterator(realData, at-a);
            }

            T& operator*() {
                if (at >= CACHE_NUM) return realData.data[at-CACHE_NUM];
                else return realData.localData[at];
            }

            T* operator->() {
                if (at >= CACHE_NUM) return realData.data + (at-CACHE_NUM);
                else return realData.localData + at;
            }

        private:
            vec2<T>& realData;
            uint32_t at;
    };

    class const_iterator : public std::iterator<std::bidirectional_iterator_tag, T> {
        public:
            explicit const_iterator(const vec2<T>& a, uint32_t _at = 0) :
                realData(a)
                , at(_at)
            {}

            //post-increment
            const_iterator operator++(int a) {
                const_iterator tmp(*this);
                at++;
                return tmp;
            }

            const_iterator operator++() {
                at++;
                return *this;
            }

            const_iterator operator+(int a) const {
                return const_iterator(realData, at+a);
            }

            const_iterator operator--(int a) {
                at-=a;
                return *this;
            }

            const int32_t operator-(const const_iterator& other) const {
                return at-other.at;
            }

            const_iterator operator-(int a) const {
                return const_iterator(realData, at-a);
            }

            const bool operator==(const const_iterator& other) const {
                return (at == other.at);
            }

            const bool operator!=(const const_iterator& other) const {
                return (at != other.at);
            }

            const T& operator*() {
                if (at >= CACHE_NUM) return realData.data[at-CACHE_NUM];
                else return realData.localData[at];
            }

            const T* operator->() {
                if (at >= CACHE_NUM) return realData.data + (at-CACHE_NUM);
                else return realData.localData + at;
            }

        private:
            const vec2<T>& realData;
            uint32_t at;
    };

    // Types:
    typedef uint32_t Key;
    typedef T   Datum;

    // Constructors:
    vec2(void)            :  sz(0)   , cap(0) , data(NULL)   { }
    vec2(const vec2<T>& other);
   ~vec2(void)                                               { clear(true); }

    // iterators:
    const_iterator getData() const {return const_iterator(*this); }
    const_iterator getDataEnd() const {
        //__builtin_prefetch(data);
        return const_iterator(*this, sz);
    }
    iterator getData() {return iterator(*this); }
    iterator getDataEnd() {
        //__builtin_prefetch(data);
        return iterator(*this, sz);
    }

    // Size operations:
    void shrink_(uint32_t nelems)  {
        assert(nelems <= sz); sz -= nelems;
    }
    const uint32_t size() const    {return sz;}
    void clear  (bool dealloc);

    const bool empty() const {
        return sz == 0;
    }

    // Stack interface:
    void     push  (const T& elem)
    {
        if (sz >= CACHE_NUM) {
            grow(sz+1);
        }
        if (sz < CACHE_NUM) localData[sz] = elem;
        else data[sz-CACHE_NUM] = elem;
        sz++;
    }
};

template<class T>
void vec2<T>::grow(uint32_t min_cap) {
    if (min_cap <= CACHE_NUM) return;
    if (min_cap <= cap) return;
    if (cap == 0) cap = (min_cap >= 2) ? min_cap : 2;
    else          do cap = (cap*3+1) >> 1; while (cap < min_cap);
    data = (T*)realloc(data, cap * sizeof(T));
}

template<class T>
void vec2<T>::clear(bool dealloc)
{
    if (data != NULL){
        for (uint32_t i = 0; i != sz; i++) data[i].~T();
        sz = 0;
        if (dealloc) {
            free(data);
            data = NULL;
            cap = 0;
        }
    }
}

template<class T>
vec2<T>::vec2(const vec2<T>& other) :
    sz(0)
    , cap(0)
    , data(NULL)
{
    sz = other.sz;
    grow(sz);
    memcpy(localData, other.localData, sizeof(T)*CACHE_NUM);
    if (sz > CACHE_NUM) memcpy(data, other.data, sizeof(T)*(sz-CACHE_NUM));
}


#endif
