/*-------------------------------------------------------------------------
Copyright (c) 2013-2019, Armin Biere, Johannes Kepler University.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
-------------------------------------------------------------------------*/

#ifndef YILS_H_INCLUDED
#define YILS_H_INCLUDED

#ifndef YALSINTERNAL
#error "this file is internal to 'libyals'"
#endif

/*------------------------------------------------------------------------*/

#include "yals.h"

/*------------------------------------------------------------------------*/

#include <stdlib.h>

/*------------------------------------------------------------------------*/

#ifndef NDEBUG
void yals_logging (Yals *, int logging);
void yals_checking (Yals *, int checking);
#endif

/*------------------------------------------------------------------------*/

void yals_abort (Yals *, const char * fmt, ...);
void yals_exit (Yals *, int exit_code, const char * fmt, ...);
void yals_msg (Yals *, int level, const char * fmt, ...);

const char * yals_default_prefix (void);
const char * yals_version ();
void yals_banner (const char * prefix);

/*------------------------------------------------------------------------*/

double yals_process_time ();				// process time

double yals_sec (Yals *);				// time in 'yals_sat'
size_t yals_max_allocated (Yals *);		// max allocated bytes

/*------------------------------------------------------------------------*/

void * yals_malloc (Yals *, size_t);
void yals_free (Yals *, void*, size_t);
void * yals_realloc (Yals *, void*, size_t, size_t);

/*------------------------------------------------------------------------*/

void yals_srand (Yals *, unsigned long long);
  
#endif
