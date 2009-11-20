#ifndef Global_h
#define Global_h

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <cfloat>
#include <new>
#include <memory>
using namespace std;
// NOTE: Machine dependent lines have been tagged "<=MD"


//=================================================================================================
// Macros:


// Misc:

#define elemsof(x) (sizeof(x) / sizeof(*(x)))

#define macro static inline

#ifndef PANIC   // (makefile may define PANIC to eg. 'throw message')
#define PANIC(message) ( fflush(stdout), \
                         fprintf(stderr, "PANIC! %s\n", message), fflush(stderr), \
                         assert(false) )
#endif
#define Ping  (fflush(stdout), fprintf(stderr, "%s %d\n", __FILE__, __LINE__), fflush(stderr))


// Debug:

#define pr(format, args...) (fflush(stdout), fprintf(stderr, format, ## args), fflush(stderr))
#define ss(obj) stempf(show(obj))


// Some GNUC extensions:

#ifdef __GNUC__
#define ___noreturn    __attribute__ ((__noreturn__))
#define ___unused      __attribute__ ((__unused__))
#define ___const       __attribute__ ((__const__))
#define ___format(type,fmt,arg) __attribute__ ((__format__(type, fmt, arg) ))
#if (__GNUC__ > 2) || (__GNUC__ >= 2 && __GNUC_MINOR__ > 95)
#define ___malloc      __attribute__ ((__malloc__))
#else
#define ___malloc
#endif
#else
#define ___noreturn
#define ___unused
#define ___const
#define ___format(type,fmt,arg)
#define ___malloc
#endif

#define INITIALIZER(tag) struct Initializer_ ## tag {  Initializer_ ## tag(); } static Initializer_ ## tag ## _instance; Initializer_ ## tag:: Initializer_ ## tag(void)
#define FINALIZER(  tag) struct Finalizer_   ## tag { ~Finalizer_   ## tag(); } static Finalizer_   ## tag ## _instance; Finalizer_   ## tag::~Finalizer_   ## tag(void)


//=================================================================================================
// Templates / meta-programming:


#if (__GNUC__ > 3) || (__GNUC__ >= 3 && __GNUC_MINOR__ >= 4)
//#define TEMPLATE_FAIL (fprintf(stderr, "TEMPLATE FAIL!\n"), exit(1))
template <bool> struct STATIC_ASSERTION_FAILURE;
template <> struct STATIC_ASSERTION_FAILURE<true>{};
#define TEMPLATE_FAIL STATIC_ASSERTION_FAILURE<false>()
#else
#define TEMPLATE_FAIL TemplateFail()
#endif

#define ASSERT_SUBTYPE(child, base) \
    static child * const __dummy_child = NULL; \
    static base  * const __dummy_base  = __dummy_child;

#define AS(type) (*static_cast<type*>(this))        // (dangerous trick)

struct True_T  { };
struct False_T { };
struct Undef_T { };

#define COMMA ,

#define DEFAULT_EXTEND(Class, Method, RetType, Formals, Params, Dummies, Default) \
    static Undef_T Method(Formals) { return Undef_T(); } \
    template <class Parent> \
    struct Extend_ ## Class : Parent { \
        RetType new_ ## Method(RetType* , Formals) { return Method(Params); } \
        RetType new_ ## Method(Undef_T* , Formals) { Default } \
        RetType new_ ## Method(Formals) { \
            return new_ ## Method((typeof(peek(Dummies))*)NULL, Params); } \
    };


//=================================================================================================
// Basic types:


typedef signed char   schar;
typedef unsigned char uchar;
typedef unsigned int  uint;
typedef unsigned long ulong;

typedef const char cchar;

typedef short              int16;
typedef unsigned short     uint16;
typedef int                int32;
typedef unsigned           uint32;
#ifdef __GNUC__
typedef long long          int64;
typedef unsigned long long uint64;
typedef __PTRDIFF_TYPE__   intp;
typedef unsigned __PTRDIFF_TYPE__ uintp;
#define I64_fmt "lld"
#else
// (assume Windows compiler)
typedef INT64              int64;
typedef UINT64             uint64;
typedef INT_PTR            intp;
typedef UINT_PTR           uintp;
#define I64_fmt "I64d"
#endif

