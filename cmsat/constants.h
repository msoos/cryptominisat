/*
 * forl
 *
 * Copyright (c) 2009-2011, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
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

#ifdef _MSC_VER
#include <msvc/stdint.h>
#define __builtin_popcount __popcnt16
#else
#include <stdint.h>
#endif //_MSC_VER

#define release_assert(a) \
    do { \
        if (!(a)) {\
            fprintf(stderr, "*** ASSERTION FAILURE in %s() [%s:%d]: %s\n", \
            __FUNCTION__, __FILE__, __LINE__, #a); \
            abort(); \
        } \
    } while (0)

#ifndef __GNUC__
#define __builtin_prefetch(x)
#endif //__GNUC__

///////////////////
// Settings (magic constants)
///////////////////
#define ANDGATEUSEFUL 20

///////////////////
// Verbose Debug
///////////////////

//#define VERBOSE_DEBUG

#ifdef VERBOSE_DEBUG
#define SILENT_DEBUG
#define DEBUG_USELESS_LEARNT_BIN_REMOVAL
#define DEBUG_ATTACH_FULL
#define VERBOSE_DEBUG_XOR
#define VERBOSE_DEBUG_RECONSTRUCT
#endif


///////////////////
// Silent Debug
///////////////////

#ifndef NDEBUG
#define SILENT_DEBUG
#endif

#ifdef SILENT_DEBUG
#define ENQUEUE_DEBUG
#define DEBUG_VARELIM
#define DEBUG_WATCHED
#define DEBUG_ATTACH
#define DEBUG_REPLACER
#endif

#ifdef MORE_DEBUG
#define DEBUG_PROPAGATEFROM
#define ENQUEUE_DEBUG
#define DEBUG_USELESS_LEARNT_BIN_REMOVAL
#define DEBUG_ATTACH_MORE
#define DEBUG_BIN_CLAUSE_NUM
#define DEBUG_TRI_SORTED_SANITY
#define DEBUG_IMPLICIT_STATS
#endif

//#define DEBUG_ATTACH_FULL

///////////////////
//  For Automake tools
///////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif //HAVE_CONFIG_H
