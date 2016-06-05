/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.0 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/

#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#include <cstdint>
#include <cstdlib>
#include <stdio.h>

#define release_assert(a) \
    do { \
        if (!(a)) {\
            fprintf(stderr, "*** ASSERTION FAILURE in %s() [%s:%d]: %s\n", \
            __FUNCTION__, __FILE__, __LINE__, #a); \
            abort(); \
        } \
    } while (0)

#if !defined(__GNUC__) && !defined(__clang__)
#define __builtin_prefetch(x)
#endif //__GNUC__


#if defined _WIN32
    #define DLL_PUBLIC __declspec(dllexport)
#else
    #define DLL_PUBLIC __attribute__ ((visibility ("default")))
    #define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
#endif

///////////////////
// Verbose Debug
///////////////////

//#define DRAT_DEBUG
//#define VERBOSE_DEBUG

#ifdef VERBOSE_DEBUG
#define FAST_DEBUG
#define DEBUG_ATTACH_FULL
#define VERBOSE_DEBUG_XOR
#define VERBOSE_DEBUG_RECONSTRUCT
#endif

//Thanks to Axel Kemper for the definitions below
#ifdef _MSC_VER
#pragma warning(disable : 4244)  //  C4244 : 'Argument': Konvertierung von 'const uint64_t' in 'double', möglicher Datenverlust
#pragma warning(disable : 4267)  //  C4267 : 'return': Konvertierung von 'size_t' nach 'uint32_t', Datenverlust möglich
#pragma warning(disable : 4302)  //  C4302 : truncation
#pragma warning(disable : 4311)  //  C4311 : pointer truncation
#pragma warning(disable : 4800)  //  C4800 : 'const uint32_t' : Variable wird auf booleschen Wert('True' oder 'False') gesetzt(Auswirkungen auf Leistungsverhalten möglich)
#pragma warning(disable : 4805)  //  C4805 : '==' : unsichere Kombination von Typ 'unsigned short' mit Typ 'bool' in einer Operation
#endif


///////////////////
// Silent Debug
///////////////////

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
#endif

#ifdef FAST_DEBUG
#define DEBUG_VARELIM
#define DEBUG_WATCHED
#define DEBUG_ATTACH
#define DEBUG_REPLACER
#define DEBUG_MARKED_CLAUSE
#endif

//#define DEBUG_ATTACH_FULL

#endif //__CONSTANTS_H__
