#include "picosat.h"

#include <assert.h>
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define GUNZIP "gunzip -c %s"
#define BUNZIP2 "bzcat %s"
#define GZIP "gzip -c -f > %s"

FILE * popen (const char *, const char*);
int pclose (FILE *);

static PicoSAT * picosat;

static int lineno;
static FILE *input;
static int inputid;
static FILE *output;
static int verbose;
static int sargc;
static char ** sargv;
static char buffer[100];
static char *bhead = buffer;
static const char *eob = buffer + 80;
static FILE * incremental_rup_file;
static signed char * sol;

extern void picosat_enter (PicoSAT *);
extern void picosat_leave (PicoSAT *);

static int
next (void)
{
  int res = getc (input);
  if (res == '\n')
    lineno++;

  return res;
}

static const char *
parse (PicoSAT * picosat, int force)
{
  int ch, sign, lit, vars, clauses;

  lineno = 1;
  inputid = fileno (input);

SKIP_COMMENTS:
  ch = next ();
  if (ch == 'c')
    {
      while ((ch = next ()) != EOF && ch != '\n')
	;
      goto SKIP_COMMENTS;
    }

  if (isspace (ch))
    goto SKIP_COMMENTS;

  if (ch != 'p')
INVALID_HEADER:
    return "missing or invalid 'p cnf <variables> <clauses>' header";

  if (!isspace (next ()))
    goto INVALID_HEADER;

  while (isspace (ch = next ()))
    ;

  if (ch != 'c' || next () != 'n' || next () != 'f' || !isspace (next ()))
    goto INVALID_HEADER;

  while (isspace (ch = next ()))
    ;
    
  if (!isdigit (ch))
    goto INVALID_HEADER;

  vars = ch - '0';
  while (isdigit (ch = next ()))
    vars = 10 * vars + (ch - '0');

  if (!isspace (ch))
    goto INVALID_HEADER;

  while (isspace (ch = next ()))
    ;

  if (!isdigit (ch))
    goto INVALID_HEADER;

  clauses = ch - '0';
  while (isdigit (ch = next ()))
    clauses = 10 * clauses + (ch - '0');

  if (!isspace (ch) && ch != '\n' )
    goto INVALID_HEADER;

  if (verbose)
    {
      fprintf (output, "c parsed header 'p cnf %d %d'\n", vars, clauses);
      fflush (output);
    }

  picosat_adjust (picosat, vars);

  if (incremental_rup_file)
    picosat_set_incremental_rup_file (picosat, incremental_rup_file, vars, clauses);

  lit = 0;
READ_LITERAL:
  ch = next ();

  if (ch == 'c')
    {
      while ((ch = next ()) != EOF && ch != '\n')
	;
      goto READ_LITERAL;
    }

  if (ch == EOF)
    {
      if (lit)
	return "trailing 0 missing";

      if (clauses && !force)
	return "clause missing";

      return 0;
    }

  if (isspace (ch))
    goto READ_LITERAL;

  sign = 1;
  if (ch == '-')
    {
      sign = -1;
      ch = next ();
    }

  if (!isdigit (ch))
    return "expected number";

  lit = ch - '0';
  while (isdigit (ch = next ()))
    lit = 10 * lit + (ch - '0');

  if (!clauses && !force)
    return "too many clauses";

  if (lit)
    {
      if (lit > vars && !force)
	return "maximal variable index exceeded";

      lit *= sign;
    }
  else
    clauses--;

  picosat_add (picosat, lit);

  goto READ_LITERAL;
}

static void
bflush (void)
{
  *bhead = 0;
  fputs (buffer, output);
  fputc ('\n', output);
  bhead = buffer;
}

static void
printi (int i)
{
  char *next;
  int l;

REENTER:
  if (bhead == buffer)
    *bhead++ = 'v';

  l = sprintf (bhead, " %d", i);
  next = bhead + l;

  if (next >= eob)
    {
      bflush ();
      goto REENTER;
    }
  else
    bhead = next;
}

static void
printa (PicoSAT * picosat, int partial)
{
  int max_idx = picosat_variables (picosat), i, lit, val;

  assert (bhead == buffer);

  for (i = 1; i <= max_idx; i++)
    {
      if (partial)
	{
	  val = picosat_deref_partial (picosat, i);
	  if (!val)
	    continue;
	}
      else
	val = picosat_deref (picosat, i);
      lit = (val > 0) ? i : -i;
      printi (lit);
    }

  printi (0);
  if (bhead > buffer)
    bflush ();
}

