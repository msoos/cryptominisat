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

#include "precosat.hh"
#include "precobnr.hh"

extern "C" {
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
};

using namespace PrecoSat;

static Solver * solverptr;
static bool catchedsig;
static int verbose, quiet;

static void (*sig_int_handler)(int);
static void (*sig_segv_handler)(int);
static void (*sig_abrt_handler)(int);
static void (*sig_term_handler)(int);

static bool hasgzsuffix (const char * str) 
  { return strlen (str) >= 3 && !strcmp (str + strlen (str) - 3, ".gz"); }

static void
resetsighandlers (void) {
  (void) signal (SIGINT, sig_int_handler);
  (void) signal (SIGSEGV, sig_segv_handler);
  (void) signal (SIGABRT, sig_abrt_handler);
  (void) signal (SIGTERM, sig_term_handler);
}

static void caughtsigmsg (int sig) {
  if (verbose) printf ("c\n");
  if (!quiet) printf ("c *** CAUGHT SIGNAL %d ***\n", sig);
  if (verbose) printf ("c\n");
  fflush (stdout);
}

static void catchsig (int sig) {
  if (!catchedsig) {
    catchedsig = true;
    caughtsigmsg (sig);
    if (verbose) solverptr->prstats (), caughtsigmsg (sig);
  }

  resetsighandlers ();
  raise (sig);
}

static void
setsighandlers (void) {
  sig_int_handler = signal (SIGINT, catchsig);
  sig_segv_handler = signal (SIGSEGV, catchsig);
  sig_abrt_handler = signal (SIGABRT, catchsig);
  sig_term_handler = signal (SIGTERM, catchsig);
}

static const char * usage =
"usage: precosat [-h|-v|-n][-l <lim>][--[no-]<opt>[=<val>]] [<dimacs>]\n"
"\n"
"  -h             this command line summary\n"
"  -v             increase verbosity\n"
"  -f             force reading DIMACS file even if header is missing\n"
"  -q             be quiet and only set exit code\n"
"  -n             do not print witness\n"
"  -N             do not print witness nor result line\n"
"  -l <lim>       set decision limit\n"
"  -p             print CNF after first simplification \n"
"  -s             simplify and print (same as '-N -p -l 0')\n"
"  -e             satelite only style preprocessing\n"
"  -k             blocked clause only style preprocessing\n"
"  -o <file>      set output file for '-p' respectively '-s'\n"
"  <dimacs>       dimacs input file (default stdin)\n"
"\n"
"  --<opt>        set internal option <opt> to 1\n"
"  --no-<opt>     set internal option <opt> to 0\n"
"  --<opt>=<val>  set internal option <opt> to integer value <val>\n"
;

