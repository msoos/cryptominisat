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

#include "yals.h"

/*------------------------------------------------------------------------*/

#define YALSINTERNAL
#include "yils.h"

/*------------------------------------------------------------------------*/

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#include "msvc/stdint.h"
#else
#include <stdint.h>
#endif

#if defined(YALSAT_FPU)
#include <fpu_control.h>	// Set FPU to double precision on Linux.
#endif

/*------------------------------------------------------------------------*/

#define YALS_INT64_MAX		(0x7fffffffffffffffll)
#define YALS_DEFAULT_PREFIX	"c "

/*------------------------------------------------------------------------*/

#define NEWN(P,N) \
  do { (P) = yals_malloc (yals, (N)*sizeof *(P)); } while (0)

#define DELN(P,N) \
  do { yals_free (yals, (P), (N)*sizeof *(P)); } while (0)

#define RSZ(P,O,N) \
do { \
  (P) = yals_realloc (yals, (P), (O)*sizeof *(P), (N)*sizeof *(P)); \
} while (0)

/*------------------------------------------------------------------------*/

#define STACK(T) \
  struct { T * start; T * top; T * end; }

#define INIT(S) \
  do { (S).start = (S).top = (S).end = 0; } while (0)

#define SIZE(S) \
  ((S).end - (S).start)

#define COUNT(S) \
  ((S).top - (S).start)

#define EMPTY(S) \
  ((S).top == (S).start)

#define FULL(S) \
  ((S).top == (S).end)

#define CLEAR(S) \
  do { (S).top = (S).start; } while (0)

#define ENLARGE(S) \
do { \
  size_t OS = SIZE (S); \
  size_t OC = COUNT (S); \
  size_t NS = OS ? 2*OS : 1; \
  assert (OC <= OS); \
  RSZ ((S).start, OS, NS); \
  (S).top = (S).start + OC; \
  (S).end = (S).start + NS; \
} while (0)

#define FIT(S) \
do { \
  size_t OS = SIZE (S); \
  size_t NS = COUNT (S); \
  RSZ ((S).start, OS, NS); \
  (S).top = (S).start + NS; \
  (S).end = (S).start + NS; \
} while (0)

#define RESET(S,N) \
do { \
  assert ((N) <= SIZE (S) ); \
  (S).top = (S).start + (N); \
} while (0)

#define PUSH(S,E) \
do { \
  if (FULL(S)) ENLARGE (S); \
  *(S).top++ = (E); \
} while (0)

#define POP(S) \
  (assert (!EMPTY (S)), *--(S).top)

#define TOP(S) \
  (assert (!EMPTY (S)), (S).top[-1])

#define PEEK(S,P) \
  (assert ((P) < COUNT(S)), (S).start[(P)])

#define POKE(S,P,E) \
  do { assert ((P) < COUNT(S)); (S).start[(P)] = (E); } while (0)

#define RELEASE(S) \
do { \
  size_t N = SIZE (S); \
  DELN ((S).start, N); \
  INIT (S); \
} while (0)

/*------------------------------------------------------------------------*/

typedef unsigned Word;

#define LD_BITS_PER_WORD	5
#define BITS_PER_WORD		(8*sizeof (Word))
#define BITMAPMASK		(BITS_PER_WORD - 1)

#define WORD(BITS,N,IDX) \
  ((BITS)[ \
    assert ((IDX) >= 0), \
    assert (((IDX) >> LD_BITS_PER_WORD) < (N)), \
    ((IDX) >> LD_BITS_PER_WORD)])

#define BIT(IDX) \
  (((Word)1u) << ((IDX) & BITMAPMASK))

#define GETBIT(BITS,N,IDX) \
  (WORD(BITS,N,IDX) & BIT(IDX))

#define SETBIT(BITS,N,IDX) \
  do { WORD(BITS,N,IDX) |= BIT(IDX); } while (0)

#define CLRBIT(BITS,N,IDX) \
  do { WORD(BITS,N,IDX) &= ~BIT(IDX); } while (0)

#define NOTBIT(BITS,N,IDX) \
  do { WORD(BITS,N,IDX) ^= BIT(IDX); } while (0)

/*------------------------------------------------------------------------*/

#define MIN(A,B) (((A) < (B)) ? (A) : (B))
#define MAX(A,B) (((A) > (B)) ? (A) : (B))
#define ABS(A) (((A) < 0) ? (assert ((A) != INT_MIN), -(A)) : (A))

#define SWAP(T,A,B) \
  do { T TMP = (A); (A) = (B); (B) = (TMP); } while (0)