static void
blocksol (PicoSAT * picosat)
{
  int max_idx = picosat_variables (picosat), i;

  if (!sol)
    {
      sol = malloc (max_idx + 1);
      memset (sol, 0, max_idx + 1);
    }

  for (i = 1; i <= max_idx; i++)
    sol[i] = (picosat_deref (picosat, i) > 0) ? 1 : -1;

  for (i = 1; i <= max_idx; i++)
    picosat_add (picosat, (sol[i] < 0) ? i : -i);

  picosat_add (picosat, 0);
}

static int
has_suffix (const char *str, const char *suffix)
{
  const char *tmp = strstr (str, suffix);
  if (!tmp)
    return 0;

  return str + strlen (str) - strlen (suffix) == tmp;
}

static void
write_core_variables (PicoSAT * picosat, FILE * file)
{
  int i, max_idx = picosat_variables (picosat), count = 0;
  for (i = 1; i <= max_idx; i++)
    if (picosat_corelit (picosat, i))
      {
	fprintf (file, "%d\n", i);
	count++;
      }

  if (verbose)
    fprintf (output, "c found and wrote %d core variables\n", count);
}

static int
next_assumption (int start)
{
  char * arg, c;
  int res;
  res = start + 1;
  while (res < sargc)
  {
    arg = sargv[res++];
    if (!strcmp (arg, "-a"))
      {
	assert (res < sargc);
	break;
      }

    if (arg[0] == '-') {
      c = arg[1];
      if (c == 'l' || c == 'i' || c == 's' || c == 'o' || c == 't' ||
	  c == 'T' || c == 'r' || c == 'R' || c == 'c' || c == 'V' ||
	  c == 'U' || c == 'A') res++;
    }
  }
  if (res >= sargc) res = 0;
  return res;
}

static void
write_failed_assumptions (PicoSAT * picosat, FILE * file)
{
  int i, lit, count = 0;
#ifndef NDEBUG
  int max_idx = picosat_variables (picosat);
#endif
  i = 0;
  while ((i = next_assumption (i))) {
    lit = atoi (sargv[i]);
    if (!picosat_failed_assumption (picosat, lit)) continue;
    fprintf (file, "%d\n", lit);
    count++;
  }
  if (verbose)
    fprintf (output, "c found and wrote %d failed assumptions\n", count);
#ifndef NDEBUG
  for (i = 1; i <= max_idx; i++)
    if (picosat_failed_assumption (picosat, i))
      count--;
#endif
  assert (!count);
}

static void
write_to_file (PicoSAT * picosat, 
               const char *name, 
               const char *type,
	       void (*writer) (PicoSAT *, FILE *))
{
  int pclose_file, zipped = has_suffix (name, ".gz");
  FILE *file;
  char *cmd;

  if (zipped)
    {
      cmd = malloc (strlen (GZIP) + strlen (name));
      sprintf (cmd, GZIP, name);
      file = popen (cmd, "w");
      free (cmd);
      pclose_file = 1;
    }
  else
    {
      file = fopen (name, "w");
      pclose_file = 0;
    }

  if (file)
    {
      if (verbose)
	fprintf (output,
		 "c\nc writing %s%s to '%s'\n",
		 zipped ? "gzipped " : "", type, name);

      writer (picosat, file);

      if (pclose_file)
	pclose (file);
      else
	fclose (file);
    }
  else
    fprintf (output, "*** picosat: can not write to '%s'\n", name);
}

static int catched;

static void (*sig_int_handler);
static void (*sig_segv_handler);
static void (*sig_abrt_handler);
static void (*sig_term_handler);
#ifndef NALLSIGNALS
static void (*sig_kill_handler);
static void (*sig_xcpu_handler);
static void (*sig_xfsz_handler);
#endif

static void
resetsighandlers (void)
{
  (void) signal (SIGINT, sig_int_handler);
  (void) signal (SIGSEGV, sig_segv_handler);
  (void) signal (SIGABRT, sig_abrt_handler);
  (void) signal (SIGTERM, sig_term_handler);
#ifndef NALLSIGNALS
  (void) signal (SIGKILL, sig_kill_handler);
  (void) signal (SIGXCPU, sig_xcpu_handler);
  (void) signal (SIGXFSZ, sig_xfsz_handler);
#endif
}

