#ifndef VecMaps_h
#define VecMaps_h

//=================================================================================================

// TODO: Adapt constructors of 'BitMap' and 'DeckMap' to those of VecMap.


class BitMap {
    vec<unsigned>   v;
    bool            bool_null;

    unsigned word(int index) const { return (unsigned)index / (sizeof(int)*8); }
    unsigned mask(int index) const { return 1 << ((unsigned)index % (sizeof(int)*8)); }
public:
    typedef int  Key;
    typedef bool Datum;

    BitMap(void)                    : bool_null()     { }
    BitMap(bool null)               : bool_null(null) { }
    BitMap(bool null, int capacity) : v((capacity+(sizeof(int)*8)-1) / (sizeof(int)*8), -(int)null), bool_null(null) { }

    bool at(int index) const {
        if (word(index) >= (unsigned)v.size()) return bool_null;
        else                                   return v[word(index)] & mask(index); }

    void set(int index, bool value) {
        if (word(index) >= (unsigned)v.size()) assert(index >= 0), v.growTo(word(index)+1, -(int)bool_null);
        if (value == false)
            v[word(index)] &= ~mask(index);
        else
            v[word(index)] |= mask(index); }

    void clear(void) { v.clear(); }
};


template <class T>
class VecMap {
    vec<T>  v;
    T       T_null;

public:
    typedef int Key;
    typedef T   Datum;

    VecMap(void)                 :                    T_null()     { }
    VecMap(T null)               :                    T_null(null) { }
    VecMap(T null, int capacity) : v(capacity, null), T_null(null) { }

    T at(int index) const {
        if ((unsigned)index >= (unsigned)v.size()) return T_null;
        else                                       return v[index];  }

    void set(int index, T value) {
        if ((unsigned)index >= (unsigned)v.size()) assert(index >= 0), v.growTo(index+1, T_null);
        v[index] = value; }

    void clear(void) { v.clear(); }
};

template <>
struct VecMap<bool> : BitMap {
    VecMap(void)                                             { }
    VecMap(bool null)               : BitMap(null)           { }
    VecMap(bool null, int capacity) : BitMap(null, capacity) { }
};


template <class T>
class DeckMap {
    VecMap<T>   pos, neg;

public:
    typedef int Key;
    typedef T   Datum;

    DeckMap(void)                                                            { }
    DeckMap(T null)               : pos(null)          , neg(null)           { }
    DeckMap(T null, int capacity) : pos(null, capacity), neg(null, capacity) { }

    T at(int index) const {
        if (index >= 0) return pos.at(index); else return neg.at(~index); }

    void set(int index, Datum value) {
        if (index >= 0) pos.set(index, value); else neg.set(~index, value); }

    void clear(void) { pos.clear(); neg.clear(); }
};

//=================================================================================================

#endif
