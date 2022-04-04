#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include "picosat.h"

typedef struct Clause { int cid, * lits; struct Clause * next; } Clause;
typedef struct MCS { int mid, * clauses; struct MCS * next; } MCS;

static int nvars;
static char * marked;

static Clause * first_clause, * last_clause;
static int nclauses, first_cid, last_cid;

static MCS * first_mcs, * last_mcs;
static int nmcs;

static int * stk, szstk, nstk;

static int verbose, join, noprint;

static int lineno = 1, close_input;
static const char * input_name;
static FILE * input;

static PicoSAT * ps;

static void release_clauses (void) {
  Clause * p, * next;
  for (p = first_clause; p; p = next) {
    next = p->next;
    free (p->lits);
    free (p);
  }
}

static void release_mss (void) {
  MCS * p, * next;
  for (p = first_mcs; p; p = next) {
    next = p->next;
    free (p->clauses);
    free (p);
  }
}

static void release (void) {
  release_clauses ();
  release_mss ();
  free (marked);
  free (stk);
}

static void push_stack (int n) {
  if (nstk == szstk)
    stk = realloc (stk, (szstk = szstk ? 2*szstk : 1) * sizeof *stk);
  stk[nstk++] = n;
}

static void push_clause (void) {
  Clause * clause;
  size_t bytes;
  clause = malloc (sizeof *clause);
  clause->cid = ++nclauses;
  clause->next = 0;
  push_stack (0);
  bytes = nstk * sizeof *clause->lits;
  clause->lits = malloc (bytes);
  memcpy (clause->lits, stk, bytes);
  if (last_clause) last_clause->next = clause;
  else first_clause = clause;
  last_clause = clause;
  nstk = 0;
}

static void push_mcs (void) {
  MCS * mcs;
  size_t bytes;
  mcs = malloc (sizeof *mcs);
  mcs->mid = ++nmcs;
  mcs->next = 0;
  push_stack (0);
  bytes = nstk * sizeof *mcs->clauses;
  mcs->clauses = malloc (bytes);
  memcpy (mcs->clauses, stk, bytes);
  if (last_mcs) last_mcs->next = mcs;
  else first_mcs = mcs;
  last_mcs = mcs;
  nstk = 0;
}

static int nextch (void) {
  int res = getc (input);
  if (res == '\n') lineno++;
  return res;
}

static void msg (int level, const char * fmt, ...) {
  va_list ap;
  if (level > verbose) return;
  printf ("c [picomcs] ");
  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);
  fputc ('\n', stdout);
  fflush (stdout);
}

static const char * parse (void) {
  int ch, expclauses, lit, sign;
  size_t bytes;
  msg (1, "parsing %s", input_name);
COMMENTS:
  ch = nextch ();
  if (ch == 'c') {
    while ((ch = nextch ()) != '\n')
      if (ch == EOF) return "out of file in comment";
    goto COMMENTS;
  }
  if (ch != 'p') 
INVALID_HEADER:
    return "invalid header";
  ungetc (ch, input);
  if (fscanf (input, "p cnf %d %d", &nvars, &expclauses) != 2)
    goto INVALID_HEADER;
  msg (1, "found 'p cnf %d %d' header", nvars, expclauses);
  bytes = (1 + nvars + expclauses) * sizeof *marked;
  marked = malloc (bytes);
  memset (marked, 0, bytes);
LIT:
  ch = nextch ();
  if (ch == ' '  || ch == '\n' || ch == '\t' || ch == '\r') goto LIT;
  if (ch == EOF) {
    assert (nclauses <= expclauses);
    if (nclauses < expclauses) return "clauses missing";
    return 0;
  }
  if (ch == '-') {
    ch = nextch ();
    if (!isdigit (ch)) return "expected digit after '-'";
    if (ch == '0') return "expected positive digit after '-'";
    sign = -1;
  } else if (!isdigit (ch)) return "expected '-' or digit";
  else sign = 1;
  lit = ch - '0';
  while (isdigit (ch = nextch ()))
    lit = 10*lit + (ch - '0');
  if (lit) {
    if (lit > nvars) return "maximum variable index exceeded";
    if (nclauses == expclauses) return "too many clauses";
    push_stack (sign * lit);
  } else {
    assert (nclauses < expclauses);
    push_clause ();
  }
  goto LIT;
}

#ifndef NDEBUG
static void dump_clause (Clause * c) {
  int * p, lit;
  for (p = c->lits; (lit = *p); p++)
    printf ("%d ", lit);
  printf ("0\n");
}

void dump (void) {
  Clause * p;
  printf ("p cnf %d %d\n", nvars, nclauses);
  for (p = first_clause; p; p = p->next)
    dump_clause (p);
}
#endif

static int clause2selvar (Clause * c) { 
  int res = c->cid + nvars;
  assert (first_cid <= res && res <= last_cid);
  return res;
}