static int time_limit_in_seconds;
static void (*sig_alarm_handler);
static int ought_to_be_interrupted, interrupt_notified;

static void
alarm_triggered (int sig)
{
  (void) sig;
  assert (sig == SIGALRM);
  assert (time_limit_in_seconds);
  assert (!ought_to_be_interrupted);
  ought_to_be_interrupted = 1;
  assert (!interrupt_notified);
}

static int
interrupt_call_back (void * dummy)
{
  (void) dummy;
  if (!ought_to_be_interrupted)
    return 0;
  if (!interrupt_notified)
    {
      if (verbose)
	{
	  picosat_message (picosat, 1, "");
	  picosat_message (picosat, 1,
	    "*** TIME LIMIT OF %d SECONDS REACHED ***",
	    time_limit_in_seconds);
	  picosat_message (picosat, 1, "");
	}
      interrupt_notified = 1;
    }
  return 1;
}

static void
setalarm ()
{
  assert (time_limit_in_seconds > 0);
  sig_alarm_handler = signal (SIGALRM, alarm_triggered);
  alarm (time_limit_in_seconds);
  assert (picosat);
  picosat_set_interrupt (picosat, 0, interrupt_call_back);
}

static void
resetalarm ()
{
  assert (time_limit_in_seconds > 0);
  (void) signal (SIGALRM, sig_term_handler);
}

static void
message (int sig)
{
  picosat_message (picosat, 1, "");
  picosat_message (picosat, 1, "*** CAUGHT SIGNAL %d ***", sig);
  picosat_message (picosat, 1, "");
}

static void
catch (int sig)
{
  if (!catched)
    {
      message (sig);
      catched = 1;
      picosat_stats (picosat);
      message (sig);
    }

  resetsighandlers ();
  raise (sig);
}

static void
setsighandlers (void)
{
  sig_int_handler = signal (SIGINT, catch);
  sig_segv_handler = signal (SIGSEGV, catch);
  sig_abrt_handler = signal (SIGABRT, catch);
  sig_term_handler = signal (SIGTERM, catch);
#ifndef NALLSIGNALS
  sig_kill_handler = signal (SIGKILL, catch);
  sig_xcpu_handler = signal (SIGXCPU, catch);
  sig_xfsz_handler = signal (SIGXFSZ, catch);
#endif
}

#define USAGE \
"usage: picosat [ <option> ... ] [ <input> ]\n" \
"\n" \
"where <option> is one of the following\n" \
"\n" \
"  -h           print this command line option summary and exit\n" \
"  --version    print version and exit\n" \
"  --config     print build configuration and exit\n" \
"\n" \
"  -v           enable verbose output\n" \
"  -f           ignore invalid header\n" \
"  -n           do not print satisfying assignment\n" \
"  -p           print formula in DIMACS format and exit\n" \
"  --plain      disable preprocessing (failed literal probing)\n" \
"  -a <lit>     start with an assumption\n" \
"  -l <limit>   set decision limit (no limit per default)\n" \
"  -L <limit>   set time limit in seconds (no limit per default)\n" \
"  -P <limit>   set propagation limit (no limit per default)\n" \
"  -i [0-3]     [0-3]=[FALSE,TRUE,JWH,RAND] initial phase (default 2=JWH)\n" \
"  -s <seed>    set random number generator seed (default 0)\n" \
"  -o <output>  set output file (<stdout> per default)\n" \
"  -t <trace>   generate compact proof trace file\n" \
"  -T <trace>   generate extended proof trace file\n" \
"  -r <trace>   generate reverse unit propagation proof file\n" \
"  -R <trace>   generate reverse unit propagation proof file incrementally\n" \
"  -c <core>    generate clausal core file in DIMACS format\n" \
"  -V <core>    generate file listing core variables\n" \
"  -U <core>    generate file listing used variables\n" \
"  -A <core>    generate file listing failed assumptions\n" \
"\n" \
"  --all        enumerate all solutions\n" \
"  --partial    generate and print only partial assignment\n" \
"\n" \
"and <input> is an optional input file in DIMACS format.\n"

