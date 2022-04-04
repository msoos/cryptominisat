/****************************************************************************
Copyright (c) 2006 - 2015, Armin Biere, Johannes Kepler University.

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>

#include "picosat.h"

/* By default code for 'all different constraints' is disabled, since 'NADC'
 * is defined.
 */
#define NADC

/* By default we enable failed literals, since 'NFL' is undefined.
 *
#define NFL
 */

/* By default we 'detach satisfied (large) clauses', e.g. NDSC undefined.
 *
#define NDSC
 */

/* Do not use luby restart schedule instead of inner/outer.
 *
#define NLUBY
 */

/* Enabling this define, will use gnuplot to visualize how the scores evolve.
 *
#define VISCORES 
 */
#ifdef VISCORES
// #define WRITEGIF		/* ... to generate a video */
#endif

#ifdef VISCORES
#ifndef WRITEGIF
#include <unistd.h>		/* for 'usleep' */
#endif
#endif

#ifdef RCODE
#include <R.h>
#endif

#define MINRESTART	100	/* minimum restart interval */
#define MAXRESTART	1000000 /* maximum restart interval */
#define RDECIDE		1000	/* interval of random decisions */
#define FRESTART	110	/* restart increase factor in percent */
#define FREDUCE		110	/* reduce increase factor in percent  */
#define FREDADJ		121	/* reduce increase adjustment factor */
#define MAXCILS		10	/* maximal number of unrecycled internals */
#define FFLIPPED	10000	/* flipped reduce factor */
#define FFLIPPEDPREC	10000000/* flipped reduce factor precision */
#define INTERRUPTLIM	1024	/* check interrupt after that many decisions */

#ifndef TRACE
#define NO_BINARY_CLAUSES	/* store binary clauses more compactly */
#endif

/* For debugging purposes you may want to define 'LOGGING', which actually
 * can be enforced by using './configure.sh --log'.
 */
#ifdef LOGGING
#define LOG(code) do { code; } while (0)
#else
#define LOG(code) do { } while (0)
#endif
#define NOLOG(code) do { } while (0)		/* log exception */
#define ONLYLOG(code) do { code; } while (0)	/* force logging */

#define FALSE ((Val)-1)
#define UNDEF ((Val)0)
#define TRUE ((Val)1)

#define COMPACT_TRACECHECK_TRACE_FMT 0
#define EXTENDED_TRACECHECK_TRACE_FMT 1
#define RUP_TRACE_FMT 2

#define NEWN(p,n) do { (p) = new (ps, sizeof (*(p)) * (n)); } while (0)
#define CLRN(p,n) do { memset ((p), 0, sizeof (*(p)) * (n)); } while (0)
#define CLR(p) CLRN(p,1)
#define DELETEN(p,n) \
  do { delete (ps, p, sizeof (*(p)) * (n)); (p) = 0; } while (0)

#define RESIZEN(p,old_num,new_num) \
  do { \
    size_t old_size = sizeof (*(p)) * (old_num); \
    size_t new_size = sizeof (*(p)) * (new_num); \
    (p) = resize (ps, (p), old_size, new_size) ; \
  } while (0)

#define ENLARGE(start,head,end) \
  do { \
    unsigned old_num = (ptrdiff_t)((end) - (start)); \
    size_t new_num = old_num ? (2 * old_num) : 1; \
    unsigned count = (head) - (start); \
    assert ((start) <= (end)); \
    RESIZEN((start),old_num,new_num); \
    (head) = (start) + count; \
    (end) = (start) + new_num; \
  } while (0)

#define NOTLIT(l) (ps->lits + (1 ^ ((l) - ps->lits)))

#define LIT2IDX(l) ((ptrdiff_t)((l) - ps->lits) / 2)
#define LIT2IMPLS(l) (ps->impls + (ptrdiff_t)((l) - ps->lits))
#define LIT2INT(l) ((int)(LIT2SGN(l) * LIT2IDX(l)))
#define LIT2SGN(l) (((ptrdiff_t)((l) - ps->lits) & 1) ? -1 : 1)
#define LIT2VAR(l) (ps->vars + LIT2IDX(l))
#define LIT2HTPS(l) (ps->htps + (ptrdiff_t)((l) - ps->lits))
#define LIT2JWH(l) (ps->jwh + ((l) - ps->lits))

#ifndef NDSC
#define LIT2DHTPS(l) (ps->dhtps + (ptrdiff_t)((l) - ps->lits))
#endif

#ifdef NO_BINARY_CLAUSES
typedef uintptr_t Wrd;
#define ISLITREASON(C) (1&(Wrd)C)
#define LIT2REASON(L) \
  (assert (L->val==TRUE), ((Cls*)(1 + (2*(L - ps->lits)))))
#define REASON2LIT(C) ((Lit*)(ps->lits + ((Wrd)C)/2))
#endif

#define ENDOFCLS(c) ((void*)((Lit**)(c)->lits + (c)->size))

#define SOC ((ps->oclauses == ps->ohead) ? ps->lclauses : ps->oclauses)
#define EOC (ps->lhead)
#define NXC(p) (((p) + 1 == ps->ohead) ? ps->lclauses : (p) + 1)

#define OIDX2IDX(idx) (2 * ((idx) + 1))
#define LIDX2IDX(idx) (2 * (idx) + 1)

#define ISLIDX(idx) ((idx)&1)

#define IDX2OIDX(idx) (assert(!ISLIDX(idx)), (idx)/2 - 1)
#define IDX2LIDX(idx) (assert(ISLIDX(idx)), (idx)/2)

#define EXPORTIDX(idx) \
  ((ISLIDX(idx) ? (IDX2LIDX (idx) + (ps->ohead - ps->oclauses)) : IDX2OIDX(idx)) + 1)

#define IDX2CLS(i) \
  (assert(i), (ISLIDX(i) ? ps->lclauses : ps->oclauses)[(i)/2 - !ISLIDX(i)])

#define IDX2ZHN(i) (assert(i), (ISLIDX(i) ? ps->zhains[(i)/2] : 0))

#define CLS2TRD(c) (((Trd*)(c)) - 1)
#define CLS2IDX(c) ((((Trd*)(c)) - 1)->idx)

#define CLS2ACT(c) \
  ((Act*)((assert((c)->learned)),assert((c)->size>2),ENDOFCLS(c)))

#define VAR2LIT(v) (ps->lits + 2 * ((v) - ps->vars))
#define VAR2RNK(v) (ps->rnks + ((v) - ps->vars))

#define RNK2LIT(r) (ps->lits + 2 * ((r) - ps->rnks))
#define RNK2VAR(r) (ps->vars + ((r) - ps->rnks))

#define BLK_FILL_BYTES 8
#define SIZE_OF_BLK (sizeof (Blk) - BLK_FILL_BYTES)

#define PTR2BLK(void_ptr) \
  ((void_ptr) ? (Blk*)(((char*)(void_ptr)) - SIZE_OF_BLK) : 0)

#define AVERAGE(a,b) ((b) ? (((double)a) / (double)(b)) : 0.0)
#define PERCENT(a,b) (100.0 * AVERAGE(a,b))

#ifndef RCODE
#define ABORT(msg) \
  do { \
    fputs ("*** picosat: " msg "\n", stderr); \
    abort (); \
  } while (0)
#else
#define ABORT(msg) \
  do { \
    Rf_error (msg); \
  } while (0)
#endif

#define ABORTIF(cond,msg) \
  do { \
    if (!(cond)) break; \
    ABORT (msg); \
  } while (0)

#define ZEROFLT		(0x00000000u)
#define EPSFLT		(0x00000001u)
#define INFFLT		(0xffffffffu)

#define FLTCARRY	(1u << 25)
#define FLTMSB		(1u << 24)
#define FLTMAXMANTISSA	(FLTMSB - 1)

#define FLTMANTISSA(d)	((d) & FLTMAXMANTISSA)
#define FLTEXPONENT(d)	((int)((d) >> 24) - 128)

#define FLTMINEXPONENT	(-128)
#define FLTMAXEXPONENT	(127)

#define CMPSWAPFLT(a,b) \
  do { \
    Flt tmp; \
    if (((a) < (b))) \
      { \
	tmp = (a); \
	(a) = (b); \
	(b) = tmp; \
      } \
  } while (0)

#define UNPACKFLT(u,m,e) \
  do { \
    (m) = FLTMANTISSA(u); \
    (e) = FLTEXPONENT(u); \
    (m) |= FLTMSB; \
  } while (0)

#define INSERTION_SORT_LIMIT 10

#define SORTING_SWAP(T,p,q) \
do { \
  T tmp = *(q); \
  *(q) = *(p); \
  *(p) = tmp; \
} while (0)

#define SORTING_CMP_SWAP(T,cmp,p,q) \
do { \
  if ((cmp) (ps, *(p), *(q)) > 0) \
    SORTING_SWAP (T, p, q); \
} while(0)

#define QUICKSORT_PARTITION(T,cmp,a,l,r) \
do { \
  T pivot; \
  int j; \
  i = (l) - 1; 			/* result in 'i' */ \
  j = (r); \
  pivot = (a)[j]; \
  for (;;) \
    { \
      while ((cmp) (ps, (a)[++i], pivot) < 0) \
	; \
      while ((cmp) (ps, pivot, (a)[--j]) < 0) \
        if (j == (l)) \
	  break; \
      if (i >= j) \
	break; \
      SORTING_SWAP (T, (a) + i, (a) + j); \
    } \
  SORTING_SWAP (T, (a) + i, (a) + (r)); \
} while(0)

#define QUICKSORT(T,cmp,a,n) \
do { \
  int l = 0, r = (n) - 1, m, ll, rr, i; \
  assert (ps->ihead == ps->indices); \
  if (r - l <= INSERTION_SORT_LIMIT) \
    break; \
  for (;;) \
    { \
      m = (l + r) / 2; \
      SORTING_SWAP (T, (a) + m, (a) + r - 1); \
      SORTING_CMP_SWAP (T, cmp, (a) + l, (a) + r - 1); \
      SORTING_CMP_SWAP (T, cmp, (a) + l, (a) + r); \
      SORTING_CMP_SWAP (T, cmp, (a) + r - 1, (a) + r); \
      QUICKSORT_PARTITION (T, cmp, (a), l + 1, r - 1); \
      if (i - l < r - i) \
	{ \
	  ll = i + 1; \
	  rr = r; \
	  r = i - 1; \
	} \
      else \
	{ \
	  ll = l; \
	  rr = i - 1; \
	  l = i + 1; \
	} \
      if (r - l > INSERTION_SORT_LIMIT) \
	{ \
	  assert (rr - ll > INSERTION_SORT_LIMIT); \
	  if (ps->ihead == ps->eoi) \
	    ENLARGE (ps->indices, ps->ihead, ps->eoi); \
	  *ps->ihead++ = ll; \
	  if (ps->ihead == ps->eoi) \
	    ENLARGE (ps->indices, ps->ihead, ps->eoi); \
	  *ps->ihead++ = rr; \
	} \
      else if (rr - ll > INSERTION_SORT_LIMIT) \
        { \
	  l = ll; \
	  r = rr; \
	} \
      else if (ps->ihead > ps->indices) \
	{ \
	  r = *--ps->ihead; \
	  l = *--ps->ihead; \
	} \
      else \
	break; \
    } \
} while (0)

#define INSERTION_SORT(T,cmp,a,n) \
do { \
  T pivot; \
  int l = 0, r = (n) - 1, i, j; \
  for (i = r; i > l; i--) \
    SORTING_CMP_SWAP (T, cmp, (a) + i - 1, (a) + i); \
  for (i = l + 2; i <= r; i++)  \
    { \
      j = i; \
      pivot = (a)[i]; \
      while ((cmp) (ps, pivot, (a)[j - 1]) < 0) \
        { \
	  (a)[j] = (a)[j - 1]; \
	  j--; \
	} \
      (a)[j] = pivot; \
    } \
} while (0)

#ifdef NDEBUG
#define CHECK_SORTED(cmp,a,n) do { } while(0)
#else
#define CHECK_SORTED(cmp,a,n) \
do { \
  int i; \
  for (i = 0; i < (n) - 1; i++) \
    assert ((cmp) (ps, (a)[i], (a)[i + 1]) <= 0); \
} while(0)
#endif

#define SORT(T,cmp,a,n) \
do { \
  T * aa = (a); \
  int nn = (n); \
  QUICKSORT (T, cmp, aa, nn); \
  INSERTION_SORT (T, cmp, aa, nn); \
  assert (ps->ihead == ps->indices); \
  CHECK_SORTED (cmp, aa, nn); \
} while (0)

#define WRDSZ (sizeof (long) * 8)

#ifdef RCODE
#define fprintf(...) do { } while (0)
#define vfprintf(...) do { } while (0)
#define fputs(...) do { } while (0)
#define fputc(...) do { } while (0)
#endif

typedef unsigned Flt;		/* 32 bit deterministic soft float */
typedef Flt Act;		/* clause and variable activity */
typedef struct Blk Blk;		/* allocated memory block */
typedef struct Cls Cls;		/* clause */
typedef struct Lit Lit;		/* literal */
typedef struct Rnk Rnk;		/* variable to score mapping */
typedef signed char Val;	/* TRUE, UNDEF, FALSE */
typedef struct Var Var;		/* variable */
#ifdef TRACE
typedef struct Trd Trd;		/* trace data for clauses */
typedef struct Zhn Zhn;		/* compressed chain (=zain) data */
typedef unsigned char Znt;	/* compressed antecedent data */
#endif

#ifdef NO_BINARY_CLAUSES
typedef struct Ltk Ltk;

struct Ltk
{
  Lit ** start;
  unsigned count : WRDSZ == 32 ? 27 : 32;
  unsigned ldsize : WRDSZ == 32 ? 5 : 32;
};
#endif

struct Lit
{
  Val val;
};

struct Var
{
  unsigned mark		: 1;	/*bit 1*/
  unsigned resolved	: 1;	/*bit 2*/
  unsigned phase	: 1;	/*bit 3*/
  unsigned assigned	: 1;	/*bit 4*/
  unsigned used		: 1;	/*bit 5*/
  unsigned failed	: 1;	/*bit 6*/
  unsigned internal	: 1;	/*bit 7*/
  unsigned usedefphase  : 1;    /*bit 8*/
  unsigned defphase     : 1;    /*bit 9*/
  unsigned msspos       : 1;    /*bit 10*/
  unsigned mssneg       : 1;    /*bit 11*/
  unsigned humuspos     : 1;    /*bit 12*/
  unsigned humusneg     : 1;    /*bit 13*/
  unsigned partial      : 1;    /*bit 14*/
#ifdef TRACE
  unsigned core		: 1;	/*bit 15*/
#endif
  unsigned level;
  Cls *reason;
#ifndef NADC
  Lit ** inado;
  Lit ** ado;
  Lit *** adotabpos;
#endif
};

struct Rnk
{
  Act score;
  unsigned pos : 30;			/* 0 iff not on heap */
  unsigned moreimportant : 1;
  unsigned lessimportant : 1;
};

struct Cls
{
  unsigned size;

  unsigned collect:1;	/* bit 1 */
  unsigned learned:1;	/* bit 2 */
  unsigned locked:1;	/* bit 3 */
  unsigned used:1;	/* bit 4 */
#ifndef NDEBUG
  unsigned connected:1;	/* bit 5 */
#endif
#ifdef TRACE
  unsigned collected:1;	/* bit 6 */
  unsigned core:1;	/* bit 7 */
#endif

#define LDMAXGLUE 25	/* 32 - 7 */
#define MAXGLUE 	((1<<LDMAXGLUE)-1)

  unsigned glue:LDMAXGLUE;

  Cls *next[2];
  Lit *lits[2];
};

#ifdef TRACE
struct Zhn
{
  unsigned ref:31;
  unsigned core:1;
  Znt * liz;
  Znt znt[0];
};

struct Trd
{
  unsigned idx;
  Cls cls[0];
};
#endif

struct Blk
{
#ifndef NDEBUG
  union
  {
    size_t size;		/* this is what we really use */
    void *as_two_ptrs[2];	/* 2 * sizeof (void*) alignment of data */
  }
  header;
#endif
  char data[BLK_FILL_BYTES];
};

enum State
{
  RESET = 0,
  READY = 1,
  SAT = 2,
  UNSAT = 3,
  UNKNOWN = 4,
};

enum Phase
{
  POSPHASE,
  NEGPHASE,
  JWLPHASE,
  RNDPHASE,
};

struct PicoSAT 
{
  enum State state;
  enum Phase defaultphase;
  int last_sat_call_result;

  FILE *out;
  char * prefix;
  int verbosity;
  int plain;
  unsigned LEVEL;
  unsigned max_var;
  unsigned size_vars;

  Lit *lits;
  Var *vars;
  Rnk *rnks;
  Flt *jwh;
  Cls **htps;
#ifndef NDSC
  Cls **dhtps;
#endif
#ifdef NO_BINARY_CLAUSES
  Ltk *impls;
  Cls impl, cimpl;
  int implvalid, cimplvalid;
#else
  Cls **impls;
#endif
  Lit **trail, **thead, **eot, **ttail, ** ttail2;
#ifndef NADC
  Lit **ttailado;
#endif
  unsigned adecidelevel;
  Lit **als, **alshead, **alstail, **eoals;
  Lit **CLS, **clshead, **eocls;
  int *rils, *rilshead, *eorils;
  int *cils, *cilshead, *eocils;
  int *fals, *falshead, *eofals;
  int *mass, szmass;
  int *mssass, szmssass;
  int *mcsass, nmcsass, szmcsass;
  int *humus, szhumus;
  Lit *failed_assumption;
  int extracted_all_failed_assumptions;
  Rnk **heap, **hhead, **eoh;
  Cls **oclauses, **ohead, **eoo;	/* original clauses */
  Cls **lclauses, **lhead, ** EOL;	/* learned clauses */
  int * soclauses, * sohead, * eoso; /* saved original clauses */
  int saveorig;
  int partial;
#ifdef TRACE
  int trace;
  Zhn **zhains, **zhead, **eoz;
  int ocore;
#endif
  FILE * rup;
  int rupstarted;
  int rupvariables;
  int rupclauses;
  Cls *mtcls;
  Cls *conflict;
  Lit **added, **ahead, **eoa;
  Var **marked, **mhead, **eom;
  Var **dfs, **dhead, **eod;
  Cls **resolved, **rhead, **eor;
  unsigned char *levels, *levelshead, *eolevels;
  unsigned *dused, *dusedhead, *eodused;
  unsigned char *buffer, *bhead, *eob;
  Act vinc, lscore, ilvinc, ifvinc;
#ifdef VISCORES
  Act fvinc, nvinc;
#endif
  Act cinc, lcinc, ilcinc, fcinc;
  unsigned srng;
  size_t current_bytes;
  size_t max_bytes;
  size_t recycled;
  double seconds, flseconds;
  double entered;
  unsigned nentered;
  int measurealltimeinlib;
  char *rline[2];
  int szrline, RCOUNT;
  double levelsum;
  unsigned iterations;
  int reports;
  int lastrheader;
  unsigned calls;
  unsigned decisions;
  unsigned restarts;
  unsigned simps;
  unsigned fsimplify;
  unsigned isimplify;
  unsigned reductions;
  unsigned lreduce;
  unsigned lreduceadjustcnt;
  unsigned lreduceadjustinc;
  unsigned lastreduceconflicts;
  unsigned llocked;	/* locked large learned clauses */
  unsigned lrestart;
#ifdef NLUBY
  unsigned drestart;
  unsigned ddrestart;
#else
  unsigned lubycnt;
  unsigned lubymaxdelta;
  int waslubymaxdelta;
#endif
  unsigned long long lsimplify;
  unsigned long long propagations;
  unsigned long long lpropagations;
  unsigned fixed;		/* top level assignments */
#ifndef NFL
  unsigned failedlits;
  unsigned ifailedlits;
  unsigned efailedlits;
  unsigned flcalls;
#ifdef STATS
  unsigned flrounds;
  unsigned long long flprops;
  unsigned long long floopsed, fltried, flskipped;
#endif
  unsigned long long fllimit;
  int simplifying;
  Lit ** saved;
  unsigned saved_size;
#endif
  unsigned conflicts;
  unsigned contexts;
  unsigned internals;
  unsigned noclauses;	/* current number large original clauses */
  unsigned nlclauses;	/* current number large learned clauses */
  unsigned olits;		/* current literals in large original clauses */
  unsigned llits;		/* current literals in large learned clauses */
  unsigned oadded;		/* added original clauses */
  unsigned ladded;		/* added learned clauses */
  unsigned loadded;	/* added original large clauses */
  unsigned lladded;	/* added learned large clauses */
  unsigned addedclauses;	/* oadded + ladded */
  unsigned vused;		/* used variables */
  unsigned llitsadded;	/* added learned literals */
  unsigned long long visits;
#ifdef STATS
  unsigned loused;		/* used large original clauses */
  unsigned llused;		/* used large learned clauses */
  unsigned long long bvisits;
  unsigned long long tvisits;
  unsigned long long lvisits;
  unsigned long long othertrue;
  unsigned long long othertrue2;
  unsigned long long othertruel;
  unsigned long long othertrue2u;
  unsigned long long othertruelu;
  unsigned long long ltraversals;
  unsigned long long traversals;
#ifdef TRACE
  unsigned long long antecedents;
#endif
  unsigned uips;
  unsigned znts;
  unsigned assumptions;
  unsigned rdecisions;
  unsigned sdecisions;
  size_t srecycled;
  size_t rrecycled;
  unsigned long long derefs;
#endif
  unsigned minimizedllits;
  unsigned nonminimizedllits;
#ifndef NADC
  Lit *** ados, *** hados, *** eados;
  Lit *** adotab;
  unsigned nadotab;
  unsigned szadotab;
  Cls * adoconflict;
  unsigned adoconflicts;
  unsigned adoconflictlimit;
  int addingtoado;
  int adodisabled;
#endif
  unsigned long long flips;
#ifdef STATS
  unsigned long long FORCED;
  unsigned long long assignments;
  unsigned inclreduces;
  unsigned staticphasedecisions;
  unsigned skippedrestarts;
#endif
  int * indices, * ihead, *eoi; 
  unsigned sdflips;

  unsigned long long saved_flips;
  unsigned saved_max_var;
  unsigned min_flipped;

  void * emgr;
  picosat_malloc enew;
  picosat_realloc eresize;
  picosat_free edelete;

  struct {
    void * state;
    int (*function) (void *);
  } interrupt;

#ifdef VISCORES
  FILE * fviscores;
#endif
};

typedef PicoSAT PS;

static Flt
packflt (unsigned m, int e)
{
  Flt res;
  assert (m < FLTMSB);
  assert (FLTMINEXPONENT <= e);
  assert (e <= FLTMAXEXPONENT);
  res = m | ((unsigned)(e + 128) << 24);
  return res;
}

static Flt
base2flt (unsigned m, int e)
{
  if (!m)
    return ZEROFLT;

  if (m < FLTMSB)
    {
      do
	{
	  if (e <= FLTMINEXPONENT)
	    return EPSFLT;

	  e--;
	  m <<= 1;

	}
      while (m < FLTMSB);
    }
  else
    {
      while (m >= FLTCARRY)
	{
	  if (e >= FLTMAXEXPONENT)
	    return INFFLT;

	  e++;
	  m >>= 1;
	}
    }

  m &= ~FLTMSB;
  return packflt (m, e);
}

static Flt
addflt (Flt a, Flt b)
{
  unsigned ma, mb, delta;
  int ea, eb;

  CMPSWAPFLT (a, b);
  if (!b)
    return a;

  UNPACKFLT (a, ma, ea);
  UNPACKFLT (b, mb, eb);

  assert (ea >= eb);
  delta = ea - eb;
  if (delta < 32) mb >>= delta; else mb = 0;
  if (!mb)
    return a;

  ma += mb;
  if (ma & FLTCARRY)
    {
      if (ea == FLTMAXEXPONENT)
	return INFFLT;

      ea++;
      ma >>= 1;
    }

  assert (ma < FLTCARRY);
  ma &= FLTMAXMANTISSA;

  return packflt (ma, ea);
}

static Flt
mulflt (Flt a, Flt b)
{
  unsigned ma, mb;
  unsigned long long accu;
  int ea, eb;

  CMPSWAPFLT (a, b);
  if (!b)
    return ZEROFLT;

  UNPACKFLT (a, ma, ea);
  UNPACKFLT (b, mb, eb);

  ea += eb;
  ea += 24;
  if (ea > FLTMAXEXPONENT)
    return INFFLT;

  if (ea < FLTMINEXPONENT)
    return EPSFLT;

  accu = ma;
  accu *= mb;
  accu >>= 24;

  if (accu >= FLTCARRY)
    {
      if (ea == FLTMAXEXPONENT)
	return INFFLT;

      ea++;
      accu >>= 1;

      if (accu >= FLTCARRY)
	return INFFLT;
    }

  assert (accu < FLTCARRY);
  assert (accu & FLTMSB);

  ma = accu;
  ma &= ~FLTMSB;

  return packflt (ma, ea);
}

static Flt
ascii2flt (const char *str)
{
  Flt ten = base2flt (10, 0);
  Flt onetenth = base2flt (26843546, -28);
  Flt res = ZEROFLT, tmp, base;
  const char *p = str;
  int ch;

  ch = *p++;

  if (ch != '.')
    {
      if (!isdigit (ch))
	return INFFLT;	/* better abort ? */

      res = base2flt (ch - '0', 0);

      while ((ch = *p++))
	{
	  if (ch == '.')
	    break;

	  if (!isdigit (ch))
	    return INFFLT;	/* better abort? */

	  res = mulflt (res, ten);
	  tmp = base2flt (ch - '0', 0);
	  res = addflt (res, tmp);
	}
    }

  if (ch == '.')
    {
      ch = *p++;
      if (!isdigit (ch))
	return INFFLT;	/* better abort ? */

      base = onetenth;
      tmp = mulflt (base2flt (ch - '0', 0), base);
      res = addflt (res, tmp);

      while ((ch = *p++))
	{
	  if (!isdigit (ch))
	    return INFFLT;	/* better abort? */

	  base = mulflt (base, onetenth);
	  tmp = mulflt (base2flt (ch - '0', 0), base);
	  res = addflt (res, tmp);
	}
    }

  return res;
}

