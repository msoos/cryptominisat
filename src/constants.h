/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

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
***********************************************/

#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <limits>
#include <random>

using std::cerr;
using std::exit;

#define unif_uint_dist(x,y) std::uniform_int_distribution<uint32_t> x(0, y)
inline uint32_t rnd_uint(std::mt19937_64& mtrand, const uint32_t maximum_inclusive) {
    unif_uint_dist(u, maximum_inclusive);
    return u(mtrand);
}


#if defined(STATS_NEEDED)
#define LARGE_OFFSETS
#endif

// #define VERBOSE_DEBUG
// #define VERBOSE_DEBUG_FULLPROP
// #define DEBUG_DEPTH
// #define SLOW_DEBUG
// #define DEBUG_ATTACH_FULL
/* #define DEBUG_FRAT */

#ifndef NDEBUG
//#define FAST_DEBUG
#endif

#ifdef SLOW_DEBUG
#define FAST_DEBUG
#define DEBUG_PROPAGATEFROM
#define ENQUEUE_DEBUG
#define DEBUG_ATTACH_MORE
#define DEBUG_IMPLICIT_PAIRS_TRIPLETS
#define DEBUG_IMPLICIT_STATS
#define DEBUG_GAUSS
#define XOR_DEBUG
#endif

#ifdef FAST_DEBUG
#define DEBUG_VARELIM
#define DEBUG_WATCHED
#define DEBUG_ATTACH
#define DEBUG_REPLACER
#define DEBUG_MARKED_CLAUSE
#define CHECK_N_OCCUR
#endif

#if defined(_MSC_VER)
#define cmsat_prefetch(x) (void)(x)
#else
#define cmsat_prefetch(x) __builtin_prefetch(x)
#endif

