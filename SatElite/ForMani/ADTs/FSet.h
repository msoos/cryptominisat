#ifndef FSet_h
#define FSet_h

//=================================================================================================


#include "ISet.h"

// This is an internal type to allow 'ISet' to store sets of formulas.
//
template <class T>
class FSet {
    struct FromISet { };

    ISet    s;
    FSet(ISet src, FromISet) : s(src) { }

public:
    FSet(void)                     : s() { }
    FSet(T singleton)              : s((unsigned)singleton) { }
    FSet(const T* array, int size) : s((const unsigned*)array, size) { }
    FSet(const vec<T>& elems)      : s((const vec<unsigned>&)elems) { }

    FSet(const FSet<T>& src)       : s(src.s) { }
   ~FSet(void)                     { }
    FSet<T>&       operator = (FSet<T>& src)       { s = src.s; return *this; }
    const FSet<T>& operator = (const FSet<T>& src) { s = src.s; return *this; }

    bool    empty        (void)           const { return s.empty(); }
    int     size         (void)           const { return s.size(); }
    T       operator[]   (int index)      const { return (T)s[index]; }
    bool    has          (T value)        const { return s.has((unsigned)value); }
    FSet<T>  operator +  (T value)        const { return FSet<T>(s + (unsigned)value, FromISet()); }
    FSet<T>& operator += (T value)              { s += (unsigned)value; return *this; }
    FSet<T>  operator +  (FSet<T>& other) const { return FSet<T>(s + other.s, FromISet()); }
    FSet<T>& operator += (FSet<T>& other)       { s += other.s; return *this; }

    unsigned hash    (void)                 const { return s.hash(); }
    bool operator == (const FSet<T>& other) const { return s == other.s; }

    unsigned detach (void) { return s.detach(); }

    // Debug:
    void dump(void) { s.dump(); }
};


//=================================================================================================

#endif