#if defined(VISCORES)

static double
flt2double (Flt f)
{
  double res;
  unsigned m;
  int e, i;

  UNPACKFLT (f, m, e);
  res = m;

  if (e < 0)
    {
      for (i = e; i < 0; i++)
	res *= 0.5;
    }
  else
    {
      for (i = 0; i < e; i++)
	res *= 2.0;
    }

  return res;
}

#endif

static int
log2flt (Flt a)
{
  return FLTEXPONENT (a) + 24;
}

static int
cmpflt (Flt a, Flt b)
{
  if (a < b)
    return -1;

  if (a > b)
    return 1;

  return 0;
}

static void *
new (PS * ps, size_t size)
{
  size_t bytes;
  Blk *b;
  
  if (!size)
    return 0;

  bytes = size + SIZE_OF_BLK;

  if (ps->enew)
    b = ps->enew (ps->emgr, bytes);
  else
    b = malloc (bytes);

  ABORTIF (!b, "out of memory in 'new'");
#ifndef NDEBUG
  b->header.size = size;
#endif
  ps->current_bytes += size;
  if (ps->current_bytes > ps->max_bytes)
    ps->max_bytes = ps->current_bytes;
  return b->data;
}

static void
delete (PS * ps, void *void_ptr, size_t size)
{
  size_t bytes;
  Blk *b;

  if (!void_ptr)
    {
      assert (!size);
      return;
    }

  assert (size);
  b = PTR2BLK (void_ptr);

  assert (size <= ps->current_bytes);
  ps->current_bytes -= size;

  assert (b->header.size == size);

  bytes = size + SIZE_OF_BLK;
  if (ps->edelete)
    ps->edelete (ps->emgr, b, bytes);
  else
    free (b);
}

static void *
resize (PS * ps, void *void_ptr, size_t old_size, size_t new_size)
{
  size_t old_bytes, new_bytes;
  Blk *b;

  b = PTR2BLK (void_ptr);

  assert (old_size <= ps->current_bytes);
  ps->current_bytes -= old_size;

  if ((old_bytes = old_size))
    {
      assert (old_size && b && b->header.size == old_size);
      old_bytes += SIZE_OF_BLK;
    }
  else
    assert (!b);

  if ((new_bytes = new_size))
    new_bytes += SIZE_OF_BLK;

  if (ps->eresize)
    b = ps->eresize (ps->emgr, b, old_bytes, new_bytes);
  else
    b = realloc (b, new_bytes);

  if (!new_size)
    {
      assert (!b);
      return 0;
    }

  ABORTIF (!b, "out of memory in 'resize'");
#ifndef NDEBUG
  b->header.size = new_size;
#endif

  ps->current_bytes += new_size;
  if (ps->current_bytes > ps->max_bytes)
    ps->max_bytes = ps->current_bytes;

  return b->data;
}

static unsigned
int2unsigned (int l)
{
  return (l < 0) ? 1 + 2 * -l : 2 * l;
}

static Lit *
int2lit (PS * ps, int l)
{
  return ps->lits + int2unsigned (l);
}

static Lit **
end_of_lits (Cls * c)
{
  return (Lit**)c->lits + c->size;
}

#if !defined(NDEBUG) || defined(LOGGING)

static void
dumplits (PS * ps, Lit ** l, Lit ** end)
{
  int first;
  Lit ** p;

  if (l == end)
    {
      /* empty clause */
    }
  else if (l + 1 == end)
    {
      fprintf (ps->out, "%d ", LIT2INT (l[0]));
    }
  else
    { 
      assert (l + 2 <= end);
      first = (abs (LIT2INT (l[0])) > abs (LIT2INT (l[1])));
      fprintf (ps->out, "%d ", LIT2INT (l[first]));
      fprintf (ps->out, "%d ", LIT2INT (l[!first]));
      for (p = l + 2; p < end; p++)
	 fprintf (ps->out, "%d ", LIT2INT (*p));
    }

  fputc ('0', ps->out);
}

static void
dumpcls (PS * ps, Cls * c)
{
  Lit **end;

  if (c)
    {
      end = end_of_lits (c);
      dumplits (ps, c->lits, end);
#ifdef TRACE
      if (ps->trace)
	 fprintf (ps->out, " clause(%u)", CLS2IDX (c));
#endif
    }
  else
    fputs ("DECISION", ps->out);
}

static void
dumpclsnl (PS * ps, Cls * c)
{
  dumpcls (ps, c);
  fputc ('\n', ps->out);
}

void
dumpcnf (PS * ps)
{
  Cls **p, *c;

  for (p = SOC; p != EOC; p = NXC (p))
    {
      c = *p;

      if (!c)
	continue;

#ifdef TRACE
      if (c->collected)
	continue;
#endif

      dumpclsnl (ps, *p);
    }
}

#endif

static void
delete_prefix (PS * ps)
{
  if (!ps->prefix)
    return;
    
  delete (ps, ps->prefix, strlen (ps->prefix) + 1);
  ps->prefix = 0;
}

static void
new_prefix (PS * ps, const char * str)
{
  delete_prefix (ps);
  assert (str);
  ps->prefix = new (ps, strlen (str) + 1);
  strcpy (ps->prefix, str);
}

static PS *
init (void * pmgr, 
      picosat_malloc pnew, picosat_realloc presize, picosat_free pdelete)
{
  PS * ps;

#if 0
  int count = 3 - !pnew - !presize - !pdelete;

  ABORTIF (count && !pnew, "API usage: missing 'picosat_set_new'");
  ABORTIF (count && !presize, "API usage: missing 'picosat_set_resize'");
  ABORTIF (count && !pdelete, "API usage: missing 'picosat_set_delete'");
#endif

  ps = pnew ? pnew (pmgr, sizeof *ps) : malloc (sizeof *ps);
  ABORTIF (!ps, "failed to allocate memory for PicoSAT manager");
  memset (ps, 0, sizeof *ps);

  ps->emgr = pmgr;
  ps->enew = pnew;
  ps->eresize = presize;
  ps->edelete = pdelete;

  ps->size_vars = 1;
  ps->state = RESET;
  ps->defaultphase = JWLPHASE;
#ifdef TRACE
  ps->ocore = -1;
#endif
  ps->lastrheader = -2;
#ifndef NADC
  ps->adoconflictlimit = UINT_MAX;
#endif
  ps->min_flipped = UINT_MAX;

  NEWN (ps->lits, 2 * ps->size_vars);
  NEWN (ps->jwh, 2 * ps->size_vars);
  NEWN (ps->htps, 2 * ps->size_vars);
#ifndef NDSC
  NEWN (ps->dhtps, 2 * ps->size_vars);
#endif
  NEWN (ps->impls, 2 * ps->size_vars);
  NEWN (ps->vars, ps->size_vars);
  NEWN (ps->rnks, ps->size_vars);

  /* because '0' pos denotes not on heap
   */
  ENLARGE (ps->heap, ps->hhead, ps->eoh); 
  ps->hhead = ps->heap + 1;

  ps->vinc = base2flt (1, 0);		/* initial var activity */
  ps->ifvinc = ascii2flt ("1.05");	/* var score rescore factor */
#ifdef VISCORES
  ps->fvinc = ascii2flt ("0.9523809");	/*     1/f =     1/1.05 */
  ps->nvinc = ascii2flt ("0.0476191");	/* 1 - 1/f = 1 - 1/1.05 */
#endif
  ps->lscore = base2flt (1, 90);	/* var activity rescore limit */
  ps->ilvinc = base2flt (1, -90);	/* inverse of 'lscore' */

  ps->cinc = base2flt (1, 0);		/* initial clause activity */
  ps->fcinc = ascii2flt ("1.001");	/* cls activity rescore factor */
  ps->lcinc = base2flt (1, 90);		/* cls activity rescore limit */
  ps->ilcinc = base2flt (1, -90);	/* inverse of 'ilcinc' */

  ps->lreduceadjustcnt = ps->lreduceadjustinc = 100;
  ps->lpropagations = ~0ull;

#ifndef RCODE
  ps->out = stdout;
#else
  ps->out = 0;
#endif
  new_prefix (ps, "c ");
  ps->verbosity = 0;
  ps->plain = 0;

#ifdef NO_BINARY_CLAUSES
  memset (&ps->impl, 0, sizeof (ps->impl));
  ps->impl.size = 2;

  memset (&ps->cimpl, 0, sizeof (ps->impl));
  ps->cimpl.size = 2;
#endif

#ifdef VISCORES
  ps->fviscores = popen (
    "/usr/bin/gnuplot -background black"
    " -xrm 'gnuplot*textColor:white'"
    " -xrm 'gnuplot*borderColor:white'"
    " -xrm 'gnuplot*axisColor:white'"
    , "w");
  fprintf (ps->fviscores, "unset key\n");
  // fprintf (ps->fviscores, "set log y\n");
  fflush (ps->fviscores);
  system ("rm -rf /tmp/picosat-viscores");
  system ("mkdir /tmp/picosat-viscores");
  system ("mkdir /tmp/picosat-viscores/data");
#ifdef WRITEGIF
  system ("mkdir /tmp/picosat-viscores/gif");
  fprintf (ps->fviscores,
           "set terminal gif giant animate opt size 1024,768 x000000 xffffff"
	   "\n");

  fprintf (ps->fviscores, 
           "set output \"/tmp/picosat-viscores/gif/animated.gif\"\n");
#endif
#endif
  ps->defaultphase = JWLPHASE;
  ps->state = READY;
  ps->last_sat_call_result = 0;

  return ps;
}

static size_t
bytes_clause (PS * ps, unsigned size, unsigned learned)
{
  size_t res;

  res = sizeof (Cls);
  res += size * sizeof (Lit *);
  res -= 2 * sizeof (Lit *);

  if (learned && size > 2)
    res += sizeof (Act);	/* add activity */

#ifdef TRACE
  if (ps->trace)
    res += sizeof (Trd);	/* add trace data */
#else
  (void) ps;
#endif

  return res;
}

static Cls *
new_clause (PS * ps, unsigned size, unsigned learned)
{
  size_t bytes;
  void * tmp;
#ifdef TRACE
  Trd *trd;
#endif
  Cls *res;

  bytes = bytes_clause (ps, size, learned);
  tmp = new (ps, bytes);

#ifdef TRACE
  if (ps->trace)
    {
      trd = tmp;

      if (learned)
	trd->idx = LIDX2IDX (ps->lhead - ps->lclauses);
      else
	trd->idx = OIDX2IDX (ps->ohead - ps->oclauses);

      res = trd->cls;
    }
  else
#endif
    res = tmp;

  res->size = size;
  res->learned = learned;

  res->collect = 0;
#ifndef NDEBUG
  res->connected = 0;
#endif
  res->locked = 0;
  res->used = 0;
#ifdef TRACE
  res->core = 0;
  res->collected = 0;
#endif

  if (learned && size > 2)
    {
      Act * p = CLS2ACT (res);
      *p = ps->cinc;
    }

  return res;
}

static void
delete_clause (PS * ps, Cls * c)
{
  size_t bytes;
#ifdef TRACE
  Trd *trd;
#endif

  bytes = bytes_clause (ps, c->size, c->learned);

#ifdef TRACE
  if (ps->trace)
    {
      trd = CLS2TRD (c);
      delete (ps, trd, bytes);
    }
  else
#endif
    delete (ps, c, bytes);
}

static void
delete_clauses (PS * ps)
{
  Cls **p;
  for (p = SOC; p != EOC; p = NXC (p))
    if (*p)
      delete_clause (ps, *p);

  DELETEN (ps->oclauses, ps->eoo - ps->oclauses);
  DELETEN (ps->lclauses, ps->EOL - ps->lclauses);

  ps->ohead = ps->eoo = ps->lhead = ps->EOL = 0;
}

#ifdef TRACE

static void
delete_zhain (PS * ps, Zhn * zhain)
{
  const Znt *p, *znt;

  assert (zhain);

  znt = zhain->znt;
  for (p = znt; *p; p++)
    ;

  delete (ps, zhain, sizeof (Zhn) + (p - znt) + 1);
}

static void
delete_zhains (PS * ps)
{
  Zhn **p, *z;
  for (p = ps->zhains; p < ps->zhead; p++)
    if ((z = *p))
      delete_zhain (ps, z);

  DELETEN (ps->zhains, ps->eoz - ps->zhains);
  ps->eoz = ps->zhead = 0;
}

#endif

#ifdef NO_BINARY_CLAUSES
static void
lrelease (PS * ps, Ltk * stk)
{
  if (stk->start)
    DELETEN (stk->start, (1 << (stk->ldsize)));
  memset (stk, 0, sizeof (*stk));
}
#endif

#ifndef NADC

static unsigned
llength (Lit ** a)
{
  Lit ** p;
  for (p = a; *p; p++)
    ;
  return p - a;
}

static void
resetadoconflict (PS * ps)
{
  assert (ps->adoconflict);
  delete_clause (ps, ps->adoconflict);
  ps->adoconflict = 0;
}

static void
reset_ados (PS * ps)
{
  Lit *** p;

  for (p = ps->ados; p < ps->hados; p++)
    DELETEN (*p, llength (*p) + 1);

  DELETEN (ps->ados, ps->eados - ps->ados);
  ps->hados = ps->eados = 0;

  DELETEN (ps->adotab, ps->szadotab);
  ps->szadotab = ps->nadotab = 0;

  if (ps->adoconflict)
    resetadoconflict (ps);

  ps->adoconflicts = 0;
  ps->adoconflictlimit = UINT_MAX;
  ps->adodisabled = 0;
}

#endif

static void
reset (PS * ps)
{
  ABORTIF (!ps || 
           ps->state == RESET, "API usage: reset without initialization");

  delete_clauses (ps);
#ifdef TRACE
  delete_zhains (ps);
#endif
#ifdef NO_BINARY_CLAUSES
  {
    unsigned i;
    for (i = 2; i <= 2 * ps->max_var + 1; i++)
      lrelease (ps, ps->impls + i);
  }
#endif
#ifndef NADC
  reset_ados (ps);
#endif
#ifndef NFL
  DELETEN (ps->saved, ps->saved_size);
#endif
  DELETEN (ps->htps, 2 * ps->size_vars);
#ifndef NDSC
  DELETEN (ps->dhtps, 2 * ps->size_vars);
#endif
  DELETEN (ps->impls, 2 * ps->size_vars);
  DELETEN (ps->lits, 2 * ps->size_vars);
  DELETEN (ps->jwh, 2 * ps->size_vars);
  DELETEN (ps->vars, ps->size_vars);
  DELETEN (ps->rnks, ps->size_vars);
  DELETEN (ps->trail, ps->eot - ps->trail);
  DELETEN (ps->heap, ps->eoh - ps->heap);
  DELETEN (ps->als, ps->eoals - ps->als);
  DELETEN (ps->CLS, ps->eocls - ps->CLS);
  DELETEN (ps->rils, ps->eorils - ps->rils);
  DELETEN (ps->cils, ps->eocils - ps->cils);
  DELETEN (ps->fals, ps->eofals - ps->fals);
  DELETEN (ps->mass, ps->szmass);
  DELETEN (ps->mssass, ps->szmssass);
  DELETEN (ps->mcsass, ps->szmcsass);
  DELETEN (ps->humus, ps->szhumus);
  DELETEN (ps->added, ps->eoa - ps->added);
  DELETEN (ps->marked, ps->eom - ps->marked);
  DELETEN (ps->dfs, ps->eod - ps->dfs);
  DELETEN (ps->resolved, ps->eor - ps->resolved);
  DELETEN (ps->levels, ps->eolevels - ps->levels);
  DELETEN (ps->dused, ps->eodused - ps->dused);
  DELETEN (ps->buffer, ps->eob - ps->buffer);
  DELETEN (ps->indices, ps->eoi - ps->indices);
  DELETEN (ps->soclauses, ps->eoso - ps->soclauses);
  delete_prefix (ps);
  delete (ps, ps->rline[0], ps->szrline);
  delete (ps, ps->rline[1], ps->szrline);
  assert (getenv ("LEAK") || !ps->current_bytes);	/* found leak if failing */
#ifdef VISCORES
  pclose (ps->fviscores);
#endif
  if (ps->edelete)
    ps->edelete (ps->emgr, ps, sizeof *ps);
  else
    free (ps);
}

inline static void
tpush (PS * ps, Lit * lit)
{
  assert (ps->lits < lit && lit <= ps->lits + 2* ps->max_var + 1);
  if (ps->thead == ps->eot)
    {
      unsigned ttail2count = ps->ttail2 - ps->trail;
      unsigned ttailcount = ps->ttail - ps->trail;
#ifndef NADC
      unsigned ttailadocount = ps->ttailado - ps->trail;
#endif
      ENLARGE (ps->trail, ps->thead, ps->eot);
      ps->ttail = ps->trail + ttailcount;
      ps->ttail2 = ps->trail + ttail2count;
#ifndef NADC
      ps->ttailado = ps->trail + ttailadocount;
#endif
    }

  *ps->thead++ = lit;
}

static void
assign_reason (PS * ps, Var * v, Cls * reason)
{
#if defined(NO_BINARY_CLAUSES) && !defined(NDEBUG)
  assert (reason != &ps->impl);
#else
  (void) ps;
#endif
  v->reason = reason;
}

static void
assign_phase (PS * ps, Lit * lit)
{
  unsigned new_phase, idx;
  Var * v = LIT2VAR (lit);

#ifndef NFL
  /* In 'simplifying' mode we only need to keep 'min_flipped' up to date if
   * we force assignments on the top level.   The other assignments will be
   * undone and thus we can keep the old saved value of the phase.
   */
  if (!ps->LEVEL || !ps->simplifying)
#endif
    {
      new_phase = (LIT2SGN (lit) > 0);

      if (v->assigned)
	{
	  ps->sdflips -= ps->sdflips/FFLIPPED;

	  if (new_phase != v->phase)
	    {
	      assert (FFLIPPEDPREC >= FFLIPPED);
	      ps->sdflips += FFLIPPEDPREC / FFLIPPED;
	      ps->flips++;

	      idx = LIT2IDX (lit);
	      if (idx < ps->min_flipped)
		ps->min_flipped = idx;

              NOLOG (fprintf (ps->out, 
	                      "%sflipped %d\n",
			       ps->prefix, LIT2INT (lit)));
	    }
	}

      v->phase = new_phase;
      v->assigned = 1;
    }

  lit->val = TRUE;
  NOTLIT (lit)->val = FALSE;
}

inline static void
assign (PS * ps, Lit * lit, Cls * reason)
{
  Var * v = LIT2VAR (lit);
  assert (lit->val == UNDEF);
#ifdef STATS
  ps->assignments++;
#endif
  v->level = ps->LEVEL;
  assign_phase (ps, lit);
  assign_reason (ps, v, reason);
  tpush (ps, lit);
}

inline static int
cmp_added (PS * ps, Lit * k, Lit * l)
{
  Val a = k->val, b = l->val;
  Var *u, *v;
  int res;

  if (a == UNDEF && b != UNDEF)
    return -1;

  if (a != UNDEF && b == UNDEF)
    return 1;

  u = LIT2VAR (k);
  v = LIT2VAR (l);

  if (a != UNDEF)
    {
      assert (b != UNDEF);
      res = v->level - u->level;
      if (res)
	return res;		/* larger level first */
    }

  res = cmpflt (VAR2RNK (u)->score, VAR2RNK (v)->score);
  if (res)
    return res;			/* smaller activity first */

  return u - v;			/* smaller index first */
}

static void
sorttwolits (Lit ** v)
{
  Lit * a = v[0], * b = v[1];

  assert (a != b);

  if (a < b)
    return;

  v[0] = b;
  v[1] = a;
}

inline static void
sortlits (PS * ps, Lit ** v, unsigned size)
{
  if (size == 2)
    sorttwolits (v);	/* same order with and with out 'NO_BINARY_CLAUSES' */
  else
    SORT (Lit *, cmp_added, v, size);
}

#ifdef NO_BINARY_CLAUSES
static Cls *
setimpl (PS * ps, Lit * a, Lit * b)
{
  assert (!ps->implvalid);
  assert (ps->impl.size == 2);

  ps->impl.lits[0] = a;
  ps->impl.lits[1] = b;

  sorttwolits (ps->impl.lits);
  ps->implvalid = 1;

  return &ps->impl;
}

static void
resetimpl (PS * ps)
{
  ps->implvalid = 0;
}

static Cls *
setcimpl (PS * ps, Lit * a, Lit * b)
{
  assert (!ps->cimplvalid);
  assert (ps->cimpl.size == 2);

  ps->cimpl.lits[0] = a;
  ps->cimpl.lits[1] = b;

  sorttwolits (ps->cimpl.lits);
  ps->cimplvalid = 1;

  return &ps->cimpl;
}

static void
resetcimpl (PS * ps)
{
  assert (ps->cimplvalid);
  ps->cimplvalid = 0;
}

#endif

static int
cmp_ptr (PS * ps, void *l, void *k)
{
  (void) ps;
  return ((char*)l) - (char*)k;		/* arbitrarily already reverse */
}

static int
cmp_rnk (Rnk * r, Rnk * s)
{
  if (!r->moreimportant && s->moreimportant)
    return -1;

  if (r->moreimportant && !s->moreimportant)
    return 1;

  if (!r->lessimportant && s->lessimportant)
    return 1;

  if (r->lessimportant && !s->lessimportant)
    return -1;

  if (r->score < s->score)
    return -1;

  if (r->score > s->score)
    return 1;

  return -cmp_ptr (0, r, s);
}

static void
hup (PS * ps, Rnk * v)
{
  int upos, vpos;
  Rnk *u;

#ifndef NFL
  assert (!ps->simplifying);
#endif

  vpos = v->pos;

  assert (0 < vpos);
  assert (vpos < ps->hhead - ps->heap);
  assert (ps->heap[vpos] == v);

  while (vpos > 1)
    {
      upos = vpos / 2;

      u = ps->heap[upos];

      if (cmp_rnk (u, v) > 0)
	break;

      ps->heap[vpos] = u;
      u->pos = vpos;

      vpos = upos;
    }

  ps->heap[vpos] = v;
  v->pos = vpos;
}

static Cls *add_simplified_clause (PS *, int);

inline static void
add_antecedent (PS * ps, Cls * c)
{
  assert (c);

#ifdef NO_BINARY_CLAUSES
  if (ISLITREASON (c))
    return;

  if (c == &ps->impl)
    return;
#elif defined(STATS) && defined(TRACE)
  ps->antecedents++;
#endif
  if (ps->rhead == ps->eor)
    ENLARGE (ps->resolved, ps->rhead, ps->eor);

  assert (ps->rhead < ps->eor);
  *ps->rhead++ = c;
}

#ifdef TRACE

#ifdef NO_BINARY_CLAUSES
#error "can not combine TRACE and NO_BINARY_CLAUSES"
#endif

#endif /* TRACE */

static void
add_lit (PS * ps, Lit * lit)
{
  assert (lit);

  if (ps->ahead == ps->eoa)
    ENLARGE (ps->added, ps->ahead, ps->eoa);

  *ps->ahead++ = lit;
}

static void
push_var_as_marked (PS * ps, Var * v)
{
  if (ps->mhead == ps->eom)
    ENLARGE (ps->marked, ps->mhead, ps->eom);

  *ps->mhead++ = v;
}

static void
mark_var (PS * ps, Var * v)
{
  assert (!v->mark);
  v->mark = 1;
  push_var_as_marked (ps, v);
}

#ifdef NO_BINARY_CLAUSES

static Cls *
impl2reason (PS * ps, Lit * lit)
{
  Lit * other;
  Cls * res;
  other = ps->impl.lits[0];
  if (lit == other)
    other = ps->impl.lits[1];
  assert (other->val == FALSE);
  res = LIT2REASON (NOTLIT (other));
  resetimpl (ps);
  return res;
}

#endif

/* Whenever we have a top level derived unit we really should derive a unit
 * clause otherwise the resolutions in 'add_simplified_clause' become
 * incorrect.
 */
static Cls *
resolve_top_level_unit (PS * ps, Lit * lit, Cls * reason)
{
  unsigned count_resolved;
  Lit **p, **eol, *other;
  Var *u, *v;

  assert (ps->rhead == ps->resolved);
  assert (ps->ahead == ps->added);

  add_lit (ps, lit);
  add_antecedent (ps, reason);
  count_resolved = 1;
  v = LIT2VAR (lit);

  eol = end_of_lits (reason);
  for (p = reason->lits; p < eol; p++)
    {
      other = *p;
      u = LIT2VAR (other);
      if (u == v)
	continue;

      add_antecedent (ps, u->reason);
      count_resolved++;
    }

  /* Some of the literals could be assumptions.  If at least one
   * variable is not an assumption, we should resolve.
   */
  if (count_resolved >= 2)
    {
#ifdef NO_BINARY_CLAUSES
      if (reason == &ps->impl)
	resetimpl (ps);
#endif
      reason = add_simplified_clause (ps, 1);
#ifdef NO_BINARY_CLAUSES
      if (reason->size == 2)
	{
	  assert (reason == &ps->impl);
	  reason = impl2reason (ps, lit);
	}
#endif
      assign_reason (ps, v, reason);
    }
  else
    {
      ps->ahead = ps->added;
      ps->rhead = ps->resolved;
    }

  return reason;
}

static void
fixvar (PS * ps, Var * v)
{
  Rnk * r;

  assert (VAR2LIT (v) != UNDEF);
  assert (!v->level);

  ps->fixed++;

  r = VAR2RNK (v);
  r->score = INFFLT;

#ifndef NFL
  if (ps->simplifying)
    return;
#endif

  if (!r->pos)
    return;

  hup (ps, r);
}

static void
use_var (PS * ps, Var * v)
{
  if (v->used)
    return;

  v->used = 1;
  ps->vused++;
}