int main (int argc, char ** argv) {
  bool fclosefile=false,pclosefile=false,nowit=false,nores=false,simp=false;
  bool force=false;
  int print=0, decision_limit=INT_MAX;
  const char * name = 0, * output = 0;
  bool satelite=false,blocked=false;
  FILE * file = stdin;
  for (int i = 1; i < argc; i++) {
    if (!strcmp (argv[i], "-h")) {
      fputs (usage, stdout);
      exit (0);
    }
    if (!strcmp (argv[i], "-v")) { 
      if (verbose >= 2) {
	fprintf (stderr, "*** precosat: too many '-v'\n");
	exit (1);
      }
      verbose++;
      continue;
    }
    if (!strcmp (argv[i], "--verbose")) { verbose=1; continue; }
    if (!strcmp (argv[i], "--no-verbose")) { verbose=0; continue; }
    if (!strcmp (argv[i], "--verbose=0")) { verbose=0; continue; }
    if (!strcmp (argv[i], "--verbose=1")) { verbose=1; continue; }
    if (!strcmp (argv[i], "--verbose=2")) { verbose=2; continue; }
    if (!strcmp (argv[i], "-f")) { force = 1; continue; }
    if (!strcmp (argv[i], "-q")) { quiet = 1; continue; }
    if (!strcmp (argv[i], "-p")) { print = 1; continue; }
    if (!strcmp (argv[i], "-s")) { simp = true; continue; }
    if (!strcmp (argv[i], "--quiet")) { quiet = 1; continue; }
    if (!strcmp (argv[i], "--no-quiet")) { quiet = 0; continue; }
    if (!strcmp (argv[i], "--quiet=0")) { quiet = 0; continue; }
    if (!strcmp (argv[i], "--quiet=1")) { quiet = 1; continue; }
    if (!strcmp (argv[i], "-n")) { nowit = true; continue; }
    if (!strcmp (argv[i], "-N")) { nores = true; continue; }
    if (!strcmp (argv[i], "-k")) { blocked = true; continue; }
    if (!strcmp (argv[i], "-e")) { satelite = true; continue; }
    if (!strcmp  (argv[i], "-l")) {
      if (i + 1 == argc) {
	fprintf (stderr, "*** precosat: argument to '-l' missing\n");
	exit (1);
      }
      decision_limit = atoi (argv[++i]);
      continue;
    }
    if (!strcmp (argv[i], "-o")) {
      if (i + 1 == argc) {
	fprintf (stderr, "*** precosat: argument to '-o' missing\n");
	exit (1);
      }
      output = argv[++i];
      continue;
    }
    if (argv[i][0] == '-' && argv[i][1] == '-') continue;
    if (argv[i][0] == '-' || name) {
      fprintf (stderr, "*** precosat: invalid usage (try '-h')\n");
      exit (1);
    }
    name = argv[i];
    if (hasgzsuffix (name)) {
      char * cmd = (char*) malloc (strlen(name) + 100);
      sprintf (cmd, "gunzip -c %s 2>/dev/null", name);
      if ((file = popen (cmd, "r")))
	pclosefile = true;
      free (cmd);
    } else if ((file = fopen (name, "r")))
      fclosefile = true;
  }
  if (!name) name = "<stdin>";
  if (!file) {
    fprintf (stderr, "*** precosat: can not read '%s'\n", name);
    exit (1);
  }
  if (blocked && satelite) die ("can not use both '-e' and '-k'");
  if (nores) nowit = true;
  if (blocked || satelite) simp = true;
  if (simp) nowit = nores = true, print = 1, decision_limit = 0;
  if (quiet) verbose = 0, nowit = true;
  if (verbose) {
    precosat_banner ();
    printf ("c\nc reading %s\n", name);
    fflush (stdout);
  }
  int ch;
  while ((ch = getc (file)) == 'c')
    while ((ch = getc (file)) != '\n' && ch != EOF)
      ;
  int m = 0, n = 0, v = 0;
  bool header = false;
  if (ch == EOF)
    goto INVALID_HEADER;
  ungetc (ch, file);
  if (fscanf (file, "p cnf %d %d\n", &m, &n) != 2) {
INVALID_HEADER:
    if (!force) {
      fprintf (stderr, "*** precosat: invalid header\n");
      exit (1);
    }
  } else header= true;
  if (verbose && header) {
    printf ("c found header 'p cnf %d %d'\n", m, n);
    fflush (stdout);
  }
  Solver solver;
  solver.init (m);
  solverptr = &solver;
  setsighandlers ();
  for (int i = 1; i < argc; i++) {
    bool ok = true;
    if (argv[i][0] == '-' && argv[i][1] == '-') {
      if (argv[i][2] == 'n' && argv[i][3] == 'o' && argv[i][4] == '-') {
	if (!solver.set (argv[i] + 5, 0)) ok = false;
      } else {
	const char * arg = argv[i] + 2, * p;
	for (p = arg; *p && *p != '='; p++)
	  ;
	if (*p) {
	  assert (*p == '=');
	  char * opt = strndup (arg, p - arg);
	  if (!strcmp (opt, "output")) ok = solver.set (opt, p + 1);
	  else ok = solver.set (opt, atoi (p + 1));
	  free (opt);
	} else ok = solver.set (argv[i] + 2, 1);
      }
    }
    if (!ok) {
      fprintf (stderr, "*** precosat: invalid option '%s'\n", argv[i]);
      exit (1);
    }
  }
  solver.set ("verbose", verbose);
  solver.set ("quiet", quiet);
  solver.set ("print", print);
  if (output) solver.set ("output", output);
  if (blocked || satelite) {
    assert (simp);
    solver.set ("decompose",0);
    solver.set ("autark",0);
    solver.set ("probe",0);
    solver.set ("otfs",0);
    solver.set ("elimasym", 0);
  }
  if (blocked) solver.set ("elim",0), solver.set ("blockrtc",2);
  if (satelite) solver.set ("block",0), solver.set ("elimrtc",2);
  solver.fxopts ();
  int lit, sign;
  int res = 0;

  if (ch != EOF) ch = getc (file);

NEXT:

  if (ch == 'c') {
      while ((ch = getc (file)) != '\n')
	if (ch == EOF) {
	  fprintf (stderr, "*** precosat: EOF in comment\n");
	  resetsighandlers ();
	  exit (1);
	}
      goto NEXT;
  }

  if (ch == ' ' || ch == '\n') {
    ch = getc (file);
    goto NEXT;
  }

  if (ch == EOF) goto DONE;

  if (header && !n) {
    fprintf (stderr, "*** precosat: too many clauses\n");
    res = 1;
    goto EXIT;
  }

  if ((sign = (ch == '-'))) ch = getc (file);

  if (!isdigit (ch) || (sign && ch == '0')) {
    fprintf (stderr, "*** precosat: expected digit\n");
    res = 1;
    goto EXIT;
  }

  lit = (ch - '0');
  while (isdigit (ch = getc (file))) lit = 10 * lit + (ch - '0');

  if (header && lit > m) {
    fprintf (stderr, "*** precosat: maximum variable index exceeded\n");
    res = 1;
    goto EXIT;
  }

  if (lit > v) v = lit;

  lit = 2 * lit + sign;
  solver.add (lit);
  if (header && !lit) n--;

  goto NEXT;

DONE:

  if (header && n) {
    fprintf (stderr, "*** precosat: clauses missing\n");
    res = 1;
    goto EXIT;
  }

  if (pclosefile) pclose (file);
  if (fclosefile) fclose (file);

  if (verbose) {
    printf ("c finished parsing\n");
    printf ("c\n"); solver.propts (); printf ("c\n");
    printf ("c starting search after %.1f seconds\n", solver.seconds ());
    if (decision_limit == INT_MAX) printf ("c no decision limit\n");
    else printf ("c search limited to %d decisions\n", decision_limit);
    fflush (stdout);
  }

  res = solver.solve (decision_limit);

  if (verbose) {
    printf ("c\n");
    fflush (stdout);
  }

  if (res > 0) {
    if (!solver.satisfied ()) {
      if (!quiet) printf ("c ERROR\n");
      abort ();
    }
    if (!quiet && !nores) printf ("s SATISFIABLE\n");
    if (!nowit) {
      fflush (stdout);
      if (!header) m = v;
      if (m) fputs ("v", stdout);
      for (int i = 1; i <= m; i++) {
	if (i % 8) fputc (' ', stdout);
	else fputs ("\nv ", stdout);
	printf ("%d", (solver.val (2 * i) < 0) ? -i : i);
      }
      if (m) fputc ('\n', stdout);
      fputs ("v 0\n", stdout);
    }
    res = 10;
  } else if (res < 0) {
    assert (res < 0);
    if (!quiet && !nores) printf ("s UNSATISFIABLE\n");
    res = 20;
  } else if (!quiet && !nores)
    printf ("s UNKNOWN\n");

  if (!quiet) fflush (stdout);

EXIT:
  resetsighandlers ();
  if (verbose) {
    printf ("c\n");
    solver.prstats ();
  }

#ifndef NDEBUG
  solver.reset ();
#endif

  return res;
}
