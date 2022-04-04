/****************************************************************************
Copyright (c) 2011-2012, Armin Biere, Johannes Kepler University.

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

#include "picosat.h"

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>
#include <limits.h>

#if 1
#define LOG(ARGS...) do { } while (0)
#else
#define LOG(ARGS...) do { printf (##ARGS); } while (0)
#endif

static int reductions, ngroups;

static PicoSAT * ps;

static void die (const char * fmt, ...) {
  va_list ap;
  fprintf (stderr, "*** picogcnf: ");
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
  exit (1);
}

static void msg (const char * fmt, ...) {
  va_list ap;
  printf ("c [picogcnf] %.2f seconds: ", picosat_time_stamp ());
  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);
  fputc ('\n', stdout);
  fflush (stdout);
}

static double percent (double a, double b) { return b?100.0*a/b:0.0; }

static void callback (void * dummy, const int * mus) {
  int remaining;
  const int * p;
  (void) dummy;
  remaining = 0;
  for (p = mus; *p; p++) remaining++;
  assert (remaining <= ngroups);
  msg ("<%d> reduction to %d out of %d (%.0f%%)",
       ++reductions, remaining, ngroups, percent (remaining, ngroups));
}

int main (int argc, char ** argv) {
  int ch, nvars, sclauses, nclauses, sign, lit, group, res;
  const int * mus, * p;
  FILE * file;
  if (argc != 2) die ("usage: picogcnf <gcnf-file>");
  if (!(file = fopen (argv[1], "r"))) die ("can not read '%s'", argv[1]);
  ps = picosat_init ();
HEADER:
  ch = getc (file);
  if (ch == 'c') {
    while ((ch = getc (file)) != '\n')
      if (ch == EOF) die ("unexpected EOF");
    goto HEADER;
  }
  if (ch != 'p' || 
      getc (file) != ' ' ||
      fscanf (file, "gcnf %d %d %d", &nvars, &sclauses, &ngroups) != 3)
    die ("invalid header");
  nclauses = lit = 0;
  group = INT_MAX;
  LOG ("p gcnf %d %d %d\n", nvars, sclauses, ngroups);
LIT:
  ch = getc (file);
  if (ch == EOF) {
    if (lit) die ("zero missing");
    if (nclauses < sclauses) die ("clauses missing");
    goto DONE;
  }
  if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') goto LIT;
  if (lit) {
    if (ch == '-') {
      sign = -1;
      ch = getc (file);
    } else sign = 1;
    lit = ch - '0';
    while (isdigit (ch = getc (file)))
      lit = 10 * lit + ch - '0';
    if (lit > nvars) die ("maximum variable exceeded");
    lit *= sign;
    if (lit) {
      LOG ("%d ", lit);
    } else {
      LOG ("0\n");
      group = INT_MAX;
      nclauses++;
    }
    picosat_add (ps, lit);
  } else if (ch == '{') {
    if (nclauses == sclauses) die ("too many clauses");
    if (group < INT_MAX) die ("multiple groups per clause");
GROUP:
    ch = getc (file);
    if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') goto GROUP;
    if (!isdigit (ch)) die ("group does not start with digit");
    group = ch - '0';
    while (isdigit (ch = getc (file)))
      group = 10 * group + (ch - '0');
    if (group > ngroups) die ("maximal group exceeded");
    while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
      ch = getc (file);
    if (ch != '}') die ("expected '}'");
    LOG ("{%d} ", group);
    if (group) picosat_add (ps, -(nvars + group));
    lit = INT_MAX;
  } else die ("expected '{'");
  goto LIT;
DONE:
  fclose (file);
  for (lit = nvars + 1; lit <= nvars + ngroups; lit++) picosat_assume (ps, lit);
  res = picosat_sat (ps, -1);
  msg ("first call to SAT solver returned");
  if (res == 10) printf ("s SATISFIABLE\n");
  else if (res == 20) printf ("s UNSATISFIABLE\n");
  else printf ("s UNKNOWN\n");
  fflush (stdout);
  if (res == 20) {
    mus = picosat_mus_assumptions (ps, 0, callback, 1);
    assert (mus);
    printf ("v");
    for (p = mus; (lit = *p); p++) {
      assert (nvars + 1 <= lit && lit <= nvars + ngroups);
      printf (" %d", lit - nvars);
    }
    printf (" 0\n");
    fflush (stdout);
  }
  msg ("max memory %.1f MB",
       picosat_max_bytes_allocated (ps) / (double)(1<<20));
  picosat_reset (ps);
  msg ("%d reductions", reductions);
  return res;
}