static void
assign_forced (PS * ps, Lit * lit, Cls * reason)
{
  Var *v;

  assert (reason);
  assert (lit->val == UNDEF);

#ifdef STATS
  ps->FORCED++;
#endif
  assign (ps, lit, reason);

#ifdef NO_BINARY_CLAUSES
  assert (reason != &ps->impl);
  if (ISLITREASON (reason)) 
    {
      reason = setimpl (ps, lit, NOTLIT (REASON2LIT (reason)));
      assert (reason);
    }
#endif
  LOG ( fprintf (ps->out,
                "%sassign %d at level %d by ",
                ps->prefix, LIT2INT (lit), ps->LEVEL);
       dumpclsnl (ps, reason));

  v = LIT2VAR (lit);
  if (!ps->LEVEL)
    use_var (ps, v);

  if (!ps->LEVEL && reason->size > 1)
    {
      reason = resolve_top_level_unit (ps, lit, reason);
      assert (reason);
    }

#ifdef NO_BINARY_CLAUSES
  if (ISLITREASON (reason) || reason == &ps->impl)
    {
      /* DO NOTHING */
    }
  else
#endif
    {
      assert (!reason->locked);
      reason->locked = 1;
      if (reason->learned && reason->size > 2)
	ps->llocked++;
    }

#ifdef NO_BINARY_CLAUSES
  if (reason == &ps->impl)
    resetimpl (ps);
#endif

  if (!ps->LEVEL)
    fixvar (ps, v);
}

#ifdef NO_BINARY_CLAUSES

static void
lpush (PS * ps, Lit * lit, Cls * c)
{
  int pos = (c->lits[0] == lit);
  Ltk * s = LIT2IMPLS (lit);
  unsigned oldsize, newsize;

  assert (c->size == 2);

  if (!s->start)
    {
      assert (!s->count);
      assert (!s->ldsize);
      NEWN (s->start, 1);
    }
  else 
    {
      oldsize = (1 << (s->ldsize));
      assert (s->count <= oldsize);
      if (s->count == oldsize)
	{
	  newsize = 2 * oldsize;
	  RESIZEN (s->start, oldsize, newsize);
	  s->ldsize++;
	}
    }

  s->start[s->count++] = c->lits[pos];
}

#endif

static void
connect_head_tail (PS * ps, Lit * lit, Cls * c)
{
  Cls ** s;
  assert (c->size >= 1);
  if (c->size == 2)
    {
#ifdef NO_BINARY_CLAUSES
      lpush (ps, lit, c);
      return;
#else
      s = LIT2IMPLS (lit);
#endif
    }
  else
    s = LIT2HTPS (lit);

  if (c->lits[0] != lit)
    {
      assert (c->size >= 2);
      assert (c->lits[1] == lit);
      c->next[1] = *s;
    }
  else
    c->next[0] = *s;

  *s = c;
}

#ifdef TRACE
static void
zpush (PS * ps, Zhn * zhain)
{
  assert (ps->trace);

  if (ps->zhead == ps->eoz)
    ENLARGE (ps->zhains, ps->zhead, ps->eoz);

  *ps->zhead++ = zhain;
}

static int
cmp_resolved (PS * ps, Cls * c, Cls * d)
{
#ifndef NDEBUG
  assert (ps->trace);
#else
  (void) ps;
#endif
  return CLS2IDX (c) - CLS2IDX (d);
}

static void
bpushc (PS * ps, unsigned char ch)
{
  if (ps->bhead == ps->eob)
    ENLARGE (ps->buffer, ps->bhead, ps->eob);

  *ps->bhead++ = ch;
}

static void
bpushu (PS * ps, unsigned u)
{
  while (u & ~0x7f)
    {
      bpushc (ps, u | 0x80);
      u >>= 7;
    }

  bpushc (ps, u);
}

static void
bpushd (PS * ps, unsigned prev, unsigned this)
{
  unsigned delta;
  assert (prev < this);
  delta = this - prev;
  bpushu (ps, delta);
}

static void
add_zhain (PS * ps)
{
  unsigned prev, this, count, rcount;
  Cls **p, *c;
  Zhn *res;

  assert (ps->trace);
  assert (ps->bhead == ps->buffer);
  assert (ps->rhead > ps->resolved);

  rcount = ps->rhead - ps->resolved;
  SORT (Cls *, cmp_resolved, ps->resolved, rcount);

  prev = 0;
  for (p = ps->resolved; p < ps->rhead; p++)
    {
      c = *p;
      this = CLS2TRD (c)->idx;
      bpushd (ps, prev, this);
      prev = this;
    }
  bpushc (ps, 0);

  count = ps->bhead - ps->buffer;

  res = new (ps, sizeof (Zhn) + count);
  res->core = 0;
  res->ref = 0;
  memcpy (res->znt, ps->buffer, count);

  ps->bhead = ps->buffer;
#ifdef STATS
  ps->znts += count - 1;
#endif
  zpush (ps, res);
}

#endif

static void
add_resolved (PS * ps, int learned)
{
#if defined(STATS) || defined(TRACE)
  Cls **p, *c;

  for (p = ps->resolved; p < ps->rhead; p++)
    {
      c = *p;
      if (c->used)
	continue;

      c->used = 1;

      if (c->size <= 2)
	continue;

#ifdef STATS
      if (c->learned)
	ps->llused++;
      else
	ps->loused++;
#endif
    }
#endif

#ifdef TRACE
  if (learned && ps->trace)
    add_zhain (ps);
#else
  (void) learned;
#endif
  ps->rhead = ps->resolved;
}

static void
incjwh (PS * ps, Cls * c)
{
  Lit **p, *lit, ** eol;
  Flt * f, inc, sum;
  unsigned size = 0;
  Var * v;
  Val val;

  eol = end_of_lits (c);

  for (p = c->lits; p < eol; p++)
    {
      lit = *p;
      val = lit->val;

      if (val && ps->LEVEL > 0)
	{
	  v = LIT2VAR (lit);
	  if (v->level > 0)
	    val = UNDEF;
	}

      if (val == TRUE)
	return;

      if (val != FALSE)
	size++;
    }

  inc = base2flt (1, -size);

  for (p = c->lits; p < eol; p++)
    {
      lit = *p;
      f = LIT2JWH (lit);
      sum = addflt (*f, inc);
      *f = sum;
    }
}

static void
write_rup_header (PS * ps, FILE * file)
{
  char line[80];
  int i;

  sprintf (line, "%%RUPD32 %u %u", ps->rupvariables, ps->rupclauses);

  fputs (line, file);
  for (i = 255 - strlen (line); i >= 0; i--)
    fputc (' ', file);

  fputc ('\n', file);
  fflush (file);
}

static Cls *
add_simplified_clause (PS * ps, int learned)
{
  unsigned num_true, num_undef, num_false, size, count_resolved;
  Lit **p, **q, *lit, ** end;
  unsigned litlevel, glue;
  Cls *res, * reason;
  int reentered;
  Val val;
  Var *v;
#if !defined(NDEBUG) && defined(TRACE)
  unsigned idx;
#endif

  reentered = 0;

REENTER:

  size = ps->ahead - ps->added;

  add_resolved (ps, learned);

  if (learned)
    {
      ps->ladded++;
      ps->llitsadded += size;
      if (size > 2)
	{
	  ps->lladded++;
	  ps->nlclauses++;
	  ps->llits += size;
	}
    }
  else
    {
      ps->oadded++;
      if (size > 2)
	{
	  ps->loadded++;
	  ps->noclauses++;
	  ps->olits += size;
	}
    }

  ps->addedclauses++;
  assert (ps->addedclauses == ps->ladded + ps->oadded);

#ifdef NO_BINARY_CLAUSES
  if (size == 2)
    res = setimpl (ps, ps->added[0], ps->added[1]);
  else
#endif
    {
      sortlits (ps, ps->added, size); 

      if (learned)
	{
	  if (ps->lhead == ps->EOL)
	    {
	      ENLARGE (ps->lclauses, ps->lhead, ps->EOL);

	      /* A very difficult to find bug, which only occurs if the
	       * learned clauses stack is immediately allocated before the
	       * original clauses stack without padding.  In this case, we
	       * have 'SOC == EOC', which terminates all loops using the
	       * idiom 'for (p = SOC; p != EOC; p = NXC(p))' immediately.
	       * Unfortunately this occurred in 'fix_clause_lits' after
	       * using a recent version of the memory allocator of 'Google'
	       * perftools in the context of one large benchmark for 
	       * our SMT solver 'Boolector'.
	       */
	      if (ps->EOL == ps->oclauses)
		ENLARGE (ps->lclauses, ps->lhead, ps->EOL);
	    }

#if !defined(NDEBUG) && defined(TRACE)
	  idx = LIDX2IDX (ps->lhead - ps->lclauses);
#endif
	}
      else
	{
	  if (ps->ohead == ps->eoo)
	    {
	      ENLARGE (ps->oclauses, ps->ohead, ps->eoo);
	      if (ps->EOL == ps->oclauses)
		ENLARGE (ps->oclauses, ps->ohead, ps->eoo);	/* ditto */
	    }

#if !defined(NDEBUG) && defined(TRACE)
	  idx = OIDX2IDX (ps->ohead - ps->oclauses);
#endif
	}

      assert (ps->EOL != ps->oclauses);			/* ditto */

      res = new_clause (ps, size, learned);

      glue = 0;

      if (learned)
	{
	  assert (ps->dusedhead == ps->dused);

	  for (p = ps->added; p < ps->ahead; p++) 
	    {
	      lit = *p;
	      if (lit->val)
		{
		  litlevel = LIT2VAR (lit)->level;
		  assert (litlevel <= ps->LEVEL);
		  while (ps->levels + litlevel >= ps->levelshead)
		    {
		      if (ps->levelshead >= ps->eolevels)
			ENLARGE (ps->levels, ps->levelshead, ps->eolevels);
		      assert (ps->levelshead < ps->eolevels);
		      *ps->levelshead++ = 0;
		    }
		  if (!ps->levels[litlevel])
		    {
		      if (ps->dusedhead >= ps->eodused)
			ENLARGE (ps->dused, ps->dusedhead, ps->eodused);
		      assert (ps->dusedhead < ps->eodused);
		      *ps->dusedhead++ = litlevel;
		      ps->levels[litlevel] = 1;
		      glue++;
		    }
		}
	      else
		glue++;
	    }

	  while (ps->dusedhead > ps->dused) 
	    {
	      litlevel = *--ps->dusedhead;
	      assert (ps->levels + litlevel < ps->levelshead);
	      assert (ps->levels[litlevel]);
	      ps->levels[litlevel] = 0;
	    }
	}

      assert (glue <= MAXGLUE);
      res->glue = glue;

#if !defined(NDEBUG) && defined(TRACE)
      if (ps->trace)
	assert (CLS2IDX (res) == idx);
#endif
      if (learned)
	*ps->lhead++ = res;
      else
	*ps->ohead++ = res;

#if !defined(NDEBUG) && defined(TRACE)
      if (ps->trace && learned)
	assert (ps->zhead - ps->zhains == ps->lhead - ps->lclauses);
#endif
      assert (ps->lhead != ps->oclauses);		/* ditto */
    }

  if (learned && ps->rup)
    {
      if (!ps->rupstarted)
	{
	  write_rup_header (ps, ps->rup);
	  ps->rupstarted = 1;
	}
    }

  num_true = num_undef = num_false = 0;

  q = res->lits;
  for (p = ps->added; p < ps->ahead; p++)
    {
      lit = *p;
      *q++ = lit;

      if (learned && ps->rup)
	fprintf (ps->rup, "%d ", LIT2INT (lit));

      val = lit->val;

      num_true += (val == TRUE);
      num_undef += (val == UNDEF);
      num_false += (val == FALSE);
    }
  assert (num_false + num_true + num_undef == size);

  if (learned && ps->rup)
    fputs ("0\n", ps->rup);

  ps->ahead = ps->added;		/* reset */

  if (!reentered)				// TODO merge
  if (size > 0)
    {
      assert (size <= 2 || !reentered);		// TODO remove
      connect_head_tail (ps, res->lits[0], res);
      if (size > 1)
	connect_head_tail (ps, res->lits[1], res);
    }

  if (size == 0)
    {
      if (!ps->mtcls)
	ps->mtcls = res;
    }

#ifdef NO_BINARY_CLAUSES
  if (size != 2)
#endif
#ifndef NDEBUG
    res->connected = 1;
#endif

  LOG ( fprintf (ps->out, "%s%s ", ps->prefix, learned ? "learned" : "original");
        dumpclsnl (ps, res));

  /* Shrink clause by resolving it against top level assignments.
   */
  if (!ps->LEVEL && num_false > 0)
    {
      assert (ps->ahead == ps->added);
      assert (ps->rhead == ps->resolved);

      count_resolved = 1;
      add_antecedent (ps, res);

      end = end_of_lits (res);
      for (p = res->lits; p < end; p++)
	{
	  lit = *p;
	  v = LIT2VAR (lit);
	  use_var (ps, v);

	  if (lit->val == FALSE)
	    {
	      add_antecedent (ps, v->reason);
	      count_resolved++;
	    }
	  else
	    add_lit (ps, lit);
	}

      assert (count_resolved >= 2);

      learned = 1;
#ifdef NO_BINARY_CLAUSES
      if (res == &ps->impl)
	resetimpl (ps);
#endif
      reentered = 1;
      goto REENTER;		/* and return simplified clause */
    }

  if (!num_true && num_undef == 1)	/* unit clause */
    {
      lit = 0;
      for (p = res->lits; p < res->lits + size; p++)
	{
	  if ((*p)->val == UNDEF)
	    lit = *p;

	  v = LIT2VAR (*p);
	  use_var (ps, v);
	}
      assert (lit);

      reason = res;
#ifdef NO_BINARY_CLAUSES
      if (size == 2)
        {
	  Lit * other = res->lits[0];
	  if (other == lit)
	    other = res->lits[1];

	  assert (other->val == FALSE);
	  reason = LIT2REASON (NOTLIT (other));
	}
#endif
      assign_forced (ps, lit, reason);
      num_true++;
    }

  if (num_false == size && !ps->conflict)
    {
#ifdef NO_BINARY_CLAUSES
      if (res == &ps->impl)
	ps->conflict = setcimpl (ps, res->lits[0], res->lits[1]);
      else
#endif
      ps->conflict = res;
    }

  if (!learned && !num_true && num_undef)
    incjwh (ps, res);

#ifdef NO_BINARY_CLAUSES
  if (res == &ps->impl)
    resetimpl (ps);
#endif
  return res;
}

static int
trivial_clause (PS * ps)
{
  Lit **p, **q, *prev;
  Var *v;

  SORT (Lit *, cmp_ptr, ps->added,  ps->ahead - ps->added);

  prev = 0;
  q = ps->added;
  for (p = q; p < ps->ahead; p++)
    {
      Lit *this = *p;

      v = LIT2VAR (this);

      if (prev == this)		/* skip repeated literals */
	continue;

      /* Top level satisfied ? 
       */
      if (this->val == TRUE && !v->level)
	 return 1;

      if (prev == NOTLIT (this))/* found pair of dual literals */
	return 1;

      *q++ = prev = this;
    }

  ps->ahead = q;			/* shrink */

  return 0;
}

static void
simplify_and_add_original_clause (PS * ps)
{
#ifdef NO_BINARY_CLAUSES
  Cls * c;
#endif
  if (trivial_clause (ps))
    {
      ps->ahead = ps->added;

      if (ps->ohead == ps->eoo)
	ENLARGE (ps->oclauses, ps->ohead, ps->eoo);

      *ps->ohead++ = 0;

      ps->addedclauses++;
      ps->oadded++;
    }
  else
    {
      if (ps->CLS != ps->clshead)
	add_lit (ps, NOTLIT (ps->clshead[-1]));

#ifdef NO_BINARY_CLAUSES
      c = 
#endif
      add_simplified_clause (ps, 0);
#ifdef NO_BINARY_CLAUSES
      if (c == &ps->impl) assert (!ps->implvalid);
#endif
    }
}

#ifndef NADC

static void
add_ado (PS * ps)
{
  unsigned len = ps->ahead - ps->added;
  Lit ** ado, ** p, ** q, *lit;
  Var * v, * u;

#ifdef TRACE
  assert (!ps->trace);
#endif

  ABORTIF (ps->ados < ps->hados && llength (ps->ados[0]) != len,
           "internal: non matching all different constraint object lengths");

  if (ps->hados == ps->eados)
    ENLARGE (ps->ados, ps->hados, ps->eados);

  NEWN (ado, len + 1);
  *ps->hados++ = ado;

  p = ps->added;
  q = ado;
  u = 0;
  while (p < ps->ahead)
    {
      lit = *p++;
      v = LIT2VAR (lit);
      ABORTIF (v->inado, 
               "internal: variable in multiple all different objects");
      v->inado = ado;
      if (!u && !lit->val)
	u = v;
      *q++ = lit;
    }

  assert (q == ado + len);
  *q++ = 0;

  /* TODO simply do a conflict test as in propado */

  ABORTIF (!u,
    "internal: "
    "adding fully instantiated all different object not implemented yet");

  assert (u);
  assert (u->inado == ado);
  assert (!u->ado);
  u->ado = ado;

  ps->ahead = ps->added;
}

#endif

static void
hdown (PS * ps, Rnk * r)
{
  unsigned end, rpos, cpos, opos;
  Rnk *child, *other;

  assert (r->pos > 0);
  assert (ps->heap[r->pos] == r);

  end = ps->hhead - ps->heap;
  rpos = r->pos;

  for (;;)
    {
      cpos = 2 * rpos;
      if (cpos >= end)
	break;

      opos = cpos + 1;
      child = ps->heap[cpos];

      if (cmp_rnk (r, child) < 0)
	{
	  if (opos < end)
	    {
	      other = ps->heap[opos];

	      if (cmp_rnk (child, other) < 0)
		{
		  child = other;
		  cpos = opos;
		}
	    }
	}
      else if (opos < end)
	{
	  child = ps->heap[opos];

	  if (cmp_rnk (r, child) >= 0)
	    break;

	  cpos = opos;
	}
      else
	break;

      ps->heap[rpos] = child;
      child->pos = rpos;
      rpos = cpos;
    }

  r->pos = rpos;
  ps->heap[rpos] = r;
}

static Rnk *
htop (PS * ps)
{
  assert (ps->hhead > ps->heap + 1);
  return ps->heap[1];
}

static Rnk *
hpop (PS * ps)
{
  Rnk *res, *last;
  unsigned end;

  assert (ps->hhead > ps->heap + 1);

  res = ps->heap[1];
  res->pos = 0;

  end = --ps->hhead - ps->heap;
  if (end == 1)
    return res;

  last = ps->heap[end];

  ps->heap[last->pos = 1] = last;
  hdown (ps, last);

  return res;
}

inline static void
hpush (PS * ps, Rnk * r)
{
  assert (!r->pos);

  if (ps->hhead == ps->eoh)
    ENLARGE (ps->heap, ps->hhead, ps->eoh);

  r->pos = ps->hhead++ - ps->heap;
  ps->heap[r->pos] = r;
  hup (ps, r);
}

static void
fix_trail_lits (PS * ps, long delta)
{
  Lit **p;
  for (p = ps->trail; p < ps->thead; p++)
    *p += delta;
}

#ifdef NO_BINARY_CLAUSES
static void
fix_impl_lits (PS * ps, long delta)
{
  Ltk * s;
  Lit ** p;

  for (s = ps->impls + 2; s <= ps->impls + 2 * ps->max_var + 1; s++)
    for (p = s->start; p < s->start + s->count; p++)
      *p += delta;
}
#endif

static void
fix_clause_lits (PS * ps, long delta)
{
  Cls **p, *clause;
  Lit **q, *lit, **eol;

  for (p = SOC; p != EOC; p = NXC (p))
    {
      clause = *p;
      if (!clause)
	continue;

      q = clause->lits;
      eol = end_of_lits (clause);
      while (q < eol)
	{
	  assert (q - clause->lits <= (int) clause->size);
	  lit = *q;
	  lit += delta;
	  *q++ = lit;
	}
    }
}

static void
fix_added_lits (PS * ps, long delta)
{
  Lit **p;
  for (p = ps->added; p < ps->ahead; p++)
    *p += delta;
}

static void
fix_assumed_lits (PS * ps, long delta)
{
  Lit **p;
  for (p = ps->als; p < ps->alshead; p++)
    *p += delta;
}

static void
fix_cls_lits (PS * ps, long delta)
{
  Lit **p;
  for (p = ps->CLS; p < ps->clshead; p++)
    *p += delta;
}

static void
fix_heap_rnks (PS * ps, long delta)
{
  Rnk **p;

  for (p = ps->heap + 1; p < ps->hhead; p++)
    *p += delta;
}

#ifndef NADC

static void
fix_ado (long delta, Lit ** ado)
{
  Lit ** p;
  for (p = ado; *p; p++)
    *p += delta;
}

static void
fix_ados (PS * ps, long delta)
{
  Lit *** p;

  for (p = ps->ados; p < ps->hados; p++)
    fix_ado (delta, *p);
}

#endif

static void
enlarge (PS * ps, unsigned new_size_vars)
{
  long rnks_delta, lits_delta;
  Lit *old_lits = ps->lits;
  Rnk *old_rnks = ps->rnks;

  RESIZEN (ps->lits, 2 * ps->size_vars, 2 * new_size_vars);
  RESIZEN (ps->jwh, 2 * ps->size_vars, 2 * new_size_vars);
  RESIZEN (ps->htps, 2 * ps->size_vars, 2 * new_size_vars);
#ifndef NDSC
  RESIZEN (ps->dhtps, 2 * ps->size_vars, 2 * new_size_vars);
#endif
  RESIZEN (ps->impls, 2 * ps->size_vars, 2 * new_size_vars);
  RESIZEN (ps->vars, ps->size_vars, new_size_vars);
  RESIZEN (ps->rnks, ps->size_vars, new_size_vars);

  if ((lits_delta = ps->lits - old_lits))
    {
      fix_trail_lits (ps, lits_delta);
      fix_clause_lits (ps, lits_delta);
      fix_added_lits (ps, lits_delta);
      fix_assumed_lits (ps, lits_delta);
      fix_cls_lits (ps, lits_delta);
#ifdef NO_BINARY_CLAUSES
      fix_impl_lits (ps, lits_delta);
#endif
#ifndef NADC
      fix_ados (ps, lits_delta);
#endif
    }

  if ((rnks_delta = ps->rnks - old_rnks))
    {
      fix_heap_rnks (ps, rnks_delta);
    }

  assert (ps->mhead == ps->marked);

  ps->size_vars = new_size_vars;
}

static void
unassign (PS * ps, Lit * lit)
{
  Cls *reason;
  Var *v;
  Rnk *r;

  assert (lit->val == TRUE);

  LOG ( fprintf (ps->out, "%sunassign %d\n", ps->prefix, LIT2INT (lit)));

  v = LIT2VAR (lit);
  reason = v->reason;

#ifdef NO_BINARY_CLAUSES
  assert (reason != &ps->impl);
  if (ISLITREASON (reason))
    {
      /* DO NOTHING */
    }
  else
#endif
  if (reason)
    {
      assert (reason->locked);
      reason->locked = 0;
      if (reason->learned && reason->size > 2)
	{
	  assert (ps->llocked > 0);
	  ps->llocked--;
	}
    }

  lit->val = UNDEF;
  NOTLIT (lit)->val = UNDEF;

  r = VAR2RNK (v);
  if (!r->pos)
    hpush (ps, r);

#ifndef NDSC
  {
    Cls * p, * next, ** q;

    q = LIT2DHTPS (lit);
    p = *q;
    *q = 0;

    while (p)
      {
	Lit * other = p->lits[0];

	if (other == lit)
	  {
	    other = p->lits[1];
	    q = p->next + 1;
	  }
	else
	  {
	    assert (p->lits[1] == lit);
	    q = p->next;
	  }

	next = *q;
	*q = *LIT2HTPS (other);
	*LIT2HTPS (other) = p;
	p = next;
      }
  }
#endif

#ifndef NADC
  if (v->adotabpos)
    {
      assert (ps->nadotab);
      assert (*v->adotabpos == v->ado);

      *v->adotabpos = 0;
      v->adotabpos = 0;

      ps->nadotab--;
    }
#endif
}

static Cls *
var2reason (PS * ps, Var * var)
{
  Cls * res = var->reason;
#ifdef NO_BINARY_CLAUSES
  Lit * this, * other;
  if (ISLITREASON (res))
    {
      this = VAR2LIT (var);
      if (this->val == FALSE)
	this = NOTLIT (this);

      other = REASON2LIT (res);
      assert (other->val == TRUE);
      assert (this->val == TRUE);
      res = setimpl (ps, NOTLIT (other), this);
    }
#else
  (void) ps;
#endif
  return res;
}

static void
mark_clause_to_be_collected (Cls * c)
{
  assert (!c->collect);
  c->collect = 1;
}

static void
undo (PS * ps, unsigned new_level)
{
  Lit *lit;
  Var *v;

  while (ps->thead > ps->trail)
    {
      lit = *--ps->thead;
      v = LIT2VAR (lit);
      if (v->level == new_level)
	{
	  ps->thead++;		/* fix pre decrement */
	  break;
	}

      unassign (ps, lit);
    }

  ps->LEVEL = new_level;
  ps->ttail = ps->thead;
  ps->ttail2 = ps->thead;
#ifndef NADC
  ps->ttailado = ps->thead;
#endif

#ifdef NO_BINARY_CLAUSES
  if (ps->conflict == &ps->cimpl)
    resetcimpl (ps);
#endif
#ifndef NADC
  if (ps->conflict && ps->conflict == ps->adoconflict)
    resetadoconflict (ps);
#endif
  ps->conflict = ps->mtcls;
  if (ps->LEVEL < ps->adecidelevel)
    {
      assert (ps->als < ps->alshead);
      ps->adecidelevel = 0;
      ps->alstail = ps->als;
    }
  LOG ( fprintf (ps->out, "%sback to level %u\n", ps->prefix, ps->LEVEL));
}

#ifndef NDEBUG

static int
clause_satisfied (Cls * c)
{
  Lit **p, **eol, *lit;

  eol = end_of_lits (c);
  for (p = c->lits; p < eol; p++)
    {
      lit = *p;
      if (lit->val == TRUE)
	return 1;
    }

  return 0;
}

static void
original_clauses_satisfied (PS * ps)
{
  Cls **p, *c;

  for (p = ps->oclauses; p < ps->ohead; p++)
    {
      c = *p;

      if (!c)
	continue;

      if (c->learned)
	continue;

      assert (clause_satisfied (c));
    }
}