#if defined(_MSC_VER)
#include "cms_windows_includes.h"
#define release_assert(a) \
    do { \
    __pragma(warning(push)) \
    __pragma(warning(disable:4127)) \
        if (!(a)) {\
    __pragma(warning(pop)) \
            fprintf(stderr, "*** ASSERTION FAILURE in %s() [%s:%d]: %s\n", \
            __FUNCTION__, __FILE__, __LINE__, #a); \
            abort(); \
        } \
    } while (0)
#else
#define release_assert(a) \
    do { \
        if (!(a)) {\
            fprintf(stderr, "*** ASSERTION FAILURE in %s() [%s:%d]: %s\n", \
            __FUNCTION__, __FILE__, __LINE__, #a); \
            abort(); \
        } \
    } while (0)
#endif

//We shift stuff around in Watched, so not all of 32 bits are useable.
//for STATS we have 64b values in the Clauses, so they must be aligned to 64

#if defined(STATS_NEEDED)
#define STATS_DO(x) do {x;} while (0)
#define INC_ID(cl) \
    do { \
        auto prev_id = (cl).stats.ID; \
        (cl).stats.ID = ++solver->clauseID; \
        if (solver->sqlStats && (cl).stats.is_tracked) solver->sqlStats->update_id(prev_id, (cl).stats.ID); \
    } while (0)
#else
#define STATS_DO(x) do {} while (0)
#define INC_ID(cl) do { (cl).stats.ID = ++solver->clauseID; } while (0)
#endif
// NOTE: XID's are not tracked during stats -- we must have XOR finding etc disabled
#define INC_XID(x) do { (x).XID = ++solver->clauseXID; } while (0)

#if defined(LARGE_OFFSETS)
#define BASE_DATA_TYPE uint64_t
#define EFFECTIVELY_USEABLE_BITS 62
#else
#define BASE_DATA_TYPE uint32_t
#define EFFECTIVELY_USEABLE_BITS 30
#endif

#define MAX_XOR_RECOVER_SIZE 8

#if defined _WIN32
    #define DLL_PUBLIC __declspec(dllexport)
#else
    #define DLL_PUBLIC __attribute__ ((visibility ("default")))
    #define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
#endif

///////////////////
// Verbose Debug
///////////////////

//#define FRAT_DEBUG
//#define VERBOSE_DEBUG

#ifdef DEBUG_FRAT
#define frat_func_start *solver->frat << __PRETTY_FUNCTION__ << " start\n"
#define frat_func_start_raw *frat << __PRETTY_FUNCTION__ << " start\n"
#define frat_func_end *solver->frat << __PRETTY_FUNCTION__ << " end\n"
#define frat_func_end_with(txt) *solver->frat << __PRETTY_FUNCTION__ << " --- " << txt << " end\n"
#define frat_func_end_raw *frat << __PRETTY_FUNCTION__ << " end\n"
#else
#define frat_func_start() do { } while (0)
#define frat_func_start_raw() do { } while (0)
#define frat_func_end() do { } while (0)
#define frat_func_end_with(x) do { } while (0)
#define frat_func_end_raw() do { } while (0)
#endif

#ifdef VERBOSE_DEBUG
#define VERBOSE_PRINT(x) do { std::cout << x << std::endl; } while (0)
#define VERBOSE_DEBUG_DO(x) do { x; } while (0)
#else
#define VERBOSE_PRINT(x) do { } while (0)
#define VERBOSE_DEBUG_DO(x) do { } while (0)
#endif

#ifdef USE_BREAKID
#define USE_BREAKID_DO(x) do { x; } while (0)
#else
#define USE_BREAKID_DO(x) do { } while (0)
#endif

#ifdef SLOW_DEBUG
#define SLOW_DEBUG_DO(x) do { x; } while (0)
#else
#define SLOW_DEBUG_DO(x) do { } while (0)
#endif

#ifdef DEBUG_ATTACH_MORE
#define DEBUG_ATTACH_MORE_DO(x) do { x; } while (0)
#else
#define DEBUG_ATTACH_MORE_DO(x) do { } while (0)
#endif


#define verb_print(a, x) \
    do { if (solver->conf.verbosity >= a) {std::cout << "c " << x << std::endl;} } while (0)

#ifdef DEBUG_WATCHED
#define DEBUG_WATCHED_DO(x) do { x; } while (0)
#else
#define DEBUG_WATCHED_DO(x) do { } while (0)
#endif

#ifdef CHECK_N_OCCUR
#define CHECK_N_OCCUR_DO(x) do { x; } while (0)
#else
#define CHECK_N_OCCUR_DO(x) do { } while (0)
#endif

#ifdef DEBUG_IMPLICIT_STATS
#define DEBUG_IMPLICIT_STATS_DO(x) do { x; } while (0)
#else
#define DEBUG_IMPLICIT_STATS_DO(x) do { } while (0)
#endif


#ifdef VERBOSE_DEBUG
#define FAST_DEBUG
#define DEBUG_ATTACH_FULL
#define VERBOSE_DEBUG_XOR
#define VERBOSE_DEBUG_RECONSTRUCT
#endif

#ifdef __GNUC__
    #define likely(x) __builtin_expect((x), 1)
    #define unlikely(x) __builtin_expect((x), 0)
#else
    #define likely(x) x
    #define unlikely(x) x
#endif


#ifdef DEBUG_MARKED_CLAUSE
#define DEBUG_MARKED_CLAUSE_DO(x) do {x;} while (0)
#else
#define DEBUG_MARKED_CLAUSE_DO(x) do {} while (0)
#endif

#define del_xor_reason(x) do {\
    if ((x).reason_cl_ID != 0) *solver->frat << del << (x).reason_cl_ID << (x).reason_cl << fin; \
    (x).reason_cl_ID = 0; \
    } while (0)

#define set_unsat_cl_id(x) do { \
    *solver->frat << "UNSAT SET HERE" <<  __PRETTY_FUNCTION__ << "\n"; \
    assert(solver->unsat_cl_ID == 0);\
    solver->unsat_cl_ID = (x);\
    /*cout << "set unsat CL ID here to " << (x) << endl;*/\
    /*assert(false);*/\
    } while (0)