static void encode_clause (Clause * c) {
  int * p, lit;
  if (verbose >= 2) {
    printf ("c [picomcs] encode clause %d :", c->cid);
    printf (" %d", -clause2selvar (c));
    for (p = c->lits; (lit = *p); p++) printf (" %d", lit);
    fputc ('\n', stdout), fflush (stdout);
  }
  picosat_add (ps, -clause2selvar (c));
  for (p = c->lits; (lit = *p); p++) picosat_add (ps, lit);
  picosat_add (ps, 0);
}

static void encode (void) {
  Clause * p;
  first_cid = nvars + 1;
  last_cid = nvars + nclauses;
  msg (2, "selector variables range %d to %d", first_cid, last_cid);
  for (p = first_clause; p; p = p->next)
    encode_clause (p);
  msg (1, "encoded %d clauses", nclauses);
}

static void camcs (void) {
  int cid, i;
  const int * mcs, * p;
  msg (1, "starting to compute all minimal correcting sets");
  while ((mcs = picosat_next_minimal_correcting_subset_of_assumptions (ps))) {
    for (p = mcs; (cid = *p); p++)
      push_stack (cid);
    if (verbose >= 2) {
      printf ("c [picomcs] mcs %d :", nmcs);
      for (i = 0; i < nstk; i++) printf (" %d", stk[i] - nvars);
      fputc ('\n', stdout);
      fflush (stdout);
    } else if (verbose && isatty (1)) {
      printf ("\rc [picomcs] mcs %d", nmcs);
      fflush (stdout);
    }
    push_mcs ();
  }
  if (verbose && isatty (1)) fputc ('\r', stdout);
  msg (1, "found %d minimal correcting sets", nmcs);
}

static void cumcscb (void * state, int nmcs, int nhumus) {
  int * ptr = state;
  *ptr = nmcs;
  ptr[0] = nmcs, ptr[1] = nhumus;
  if (!verbose || (!isatty (1) && verbose == 1)) return;
  if (verbose == 1) fputc ('\r', stdout);
  printf ("c [picomcs] mcs %d humus %d", nmcs, nhumus);
  if (verbose >= 2) fputc ('\n', stdout);
  fflush (stdout);
}

static void cumcs (void) {
  int stats[2], count, cid;
  const int * humus, * p;
  stats[0] = stats[1] = 0;
  humus = picosat_humus (ps, cumcscb, stats);
  if (isatty (1) && verbose == 1) fputc ('\n', stdout);
  count = 0;
  for (p = humus; (cid = *p); p++) {
    if (marked[cid]) continue;
    marked[cid] = 1;
    count++;
  }
  assert (count == stats[1]);
  msg (1, 
    "computed union of minimal correcting sets of size %d with %d mcs", 
    stats[1], stats[0]);
}

static void
print_umcs (void) {
  int cid;
  printf ("v");
  for (cid = first_cid; cid <= last_cid; cid++)
    if (marked[cid])
      printf (" %d", cid - nvars);
  printf (" 0\n");
}

static void
print_mcs (MCS * mcs) 
{
  const int * p;
  int cid;
  printf ("v");
  for (p = mcs->clauses; (cid = *p); p++)
    printf (" %d", cid - nvars);
  printf (" 0\n");
}

static void
print_all_mcs (void)
{
  MCS * p;
  for (p = first_mcs; p; p = p->next)
    print_mcs (p);
}

int main (int argc, char ** argv) {
  const char * perr;
  int i, res;
  for (i = 1; i < argc; i++) {
    if (!strcmp (argv[i], "-h")) {
      printf ("usage: picomcs [-h][-v][-j][-n][<input>]\n");
      exit (0);
    }
    else if (!strcmp (argv[i], "-v")) verbose++;
    else if (!strcmp (argv[i], "-j")) join = 1;
    else if (!strcmp (argv[i], "-n")) noprint = 1;
    else if (argv[i][0] == '-') {
      fprintf (stderr, "*** picomcs: invalid option '%s'\n", argv[i]);
      exit (1);
    } else if (input_name) {
      fprintf (stderr, "*** picomcs: two input files specified\n");
      exit (1);
    } else if (!(input = fopen ((input_name = argv[i]), "r"))) {
      fprintf (stderr, "*** picomcs: can not read '%s'\n", argv[i]);
      exit (1);
    } else close_input = 1;
  }
  if (!input_name) input_name = "<stdin>", input = stdin;
  if ((perr = parse ())) {
    fprintf (stderr, "%s:%d: parse error: %s\n", input_name, lineno, perr);
    exit (1);
  }
  if (close_input) fclose (input);
  ps = picosat_init ();
  picosat_set_prefix (ps, "c [picosat] ");
  encode ();
  for (i = first_cid; i <= last_cid; i++) 
    picosat_set_default_phase_lit (ps, i, 1);
  for (i = first_cid; i <= last_cid; i++) picosat_assume (ps, i);
  res = picosat_sat (ps, -1);
  if (res == 10) printf ("s SATISFIABLE\n");
  else printf ("s UNSATISFIABLE\n");
  fflush (stdout);
  if (join) cumcs (); else camcs ();
  if (verbose) picosat_stats (ps);
  picosat_reset (ps);
  if (!noprint) {
    if (join) print_umcs (); else print_all_mcs ();
  }
  release ();
  return res;
}