static void
assumptions_satisfied (PS * ps)
{
  Lit *lit, ** p;

  for (p = ps->als; p < ps->alshead; p++)
    {
      lit = *p;
      assert (lit->val == TRUE);
    }
}

#endif

static void
sflush (PS * ps)
{
  double now = picosat_time_stamp ();
  double delta = now - ps->entered;
  delta = (delta < 0) ? 0 : delta;
  ps->seconds += delta;
  ps->entered = now;
}

static double
mb (PS * ps)
{
  return ps->current_bytes / (double) (1 << 20);
}

static double
avglevel (PS * ps)
{
  return ps->decisions ? ps->levelsum / ps->decisions : 0.0;
}

static void
rheader (PS * ps)
{
  assert (ps->lastrheader <= ps->reports);

  if (ps->lastrheader == ps->reports)
    return;

  ps->lastrheader = ps->reports;

   fprintf (ps->out, "%s\n", ps->prefix);
   fprintf (ps->out, "%s %s\n", ps->prefix, ps->rline[0]);
   fprintf (ps->out, "%s %s\n", ps->prefix, ps->rline[1]);
   fprintf (ps->out, "%s\n", ps->prefix);
}

static unsigned
dynamic_flips_per_assignment_per_mille (PS * ps)
{
  assert (FFLIPPEDPREC >= 1000);
  return ps->sdflips / (FFLIPPEDPREC / 1000);
}

#ifdef NLUBY

static int
high_agility (PS * ps)
{
  return dynamic_flips_per_assignment_per_mille (ps) >= 200;
}

static int
very_high_agility (PS * ps)
{
  return dynamic_flips_per_assignment_per_mille (ps) >= 250;
}

#else

static int
medium_agility (PS * ps)
{
  return dynamic_flips_per_assignment_per_mille (ps) >= 230;
}

#endif

static void
relemdata (PS * ps)
{
  char *p;
  int x;

  if (ps->reports < 0)
    {
      /* strip trailing white space 
       */
      for (x = 0; x <= 1; x++)
	{
	  p = ps->rline[x] + strlen (ps->rline[x]);
	  while (p-- > ps->rline[x])
	    {
	      if (*p != ' ')
		break;

	      *p = 0;
	    }
	}

      rheader (ps);
    }
  else
    fputc ('\n', ps->out);

  ps->RCOUNT = 0;
}

static void
relemhead (PS * ps, const char * name, int fp, double val)
{
  int x, y, len, size;
  const char *fmt;
  unsigned tmp, e;

  if (ps->reports < 0)
    {
      x = ps->RCOUNT & 1;
      y = (ps->RCOUNT / 2) * 12 + x * 6;

      if (ps->RCOUNT == 1)
	sprintf (ps->rline[1], "%6s", "");

      len = strlen (name);
      while (ps->szrline <= len + y + 1)
	{
	  size = ps->szrline ? 2 * ps->szrline : 128;
	  ps->rline[0] = resize (ps, ps->rline[0], ps->szrline, size);
	  ps->rline[1] = resize (ps, ps->rline[1], ps->szrline, size);
	  ps->szrline = size;
	}

      fmt = (len <= 6) ? "%6s%10s" : "%-10s%4s";
      sprintf (ps->rline[x] + y, fmt, name, "");
    }
  else if (val < 0)
    {
      assert (fp);

      if (val > -100 && (tmp = val * 10.0 - 0.5) > -1000.0)
	{
	   fprintf (ps->out, "-%4.1f ", -tmp / 10.0);
	}
      else
	{
	  tmp = -val / 10.0 + 0.5;
	  e = 1;
	  while (tmp >= 100)
	    {
	      tmp /= 10;
	      e++;
	    }

	   fprintf (ps->out, "-%2ue%u ", tmp, e);
	}
    }
  else
    {
      if (fp && val < 1000 && (tmp = val * 10.0 + 0.5) < 10000)
	{
	   fprintf (ps->out, "%5.1f ", tmp / 10.0);
	}
      else if (!fp && (tmp = val) < 100000)
	{
	   fprintf (ps->out, "%5u ", tmp);
	}
      else
	{
	  tmp = val / 10.0 + 0.5;
	  e = 1;

	  while (tmp >= 1000)
	    {
	      tmp /= 10;
	      e++;
	    }

	   fprintf (ps->out, "%3ue%u ", tmp, e);
	}
    }

  ps->RCOUNT++;
}

inline static void
relem (PS * ps, const char *name, int fp, double val)
{
  if (name)
    relemhead (ps, name, fp, val);
  else
    relemdata (ps);
}

static unsigned
reduce_limit_on_lclauses (PS * ps)
{
  unsigned res = ps->lreduce;
  res += ps->llocked;
  return res;
}

static void
report (PS * ps, int replevel, char type)
{
  int rounds;

#ifdef RCODE
  (void) type;
#endif

  if (ps->verbosity < replevel)
    return;

  sflush (ps);

  if (!ps->reports)
    ps->reports = -1;

  for (rounds = (ps->reports < 0) ? 2 : 1; rounds; rounds--)
    {
      if (ps->reports >= 0)
	 fprintf (ps->out, "%s%c ", ps->prefix, type);

      relem (ps, "seconds", 1, ps->seconds);
      relem (ps, "level", 1, avglevel (ps));
      assert (ps->fixed <=  ps->max_var);
      relem (ps, "variables", 0, ps->max_var - ps->fixed);
      relem (ps, "used", 1, PERCENT (ps->vused, ps->max_var));
      relem (ps, "original", 0, ps->noclauses);
      relem (ps, "conflicts", 0, ps->conflicts);
      // relem (ps, "decisions", 0, ps->decisions);
      // relem (ps, "conf/dec", 1, PERCENT(ps->conflicts,ps->decisions));
      // relem (ps, "limit", 0, reduce_limit_on_lclauses (ps));
      relem (ps, "learned", 0, ps->nlclauses);
      // relem (ps, "limit", 1, PERCENT (ps->nlclauses, reduce_limit_on_lclauses (ps)));
      relem (ps, "limit", 0, ps->lreduce);
#ifdef STATS
      relem (ps, "learning", 1, PERCENT (ps->llused, ps->lladded));
#endif
      relem (ps, "agility", 1, dynamic_flips_per_assignment_per_mille (ps) / 10.0);
      // relem (ps, "original", 0, ps->noclauses);
      relem (ps, "MB", 1, mb (ps));
      // relem (ps, "lladded", 0, ps->lladded);
      // relem (ps, "llused", 0, ps->llused);

      relem (ps, 0, 0, 0);

      ps->reports++;
    }

  /* Adapt this to the number of rows in your terminal.
   */
  #define ROWS 25

  if (ps->reports % (ROWS - 3) == (ROWS - 4))
    rheader (ps);

  fflush (ps->out);
}

static int
bcp_queue_is_empty (PS * ps)
{
  if (ps->ttail != ps->thead)
    return 0;

  if (ps->ttail2 != ps->thead)
    return 0;

#ifndef NADC
  if (ps->ttailado != ps->thead)
    return 0;
#endif

  return 1;
}

static int
satisfied (PS * ps)
{
  assert (!ps->mtcls);
  assert (!ps->failed_assumption);
  if (ps->alstail < ps->alshead)
    return 0;
  assert (!ps->conflict);
  assert (bcp_queue_is_empty (ps));
  return ps->thead == ps->trail + ps->max_var;	/* all assigned */
}

static void
vrescore (PS * ps)
{
  Rnk *p, *eor = ps->rnks + ps->max_var;
  for (p = ps->rnks + 1; p <= eor; p++)
    if (p->score != INFFLT)
      p->score = mulflt (p->score, ps->ilvinc);
  ps->vinc = mulflt (ps->vinc, ps->ilvinc);;
#ifdef VISCORES
  ps->nvinc = mulflt (ps->nvinc, ps->lscore);;
#endif
}

static void
inc_score (PS * ps, Var * v)
{
  Flt score;
  Rnk *r;

#ifndef NFL
  if (ps->simplifying)
    return;
#endif

  if (!v->level)
    return;

  if (v->internal)
    return;

  r = VAR2RNK (v);
  score = r->score;

  assert (score != INFFLT);

  score = addflt (score, ps->vinc);
  assert (score < INFFLT);
  r->score = score;
  if (r->pos > 0)
    hup (ps, r);

  if (score > ps->lscore)
    vrescore (ps);
}

static void
inc_activity (PS * ps, Cls * c)
{
  Act *p;

  if (!c->learned)
    return;

  if (c->size <= 2)
    return;

  p = CLS2ACT (c);
  *p = addflt (*p, ps->cinc);
}

static unsigned
hashlevel (unsigned l)
{
  return 1u << (l & 31);
}

static void
push (PS * ps, Var * v)
{
  if (ps->dhead == ps->eod)
    ENLARGE (ps->dfs, ps->dhead, ps->eod);

  *ps->dhead++ = v;
}

static Var * 
pop (PS * ps)
{
  assert (ps->dfs < ps->dhead);
  return *--ps->dhead;
}

static void
analyze (PS * ps)
{
  unsigned open, minlevel, siglevels, l, old, i, orig;
  Lit *this, *other, **p, **q, **eol;
  Var *v, *u, **m, *start, *uip;
  Cls *c;

  assert (ps->conflict);

  assert (ps->ahead == ps->added);
  assert (ps->mhead == ps->marked);
  assert (ps->rhead == ps->resolved);

  /* First, search for First UIP variable and mark all resolved variables.
   * At the same time determine the minimum decision level involved.
   * Increase activities of resolved variables.
   */
  q = ps->thead;
  open = 0;
  minlevel = ps->LEVEL;
  siglevels = 0;
  uip = 0;

  c = ps->conflict;

  for (;;)
    {
      add_antecedent (ps, c);
      inc_activity (ps, c);
      eol = end_of_lits (c);
      for (p = c->lits; p < eol; p++)
	{
	  other = *p;

	  if (other->val == TRUE)
	    continue;

	  assert (other->val == FALSE);

	  u = LIT2VAR (other);
	  if (u->mark)
	    continue;
	  
	  u->mark = 1;
	  inc_score (ps, u);
	  use_var (ps, u);

	  if (u->level == ps->LEVEL)
	    {
	      open++;
	    }
	  else 
	    {
	      push_var_as_marked (ps, u);

	      if (u->level)
		{
		  /* The statistics counter 'nonminimizedllits' sums up the
		   * number of literals that would be added if only the
		   * 'first UIP' scheme for learned clauses would be used
		   * and no clause minimization.
		   */
		  ps->nonminimizedllits++;

		  if (u->level < minlevel)
		    minlevel = u->level;

		  siglevels |= hashlevel (u->level);
		}
	      else
		{
		  assert (!u->level);
		  assert (u->reason);
		}
	    }
	}

      do
	{
	  if (q == ps->trail)
	    {
	      uip = 0;
	      goto DONE_FIRST_UIP;
	    }

	  this = *--q;
	  uip = LIT2VAR (this);
	}
      while (!uip->mark);

      uip->mark = 0;

      c = var2reason (ps, uip);
#ifdef NO_BINARY_CLAUSES
      if (c == &ps->impl)
	resetimpl (ps);
#endif
     open--;
     if ((!open && ps->LEVEL) || !c)
	break;

     assert (c);
    }

DONE_FIRST_UIP:

  if (uip)
    {
      assert (ps->LEVEL);
      this = VAR2LIT (uip);
      this += (this->val == TRUE);
      ps->nonminimizedllits++;
      ps->minimizedllits++;
      add_lit (ps, this);
#ifdef STATS
      if (uip->reason)
	ps->uips++;
#endif
    }
  else
    assert (!ps->LEVEL);

  /* Second, try to mark more intermediate variables, with the goal to
   * minimize the conflict clause.  This is a DFS from already marked
   * variables backward through the implication graph.  It tries to reach
   * other marked variables.  If the search reaches an unmarked decision
   * variable or a variable assigned below the minimum level of variables in
   * the first uip learned clause or a level on which no variable has been
   * marked, then the variable from which the DFS is started is not
   * redundant.  Otherwise the start variable is redundant and will
   * eventually be removed from the learned clause in step 4.  We initially
   * implemented BFS, but then profiling revelead that this step is a bottle
   * neck for certain incremental applications.  After switching to DFS this
   * hot spot went away.
   */
  orig = ps->mhead - ps->marked;
  for (i = 0; i < orig; i++)
    {
      start = ps->marked[i];

      assert (start->mark);
      assert (start != uip);
      assert (start->level < ps->LEVEL);

      if (!start->reason)
	continue;

      old = ps->mhead - ps->marked;
      assert (ps->dhead == ps->dfs);
      push (ps, start);

      while (ps->dhead > ps->dfs)
	{
	  u = pop (ps);
	  assert (u->mark);

	  c = var2reason (ps, u);
#ifdef NO_BINARY_CLAUSES
	  if (c == &ps->impl)
	    resetimpl (ps);
#endif
	  if (!c || 
	      ((l = u->level) && 
	       (l < minlevel || ((hashlevel (l) & ~siglevels)))))
	    {
	      while (ps->mhead > ps->marked + old)	/* reset all marked */
		(*--ps->mhead)->mark = 0;

	      ps->dhead = ps->dfs;		/* and DFS stack */
	      break;
	    }

	  eol = end_of_lits (c);
	  for (p = c->lits; p < eol; p++)
	    {
	      v = LIT2VAR (*p);
	      if (v->mark)
		continue;

	      mark_var (ps, v);
	      push (ps, v);
	    }
	}
    }

  for (m = ps->marked; m < ps->mhead; m++)
    {
      v = *m;

      assert (v->mark);
      assert (!v->resolved);

      use_var (ps, v);

      c = var2reason (ps, v);
      if (!c)
	continue;

#ifdef NO_BINARY_CLAUSES
      if (c == &ps->impl)
	resetimpl (ps);
#endif
      eol = end_of_lits (c);
      for (p = c->lits; p < eol; p++)
	{
	  other = *p;

	  u = LIT2VAR (other);
	  if (!u->level)
	    continue;

	  if (!u->mark)		/* 'MARKTEST' */
	    break;
	}

      if (p != eol)
	continue;

      add_antecedent (ps, c);
      v->resolved = 1;
    }

  for (m = ps->marked; m < ps->mhead; m++)
    {
      v = *m;

      assert (v->mark);
      v->mark = 0;

      if (v->resolved)
	{
	  v->resolved = 0;
	  continue;
	}

      this = VAR2LIT (v);
      if (this->val == TRUE)
	this++;			/* actually NOTLIT */

      add_lit (ps, this);
      ps->minimizedllits++;
    }

  assert (ps->ahead <= ps->eoa);
  assert (ps->rhead <= ps->eor);

  ps->mhead = ps->marked;
}

static void
fanalyze (PS * ps)
{
  Lit ** eol, ** p, * lit;
  Cls * c, * reason;
  Var * v, * u;
  int next;

#ifndef RCODE
  double start = picosat_time_stamp ();
#endif

  assert (ps->failed_assumption);
  assert (ps->failed_assumption->val == FALSE);

  v = LIT2VAR (ps->failed_assumption);
  reason = var2reason (ps, v);
  if (!reason) return;
#ifdef NO_BINARY_CLAUSES
  if (reason == &ps->impl)
    resetimpl (ps);
#endif

  eol = end_of_lits (reason);
  for (p = reason->lits; p != eol; p++)
    {
      lit = *p;
      u = LIT2VAR (lit);
      if (u == v) continue;
      if (u->reason) break;
    }
  if (p == eol) return;

  assert (ps->ahead == ps->added);
  assert (ps->mhead == ps->marked);
  assert (ps->rhead == ps->resolved);

  next = 0;
  mark_var (ps, v);
  add_lit (ps, NOTLIT (ps->failed_assumption));

  do
    {
      v = ps->marked[next++];
      use_var (ps, v);
      if (v->reason)
	{
	  reason = var2reason (ps, v);
#ifdef NO_BINARY_CLAUSES
	  if (reason == &ps->impl)
	    resetimpl (ps);
#endif
	  add_antecedent (ps, reason);
	  eol = end_of_lits (reason);
	  for (p = reason->lits; p != eol; p++)
	    {
	      lit = *p;
	      u = LIT2VAR (lit);
	      if (u == v) continue;
	      if (u->mark) continue;
	      mark_var (ps, u);
	    }
	}
      else
	{
	  lit = VAR2LIT (v);
	  if (lit->val == TRUE) lit = NOTLIT (lit);
	  add_lit (ps, lit);
	}
    } 
  while (ps->marked + next < ps->mhead);

  c = add_simplified_clause (ps, 1);
  v = LIT2VAR (ps->failed_assumption);
  reason = v->reason;
#ifdef NO_BINARY_CLAUSES
  if (!ISLITREASON (reason))
#endif
    {
      assert (reason->locked);
      reason->locked = 0;
      if (reason->learned && reason->size > 2)
	{
	  assert (ps->llocked > 0);
	  ps->llocked--;
	}
    }

#ifdef NO_BINARY_CLAUSES
  if (c == &ps->impl)
    {
      c = impl2reason (ps, NOTLIT (ps->failed_assumption));
    }
  else
#endif
    {
      assert (c->learned);
      assert (!c->locked);
      c->locked = 1;
      if (c->size > 2)
	{
	  ps->llocked++;
	  assert (ps->llocked > 0);
	}
    }

  v->reason = c;

  while (ps->mhead > ps->marked)
    (*--ps->mhead)->mark = 0;

  if (ps->verbosity)
     fprintf (ps->out, "%sfanalyze took %.1f seconds\n", 
	     ps->prefix, picosat_time_stamp () - start);
}

/* Propagate assignment of 'this' to 'FALSE' by visiting all binary clauses in
 * which 'this' occurs.
 */
inline static void
prop2 (PS * ps, Lit * this)
{
#ifdef NO_BINARY_CLAUSES
  Lit ** l, ** start;
  Ltk * lstk;
#else
  Cls * c, ** p;
  Cls * next;
#endif
  Lit * other;
  Val tmp;

  assert (this->val == FALSE);

#ifdef NO_BINARY_CLAUSES
  lstk = LIT2IMPLS (this);
  start = lstk->start;
  l = start + lstk->count;
  while (l != start)
    {
      /* The counter 'visits' is the number of clauses that are
       * visited during propagations of assignments.
       */
      ps->visits++;
#ifdef STATS
      ps->bvisits++;
#endif
      other = *--l;
      tmp = other->val;

      if (tmp == TRUE)
	{
#ifdef STATS
	  ps->othertrue++;
	  ps->othertrue2++;
	  if (LIT2VAR (other)->level < ps->LEVEL)
	    ps->othertrue2u++;
#endif
	  continue;
	}

      if (tmp != FALSE)
	{
	  assign_forced (ps, other, LIT2REASON (NOTLIT(this)));
	  continue;
	}

      if (ps->conflict == &ps->cimpl)
	resetcimpl (ps);
      ps->conflict = setcimpl (ps, this, other);
    }
#else
  /* Traverse all binary clauses with 'this'.  Head/Tail pointers for binary
   * clauses do not have to be modified here.
   */
  p = LIT2IMPLS (this);
  for (c = *p; c; c = next)
    {
      ps->visits++;
#ifdef STATS
      ps->bvisits++;
#endif
      assert (!c->collect);
#ifdef TRACE
      assert (!c->collected);
#endif
      assert (c->size == 2);
      
      other = c->lits[0];
      if (other == this)
	{
	  next = c->next[0];
	  other = c->lits[1];
	}
      else
	next = c->next[1];

      tmp = other->val;

      if (tmp == TRUE)
	{
#ifdef STATS
	  ps->othertrue++;
	  ps->othertrue2++;
	  if (LIT2VAR (other)->level < ps->LEVEL)
	    ps->othertrue2u++;
#endif
	  continue;
	}

      if (tmp == FALSE)
	ps->conflict = c;
      else
	assign_forced (ps, other, c);	/* unit clause */
    }
#endif /* !defined(NO_BINARY_CLAUSES) */
}

#ifndef NDSC
static int
should_disconnect_head_tail (PS * ps, Lit * lit)
{
  unsigned litlevel;
  Var * v;

  assert (lit->val == TRUE);

  v = LIT2VAR (lit);
  litlevel = v->level;

  if (!litlevel)
    return 1;

#ifndef NFL
  if (ps->simplifying)
    return 0;
#endif

  return litlevel < ps->LEVEL;
}
#endif

inline static void
propl (PS * ps, Lit * this)
{
  Lit **l, *other, *prev, *new_lit, **eol;
  Cls *next, **htp_ptr, **new_htp_ptr;
  Cls *c;
#ifdef STATS
  unsigned size;
#endif

  htp_ptr = LIT2HTPS (this);
  assert (this->val == FALSE);

  /* Traverse all non binary clauses with 'this'.  Head/Tail pointers are
   * updated as well.
   */
  for (c = *htp_ptr; c; c = next)
    {
      ps->visits++;
#ifdef STATS
      size = c->size;
      assert (size >= 3);
      ps->traversals++;	/* other is dereferenced at least */

      if (size == 3)
	ps->tvisits++;
      else if (size >= 4)
	{
	  ps->lvisits++;
	  ps->ltraversals++;
	}
#endif
#ifdef TRACE
      assert (!c->collected);
#endif
      assert (c->size > 0);

      other = c->lits[0];
      if (other != this)
	{
	  assert (c->size != 1);
	  c->lits[0] = this;
	  c->lits[1] = other;
	  next = c->next[1];
	  c->next[1] = c->next[0];
	  c->next[0] = next;
	}
      else if (c->size == 1)	/* With assumptions we need to
	                         * traverse unit clauses as well.
			         */
	{
	  assert (!ps->conflict);
	  ps->conflict = c;
	  break;
	}
      else
	{
	  assert (other == this && c->size > 1);
	  other = c->lits[1];
	  next = c->next[0];
	}
      assert (other == c->lits[1]);
      assert (this == c->lits[0]);
      assert (next == c->next[0]);
      assert (!c->collect);

      if (other->val == TRUE)
	{
#ifdef STATS
	  ps->othertrue++;
	  ps->othertruel++;
#endif
#ifndef NDSC
	  if (should_disconnect_head_tail (ps, other))
	    {
	      new_htp_ptr = LIT2DHTPS (other);
	      c->next[0] = *new_htp_ptr;
	      *new_htp_ptr = c;
#ifdef STATS
	      ps->othertruelu++;
#endif
	      *htp_ptr = next;
	      continue;
	    }
#endif
	  htp_ptr = c->next;
	  continue;
	}

      l = c->lits + 1;
      eol = (Lit**) c->lits + c->size;
      prev = this;

      while (++l != eol)
	{
#ifdef STATS
	  if (size >= 3)
	    {
	      ps->traversals++;
	      if (size > 3)
		ps->ltraversals++;
	    }
#endif
	  new_lit = *l;
	  *l = prev;
	  prev = new_lit;
	  if (new_lit->val != FALSE) break;
	}

      if (l == eol)
	{
	  while (l > c->lits + 2) 
	    {
	      new_lit = *--l;
	      *l = prev;
	      prev = new_lit;
	    }
	  assert (c->lits[0] == this);

	  assert (other == c->lits[1]);
	  if (other->val == FALSE)	/* found conflict */
	    {
	      assert (!ps->conflict);
	      ps->conflict = c;
	      return;
	    }

	  assign_forced (ps, other, c);		/* unit clause */
	  htp_ptr = c->next;
	}
      else
	{
	  assert (new_lit->val == TRUE || new_lit->val == UNDEF);
	  c->lits[0] = new_lit;
	  // *l = this;
	  new_htp_ptr = LIT2HTPS (new_lit);
	  c->next[0] = *new_htp_ptr;
	  *new_htp_ptr = c;
	  *htp_ptr = next;
	}
    }
}

#ifndef NADC

static unsigned primes[] = { 996293, 330643, 753947, 500873 };

#define PRIMES ((sizeof primes)/sizeof *primes)

static unsigned
hash_ado (PS * ps, Lit ** ado, unsigned salt)
{
  unsigned i, res, tmp;
  Lit ** p, * lit;

  assert (salt < PRIMES);

  i = salt;
  res = 0;

  for (p = ado; (lit = *p); p++)
    {
      assert (lit->val);

      tmp = res >> 31;
      res <<= 1;

      if (lit->val > 0)
	res |= 1;

      assert (i < PRIMES);
      res *= primes[i++];
      if (i == PRIMES)
	i = 0;

      res += tmp;
    }

  return res & (ps->szadotab - 1);
}

static unsigned
cmp_ado (Lit ** a, Lit ** b)
{
  Lit ** p, ** q, * l, * k;
  int res;

  for (p = a, q = b; (l = *p); p++, q++)
    {
      k = *q;
      assert (k);
      if ((res = (l->val - k->val)))
	return res;
    }

  assert (!*q);

  return 0;
}

static Lit ***
find_ado (PS * ps, Lit ** ado)
{
  Lit *** res, ** other;
  unsigned pos, delta;

  pos = hash_ado (ps, ado, 0);
  assert (pos < ps->szadotab);
  res = ps->adotab + pos;

  other = *res;
  if (!other || !cmp_ado (other, ado))
    return res;

  delta = hash_ado (ps, ado, 1);
  if (!(delta & 1))
    delta++;

  assert (delta & 1);
  assert (delta < ps->szadotab);

  for (;;)
    {
      pos += delta;
      if (pos >= ps->szadotab)
	pos -= ps->szadotab;

      assert (pos < ps->szadotab);
      res = ps->adotab + pos;
      other = *res;
      if (!other || !cmp_ado (other, ado))
	return res;
    }
}

static void
enlarge_adotab (PS * ps)
{
  /* TODO make this generic */

  ABORTIF (ps->szadotab, 
           "internal: all different objects table needs larger initial size");
  assert (!ps->nadotab);
  ps->szadotab = 10000;
  NEWN (ps->adotab, ps->szadotab);
  CLRN (ps->adotab, ps->szadotab);
}

