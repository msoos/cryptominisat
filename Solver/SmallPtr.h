/*
Please see LICENSE-CPOL.html in the root directory for the licencing of this file.
Originally by: cppnow
Link: http://www.codeproject.com/KB/cpp/smallptr.aspx
*/

#ifndef __SMALL_PTR_H__
#define __SMALL_PTR_H__

//#include <boost/static_assert.hpp>

#include <cstring>
#include <stdlib.h>
#include "singleton.hpp"
//#include <boost/thread/mutex.hpp>
//#include <boost/thread/locks.hpp>

#ifdef _MSC_VER
#include <msvc/stdint.h>
#else
#include <stdint.h>
#endif //_MSC_VER

#define is_n_aligned(T, N) ((sizeof(T) % (N)) == 0)

#ifdef WIN32
#  ifdef WIN64
#    define USE64
#  else
#    define USE32
#  endif
#endif

#ifdef __GNUG__
#  if defined(__amd64__) || defined(__x86_64__)
#    define USE64
#  elif defined(__i386__)
#    define USE32
#  endif
#endif

#include <exception>

class bad_alignment : public std::exception
{
public:
    bad_alignment(const char *const& w)  {}
};
class bad_segment : public std::exception
{
public:
    bad_segment(const char *const& w)  {}
};

class sptr_base
{   
protected:
    static const uint32_t ALIGNMENT_BITS = 2;
    static const uint32_t ALIGNMENT = (1<<ALIGNMENT_BITS);
    static const uintptr_t ALIGNMENT_MASK = ALIGNMENT - 1;

protected:
    static uintptr_t     _seg_map[ALIGNMENT];
    static uintptr_t     _segs;
    //static boost::mutex  _m;

    inline static uintptr_t ptr2seg(uintptr_t p)
    {
        p &= ~0xFFFFFFFFULL; // Keep only high part
        uintptr_t s = _segs;
        uintptr_t i = 0;
        for (; i < s; ++i)
            if (_seg_map[i] == p)
                return i;

        // Not found - now we do it the "right" way (mutex and all)
        //boost::lock_guard<boost::mutex> lock(_m);
        for (i = 0; i < s; ++i)
            if (_seg_map[i] == p)
                return i;

        i = _segs++;
        if (_segs > ALIGNMENT) {
            //throw bad_segment("Segment out of range");
            exit(-1);
        }

        _seg_map[i] = p;
        return i;
    }

};

template<class TYPE>
class sptr : public sptr_base
{
public:
    typedef TYPE  type;
    typedef TYPE* native_pointer_type;
    typedef const TYPE* const_native_pointer_type;

    sptr() throw()
        : _ptr(0)
    {}

    // Copy constructor
    sptr(const sptr<TYPE>& o) throw() 
        : _ptr(o._ptr)
    {
    }

    // Copy from a related pointer type
    // Although just copying _ptr would be more efficient - it may
    // also be wrong - e.g. multiple inheritence
    template<class O>
    sptr(const sptr<O>& o)
        : _ptr(encode(static_cast<TYPE*>(o.get())))
    {
    }

    template<class O>
    sptr(const O* p)
        : _ptr(encode(static_cast<const TYPE*>(p)))
    {
    }

    sptr<TYPE>& operator=(const sptr<TYPE>& o) throw()
    {
        _ptr = o._ptr;
        return *this;
    }

    template<class O>
    sptr<TYPE>& operator=(const sptr<O>& o)
    {
        _ptr = encode(static_cast<const TYPE*>(o.get()));
        return *this;
    }

private:
    inline uint32_t encode(const_native_pointer_type ptr) const
    {
#ifdef USE64

        uintptr_t p = reinterpret_cast<uintptr_t>(ptr);

        if ((p & ALIGNMENT_MASK) != 0) {
            //throw bad_alignment("Pointer is not aligned");
            exit(-1);
        }

        return (uint32_t)(ptr2seg(p) + p);
    
#else // 32 bit machine
        return reinterpret_cast<uint32_t>(ptr);
#endif
    }

    inline native_pointer_type decode(uint32_t e) const
    {
#ifdef USE64
        uintptr_t el = e;
        uintptr_t ptr = (_seg_map[el & ALIGNMENT_MASK] + el) & ~ALIGNMENT_MASK;

        return reinterpret_cast<native_pointer_type>(ptr);
#else
        return reinterpret_cast<native_pointer_type>(e);
#endif
    }

    void check_alignment() const
    {
        //BOOST_STATIC_ASSERT(is_n_aligned(TYPE, ALIGNMENT));
    }

    void inc_sptr(uint32_t& e, uintptr_t offset = 1)
    {
        check_alignment();
#ifdef USE64
        uintptr_t el = e;
        uintptr_t seg = el & ALIGNMENT_MASK;
        el += offset*ALIGNMENT;

        // check for overflow
        if (el > 0xFFFFFFFFULL)
            return encode(decode(e)+1);
        e = (uint32_t)el;
#else
        e+= (uint32_t)offset;
#endif
    }

    void dec_sptr(uint32_t& e, size_t offset = 1)
    {
        check_alignment();
#ifdef USE64
        uintptr_t el = e;
        uintptr_t seg = el & ALIGNMENT_MASK;
        e-= offset*ALIGNMENT;

        // check for underflow
        if (el > 0xFFFFFFFFULL)
            return encode(decode(e)-1);
        e = (uint32_t)el;
#else
        e -= (uint32_t)offset;
#endif
    }
public:

    TYPE* get() const throw() { return decode(_ptr); }

    // Pointer operators

    TYPE& operator*() { return *decode(_ptr); }
    const TYPE& operator*() const { return *decode(_ptr); }

    TYPE* operator->() { return decode(_ptr); }
    const TYPE* operator->() const { return decode(_ptr); }

    template<class O>
    bool operator==(const sptr<O>& o) const
    {
        return o._ptr == this->_ptr;
    }

    operator TYPE*() const { return get(); }

    sptr<TYPE>& operator++( )
    {
        inc_sptr(_ptr);
        return *this;
    }

    sptr<TYPE> operator++( int )
    {
        sptr<TYPE> p = *this;
        inc_sptr(_ptr);
        return p;
    }

    sptr<TYPE>& operator--( )
    {
        dec_sptr(_ptr);
        return *this;
    }
    sptr<TYPE> operator--( int )
    {
        sptr<TYPE> p = *this;
        dec_sptr(_ptr);
        return p;
    }

    sptr<TYPE>& operator+=(size_t offset)
    {
        inc_sptr(_ptr, offset);
        return *this;
    }

    sptr<TYPE>& operator-=(size_t offset)
    {
        dec_sptr(_ptr, offset);
        return *this;
    }

private:
    uint32_t _ptr;
};


#endif