/*------------------------------------------------------------------------*/
#ifndef NDEBUG
#define LOG(...) \
do { \
  if (!yals->opts.logging.val) break; \
  yals_log_start (yals, ##__VA_ARGS__); \
  yals_log_end (yals); \
} while (0)
#define LOGLITS(LITS,...) \
do { \
  const int * P; \
  if (!yals->opts.logging.val) break; \
  yals_log_start (yals, ##__VA_ARGS__); \
  fprintf (yals->out, " clause :"); \
  for (P = (LITS); *P; P++) \
    fprintf (yals->out, " %d", *P); \
  yals_log_end (yals); \
} while (0)
#define LOGCIDX(CIDX,...) \
do { \
  const int * P, * LITS = yals_lits (yals, (CIDX)); \
  if (!yals->opts.logging.val) break; \
  yals_log_start (yals, ##__VA_ARGS__); \
  fprintf (yals->out, " clause %d :", (CIDX)); \
  for (P = (LITS); *P; P++) \
    fprintf (yals->out, " %d", *P); \
  yals_log_end (yals); \
} while (0)
#else
#define LOG(...) do { } while (0)
#define LOGLITS(...) do { } while (0)
#define LOGCIDX(...) do { } while (0)
#endif
/*------------------------------------------------------------------------*/

#define LENSHIFT	6
#define MAXLEN		((1<<LENSHIFT)-1)
#define LENMASK		MAXLEN

/*------------------------------------------------------------------------*/

enum ClausePicking {
  PSEUDO_BFS_CLAUSE_PICKING = -1,
  RANDOM_CLAUSE_PICKING = 0,
  BFS_CLAUSE_PICKING = 1,
  DFS_CLAUSE_PICKING = 2,
  RELAXED_BFS_CLAUSE_PICKING = 3,
  UNFAIR_BFS_CLAUSE_PICKING = 4,
};

/*------------------------------------------------------------------------*/

#define OPTSTEMPLATE \
  OPT (best,0,0,1,"always pick best assignment during restart"); \
  OPT (breakzero,0,0,1,"always use break zero literal if possible"); \
  OPT (cached,1,0,1,"use cached assignment during restart"); \
  OPT (cacheduni,0,0,1,"pick random cached assignment uniformly"); \
  OPT (cachemax,(1<<10),0,(1<<20),"max cache size of saved assignments"); \
  OPT (cachemin,1,0,(1<<10),"minimum cache size of saved assignments"); \
  OPT (correct,0,0,1,"correct CB value depending on maximum length"); \
  OPT (crit,1,0,1,"dynamic break values (using critical lits)"); \
  OPT (defrag,1,0,1,"defragemtation of unsat queue"); \
  OPT (fixed,4,0,INT_MAX,"fixed default strategy frequency (1=always)"); \
  OPT (geomfreq,66,0,100,"geometric picking first frequency (percent)"); \
  OPT (hitlim,-1,-1,INT_MAX,"minimum hit limit"); \
  OPT (keep,0,0,1,"keep assignment during restart"); \
  OPT (minchunksize,(1<<8),2,(1<<20),"minimum queue chunk size"); \
  OPT (pick,4,-1,4,"-1=pbfs,0=rnd,1=bfs,2=dfs,3=rbfs,4=ubfs"); \
  OPT (pol,0,-1,1,"negative=-1 positive=1 or random=0 polarity"); \
  OPT (prep,1,0,1,"preprocessing through unit propagation"); \
  OPT (rbfsrate,10,1,INT_MAX,"relaxed BFS rate"); \
  OPT (reluctant,1,0,1,"reluctant doubling of restart interval"); \
  OPT (restart,100000,0,INT_MAX,"basic (inner) restart interval"); \
  OPT (restartouter,0,0,1,"enable restart outer"); \
  OPT (restartouterfactor,100,1,INT_MAX,"outer restart interval factor"); \
  OPT (setfpu,1,0,1,"set FPU to use double precision on Linux"); \
  OPT (termint,1000,0,INT_MAX,"termination call back check interval"); \
  OPT (toggleuniform,0,0,1,"toggle uniform strategy"); \
  OPT (unfairfreq,50,0,100,"unfair picking first frequency (percent)"); \
  OPT (uni,0,-1,1,"weighted=0,uni=1,antiuni=-1 clause weights"); \
  OPT (unipick,-1,-1,4,"clause picking strategy for uniform formulas"); \
  OPT (unirestarts,0,0,INT_MAX,"max number restarts for uniform formulas"); \
  OPT (verbose,0,0,2,"set verbose level"); \
  OPT (weight,5,1,7,"maximum clause weight"); \
  OPT (witness,1,0,1,"print witness"); \
  OPTSTEMPLATENDEBUG

#ifndef NDEBUG
#define OPTSTEMPLATENDEBUG \
  OPT (logging, 0, 0, 1, "set logging level"); \
  OPT (checking, 0, 0, 1, "set checking level");
#else
#define OPTSTEMPLATENDEBUG
#endif

#define OPT(NAME,DEFAULT,MIN,MAX,DESCRIPTION) Opt NAME

/*------------------------------------------------------------------------*/

#define STRATSTEMPLATE \
  STRAT (cached,1); \
  STRAT (correct,1); \
  STRAT (pol,1); \
  STRAT (uni,1); \
  STRAT (weight,1);

#define STRAT(NAME,ENABLED) int NAME

/*------------------------------------------------------------------------*/

#ifndef NYALSMEMS
#define ADD(NAME,NUM) \
do { \
  yals->stats.mems.all += (NUM); \
  yals->stats.mems.NAME += (NUM); \
} while (0)
#else
#define ADD(NAME,NUM) do { } while (0)
#endif

#define INC(NAME) ADD (NAME, 1)

/*------------------------------------------------------------------------*/

#define assert_valid_occs(OCCS) \
  do { assert (0 <= OCCS), assert (OCCS < yals->noccs); } while (0)

#define assert_valid_idx(IDX) \
  do { assert (0 <= IDX), assert (IDX < yals->nvars); } while (0)

#define assert_valid_cidx(CIDX) \
  do { assert (0 <= CIDX), assert (CIDX < yals->nclauses); } while (0)

#define assert_valid_len(LEN) \
  do { assert (0 <= LEN), assert (LEN <= MAXLEN); } while (0)

#define assert_valid_pos(POS) \
  do { \
    assert (0 <= POS), assert (POS < COUNT (yals->unsat.stack)); \
} while (0)

/*------------------------------------------------------------------------*/

typedef struct RDS { unsigned u, v; } RDS;

typedef struct RNG { unsigned z, w; } RNG;

typedef struct Mem {
  void * mgr;
  YalsMalloc malloc;
  YalsRealloc realloc;
  YalsFree free;
} Mem;

typedef struct Strat { STRATSTEMPLATE } Strat;

typedef struct Stats {
  int best, worst, last, tmp, maxstacksize;
  int64_t flips, bzflips, hits, unsum;
  struct {
    struct { int64_t count; } outer;
    struct { int64_t count, maxint; } inner;
  } restart;
  struct { struct { int chunks, lnks; } max; int64_t unfair; } queue;
  struct { int64_t inserted, replaced, skipped; } cache;
  struct { int64_t search, neg, falsepos, truepos; } sig;
  struct { int64_t def, rnd; } strat;
  struct { int64_t best, cached, keep, pos, neg, rnd; } pick;
  struct { int64_t count, moved; } defrag;
  struct { size_t current, max; } allocated;
  struct { volatile double total, defrag, restart, entered; } time;
#ifdef __GNUC__
  volatile int flushing_time;
#endif
#ifndef NYALSMEMS
  struct { long long all, crit, lits, occs, read, update, weight; } mems;
#endif
#ifndef NYALSTATS
  int64_t * inc, * dec, broken, made; int nincdec;
  struct { unsigned min, max; } wb;
#endif
} Stats;

typedef struct Limits {
#ifndef NYALSMEMS
  int64_t mems;
#endif
  int64_t flips;
  struct { 
    struct { int64_t lim, interval; } outer;
    struct { int64_t lim; union { int64_t interval; RDS rds; }; } inner;
  } restart;
  struct { int min; } report;
  int term;
} Limits;

typedef struct Lnk {
  int cidx;
  struct Lnk * prev, * next;
} Lnk;

typedef union Chunk {
  struct { int size; union Chunk * next; };
  Lnk lnks[1];					// actually of 'size'
} Chunk;

typedef struct Queue {
  int count, chunksize, nchunks, nlnks, nfree;
  Lnk * first, * last, * free;
  Chunk * chunks; 
} Queue;

typedef struct Exp {
  struct { STACK(double) two, cb; } table;
  struct { unsigned two, cb; } max;
  struct { double two, cb; } eps;
} Exp;

typedef struct Opt { int val, def, min, max; } Opt;

typedef struct Opts { char * prefix; OPTSTEMPLATE } Opts;

typedef struct Callbacks {
  double (*time)(void);
  struct { void * state; int (*fun)(void*); } term;
  struct { void * state; void (*lock)(void*); void (*unlock)(void*); } msg;
} Callbacks;

typedef unsigned char U1;
typedef unsigned short U2;
typedef unsigned int U4;

typedef struct FPU {
#if defined(YALSAT_FPU)
  fpu_control_t control;
#endif
  int saved;
} FPU;

struct Yals {
  RNG rng;
  FILE * out;
  struct { int usequeue; Queue queue; STACK(int) stack; } unsat;
  int nvars, * refs; int64_t * flips;
  STACK(signed char) mark;
  int trivial, mt, uniform, pick;
  Word * vals, * best, * tmp, * clear, * set; int nvarwords;
  STACK(int) cdb, trail, phases, clause, mins;
  int satcntbytes; union { U1 * satcnt1; U2 * satcnt2; U4 * satcnt4; };
  int * occs, noccs; unsigned * weights;
  int * pos, * lits; Lnk ** lnk;
  int * crit; unsigned * weightedbreak;
  int nclauses, nbin, ntrn, minlen, maxlen; double avglen;
  STACK(unsigned) breaks; STACK(double) scores; STACK(int) cands;
  STACK(Word*) cache; int cachesizetarget; STACK(Word) sigs;
  STACK(int) minlits;
  Callbacks cbs;
  Limits limits;
  Strat strat;
  Stats stats;
  Opts opts;
  Mem mem;
  FPU fpu;
  Exp exp;
};

/*------------------------------------------------------------------------*/

const char * yals_default_prefix () { return YALS_DEFAULT_PREFIX; }

/*------------------------------------------------------------------------*/

static void yals_msglock (Yals * yals) {
  if (yals->cbs.msg.lock) yals->cbs.msg.lock (yals->cbs.msg.state);
}

static void yals_msgunlock (Yals * yals) {
  if (yals->cbs.msg.unlock) yals->cbs.msg.unlock (yals->cbs.msg.state);
}

void yals_abort (Yals * yals, const char * fmt, ...) {
  va_list ap;
  yals_msglock (yals);
  fflush (yals->out);
  fprintf (stderr, "%s*** libyals abort: ", yals->opts.prefix);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
  fflush (stderr);
  yals_msgunlock (yals);
  abort ();
}

void yals_exit (Yals * yals, int exit_code, const char * fmt, ...) {
  va_list ap;
  yals_msglock (yals);
  fflush (yals->out);
  fprintf (stderr, "%s*** libyals exit: ", yals->opts.prefix);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
  fflush (stderr);
  yals_msgunlock (yals);
  exit (exit_code);
}

void yals_warn (Yals * yals, const char * fmt, ...) {
  va_list ap;
  yals_msglock (yals);
  fprintf (yals->out, "%sWARNING ", yals->opts.prefix);
  va_start (ap, fmt);
  vfprintf (yals->out, fmt, ap);
  va_end (ap);
  fputc ('\n', yals->out);
  fflush (yals->out);
  yals_msgunlock (yals);
}

void yals_msg (Yals * yals, int level, const char * fmt, ...) {
  va_list ap;
  if (level > 0 && (!yals || yals->opts.verbose.val < level)) return;
  yals_msglock (yals);
  fprintf (yals->out, "%s", yals->opts.prefix);
  va_start (ap, fmt);
  vfprintf (yals->out, fmt, ap);
  va_end (ap);
  fputc ('\n', yals->out);
  fflush (yals->out);
  yals_msgunlock (yals);
}

/*------------------------------------------------------------------------*/

static void yals_set_fpu (Yals * yals) {
#if defined(YALSAT_FPU)
  fpu_control_t control;
  _FPU_GETCW (yals->fpu.control);
  control = yals->fpu.control;
  control &= ~_FPU_EXTENDED;
  control &= ~_FPU_SINGLE;
  control |= _FPU_DOUBLE;
  _FPU_SETCW (control);
  yals_msg (yals, 1, "set FPU mode to use double precision");
#endif
  yals->fpu.saved = 1;
}

static void yals_reset_fpu (Yals * yals) {
  (void) yals;
  assert (yals->fpu.saved);
#if defined(YALSAT_FPU)
  _FPU_SETCW (yals->fpu.control);
  yals_msg (yals, 1, "reset FPU to original double precision mode");
#endif
}

/*------------------------------------------------------------------------*/
#ifndef NDEBUG

static void yals_log_start (Yals * yals, const char * fmt, ...) {
  va_list ap;
  yals_msglock (yals);
  assert (yals->opts.logging.val);
  fputs ("c [LOGYALS] ", yals->out);
  va_start (ap, fmt);
  vfprintf (yals->out, fmt, ap);
  va_end (ap);
}

static void yals_log_end (Yals * yals) {
  (void) yals;
  assert (yals->opts.logging.val);
  fputc ('\n', yals->out);
  fflush (yals->out);
  yals_msgunlock (yals);
}

#endif
/*------------------------------------------------------------------------*/

static double yals_avg (double a, double b) { return b ? a/b : 0; }

static double yals_pct (double a, double b) { return b ? 100.0 * a / b : 0; }

/*double yals_process_time () {
  struct rusage u;
  double res;
  if (getrusage (RUSAGE_SELF, &u)) return 0;
  res = u.ru_utime.tv_sec + 1e-6 * u.ru_utime.tv_usec;
  res += u.ru_stime.tv_sec + 1e-6 * u.ru_stime.tv_usec;
  return res;
}*/

// note: MinGW64 defines both __MINGW32__ and __MINGW64__
#if defined (_MSC_VER) || defined (__MINGW32__) || defined(_WIN32)
#include <time.h>
double yals_process_time(void)
{
    return (double)clock() / CLOCKS_PER_SEC;
}

#else //Linux or POSIX
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

double yals_process_time(void)
{
    struct rusage u;
    if (getrusage (RUSAGE_SELF, &u)) return 0;
    double res;
    res = u.ru_utime.tv_sec + 1e-6 * u.ru_utime.tv_usec;
    res += u.ru_stime.tv_sec + 1e-6 * u.ru_stime.tv_usec;
    return res;
}
#endif


static double yals_time (Yals * yals) {
  if (yals && yals->cbs.time) return yals->cbs.time ();
  else return yals_process_time ();
}

static void yals_flush_time (Yals * yals) {
  double time, entered;
#ifdef __GNUC__
  int old;
  // begin{atomic}
  old = __sync_val_compare_and_swap (&yals->stats.flushing_time, 0, 42);
  assert (old == 0 || old == 42);
  if (old) return;
  //
  // TODO I still occasionally have way too large kflips/sec if interrupted
  // and I do not know why?  Either there is a bug in flushing or there is
  // still a data race here and I did not apply this CAS sequence correctly.
  //
#endif
  time = yals_time (yals);
  entered = yals->stats.time.entered;
  yals->stats.time.entered = time;
  assert (time >= entered);
  time -= entered;
  yals->stats.time.total += time;
#ifdef __GNUC__
  old = __sync_val_compare_and_swap (&yals->stats.flushing_time, 42, 0);
  assert (old == 42);
  (void) old;
  // end{atomic}
#endif
}

double yals_sec (Yals * yals) {
  yals_flush_time (yals);
  return yals->stats.time.total;
}

/*------------------------------------------------------------------------*/

static void yals_inc_allocated (Yals * yals, size_t bytes) {
  yals->stats.allocated.current += bytes;
  if (yals->stats.allocated.current > yals->stats.allocated.max)
    yals->stats.allocated.max = yals->stats.allocated.current;
}

static void yals_dec_allocated (Yals * yals, size_t bytes) {
  assert (yals->stats.allocated.current >= bytes);
  yals->stats.allocated.current -= bytes;
}

void * yals_malloc (Yals * yals, size_t bytes) {
  void * res;
  if (!bytes) return 0;
  res = yals->mem.malloc (yals->mem.mgr, bytes);
  if (!res) yals_abort (yals, "out of memory in 'yals_malloc'");
  yals_inc_allocated (yals, bytes);
  memset (res, 0, bytes);
  return res;
}

void yals_free (Yals * yals, void * ptr, size_t bytes) {
  assert (!ptr == !bytes);
  if (!ptr) return;
  yals_dec_allocated (yals, bytes);
  yals->mem.free (yals->mem.mgr, ptr, bytes);
}

void * yals_realloc (Yals * yals, void * ptr, size_t o, size_t n) {
  char * res;
  assert (!ptr == !o);
  if (!n) { yals_free (yals, ptr, o); return 0; }
  if (!o) return yals_malloc (yals, n);
  yals_dec_allocated (yals, o);
  res = yals->mem.realloc (yals->mem.mgr, ptr, o, n);
  if (n && !res) yals_abort (yals, "out of memory in 'yals_realloc'");
  yals_inc_allocated (yals, n);
  if (n > o) memset (res + o, 0, n - o);
  return res;
}

size_t yals_max_allocated (Yals * yals) {
  return yals->stats.allocated.max;
}

/*------------------------------------------------------------------------*/

static char * yals_strdup (Yals * yals, const char * str) {
  assert (str);
  return strcpy (yals_malloc (yals, strlen (str) + 1), str);
}

static void yals_strdel (Yals * yals, char * str) {
  assert (str);
  yals_free (yals, str, strlen (str) + 1);
}

/*------------------------------------------------------------------------*/

static void yals_rds (RDS * r) {
  if ((r->u & -r->u) == r->v) r->u++, r->v = 1; else r->v *= 2;
}

/*------------------------------------------------------------------------*/

void yals_srand (Yals * yals, unsigned long long seed) {
  unsigned z = seed >> 32, w = seed;
  if (!z) z = ~z;
  if (!w) w = ~w;
  yals->rng.z = z, yals->rng.w = w;
  yals_msg (yals, 2, "setting random seed %llu", seed);
}

static unsigned yals_rand (Yals * yals) {
  unsigned res;
  yals->rng.z = 36969 * (yals->rng.z & 65535) + (yals->rng.z >> 16);
  yals->rng.w = 18000 * (yals->rng.w & 65535) + (yals->rng.w >> 16);
  res = (yals->rng.z << 16) + yals->rng.w;
  return res;
}

static unsigned yals_rand_mod (Yals * yals, unsigned mod) {
  unsigned res;
  assert (mod >= 1);
  if (mod <= 1) return 0;
  res = yals_rand (yals);
  res %= mod;
  return res;
}

/*------------------------------------------------------------------------*/

static int * yals_refs (Yals * yals, int lit) {
  int idx = ABS (lit);
  assert_valid_idx (idx);
  assert (yals->refs);
  return yals->refs + 2*idx + (lit < 0);
}

static int * yals_occs (Yals * yals, int lit) {
  int occs;
  INC (occs);
  occs = *yals_refs (yals, lit);
  assert_valid_occs (occs);
  return yals->occs + occs;
}

static int yals_val (Yals * yals, int lit) {
  int idx = ABS (lit), res = !GETBIT (yals->vals, yals->nvarwords, idx);
  if (lit > 0) res = !res;
  return res;
}

static int yals_best (Yals * yals, int lit) {
  int idx = ABS (lit), res = !GETBIT (yals->best, yals->nvarwords, idx);
  if (lit > 0) res = !res;
  return res;
}

static unsigned yals_weighted_break (Yals * yals, int lit) {
  int idx = ABS (lit);
  assert (yals->crit);
  assert_valid_idx (idx);
  return yals->weightedbreak[2*idx + (lit < 0)];
}

static void yals_inc_weighted_break (Yals * yals, int lit, int len) {
  int idx = ABS (lit), pos;
  unsigned w;
  assert (yals->crit);
  assert_valid_idx (idx);
  assert_valid_len (len);
  pos = 2*idx + (lit < 0);
  w = yals->weights[len];		// TODO avoid mem if uniform
  yals->weightedbreak[pos] += w;
  assert (yals->weightedbreak[pos] >= w);
  INC (weight);
}

static void yals_dec_weighted_break (Yals * yals, int lit, int len) {
  int idx = ABS (lit), pos;
  unsigned w;
  assert (yals->crit);
  assert_valid_idx (idx);
  assert_valid_len (len);
  pos = 2*idx + (lit < 0);
  w = yals->weights[len];		// TODO avoid mem if uniform
  assert (yals->weightedbreak[pos] >= w);
  yals->weightedbreak[pos] -= w;
  INC (weight);
}

static unsigned yals_satcnt (Yals * yals, int cidx) {
  assert_valid_cidx (cidx);
  if (yals->satcntbytes == 1) return yals->satcnt1[cidx];
  if (yals->satcntbytes == 2) return yals->satcnt2[cidx];
  return yals->satcnt4[cidx];
}

static void yals_setsatcnt (Yals * yals, int cidx, unsigned satcnt) {
  assert_valid_cidx (cidx);
  if (yals->satcntbytes == 1) {
    assert (satcnt < 256);
    yals->satcnt1[cidx] = satcnt;
  } else if (yals->satcntbytes == 2) {
    assert (satcnt < 65536);
    yals->satcnt2[cidx] = satcnt;
  } else {
    yals->satcnt4[cidx] = satcnt;
  }
}

static unsigned yals_incsatcnt (Yals * yals, int cidx, int lit, int len) {
  unsigned res;
  assert_valid_cidx (cidx);
  assert_valid_len (len);
  if (yals->satcntbytes == 1) {
    res = yals->satcnt1[cidx]++;
    assert (yals->satcnt1[cidx]);
  } else if (yals->satcntbytes == 2) {
    res = yals->satcnt2[cidx]++;
    assert (yals->satcnt2[cidx]);
  } else {
    res = yals->satcnt4[cidx]++;
    assert (yals->satcnt4[cidx]);
  }
#ifndef NYALSTATS
  assert (res + 1 <= yals->maxlen);
  yals->stats.inc[res]++;
#endif
  if (yals->crit) {
    if (res == 1) yals_dec_weighted_break (yals, yals->crit[cidx], len);
    else if (!res) yals_inc_weighted_break (yals, lit, len);
    yals->crit[cidx] ^= lit;
    assert (res || yals->crit[cidx] == lit);
  }
  return res;
}

static unsigned yals_decsatcnt (Yals * yals, int cidx, int lit, int len) {
  unsigned res;
  assert_valid_cidx (cidx);
  assert_valid_len (len);
  if (yals->satcntbytes == 1) {
    assert (yals->satcnt1[cidx]);
    res = --yals->satcnt1[cidx];
  } else if (yals->satcntbytes == 2) {
    assert (yals->satcnt2[cidx]);
    res = --yals->satcnt2[cidx];
  } else {
    assert (yals->satcnt4[cidx]);
    res = --yals->satcnt4[cidx];
  }
#ifndef NYALSTATS
  assert (res + 1 <= yals->maxlen);
  yals->stats.dec[res + 1]++;
#endif
  if (yals->crit) {
    int other = yals->crit[cidx] ^ lit;
    yals->crit[cidx] = other;
    if (res == 1) yals_inc_weighted_break (yals, other, len);
    else if (!res) yals_dec_weighted_break (yals, lit, len);
    assert (res || !yals->crit[cidx]);
  }
  return res;
}

static int * yals_lits (Yals * yals, int cidx) {
  INC (lits);
  assert_valid_cidx (cidx);
  return yals->cdb.start + yals->lits[cidx];
}

/*------------------------------------------------------------------------*/

static void yals_report (Yals * yals, const char * fmt, ...) {
  double t, f;
  va_list ap;
  assert (yals->opts.verbose.val);
  yals_msglock (yals);
  f = yals->stats.flips;
  t = yals_sec (yals);
  fprintf (yals->out, "%s", yals->opts.prefix);
  va_start (ap, fmt);
  vfprintf (yals->out, fmt, ap);
  va_end (ap);
  fprintf (yals->out,
    " : best %d (tmp %d), kflips %.0f, %.2f sec, %.2f kflips/sec\n",
    yals->stats.best, yals->stats.tmp, f/1e3, t, yals_avg (f/1e3, t));
  fflush (yals->out);
  yals_msgunlock (yals);
}

static int yals_nunsat (Yals * yals) {
  if (yals->unsat.usequeue) return yals->unsat.queue.count;
  else return COUNT (yals->unsat.stack);
}

static void yals_save_new_minimum (Yals * yals) {
  int nunsat = yals_nunsat (yals);
  size_t bytes = yals->nvarwords * sizeof (Word);
  if (yals->stats.worst < nunsat) yals->stats.worst = nunsat;
  if (yals->stats.tmp > nunsat) {
    LOG ("minimum %d is best assignment since last restart", nunsat);
    memcpy (yals->tmp, yals->vals, bytes);
    yals->stats.tmp = nunsat;
  }
  if (yals->stats.best < nunsat) return;
  if (yals->stats.best == nunsat) { 
    LOG ("minimum %d matches previous overall best assignment", nunsat);
    yals->stats.hits++;
    return;
  }
  LOG ("minimum %d with new best overall assignment", nunsat);
  yals->stats.best = nunsat;
  yals->stats.hits = 1;
  memcpy (yals->best, yals->vals, bytes);
  if (yals->opts.verbose.val >= 2 ||
      (yals->opts.verbose.val >= 1 && nunsat <= yals->limits.report.min/2)) {
    yals_report (yals, "new minimum");
    yals->limits.report.min = nunsat;
  }
}

/*------------------------------------------------------------------------*/

static void yals_check_global_invariant (Yals * yals) {
#ifndef NDEBUG
  int cidx, lit, nunsat = 0;
  const int * p;
  assert (BITS_PER_WORD == (1 << LD_BITS_PER_WORD));
  if (!yals->opts.checking.val) return;
  for (cidx = 0; cidx < yals->nclauses; cidx++) {
    unsigned sat;
    for (p = yals_lits (yals, cidx), sat = 0; (lit = *p); p++)
      if (yals_val (yals, lit)) sat++;
    assert (yals_satcnt (yals, cidx) == sat);
    if (!sat) nunsat++;
    if (yals->unsat.usequeue) {
      Lnk * l = yals->lnk[cidx];
      assert (l);
      assert (l->cidx == cidx);
    } else {
      int pos = yals->pos[cidx];
      if (sat) assert (pos < 0);
      else {
	assert_valid_pos (pos);
	assert (PEEK (yals->unsat.stack, pos) == cidx);
      }
    }
  }
  assert (nunsat == yals_nunsat (yals));
#endif
  (void) yals;
}

/*------------------------------------------------------------------------*/

static void yals_enqueue_lnk (Yals * yals, Lnk * l) {
  Lnk * last = yals->unsat.queue.last;
  if (last) last->next = l;
  else yals->unsat.queue.first = l;
  yals->unsat.queue.last = l;
  l->prev = last;
  l->next = 0;
}

static void yals_dequeue_lnk (Yals * yals, Lnk * l) {
  Lnk * prev = l->prev, * next = l->next;
  if (prev) {
    assert (yals->unsat.queue.first);
    assert (yals->unsat.queue.last);
    assert (prev->next == l);
    prev->next = next;
  } else {
    assert (yals->unsat.queue.first == l);
    yals->unsat.queue.first = next;
  }
  if (next) {
    assert (yals->unsat.queue.first);
    assert (yals->unsat.queue.last);
    assert (next->prev == l);
    next->prev = prev;
  } else {
    assert (yals->unsat.queue.last == l);
    yals->unsat.queue.last = prev;
  }
}

/*------------------------------------------------------------------------*/

static const char * yals_pick_to_str (Yals * yals) {
  switch (yals->pick) {
    case BFS_CLAUSE_PICKING:
      return "clause by BFS strategy";
    case RELAXED_BFS_CLAUSE_PICKING:
      return "clause by relaxed BFS strategy";
    case DFS_CLAUSE_PICKING:
      return "clause by DFS strategy";
    case PSEUDO_BFS_CLAUSE_PICKING:
      return "clause by pseudo BFS strategy";
    case UNFAIR_BFS_CLAUSE_PICKING:
      return "clause by unfair BFS strategy";
    case RANDOM_CLAUSE_PICKING:
    default:
      assert (yals->pick == RANDOM_CLAUSE_PICKING);
      return "uniformly random clause";
  }
}

static int yals_pick_clause (Yals * yals) {
  int cidx, nunsat = yals_nunsat (yals);
  assert (nunsat > 0);
  if (yals->unsat.usequeue) {
    Lnk * lnk;
    if (yals->pick == BFS_CLAUSE_PICKING) {
      lnk = yals->unsat.queue.first;
    } else if (yals->pick == RELAXED_BFS_CLAUSE_PICKING) {
      lnk = yals->unsat.queue.first;
      while (lnk->next && !yals_rand_mod (yals, yals->opts.rbfsrate.val))
	lnk = lnk->next;
    } else if (yals->pick == UNFAIR_BFS_CLAUSE_PICKING) {
      if (yals->unsat.queue.count > 1 &&
	  yals_rand_mod (yals, 100) >= yals->opts.unfairfreq.val) {
	lnk = yals->unsat.queue.first;
	yals_dequeue_lnk (yals, lnk);
	yals_enqueue_lnk (yals, lnk);
	yals->stats.queue.unfair++;
      }
      lnk = yals->unsat.queue.first;
    } else {
      assert (yals->pick == DFS_CLAUSE_PICKING);
      lnk = yals->unsat.queue.last;
    }
    assert (lnk);
    cidx = lnk->cidx;
  } else {
    int cpos;
    if (yals->pick == PSEUDO_BFS_CLAUSE_PICKING) {
      cpos = yals->stats.flips % nunsat;
    } else {
      assert (yals->pick == RANDOM_CLAUSE_PICKING);
      cpos = yals_rand_mod (yals, nunsat);
    }
    cidx = PEEK (yals->unsat.stack, cpos);
    assert (yals->pos[cidx] == cpos);
  }
  assert_valid_cidx (cidx);
  LOG ("picking %s out of %d", yals_pick_to_str (yals), nunsat);
  assert (!yals_satcnt (yals, cidx));
  LOGCIDX (cidx, "picked");
  return cidx;
}

/*------------------------------------------------------------------------*/

static unsigned yals_dynamic_weighted_break (Yals * yals, int lit) {
  unsigned res = yals_weighted_break (yals, -lit);
  LOG ("literal %d results in weighted break %u", lit, res);
  return res;
}

#define ACCU(SUM,INC) \
do { \
  if (UINT_MAX - (INC) < (SUM)) (SUM) = UINT_MAX; else (SUM) += (INC); \
} while (0)

static unsigned yals_compute_weighted_break (Yals * yals, int lit) {
  unsigned wb, b, w, cnt;
  const int * p, * occs;
  int occ, cidx, len;

  assert (!yals_val (yals, lit));
  wb = b = 0;
  occs = yals_occs (yals, -lit);
  for (p = occs ; (occ = *p) >= 0; p++) {
    len = occ & LENMASK;
    cidx = occ >> LENSHIFT;
    cnt = yals_satcnt (yals, cidx);
    if (cnt != 1) continue;
    w = yals->weights [len];		  // TODO remove mem for uniform
    assert (0 < w && w <= yals->strat.weight);
    ACCU (wb, w);
    b++;
  }

  LOG ("literal %d breaks %u clauses out of %d results in weighted break %u",
    lit, b, (int)(p - occs), wb);

  ADD (read, p - occs);

  return wb;
}

static unsigned yals_determine_weighted_break (Yals * yals, int lit) {
  unsigned res;
  if (yals->crit) res = yals_dynamic_weighted_break (yals, lit);
  else res = yals_compute_weighted_break (yals, lit);
#ifndef NYALSTATS
  if (yals->stats.wb.max < res) yals->stats.wb.max = res;
  if (yals->stats.wb.min > res) yals->stats.wb.min = res;
#endif
  return res;
}

/*------------------------------------------------------------------------*/

static double
yals_compute_score_from_weighted_break (Yals * yals, unsigned w) {

  double s;

  if (yals->strat.correct)
    s = (w < yals->exp.max.cb) ? 
          PEEK (yals->exp.table.cb, w) : yals->exp.eps.cb;
  else
    s = (w < yals->exp.max.two) ? 
          PEEK (yals->exp.table.two, w) : yals->exp.eps.two;

  assert (s);

  LOG ("weighted break %u gives score %g", w, s);

  return s;
}

static int yals_pick_by_score (Yals * yals) {
  double s, lim, sum;
  const double * q;
  const int * p;
  int res;

  assert (!EMPTY (yals->scores));
  assert (COUNT (yals->scores) == COUNT (yals->cands));

  sum = 0;
  for (q = yals->scores.start; q < yals->scores.top; q++)
    sum += *q;

  assert (sum > 0);
  lim = (yals_rand (yals) / (1.0 + (double) UINT_MAX))*sum;
  assert (lim <= sum);

  LOG ("random choice %g mod %g", lim, sum);

  p = yals->cands.start;	assert (p < yals->cands.top);
  res = *p++;

  q = yals->scores.start;	assert (q < yals->scores.top);
  s = *q++;			assert (s > 0);

  while (p < yals->cands.top && s <= lim) {
    lim -= s;
    res = *p++;			assert (q < yals->scores.top);
    s = *q++;			assert (s > 0);
  }

  return res;
}

/*------------------------------------------------------------------------*/

static int yals_pick_literal (Yals * yals, int cidx) {
  const int pick_break_zero = yals->opts.breakzero.val;
  const int * p, * lits;
  int lit, zero;
  unsigned w;
  double s;

  assert (EMPTY (yals->breaks));
  assert (EMPTY (yals->cands));

  lits = yals_lits (yals, cidx);

  zero = 0;
  for (p = lits; (lit = *p); p++) {
    w = yals_determine_weighted_break (yals, lit);
    LOG ("literal %d weighted break %u", lit, w);
    if (pick_break_zero && !w) {
      if (!zero++) CLEAR (yals->cands);
      PUSH (yals->cands, lit);
    } else if (!zero) {
      PUSH (yals->breaks, w);
      PUSH (yals->cands, lit);
    }
  }

  if (zero) {

    yals->stats.bzflips++;
    assert (zero == COUNT (yals->cands));
    lit = yals->cands.start[yals_rand_mod (yals, zero)];
    LOG ("picked random break zero literal %d out of %d", lit, zero);

  } else {

    const unsigned * wbs = yals->breaks.start;
    const unsigned n = COUNT (yals->breaks);
    unsigned i;

    assert (EMPTY (yals->scores));

    for (i = 0; i < n; i++) {
      w = wbs[i];
      s = yals_compute_score_from_weighted_break (yals, w);
      LOG ("literal %d weighted break %u score %g", lits[i], w, s);
      PUSH (yals->scores, s);
    }
    lit = yals_pick_by_score (yals);

#ifndef NDEBUG
    for (i = 0; i < n; i++) {
      int tmp = lits[i];
      if (tmp != lit) continue;
      s = yals->scores.start[i];
      w = wbs[i];
      break;
    }
#endif
    LOG ("picked literal %d weigted break %d score %g", lit, w, s);

    CLEAR (yals->scores);
  }

  CLEAR (yals->cands);
  CLEAR (yals->breaks);

  return lit;
}

/*------------------------------------------------------------------------*/

static void yals_flip_value_of_lit (Yals * yals, int lit) {
  int idx = ABS (lit);
  LOG ("flipping %d", lit);
  NOTBIT (yals->vals, yals->nvarwords, idx);
  yals->flips[idx]++;
}

/*------------------------------------------------------------------------*/

static void yals_flush_queue (Yals * yals) {
  int count = 0;
  Lnk * p;
  assert (yals->unsat.usequeue);
  for (p = yals->unsat.queue.first; p; p = p->next) {
    int cidx = p->cidx;
    assert_valid_cidx (cidx);
    assert (yals->lnk[cidx] == p);
    yals->lnk[cidx] = 0;
    count++;
  }
  yals->unsat.queue.first = yals->unsat.queue.last = 0;
  LOG ("flushed %d queue elements", count);
  assert (count == yals->unsat.queue.count);
  yals->unsat.queue.count = 0;
}

static void yals_release_lnks (Yals * yals) {
  int chunks = 0, lnks = 0;
  Chunk * q, * n;
  assert (yals->unsat.usequeue);
  for (q = yals->unsat.queue.chunks; q; q = n) {
    n = q->next;
    chunks++, lnks += q->size - 1;
    DELN (&q->lnks, q->size);
  }
  LOG ("released %d links in %d chunks", lnks, chunks);
  assert (yals->unsat.queue.nchunks == chunks);
  assert (yals->unsat.queue.nlnks == lnks);
  yals->unsat.queue.chunks = 0;
  yals->unsat.queue.free = 0;
  yals->unsat.queue.nfree = 0;
  yals->unsat.queue.chunksize = 0;
  yals->unsat.queue.nchunks = 0;
  yals->unsat.queue.nlnks = 0;
}

static void yals_reset_unsat_queue (Yals * yals) {
  assert (yals->unsat.usequeue);
  yals_flush_queue (yals);
  yals_release_lnks (yals);
}

static void yals_defrag_queue (Yals * yals) {
  const int count = yals->unsat.queue.count;
  const int size = MAX(2*(count + 1), yals->opts.minchunksize.val);
  Lnk * p, * first, * free, * prev = 0;
  double start = yals_time (yals);
  const Lnk * q;
  Chunk * c;
  assert (count);
  yals->stats.defrag.count++;
  yals->stats.defrag.moved += count;
  LOG ("defragmentation chunk of size %d moving %d", size, count);
  NEWN (p, size);
  c = (Chunk*) p;
  c->size = size;
  first = c->lnks + 1;
  for (q = yals->unsat.queue.first, p = first; q; q = q->next, p++) {
    *p = *q;
    p->prev = prev;
    if (prev) prev->next = p;
    prev = p;
  }
  assert (prev);
  prev->next = 0;
  free = p;
  yals_reset_unsat_queue (yals);
  assert (prev);
  yals->unsat.queue.first = first;
  yals->unsat.queue.last = prev;
  yals->unsat.queue.count = count;
  for (p = yals->unsat.queue.first; p; p = p->next) {
    int cidx = p->cidx;
    assert_valid_cidx (cidx);
    assert (!yals->lnk[cidx]);
    yals->lnk[cidx] = p;
    assert (!p->next || p->next == p + 1);
  }
  assert (free < c->lnks + size);
  yals->unsat.queue.nfree = (c->lnks + size) - free;
  assert (yals->unsat.queue.nfree > 0);
  prev = 0;
  for (p = c->lnks + size-1; p >= free; p--) p->next = prev, prev = p;
  yals->unsat.queue.free = free;
  assert (!c->next);
  yals->unsat.queue.chunks = c;
  yals->unsat.queue.nchunks = 1;
  yals->unsat.queue.nlnks = size - 1;
  assert (yals->stats.queue.max.lnks >= yals->unsat.queue.nlnks);
  assert (yals->stats.queue.max.chunks >= yals->unsat.queue.nchunks);
  yals->stats.time.defrag += yals_time (yals) - start;
}


static int yals_need_to_defrag_queue (Yals * yals) {
  if (!yals->opts.defrag.val) return 0;
  if (!yals->unsat.queue.count) return 0;
  if (yals->unsat.queue.nlnks <= yals->opts.minchunksize.val) return 0;
  if (yals->unsat.queue.count > yals->unsat.queue.nfree/4) return 0;
  return 1;
}

static void yals_dequeue_queue (Yals * yals, int cidx) {
  Lnk * l;
  assert (yals->unsat.usequeue);
  assert (yals->unsat.queue.count > 0);
  yals->unsat.queue.count--;
  l = yals->lnk[cidx];
  assert (l);
  assert (l->cidx == cidx);
  yals->lnk[cidx] = 0;
  yals_dequeue_lnk (yals, l);
  l->next = yals->unsat.queue.free;
  yals->unsat.queue.free = l;
  yals->unsat.queue.nfree++;
  assert (yals->unsat.queue.nlnks ==
          yals->unsat.queue.nfree + yals->unsat.queue.count);
  if (yals_need_to_defrag_queue (yals)) yals_defrag_queue (yals);
}

static void yals_dequeue_stack (Yals * yals, int cidx) {
  int cpos = yals->pos[cidx], didx;
  assert (!yals->unsat.usequeue);
  assert_valid_pos (cpos);
  assert (PEEK (yals->unsat.stack, cpos) == cidx);
  didx = POP (yals->unsat.stack);
  assert_valid_cidx (didx);
  if (didx != cidx) {
    assert (yals->pos[didx] == COUNT (yals->unsat.stack));
    POKE (yals->unsat.stack, cpos, didx);
    yals->pos[didx] = cpos;
  }
  yals->pos[cidx] = -1;
}

static void yals_dequeue (Yals * yals, int cidx) {
  LOG ("dequeue %d", cidx);
  assert_valid_cidx (cidx);
  if (yals->unsat.usequeue) yals_dequeue_queue (yals, cidx);
  else yals_dequeue_stack (yals, cidx);
}

static void yals_new_chunk (Yals * yals) {
  Lnk * p, * first, * prev = 0;
  Chunk * c;
  int size;
  size = yals->unsat.queue.chunksize;
  assert (size >= yals->opts.minchunksize.val);
  LOG ("new chunk of size %d", size);
  NEWN (p, size);
  c = (Chunk*) p;
  c->size = size;
  first = c->lnks + 1;
  for (p = c->lnks + size-1; p >= first; p--) p->next = prev, prev = p;
  yals->unsat.queue.nfree += size-1;
  yals->unsat.queue.free = first;
  c->next = yals->unsat.queue.chunks;
  yals->unsat.queue.chunks = c;
  yals->unsat.queue.nlnks += size - 1;
  if (yals->stats.queue.max.lnks < yals->unsat.queue.nlnks)
    yals->stats.queue.max.lnks = yals->unsat.queue.nlnks;
  if (yals->stats.queue.max.chunks < ++yals->unsat.queue.nchunks)
    yals->stats.queue.max.chunks = yals->unsat.queue.nchunks;
}

static void yals_larger_new_chunk (Yals * yals) {
  if (!yals->unsat.queue.chunksize)
    yals->unsat.queue.chunksize = yals->opts.minchunksize.val;
  else yals->unsat.queue.chunksize *= 2;
  yals_new_chunk (yals);
}

static Lnk * yals_new_lnk (Yals * yals) {
  Lnk * res = yals->unsat.queue.free;
  if (!res) {
    yals_larger_new_chunk (yals);
    res = yals->unsat.queue.free;
  }
  yals->unsat.queue.free = res->next;
  assert (yals->unsat.queue.nfree);
  yals->unsat.queue.nfree--;
  return res;
}

static void yals_enqueue_queue (Yals * yals, int cidx) {
  Lnk * res;
  assert (yals->unsat.usequeue);
  res = yals_new_lnk (yals);
  assert (!yals->lnk[cidx]);
  yals->lnk[cidx] = res;
  res->cidx = cidx;
  yals_enqueue_lnk (yals, res);
  yals->unsat.queue.count++;
  assert (yals->unsat.queue.count > 0);
  assert (yals->unsat.queue.nlnks ==
          yals->unsat.queue.nfree + yals->unsat.queue.count);
}

static void yals_enqueue_stack (Yals * yals, int cidx) {
  int size;
  assert (!yals->unsat.usequeue);
  assert (yals->pos[cidx] < 0);
  yals->pos[cidx] = COUNT (yals->unsat.stack);
  PUSH (yals->unsat.stack, cidx);
  if (yals->stats.maxstacksize < (size = SIZE (yals->unsat.stack)))
    yals->stats.maxstacksize = size;
}

static void yals_enqueue (Yals * yals, int cidx) {
  LOG ("enqueue %d", cidx);
  assert_valid_cidx (cidx);
  if (yals->unsat.usequeue) yals_enqueue_queue (yals, cidx);
  else yals_enqueue_stack (yals, cidx);
}

static void yals_reset_unsat_stack (Yals * yals) {
  assert (!yals->unsat.usequeue);
  while (!EMPTY (yals->unsat.stack)) {
    int cidx = POP (yals->unsat.stack);
    assert_valid_cidx (cidx);
    assert (yals->pos[cidx] == COUNT (yals->unsat.stack));
    yals->pos[cidx] = -1;
  }
  RELEASE (yals->unsat.stack);
}

static void yals_reset_unsat (Yals * yals) {
  if (yals->unsat.usequeue) yals_reset_unsat_queue (yals);
  else yals_reset_unsat_stack (yals);
}

/*------------------------------------------------------------------------*/

static void yals_make_clauses_after_flipping_lit (Yals * yals, int lit) {
  const int * p, * occs;
  int cidx, len, occ;
#if !defined(NDEBUG) || !defined(NYALSTATS)
  int made = 0;
#endif
  assert (yals_val (yals, lit));
  occs = yals_occs (yals, lit);
  for (p = occs; (occ = *p) >= 0; p++) {
    len = occ & LENMASK;
    cidx = occ >> LENSHIFT;
    if (yals_incsatcnt (yals, cidx, lit, len)) continue;
    yals_dequeue (yals, cidx);
    LOGCIDX (cidx, "made");
#if !defined(NDEBUG) || !defined(NYALSTATS)
    made++;
#endif
  }
  LOG ("flipping %d has made %d clauses", lit, made);
#ifndef NYALSMEMS
  {
    int updated = p - occs;
    ADD (update, updated);
    if (yals->crit) ADD (crit, updated);
  }
#endif
#ifndef NYALSTATS
  yals->stats.made += made;
#endif
}

static void yals_break_clauses_after_flipping_lit (Yals * yals, int lit) {
  const int * p, * occs;
  int occ, cidx, len;
#if !defined(NDEBUG) || !defined(NYALSTATS)
  int broken = 0;
#endif
  occs = yals_occs (yals, -lit);
  for (p = occs; (occ = *p) >= 0; p++) {
    len = occ & LENMASK;
    cidx = occ >> LENSHIFT;
    if (yals_decsatcnt (yals, cidx, -lit, len)) continue;
    yals_enqueue (yals, cidx);
    LOGCIDX (cidx, "broken");
#if !defined(NDEBUG) || !defined(NYALSTATS)
    broken++;
#endif
  }
  LOG ("flipping %d has broken %d clauses", lit, broken);
#ifndef NYALSMEMS
  {
    int updated = p - occs;
    ADD (update, updated);
    if (yals->crit) ADD (crit, updated);
  }
#endif
#ifndef NYALSTATS
  yals->stats.broken += broken;
#endif
}

static void yals_update_minimum (Yals * yals) {
  yals_save_new_minimum (yals);
  LOG ("now %d clauses unsatisfied", yals_nunsat (yals));
  yals_check_global_invariant (yals);
}

static void yals_flip (Yals * yals) {
  int cidx = yals_pick_clause (yals);
  int lit = yals_pick_literal (yals, cidx);
  yals->stats.flips++;
  yals->stats.unsum += yals_nunsat (yals);
  yals_flip_value_of_lit (yals, lit);
  yals_make_clauses_after_flipping_lit (yals, lit);
  yals_break_clauses_after_flipping_lit (yals, lit);
  yals_update_minimum (yals);
}

/*------------------------------------------------------------------------*/

static void yals_preprocess (Yals * yals) {
  int nvars = yals->nvars, lit, other, next, occ, w0, w1;
  int * p, * c, * q, oldnlits, newnlits, satisfied, nsat, nstr;
  STACK(int) * watches, * s;
  signed char * vals;

  FIT (yals->cdb);

  NEWN (vals, 2*nvars);
  vals += nvars;

  NEWN (watches, 2*nvars);
  watches += nvars;

  for (c = yals->cdb.start; c < yals->cdb.top; c++) {
    occ = c - yals->cdb.start;
    assert (occ >= 0);
    if (!(w0 = *c)) continue;
    if (!(w1 = *++c)) continue;
    LOGLITS (c - 1, "watching %d and %d in", w0, w1);
    PUSH (watches[w0], occ);
    PUSH (watches[w1], occ);
    while (*++c)
      ;
  }

  for (next = 0; !yals->mt && next < COUNT (yals->trail); next++) {
    lit = PEEK (yals->trail, next);
    LOG ("propagating %d", lit);
    if (vals[lit] > 0) continue;
    if (vals[lit] < 0) {
      LOG ("inconsistent units %d and %d", -lit, lit);
      yals->mt = 1;
    } else {
      assert (!vals[lit]);
      vals[lit] = 1;
      assert (!vals[-lit]);
      vals[-lit] = -1;
      s = watches - lit;
      for (p = s->start; !yals->mt && p < s->top; p++) {
	occ = *p;
	assert (occ >= 0);
	c = yals->cdb.start + occ;
	assert (c < yals->cdb.top);
	if (c[1] != -lit) SWAP (int, c[0], c[1]);
	assert (c[1] == -lit); 
	if (vals[w0 = c[0]] > 0) continue;
	for (q = c + 2; (other = *q); q++)
	  if (vals[other] >= 0) break;
	if (other) {
	  SWAP (int, c[1], *q);
	  LOGLITS (c, "watching %d instead of %d in", other, lit);
	  PUSH (watches[other], occ);
	} else if (!vals[w0]) {
	  LOGLITS (c, "found new unit %d in", w0);
	  PUSH (yals->trail, w0);
	} else {
	  assert (vals[w0] < 0);
	  LOGLITS (c, "found inconsistent");
	  yals->mt = 1;
	}
      }
      RELEASE (*s);
    }
  }
  FIT (yals->trail);
  yals_msg (yals, 1,
    "found %d units during unit propagation",
    COUNT (yals->trail));

  for (lit = -nvars; lit < nvars; lit++) RELEASE (watches[lit]);
  watches -= nvars;
  DELN (watches, 2*nvars);
  oldnlits = COUNT (yals->cdb);

  if (yals->mt) goto DONE;

  nstr = nsat = 0;
  q = yals->cdb.start;
  for (c = q; c < yals->cdb.top; c++) {
    satisfied = 0;
    for (p = c; (lit = *p); p++)
      if (vals[lit] > 0) satisfied = 1;
    if (!satisfied) {
      for (p = c; (lit = *p); p++) {
	if (vals[lit] < 0) continue;
	assert (!vals[lit]);
	*q++ = lit;
	nstr++;
      }
      assert (q >= yals->cdb.start + 2);
      assert (q[-1]); assert (q[-2]);
      *q++ = 0;
    } else nsat++;
    c = p;
  }
  yals->cdb.top = q;
  FIT (yals->cdb);
  newnlits = COUNT (yals->cdb);
  assert (newnlits <= oldnlits);
  yals_msg (yals, 1, "removed %d satisfied clauses", nsat);
  yals_msg (yals, 1, "removed %d falsified literal occurrences", nstr);
  yals_msg (yals, 1,
    "literal stack reduced by %d to %d from% d",
    oldnlits - newnlits, newnlits, oldnlits);

DONE:

  vals -= nvars;
  DELN (vals, 2*nvars);
}

/*------------------------------------------------------------------------*/

static Word yals_primes[] = {
  2000000011u, 2000000033u, 2000000063u, 2000000087u, 2000000089u,
  2000000099u, 2000000137u, 2000000141u, 2000000143u, 2000000153u,
  2000000203u, 2000000227u, 2000000239u, 2000000243u, 2000000269u,
  2000000273u, 2000000279u, 2000000293u, 2000000323u, 2000000333u,
  2000000357u, 2000000381u, 2000000393u, 2000000407u, 2000000413u,
  2000000441u, 2000000503u, 2000000507u, 2000000531u, 2000000533u,
  2000000579u, 2000000603u, 2000000609u, 2000000621u, 2000000641u,
  2000000659u, 2000000671u, 2000000693u, 2000000707u, 2000000731u,
};

#define NPRIMES (sizeof(yals_primes)/sizeof(unsigned))

static Word yals_sig (Yals * yals) {
  unsigned i = 0, j;
  Word res = 0;
  for (j = 0; j < yals->nvarwords; j++) {
    res += yals_primes[i++] * yals->vals[j];
    if (i == NPRIMES) i = 0;
  }
  return res;
}

static unsigned yals_gcd (unsigned a, unsigned b) {
  while (b) {
    unsigned r = a % b;
    a = b, b = r;
  }
  return a;
}

static void yals_cache_assignment (Yals * yals) {
  int min, other_min, cachemax, cachemin, cachemincount, cachemaxcount;
  unsigned start, delta, i, j, ncache, rpos;
  Word sig, other_sig, * other_vals;
  size_t bytes;
#ifndef NDEBUG
  int count;
#endif

  if (!yals->opts.cachemin.val) return;
  sig = yals_sig (yals);
  ncache = COUNT (yals->cache);
  for (i = 0; i < ncache; i++) {
    other_sig = PEEK (yals->sigs, i);
    yals->stats.sig.search++;
    if (other_sig != sig) { yals->stats.sig.neg++; continue; }
    other_vals = PEEK (yals->cache, i);
    for (j = 0; j < yals->nvarwords; j++)
      if (other_vals[j] != yals->tmp[j]) break;
    if (j == yals->nvarwords) {
      yals_msg (yals, 2, "current assignment already in cache");
      yals->stats.cache.skipped++;
      yals->stats.sig.truepos++;
      return;
    }
    yals->stats.sig.falsepos++;
  }

  cachemin = INT_MAX, cachemax = -1;
  cachemaxcount = cachemincount = 0;
  for (j = 0; j < ncache; j++) {
    other_min = PEEK (yals->mins, j);
    if (other_min < cachemin) cachemin = other_min, cachemincount = 0;
    if (other_min > cachemax) cachemax = other_min, cachemaxcount = 0;
    if (other_min == cachemin) cachemincount++;
    if (other_min == cachemax) cachemaxcount++;
  }
  yals_msg (yals, 2,
    "cache of size %d minimum %d (%d = %.0f%%) maximum %d (%d = %.0f%%)",
    ncache,
    cachemin, cachemincount, yals_pct (cachemincount, ncache),
    cachemax, cachemaxcount, yals_pct (cachemaxcount, ncache));

  min = yals->stats.tmp;
  assert (min <= yals_nunsat (yals));
  bytes = yals->nvarwords * sizeof (Word);
  if (!yals->cachesizetarget) {
    yals->cachesizetarget = yals->opts.cachemin.val;
    assert (yals->cachesizetarget);
    yals_msg (yals, 2,
      "initial cache size target of %d",
      yals->cachesizetarget);
  }

  if (ncache < yals->cachesizetarget) {
PUSH_ASSIGNMENT:
    yals_msg (yals, 2,
      "pushing current assignment with minimum %d in cache as assignment %d",
      min, ncache);
    NEWN (other_vals, yals->nvarwords);
    memcpy (other_vals, yals->tmp, bytes);
    PUSH (yals->cache, other_vals);
    PUSH (yals->sigs, sig);
    PUSH (yals->mins, min);
    yals->stats.cache.inserted++;
  } else {
    assert (ncache == yals->cachesizetarget);
    if (ncache == 1) start = delta = 0;
    else {
      start = yals_rand_mod (yals, ncache);
      delta = yals_rand_mod (yals, ncache - 1) + 1;
      while (yals_gcd (ncache, delta) != 1)
	delta--;
    }
    rpos = ncache;
    j = start;
#ifndef NDEBUG
    count = 0;
#endif
    do {
      other_min = PEEK (yals->mins, j);
      assert (other_min >= cachemin);
      assert (other_min <= cachemax);
      if (other_min == cachemax && other_min > min) rpos = j;
      j += delta;
      if (j >= ncache) j -= ncache, assert (j < ncache);
#ifndef NDEBUG
      count++;
#endif
    } while (j != start);
    assert (count == ncache);

    if (rpos < ncache) {
      assert (min < cachemax);
      assert (PEEK (yals->mins, rpos) == cachemax);
      yals_msg (yals, 2,
	"replacing cached %d (minimum %d) better minimum %d",
	rpos, cachemax, min);
      other_vals = PEEK (yals->cache, rpos);
      memcpy (other_vals, yals->tmp, bytes);
      POKE (yals->mins, rpos, min);
      POKE (yals->sigs, rpos, sig);
      yals->stats.cache.replaced++;
    } else if (min > cachemax ||
               (cachemin < cachemax && min == cachemax)) {
DO_NOT_CACHE_ASSSIGNEMNT:
      yals_msg (yals, 2,
	"local minimum %d not cached needs %d",
	min, cachemax-1);
      yals->stats.cache.skipped++;
    } else {
      assert (min == cachemin);
      assert (min == cachemax);
      assert (min == yals->stats.best);
      if (yals->cachesizetarget < yals->opts.cachemax.val) {
	yals->cachesizetarget *= 2;
	if (yals->cachesizetarget < ncache) {
	  yals->cachesizetarget = ncache;
	  goto DO_NOT_CACHE_ASSSIGNEMNT;
	}
	yals_msg (yals, 2,
	  "new cache size target of %d",
	  yals->cachesizetarget);
	goto PUSH_ASSIGNMENT;
      } else goto DO_NOT_CACHE_ASSSIGNEMNT;
    }
  }
}

static void yals_remove_trailing_bits (Yals * yals) {
  unsigned i;
  Word mask;
  if (!yals->nvarwords) return;
  i = yals->nvars & BITMAPMASK;
  mask = ((Word)1) << (i + 1);
  yals->vals[yals->nvarwords-1] &= mask - 1;
}

static void yals_set_units (Yals * yals) {
  const Word * eow, * c, * s;
  Word * p = yals->vals, tmp;
  eow = p + yals->nvarwords;
  c = yals->clear;
  s = yals->set;
  while (p < eow) {
    tmp = *p;
    tmp &= *c++;
    tmp |= *s++;
    *p++ = tmp;
  }
  assert (s == yals->set + yals->nvarwords);
  assert (c == yals->clear + yals->nvarwords);
}

static void yals_setphases (Yals * yals) {
  int i, idx, lit;
  yals_msg (yals, 1,
    "forcing %d initial phases", (int) COUNT (yals->phases));
  for (i = 0; i < COUNT (yals->phases); i++) {
    lit = PEEK (yals->phases, i);
    assert (lit);
    idx = ABS (lit);
    if (idx >= yals->nvars) {
      LOG ("skipping setting phase of %d", lit);
      continue;
    }
    LOG ("setting phase of %d", lit);
    if (lit > 0) SETBIT (yals->vals, yals->nvarwords, idx);
    else CLRBIT (yals->vals, yals->nvarwords, idx);
  }
  RELEASE (yals->phases);
}

static void yals_pick_assignment (Yals * yals, int initial) {
  int idx, pos, neg, i, nvars = yals->nvars, ncache;
  size_t bytes = yals->nvarwords * sizeof (Word);
  const int vl = 1 + !initial;
  if (!initial && yals->opts.best.val) {
    yals->stats.pick.best++;
    yals_msg (yals, vl, "picking previous best assignment");
    memcpy (yals->vals, yals->best, bytes);
  } else if (!initial && yals->opts.keep.val) {
    yals->stats.pick.keep++;
    yals_msg (yals, vl, "picking current assignment (actually keeping it)");
  } else if (!initial && 
             yals->strat.cached &&
	     (ncache = COUNT (yals->cache)) > 0) {
    if (!yals->opts.cacheduni.val)  {
      assert (EMPTY (yals->cands));
      assert (EMPTY (yals->scores));
      for (i = 0; i < ncache; i++) {
	int min = PEEK (yals->mins, i);
	assert (min >= 0);
	PUSH (yals->cands, i);
	PUSH (yals->scores, min);
      }
      pos = yals_pick_by_score (yals);
      CLEAR (yals->scores);
      CLEAR (yals->cands);
    } else pos = yals_rand_mod (yals, ncache);
    yals->stats.pick.cached++;
    yals_msg (yals, vl,
      "picking cached assignment %d with minimum %d",
      pos, PEEK (yals->mins, pos));
    memcpy (yals->vals, PEEK (yals->cache, pos), bytes);
  } else if (yals->strat.pol < 0) {
    yals->stats.pick.neg++;
    yals_msg (yals, vl, "picking all negative assignment");
    memset (yals->vals, 0, bytes);
  } else if (yals->strat.pol > 0) {
    yals->stats.pick.pos++;
    yals_msg (yals, vl, "picking all positive assignment");
    memset (yals->vals, 0xff, bytes);
  } else {
    yals->stats.pick.rnd++;
    yals_msg (yals, vl, "picking new random assignment");
    for (i = 0; i < yals->nvarwords; i++)
      yals->vals[i] = yals_rand (yals);
  }
  yals_remove_trailing_bits (yals);
  if (initial) yals_setphases (yals);
  yals_set_units (yals);
  if (yals->opts.verbose.val <= 2) return;
  pos = neg = 0;
  for (idx = 1; idx < nvars; idx++)
    if (GETBIT (yals->vals, yals->nvarwords, idx)) pos++;
  neg = nvars - pos;
  yals_msg (yals, vl,
    "new full assignment %d positive %d negative", pos, neg);
}

static void yals_log_assignment (Yals * yals) {
#ifndef NDEBUG
  int idx;
  if (!yals->opts.logging.val) return;
  for (idx = 1; idx < yals->nvars; idx++) {
    int lit = yals_val (yals, idx) ? idx : -idx;
    LOG ("assigned %d", lit);
  }
#endif
}

static unsigned yals_len_to_weight (Yals * yals, int len) {
  const int uni = yals->strat.uni;
  const int weight = yals->strat.weight;
  unsigned w;

  if (uni > 0) w = weight;
  else if (uni < 0) w = MIN (len, weight);
  else w = MAX (weight - len, 1);

  return w;
}

static void yals_update_sat_and_unsat (Yals * yals) {
  int lit, cidx, len, cappedlen, crit;
  const int * lits, * p;
  unsigned satcnt;
  yals_log_assignment (yals);
  yals_reset_unsat (yals);
  for (len = 1; len <= MAXLEN; len++)
    yals->weights[len] = yals_len_to_weight (yals, len);
  if (yals->crit)
    memset (yals->weightedbreak, 0, 2*yals->nvars*sizeof(int));
  for (cidx = 0; cidx < yals->nclauses; cidx++) {
    satcnt = 0;
    lits = yals_lits (yals, cidx);
    crit = 0;
    for (p = lits; (lit = *p); p++) {
      if (!yals_val (yals, lit)) continue;
      crit ^= lit;
      satcnt++;
    }
    if (yals->crit) yals->crit[cidx] = crit;
    len = p - lits;
    cappedlen = MIN (len, MAXLEN);
    LOGCIDX (cidx,
       "sat count %u length %d weight %u for",
       satcnt, len, yals->weights[cappedlen]);
    yals_setsatcnt (yals, cidx, satcnt);
    if (!satcnt) {
      yals_enqueue (yals, cidx);
      LOGCIDX (cidx, "broken");
    } else if (yals->crit && satcnt == 1)
      yals_inc_weighted_break (yals, yals->crit[cidx], cappedlen);
  }
  yals_check_global_invariant (yals);
}

static void yals_init_weight_to_score_table (Yals * yals) {
  double cb, invcb, score, eps;
  const double start = 1e150;
  int maxlen = yals->maxlen;
  unsigned i;

  // probSAT SC'13 values:

       if (maxlen <= 3) cb = 2.5;	// from Adrian's thesis ...
  else if (maxlen <= 4) cb = 2.85;
  else if (maxlen <= 5) cb = 3.7;
  else if (maxlen <= 6) cb = 5.1;
  else                  cb = 5.4;

  yals_msg (yals, 1,
    "exponential base cb = %f for maxlen %d",
    cb, maxlen);

  eps = 0;
  score = start;
  for (i = 0; score; i++) {
    assert (i < 100000);
    LOG ("exp2(-%d) = %g", i, score);
    PUSH (yals->exp.table.two, score);
    eps = score;
    score *= .5;
  }
  assert (eps > 0);
  assert (i == COUNT (yals->exp.table.two));
  yals->exp.max.two = i;
  yals->exp.eps.two = eps;
  yals_msg (yals, 1, "exp2(<= %d) = %g", -i, eps);

  invcb = 1.0 / cb;
  assert (invcb < 1.0);
  score = start;
  eps = 0.0;
  for (i = 0; score; i++) {
    assert (i < 1000000);
    LOG ("pow(%f,-%d) = %g", cb, i, score);
    PUSH (yals->exp.table.cb, score);
    eps = score;
    score *= invcb;
  }
  assert (eps > 0);
  assert (i == COUNT (yals->exp.table.cb));
  yals->exp.max.cb = i;
  yals->exp.eps.cb = eps;
  yals_msg (yals, 1, "pow(%f,(<= %d)) = %g", cb, -i, eps);
}

/*------------------------------------------------------------------------*/

static void yals_connect (Yals * yals) {
  int idx, n, lit, nvars = yals->nvars, * count, cidx, sign;
  long long sumoccs, sumlen; int minoccs, maxoccs, minlen, maxlen;
  int * occsptr, occs, len, lits, maxidx, nused, uniform;
  int nclauses, nbin, ntrn, nquad, nlarge;
  const int * p,  * q;

  FIT (yals->cdb);
  RELEASE (yals->mark);
  RELEASE (yals->clause);

  maxlen = 0;
  sumlen = 0;
  minlen = INT_MAX;
  nclauses = nbin = ntrn = nquad = nlarge = 0;

  for (p = yals->cdb.start; p < yals->cdb.top; p = q + 1) {
    for (q = p; *q; q++)
      ;
    len = q - p;

         if (len == 2) nbin++;
    else if (len == 3) ntrn++;
    else if (len == 4) nquad++;
    else               nlarge++;

    nclauses++;

    sumlen += len;
    if (len > maxlen) maxlen = len;
    if (len < minlen) minlen = len;
  }

  yals_msg (yals, 1,
    "found %d binary, %d ternary and %d large clauses",
    nbin, ntrn, nclauses - nbin - ntrn);

  yals_msg (yals, 1,
    "size of literal stack %d (%d for large clauses only)",
    (int) COUNT (yals->cdb),
    ((int) COUNT (yals->cdb)) - 3*nbin - 4*ntrn);

  yals->maxlen = maxlen;
  yals->minlen = minlen;
#ifndef NYALSTATS
  yals->stats.nincdec = MAX (maxlen + 1, 3);
  NEWN (yals->stats.inc, yals->stats.nincdec);
  NEWN (yals->stats.dec, yals->stats.nincdec);
#endif

  if ((INT_MAX >> LENSHIFT) < nclauses)
    yals_abort (yals,
      "maximum number of clauses %d exceeded",
      (INT_MAX >> LENSHIFT));

  yals->nclauses = nclauses;
  yals->nbin = nbin;
  yals->ntrn = ntrn;
  yals_msg (yals, 1, "connecting %d clauses", nclauses);
  NEWN (yals->lits, nclauses);

  lits = 0;
  for (cidx = 0; cidx < nclauses; cidx++) {
    yals->lits[cidx] = lits;
    while (PEEK (yals->cdb, lits)) lits++;
    lits++;
  }
  assert (lits == COUNT (yals->cdb));

  NEWN (yals->weights, MAXLEN + 1);

  NEWN (count, 2*nvars);
  count += nvars;

  maxidx = maxoccs = -1;
  minoccs = INT_MAX;
  sumoccs = 0;

  for (cidx = 0; cidx < yals->nclauses; cidx++) {
    for (p = yals_lits (yals, cidx); (lit = *p); p++) {
      idx = ABS (lit);
      if (idx > maxidx) maxidx = idx;
      count[lit]++;
    }
  }

  occs = 0;
  nused = 0;
  for (lit = 1; lit < nvars; lit++) {
    int pos = count[lit], neg = count[-lit], sum = pos + neg;
    occs += sum + 2;
    if (sum) nused++;
  }

  assert (nused <= nvars);
  if (nused == nvars)
    yals_msg (yals, 1, "all variables occur");
  else
    yals_msg (yals, 1,
      "%d variables used out of %d, %d do not occur %.0f%%",
      nused, nvars, nvars - nused, yals_pct (nvars-nused, nvars));

  yals->noccs = occs;
  LOG ("size of occurrences stack %d", occs);
  NEWN (yals->occs, yals->noccs);

  NEWN (yals->refs, 2*nvars);

  occs = 0;
  for (lit = 1; lit < nvars; lit++) {
    n = count[lit];
    LOG ("literal %d occurs %d times", lit, n);
    *yals_refs (yals, lit) = occs;
    occs += n, yals->occs[occs++] = -1;
    n = count[-lit];
    LOG ("literal %d occurs %d times", -lit, n);
    *yals_refs (yals, -lit) = occs;
    occs += n, yals->occs[occs++] = -1;
  }
  assert (occs == yals->noccs);

  yals->avglen = yals_avg (sumlen, yals->nclauses);

  if (minlen == maxlen) {
    yals_msg (yals, 1,
      "all %d clauses are of uniform length %d",
      yals->nclauses, maxlen);
  } else if (maxlen >= 0) {
    yals_msg (yals, 1,
      "average clause length %.2f (min %d, max %d)",
      yals->avglen, minlen, maxlen);
    yals_msg (yals, 2,
      "%d binary %.0f%%, %d ternary %.0f%% ",
      nbin, yals_pct (nbin, yals->nclauses),
      ntrn, yals_pct (ntrn, yals->nclauses));
    yals_msg (yals, 2,
      "%d quaterny %.0f%%, %d large clauses %.0f%%",
      nquad, yals_pct (nquad, yals->nclauses),
      nlarge, yals_pct (nlarge, yals->nclauses));
  }

  if (minlen == maxlen && !yals->opts.toggleuniform.val) uniform = 1;
  else if (minlen != maxlen && yals->opts.toggleuniform.val) uniform = 1;
  else uniform = 0;

  if (uniform) {
    yals_msg (yals, 1,
      "using uniform strategy for clauses of length %d", maxlen);
    yals->uniform = maxlen;
  } else {
    yals_msg (yals, 1, "using standard non-uniform strategy");
    yals->uniform = 0;
  }

  yals_msg (yals, 1,
    "clause variable ratio %.3f = %d / %d",
    yals_avg (nclauses, nused), nclauses, nused);

  for (idx = 1; idx < nvars; idx++)
    for (sign = 1; sign >= -1; sign -= 2) {
      lit = sign * idx;
      int occs = count[lit] + count[-lit];
      if (!occs) continue;
      sumoccs += occs;
      if (occs > maxoccs) maxoccs = occs;
      if (occs < minoccs) minoccs = occs;
    }

  for (cidx = 0; cidx < yals->nclauses; cidx++) {
    p = yals_lits (yals, cidx);
    len = 0;
    while (len < MAXLEN && p[len]) len++;
    while ((lit = *p++)) {
      occsptr = yals_refs (yals, lit);
      occs = *occsptr;
      assert_valid_occs (occs);
      assert (!yals->occs[occs]);
      yals->occs[occs] = (cidx << LENSHIFT) | len;
      *occsptr = occs + 1;
    }
  }

  for (idx = 1; idx < nvars; idx++) {
    for (sign = 1; sign >= -1; sign -= 2) {
      lit = sign * idx;
      occsptr = yals_refs (yals, lit);
      occs = *occsptr;
      assert_valid_occs (occs);
      assert (yals->occs[occs] == -1);
      n = count[lit];
      assert (occs >= n);
      *occsptr = occs - n;
    }
  }

  count -= nvars;
  DELN (count, 2*nvars);

  yals_msg (yals, 1,
    "average literal occurrence %.2f (min %d, max %d)",
    yals_avg (sumoccs, yals->nvars)/2.0, minoccs, maxoccs);

  if (yals->uniform) yals->pick = yals->opts.unipick.val;
  else yals->pick = yals->opts.pick.val;

  yals_msg (yals, 1, "picking %s", yals_pick_to_str (yals));

  yals->unsat.usequeue = (yals->pick > 0);

  yals_msg (yals, 1,
    "using %s for unsat clauses",
    yals->unsat.usequeue ? "queue" : "stack");

  if (yals->unsat.usequeue) NEWN (yals->lnk, nclauses);
  else {
    NEWN (yals->pos, nclauses);
    for (cidx = 0; cidx < nclauses; cidx++) yals->pos[cidx] = -1;
  }

  yals->nvarwords = (nvars + BITS_PER_WORD - 1) / BITS_PER_WORD;

  yals_msg (yals, 1, "%d x %d-bit words per assignment (%d bytes = %d KB)",
    yals->nvarwords,
    BITS_PER_WORD,
    yals->nvarwords * sizeof (Word),
    (yals->nvarwords * sizeof (Word) >> 10));

  NEWN (yals->set, yals->nvarwords);
  NEWN (yals->clear, yals->nvarwords);
  memset (yals->clear, 0xff, yals->nvarwords * sizeof (Word));
  while (!EMPTY (yals->trail)) {
    lit = POP (yals->trail);
    idx = ABS (lit);
    if (lit < 0) CLRBIT (yals->clear, yals->nvarwords, idx);
    else SETBIT (yals->set, yals->nvarwords, idx);
  }
  RELEASE (yals->trail);

  NEWN (yals->vals, yals->nvarwords);
  NEWN (yals->best, yals->nvarwords);
  NEWN (yals->tmp, yals->nvarwords);
  NEWN (yals->flips, nvars);

  if (maxlen < (1<<8)) {
    yals->satcntbytes = 1;
    NEWN (yals->satcnt1, yals->nclauses);
  } else if (maxlen < (1<<16)) {
    yals->satcntbytes = 2;
    NEWN (yals->satcnt2, yals->nclauses);
  } else {
    yals->satcntbytes = 4;
    NEWN (yals->satcnt4, yals->nclauses);
  }
  yals_msg (yals, 1,
    "need %d bytes per clause for counting satisfied literals",
    yals->satcntbytes);

  if (yals->opts.crit.val) {
    yals_msg (yals, 1,
      "dynamically computing break values on-the-fly "
      "using critical literals");
    NEWN (yals->crit, nclauses);
    NEWN (yals->weightedbreak, 2*nvars);
  } else
    yals_msg (yals, 1, "eagerly computing break values");

  yals_init_weight_to_score_table (yals);
}

/*------------------------------------------------------------------------*/

#define SETOPT(NAME,DEFAULT,MIN,MAX,DESCRIPTION) \
do { \
  int OLD; \
  if (strcmp (name, #NAME)) break; \
  if(val < (MIN)) { \
    yals_warn (yals, \
      "can not set option '%s' smaller than %d", name, (MIN)); \
    val = (MIN); \
  } \
  if(val > (MAX)) { \
    yals_warn (yals, \
      "can not set option '%s' larger than %d", name, (MAX)); \
    val = (MAX); \
  } \
  OLD = yals->opts.NAME.val; \
  if (OLD == val) \
    yals_msg (yals, 2, \
      "keeping option '%s' add old value %d", name, val); \
  else { \
    yals_msg (yals, 1, \
      "setting option '%s' to %d (previous value %d)", name, val, OLD); \
    yals->opts.NAME.val = val; \
  } \
  return 1; \
} while (0)

#undef OPT
#define OPT SETOPT

int yals_setopt (Yals * yals, const char * name, int val) {
  OPTSTEMPLATE
  return 0;
}

/*------------------------------------------------------------------------*/

#define GETOPT(NAME,DEFAULT,MIN,MAX,DESCRIPTION) \
do { \
  if (!strcmp (name, #NAME)) return yals->opts.NAME.val; \
} while (0)

#undef OPT
#define OPT GETOPT

int yals_getopt (Yals * yals, const char * name) {
  OPTSTEMPLATE
  return 0;
}

/*------------------------------------------------------------------------*/

#define USGOPT(NAME,DEFAULT,MIN,MAX,DESCRIPTION) \
do { \
  char BUFFER[120]; int I; \
  sprintf (BUFFER, "--%s=%d..%d", #NAME, (MIN), (MAX)); \
  fputs (BUFFER, yals->out); \
  for (I = 28 - strlen (BUFFER); I > 0; I--) fputc (' ', yals->out); \
  fprintf (yals->out, "%s [%d]\n", (DESCRIPTION), (int)(DEFAULT)); \
} while (0)

#undef OPT
#define OPT USGOPT

void yals_usage (Yals * yals) {
  yals_msglock (yals);
  OPTSTEMPLATE
  yals_msgunlock (yals);
}

/*------------------------------------------------------------------------*/

#define SHOWOPT(NAME,DEFAULT,MIN,MAX,DESCRIPTION) \
do { \
  yals_msg (yals, 0, "--%s=%d", #NAME, yals->opts.NAME); \
} while (0)

#undef OPT
#define OPT SHOWOPT

void yals_showopts (Yals * yals) { OPTSTEMPLATE }

/*------------------------------------------------------------------------*/

static void yals_envopt (Yals * yals, const char * name, Opt * opt) {
  int len = strlen (name) + strlen ("YALS") + 1, val, ch;
  char * env = yals_malloc (yals, len), * p;
  const char * str;
  sprintf (env, "yals%s",name);
  for (p = env; (ch = *p); p++) *p = toupper (ch);
  if ((str = getenv (env))) {
    val = atoi (str);
    val = MIN (opt->min, val);
    val = MAX (opt->max, val);
    opt->val = val;
  }
  yals_free (yals, env, len);
}

#define INITOPT(NAME,DEFAULT,MIN,MAX,DESCRIPTION) \
do { \
  assert ((MIN) <= (DEFAULT)); \
  assert ((DEFAULT) <= (MAX)); \
  yals->opts.NAME.def = (DEFAULT); \
  yals->opts.NAME.val = (DEFAULT); \
  yals->opts.NAME.min = (MIN); \
  yals->opts.NAME.max = (MAX); \
  yals_envopt (yals, #NAME, &yals->opts.NAME); \
} while (0)

#undef OPT
#define OPT INITOPT

static void yals_initopts (Yals * yals) { OPTSTEMPLATE }

/*------------------------------------------------------------------------*/

static void * yals_default_malloc (void * state, size_t bytes) {
  (void) state;
  return malloc (bytes);
}

static void * yals_default_realloc (void * state, void * ptr,
                                    size_t old_bytes, size_t new_bytes) {
  (void) state;
  (void) old_bytes;
  return realloc (ptr, new_bytes);
}

static void yals_default_free (void * state, void * ptr, size_t bytes) {
  (void) state;
  (void) bytes;
  return free (ptr);
}

/*------------------------------------------------------------------------*/

Yals * yals_new_with_mem_mgr (void * mgr,
                              YalsMalloc m, YalsRealloc r, YalsFree f) {
  Yals * yals;
  assert (m), assert (r), assert (f);
  yals = m (mgr, sizeof *yals);
  if (!yals) yals_abort (0, "out-of-memory allocating manager in 'yals_new'");
  memset (yals, 0, sizeof *yals);
  yals->out = stdout;
  yals->mem.mgr = mgr;
  yals->mem.malloc = m;
  yals->mem.realloc = r;
  yals->mem.free = f;
  yals->stats.tmp = INT_MAX;
  yals->stats.best = INT_MAX;
  yals->stats.last = INT_MAX;
  yals->limits.report.min = INT_MAX;
  yals_inc_allocated (yals, sizeof *yals);
  yals_srand (yals, 0);
  yals_initopts (yals);
  yals->opts.prefix = yals_strdup (yals, YALS_DEFAULT_PREFIX);
  yals->limits.flips = -1;
#ifndef NYALSMEMS
  yals->limits.mems = -1;
#endif
#ifndef NYALSTATS
  yals->stats.wb.min = UINT_MAX;
  yals->stats.wb.max = 0;
#endif
#if 0
  if (getenv ("YALSAMPLES") && getenv ("YALSMOD")) {
    int s = atoi (getenv ("YALSAMPLES")), i;
    int m = atoi (getenv ("YALSMOD"));
    double start = yals_time (yals), delta;
    int64_t * count;
    if (m <= 0) m = 1;
    yals_msg (yals, 0, "starting to sample %d times RNG mod %d", s, m);
    NEWN (count, m);
    for (i = 0; i < s; i++) {
      int r = yals_rand_mod (yals, m);
      assert (0 <= r), assert (r < m);
      count[r]++;
    }
    for (i = 0; i < m; i++)
      yals_msg (yals, 0,
        " mod %6d hit %10d times %7.2f%% with error %7.2f%%",
	i, count[i], yals_pct (count[i], s),
        yals_pct (count[i] - s/(double)m, s/(double) m));
    DELN (count, m);
    delta = yals_time (yals) - start;
    yals_msg (yals, 0,
      "finished sampling RNG in %.3f seconds, %.2f ns per 'rand'",
      delta, yals_avg (delta*1e9, s));
  }
#endif
  return yals;
}

Yals * yals_new () {
  return yals_new_with_mem_mgr (0,
	   yals_default_malloc, yals_default_realloc, yals_default_free);
}

static void yals_reset_cache (Yals * yals) {
  int ncache = COUNT (yals->cache);
  Word ** w;
  for (w = yals->cache.start; w < yals->cache.top; w++)
    DELN (*w, yals->nvarwords);
  RELEASE (yals->cache);
  yals->cachesizetarget = 0;
  yals_msg (yals, 1, "reset %d cache lines", ncache);
}

void yals_del (Yals * yals) {
  yals_reset_cache (yals);
  yals_reset_unsat (yals);
  RELEASE (yals->cdb);
  RELEASE (yals->clause);
  RELEASE (yals->mark);
  RELEASE (yals->mins);
  RELEASE (yals->sigs);
  RELEASE (yals->breaks);
  RELEASE (yals->scores);
  RELEASE (yals->cands);
  RELEASE (yals->trail);
  RELEASE (yals->phases);
  RELEASE (yals->exp.table.two);
  RELEASE (yals->exp.table.cb);
  RELEASE (yals->minlits);
  if (yals->unsat.usequeue) DELN (yals->lnk, yals->nclauses);
  else DELN (yals->pos, yals->nclauses);
  DELN (yals->lits, yals->nclauses);
  if (yals->crit) DELN (yals->crit, yals->nclauses);
  if (yals->weightedbreak) DELN (yals->weightedbreak, 2*yals->nvars);
  if (yals->satcntbytes == 1) DELN (yals->satcnt1, yals->nclauses);
  else if (yals->satcntbytes == 2) DELN (yals->satcnt2, yals->nclauses);
  else DELN (yals->satcnt4, yals->nclauses);
  if (yals->weights) DELN (yals->weights, MAXLEN + 1);
  DELN (yals->vals, yals->nvarwords);
  DELN (yals->best, yals->nvarwords);
  DELN (yals->tmp, yals->nvarwords);
  DELN (yals->clear, yals->nvarwords);
  DELN (yals->set, yals->nvarwords);
  DELN (yals->occs, yals->noccs);
  if (yals->refs) DELN (yals->refs, 2*yals->nvars);
  if (yals->flips) DELN (yals->flips, yals->nvars);
#ifndef NYALSTATS
  DELN (yals->stats.inc, yals->stats.nincdec);
  DELN (yals->stats.dec, yals->stats.nincdec);
#endif
  yals_strdel (yals, yals->opts.prefix);
  yals_dec_allocated (yals, sizeof *yals);
  assert (getenv ("YALSLEAK") || !yals->stats.allocated.current);
  yals->mem.free (yals->mem.mgr, yals, sizeof *yals);
}

void yals_setprefix (Yals * yals, const char * prefix) {
  yals_strdel (yals, yals->opts.prefix);
  yals->opts.prefix = yals_strdup (yals, prefix ? prefix : "");
}

void yals_setout (Yals * yals, FILE * out) {
  yals->out = out;
}

void yals_setphase (Yals * yals, int lit) {
  if (!lit) yals_abort (yals, "zero literal argument to 'yals_val'");
  PUSH (yals->phases, lit);
}

void yals_setflipslimit (Yals * yals, long long flips) {
  yals->limits.flips = flips;
  yals_msg (yals, 1, "new flips limit %lld", (long long) flips);
}

void yals_setmemslimit (Yals * yals, long long mems) {
#ifndef NYALSMEMS
  yals->limits.mems = mems;
  yals_msg (yals, 1, "new mems limit %lld", (long long) mems);
#else
  yals_warn (yals,
    "can not set mems limit to %lld "
    "(compiled without 'mems' support)",
    (long long) mems);
#endif
}

/*------------------------------------------------------------------------*/

void yals_setime (Yals * yals, double (*time)(void)) {
  yals->cbs.time = time;
}

void yals_seterm (Yals * yals, int (*term)(void *), void * state) {
  yals->cbs.term.state = state;
  yals->cbs.term.fun = term;
}

void yals_setmsglock (Yals * yals,
                      void (*lock)(void *),
                      void (*unlock)(void *),
		      void * state) {
  yals->cbs.msg.state = state;
  yals->cbs.msg.lock = lock;
  yals->cbs.msg.unlock = unlock;
}

/*------------------------------------------------------------------------*/

static void yals_new_clause (Yals * yals) {
  int len = COUNT (yals->clause), * p, lit;
  if (!len) {
    LOG ("found empty clause in original formula");
    yals->mt = 1;
  }
  if (len == 1) {
    lit = PEEK (yals->clause, 0);
    LOG ("found unit clause %d in original formula", lit);
    PUSH (yals->trail, lit);
  }
  for (p = yals->clause.start; p < yals->clause.top; p++) {
    lit = *p;
    PUSH (yals->cdb, lit);
  }
  PUSH (yals->cdb, 0);
  LOGLITS (yals->cdb.top - len - 1, "new length %d", len);
}

static signed char yals_sign (int lit) { return (lit < 0) ? -1 : 1; }

void yals_add (Yals * yals, int lit) {
  if (lit) {
    signed char mark;
    int idx;
    if (lit == INT_MIN)
      yals_abort (yals, "can not add 'INT_MIN' as literal");
    idx = ABS (lit);
    if (idx == INT_MAX)
      yals_abort (yals, "can not add 'INT_MAX' as literal");
    if (idx >= yals->nvars) yals->nvars = idx + 1;
    while (idx >= COUNT (yals->mark)) PUSH (yals->mark, 0);
    mark = PEEK (yals->mark, idx);
    if (lit < 0) mark = -mark;
    if (mark < 0) yals->trivial = 1;
    else if (!mark) {
      PUSH (yals->clause, lit);
      POKE (yals->mark, idx, yals_sign (lit));
    }
  } else {
    const int * p;
    for (p = yals->clause.start; p < yals->clause.top; p++)
      POKE (yals->mark, ABS (*p), 0);
    if (yals->trivial) yals->trivial = 0;
    else yals_new_clause (yals);
    CLEAR (yals->clause);
  } 
}

/*------------------------------------------------------------------------*/

#define ISDEFSTRAT(NAME,ENABLED) \
do { \
  if (!(ENABLED)) { \
    assert (yals->strat.NAME == yals->opts.NAME.val); \
    break; \
  } \
  if (yals->strat.NAME != yals->opts.NAME.val) return 0; \
} while (0)
#undef STRAT
#define STRAT ISDEFSTRAT

static int yals_is_default_strategy (Yals * yals) {
  STRATSTEMPLATE
  return 1;
}

#define PRINTSTRAT(NAME,ENABLED) \
do { \
  if (!(ENABLED)) break; \
  fprintf (yals->out, " --%s=%d", #NAME, yals->strat.NAME); \
} while (0)
#undef STRAT
#define STRAT PRINTSTRAT

static void yals_print_strategy (Yals * yals, const char * type, int vl) {
  if (yals->opts.verbose.val < vl) return;
  yals_msglock (yals);
  fprintf (yals->out, "%s%s", yals->opts.prefix, type);
  STRATSTEMPLATE
  if (yals_is_default_strategy (yals)) fprintf (yals->out, " (default)");
  else fprintf (yals->out, " (random)");
  fprintf (yals->out, "\n");
  yals_msgunlock (yals);
}

/*------------------------------------------------------------------------*/

#define DEFSTRAT(NAME,ENABLED) \
do { \
  yals->strat.NAME = yals->opts.NAME.val; \
} while (0)
#undef STRAT
#define STRAT DEFSTRAT

static void yals_set_default_strategy (Yals * yals) { 
  STRATSTEMPLATE
  yals->stats.strat.def++;
}

/*------------------------------------------------------------------------*/

static int yals_rand_opt (Yals * yals, Opt * opt, const char * name) {
  unsigned mod, r;
  int res;
  mod = opt->max;
  mod -= opt->min;
  mod++;
  if (mod) {
    r = yals_rand_mod (yals, mod);
    res = (int)(r + (unsigned) opt->min);
  } else {
    assert (opt->min == INT_MIN);
    assert (opt->max == INT_MAX);
    res = (int) yals_rand (yals);
  }
  assert (opt->min <= res), assert (res <= opt->max);
  (void) name;
  LOG ("randomly picking strategy %s=%d from [%d,%d] default %d",
    name, res, opt->min, opt->max, opt->val);
  return res;
}

#define RANDSTRAT(NAME,ENABLED) \
do { \
  if ((ENABLED)) { \
    yals->strat.NAME = \
      yals_rand_opt (yals, &yals->opts.NAME, #NAME); \
  } else yals->strat.NAME = yals->opts.NAME.val; \
} while (0)
#undef STRAT
#define STRAT RANDSTRAT

static void yals_set_random_strategy (Yals * yals) { 
  STRATSTEMPLATE
  assert (yals->stats.restart.inner.count > 1);
  yals->stats.strat.rnd++;
  if (yals->strat.cached)
    yals->strat.pol = yals->opts.pol.val;
}

/*------------------------------------------------------------------------*/

static void yals_pick_strategy (Yals * yals) {
  assert (yals->stats.restart.inner.count > 0);
  if (yals->stats.restart.inner.count == 1 ||
      !yals_rand_mod (yals, yals->opts.fixed.val))
    yals_set_default_strategy (yals);
  else yals_set_random_strategy (yals);
  yals_print_strategy (yals, "picked strategy:", 2);
}

static void yals_fix_strategy (Yals * yals) {
  if (yals->uniform) {
    yals->strat.correct = 1;
    yals->strat.pol = 0;
    yals->strat.uni = 1;
    yals->strat.weight = 1;
    yals_print_strategy (yals, "fixed strategy:", 2);
  }
}

/*------------------------------------------------------------------------*/

static int yals_inner_restart_interval (Yals * yals) {
  int res = yals->opts.restart.val;
  if (res < yals->nvars/2) res = yals->nvars/2;
  return res;
}

static int64_t yals_outer_restart_interval (Yals * yals) {
  int64_t res = yals_inner_restart_interval (yals);
  res *= yals->opts.restartouterfactor.val;
  assert (res >= 0);
  return res;
}

static int yals_inc_inner_restart_interval (Yals * yals) {
  int64_t interval;
  int res;

  if (yals->opts.restart.val > 0) {
    if (yals->opts.reluctant.val) {
      if (!yals->limits.restart.inner.rds.u)
	yals->limits.restart.inner.rds.u =
	  yals->limits.restart.inner.rds.v = 1;
      interval = yals->limits.restart.inner.rds.v;
      interval *= yals_inner_restart_interval (yals);
      yals_rds (&yals->limits.restart.inner.rds);
    } else {
      if (!yals->limits.restart.inner.interval)
	yals->limits.restart.inner.interval =
	  yals_inner_restart_interval (yals);
      interval = yals->limits.restart.inner.interval;
      if (yals->limits.restart.inner.interval <= YALS_INT64_MAX/2)
	yals->limits.restart.inner.interval *= 2;
      else yals->limits.restart.inner.interval = YALS_INT64_MAX;
    }
  } else interval = YALS_INT64_MAX;

  yals_msg (yals, 2, "next restart interval %lld", interval);

  if (YALS_INT64_MAX - yals->stats.flips >= interval)
    yals->limits.restart.inner.lim = yals->stats.flips + interval;
  else
    yals->limits.restart.inner.lim = YALS_INT64_MAX;

  yals_msg (yals, 2,
    "next restart at %lld",
    yals->limits.restart.inner.lim);

  res = (yals->stats.restart.inner.maxint < interval);
  if (res) {
    yals->stats.restart.inner.maxint = interval;
    yals_msg (yals, 2, "new maximal restart interval %lld", interval);
  }

  return res;
}

static int yals_need_to_restart_inner (Yals * yals) {
  if (yals->uniform && 
      yals->stats.restart.inner.count >= yals->opts.unirestarts.val)
    return 0;
  return yals->stats.flips >= yals->limits.restart.inner.lim;
}

static int yals_need_to_restart_outer (Yals * yals) {
  return yals->stats.flips >= yals->limits.restart.outer.lim;
}

static void yals_restart_inner (Yals * yals) {
  double start;
  assert (yals_need_to_restart_inner (yals));
  start = yals_time (yals);
  yals->stats.restart.inner.count++;
  if ((yals_inc_inner_restart_interval (yals) && yals->opts.verbose.val) ||
      yals->opts.verbose.val >= 2)
    yals_report (yals, "restart %lld", yals->stats.restart.inner.count);
  if (yals->stats.best < yals->stats.last) {
    yals->stats.pick.keep++;
    yals_msg (yals, 2,
      "keeping strategy and assignment thus essentially skipping restart");
  } else {
    yals_cache_assignment (yals);
    yals_pick_strategy (yals);
    yals_fix_strategy (yals);
    yals_pick_assignment (yals, 0);
    yals_update_sat_and_unsat (yals);
    yals->stats.tmp = INT_MAX;
    yals_save_new_minimum (yals);
  }
  yals->stats.last = yals->stats.best;
  yals->stats.time.restart += yals_time (yals) - start;
}

/*------------------------------------------------------------------------*/

static int yals_done (Yals * yals) {
  assert (!yals->mt);
  if (!yals_nunsat (yals)) return 1;
  if (yals->limits.flips >= 0 &&
      yals->limits.flips <= yals->stats.flips) {
    yals_msg (yals, 1,
      "flips limit %lld after %lld flips reached",
      yals->limits.flips, yals->stats.flips);
    return -1;
  }
#ifndef NYALSMEMS
  if (yals->limits.mems >= 0 &&
      yals->limits.mems <= yals->stats.mems.all) {
    yals_msg (yals, 1,
      "mems limit %lld after %lld mems reached",
      yals->limits.mems, yals->stats.mems.all);
    return -1;
  }
#endif
  if (yals->cbs.term.fun && yals->limits.term-- <= 0) {
    yals->limits.term = yals->opts.termint.val;
    if (yals->cbs.term.fun (yals->cbs.term.state)) {
      yals_msg (yals, 1, "forced to terminate");
      return -1;
    }
  }
  if (yals->opts.hitlim.val >= 0 &&
      yals->stats.hits  >= yals->opts.hitlim.val) {
    yals_msg (yals, 1,
      "minimum hits limit %lld reached after %lld hits",
      yals->opts.hitlim.val, yals->stats.hits);
    return -1;
  }
  return 0;
}

static void yals_init_inner_restart_interval (Yals * yals) {
  memset (&yals->limits.restart.inner, 0, sizeof yals->limits.restart.inner);
  yals->limits.restart.inner.lim = yals->stats.flips;
  yals->stats.restart.inner.maxint = 0;
}

static int yals_inner_loop (Yals * yals) {
  int res = 0;
  yals_init_inner_restart_interval (yals);
  LOG ("entering yals inner loop");
  while (!(res = yals_done (yals)) && !yals_need_to_restart_outer (yals))
    if (yals_need_to_restart_inner (yals)) yals_restart_inner (yals);
    else yals_flip (yals);
  return res;
}

static void yals_init_outer_restart_interval (Yals * yals) {
  if (yals->opts.restartouter.val) {
    yals->limits.restart.outer.interval = yals_outer_restart_interval (yals);
    yals->limits.restart.outer.lim = 
      yals->stats.flips + yals->limits.restart.outer.interval;
    yals_msg (yals, 1,
      "initial outer restart limit at %lld flips",
      yals->limits.restart.outer.lim);
  } else {
    yals_msg (yals, 1, "outer restarts disabled");
    yals->limits.restart.outer.lim = YALS_INT64_MAX;
  }
}

static void yals_restart_outer (Yals * yals) {
  double start = yals_time (yals);
  unsigned long long seed;
  int64_t interval;
  yals->stats.restart.outer.count++;
  seed = yals_rand (yals);
  seed |= ((unsigned long long) yals_rand (yals)) << 32;
  yals_srand (yals, seed);
  interval = yals->limits.restart.outer.interval;
  if (yals->limits.restart.outer.interval <= YALS_INT64_MAX/2)
    yals->limits.restart.outer.interval *= 2;
  else yals->limits.restart.outer.interval = YALS_INT64_MAX;
  if (YALS_INT64_MAX - yals->stats.flips >= interval)
    yals->limits.restart.outer.lim = yals->stats.flips + interval;
  else
    yals->limits.restart.outer.lim = YALS_INT64_MAX;
  yals_msg (yals, 1,
    "starting next outer restart round %lld after %.2f seconds",
    (long long) yals->stats.restart.outer.count, yals_sec (yals));
  yals_msg (yals, 1, "current seed %llu", seed);
  yals_msg (yals, 1,
    "next outer restart limit %lld",
    (long long) yals->limits.restart.outer.lim);
  yals_reset_cache (yals);
  yals->stats.time.restart += yals_time (yals) - start;
}

static void yals_outer_loop (Yals * yals) {
  yals_init_outer_restart_interval (yals);
  for (;;) {
    yals_set_default_strategy (yals);
    yals_fix_strategy (yals);
    yals_pick_assignment (yals, 1);
    yals_update_sat_and_unsat (yals);
    yals->stats.tmp = INT_MAX;
    yals_save_new_minimum (yals);
    yals->stats.last = yals_nunsat (yals);
    if (yals_inner_loop (yals)) return;
    yals_restart_outer (yals);
  }
}

/*------------------------------------------------------------------------*/

static void yals_check_assignment (Yals * yals) {
  const int * c = yals->cdb.start;
  while (c < yals->cdb.top) {
    int satisfied = 0, lit;
    while ((lit = *c++))
      satisfied += yals_best (yals, lit);
    if (!satisfied)
      yals_abort (yals,
	"internal error in 'yals_sat' (invalid satisfying assignment)");
  }
}

static int yals_lkhd_internal (Yals * yals) {
  int64_t maxflips;
  int res = 0, idx;
  if (!yals->flips) goto DONE;
  maxflips = -1;
  for (idx = 1; idx < yals->nvars; idx++) {
    int64_t tmpflips = yals->flips[idx];
    if (tmpflips <= maxflips) continue;
    res = idx, maxflips = tmpflips;
  }
  if (res &&
      yals->best &&
      !GETBIT (yals->best, yals->nvarwords, res))
    res = -res;
DONE:
  return res;
}

int yals_sat (Yals * yals) {
  int res, limited = 0, lkhd;

  if (!EMPTY (yals->clause))
    yals_abort (yals, "added clause incomplete in 'yals_sat'");

  if (yals->mt) {
    yals_msg (yals, 1, "original formula contains empty clause");
    return 20;
  }

  if (yals->opts.prep.val && !EMPTY (yals->trail)) {
    yals_preprocess (yals);
    if (yals->mt) {
      yals_msg (yals, 1,
	"formula after unit propagation contains empty clause");
      return 20;
    }
  }

  yals->stats.time.entered = yals_time (yals);

  if (yals->opts.setfpu.val) yals_set_fpu (yals);
  yals_connect (yals);

  res = 0;
  limited += (yals->limits.flips >= 0);
#ifndef NYALSMEMS
  limited += (yals->limits.mems >= 0);
#endif
  if (!limited)
    yals_msg (yals, 1, "starting unlimited search");
  else {

    if (yals->limits.flips < 0)
      yals_msg (yals, 1,
	"search not limited by the number of flips");
    else
      yals_msg (yals, 1,
	"search limited by %lld flips",
	 (long long) yals->limits.flips);

#ifndef NYALSMEMS
    if (yals->limits.mems < 0)
      yals_msg (yals, 1,
	"search not limited by the number of mems");
    else
      yals_msg (yals, 1,
	"search limited by %lld mems",
	 (long long) yals->limits.mems);
#endif
  }

  yals_outer_loop (yals);

  assert (!yals->mt);
  if (!yals->stats.best) { 
    yals_print_strategy (yals, "winning strategy:", 1);
    yals_check_assignment (yals);
    res = 10;
  } else assert (!res);

  if ((lkhd = yals_lkhd_internal (yals)))
    yals_msg (yals, 1,
      "most flipped literal %d flipped %lld times",
      lkhd, (long long) yals->flips[ABS (lkhd)]);

  if (yals->opts.setfpu.val) yals_reset_fpu (yals);
  yals_flush_time (yals);

  return res;
}

/*------------------------------------------------------------------------*/

int yals_deref (Yals * yals, int lit) {
  if (!lit) yals_abort (yals, "zero literal argument to 'yals_val'");
  if (yals->mt || ABS (lit) >= yals->nvars) return lit < 0 ? 1 : -1;
  return yals_best (yals, lit) ? 1 : -1;
}

int yals_minimum (Yals * yals) { return yals->stats.best; }

long long yals_flips (Yals * yals) { return yals->stats.flips; }

long long yals_mems (Yals * yals) { 
#ifndef NYALSMEMS
  return yals->stats.mems.all;
#else
  return 0;
#endif
}

int yals_lkhd (Yals * yals) {
  int res = yals_lkhd_internal (yals);
  if (res) 
    yals_msg (yals, 1,
      "look ahead literal %d flipped %lld times",
      res, (long long) yals->flips [ABS (res)]);
  else
    yals_msg (yals, 2, "no look ahead literal found");
  return res;
}

/*------------------------------------------------------------------------*/

static void yals_minlits_cidx (Yals * yals, int cidx) {
  const int * lits, * p;
  int lit;
  assert_valid_cidx (cidx);
  lits = yals_lits (yals, cidx);
  for (p = lits; (lit = *p); p++)
    if (yals_best (yals, lit))
      return;
  for (p = lits; (lit = *p); p++) {
    int idx = ABS (lit);
    assert (idx < yals->nvars);
    if (yals->mark.start[idx]) continue;
    yals->mark.start[idx] = 1;
    PUSH (yals->minlits, lit);
  }
}

const int * yals_minlits (Yals * yals) {
  int count, cidx;
  RELEASE (yals->mark);
  while (COUNT (yals->mark) < yals->nvars)
    PUSH (yals->mark, 0);
  FIT (yals->mark);
  CLEAR (yals->minlits);
  for (cidx = 0; cidx < yals->nclauses; cidx++)
    yals_minlits_cidx (yals, cidx);
  count = COUNT (yals->minlits);
  yals_msg (yals, 1,
    "found %d literals in unsat clauses %.0f%%",
    count, yals_pct (count, yals->nvars));
  PUSH (yals->minlits, 0);
  RELEASE (yals->mark);
  FIT (yals->minlits);
  return yals->minlits.start;
}

/*------------------------------------------------------------------------*/

void yals_stats (Yals * yals) {
  Stats * s = &yals->stats;
  double t = s->time.total;
  int64_t sum;
  yals_msg (yals, 0,
    "restart time %.3f seconds %.0f%%",
    s->time.restart, yals_pct (s->time.restart, s->time.total));
  yals_msg (yals, 0,
    "%lld inner restarts, %lld maximum interval",
    (long long) s->restart.inner.count,
    (long long) s->restart.inner.maxint);
  yals_msg (yals, 0,
    "%lld outer restarts, %lld maximum interval",
    (long long) s->restart.outer.count,
    (long long) yals->limits.restart.outer.interval);
  sum = s->strat.def + s->strat.rnd;
  yals_msg (yals, 0,
    "default strategy %lld %.0f%%, random strategy %lld %.0f%%",
    (long long) s->strat.def, yals_pct (s->strat.def, sum),
    (long long) s->strat.rnd, yals_pct (s->strat.rnd, sum));
  yals_msg (yals, 0,
    "picked best=%lld cached=%lld keep=%lld pos=%lld neg=%lld rnd=%lld",
    (long long) s->pick.best, (long long) s->pick.cached,
    (long long) s->pick.keep, (long long) s->pick.pos,
    (long long) s->pick.neg, (long long) s->pick.rnd);
  sum = s->cache.inserted + s->cache.replaced;
  yals_msg (yals, 0,
    "cached %lld assignments, %lld replaced %.0f%%, %lld skipped, %d size",
    (long long) sum,
    (long long) s->cache.replaced, yals_pct (s->cache.replaced, sum),
    (long long) s->cache.skipped, (int) COUNT (yals->cache));
  sum = s->sig.falsepos + s->sig.truepos;
  yals_msg (yals, 0,
    "%lld sigchecks, %lld negative %.0f%%, "
    "%lld positive %.0f%%, %lld false %.0f%%",
    (long long) s->sig.search,
    (long long) s->sig.neg, yals_pct (s->sig.neg, s->sig.search),
    (long long) sum, yals_pct (sum, s->sig.search),
    (long long) s->sig.falsepos, yals_pct (s->sig.falsepos, s->sig.search));
  if (yals->unsat.usequeue) {
    yals_msg (yals, 0,
      "allocated max %d chunks %d links %lld unfair",
      s->queue.max.chunks, s->queue.max.lnks, (long long) s->queue.unfair);
    yals_msg (yals, 0,
      "%lld defragmentations in %.3f seconds %.0f%%",
      (long long) s->defrag.count,
      s->time.defrag, yals_pct (s->time.defrag, s->time.total));
    yals_msg (yals, 0,
      "moved %lld in total and %.1f on average per defragmentation",
      (long long) s->defrag.moved,
      yals_avg (s->defrag.moved, s->defrag.count));
  } else
    yals_msg (yals, 0, "maximum unsat stack size %d", s->maxstacksize);

#ifndef NYALSTATS
  yals_msg (yals, 0,
    "%lld broken, %.2f broken/flip, %lld made, %.2f made/flip",
    (long long) s->broken, yals_avg (s->broken, s->flips),
    (long long) s->made, yals_avg (s->made, s->flips));

  yals_msg (yals, 0,
    "%u maximum %u minimum sum of weighted breaks",
    s->wb.max, s->wb.min);
#endif

  yals_msg (yals, 0,
    "%.3f unsat clauses on average",
    yals_pct (s->unsum, s->flips));

#ifndef NYALSTATS
  if (s->inc && s->dec) {
    double suminc, sumdec, f = s->flips;
    int64_t ninc, ndec, ninclarge, ndeclarge;
    int len;
    assert (!s->dec[0]);
    assert (!s->inc[yals->maxlen]);
    suminc = sumdec = ninc = ndec = ndeclarge = ninclarge = 0;
    for (len = 0; len <= yals->maxlen; len++) {
      int64_t inc = s->inc[len];
      int64_t dec = s->dec[len];
      ninc += inc, ndec += dec;
      suminc += len * inc, sumdec += len * dec;
      if (len > 2) ninclarge += inc;
      if (len > 3) ndeclarge += dec;
    }
    yals_msg (yals, 0,
      "%lld incs %.3f avg satcnt, %lld decs %.3f avg satcnt",
      (long long) ninc, yals_avg (suminc, ninc),
      (long long) ndec, yals_avg (sumdec, ndec));
    yals_msg (yals, 0,
      "inc0 %.2f %.2f%%, inc1 %.2f %.2f%%, "
      "inc2 %.2f %.2f%%, incl %.2f %.2f%%",
      yals_avg (s->inc[0], f), yals_pct (s->inc[0], ninc),
      yals_avg (s->inc[1], f), yals_pct (s->inc[1], ninc),
      yals_avg (s->inc[2], f), yals_pct (s->inc[2], ninc),
      yals_avg (ninclarge, f), yals_pct (ninclarge, ninc));
    yals_msg (yals, 0,
      "dec1 %.2f %.2f%%, dec2 %.2f %.2f%%, "
      "dec3 %.2f %.2f%%, decl %.2f %.2f%%",
      yals_avg ( s->dec[1], f), yals_pct (s->dec[1], ndec),
      yals_avg (s->dec[2], f), yals_pct (s->dec[2], ndec),
      yals_avg (s->dec[3], f), yals_pct (s->dec[3], ndec),
      yals_avg (ndeclarge, f), yals_pct (ndeclarge, ndec));
  }
#endif

#ifndef NYALSMEMS
  {
    double M = 1000000.0;
    long long all = 0;

    all += s->mems.lits;
    all += s->mems.crit;
    all += s->mems.occs;
    all += s->mems.read;
    all += s->mems.update;
    all += s->mems.weight;

    assert (all == s->mems.all);

    yals_msg (yals, 0,
      "megamems: %.0f all 100%, %.0f lits %.0f%%, %.0f crit %.0f%%, "
      "%.0f weight %.0f%%",
      all/M,
      s->mems.lits/M, yals_pct (s->mems.lits, all),
      s->mems.crit/M, yals_pct (s->mems.crit, all),
      s->mems.weight/M, yals_pct (s->mems.weight, all));

    yals_msg (yals, 0,
      "megamems: %.0f occs %.0f%%, %.0f read %.0f%%, %.0f update %.0f%%",
      s->mems.occs/M, yals_pct (s->mems.occs, all),
      s->mems.read/M, yals_pct (s->mems.read, all),
      s->mems.update/M, yals_pct (s->mems.update, all));

    yals_msg (yals, 0,
      "%.1f million mems per second, %.1f mems per flip",
      yals_avg (all/1e6, t), yals_avg (all, s->flips));
  }
#endif

  yals_msg (yals, 0,
    "%.1f seconds searching, %.1f MB maximally allocated (%lld bytes)",
    t, s->allocated.max/(double)(1<<20), (long long) s->allocated.max);

  yals_msg (yals, 0,
    "%lld flips, %.1f thousand flips per second, %lld break zero %.1f%%",
    (long long) s->flips, yals_avg (s->flips/1e3, t),
    s->bzflips, yals_pct (s->bzflips, s->flips));

  yals_msg (yals, 0,
    "minimum %d hit %lld times, maximum %d",
    s->best, (long long) s->hits, s->worst);
}