static int
propado (PS * ps, Var * v)
{
  Lit ** p, ** q, *** adotabpos, **ado, * lit;
  Var * u;

  if (ps->LEVEL && ps->adodisabled)
    return 1;

  assert (!ps->conflict);
  assert (!ps->adoconflict);
  assert (VAR2LIT (v)->val != UNDEF);
  assert (!v->adotabpos);

  if (!v->ado)
    return 1;

  assert (v->inado);

  for (p = v->ado; (lit = *p); p++)
    if (lit->val == UNDEF)
      {
	u = LIT2VAR (lit);
	assert (!u->ado);
	u->ado = v->ado;
	v->ado = 0;

	return 1;
      }

  if (4 * ps->nadotab >= 3 * ps->szadotab)	/* at least 75% filled */
    enlarge_adotab (ps);

  adotabpos = find_ado (ps, v->ado);
  ado = *adotabpos;

  if (!ado)
    {
      ps->nadotab++;
      v->adotabpos = adotabpos;
      *adotabpos = v->ado;
      return 1;
    }

  assert (ado != v->ado);

  ps->adoconflict = new_clause (ps, 2 * llength (ado), 1);
  q = ps->adoconflict->lits;

  for (p = ado; (lit = *p); p++)
    *q++ = lit->val == FALSE ? lit : NOTLIT (lit);

  for (p = v->ado; (lit = *p); p++)
    *q++ = lit->val == FALSE ? lit : NOTLIT (lit);

  assert (q == ENDOFCLS (ps->adoconflict));
  ps->conflict = ps->adoconflict;
  ps->adoconflicts++;
  return 0;
}

#endif

static void
bcp (PS * ps)
{
  int props = 0;
  assert (!ps->conflict);

  if (ps->mtcls)
    return;

  for (;;)
    {
      if (ps->ttail2 < ps->thead)	/* prioritize implications */
	{
	  props++;
	  prop2 (ps, NOTLIT (*ps->ttail2++));
	}
      else if (ps->ttail < ps->thead)	/* unit clauses or clauses with length > 2 */
	{
	  if (ps->conflict) break;
	  propl (ps, NOTLIT (*ps->ttail++));
	  if (ps->conflict) break;
	}
#ifndef NADC
      else if (ps->ttailado < ps->thead)
	{
	  if (ps->conflict) break;
	  propado (ps, LIT2VAR (*ps->ttailado++));
	  if (ps->conflict) break;
	}
#endif
      else
	break;		/* all assignments propagated, so break */
    }

  ps->propagations += props;
}

static unsigned
drive (PS * ps)
{
  unsigned res, vlevel;
  Lit **p;
  Var *v;

  res = 0;
  for (p = ps->added; p < ps->ahead; p++)
    {
      v = LIT2VAR (*p);
      vlevel = v->level;
      assert (vlevel <= ps->LEVEL);
      if (vlevel < ps->LEVEL && vlevel > res)
	res = vlevel;
    }

  return res;
}

#ifdef VISCORES

static void
viscores (PS * ps)
{
  Rnk *p, *eor = ps->rnks + ps->max_var;
  char name[100], cmd[200];
  FILE * data;
  Flt s;
  int i;

  for (p = ps->rnks + 1; p <= ps->eor; p++)
    {
      s = p->score;
      if (s == INFFLT)
	continue;
      s = mulflt (s, ps->nvinc);
      assert (flt2double (s) <= 1.0);
    }

  sprintf (name, "/tmp/picosat-viscores/data/%08u", ps->conflicts);
  sprintf (cmd, "sort -n|nl>%s", name);

  data = popen (cmd, "w");
  for (p = ps->rnks + 1; p <= ps->eor; p++)
    {
      s = p->score;
      if (s == INFFLT)
	continue;
      s = mulflt (s, ps->nvinc);
      fprintf (data, "%lf %d\n", 100.0 * flt2double (s), (int)(p - ps->rnks));
    }
  fflush (data);
  pclose (data);

  for (i = 0; i < 8; i++)
    {
      sprintf (cmd, "awk '$3%%8==%d' %s>%s.%d", i, name, name, i);
      system (cmd);
    }

  fprintf (ps->fviscores, "set title \"%u\"\n", ps->conflicts);
  fprintf (ps->fviscores, "plot [0:%u] 0, 100 * (1 - 1/1.1), 100", ps->max_var);

  for (i = 0; i < 8; i++)
    fprintf (ps->fviscores, 
             ", \"%s.%d\" using 1:2:3 with labels tc lt %d", 
	     name, i, i + 1);

  fputc ('\n', ps->fviscores);
  fflush (ps->fviscores);
#ifndef WRITEGIF
  usleep (50000);		/* refresh rate of 20 Hz */
#endif
}

#endif

static void
crescore (PS * ps)
{
  Cls **p, *c;
  Act *a;
  Flt factor;
  int l = log2flt (ps->cinc);
  assert (l > 0);
  factor = base2flt (1, -l);

  for (p = ps->lclauses; p != ps->lhead; p++)
    {
      c = *p;

      if (!c)
	continue;

#ifdef TRACE
      if (c->collected)
	continue;
#endif
      assert (c->learned);

      if (c->size <= 2)
	continue;

      a = CLS2ACT (c);
      *a = mulflt (*a, factor);
    }

  ps->cinc = mulflt (ps->cinc, factor);
}

static void
inc_vinc (PS * ps)
{
#ifdef VISCORES
  ps->nvinc = mulflt (ps->nvinc, ps->fvinc);
#endif
  ps->vinc = mulflt (ps->vinc, ps->ifvinc);
}

inline static void
inc_max_var (PS * ps)
{
  Lit *lit;
  Rnk *r;
  Var *v;

  assert (ps->max_var < ps->size_vars);

  if (ps->max_var + 1 == ps->size_vars)
    enlarge (ps, ps->size_vars + 2*(ps->size_vars + 3) / 4); /* +25% */

  ps->max_var++;			/* new index of variable */
  assert (ps->max_var);			/* no unsigned overflow */

  assert (ps->max_var < ps->size_vars);

  lit = ps->lits + 2 * ps->max_var;
  lit[0].val = lit[1].val = UNDEF;

  memset (ps->htps + 2 * ps->max_var, 0, 2 * sizeof *ps->htps);
#ifndef NDSC
  memset (ps->dhtps + 2 * ps->max_var, 0, 2 * sizeof *ps->dhtps);
#endif
  memset (ps->impls + 2 * ps->max_var, 0, 2 * sizeof *ps->impls);
  memset (ps->jwh + 2 * ps->max_var, 0, 2 * sizeof *ps->jwh);

  v = ps->vars + ps->max_var;		/* initialize variable components */
  CLR (v);

  r = ps->rnks + ps->max_var;		/* initialize rank */
  CLR (r);

  hpush (ps, r);
}

static void
force (PS * ps, Cls * c)
{
  Lit ** p, ** eol, * lit, * forced;
  Cls * reason;

  forced = 0;
  reason = c;

  eol = end_of_lits (c);
  for (p = c->lits; p < eol; p++)
    {
      lit = *p;
      if (lit->val == UNDEF)
	{
	  assert (!forced);
	  forced = lit;
#ifdef NO_BINARY_CLAUSES
	  if (c == &ps->impl)
	    reason = LIT2REASON (NOTLIT (p[p == c->lits ? 1 : -1]));
#endif
	}
      else
	assert (lit->val == FALSE);
    }

#ifdef NO_BINARY_CLAUSES
  if (c == &ps->impl)
    resetimpl (ps);
#endif
  if (!forced)
    return;

  assign_forced (ps, forced, reason);
}

static void
inc_lreduce (PS * ps)
{
#ifdef STATS
  ps->inclreduces++;
#endif
  ps->lreduce *= FREDUCE;
  ps->lreduce /= 100;
  report (ps, 1, '+');
}

static void
backtrack (PS * ps)
{
  unsigned new_level;
  Cls * c;

  ps->conflicts++;
  LOG ( fprintf (ps->out, "%sconflict ", ps->prefix); dumpclsnl (ps, ps->conflict));

  analyze (ps);
  new_level = drive (ps);
  // TODO: why not? assert (new_level != 1  || (ps->ahead - ps->added) == 2);
  c = add_simplified_clause (ps, 1);
  undo (ps, new_level);
  force (ps, c);

  if (
#ifndef NFL
      !ps->simplifying && 
#endif
      !--ps->lreduceadjustcnt)
    {
      /* With FREDUCE==110 and FREDADJ=121 we stir 'lreduce' to be
       * proportional to 'sqrt(conflicts)'.  In earlier version we actually
       * used  'FREDADJ=150', which results in 'lreduce' to approximate
       * 'conflicts^(log(1.1)/log(1.5))' which is close to the fourth root
       * of 'conflicts', since log(1.1)/log(1.5)=0.235 (as observed by
       * Donald Knuth). The square root is the same we get by a Glucose
       * style increase, which simply adds a constant at every reduction.
       * This would be way simpler to implement but for now we keep the more
       * complicated code using the adjust increments and counters.
       */
      ps->lreduceadjustinc *= FREDADJ; ps->lreduceadjustinc /= 100; ps->lreduceadjustcnt
      = ps->lreduceadjustinc;
      inc_lreduce (ps);
    }

  if (ps->verbosity >= 4 && !(ps->conflicts % 1000))
    report (ps, 4, 'C');
}

static void
inc_cinc (PS * ps)
{
  ps->cinc = mulflt (ps->cinc, ps->fcinc);
  if (ps->lcinc < ps->cinc)
    crescore (ps);
}

static void
incincs (PS * ps)
{
  inc_vinc (ps);
  inc_cinc (ps);
#ifdef VISCORES
  viscores (ps);
#endif
}

static void
disconnect_clause (PS * ps, Cls * c)
{
  assert (c->connected);

  if (c->size > 2)
    {
      if (c->learned)
	{
	  assert (ps->nlclauses > 0);
	  ps->nlclauses--;

	  assert (ps->llits >= c->size);
	  ps->llits -= c->size;
	}
      else
	{
	  assert (ps->noclauses > 0);
	  ps->noclauses--;

	  assert (ps->olits >= c->size);
	  ps->olits -= c->size;
	}
    }

#ifndef NDEBUG
  c->connected = 0;
#endif
}

static int
clause_is_toplevel_satisfied (PS * ps, Cls * c)
{
  Lit *lit, **p, **eol = end_of_lits (c);
  Var *v;

  for (p = c->lits; p < eol; p++)
    {
      lit = *p;
      if (lit->val == TRUE)
	{
	  v = LIT2VAR (lit);
	  if (!v->level)
	    return 1;
	}
    }

  return 0;
}

static int
collect_clause (PS * ps, Cls * c)
{
  assert (c->collect);
  c->collect = 0;

#ifdef TRACE
  assert (!c->collected);
  c->collected = 1;
#endif
  disconnect_clause (ps, c);

#ifdef TRACE
  if (ps->trace && (!c->learned || c->used))
    return 0;
#endif
  delete_clause (ps, c);

  return 1;
}

static size_t
collect_clauses (PS * ps)
{
  Cls *c, **p, **q, * next;
  Lit * lit, * eol;
  size_t res;
  int i;

  res = ps->current_bytes;

  eol = ps->lits + 2 * ps->max_var + 1;
  for (lit = ps->lits + 2; lit <= eol; lit++)
    {
      for (i = 0; i <= 1; i++)
	{
	  if (i)
	    {
#ifdef NO_BINARY_CLAUSES
	      Ltk * lstk = LIT2IMPLS (lit);
	      Lit ** r, ** s;
	      r = lstk->start;
	      if (lit->val != TRUE || LIT2VAR (lit)->level)
		for (s = r; s < lstk->start + lstk->count; s++)
		  {
		    Lit * other = *s;
		    Var *v = LIT2VAR (other);
		    if (v->level ||
		        other->val != TRUE)
		      *r++ = other;
		  }
	      lstk->count = r - lstk->start;
	      continue;
#else
	      p = LIT2IMPLS (lit);
#endif
	    }
	  else
	    p = LIT2HTPS (lit);

	  for (c = *p; c; c = next)
	    {
	      q = c->next;
	      if (c->lits[0] != lit)
		q++;

	      next = *q;
	      if (c->collect)
		*p = next;
	      else
		p = q;
	    }
	}
    }

#ifndef NDSC
  for (lit = ps->lits + 2; lit <= eol; lit++)
    {
      p = LIT2DHTPS (lit); 
      while ((c = *p))
	{
	  Lit * other = c->lits[0];
	  if (other == lit)
	    {
	      q = c->next + 1;
	    }
	  else
	    {
	      assert (c->lits[1] == lit);
	      q = c->next;
	    }

	  if (c->collect)
	    *p = *q;
	  else
	    p = q;
	}
    }
#endif

  for (p = SOC; p != EOC; p = NXC (p))
    {
      c = *p;

      if (!c)
	continue;

      if (!c->collect)
	continue;

      if (collect_clause (ps, c))
	*p = 0;
    }

#ifdef TRACE
  if (!ps->trace)
#endif
    {
      q = ps->oclauses;
      for (p = q; p < ps->ohead; p++)
	if ((c = *p))
	  *q++ = c;
      ps->ohead = q;

      q = ps->lclauses;
      for (p = q; p < ps->lhead; p++)
	if ((c = *p))
	  *q++ = c;
      ps->lhead = q;
    }

  assert (ps->current_bytes <= res);
  res -= ps->current_bytes;
  ps->recycled += res;

  LOG ( fprintf (ps->out, "%scollected %ld bytes\n", ps->prefix, (long)res));

  return res;
}

static int
need_to_reduce (PS * ps)
{
  return ps->nlclauses >= reduce_limit_on_lclauses (ps);
}

#ifdef NLUBY

static void
inc_drestart (PS * ps)
{
  ps->drestart *= FRESTART;
  ps->drestart /= 100;

  if (ps->drestart >= MAXRESTART)
    ps->drestart = MAXRESTART;
}

static void
inc_ddrestart (PS * ps)
{
  ps->ddrestart *= FRESTART;
  ps->ddrestart /= 100;

  if (ps->ddrestart >= MAXRESTART)
    ps->ddrestart = MAXRESTART;
}

#else

static unsigned
luby (unsigned i)
{
  unsigned k;
  for (k = 1; k < 32; k++)
    if (i == (1u << k) - 1)
      return 1u << (k - 1);

  for (k = 1;; k++)
    if ((1u << (k - 1)) <= i && i < (1u << k) - 1)
      return luby (i - (1u << (k-1)) + 1);
}

#endif

#ifndef NLUBY
static void
inc_lrestart (PS * ps, int skip)
{
  unsigned delta;

  delta = 100 * luby (++ps->lubycnt);
  ps->lrestart = ps->conflicts + delta;

  if (ps->waslubymaxdelta)
    report (ps, 1, skip ? 'N' : 'R');
  else
    report (ps, 2, skip ? 'n' : 'r');

  if (delta > ps->lubymaxdelta)
    {
      ps->lubymaxdelta = delta;
      ps->waslubymaxdelta = 1;
    }
  else
    ps->waslubymaxdelta = 0;
}
#endif

static void
init_restart (PS * ps)
{
#ifdef NLUBY
  /* TODO: why is it better in incremental usage to have smaller initial
   * outer restart interval?
   */
  ps->ddrestart = ps->calls > 1 ? MINRESTART : 1000;
  ps->drestart = MINRESTART;
  ps->lrestart = ps->conflicts + ps->drestart;
#else
  ps->lubycnt = 0;
  ps->lubymaxdelta = 0;
  ps->waslubymaxdelta = 0;
  inc_lrestart (ps, 0);
#endif
}

static void
restart (PS * ps)
{
  int skip; 
#ifdef NLUBY
  char kind;
  int outer;
 
  inc_drestart (ps);
  outer = (ps->drestart >= ps->ddrestart);

  if (outer)
    skip = very_high_agility (ps);
  else
    skip = high_agility (ps);
#else
  skip = medium_agility (ps);
#endif

#ifdef STATS
  if (skip)
    ps->skippedrestarts++;
#endif

  assert (ps->conflicts >= ps->lrestart);

  if (!skip)
    {
      ps->restarts++;
      assert (ps->LEVEL > 1);
      LOG ( fprintf (ps->out, "%srestart %u\n", ps->prefix, ps->restarts));
      undo (ps, 0);
    }

#ifdef NLUBY
  if (outer)
    {
      kind = skip ? 'N' : 'R';
      inc_ddrestart (ps);
      ps->drestart = MINRESTART;
    }
  else  if (skip)
    {
      kind = 'n';
    }
  else
    {
      kind = 'r';
    }

  assert (ps->drestart <= MAXRESTART);
  ps->lrestart = ps->conflicts + ps->drestart;
  assert (ps->lrestart > ps->conflicts);

  report (outer ? 1 : 2, kind);
#else
  inc_lrestart (ps, skip);
#endif
}

inline static void
assign_decision (PS * ps, Lit * lit)
{
  assert (!ps->conflict);

  ps->LEVEL++;

  LOG ( fprintf (ps->out, "%snew level %u\n", ps->prefix, ps->LEVEL));
  LOG ( fprintf (ps->out,
		 "%sassign %d at level %d <= DECISION\n",
		 ps->prefix, LIT2INT (lit), ps->LEVEL));

  assign (ps, lit, 0);
}

#ifndef NFL

static int
lit_has_binary_clauses (PS * ps, Lit * lit)
{
#ifdef NO_BINARY_CLAUSES
  Ltk* lstk = LIT2IMPLS (lit);
  return lstk->count != 0;
#else
  return *LIT2IMPLS (lit) != 0;
#endif
}

static void
flbcp (PS * ps)
{
#ifdef STATS
  unsigned long long propagaions_before_bcp = ps->propagations;
#endif
  bcp (ps);
#ifdef STATS
  ps->flprops += ps->propagations - propagaions_before_bcp;
#endif
}

inline static int
cmp_inverse_rnk (PS * ps, Rnk * a, Rnk * b)
{
  (void) ps;
  return -cmp_rnk (a, b);
}

inline static Flt
rnk2jwh (PS * ps, Rnk * r)
{
  Flt res, sum, pjwh, njwh;
  Lit * plit, * nlit;

  plit = RNK2LIT (r);
  nlit = plit + 1;
  
  pjwh = *LIT2JWH (plit);
  njwh = *LIT2JWH (nlit);

  res = mulflt (pjwh, njwh);

  sum = addflt (pjwh, njwh);
  sum = mulflt (sum, base2flt (1, -10));
  res = addflt (res, sum);

  return res;
}

static int
cmp_inverse_jwh_rnk (PS * ps, Rnk * r, Rnk * s)
{
  Flt a = rnk2jwh (ps, r);
  Flt b = rnk2jwh (ps, s);
  int res = cmpflt (a, b);

  if (res)
    return -res;

  return cmp_inverse_rnk (ps, r, s);
}

static void
faillits (PS * ps)
{
  unsigned i, j, old_trail_count, common, saved_count;
  unsigned new_saved_size, oldladded = ps->ladded;
  unsigned long long limit, delta;
  Lit * lit, * other, * pivot;
  Rnk * r, ** p, ** q;
  int new_trail_count;
  double started;

  if (ps->plain)
    return;

  if (ps->heap + 1 >= ps->hhead)
    return;

  if (ps->propagations < ps->fllimit)
    return;

  sflush (ps);
  started = ps->seconds;

  ps->flcalls++;
#ifdef STATSA
  ps->flrounds++;
#endif
  delta = ps->propagations/10;
  if (delta >= 100*1000*1000) delta = 100*1000*1000;
  else if (delta <= 100*1000) delta = 100*1000;

  limit = ps->propagations + delta;
  ps->fllimit = ps->propagations;

  assert (!ps->LEVEL);
  assert (ps->simplifying);

  if (ps->flcalls <= 1)
    SORT (Rnk *, cmp_inverse_jwh_rnk, ps->heap + 1, ps->hhead - (ps->heap + 1));
  else
    SORT (Rnk *, cmp_inverse_rnk, ps->heap + 1, ps->hhead - (ps->heap + 1));

  i = 1;		/* NOTE: heap starts at position '1' */

  while (ps->propagations < limit)
    {
      if (ps->heap + i == ps->hhead)
	{
	  if (ps->ladded == oldladded)
	    break;

	  i = 1;
#ifdef STATS
	  ps->flrounds++;
#endif
	  oldladded = ps->ladded;
	}

      assert (ps->heap + i < ps->hhead);

      r = ps->heap[i++];
      lit = RNK2LIT (r);

      if (lit->val)
	continue;

      if (!lit_has_binary_clauses (ps, NOTLIT (lit)))
	{
#ifdef STATS
	  ps->flskipped++;
#endif
	  continue;
	}

#ifdef STATS
      ps->fltried++;
#endif
      LOG ( fprintf (ps->out, "%strying %d as failed literal\n",
	    ps->prefix, LIT2INT (lit)));

      assign_decision (ps, lit);
      old_trail_count = ps->thead - ps->trail;
      flbcp (ps);

      if (ps->conflict)
	{
EXPLICITLY_FAILED_LITERAL:
	  LOG ( fprintf (ps->out, "%sfound explicitly failed literal %d\n",
		ps->prefix, LIT2INT (lit)));

	  ps->failedlits++;
	  ps->efailedlits++;

	  backtrack (ps);
	  flbcp (ps);

	  if (!ps->conflict)
	    continue;

CONTRADICTION:
	  assert (!ps->LEVEL);
	  backtrack (ps);
	  assert (ps->mtcls);

	  goto RETURN;
	}

      if (ps->propagations >= limit)
	{
	  undo (ps, 0);
	  break;
	}

      lit = NOTLIT (lit);

      if (!lit_has_binary_clauses (ps, NOTLIT (lit)))
	{
#ifdef STATS
	  ps->flskipped++;
#endif
	  undo (ps, 0);
	  continue;
	}

#ifdef STATS
      ps->fltried++;
#endif
      LOG ( fprintf (ps->out, "%strying %d as failed literals\n",
	    ps->prefix, LIT2INT (lit)));

      new_trail_count = ps->thead - ps->trail;
      saved_count = new_trail_count - old_trail_count;

      if (saved_count > ps->saved_size)
	{
	  new_saved_size = ps->saved_size ? 2 * ps->saved_size : 1;
	  while (saved_count > new_saved_size)
	    new_saved_size *= 2;

	  RESIZEN (ps->saved, ps->saved_size, new_saved_size);
	  ps->saved_size = new_saved_size;
	}

      for (j = 0; j < saved_count; j++)
	ps->saved[j] = ps->trail[old_trail_count + j];

      undo (ps, 0);

      assign_decision (ps, lit);
      flbcp (ps);

      if (ps->conflict)
	goto EXPLICITLY_FAILED_LITERAL;

      pivot = (ps->thead - ps->trail <= new_trail_count) ? lit : NOTLIT (lit);

      common = 0;
      for (j = 0; j < saved_count; j++)
	if ((other = ps->saved[j])->val == TRUE)
	  ps->saved[common++] = other;

      undo (ps, 0);

      LOG (if (common)
	     fprintf (ps->out, 
		      "%sfound %d literals implied by %d and %d\n",
		      ps->prefix, common, 
		      LIT2INT (NOTLIT (lit)), LIT2INT (lit)));

#if 1 // set to zero to disable 'lifting'
      for (j = 0; 
	   j < common 
	  /* TODO: For some Velev benchmarks, extracting the common implicit
	   * failed literals took quite some time.  This needs to be fixed by
	   * a dedicated analyzer.  Up to then we bound the number of
	   * propagations in this loop as well.
	   */
	   && ps->propagations < limit + delta
	   ; j++)
	{
	  other = ps->saved[j];

	  if (other->val == TRUE)
	    continue;

	  assert (!other->val);

	  LOG ( fprintf (ps->out, 
			"%sforcing %d as forced implicitly failed literal\n",
			ps->prefix, LIT2INT (other)));

	  assert (pivot != NOTLIT (other));
	  assert (pivot != other);

	  assign_decision (ps, NOTLIT (other));
	  flbcp (ps);

	  assert (ps->LEVEL == 1);

	  if (ps->conflict)
	    {
	      backtrack (ps);
	      assert (!ps->LEVEL);
	    }
	  else
	    {
	      assign_decision (ps, pivot);
	      flbcp (ps);

	      backtrack (ps);

	      if (ps->LEVEL)
		{
		  assert (ps->LEVEL == 1);

		  flbcp (ps);

		  if (ps->conflict)
		    {
		      backtrack (ps);
		      assert (!ps->LEVEL);
		    }
		  else
		    {
		      assign_decision (ps, NOTLIT (pivot));
		      flbcp (ps);
		      backtrack (ps);

		      if (ps->LEVEL)
			{
			  assert (ps->LEVEL == 1);
			  flbcp (ps);

			  if (!ps->conflict)
			    {
#ifdef STATS
			      ps->floopsed++;
#endif
			      undo (ps, 0);
			      continue;
			    }

			  backtrack (ps);
			}

		      assert (!ps->LEVEL);
		    }

		  assert (!ps->LEVEL);
		}
	    }
	  assert (!ps->LEVEL);
	  flbcp (ps);

	  ps->failedlits++;
	  ps->ifailedlits++;

	  if (ps->conflict)
	    goto CONTRADICTION;
	}
#endif
    }

  ps->fllimit += 9 * (ps->propagations - ps->fllimit);	/* 10% for failed literals */

RETURN:

  /* First flush top level assigned literals.  Those are prohibited from
   * being pushed up the heap during 'faillits' since 'simplifying' is set.
   */
  assert (ps->heap < ps->hhead);
  for (p = q = ps->heap + 1; p < ps->hhead; p++)
    {
      r = *p;
      lit = RNK2LIT (r);
      if (lit->val)
       	r->pos = 0;
      else
	*q++ = r;
    }

  /* Then resort with respect to EVSIDS score and fix positions.
   */
  SORT (Rnk *, cmp_inverse_rnk, ps->heap + 1, ps->hhead - (ps->heap + 1));
  for (p = ps->heap + 1; p < ps->hhead; p++)
    (*p)->pos = p - ps->heap;

  sflush (ps);
  ps->flseconds += ps->seconds - started;
}

#endif