#ifdef __LP64__
#define LP64        // LP : int=32 long,ptr=64 (unix model -- the windows model is LLP : int,long=32 ptr=64)
#endif


//=================================================================================================
// 'malloc()'-style memory allocation -- never returns NULL; aborts instead:


#ifdef NoExceptions
  #define memAssert(cond) assert(cond)
#else
  class Exception_MemOut {};
  #define memAssert(cond) if (!(cond)) throw Exception_MemOut()
#endif

template<class T> macro T* xmalloc(size_t size) ___malloc;
template<class T> macro T* xmalloc(size_t size) {
    T*   tmp = (T*)malloc(size * sizeof(T));
    memAssert(size == 0 || tmp != NULL);
    return tmp; }

template<class T> macro T* xrealloc(T* ptr, size_t size) ___malloc;
template<class T> macro T* xrealloc(T* ptr, size_t size) {
    T*   tmp = (T*)realloc((void*)ptr, size * sizeof(T));
    memAssert(size == 0 || tmp != NULL);
    return tmp; }

template<class T> macro void xfree(T* ptr) {
    if (ptr != NULL) free((void*)ptr); }


//=================================================================================================
// Bit macros:


// Signed min/max -- These are only 31 bit safe (or 63 bits; one less than the word size).
// To get the 32nd bit (64th) right you need "subtract with borrow", which C does not support.
//
macro int imin(int x, int y) {
    int mask = (x-y) >> (sizeof(int)*8-1);
    return (x&mask) + (y&(~mask)); }

macro int imax(int x, int y) {
    int mask = (y-x) >> (sizeof(int)*8-1);
    return (x&mask) + (y&(~mask)); }

macro int isign(int x) {
    return (x >> (sizeof(int)*8-1)) & 1; }


//=================================================================================================
// Automatic disposal of C arrays:


template <class T>
class Free {
    T*  vec;
public:
    Free(T* vec_) { vec = vec_; }
   ~Free(void)    { free(vec);  }
    operator T* (void) { return vec; }
};

#define sFree(cstr) ((cchar*)Free<char>(cstr))   // (string disposal (cchar*)) -- NOTE! Unsafe in new C++ standard!


//=================================================================================================
// Strings and Show:


macro char* Xstrdup(cchar* src) ___malloc;
macro char* Xstrdup(cchar* src) {
    int     size = strlen(src)+1;
    char*   tmp = xmalloc<char>(size);
    memcpy(tmp, src, size);
    return tmp; }
#define xstrdup(s) Xstrdup(s)

macro char* xstrndup(cchar* src, int len) ___malloc;
macro char* xstrndup(cchar* src, int len) {
    int     size; for (size = 0; size < len && src[size] != '\0'; size++);
    char*   tmp = xmalloc<char>(size+1);
    memcpy(tmp, src, size); tmp[size] = '\0';
    return tmp; }

#include "String.h"

char* nsprintf (cchar* format, ...) ___format(printf, 1, 2) ___malloc;
char* strnum   (int num) ___malloc;
char* strtokr  (char** src, cchar* seps);
int   fileSize (cchar* filename);
char* readFile (cchar* filename, int* size_ = NULL);
char* readFile (FILE* in, int* size_ = NULL);
char* extendedReadFile(cchar* sources, int* size = NULL, bool output_error = true);
void  writeFile(cchar* filename, cchar* data, int size);

#define tempf(format, args...) sFree(nsprintf(format , ## args))

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <class T> macro String show(const T& t) { return t.show(); }

macro String show(int x)    { return sown(strnum(x)); }
macro String show(cchar* x) { return scopy(x); }
macro String show(char* x)  { return scopy(x); }

#define showf(format, args...) sown(nsprintf(format , ## args))


//=================================================================================================
// Random numbers:


// Returns a random float 0 <= x < 1. Seed must never be 0.
macro double drand(double& seed) {
    seed *= 1389796;
    int q = (int)(seed / 2147483647);
    seed -= (double)q * 2147483647;
    return seed / 2147483647; }

