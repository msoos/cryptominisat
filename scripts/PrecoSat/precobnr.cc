/***************************************************************************
Copyright (c) 2009 - 2010, Armin Biere, Johannes Kepler University.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
****************************************************************************/

#include "precobnr.hh"
#include "precocfg.hh"

#include <stdio.h>
#include <assert.h>

const char * precosat_version () {
  return PRECOSAT_VERSION " " PRECOSAT_ID;
}

void precosat_banner () {
  printf ("c PrecoSAT Version %s %s\n", PRECOSAT_VERSION, PRECOSAT_ID);
  printf ("c Copyright (C) 2009 - 2010, Armin Biere, JKU, Linz, Austria."
          "  All rights reserved.\n");
  printf ("c Released on %s\n", PRECOSAT_RELEASED);
  printf ("c Compiled on %s\n", PRECOSAT_COMPILED);
  printf ("c %s\n", PRECOSAT_CXX);
  const char * p = PRECOSAT_CXXFLAGS;
  assert (*p);
  for (;;) {
    fputs ("c ", stdout);
    const char * q;
    for (q = p; *q && *q != ' '; q++)
      ;
    if (*q && q - p < 76) {
      for (;;) {
	const char * n;
	for (n = q + 1; *n && *n != ' '; n++)
	  ;
	if (n - p >= 76) break;
	q = n;
	if (!*n) break;
      }
    }
    while (p < q) fputc (*p++, stdout);
    fputc ('\n', stdout);
    if (!*p) break;
    assert(*p == ' ');
    p++;
  }
  printf ("c %s\n", PRECOSAT_OS);
  fflush (stdout);
}