static void
simplify (PS * ps, int forced)
{
  Lit * lit, * notlit, ** t;
  unsigned collect, delta;
#ifdef STATS
  size_t bytes_collected;
#endif
  int * q, ilit;
  Cls **p, *c;
  Var * v;

#ifndef NDEDBUG
  (void) forced;
#endif

  assert (!ps->mtcls);
  assert (!satisfied (ps));
  assert (forced || ps->lsimplify <= ps->propagations);
  assert (forced || ps->fsimplify <= ps->fixed);

  if (ps->LEVEL)
    undo (ps, 0);
#ifndef NFL
  ps->simplifying = 1;
  faillits (ps);
  ps->simplifying = 0;

  if (ps->mtcls)
    return;
#endif

  if (ps->cils != ps->cilshead)
    {
      assert (ps->ttail == ps->thead);
      assert (ps->ttail2 == ps->thead);
      ps->ttail = ps->trail;
      for (t = ps->trail; t < ps->thead; t++)
	{
	  lit = *t;
	  v = LIT2VAR (lit);
	  if (v->internal)
	    {
	      assert (LIT2INT (lit) < 0);
	      assert (lit->val == TRUE);
	      unassign (ps, lit);
	    }
	  else
	    *ps->ttail++ = lit;
	}
      ps->ttail2 = ps->thead = ps->ttail;

      for (q = ps->cils; q != ps->cilshead; q++)
	{
	  ilit = *q;
	  assert (0 < ilit && ilit <= (int) ps->max_var);
	  v = ps->vars + ilit;
	  assert (v->internal);
	  v->level = 0;
	  v->reason = 0;
	  lit = int2lit (ps, -ilit);
	  assert (lit->val == UNDEF);
	  lit->val = TRUE;
	  notlit = NOTLIT (lit);
	  assert (notlit->val == UNDEF);
	  notlit->val = FALSE;
	}
    }

  collect = 0;
  for (p = SOC; p != EOC; p = NXC (p))
    {
      c = *p;
      if (!c)
	continue;

#ifdef TRACE
      if (c->collected)
	continue;
#endif

      if (c->locked)
	continue;
      
      assert (!c->collect);
      if (clause_is_toplevel_satisfied (ps, c))
	{
	  mark_clause_to_be_collected (c);
	  collect++;
	}
    }

  LOG ( fprintf (ps->out, "%scollecting %d clauses\n", ps->prefix, collect));
#ifdef STATS
  bytes_collected = 
#endif
  collect_clauses (ps);
#ifdef STATS
  ps->srecycled += bytes_collected;
#endif

  if (ps->cils != ps->cilshead)
    {
      for (q = ps->cils; q != ps->cilshead; q++)
	{
	  ilit = *q;
	  assert (0 < ilit && ilit <= (int) ps->max_var);
	  assert (ps->vars[ilit].internal);
	  if (ps->rilshead == ps->eorils)
	    ENLARGE (ps->rils, ps->rilshead, ps->eorils);
	  *ps->rilshead++ = ilit;
	  lit = int2lit (ps, -ilit);
	  assert (lit->val == TRUE);
	  lit->val = UNDEF;
	  notlit = NOTLIT (lit);
	  assert (notlit->val == FALSE);
	  notlit->val = UNDEF;
	}
      ps->cilshead = ps->cils;
    }

  delta = 10 * (ps->olits + ps->llits) + 100000;
  if (delta > 2000000)
    delta = 2000000;
  ps->lsimplify = ps->propagations + delta;
  ps->fsimplify = ps->fixed;
  ps->simps++;

  report (ps, 1, 's');
}

static void
iteration (PS * ps)
{
  assert (!ps->LEVEL);
  assert (bcp_queue_is_empty (ps));
  assert (ps->isimplify < ps->fixed);

  ps->iterations++;
  report (ps, 2, 'i');
#ifdef NLUBY
  ps->drestart = MINRESTART;
  ps->lrestart = ps->conflicts + ps->drestart;
#else
  init_restart (ps);
#endif
  ps->isimplify = ps->fixed;
}

static int
cmp_glue_activity_size (PS * ps, Cls * c, Cls * d)
{
  Act a, b, * p, * q;

  (void) ps;

  assert (c->learned);
  assert (d->learned);

  if (c->glue < d->glue)		// smaller glue preferred
    return 1;

  if (c->glue > d->glue)
    return -1;

  p = CLS2ACT (c);
  q = CLS2ACT (d);
  a = *p;
  b = *q;

  if (a < b)				// then higher activity
    return -1;

  if (b < a)
    return 1;

  if (c->size < d->size)		// then smaller size
    return 1;

  if (c->size > d->size)
    return -1;

  return 0;
}

static void
reduce (PS * ps, unsigned percentage)
{
  unsigned redcount, lcollect, collect, target;
#ifdef STATS
  size_t bytes_collected;
#endif
  Cls **p, *c;

  assert (ps->rhead == ps->resolved);

  ps->lastreduceconflicts = ps->conflicts;

  assert (percentage <= 100);
  LOG ( fprintf (ps->out, 
                "%sreducing %u%% learned clauses\n",
		ps->prefix, percentage));

  while (ps->nlclauses - ps->llocked > (unsigned)(ps->eor - ps->resolved))
    ENLARGE (ps->resolved, ps->rhead, ps->eor);

  collect = 0;
  lcollect = 0;

  for (p = ((ps->fsimplify < ps->fixed) ? SOC : ps->lclauses); p != EOC; p = NXC (p))
    {
      c = *p;
      if (!c)
	continue;

#ifdef TRACE
      if (c->collected)
	continue;
#endif

      if (c->locked)
	continue;

      assert (!c->collect);
      if (ps->fsimplify < ps->fixed && clause_is_toplevel_satisfied (ps, c))
	{
	  mark_clause_to_be_collected (c);
	  collect++;

	  if (c->learned && c->size > 2)
	    lcollect++;

	  continue;
	}

      if (!c->learned)
	continue;

      if (c->size <= 2)
	continue;

      assert (ps->rhead < ps->eor);
      *ps->rhead++ = c;
    }
  assert (ps->rhead <= ps->eor);

  ps->fsimplify = ps->fixed;

  redcount = ps->rhead - ps->resolved;
  SORT (Cls *, cmp_glue_activity_size, ps->resolved, redcount);

  assert (ps->nlclauses >= lcollect);
  target = ps->nlclauses - lcollect + 1;

  target = (percentage * target + 99) / 100;

  if (target >= redcount)
    target = redcount;

  ps->rhead = ps->resolved + target;
  while (ps->rhead > ps->resolved)
    {
      c = *--ps->rhead;
      mark_clause_to_be_collected (c);

      collect++;
      if (c->learned && c->size > 2)	/* just for consistency */
	lcollect++;
    }

  if (collect)
    {
      ps->reductions++;
#ifdef STATS
      bytes_collected = 
#endif
      collect_clauses (ps);
#ifdef STATS
      ps->rrecycled += bytes_collected;
#endif
      report (ps, 2, '-');
    }

  if (!lcollect)
    inc_lreduce (ps);		/* avoid dead lock */

  assert (ps->rhead == ps->resolved);
}

static void
init_reduce (PS * ps)
{
  // lreduce = loadded / 2;
  ps->lreduce = 1000;

  if (ps->lreduce < 100)
    ps->lreduce = 100;

  if (ps->verbosity)
     fprintf (ps->out, 
             "%s\n%sinitial reduction limit %u clauses\n%s\n",
	     ps->prefix, ps->prefix, ps->lreduce, ps->prefix);
}

static unsigned
rng (PS * ps)
{
  unsigned res = ps->srng;
  ps->srng *= 1664525u;
  ps->srng += 1013904223u;
  NOLOG ( fprintf (ps->out, "%srng () = %u\n", ps->prefix, res));
  return res;
}

static unsigned
rrng (PS * ps, unsigned low, unsigned high)
{
  unsigned long long tmp;
  unsigned res, elements;
  assert (low <= high);
  elements = high - low + 1;
  tmp = rng (ps);
  tmp *= elements;
  tmp >>= 32;
  tmp += low;
  res = tmp;
  NOLOG ( fprintf (ps->out, "%srrng (ps, %u, %u) = %u\n", ps->prefix, low, high, res));
  assert (low <= res);
  assert (res <= high);
  return res;
}

static Lit *
decide_phase (PS * ps, Lit * lit)
{
  Lit * not_lit = NOTLIT (lit);
  Var *v = LIT2VAR (lit);

  assert (LIT2SGN (lit) > 0);
  if (v->usedefphase)
    {
      if (v->defphase)
	{
	  /* assign to TRUE */
	}
      else
	{
	  /* assign to FALSE */
	  lit = not_lit;
	}
    }
  else if (!v->assigned)
    {
#ifdef STATS
      ps->staticphasedecisions++;
#endif
      if (ps->defaultphase == POSPHASE)
	{
	  /* assign to TRUE */
	}
      else if (ps->defaultphase == NEGPHASE)
	{
	  /* assign to FALSE */
	  lit = not_lit;
	}
      else if (ps->defaultphase == RNDPHASE)
	{
	  /* randomly assign default phase */
	  if (rrng (ps, 1, 2) != 2)
	    lit = not_lit;
	}
      else if (*LIT2JWH(lit) <= *LIT2JWH (not_lit))
	{
	  /* assign to FALSE (Jeroslow-Wang says there are more short
	   * clauses with negative occurence of this variable, so satisfy
	   * those, to minimize BCP) 
	   */
	  lit = not_lit;
	}
      else
	{
	  /* assign to TRUE (... but strictly more positive occurrences) */
	}
    }
  else 
    {
      /* repeat last phase: phase saving heuristic */

      if (v->phase)
	{
	  /* assign to TRUE (last phase was TRUE as well) */
	}
      else
	{
	  /* assign to FALSE (last phase was FALSE as well) */
	  lit = not_lit;
	}
    }

  return lit;
}

static unsigned
gcd (unsigned a, unsigned b)
{
  unsigned tmp;

  assert (a);
  assert (b);

  if (a < b)
    {
      tmp = a;
      a = b;
      b = tmp;
    }

  while (b)
    {
      assert (a >= b);
      tmp = b;
      b = a % b;
      a = tmp;
    }

  return a;
}

static Lit *
rdecide (PS * ps)
{
  unsigned idx, delta, spread;
  Lit * res;

  spread = RDECIDE;
  if (rrng (ps, 1, spread) != 2)
    return 0;

  assert (1 <= ps->max_var);
  idx = rrng (ps, 1, ps->max_var);
  res = int2lit (ps, idx);

  if (res->val != UNDEF)
    {
      delta = rrng (ps, 1, ps->max_var);
      while (gcd (delta, ps->max_var) != 1)
	delta--;

      assert (1 <= delta);
      assert (delta <= ps->max_var);

      do {
	idx += delta;
	if (idx > ps->max_var)
	  idx -= ps->max_var;
	res = int2lit (ps, idx);
      } while (res->val != UNDEF);
    }

#ifdef STATS
  ps->rdecisions++;
#endif
  res = decide_phase (ps, res);
  LOG ( fprintf (ps->out, "%srdecide %d\n", ps->prefix, LIT2INT (res)));

  return res;
}

static Lit *
sdecide (PS * ps)
{
  Lit *res;
  Rnk *r;

  for (;;)
    {
      r = htop (ps);
      res = RNK2LIT (r);
      if (res->val == UNDEF) break;
      (void) hpop (ps);
      NOLOG ( fprintf (ps->out, 
                      "%shpop %u %u %u\n",
		      ps->prefix, r - ps->rnks,
		      FLTMANTISSA(r->score),
		      FLTEXPONENT(r->score)));
    }

#ifdef STATS
  ps->sdecisions++;
#endif
  res = decide_phase (ps, res);

  LOG ( fprintf (ps->out, "%ssdecide %d\n", ps->prefix, LIT2INT (res)));

  return res;
}

static Lit *
adecide (PS * ps)
{
  Lit *lit;
  Var * v;

  assert (ps->als < ps->alshead);
  assert (!ps->failed_assumption);

  while (ps->alstail < ps->alshead)
    {
      lit = *ps->alstail++;

      if (lit->val == FALSE)
	{
	  ps->failed_assumption = lit;
	  v = LIT2VAR (lit);

	  use_var (ps, v);

	  LOG ( fprintf (ps->out, "%sfirst failed assumption %d\n",
			ps->prefix, LIT2INT (ps->failed_assumption)));
	  fanalyze (ps);
	  return 0;
	}

      if (lit->val == TRUE)
	{
	  v = LIT2VAR (lit);
	  if (v->level > ps->adecidelevel)
	    ps->adecidelevel = v->level;
	  continue;
	}

#ifdef STATS
      ps->assumptions++;
#endif
      LOG ( fprintf (ps->out, "%sadecide %d\n", ps->prefix, LIT2INT (lit)));
      ps->adecidelevel = ps->LEVEL + 1;

      return lit;
    }

  return 0;
}

static void
decide (PS * ps)
{
  Lit * lit;

  assert (!satisfied (ps));
  assert (!ps->conflict);

  if (ps->alstail < ps->alshead && (lit = adecide (ps)))
    ;
  else if (ps->failed_assumption)
    return;
  else if (satisfied (ps))
    return;
  else if (!(lit = rdecide (ps)))
    lit = sdecide (ps);

  assert (lit);
  assign_decision (ps, lit);

  ps->levelsum += ps->LEVEL;
  ps->decisions++;
}

static int
sat (PS * ps, int l)
{
  int count = 0, backtracked;

  if (!ps->conflict)
    bcp (ps);

  if (ps->conflict)
    backtrack (ps);

  if (ps->mtcls)
    return PICOSAT_UNSATISFIABLE;

  if (satisfied (ps))
    goto SATISFIED;

  if (ps->lsimplify <= ps->propagations)
    simplify (ps, 0);

  if (ps->mtcls)
    return PICOSAT_UNSATISFIABLE;

  if (satisfied (ps))
    goto SATISFIED;

  init_restart (ps);

  if (!ps->lreduce)
    init_reduce (ps);

  ps->isimplify = ps->fixed;
  backtracked = 0;

  for (;;)
    {
      if (!ps->conflict)
	bcp (ps);

      if (ps->conflict)
	{
	  incincs (ps);
	  backtrack (ps);

	  if (ps->mtcls)
	    return PICOSAT_UNSATISFIABLE;
	  backtracked = 1;
	  continue;
	}

      if (satisfied (ps))
	{
SATISFIED:
#ifndef NDEBUG
	  original_clauses_satisfied (ps);
	  assumptions_satisfied (ps);
#endif
	  return PICOSAT_SATISFIABLE;
	}

      if (backtracked)
	{
	  backtracked = 0;
	  if (!ps->LEVEL && ps->isimplify < ps->fixed)
	    iteration (ps);
	}

      if (l >= 0 && count >= l)		/* decision limit reached ? */
	return PICOSAT_UNKNOWN;

      if (ps->interrupt.function &&		/* external interrupt */
	  count > 0 && !(count % INTERRUPTLIM) &&
	  ps->interrupt.function (ps->interrupt.state))
	return PICOSAT_UNKNOWN;

      if (ps->propagations >= ps->lpropagations)/* propagation limit reached ? */
	return PICOSAT_UNKNOWN;

#ifndef NADC
      if (!ps->adodisabled && ps->adoconflicts >= ps->adoconflictlimit)
	{
	  assert (bcp_queue_is_empty (ps));
	  return PICOSAT_UNKNOWN;
	}
#endif

      if (ps->fsimplify < ps->fixed && ps->lsimplify <= ps->propagations)
	{
	  simplify (ps, 0);
	  if (!bcp_queue_is_empty (ps))
	    continue;
#ifndef NFL
	  if (ps->mtcls)
	    return PICOSAT_UNSATISFIABLE;

	  if (satisfied (ps))
	    return PICOSAT_SATISFIABLE;

	  assert (!ps->LEVEL);
#endif
	}

      if (need_to_reduce (ps))
	reduce (ps, 50);

      if (ps->conflicts >= ps->lrestart && ps->LEVEL > 2)
	restart (ps);

      decide (ps);
      if (ps->failed_assumption)
	return PICOSAT_UNSATISFIABLE;
      count++;
    }
}

static void
rebias (PS * ps)
{
  Cls ** p, * c;
  Var * v;

  for (v = ps->vars + 1; v <= ps->vars + ps->max_var; v++)
    v->assigned = 0;

  memset (ps->jwh, 0, 2 * (ps->max_var + 1) * sizeof *ps->jwh);

  for (p = ps->oclauses; p < ps->ohead; p++) 
    {
      c = *p;

      if (!c) 
	continue;

      if (c->learned)
	continue;

      incjwh (ps, c);
    }
}

#ifdef TRACE

static unsigned
core (PS * ps)
{
  unsigned idx, prev, this, delta, i, lcore, vcore;
  unsigned *stack, *shead, *eos;
  Lit **q, **eol, *lit;
  Cls *c, *reason;
  Znt *p, byte;
  Zhn *zhain;
  Var *v;

  assert (ps->trace);

  assert (ps->mtcls || ps->failed_assumption);
  if (ps->ocore >= 0)
    return ps->ocore;

  lcore = ps->ocore = vcore = 0;

  stack = shead = eos = 0;
  ENLARGE (stack, shead, eos);

  if (ps->mtcls)
    {
      idx = CLS2IDX (ps->mtcls);
      *shead++ = idx;
    }
  else
    {
      assert (ps->failed_assumption);
      v = LIT2VAR (ps->failed_assumption);
      reason = v->reason;
      assert (reason);
      idx = CLS2IDX (reason);
      *shead++ = idx;
    }

  while (shead > stack)
    {
      idx = *--shead;
      zhain = IDX2ZHN (idx);

      if (zhain)
	{
	  if (zhain->core)
	    continue;

	  zhain->core = 1;
	  lcore++;

	  c = IDX2CLS (idx);
	  if (c)
	    {
	      assert (!c->core);
	      c->core = 1;
	    }

	  i = 0;
	  delta = 0;
	  prev = 0;
	  for (p = zhain->znt; (byte = *p); p++, i += 7)
	    {
	      delta |= (byte & 0x7f) << i;
	      if (byte & 0x80)
		continue;

	      this = prev + delta;
	      assert (prev < this);	/* no overflow */

	      if (shead == eos)
		ENLARGE (stack, shead, eos);
	      *shead++ = this;

	      prev = this;
	      delta = 0;
	      i = -7;
	    }
	}
      else
	{
	  c = IDX2CLS (idx);

	  assert (c);
	  assert (!c->learned);

	  if (c->core)
	    continue;

	  c->core = 1;
	  ps->ocore++;

	  eol = end_of_lits (c);
	  for (q = c->lits; q < eol; q++)
	    {
	      lit = *q;
	      v = LIT2VAR (lit);
	      if (v->core)
		continue;

	      v->core = 1;
	      vcore++;

	      if (!ps->failed_assumption) continue;
	      if (lit != ps->failed_assumption) continue;

	      reason = v->reason;
	      if (!reason) continue;
	      if (reason->core) continue;

	      idx = CLS2IDX (reason);
	      if (shead == eos)
		ENLARGE (stack, shead, eos);
	      *shead++ = idx;
	    }
	}
    }

  DELETEN (stack, eos - stack);

  if (ps->verbosity)
     fprintf (ps->out,
	     "%s%u core variables out of %u (%.1f%%)\n"
	     "%s%u core original clauses out of %u (%.1f%%)\n"
	     "%s%u core learned clauses out of %u (%.1f%%)\n",
	     ps->prefix, vcore, ps->max_var, PERCENT (vcore, ps->max_var),
	     ps->prefix, ps->ocore, ps->oadded, PERCENT (ps->ocore, ps->oadded),
	     ps->prefix, lcore, ps->ladded, PERCENT (lcore, ps->ladded));

  return ps->ocore;
}

static void
trace_lits (PS * ps, Cls * c, FILE * file)
{
  Lit **p, **eol = end_of_lits (c);

  assert (c);
  assert (c->core);

  for (p = c->lits; p < eol; p++)
    fprintf (file, "%d ", LIT2INT (*p));

  fputc ('0', file);
}

static void
write_idx (PS * ps, unsigned idx, FILE * file)
{
  fprintf (file, "%ld", EXPORTIDX (idx));
}

static void
trace_clause (PS * ps, unsigned idx, Cls * c, FILE * file, int fmt)
{
  assert (c);
  assert (c->core);
  assert (fmt == RUP_TRACE_FMT || !c->learned);
  assert (CLS2IDX (c) == idx);

  if (fmt != RUP_TRACE_FMT)
    {
      write_idx (ps, idx, file);
      fputc (' ', file);
    }

  trace_lits (ps, c, file);

  if (fmt != RUP_TRACE_FMT)
    fputs (" 0", file);

  fputc ('\n', file);
}

static void
trace_zhain (PS * ps, unsigned idx, Zhn * zhain, FILE * file, int fmt)
{
  unsigned prev, this, delta, i;
  Znt *p, byte;
  Cls * c;

  assert (zhain);
  assert (zhain->core);

  write_idx (ps, idx, file);
  fputc (' ', file);

  if (fmt == EXTENDED_TRACECHECK_TRACE_FMT)
    {
      c = IDX2CLS (idx);
      assert (c);
      trace_lits (ps, c, file);
    }
  else
    {
      assert (fmt == COMPACT_TRACECHECK_TRACE_FMT);
      putc ('*', file);
    }

  i = 0;
  delta = 0;
  prev = 0;

  for (p = zhain->znt; (byte = *p); p++, i += 7)
    {
      delta |= (byte & 0x7f) << i;
      if (byte & 0x80)
	continue;

      this = prev + delta;

      putc (' ', file);
      write_idx (ps, this, file);

      prev = this;
      delta = 0;
      i = -7;
    }

  fputs (" 0\n", file);
}

static void
write_core (PS * ps, FILE * file)
{
  Lit **q, **eol;
  Cls **p, *c;

  fprintf (file, "p cnf %u %u\n", ps->max_var, core (ps));

  for (p = SOC; p != EOC; p = NXC (p))
    {
      c = *p;

      if (!c || c->learned || !c->core)
	continue;

      eol = end_of_lits (c);
      for (q = c->lits; q < eol; q++)
	fprintf (file, "%d ", LIT2INT (*q));

      fputs ("0\n", file);
    }
}

#endif

static void
write_trace (PS * ps, FILE * file, int fmt)
{
#ifdef TRACE
  Cls *c, ** p;
  Zhn *zhain;
  unsigned i;

  core (ps);

  if (fmt == RUP_TRACE_FMT)
    {
      ps->rupvariables = picosat_variables (ps),
      ps->rupclauses = picosat_added_original_clauses (ps);
      write_rup_header (ps, file);
    }

  for (p = SOC; p != EOC; p = NXC (p))
    {
      c = *p;

      if (ps->oclauses <= p && p < ps->eoo)
	{
	  i = OIDX2IDX (p - ps->oclauses);
	  assert (!c || CLS2IDX (c) == i);
	}
      else
	{
          assert (ps->lclauses <= p && p < ps->EOL);
	  i = LIDX2IDX (p - ps->lclauses);
	}

      zhain = IDX2ZHN (i);

      if (zhain)
	{
	  if (zhain->core)
	    {
	      if (fmt == RUP_TRACE_FMT)
		trace_clause (ps,i, c, file, fmt);
	      else
		trace_zhain (ps, i, zhain, file, fmt);
	    }
	}
      else if (c)
	{
	  if (fmt != RUP_TRACE_FMT && c)
	    {
	      if (c->core)
		trace_clause (ps, i, c, file, fmt);
	    }
	}
    }
#else
  (void) file;
  (void) fmt;
  (void) ps;
#endif
}

static void
write_core_wrapper (PS * ps, FILE * file, int fmt)
{
  (void) fmt;
#ifdef TRACE
  write_core (ps, file);
#else
  (void) ps;
  (void) file;
#endif
}

static Lit *
import_lit (PS * ps, int lit, int nointernal)
{
  Lit * res;
  Var * v;

  ABORTIF (lit == INT_MIN, "API usage: INT_MIN literal");
  ABORTIF (abs (lit) > (int) ps->max_var && ps->CLS != ps->clshead,
           "API usage: new variable index after 'picosat_push'");

  if (abs (lit) <= (int) ps->max_var)
    {
      res = int2lit (ps, lit);
      v = LIT2VAR (res);
      if (nointernal && v->internal)
	ABORT ("API usage: trying to import invalid literal");
      else if (!nointernal && !v->internal)
	ABORT ("API usage: trying to import invalid context");
    }
  else
    {
      while (abs (lit) > (int) ps->max_var)
	inc_max_var (ps);
      res = int2lit (ps, lit);
    }

  return res;
}

#ifdef TRACE
static void
reset_core (PS * ps)
{
  Cls ** p, * c;
  Zhn ** q, * z;
  unsigned i;

  for (i = 1; i <= ps->max_var; i++)
    ps->vars[i].core = 0;

  for (p = SOC; p != EOC; p = NXC (p))
    if ((c = *p))
      c->core = 0;

  for (q = ps->zhains; q != ps->zhead; q++)
    if ((z = *q))
      z->core = 0;

  ps->ocore = -1;
}
#endif

static void
reset_assumptions (PS * ps)
{
  Lit ** p;

  ps->failed_assumption = 0;

  if (ps->extracted_all_failed_assumptions)
    {
      for (p = ps->als; p < ps->alshead; p++)
	LIT2VAR (*p)->failed = 0;

      ps->extracted_all_failed_assumptions = 0;
    }

  ps->alstail = ps->alshead = ps->als;
  ps->adecidelevel = 0;
}

static void
check_ready (PS * ps)
{
  ABORTIF (!ps || ps->state == RESET, "API usage: uninitialized");
}

static void
check_sat_state (PS * ps)
{
  ABORTIF (ps->state != SAT, "API usage: expected to be in SAT state");
}

static void
check_unsat_state (PS * ps)
{
  ABORTIF (ps->state != UNSAT, "API usage: expected to be in UNSAT state");
}

static void
check_sat_or_unsat_or_unknown_state (PS * ps)
{
  ABORTIF (ps->state != SAT && ps->state != UNSAT && ps->state != UNKNOWN,
           "API usage: expected to be in SAT, UNSAT, or UNKNOWN state");
}

static void
reset_partial (PS * ps)
{
  unsigned idx;
  if (!ps->partial)
    return;
  for (idx = 1; idx <= ps->max_var; idx++)
    ps->vars[idx].partial = 0;
  ps->partial = 0;
}