// Returns a random integer 0 <= x < size. Seed must never be 0.
macro int irand(double& seed, int size) {
    return (int)(drand(seed) * size); }


//=================================================================================================
// 'Pair':


template <class Fst, class Snd>
struct Pair {
    typedef Fst Fst_t;
    typedef Snd Snd_t;

    Fst     fst;
    Snd     snd;

    Pair(void) { }
    Pair(const Fst& x, const Snd& y) : fst(x), snd(y) { }

    template <class FstCompat, class SndCompat>
    Pair(const Pair<FstCompat, SndCompat>& p) : fst(p.fst), snd(p.snd) { }

    void split(Fst& out_fst, Snd& out_snd) { out_fst = fst; out_snd = snd; }
};


template <class Fst, class Snd>
inline bool operator == (const Pair<Fst, Snd>& x, const Pair<Fst, Snd>& y) {
    return (x.fst == y.fst) && (x.snd == y.snd); }

template <class Fst, class Snd>
inline bool operator < (const Pair<Fst, Snd>& x, const Pair<Fst, Snd>& y) {
    return x.fst < y.fst ||
           (!(y.fst < x.fst) && x.snd < y.snd); }

template <class Fst, class Snd>
inline Pair<Fst, Snd> Pair_new(const Fst& x, const Snd& y) {
    return Pair<Fst, Snd>(x, y); }


//=================================================================================================
// 'vec' -- automatically resizable arrays (via 'push()' method):


// NOTE! Don't use this vector on datatypes that cannot be moved with 'memcpy'.

template<class T>
class vec {
    T*  data;
    int sz;
    int cap;

    void     init(int size, const T& pad);
    void     grow(int min_cap);

public:
    // Types:
    typedef int Key;
    typedef T   Datum;

    // Constructors:
    vec(void)                   : data(NULL) , sz(0)   , cap(0)    { }
    vec(int size)               : data(NULL) , sz(0)   , cap(0)    { growTo(size); }
    vec(int size, const T& pad) : data(NULL) , sz(0)   , cap(0)    { growTo(size, pad); }
    vec(T* array, int size)     : data(array), sz(size), cap(size) { }      // (takes ownership of array -- will be deallocated with 'xfree()')
   ~vec(void)                                                      { clear(true); }

    // Ownership of underlying array:
    T*       release  (void)           { T* ret = data; data = NULL; sz = 0; cap = 0; return ret; }
    operator T*       (void)           { return data; }     // (unsafe but convenient)
    operator const T* (void) const     { return data; }

    // Size operations:
    int      size   (void) const       { return sz; }
    void     shrink (int nelems)       { assert(nelems <= sz); for (int i = 0; i < nelems; i++) sz--, data[sz].~T(); }
    void     pop    (void)             { sz--, data[sz].~T(); }
    void     growTo (int size);
    void     growTo (int size, const T& pad);
    void     clear  (bool dealloc = false);

    // Stack interface:
    void     push  (void)              { if (sz == cap) grow(sz+1); new (&data[sz]) T()    ; sz++; }
    void     push  (const T& elem)     { if (sz == cap) grow(sz+1); new (&data[sz]) T(elem); sz++; }
    const T& last  (void) const        { return data[sz-1]; }
    T&       last  (void)              { return data[sz-1]; }

    // Vector interface:
  #ifdef PARANOID
    const T& operator [] (int index) const  { assert((uint)index < (uint)sz); return data[index]; }
    T&       operator [] (int index)        { assert((uint)index < (uint)sz); return data[index]; }
  #else
    const T& operator [] (int index) const  { return data[index]; }
    T&       operator [] (int index)        { return data[index]; }
  #endif

    // Don't allow copying:
    vec<T>&  operator = (vec<T>& other) { TEMPLATE_FAIL; }
             vec        (vec<T>& other) { TEMPLATE_FAIL; }

    // Duplicatation:
    void copyTo(vec<T>& copy) const { copy.clear(); copy.growTo(sz); for (int i = 0; i < sz; i++) new (&copy[i]) T(data[i]); }
    void moveTo(vec<T>& dest) { dest.clear(true); dest.data = data; dest.sz = sz; dest.cap = cap; data = NULL; sz = 0; cap = 0; }

