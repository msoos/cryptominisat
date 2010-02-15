#ifndef Hash_standard_h
#define Hash_standard_h


//=================================================================================================
// Some Primes...


static int prime_twins[25] = { 31, 73, 151, 313, 643, 1291, 2593, 5233, 10501, 21013, 42073, 84181, 168451, 337219, 674701, 1349473, 2699299, 5398891, 10798093, 21596719, 43193641, 86387383, 172775299, 345550609, 691101253 };


//=================================================================================================
// Standard hash parameters:


template <class K> struct Hash  { uint operator () (const K& key)                 const { return key.hash(); } };
template <class K> struct Equal { bool operator () (const K& key1, const K& key2) const { return key1 == key2; } };

template <class K> struct Hash_params {
    static uint hash (K key)          { return Hash <K>()(key);        }
    static bool equal(K key1, K key2) { return Equal<K>()(key1, key2); }
};


//=================================================================================================


// DEFINE PATTERN MACRO:
//
// 'code' should use 'key' (hash) or 'key1'/'key2' (equal) to access the elements. The last statement should be a return.
//
#define DefineHash( type, code) template <> struct Hash<type>  { uint operator () (type key) const { code } };
#define DefineEqual(type, code) template <> struct Equal<type> { bool operator () (type key1, type key2) const { code } };


// PAIRS:

template <class S, class T> struct Hash <Pair<S,T> > { uint operator () (const Pair<S,T>& key)                         const { return Hash<S>()(key.fst) ^ Hash<T>()(key.snd); } };
template <class S, class T> struct Equal<Pair<S,T> > { bool operator () (const Pair<S,T>& key1, const Pair<S,T>& key2) const { return Equal<S>()(key1.fst, key2.fst) && Equal<T>()(key1.snd, key2.snd); } };


// POINTERS:

#ifdef LP64
template <class K> struct Hash<const K*> { uint operator () (const K* key) const { uintp tmp = reinterpret_cast<uintp>(key); return (unsigned)((tmp >> 32) ^ tmp); } };
template <class K> struct Hash<K*>       { uint operator () (K* key)       const { uintp tmp = reinterpret_cast<uintp>(key); return (unsigned)((tmp >> 32) ^ tmp); } };
#else
template <class K> struct Hash<const K*> { uint operator () (const K* key) const { return reinterpret_cast<uint>(key); } };
template <class K> struct Hash<K*>       { uint operator () (K* key)       const { return reinterpret_cast<uint>(key); } };
#endif

// C-STRINGS:

DefineHash (const char*, uint v = 0; for (int i = 0; key[i] != '\0'; i++) v = (v << 3) + key[i]; return v;)
DefineEqual(const char*, if (key1 == key2) return true; else return strcmp(key1, key2) == 0;)
DefineHash (char*, uint v = 0; for (int i = 0; key[i] != '\0'; i++) v = (v << 3) + key[i]; return v;)
DefineEqual(char*, if (key1 == key2) return true; else return strcmp(key1, key2) == 0;)


// INTEGER TYPES:

DefineHash(char  , return (uint)key;)
DefineHash(schar , return (uint)key;)
DefineHash(uchar , return (uint)key;)
DefineHash(short , return (uint)key;)
DefineHash(ushort, return (uint)key;)
DefineHash(int   , return (uint)key;)
DefineHash(uint  , return (uint)key;)
#ifdef LP64
DefineHash(long  , return (uint)(((ulong)key >> 32) ^ key);)
DefineHash(ulong , return (uint)(((ulong)key >> 32) ^ key);)
#else
DefineHash(long  , return (uint)key;)
DefineHash(ulong , return (uint)key;)
#endif
DefineHash(int64 , return (uint)(((uint64)key >> 32) ^ key);)
DefineHash(uint64, return (uint)(((uint64)key >> 32) ^ key);)


//=================================================================================================

#endif