static void
reset_incremental_usage (PS * ps)
{
  unsigned num_non_false;
  Lit * lit, ** q;

  check_sat_or_unsat_or_unknown_state (ps);

  LOG ( fprintf (ps->out, "%sRESET incremental usage\n", ps->prefix));

  if (ps->LEVEL)
    undo (ps, 0);

  reset_assumptions (ps);

  if (ps->conflict)
    { 
      num_non_false = 0;
      for (q = ps->conflict->lits; q < end_of_lits (ps->conflict); q++)
	{
	  lit = *q;
	  if (lit->val != FALSE)
	    num_non_false++;
	}

      // assert (num_non_false >= 2); // TODO: why this assertion?
#ifdef NO_BINARY_CLAUSES
      if (ps->conflict == &ps->cimpl)
	resetcimpl (ps);
#endif
#ifndef NADC
      if (ps->conflict == ps->adoconflict)
	resetadoconflict (ps);
#endif
      ps->conflict = 0;
    }

#ifdef TRACE
  reset_core (ps);
#endif

  reset_partial (ps);

  ps->saved_flips = ps->flips;
  ps->min_flipped = UINT_MAX;
  ps->saved_max_var = ps->max_var;

  ps->state = READY;
}

static void
enter (PS * ps)
{
  if (ps->nentered++)
    return;

  check_ready (ps);
  ps->entered = picosat_time_stamp ();
}

static void
leave (PS * ps)
{
  assert (ps->nentered);
  if (--ps->nentered)
    return;

  sflush (ps);
}

static void
check_trace_support_and_execute (PS * ps,
                                 FILE * file, 
				 void (*f)(PS*,FILE*,int), int fmt)
{
  check_ready (ps);
  check_unsat_state (ps);
#ifdef TRACE
  ABORTIF (!ps->trace, "API usage: tracing disabled");
  enter (ps);
  f (ps, file, fmt);
  leave (ps);
#else
  (void) file;
  (void) fmt;
  (void) f;
  ABORT ("compiled without trace support");
#endif
}

static void
extract_all_failed_assumptions (PS * ps)
{
  Lit ** p, ** eol;
  Var * v, * u;
  int pos;
  Cls * c;

  assert (!ps->extracted_all_failed_assumptions);

  assert (ps->failed_assumption);
  assert (ps->mhead == ps->marked);

  if (ps->marked == ps->eom)
    ENLARGE (ps->marked, ps->mhead, ps->eom);

  v = LIT2VAR (ps->failed_assumption);
  mark_var (ps, v);
  pos = 0;

  while (pos < ps->mhead - ps->marked)
    {
      v = ps->marked[pos++];
      assert (v->mark);
      c = var2reason (ps, v);
      if (!c)
	continue;
      eol = end_of_lits (c);
      for (p = c->lits; p < eol; p++)
	{
	  u = LIT2VAR (*p);
	  if (!u->mark)
	    mark_var (ps, u);
	}
#ifdef NO_BINARY_CLAUSES
      if (c == &ps->impl)
	resetimpl (ps);
#endif
    }

  for (p = ps->als; p < ps->alshead; p++)
    {
      u = LIT2VAR (*p);
      if (!u->mark) continue;
      u->failed = 1;
      LOG ( fprintf (ps->out,
                     "%sfailed assumption %d\n",
		     ps->prefix, LIT2INT (*p)));
    }

  while (ps->mhead > ps->marked)
    (*--ps->mhead)->mark = 0;

  ps->extracted_all_failed_assumptions = 1;
}

const char *
picosat_copyright (void)
{
  return "Copyright (c) 2006 - 2014 Armin Biere JKU Linz";
}

PicoSAT *
picosat_init (void)
{
  return init (0, 0, 0, 0);
}

PicoSAT * 
picosat_minit (void * pmgr,
	       picosat_malloc pnew,
	       picosat_realloc presize,
	       picosat_free pfree)
{
  ABORTIF (!pnew, "API usage: zero 'picosat_malloc' argument");
  ABORTIF (!presize, "API usage: zero 'picosat_realloc' argument");
  ABORTIF (!pfree, "API usage: zero 'picosat_free' argument");
  return init (pmgr, pnew, presize, pfree);
}


void
picosat_adjust (PS * ps, int new_max_var)
{
  unsigned new_size_vars;

  ABORTIF (abs (new_max_var) > (int) ps->max_var && ps->CLS != ps->clshead,
           "API usage: adjusting variable index after 'picosat_push'");
  enter (ps);

  new_max_var = abs (new_max_var);
  new_size_vars = new_max_var + 1;

  if (ps->size_vars < new_size_vars)
    enlarge (ps, new_size_vars);

  while (ps->max_var < (unsigned) new_max_var)
    inc_max_var (ps);

  leave (ps);
}

int
picosat_inc_max_var (PS * ps)
{
  if (ps->measurealltimeinlib)
    enter (ps);
  else
    check_ready (ps);

  inc_max_var (ps);

  if (ps->measurealltimeinlib)
    leave (ps);

  return ps->max_var;
}

int
picosat_context (PS * ps)
{
  return ps->clshead == ps->CLS ? 0 : LIT2INT (ps->clshead[-1]);
}

int
picosat_push (PS * ps)
{
  int res;
  Lit *lit;
  Var * v;

  if (ps->measurealltimeinlib)
    enter (ps);
  else
    check_ready (ps);

  if (ps->state != READY)
    reset_incremental_usage (ps);

  if (ps->rils != ps->rilshead)
    {
      res = *--ps->rilshead;
      assert (ps->vars[res].internal);
    }
  else
    {
      inc_max_var (ps);
      res = ps->max_var;
      v = ps->vars + res;
      assert (!v->internal);
      v->internal = 1;
      ps->internals++;
      LOG ( fprintf (ps->out, "%snew internal variable index %d\n", ps->prefix, res));
    }

  lit = int2lit (ps, res);

  if (ps->clshead == ps->eocls)
    ENLARGE (ps->CLS, ps->clshead, ps->eocls);
  *ps->clshead++ = lit;

  ps->contexts++;

  LOG ( fprintf (ps->out, "%snew context %d at depth %ld after push\n",
                 ps->prefix, res, (long)(ps->clshead - ps->CLS)));

  if (ps->measurealltimeinlib)
    leave (ps);

  return res;
}

int 
picosat_pop (PS * ps)
{
  Lit * lit;
  int res;
  ABORTIF (ps->CLS == ps->clshead, "API usage: too many 'picosat_pop'");
  ABORTIF (ps->added != ps->ahead, "API usage: incomplete clause");

  if (ps->measurealltimeinlib)
    enter (ps);
  else
    check_ready (ps);

  if (ps->state != READY)
    reset_incremental_usage (ps);

  assert (ps->CLS < ps->clshead);
  lit = *--ps->clshead;
  LOG ( fprintf (ps->out, "%sclosing context %d at depth %ld after pop\n",
                 ps->prefix, LIT2INT (lit), (long)(ps->clshead - ps->CLS) + 1));

  if (ps->cilshead == ps->eocils)
    ENLARGE (ps->cils, ps->cilshead, ps->eocils);
  *ps->cilshead++ = LIT2INT (lit);

  if (ps->cilshead - ps->cils > MAXCILS) {
    LOG ( fprintf (ps->out,
                  "%srecycling %ld interals with forced simplification\n",
		  ps->prefix, (long)(ps->cilshead - ps->cils)));
    simplify (ps, 1);
  }

  res = picosat_context (ps);
  if (res)
    LOG ( fprintf (ps->out, "%snew context %d at depth %ld after pop\n",
		   ps->prefix, res, (long)(ps->clshead - ps->CLS)));
  else
    LOG ( fprintf (ps->out, "%souter most context reached after pop\n", ps->prefix));

  if (ps->measurealltimeinlib)
    leave (ps);
  
  return res;
}

void
picosat_set_verbosity (PS * ps, int new_verbosity_level)
{
  check_ready (ps);
  ps->verbosity = new_verbosity_level;
}

void
picosat_set_plain (PS * ps, int new_plain_value)
{
  check_ready (ps);
  ps->plain = new_plain_value;
}

int
picosat_enable_trace_generation (PS * ps)
{
  int res = 0;
  check_ready (ps);
#ifdef TRACE
  ABORTIF (ps->addedclauses, 
           "API usage: trace generation enabled after adding clauses");
  res = ps->trace = 1;
#endif
  return res;
}

void
picosat_set_incremental_rup_file (PS * ps, FILE * rup_file, int m, int n)
{
  check_ready (ps);
  assert (!ps->rupstarted);
  ps->rup = rup_file;
  ps->rupvariables = m;
  ps->rupclauses = n;
}

void
picosat_set_output (PS * ps, FILE * output_file)
{
  check_ready (ps);
  ps->out = output_file;
}

void
picosat_measure_all_calls (PS * ps)
{
  check_ready (ps);
  ps->measurealltimeinlib = 1;
}

void
picosat_set_prefix (PS * ps, const char * str)
{
  check_ready (ps);
  new_prefix (ps, str);
}

void
picosat_set_seed (PS * ps, unsigned s)
{
  check_ready (ps);
  ps->srng = s;
}

void
picosat_reset (PS * ps)
{
  check_ready (ps);
  reset (ps);
}

int
picosat_add (PS * ps, int int_lit)
{
  int res = ps->oadded;
  Lit *lit;

  if (ps->measurealltimeinlib)
    enter (ps);
  else
    check_ready (ps);

  ABORTIF (ps->rup && ps->rupstarted && ps->oadded >= (unsigned)ps->rupclauses,
           "API usage: adding too many clauses after RUP header written");
#ifndef NADC
  ABORTIF (ps->addingtoado, 
           "API usage: 'picosat_add' and 'picosat_add_ado_lit' mixed");
#endif
  if (ps->state != READY)
    reset_incremental_usage (ps);

  if (ps->saveorig)
    {
      if (ps->sohead == ps->eoso)
	ENLARGE (ps->soclauses, ps->sohead, ps->eoso);

      *ps->sohead++ = int_lit;
    }

  if (int_lit)
    {
      lit = import_lit (ps, int_lit, 1);
      add_lit (ps, lit);
    }
  else
    simplify_and_add_original_clause (ps);

  if (ps->measurealltimeinlib)
    leave (ps);

  return res;
}

int
picosat_add_arg (PS * ps, ...)
{
  int lit;
  va_list ap;
  va_start (ap, ps);
  while ((lit = va_arg (ap, int)))
    (void) picosat_add (ps, lit);
  va_end (ap);
  return picosat_add (ps, 0);
}

int
picosat_add_lits (PS * ps, int * lits)
{
  const int * p;
  int lit;
  for (p = lits; (lit = *p); p++)
    (void) picosat_add (ps, lit);
  return picosat_add (ps, 0);
}

void
picosat_add_ado_lit (PS * ps, int external_lit)
{
#ifndef NADC
  Lit * internal_lit;

  if (ps->measurealltimeinlib)
    enter (ps);
  else
    check_ready (ps);

  if (ps->state != READY)
    reset_incremental_usage (ps);

  ABORTIF (!ps->addingtoado && ps->ahead > ps->added,
           "API usage: 'picosat_add' and 'picosat_add_ado_lit' mixed");

  if (external_lit)
    {
      ps->addingtoado = 1;
      internal_lit = import_lit (ps, external_lit, 1);
      add_lit (ps, internal_lit);
    }
  else
    {
      ps->addingtoado = 0;
      add_ado (ps);
    }
  if (ps->measurealltimeinlib)
    leave (ps);
#else
  (void) ps;
  (void) external_lit;
  ABORT ("compiled without all different constraint support");
#endif
}

static void
assume (PS * ps, Lit * lit)
{
  if (ps->alshead == ps->eoals)
    {
      assert (ps->alstail == ps->als);
      ENLARGE (ps->als, ps->alshead, ps->eoals);
      ps->alstail = ps->als;
    }

  *ps->alshead++ = lit;
  LOG ( fprintf (ps->out, "%sassumption %d\n", ps->prefix, LIT2INT (lit)));
}

static void
assume_contexts (PS * ps)
{
  Lit ** p;
  if (ps->als != ps->alshead)
    return;
  for (p = ps->CLS; p != ps->clshead; p++)
    assume (ps, *p);
}

#ifndef RCODE
static const char * enumstr (int i) {
  int last = i % 10;
  if (last == 1) return "st";
  if (last == 2) return "nd";
  if (last == 3) return "rd";
  return "th";
}
#endif

static int
tderef (PS * ps, int int_lit)
{
  Lit * lit;
  Var * v;

  assert (abs (int_lit) <= (int) ps->max_var);

  lit = int2lit (ps, int_lit);

  v = LIT2VAR (lit);
  if (v->level > 0)
    return 0;

  if (lit->val == TRUE)
    return 1;

  if (lit->val == FALSE)
    return -1;

  return 0;
}

static int
pderef (PS * ps, int int_lit)
{
  Lit * lit;
  Var * v;

  assert (abs (int_lit) <= (int) ps->max_var);

  v = ps->vars + abs (int_lit);
  if (!v->partial)
    return 0;

  lit = int2lit (ps, int_lit);

  if (lit->val == TRUE)
    return 1;

  if (lit->val == FALSE)
    return -1;

  return 0;
}

static void
minautarky (PS * ps)
{
  unsigned * occs, maxoccs, tmpoccs, npartial;
  int * p, * c, lit, best, val;
#ifdef LOGGING
  int tl;
#endif

  assert (!ps->partial);

  npartial = 0;

  NEWN (occs, 2*ps->max_var + 1);
  CLRN (occs, 2*ps->max_var + 1);
  occs += ps->max_var;
  for (p = ps->soclauses; p < ps->sohead; p++)
    occs[*p]++;
  assert (occs[0] == ps->oadded);

  for (c = ps->soclauses; c < ps->sohead; c = p + 1) 
    {
#ifdef LOGGING
      tl = 0;
#endif
      best = 0; 
      maxoccs = 0;
      for (p = c; (lit = *p); p++)
	{
	  val = tderef (ps, lit);
	  if (val < 0)
	    continue;
	  if (val > 0)
	    {
#ifdef LOGGING
	      tl = 1;
#endif
	      best = lit;
	      maxoccs = occs[lit];
	    }

	  val = pderef (ps, lit);
	  if (val > 0)
	    break;
	  if (val < 0)
	    continue;
	  val = int2lit (ps, lit)->val;
	  assert (val);
	  if (val < 0)
	    continue;
	  tmpoccs = occs[lit];
	  if (best && tmpoccs <= maxoccs)
	    continue;
	  best = lit;
	  maxoccs = tmpoccs;
	}
      if (!lit)
	{
	  assert (best);
	  LOG ( fprintf (ps->out, "%sautark %d with %d occs%s\n", 
	       ps->prefix, best, maxoccs, tl ? " (top)" : ""));
	  ps->vars[abs (best)].partial = 1;
	  npartial++;
	}
      for (p = c; (lit = *p); p++)
	{
	  assert (occs[lit] > 0);
	  occs[lit]--;
	}
    }
  occs -= ps->max_var;
  DELETEN (occs, 2*ps->max_var + 1);
  ps->partial = 1;

  if (ps->verbosity)
     fprintf (ps->out,
      "%sautarky of size %u out of %u satisfying all clauses (%.1f%%)\n",
      ps->prefix, npartial, ps->max_var, PERCENT (npartial, ps->max_var));
}

void
picosat_assume (PS * ps, int int_lit)
{
  Lit *lit;

  if (ps->measurealltimeinlib)
    enter (ps);
  else
    check_ready (ps);

  if (ps->state != READY)
    reset_incremental_usage (ps);

  assume_contexts (ps);
  lit = import_lit (ps, int_lit, 1);
  assume (ps, lit);

  if (ps->measurealltimeinlib)
    leave (ps);
}

int
picosat_sat (PS * ps, int l)
{
  int res;
  char ch;

  enter (ps);

  ps->calls++;
  LOG ( fprintf (ps->out, "%sSTART call %u\n", ps->prefix, ps->calls));

  if (ps->added < ps->ahead)
    {
#ifndef NADC
      if (ps->addingtoado)
	ABORT ("API usage: incomplete all different constraint");
      else
#endif
	ABORT ("API usage: incomplete clause");
    }

  if (ps->state != READY)
    reset_incremental_usage (ps);

  assume_contexts (ps);

  res = sat (ps, l);

  assert (ps->state == READY);

  switch (res)
    {
    case PICOSAT_UNSATISFIABLE:
      ch = '0';
      ps->state = UNSAT;
      break;
    case PICOSAT_SATISFIABLE:
      ch = '1';
      ps->state = SAT;
      break;
    default:
      ch = '?';
      ps->state = UNKNOWN;
      break;
    }

  if (ps->verbosity)
    {
      report (ps, 1, ch);
      rheader (ps);
    }

  leave (ps);
  LOG ( fprintf (ps->out, "%sEND call %u result %d\n", ps->prefix, ps->calls, res));

  ps->last_sat_call_result = res;

  return res;
}

int
picosat_res (PS * ps)
{
  return ps->last_sat_call_result;
}

int
picosat_deref (PS * ps, int int_lit)
{
  Lit *lit;

  check_ready (ps);
  check_sat_state (ps);
  ABORTIF (!int_lit, "API usage: can not deref zero literal");
  ABORTIF (ps->mtcls, "API usage: deref after empty clause generated");

#ifdef STATS
  ps->derefs++;
#endif

  if (abs (int_lit) > (int) ps->max_var)
    return 0;

  lit = int2lit (ps, int_lit);

  if (lit->val == TRUE)
    return 1;

  if (lit->val == FALSE)
    return -1;

  return 0;
}

int
picosat_deref_toplevel (PS * ps, int int_lit)
{
  check_ready (ps);
  ABORTIF (!int_lit, "API usage: can not deref zero literal");

#ifdef STATS
  ps->derefs++;
#endif
  if (abs (int_lit) > (int) ps->max_var)
    return 0;

  return tderef (ps, int_lit);
}

int
picosat_inconsistent (PS * ps)
{
  check_ready (ps);
  return ps->mtcls != 0;
}

int
picosat_corelit (PS * ps, int int_lit)
{
  check_ready (ps);
  check_unsat_state (ps);
  ABORTIF (!int_lit, "API usage: zero literal can not be in core");

  assert (ps->mtcls || ps->failed_assumption);

#ifdef TRACE
  {
    int res = 0;
    ABORTIF (!ps->trace, "tracing disabled");
    if (ps->measurealltimeinlib)
      enter (ps);
    core (ps);
    if (abs (int_lit) <= (int) ps->max_var)
      res = ps->vars[abs (int_lit)].core;
    assert (!res || ps->failed_assumption || ps->vars[abs (int_lit)].used);
    if (ps->measurealltimeinlib)
      leave (ps);
    return res;
  }
#else
  ABORT ("compiled without trace support");
  return 0;
#endif
}

int
picosat_coreclause (PS * ps, int ocls)
{
  check_ready (ps);
  check_unsat_state (ps);

  ABORTIF (ocls < 0, "API usage: negative original clause index");
  ABORTIF (ocls >= (int)ps->oadded, "API usage: original clause index exceeded");

  assert (ps->mtcls || ps->failed_assumption);

#ifdef TRACE
  {
    Cls ** clsptr, * c;
    int res  = 0;

    ABORTIF (!ps->trace, "tracing disabled");
    if (ps->measurealltimeinlib)
      enter (ps);
    core (ps);
    clsptr = ps->oclauses + ocls;
    assert (clsptr < ps->ohead);
    c = *clsptr;
    if (c) 
      res = c->core;
    if (ps->measurealltimeinlib)
      leave (ps);

    return res;
  }
#else
  ABORT ("compiled without trace support");
  return 0;
#endif
}

int
picosat_failed_assumption (PS * ps, int int_lit)
{
  Lit * lit;
  Var * v;
  ABORTIF (!int_lit, "API usage: zero literal as assumption");
  check_ready (ps);
  check_unsat_state (ps);
  if (ps->mtcls)
    return 0;
  assert (ps->failed_assumption);
  if (abs (int_lit) > (int) ps->max_var)
    return 0;
  if (!ps->extracted_all_failed_assumptions)
    extract_all_failed_assumptions (ps);
  lit = import_lit (ps, int_lit, 1);
  v = LIT2VAR (lit);
  return v->failed;
}

int
picosat_failed_context (PS * ps, int int_lit)
{
  Lit * lit;
  Var * v;
  ABORTIF (!int_lit, "API usage: zero literal as context");
  ABORTIF (abs (int_lit) > (int) ps->max_var, "API usage: invalid context");
  check_ready (ps);
  check_unsat_state (ps);
  assert (ps->failed_assumption);
  if (!ps->extracted_all_failed_assumptions)
    extract_all_failed_assumptions (ps);
  lit = import_lit (ps, int_lit, 0);
  v = LIT2VAR (lit);
  return v->failed;
}

const int *
picosat_failed_assumptions (PS * ps)
{
  Lit ** p, * lit;
  Var * v;
  int ilit;

  ps->falshead = ps->fals;
  check_ready (ps);
  check_unsat_state (ps);
  if (!ps->mtcls) 
    {
      assert (ps->failed_assumption);
      if (!ps->extracted_all_failed_assumptions)
	extract_all_failed_assumptions (ps);

      for (p = ps->als; p < ps->alshead; p++)
	{
	  lit = *p;
	  v = LIT2VAR (*p);
	  if (!v->failed)
	    continue;
	  ilit = LIT2INT (lit);
	  if (ps->falshead == ps->eofals)
	    ENLARGE (ps->fals, ps->falshead, ps->eofals);
	  *ps->falshead++ = ilit;
	}
    }
  if (ps->falshead == ps->eofals)
    ENLARGE (ps->fals, ps->falshead, ps->eofals);
  *ps->falshead++ = 0;
  return ps->fals;
}

const int *
picosat_mus_assumptions (PS * ps, void * s, void (*cb)(void*,const int*), int fix)
{
  int i, j, ilit, len, nwork, * work, res;
  signed char * redundant;
  Lit ** p, * lit;
  int failed;
  Var * v;
#ifndef NDEBUG
  int oldlen;
#endif
#ifndef RCODE
  int norig = ps->alshead - ps->als; 
#endif

  check_ready (ps);
  check_unsat_state (ps);
  len = 0;
  if (!ps->mtcls) 
    {
      assert (ps->failed_assumption);
      if (!ps->extracted_all_failed_assumptions)
	extract_all_failed_assumptions (ps);

      for (p = ps->als; p < ps->alshead; p++)
	if (LIT2VAR (*p)->failed)
	  len++;
    }

  if (ps->mass)
    DELETEN (ps->mass, ps->szmass);
  ps->szmass = len + 1;
  NEWN (ps->mass, ps->szmass);

  i = 0;
  for (p = ps->als; p < ps->alshead; p++)
    {
      lit = *p;
      v = LIT2VAR (lit);
      if (!v->failed)
	continue;
      ilit = LIT2INT (lit);
      assert (i < len);
      ps->mass[i++] = ilit;
    }
  assert (i == len);
  ps->mass[i] = 0;
  if (ps->verbosity)
     fprintf (ps->out, 
      "%sinitial set of failed assumptions of size %d out of %d (%.0f%%)\n",
      ps->prefix, len, norig, PERCENT (len, norig));
  if (cb)
    cb (s, ps->mass);

  nwork = len;
  NEWN (work, nwork);
  for (i = 0; i < len; i++)
    work[i] = ps->mass[i];

  NEWN (redundant, nwork);
  CLRN (redundant, nwork);

  for (i = 0; i < nwork; i++)
    {
      if (redundant[i])
	continue;

      if (ps->verbosity > 1)
	 fprintf (ps->out,
	         "%strying to drop %d%s assumption %d\n", 
		 ps->prefix, i, enumstr (i), work[i]);
      for (j = 0; j < nwork; j++)
	{
	  if (i == j) continue;
	  if (j < i && fix) continue;
	  if (redundant[j]) continue;
	  picosat_assume (ps, work[j]);
	}

      res = picosat_sat (ps, -1);
      if (res == 10)
	{
	  if (ps->verbosity > 1)
	     fprintf (ps->out,
		     "%sfailed to drop %d%s assumption %d\n", 
		     ps->prefix, i, enumstr (i), work[i]);

	  if (fix)
	    {
	      picosat_add (ps, work[i]);
	      picosat_add (ps, 0);
	    }
	}
      else
	{
	  assert (res == 20);
	  if (ps->verbosity > 1)
	     fprintf (ps->out,
		     "%ssuceeded to drop %d%s assumption %d\n", 
		     ps->prefix, i, enumstr (i), work[i]);
	  redundant[i] = 1;
	  for (j = 0; j < nwork; j++)
	    {
	      failed = picosat_failed_assumption (ps, work[j]);
	      if (j <= i) 
		{
		  assert ((j < i && fix) || redundant[j] == !failed);
		  continue;
		}

	      if (!failed)
		{
		  redundant[j] = -1;
		  if (ps->verbosity > 1)
		     fprintf (ps->out,
			     "%salso suceeded to drop %d%s assumption %d\n", 
			     ps->prefix, j, enumstr (j), work[j]);
		}
	    }

#ifndef NDEBUG
	    oldlen = len;
#endif
	    len = 0;
	    for (j = 0; j < nwork; j++)
	      if (!redundant[j])
		ps->mass[len++] = work[j];
	    ps->mass[len] = 0;
	    assert (len < oldlen);

	    if (fix)
	      {
		picosat_add (ps, -work[i]);
		picosat_add (ps, 0);
	      }

#ifndef NDEBUG
	    for (j = 0; j <= i; j++)
	      assert (redundant[j] >= 0);
#endif
	    for (j = i + 1; j < nwork; j++) 
	      {
		if (redundant[j] >= 0)
		  continue;

		if (fix)
		  {
		    picosat_add (ps, -work[j]);
		    picosat_add (ps, 0);
		  }

		redundant[j] = 1;
	      }

	    if (ps->verbosity)
	       fprintf (ps->out, 
	"%sreduced set of failed assumptions of size %d out of %d (%.0f%%)\n",
		ps->prefix, len, norig, PERCENT (len, norig));
	    if (cb)
	      cb (s, ps->mass);
	}
    }

  DELETEN (work, nwork);
  DELETEN (redundant, nwork);

  if (ps->verbosity)
    {
       fprintf (ps->out, "%sreinitializing unsat state\n", ps->prefix);
      fflush (ps->out);
    }

  for (i = 0; i < len; i++)
    picosat_assume (ps, ps->mass[i]);

#ifndef NDEBUG
  res = 
#endif
  picosat_sat (ps, -1);
  assert (res == 20);

  if (!ps->mtcls)
    {
      assert (!ps->extracted_all_failed_assumptions);
      extract_all_failed_assumptions (ps);
    }

  return ps->mass;
}