    // Show:
    String   show(void) const {
        String  result = showf("%d/%d @ %p :", sz, cap, data);
        for (int i = 0; i < sz; i++) result = result + sref(" ") + sref("{") + ::show(data[i]) + sref("}");
        return result; }
};

template<class T>
void vec<T>::grow(int min_cap) {
    if (min_cap <= cap) return;
    if (cap == 0) cap = (min_cap >= 16) ? min_cap : 16;
    else          do cap = cap*2 + 16; while (cap < min_cap);
    data = xrealloc(data, cap); }

template<class T>
void vec<T>::growTo(int size, const T& pad) {
    if (sz >= size) return;
    grow(size);
    for (int i = sz; i < size; i++) new (&data[i]) T(pad);
    sz = size; }

template<class T>
void vec<T>::growTo(int size) {
    if (sz >= size) return;
    grow(size);
    for (int i = sz; i < size; i++) new (&data[i]) T();
    sz = size; }

template<class T>
void vec<T>::clear(bool dealloc) {
    if (data != NULL){
        for (int i = 0; i < sz; i++) data[i].~T();
        sz = 0;
        if (dealloc) xfree(data), data = NULL, cap = 0; } }


//=================================================================================================
// Relation operators -- extend definitions from '==' and '<'


#ifndef __SGI_STL_INTERNAL_RELOPS   // (be aware of SGI's STL implementation...)
#define __SGI_STL_INTERNAL_RELOPS
template <class T> macro bool operator != (const T& x, const T& y) { return !(x == y); }
template <class T> macro bool operator >  (const T& x, const T& y) { return y < x;     }
template <class T> macro bool operator <= (const T& x, const T& y) { return !(y < x);  }
template <class T> macro bool operator >= (const T& x, const T& y) { return !(x < y);  }
#endif


//=================================================================================================
// Lifted booleans:


class lbool {
    int     value;


public:
    explicit lbool(int v) : value(v) { } 
    lbool()       : value(0) { }
    lbool(bool x) : value((int)x*2-1) { }
    int toInt(void) const { return value; }

    bool  operator == (const lbool& other) const { return value == other.value; }
    bool  operator != (const lbool& other) const { return value != other.value; }
    lbool operator ~  (void)               const { return lbool(-value); }

    friend int   toInt  (lbool l) { return l.toInt(); }
    //    friend lbool toLbool(int   v);
    friend char  name   (lbool l) { static char name[4] = {'!','0','?','1'}; int x = l.value; x = 2 + ((x >> (sizeof(int)*8-2)) | (x & ~(1 << (sizeof(int)*8-1)))); return name[x]; }
};

//lbool toLbool(int   v) { return lbool(v);  }

const lbool l_True  = lbool(1);//toLbool( 1);
const lbool l_False = lbool(-1);//toLbool(-1);
const lbool l_Undef = lbool(0);//toLbool( 0);
const lbool l_Error = lbool(1 << (sizeof(int)*8-1));//toLbool(1 << (sizeof(int)*8-1));




//=================================================================================================
// Resource usage:


#include <sys/resource.h>
#include <sys/time.h>
#include <sys/timex.h>

macro double cpuTime(void) {
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return (double)ru.ru_utime.tv_sec + (double)ru.ru_utime.tv_usec / 1000000; }

macro double realTime(void) {
    struct ntptimeval t;
    ntp_gettime(&t);
    return (double)t.time.tv_sec + (double)t.time.tv_usec / 1000000; }

int64 memUsed    (void);
int64 memResident(void);
int64 memPhysical(void);


//=================================================================================================
// Other:


template <class T>
macro void swp(T& x, T& y) {        // 'swap' is used by STL
    T tmp = x; x = y; y = tmp; }


template <class T>
class Restore {
    T*  variable_ptr;
    T   old_value;
public:
    Restore(T& x) : variable_ptr(&x), old_value(x) {}
   ~Restore(void) { *variable_ptr = old_value; }
};


//=================================================================================================

#endif