int
picosat_main (int argc, char **argv)
{
  int res, done, err, print_satisfying_assignment, force, print_formula;
  const char *compact_trace_name, *extended_trace_name, * rup_trace_name;
  int assumption, assumptions, defaultphase, allsat, partial, plain;
  const char * clausal_core_name, * variable_core_name;
  const char *input_name, *output_name;
  const char * failed_assumptions_name;
  int close_input, pclose_input;
  long long propagation_limit;
  int i, decision_limit;
  double start_time;
  long long sols;
  unsigned seed;
  FILE *file;
  int trace;

  start_time = picosat_time_stamp ();

  sargc = argc;
  sargv = argv;

  clausal_core_name = 0;
  variable_core_name = 0;
  failed_assumptions_name = 0;
  output_name = 0;
  compact_trace_name = 0;
  extended_trace_name = 0;
  rup_trace_name = 0;
  incremental_rup_file = 0;
  close_input = 0;
  pclose_input = 0;
  input_name = "<stdin>";
  input = stdin;
  output = stdout;
  verbose = 0;
  done = err = 0;
  decision_limit = -1;
  propagation_limit = -1;
  defaultphase = 2;
  assumptions = 0;
  force = 0;
  allsat = 0;
  partial = 0;
  trace = 0;
  plain = 0;
  seed = 0;
  sols= 0;

  picosat = 0;

  print_satisfying_assignment = 1;
  print_formula = 0;

  for (i = 1; !done && !err && i < argc; i++)
    {
      if (!strcmp (argv[i], "-h"))
	{
	  fputs (USAGE, output);
	  done = 1;
	}
      else if (!strcmp (argv[i], "--version"))
	{
	  fprintf (output, "%s\n", picosat_version ());
	  done = 1;
	}
      else if (!strcmp (argv[i], "--config"))
	{
	  fprintf (output, "%s\n", picosat_config ());
	  done = 1;
	}
      else if (!strcmp (argv[i], "-v"))
	{
	  verbose++;
	}
      else if (!strcmp (argv[i], "--plain"))
	{
	  plain = 1;
	}
      else if (!strcmp (argv[i], "-f"))
	{
	  force = 1;
	}
      else if (!strcmp (argv[i], "-n"))
	{
	  print_satisfying_assignment = 0;
	}
      else if (!strcmp (argv[i], "--partial"))
	{
	  partial = 1;
	}
      else if (!strcmp (argv[i], "-p"))
	{
	  print_formula = 1;
	}
      else if (!strcmp (argv[i], "-l"))
	{
	  if (++i == argc)
	    {
	      fprintf (output, "*** picosat: argument to '-l' missing\n");
	      err = 1;
	    }
	  else
	    decision_limit = atoi (argv[i]);
	}
      else if (!strcmp (argv[i], "-L"))
	{
	  if (++i == argc)
	    {
	      fprintf (output, "*** picosat: argument to '-L' missing\n");
	      err = 1;
	    }
	  else
	    {
	      time_limit_in_seconds = atoi (argv[i]);
	      if (time_limit_in_seconds <= 0)
		{
		  fprintf (output, "*** picosat: invalid '-L' argument\n");
		  err = 1;
		}
	    }
	}
      else if (!strcmp (argv[i], "-P"))
	{
	  if (++i == argc)
	    {
	      fprintf (output, "*** picosat: argument to '-P' missing\n");
	      err = 1;
	    }
	  else
	    propagation_limit = atoll (argv[i]);
	}
      else if (!strcmp (argv[i], "-i"))
	{
	  if (++i == argc)
	    {
	      fprintf (output, "*** picosat: argument to '-i' missing\n");
	      err = 1;
	    }
	  else if (!argv[i][1] && ('0' <= argv[i][0] && argv[i][0] <= '3'))
	    {
	      defaultphase = argv[i][0] - '0';
	    }
	  else
	    {
	      fprintf (output, "*** picosat: invalid argument to '-i'\n");
	      err = 1;
	    }
	}
      else if (!strcmp (argv[i], "-a"))
	{
	  if (++i == argc)
	    {
	      fprintf (output, "*** picosat: argument to '-a' missing\n");
	      err = 1;
	    }
	  else if (!atoi (argv[i]))
	    {
	      fprintf (output, "*** picosat: argument to '-a' zero\n");
	      err = 1;
	    }
	  else
	    {
	      /* Handle assumptions further down
	       */
	      assumptions++;
	    }
	}
      else if (!strcmp (argv[i], "--all"))
	{
	  allsat = 1;
	}
      else if (!strcmp (argv[i], "-s"))
	{
	  if (++i == argc)
	    {
	      fprintf (output, "*** picosat: argument to '-s' missing\n");
	      err = 1;
	    }
	  else
	    seed = atoi (argv[i]);
	}
      else if (!strcmp (argv[i], "-o"))
	{
	  if (output_name)
	    {
	      fprintf (output,
		       "*** picosat: "
		       "multiple output files '%s' and '%s'\n",
		       output_name, argv[i]);
	      err = 1;
	    }
	  else if (++i == argc)
	    {
	      fprintf (output, "*** picosat: argument ot '-o' missing\n");
	      err = 1;
	    }
	  else if (!(file = fopen (argv[i], "w")))
	    {
	      fprintf (output,
		       "*** picosat: "
		       "can not write output file '%s'\n", argv[i]);
	      err = 1;
	    }
	  else
	    {
	      output_name = argv[i];
	      output = file;
	    }
	}
      else if (!strcmp (argv[i], "-t"))
	{
	  if (compact_trace_name)
	    {
	      fprintf (output,
		       "*** picosat: "
		       "multiple compact trace files '%s' and '%s'\n",
		       compact_trace_name, argv[i]);
	      err = 1;
	    }
	  else if (++i == argc)
	    {
	      fprintf (output, "*** picosat: argument ot '-t' missing\n");
	      err = 1;
	    }
	  else
	    {
	      compact_trace_name = argv[i];
	      trace = 1;
	    }
	}
      else if (!strcmp (argv[i], "-T"))
	{
	  if (extended_trace_name)
	    {
	      fprintf (output,
		       "*** picosat: "
		       "multiple extended trace files '%s' and '%s'\n",
		       extended_trace_name, argv[i]);
	      err = 1;
	    }
	  else if (++i == argc)
	    {
	      fprintf (output, "*** picosat: argument ot '-T' missing\n");
	      err = 1;
	    }
	  else
	    {
	      extended_trace_name = argv[i];
	      trace = 1;
	    }
	}
      else if (!strcmp (argv[i], "-r"))
	{
	  if (rup_trace_name)
	    {
	      fprintf (output,
		       "*** picosat: "
		       "multiple RUP trace files '%s' and '%s'\n",
		       rup_trace_name, argv[i]);
	      err = 1;
	    }
	  else if (++i == argc)
	    {
	      fprintf (output, "*** picosat: argument ot '-r' missing\n");
	      err = 1;
	    }
	  else
	    {
	      rup_trace_name = argv[i];
	      trace = 1;
	    }
	}
      else if (!strcmp (argv[i], "-R"))
	{
	  if (rup_trace_name)
	    {
	      fprintf (output,
		       "*** picosat: "
		       "multiple RUP trace files '%s' and '%s'\n",
		       rup_trace_name, argv[i]);
	      err = 1;
	    }
	  else if (++i == argc)
	    {
	      fprintf (output, "*** picosat: argument ot '-R' missing\n");
	      err = 1;
	    }
	  else if (!(file = fopen (argv[i], "w")))
	    {
	      fprintf (output,
		       "*** picosat: can not write to '%s'\n", argv[i]);
	      err = 1;
	    }
	  else
	    {
	      rup_trace_name = argv[i];
	      incremental_rup_file = file;
	    }
	}
      else if (!strcmp (argv[i], "-c"))
	{
	  if (clausal_core_name)
	    {
	      fprintf (output,
		       "*** picosat: "
		       "multiple clausal core files '%s' and '%s'\n",
		       clausal_core_name, argv[i]);
	      err = 1;
	    }
	  else if (++i == argc)
	    {
	      fprintf (output, "*** picosat: argument ot '-c' missing\n");
	      err = 1;
	    }
	  else
	    {
	      clausal_core_name = argv[i];
	      trace = 1;
	    }
	}
      else if (!strcmp (argv[i], "-V"))
	{
	  if (variable_core_name)
	    {
	      fprintf (output,
		       "*** picosat: "
		       "multiple variable core files '%s' and '%s'\n",
		       variable_core_name, argv[i]);
	      err = 1;
	    }
	  else if (++i == argc)
	    {
	      fprintf (output, "*** picosat: argument ot '-V' missing\n");
	      err = 1;
	    }
	  else
	    {
	      variable_core_name = argv[i];
	      trace = 1;
	    }
	}
      else if (!strcmp (argv[i], "-A"))
	{
	  if (failed_assumptions_name)
	    {
	      fprintf (output,
		       "*** picosat: "
		       "multiple failed assumptions files '%s' and '%s'\n",
		       failed_assumptions_name, argv[i]);
	      err = 1;
	    }
	  else if (++i == argc)
	    {
	      fprintf (output, "*** picosat: argument ot '-A' missing\n");
	      err = 1;
	    }
	  else
	    failed_assumptions_name = argv[i];
	}
      else if (argv[i][0] == '-')
	{
	  fprintf (output,
		   "*** picosat: "
		   "unknown command line option '%s' (try '-h')\n", argv[i]);
	  err = 1;
	}
      else if (close_input || pclose_input)
	{
	  fprintf (output,
		   "*** picosat: "
		   "multiple input files '%s' and '%s'\n",
		   input_name, argv[i]);
	  err = 1;
	}
      else if (has_suffix (argv[i], ".gz"))
	{
	  char *cmd = malloc (strlen (GUNZIP) + strlen (argv[i]));
	  sprintf (cmd, GUNZIP, argv[i]);
	  if ((file = popen (cmd, "r")))
	    {
	      input_name = argv[i];
	      pclose_input = 1;
	      input = file;
	    }
	  else
	    {
	      fprintf (output,
		       "*** picosat: "
		       "can not read compressed input file '%s'\n", argv[i]);
	      err = 1;
	    }
	  free (cmd);
	}
      else if (has_suffix (argv[i], ".bz2"))
	{
	  char *cmd = malloc (strlen (BUNZIP2) + strlen (argv[i]));
	  sprintf (cmd, BUNZIP2, argv[i]);
	  if ((file = popen (cmd, "r")))
	    {
	      input_name = argv[i];
	      pclose_input = 1;
	      input = file;
	    }
	  else
	    {
	      fprintf (output,
		       "*** picosat: "
		       "can not read compressed input file '%s'\n", argv[i]);
	      err = 1;
	    }
	  free (cmd);
	}
      else if (!(file = fopen (argv[i], "r")))	/* TODO .gz ? */
	{
	  fprintf (output,
		   "*** picosat: can not read input file '%s'\n", argv[i]);
	  err = 1;
	}
      else
	{
	  input_name = argv[i];
	  close_input = 1;
	  input = file;
	}
    }

  if (allsat && partial)
    {
      fprintf (output,
	       "*** picosat: can not combine '--all' and '--partial'");
      err = 1;
    }

  res = PICOSAT_UNKNOWN;

  if (!done && !err)
    {
      const char *err_msg;

      if (verbose)
	{
	  fprintf (output,
		   "c PicoSAT SAT Solver Version %s\n",
		   picosat_version ());

	  fprintf (output, "c %s\n", picosat_copyright ());
	  fprintf (output, "c %s\n", picosat_config ());
	}

      picosat = picosat_init ();

      if (verbose)
	setsighandlers ();

      if (time_limit_in_seconds)
	setalarm ();

      picosat_enter (picosat);

      if (output_name)
	picosat_set_output (picosat, output);

      picosat_set_verbosity (picosat, verbose);
      picosat_set_plain (picosat, plain);

      if (verbose) fputs ("c\n", output);

      if (trace)
	{
	  if (verbose)
	    fprintf (output, "c tracing proof\n");
	  picosat_enable_trace_generation (picosat);
	}

      if (defaultphase)
	{
	  if (verbose)
	    fprintf (output, "c using %d as default phase\n", defaultphase);
	  picosat_set_global_default_phase (picosat, defaultphase);
	}

      if (propagation_limit >= 0)
	{
	  if (verbose)
	    fprintf (output, "c propagation limit of %lld propagations\n",
	             propagation_limit);
	  picosat_set_propagation_limit (picosat, 
	    (unsigned long long) propagation_limit);
	}

      if (partial) 
	{
	  if (verbose)
	    fprintf (output, 
	      "c saving original clauses for partial assignment\n");

	  picosat_save_original_clauses (picosat);
	}

      if (verbose)
	fprintf (output, "c\nc parsing %s\n", input_name);

      if (verbose)
	fflush (output);

      if ((err_msg = parse (picosat, force)))
	{
	  fprintf (output, "%s:%d: %s\n", input_name, lineno, err_msg);
	  err = 1;
	}
      else
	{
NEXT_SOLUTION:
	  if (assumptions)
	    {
	      i = 0;
	      while ((i = next_assumption (i)))
		{
		  assert (i < argc);
		  assumption = atoi (argv[i]);
		  assert (assumption);

		  picosat_assume (picosat, assumption);

		  if (verbose)
		    fprintf (output, "c assumption %d\n", assumption);
		}
	    }

	  if (print_formula)
	    {
	      picosat_print (picosat, output);
	    }
	  else
	    {
	      if (verbose)
		fprintf (output,
			 "c initialized %u variables\n"
			 "c found %u non trivial clauses\n",
			 picosat_variables (picosat),
			 picosat_added_original_clauses (picosat));

	      picosat_set_seed (picosat, seed);
	      if (verbose)
		fprintf (output,
			 "c\nc random number generator seed %u\n", 
			 seed);

	      res = picosat_sat (picosat, decision_limit);

	      if (res == PICOSAT_UNSATISFIABLE)
		{

		  if (allsat)
		    fprintf (output, "s SOLUTIONS %lld\n", sols);
		  else
		    fputs ("s UNSATISFIABLE\n", output);

		  fflush (output);

		  if (compact_trace_name)
		    write_to_file (picosat,
		                   compact_trace_name,
				   "compact trace", 
				   picosat_write_compact_trace);

		  if (extended_trace_name)
		    write_to_file (picosat,
		                   extended_trace_name,
				   "extended trace", 
				   picosat_write_extended_trace);

		  if (!incremental_rup_file && rup_trace_name)
		    write_to_file (picosat,
		                   rup_trace_name,
				   "rup trace", 
				   picosat_write_rup_trace);

		  if (clausal_core_name)
		    write_to_file (picosat,
		                   clausal_core_name, 
				   "clausal core",
				   picosat_write_clausal_core);

		  if (variable_core_name)
		    write_to_file (picosat,
		                   variable_core_name, 
				   "variable core",
				   write_core_variables);

		  if (failed_assumptions_name)
		    write_to_file (picosat,
		                   failed_assumptions_name,
		                   "failed assumptions", 
				   write_failed_assumptions);
		}
	      else if (res == PICOSAT_SATISFIABLE)
		{
	          if (allsat)
		    {
		      sols++;
		      if (verbose)
			fprintf (output, "c\nc solution %lld\nc\n", sols);
		    }

		  if (!allsat || print_satisfying_assignment)
		    fputs ("s SATISFIABLE\n", output);

		  if (!allsat || verbose || print_satisfying_assignment)
		    fflush (output);

		  if (print_satisfying_assignment)
		    printa (picosat, partial);

		  if (allsat)
		    {
		      blocksol (picosat);
		      goto NEXT_SOLUTION;
		    }
		}
	      else
		{
		  fputs ("s UNKNOWN\n", output);

		  if (allsat && verbose)
		    fprintf (output,
			     "c\nc limit reached after %lld solutions\n",
			     sols);
		  fflush (output);
		}
	    }
	}

      if (!err && verbose)
	{
	  fputs ("c\n", output);
	  picosat_stats (picosat);
	  fprintf (output,
	           "c %.1f seconds total run time\n",
		   picosat_time_stamp () - start_time);
	}

      if (sol)
	{
	  free (sol);
	  sol = 0;
	}

      picosat_leave (picosat);

      if (time_limit_in_seconds)
	resetalarm ();

      if (verbose)
	resetsighandlers ();

      picosat_reset (picosat);
    }

  if (incremental_rup_file)
    fclose (incremental_rup_file);

  if (close_input)
    fclose (input);

  if (pclose_input)
    pclose (input);

  if (output_name)
    fclose (output);

  return res;
}