static const int *
mss (PS * ps, int * a, int size)
{
  int i, j, k, res;

  assert (!ps->mtcls);

  if (ps->szmssass)
    DELETEN (ps->mssass, ps->szmssass);

  ps->szmssass = 0;
  ps->mssass = 0;

  ps->szmssass = size + 1;
  NEWN (ps->mssass, ps->szmssass);

  LOG ( fprintf (ps->out, "%ssearch MSS over %d assumptions\n", ps->prefix, size));

  k = 0;
  for (i = k; i < size; i++)
    {
      for (j = 0; j < k; j++)
	picosat_assume (ps, ps->mssass[j]);

      LOG ( fprintf (ps->out, 
             "%strying to add assumption %d to MSS : %d\n", 
	     ps->prefix, i, a[i])); 

      picosat_assume (ps, a[i]);

      res = picosat_sat (ps, -1);
      if (res == 10)
	{
	  LOG ( fprintf (ps->out, 
		 "%sadding assumption %d to MSS : %d\n", ps->prefix, i, a[i])); 

	  ps->mssass[k++] = a[i];

	  for (j = i + 1; j < size; j++)
	    {
	      if (picosat_deref (ps, a[j]) <= 0)
		continue;

	      LOG ( fprintf (ps->out, 
		     "%salso adding assumption %d to MSS : %d\n", 
		     ps->prefix, j, a[j])); 

	      ps->mssass[k++] = a[j];

	      if (++i != j)
		{
		  int tmp = a[i];
		  a[i] = a[j];
		  a[j] = tmp;
		}
	    }
	}
      else
	{
	  assert (res == 20);

	  LOG ( fprintf (ps->out, 
		 "%signoring assumption %d in MSS : %d\n", ps->prefix, i, a[i])); 
	}
    }
  ps->mssass[k] = 0;
  LOG ( fprintf (ps->out, "%sfound MSS of size %d\n", ps->prefix, k));

  return ps->mssass;
}

static void
reassume (PS * ps, const int * a, int size)
{
  int i;
  LOG ( fprintf (ps->out, "%sreassuming all assumptions\n", ps->prefix));
  for (i = 0; i < size; i++)
    picosat_assume (ps, a[i]);
}

const int *
picosat_maximal_satisfiable_subset_of_assumptions (PS * ps)
{
  const int * res;
  int i, *a, size;

  ABORTIF (ps->mtcls,
           "API usage: CNF inconsistent (use 'picosat_inconsistent')");

  enter (ps);

  size = ps->alshead - ps->als;
  NEWN (a, size);

  for (i = 0; i < size; i++)
    a[i] = LIT2INT (ps->als[i]);

  res = mss (ps, a, size);
  reassume (ps, a, size);

  DELETEN (a, size);

  leave (ps);

  return res;
}

static void
check_mss_flags_clean (PS * ps)
{
#ifndef NDEBUG
  unsigned i;
  for (i = 1; i <= ps->max_var; i++)
    {
      assert (!ps->vars[i].msspos);
      assert (!ps->vars[i].mssneg);
    }
#else
  (void) ps;
#endif
}

static void
push_mcsass (PS * ps, int lit)
{
  if (ps->nmcsass == ps->szmcsass)
    {
      ps->szmcsass = ps->szmcsass ? 2*ps->szmcsass : 1;
      RESIZEN (ps->mcsass, ps->nmcsass, ps->szmcsass);
    }

  ps->mcsass[ps->nmcsass++] = lit;
}

static const int *
next_mss (PS * ps, int mcs)
{
  int i, *a, size, mssize, mcsize, lit, inmss;
  const int * res, * p;
  Var * v;

  if (ps->mtcls) return 0;

  check_mss_flags_clean (ps);

  if (mcs && ps->mcsass)
    {
      DELETEN (ps->mcsass, ps->szmcsass);
      ps->nmcsass = ps->szmcsass = 0;
      ps->mcsass = 0;
    }

  size = ps->alshead - ps->als;
  NEWN (a, size);

  for (i = 0; i < size; i++)
    a[i] = LIT2INT (ps->als[i]);

  (void) picosat_sat (ps, -1);

  //TODO short cut for 'picosat_res () == 10'?

  if (ps->mtcls)
    {
      assert (picosat_res (ps) == 20);
      res = 0;
      goto DONE;
    }

  res = mss (ps, a, size);

  if (ps->mtcls)
    {
      res = 0;
      goto DONE;
    }

  for (p = res; (lit = *p); p++) 
    {
      v = ps->vars + abs (lit);
      if (lit < 0)
	{
	  assert (!v->msspos);
	  v->mssneg = 1;
	}
      else
	{
	  assert (!v->mssneg);
	  v->msspos = 1;
	}
    }

  mssize = p - res;
  mcsize = INT_MIN;

  for (i = 0; i < size; i++)
    {
      lit = a[i];
      v = ps->vars + abs (lit);
      if (lit > 0 && v->msspos)
	inmss = 1;
      else if (lit < 0 && v->mssneg)
	inmss = 1;
      else 
	inmss = 0;

      if (mssize < mcsize)
	{
	  if (inmss)
	    picosat_add (ps, -lit);
	}
      else
	{
	  if (!inmss)
	    picosat_add (ps, lit);
	}

      if (!inmss && mcs)
	push_mcsass (ps, lit);
    }
  picosat_add (ps, 0);
  if (mcs)
    push_mcsass (ps, 0);

  for (i = 0; i < size; i++)
    {
      lit = a[i];
      v = ps->vars + abs (lit);
      v->msspos = 0;
      v->mssneg = 0;
    }

DONE:

  reassume (ps, a, size);
  DELETEN (a, size);

  return res;
}

const int *
picosat_next_maximal_satisfiable_subset_of_assumptions (PS * ps)
{
  const int * res;
  enter (ps);
  res = next_mss (ps, 0);
  leave (ps);
  return  res;
}

const int *
picosat_next_minimal_correcting_subset_of_assumptions (PS * ps)
{
  const int * res, * tmp;
  enter (ps);
  tmp = next_mss (ps, 1);
  res = tmp ? ps->mcsass : 0;
  leave (ps);
  return res;
}

const int *
picosat_humus (PS * ps, 
               void (*callback)(void*state,int nmcs,int nhumus),
	       void * state)
{
  int lit, nmcs, j, nhumus;
  const int * mcs, * p;
  unsigned i;
  Var * v;
  enter (ps);
#ifndef NDEBUG
  for (i = 1; i <= ps->max_var; i++)
    {
      v = ps->vars + i;
      assert (!v->humuspos);
      assert (!v->humusneg);
    }
#endif
  nhumus = nmcs = 0;
  while ((mcs = picosat_next_minimal_correcting_subset_of_assumptions (ps)))
    {
      for (p = mcs; (lit = *p); p++)
	{
	  v = ps->vars + abs (lit);
	  if (lit < 0)
	    {
	      if (!v->humusneg)
		{
		  v->humusneg = 1;
		  nhumus++;
		}
	    }
	  else
	    {
	      if (!v->humuspos)
		{
		  v->humuspos = 1;
		  nhumus++;
		}
	    }
	}
      nmcs++;
      LOG ( fprintf (ps->out, 
             "%smcs %d of size %d humus %d\n",
	     ps->prefix, nmcs, (int)(p - mcs), nhumus));
      if (callback)
	callback (state, nmcs, nhumus);
    }
  assert (!ps->szhumus);
  ps->szhumus = 1;
  for (i = 1; i <= ps->max_var; i++)
    {
      v = ps->vars + i;
      if (v->humuspos)
	ps->szhumus++;
      if (v->humusneg)
	ps->szhumus++;
    }
  assert (nhumus + 1 == ps->szhumus);
  NEWN (ps->humus, ps->szhumus);
  j = 0;
  for (i = 1; i <= ps->max_var; i++)
    {
      v = ps->vars + i;
      if (v->humuspos)
	{
	  assert (j < nhumus);
	  ps->humus[j++] = (int) i;
	}
      if (v->humusneg)
	{
	  assert (j < nhumus);
	  assert (i < INT_MAX);
	  ps->humus[j++] = - (int) i;
	}
    }
  assert (j == nhumus);
  assert (j < ps->szhumus);
  ps->humus[j] = 0;
  leave (ps);
  return ps->humus;
}

int
picosat_usedlit (PS * ps, int int_lit)
{
  int res;
  check_ready (ps);
  check_sat_or_unsat_or_unknown_state (ps);
  ABORTIF (!int_lit, "API usage: zero literal can not be used");
  int_lit = abs (int_lit);
  res = (int_lit <= (int) ps->max_var) ? ps->vars[int_lit].used : 0;
  return res;
}

void
picosat_write_clausal_core (PS * ps, FILE * file)
{
  check_trace_support_and_execute (ps, file, write_core_wrapper, 0);
}

void
picosat_write_compact_trace (PS * ps, FILE * file)
{
  check_trace_support_and_execute (ps, file, write_trace,
                                   COMPACT_TRACECHECK_TRACE_FMT);
}

void
picosat_write_extended_trace (PS * ps, FILE * file)
{
  check_trace_support_and_execute (ps, file, write_trace,
                                   EXTENDED_TRACECHECK_TRACE_FMT);
}

void
picosat_write_rup_trace (PS * ps, FILE * file)
{
  check_trace_support_and_execute (ps, file, write_trace, RUP_TRACE_FMT);
}

size_t
picosat_max_bytes_allocated (PS * ps)
{
  check_ready (ps);
  return ps->max_bytes;
}

void
picosat_set_propagation_limit (PS * ps, unsigned long long l)
{
  ps->lpropagations = l;
}

unsigned long long
picosat_propagations (PS * ps)
{
  return ps->propagations;
}

unsigned long long
picosat_visits (PS * ps)
{
  return ps->visits;
}

unsigned long long
picosat_decisions (PS * ps)
{
  return ps->decisions;
}

int
picosat_variables (PS * ps)
{
  check_ready (ps);
  return (int) ps->max_var;
}

int
picosat_added_original_clauses (PS * ps)
{
  check_ready (ps);
  return (int) ps->oadded;
}

void
picosat_stats (PS * ps)
{
#ifndef RCODE
  unsigned redlits;
#endif
#ifdef STATS
  check_ready (ps);
  assert (ps->sdecisions + ps->rdecisions + ps->assumptions == ps->decisions);
#endif
  if (ps->calls > 1)
     fprintf (ps->out, "%s%u calls\n", ps->prefix, ps->calls);
  if (ps->contexts)
    {
       fprintf (ps->out, "%s%u contexts", ps->prefix, ps->contexts);
#ifdef STATS
       fprintf (ps->out, " %u internal variables", ps->internals);
#endif
       fprintf (ps->out, "\n");
    }
   fprintf (ps->out, "%s%u iterations\n", ps->prefix, ps->iterations);
   fprintf (ps->out, "%s%u restarts", ps->prefix, ps->restarts);
#ifdef STATS
   fprintf (ps->out, " (%u skipped)", ps->skippedrestarts);
#endif
  fputc ('\n', ps->out);
#ifndef NFL
   fprintf (ps->out, "%s%u failed literals", ps->prefix, ps->failedlits);
#ifdef STATS
   fprintf (ps->out,
           ", %u calls, %u rounds, %llu propagations",
           ps->flcalls, ps->flrounds, ps->flprops);
#endif
  fputc ('\n', ps->out);
#ifdef STATS
   fprintf (ps->out, 
    "%sfl: %u = %.1f%% implicit, %llu oopsed, %llu tried, %llu skipped\n", 
    ps->prefix, 
    ps->ifailedlits, PERCENT (ps->ifailedlits, ps->failedlits),
    ps->floopsed, ps->fltried, ps->flskipped);
#endif
#endif
   fprintf (ps->out, "%s%u conflicts", ps->prefix, ps->conflicts);
#ifdef STATS
   fprintf (ps->out, " (%u uips = %.1f%%)\n", ps->uips, PERCENT(ps->uips,ps->conflicts));
#else
  fputc ('\n', ps->out);
#endif
#ifndef NADC
   fprintf (ps->out, "%s%u adc conflicts\n", ps->prefix, ps->adoconflicts);
#endif
#ifdef STATS
   fprintf (ps->out, "%s%llu dereferenced literals\n", ps->prefix, ps->derefs);
#endif
   fprintf (ps->out, "%s%u decisions", ps->prefix, ps->decisions);
#ifdef STATS
   fprintf (ps->out, " (%u random = %.2f%%",
           ps->rdecisions, PERCENT (ps->rdecisions, ps->decisions));
   fprintf (ps->out, ", %u assumptions", ps->assumptions);
  fputc (')', ps->out);
#endif
  fputc ('\n', ps->out);
#ifdef STATS
   fprintf (ps->out,
           "%s%u static phase decisions (%.1f%% of all variables)\n",
	   ps->prefix,
	   ps->staticphasedecisions, PERCENT (ps->staticphasedecisions, ps->max_var));
#endif
   fprintf (ps->out, "%s%u fixed variables\n", ps->prefix, ps->fixed);
  assert (ps->nonminimizedllits >= ps->minimizedllits);
#ifndef RCODE
  redlits = ps->nonminimizedllits - ps->minimizedllits;
#endif
   fprintf (ps->out, "%s%u learned literals\n", ps->prefix, ps->llitsadded);
   fprintf (ps->out, "%s%.1f%% deleted literals\n",
     ps->prefix, PERCENT (redlits, ps->nonminimizedllits));

#ifdef STATS
#ifdef TRACE
   fprintf (ps->out,
	   "%s%llu antecedents (%.1f antecedents per clause",
	   ps->prefix, ps->antecedents, AVERAGE (ps->antecedents, ps->conflicts));
  if (ps->trace)
     fprintf (ps->out, ", %.1f bytes/antecedent)", AVERAGE (ps->znts, ps->antecedents));
  fputs (")\n", ps->out);
#endif

   fprintf (ps->out, "%s%llu propagations (%.1f propagations per decision)\n",
           ps->prefix, ps->propagations, AVERAGE (ps->propagations, ps->decisions));
   fprintf (ps->out, "%s%llu visits (%.1f per propagation)\n",
	   ps->prefix, ps->visits, AVERAGE (ps->visits, ps->propagations));
   fprintf (ps->out, 
           "%s%llu binary clauses visited (%.1f%% %.1f per propagation)\n",
	   ps->prefix, ps->bvisits, 
	   PERCENT (ps->bvisits, ps->visits),
	   AVERAGE (ps->bvisits, ps->propagations));
   fprintf (ps->out, 
           "%s%llu ternary clauses visited (%.1f%% %.1f per propagation)\n",
	   ps->prefix, ps->tvisits, 
	   PERCENT (ps->tvisits, ps->visits),
	   AVERAGE (ps->tvisits, ps->propagations));
   fprintf (ps->out, 
           "%s%llu large clauses visited (%.1f%% %.1f per propagation)\n",
	   ps->prefix, ps->lvisits, 
	   PERCENT (ps->lvisits, ps->visits),
	   AVERAGE (ps->lvisits, ps->propagations));
   fprintf (ps->out, "%s%llu other true (%.1f%% of visited clauses)\n",
	   ps->prefix, ps->othertrue, PERCENT (ps->othertrue, ps->visits));
   fprintf (ps->out, 
           "%s%llu other true in binary clauses (%.1f%%)"
	   ", %llu upper (%.1f%%)\n",
           ps->prefix, ps->othertrue2, PERCENT (ps->othertrue2, ps->othertrue),
	   ps->othertrue2u, PERCENT (ps->othertrue2u, ps->othertrue2));
   fprintf (ps->out, 
           "%s%llu other true in large clauses (%.1f%%)"
	   ", %llu upper (%.1f%%)\n",
           ps->prefix, ps->othertruel, PERCENT (ps->othertruel, ps->othertrue),
	   ps->othertruelu, PERCENT (ps->othertruelu, ps->othertruel));
   fprintf (ps->out, "%s%llu ternary and large traversals (%.1f per visit)\n",
	   ps->prefix, ps->traversals, AVERAGE (ps->traversals, ps->visits));
   fprintf (ps->out, "%s%llu large traversals (%.1f per large visit)\n",
	   ps->prefix, ps->ltraversals, AVERAGE (ps->ltraversals, ps->lvisits));
   fprintf (ps->out, "%s%llu assignments\n", ps->prefix, ps->assignments);
#else
   fprintf (ps->out, "%s%llu propagations\n", ps->prefix, picosat_propagations (ps));
   fprintf (ps->out, "%s%llu visits\n", ps->prefix, picosat_visits (ps));
#endif
   fprintf (ps->out, "%s%.1f%% variables used\n", ps->prefix, PERCENT (ps->vused, ps->max_var));

  sflush (ps);
   fprintf (ps->out, "%s%.1f seconds in library\n", ps->prefix, ps->seconds);
   fprintf (ps->out, "%s%.1f megaprops/second\n",
	   ps->prefix, AVERAGE (ps->propagations / 1e6f, ps->seconds));
   fprintf (ps->out, "%s%.1f megavisits/second\n",
	   ps->prefix, AVERAGE (ps->visits / 1e6f, ps->seconds));
   fprintf (ps->out, "%sprobing %.1f seconds %.0f%%\n",
           ps->prefix, ps->flseconds, PERCENT (ps->flseconds, ps->seconds));
#ifdef STATS
   fprintf (ps->out,
	   "%srecycled %.1f MB in %u reductions\n",
	   ps->prefix, ps->rrecycled / (double) (1 << 20), ps->reductions);
   fprintf (ps->out,
	   "%srecycled %.1f MB in %u simplifications\n",
	   ps->prefix, ps->srecycled / (double) (1 << 20), ps->simps);
#else
   fprintf (ps->out, "%s%u simplifications\n", ps->prefix, ps->simps);
   fprintf (ps->out, "%s%u reductions\n", ps->prefix, ps->reductions);
   fprintf (ps->out, "%s%.1f MB recycled\n", ps->prefix, ps->recycled / (double) (1 << 20));
#endif
   fprintf (ps->out, "%s%.1f MB maximally allocated\n",
	    ps->prefix, picosat_max_bytes_allocated (ps) / (double) (1 << 20));
}

#ifndef NGETRUSAGE
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/unistd.h>
#endif

double
picosat_time_stamp (void)
{
  double res = -1;
#ifndef NGETRUSAGE
  struct rusage u;
  res = 0;
  if (!getrusage (RUSAGE_SELF, &u))
    {
      res += u.ru_utime.tv_sec + 1e-6 * u.ru_utime.tv_usec;
      res += u.ru_stime.tv_sec + 1e-6 * u.ru_stime.tv_usec;
    }
#endif
  return res;
}

double
picosat_seconds (PS * ps)
{
  check_ready (ps);
  return ps->seconds;
}

void
picosat_print (PS * ps, FILE * file)
{
#ifdef NO_BINARY_CLAUSES
  Lit * lit, *other, * last;
  Ltk * stack;
#endif
  Lit **q, **eol;
  Cls **p, *c;
  unsigned n;

  if (ps->measurealltimeinlib)
    enter (ps);
  else
    check_ready (ps);

  n = 0;
  n +=  ps->alshead - ps->als;

  for (p = SOC; p != EOC; p = NXC (p))
    {
      c = *p;

      if (!c)
	continue;

#ifdef TRACE
      if (c->collected)
	continue;
#endif
      n++;
    }

#ifdef NO_BINARY_CLAUSES
  last = int2lit (ps, -ps->max_var);
  for (lit = int2lit (ps, 1); lit <= last; lit++)
    {
      stack = LIT2IMPLS (lit);
      eol = stack->start + stack->count;
      for (q = stack->start; q < eol; q++)
	if (*q >= lit)
	  n++;
    }
#endif

  fprintf (file, "p cnf %d %u\n", ps->max_var, n);

  for (p = SOC; p != EOC; p = NXC (p))
    {
      c = *p;
      if (!c)
	continue;

#ifdef TRACE
      if (c->collected)
	continue;
#endif

      eol = end_of_lits (c);
      for (q = c->lits; q < eol; q++)
	fprintf (file, "%d ", LIT2INT (*q));

      fputs ("0\n", file);
    }

#ifdef NO_BINARY_CLAUSES
  last = int2lit (ps, -ps->max_var);
  for (lit = int2lit (ps, 1); lit <= last; lit++)
    {
      stack = LIT2IMPLS (lit);
      eol = stack->start + stack->count;
      for (q = stack->start; q < eol; q++)
	if ((other = *q) >= lit)
	  fprintf (file, "%d %d 0\n", LIT2INT (lit), LIT2INT (other));
    }
#endif

  {
    Lit **r;
    for (r = ps->als; r < ps->alshead; r++)
      fprintf (file, "%d 0\n", LIT2INT (*r));
  }

  fflush (file);

  if (ps->measurealltimeinlib)
    leave (ps);
}

void
picosat_enter (PS * ps)
{
  enter (ps);
}

void
picosat_leave (PS * ps)
{
  leave (ps);
}

void
picosat_message (PS * ps, int vlevel, const char * fmt, ...)
{
  va_list ap;

  if (vlevel > ps->verbosity)
    return;

  fputs (ps->prefix, ps->out);
  va_start (ap, fmt);
  vfprintf (ps->out, fmt, ap);
  va_end (ap);
  fputc ('\n', ps->out);
}

int
picosat_changed (PS * ps)
{
  int res;

  check_ready (ps);
  check_sat_state (ps);

  res = (ps->min_flipped <= ps->saved_max_var);
  assert (!res || ps->saved_flips != ps->flips);

  return res;
}

void
picosat_reset_phases (PS * ps)
{
  rebias (ps);
}

void
picosat_reset_scores (PS * ps)
{
  Rnk * r;
  ps->hhead = ps->heap + 1;
  for (r = ps->rnks + 1; r <= ps->rnks + ps->max_var; r++)
    {
      CLR (r);
      hpush (ps, r);
    }
}

void
picosat_remove_learned (PS * ps, unsigned percentage)
{
  enter (ps);
  reset_incremental_usage (ps);
  reduce (ps, percentage);
  leave (ps);
}

void
picosat_set_global_default_phase (PS * ps, int phase)
{
  check_ready (ps);
  ABORTIF (phase < 0, "API usage: 'picosat_set_global_default_phase' "
                      "with negative argument");
  ABORTIF (phase > 3, "API usage: 'picosat_set_global_default_phase' "
                      "with argument > 3");
  ps->defaultphase = phase;
}

void
picosat_set_default_phase_lit (PS * ps, int int_lit, int phase)
{
  unsigned newphase;
  Lit * lit;
  Var * v;

  check_ready (ps);

  lit = import_lit (ps, int_lit, 1);
  v = LIT2VAR (lit);

  if (phase)
    {
      newphase = (int_lit < 0) == (phase < 0);
      v->defphase = v->phase = newphase;
      v->usedefphase = v->assigned = 1;
    }
  else
    {
      v->usedefphase = v->assigned = 0;
    }
}

void
picosat_set_more_important_lit (PS * ps, int int_lit)
{
  Lit * lit;
  Var * v;
  Rnk * r;

  check_ready (ps);

  lit = import_lit (ps, int_lit, 1);
  v = LIT2VAR (lit);
  r = VAR2RNK (v);

  ABORTIF (r->lessimportant, "can not mark variable more and less important"); 

  if (r->moreimportant)
    return;

  r->moreimportant = 1;

  if (r->pos)
    hup (ps, r);
}

void
picosat_set_less_important_lit (PS * ps, int int_lit)
{
  Lit * lit;
  Var * v;
  Rnk * r;

  check_ready (ps);

  lit = import_lit (ps, int_lit, 1);
  v = LIT2VAR (lit);
  r = VAR2RNK (v);

  ABORTIF (r->moreimportant, "can not mark variable more and less important"); 

  if (r->lessimportant)
    return;

  r->lessimportant = 1;

  if (r->pos)
    hdown (ps, r);
}

#ifndef NADC

unsigned 
picosat_ado_conflicts (PS * ps)
{
  check_ready (ps);
  return ps->adoconflicts;
}

void
picosat_disable_ado (PS * ps)
{
  check_ready (ps);
  assert (!ps->adodisabled);
  ps->adodisabled = 1;
}

void
picosat_enable_ado (PS * ps)
{
  check_ready (ps);
  assert (ps->adodisabled);
  ps->adodisabled = 0;
}

void
picosat_set_ado_conflict_limit (PS * ps, unsigned newadoconflictlimit)
{
  check_ready (ps);
  ps->adoconflictlimit = newadoconflictlimit;
}

#endif

void
picosat_simplify (PS * ps)
{
  enter (ps);
  reset_incremental_usage (ps);
  simplify (ps, 1);
  leave (ps);
}

int
picosat_haveados (void)
{
#ifndef NADC
  return 1;
#else
  return 0;
#endif
}

void
picosat_save_original_clauses (PS * ps)
{
  if (ps->saveorig) return;
  ABORTIF (ps->oadded, "API usage: 'picosat_save_original_clauses' too late");
  ps->saveorig = 1;
}

void picosat_set_interrupt (PicoSAT * ps,
                            void * external_state,
			    int (*interrupted)(void * external_state)) 
{
  ps->interrupt.state = external_state;
  ps->interrupt.function = interrupted;
}

int
picosat_deref_partial (PS * ps, int int_lit) 
{
  check_ready (ps);
  check_sat_state (ps);
  ABORTIF (!int_lit, "API usage: can not partial deref zero literal");
  ABORTIF (ps->mtcls, "API usage: deref partial after empty clause generated");
  ABORTIF (!ps->saveorig, "API usage: 'picosat_save_original_clauses' missing");

#ifdef STATS
  ps->derefs++;
#endif

  if (!ps->partial)
    minautarky (ps);

  return pderef (ps, int_lit);
}
