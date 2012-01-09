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

#include <cstdio>
#include <cstring>
#include <cstdarg>

extern "C" {
#include <ctype.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/unistd.h>
#include <malloc.h>
#include <unistd.h>
};

#ifndef NLOGPRECO
#include <iostream>
#define LOG(code) \
  do { std::cout << prfx << "LOG " << code << std::endl; } while (false)
#else
#define LOG(code) do { } while (false)
#endif

#if 0
#include <iostream>
#define COVER(code) \
  do { std::cout << prfx << "COVER " << __FUNCTION__ << ' ' \
       << __LINE__ << ' ' << code << std::endl; } while (false)
#else
#define COVER(code) do { } while (false)
#endif

//#define PRECOCHECK
#ifdef PRECOCHECK
#warning "PRECOCHECK enabled"
#endif

#ifndef NSTATSPRECO
#define INC(s) do { stats.s++; } while (0)
#else
#define INC(s) do { } while (0)
#endif

#ifdef CHECKWITHPICOSAT
extern "C" {
#include "../picosat/picosat.h"
};
#endif

namespace PrecoSat {

template<class T> inline static void swap (T & a, T & b)
  { T tmp = a; a = b; b = tmp; }

template<class T> inline static const T min (const T a, const T b)
  { return a < b ? a : b; }

template<class T> inline static const T max (const T a, const T b)
  { return a > b ? a : b; }

template<class T> inline static void fix (T * & p, long moved)
  { char * q = (char*) p; q += moved; p = (T*) q; }

inline static int logsize (int size) { return (128 >> size) + 1; }

static inline Val lit2val (int lit) { return (lit & 1) ? -1 : 1; }

static inline Cls * lit2conflict (Cls & bins, int a, int b) {
  bins.lits[0] = a;
  bins.lits[1] = b;
  assert (!bins.lits[2]);
  return &bins;
}

static inline unsigned ggt (unsigned a, unsigned b)
  { while (b) { unsigned r = a % b; a = b, b = r; } return a; }

inline static unsigned  minuscap (unsigned a, unsigned b)
  { return (a <= b) ? 0 : (a - b); }

static inline double mb (size_t bytes)
  { return bytes / (double) (1<<20); }

static inline bool sigsubs (unsigned s, unsigned t)
  { return !(s&~t); }

static inline unsigned listig (int lit)
  { return (1u << (31u & (unsigned)(lit/2))); }

static inline double average (double a, double b) { return b ? a/b : 0; }

static inline double percent (double a, double b) { return 100*average(a,b); }

static unsigned gcd (unsigned a, unsigned b) {
  unsigned tmp;
  assert (a), assert (b);
  if (a < b) swap (a, b);
  while (b) tmp = b, b = a % b, a = tmp;
  return a;
}

#if !defined(NDEBUG) || CHECKWITHPICOSAT
int ulit2ilit (int u) { return (u/2)*((u&1)?-1:1); }
#endif

static bool parity (unsigned x)
  { bool res = false; while (x) res = !res, x &= x-1; return res; }

class Progress {
    Solver * solver;
    int countdown, interval;
    char type;
  public:
    Progress (Solver * s, char t, int c = 111) :
      solver (s), countdown (c), interval (c), type (t)
    { assert (countdown > 0); }
    void tick () {
      if (!solver->hasterm ()) return;
      if (countdown--) return;
      solver->report (2, type);
      countdown = interval;
    }
};

struct LitScore {
  int lit, score;
  LitScore (int l, int s) : lit (l), score (s) { }
};

struct FreeVarsLt {
  LitScore a, b;
  FreeVarsLt (LitScore c, LitScore d) : a (c), b (d) { }
  operator bool () const {
    if (b.score < a.score) return true;
    if (b.score > a.score) return false;
    return a.lit < b.lit;
  }
};

template<class T, class L> class Sorter {
  Mem & mem;
  Stack<long> stack;
  const static long limit = 10;
  static void swap (T & a, T & b) { T tmp = a; a = b; b = tmp; }
  static void cwap (T & a, T & b) {
    if (L (b, a)) swap (a, b);
    assert (!L (b, a)); // otherwise 'L' inconsistent
  }
  void part (T * a, long l, long r, long & i) {
    long j = r; i = l - 1;
    T pivot = a[j];
    for (;;) {
      while (L (a[++i], pivot)) ;
      while (L (pivot, a[--j])) if (j == l) break;
      if (i >= j) break;
      swap (a[i], a[j]);
    }
    swap (a[i], a[r]);
  }
  void qsort (T * a, long n) {
    long l = 0, r = n - 1;
    if (r - l <= limit) return;
    long m, ll, rr, i;
    assert (!stack);
    for (;;) {
      m = (l + r) / 2;
      swap (a[m], a[r-1]);
      cwap (a[l], a[r-1]);
      cwap (a[l], a[r]);
      cwap (a[r-1], a[r]);
      part (a, l+1, r-1, i);
      if (i - l < r - i) ll = i+1, rr = r, r = i-1;
      else ll = l, rr = i-1, l = i+1;
      if (r - l > limit) {
	assert (rr - ll > limit);
	stack.push (mem, ll);
	stack.push (mem, rr);
      } else if (rr - ll > limit) l = ll, r = rr;
      else if (stack) r = stack.pop (), l = stack.pop ();
      else break;
    }
  }
  void isort (T * a, long n) {
    for (long i = n-1; i > 0; i--) cwap (a[i-1], a[i]);
#ifndef NDEBUG
    for (long i = 1; i < n; i++) assert (!L (a[i], a[0]));
#endif
    for (long i = 2; i < n; i++) {
      long j = i;
      T pivot = a[i];
      while (L (pivot, a[j-1]))
	a[j] = a[j-1], j--, assert (j > 0);
      a[j] = pivot;
    }
  }
  void check (T * a, long n) {
#ifndef NDEBUG
    for (long i = 0; i < n-1; i++) assert (!L (a[i+1], a[i]));
#else
    (void) a, (void) n;
#endif
  }
public:
  Sorter (Mem & m) : mem (m) { }
  ~Sorter () { stack.release (mem); }
  void sort (T * a, long n) {
    if (!n) return;
    qsort (a, n);
    isort (a, n);
    check (a, n);
  }
};

}

using namespace PrecoSat;

inline unsigned RNG::next () {
  unsigned res = state;
  state *= 1664525u;
  state += 1013904223u;
  return res;
}

inline bool RNG::oneoutof (unsigned spread) {
  unsigned tmp = next (), mask = 0xffffffffu, shift = 16, res;
  if (!spread) return true;
  for (;;) {
    mask >>= shift;
    if (mask < spread) break;
    tmp ^= tmp >> shift;
    tmp &= mask;
    shift /= 2;
  }
  res = tmp % spread;
  return res == spread/2;
}

inline bool RNG::choose () {
  unsigned tmp = next ();
  bool res = 1 & ((tmp >> 13) ^ (tmp >> 9) ^ (tmp >> 5));
  return res;
}

inline size_t Cls::bytes (int n) {
  return sizeof (Cls) + (n - 3 + 1) * sizeof (int);
}

inline size_t Cls::bytes () const { return bytes (size); }

inline Anchor<Cls> & Solver::anchor (Cls * c) {
  if (c->binary) return binary;
  if (!c->lnd) return original;
  if (c->fresh) return fresh;
  return learned[c->glue];
}

inline int Cls::minlit () const {
  int res = INT_MAX, other;
  for (const int * p = lits; (other = *p); p++)
    if (other < res) res = other;
  return res;
}

inline bool Cls::contains (int lit) const {
  if (!(sig & listig (lit))) return false;
  for (const int * p = lits; *p; p++)
    if (*p == lit) return true;
  return false;
}

inline void Solver::setsig (Cls * cls) {
  int except = cls->minlit (), lit;
  unsigned fwsig = 0, bwsig = 0;
  for (const int * p = cls->lits; (lit = *p); p++) {
    unsigned sig = listig (lit);
    bwsig |= sig;
    if (lit != except) fwsig |= sig;
  }
  cls->sig = bwsig;
  for (const int * p = cls->lits; (lit = *p); p++)
    bwsigs[lit] |= bwsig;
  if (fwsigs)
    fwsigs[except] |= fwsig;
}

inline unsigned Solver::litsig () {
  unsigned res = 0;
  for (int i = 0; i < lits; i++)
    res |= listig (lits[i]);
  return res;
}

inline bool Solver::clt (int a, int b) const {
  Var & u = vars[a/2], &v = vars[b/2];
  int l = u.dlevel, k = v.dlevel;
  if (l < 0) return false;
  if (k < 0) return true;
  if (l < k) return true;
  if (l > k) return false;
  Val r = vals[a], s = vals[b];
  if (r < s) return true;
  return false;
}

inline void Solver::connect (Cls * c) {
  for (int i = 0; i <= 1; i++) {
    int lit, * r = c->lits + i, * q = r, best = *q;
    for (int * p = q + 1; (lit = *p); p++)
      if (clt (best, lit)) q = p, best = lit;
    *q = *r, *r = best;
  }
  assert (c->lits[0] && c->lits[1]);
  Anchor<Cls> & a = anchor (c);
  if (!connected (a, c)) push (a, c);
  if (c->binary) {
    for (int i = 0; i <= 1; i++)
      occs[c->lits[i]].bins.push (mem, c->lits[!i]);
  } else {
    for (int i = 0; i <= 1; i++)
      occs[c->lits[i]].large.push (mem, Occ (c->lits[!i], c));
  }
  if (orgs)
    for (const int * p = c->lits; *p; p++)
      orgs[*p].push (mem, c);
  if (fwds) fwds[c->minlit ()].push (mem, c);
  setsig (c);
}

inline void Solver::disconnect (Cls * c) {
  assert (!c->locked && !c->dirty && !c->trash);
  int l0 = c->lits[0], l1 = c->lits[1];
  assert (l0 && l1);
  Anchor<Cls> & a = anchor (c);
  if (connected (a, c)) dequeue (a, c);
  if (c->binary) {
    occs[l0].bins.trymove(l1);
    occs[l1].bins.trymove(l0);
  } else {
    occs[l0].large.trymove(Occ(0,c));
    occs[l1].large.trymove(Occ(0,c));
  }
  if (fwds) fwds[c->minlit ()].remove (c);
  if (!orgs || c->lnd) return;
  for (const int * p = c->lits; *p; p++)
    orgs[*p].remove (c);
}

inline Rnk * Solver::rnk (const Var * v) { return rnks + (v - vars); }
inline Var * Solver::var (const Rnk * r) { return vars + (r - rnks); }

inline Val Solver::fixed (int lit) const {
  return vars[lit/2].dlevel ? 0 : vals[lit];
}

inline void Solver::collect (Cls * cls) {
  assert (!cls->locked && !cls->dirty && !cls->trash && !cls->gate);
  if (cls->binary) assert (stats.clauses.bin), stats.clauses.bin--;
  else if (cls->lnd) assert (stats.clauses.lnd), stats.clauses.lnd--;
  else assert (stats.clauses.orig), stats.clauses.orig--;
  if (!cls->lnd) assert (stats.clauses.irr), stats.clauses.irr--;
  size_t bytes = cls->bytes ();
#ifdef PRECOCHECK
  if (!cls->lnd) {
    for (const int * p = cls->lits; *p; p++)
      check.push (mem, *p);
    check.push (mem, 0);
  }
#endif
#ifndef NLOGPRECO
  dbgprint ("LOG recycle clause ", cls);
#endif
  mem.deallocate (cls, bytes);
  stats.collected += bytes;
  simplified = true;
}

void Solver::touchblkd (int lit) {
  assert (blkmode);
  int idx = lit/2, notlit = lit^1;
  Var * v = vars + idx;
  if (v->type != FREE) return;
  assert (!val (lit) && !repr[lit]);
  Rnk * r = blks + notlit;
  assert (orgs);
  int nh = -orgs[lit], oh = r->heat; r->heat = nh;
  if (oh == nh && schedule.block.contains (r)) return;
  if (oh == nh) LOG ("touchblkd " << notlit << " again " << nh);
  else LOG ("touchblkd " << notlit << " from " << oh << " to " << nh);
  if (!schedule.block.contains (r)) schedule.block.push (mem, r);
  else if (nh > oh) schedule.block.up (r);
  else if (nh < oh) schedule.block.down (r);
}

void Solver::touchelim (int lit) {
  assert (elimode);
  lit &= ~1;
  int idx = lit/2;
  Var * v = vars + idx;
  if (v->type != FREE) return;
  assert (!val (lit) && !repr[lit]);
  if (idx == elimvar) return;
  Rnk * r = elms + idx;
  assert (orgs);
  int pos = orgs[lit];
  int neg = orgs[1^lit];
#if 0
  long long tmp = -(((long long)pos) * (long long) neg);
  tmp += pos + neg;
  int nh =  (tmp >= INT_MIN) ? (int) tmp : INT_MIN;
#else
  int nh = -(pos + neg);
#endif
  int oh = r->heat; r->heat = nh;
  if (oh == nh && schedule.elim.contains (r)) return;
  if (oh == nh) LOG ("touchelim " << lit << " again " << nh);
  else LOG ("touchelim " << lit << " from " << oh << " to " << nh);
  if (pos > 1 && neg > 1 && nh <= opts.elimin) {
    if (schedule.elim.contains (r)) {
      schedule.elim.remove (r);
    }
  } else if (!schedule.elim.contains (r)) schedule.elim.push (mem, r);
  else if (nh > oh) schedule.elim.up (r);
  else if (nh < oh) schedule.elim.down (r);
}

void Solver::touchpure (int lit) {
  assert (puremode);
  int idx = lit/2;
  Var * v = vars + idx;
  if (v->onplits) return;
  LOG ("touchpure " << lit);
  v->onplits = true;
  plits.push (mem, idx);
}

void Solver::touch (int lit) {
  assert (elimode + blkmode == 1);
  if (elimode) touchelim (lit);
  else touchblkd (lit);
  if (puremode) touchpure (lit);
}

void Solver::touch (Cls * c) {
  assert (asymode || !c->lnd);
  assert (elimode || blkmode);
  for (int * p = c->lits; *p; p++)
    touch (*p);
}

inline void Solver::recycle (Cls * c) {
  stats.clauses.gc++;
  disconnect (c);
  if (c->trash) trash.remove (c);
  if (c->gate) cleangate ();
  if (c->str) strnd.remove (c);
  if (elimode || blkmode) touch (c);
  collect (c);
}

inline void Solver::dump (Cls * c) {
  assert (!c->dirty);
  if (c->trash) return;
  c->trash = true;
  trash.push (mem, c);
}

inline void Solver::cleantrash () {
  if (!trash) return;
  int old = stats.clauses.orig;
  while (trash) {
    Cls * cls = trash.pop ();
    assert (cls->trash);
    cls->trash = false;
    recycle (cls);
  }
  shrink (old);
}

inline void Solver::gcls (Cls * c) {
  assert (!c->gate);
  c->gate = true;
  gate.push (mem, c);
}

inline void Solver::strcls (Cls * c) {
  assert (!c->str);
  c->str = true;
  strnd.push (mem, c);
}

inline void Solver::cleangate () {
  while (gate) {
    Cls * cls = gate.pop ();
    assert (cls->gate); cls->gate = false;
  }
  gatepivot = 0;
  gatestats = 0;
  gatelen = 0;
}

inline void Solver::recycle (int lit) {
  LOG ("recycle literal " << lit);
  assert (!level);
#ifndef NDEBUG
  Var * v = vars + (lit/2); Vrt t = v->type;
  assert (t == FIXED || t == PURE || t == ZOMBIE || t == ELIM || t == AUTARK);
#endif
  if (orgs) {
    Stack<Cls*> & s = orgs[lit];
    while (s) recycle (s.top ());
    s.release (mem);
  }
  occs[lit].bins.release (mem);
  occs[lit].large.release (mem);
}

Opt::Opt (const char * n, int v, int * vp, int mi, int ma) :
name (n), valptr (vp), min (mi), max (ma)
{
  assert (min <= v);
  assert (v <= max);
  *vp = v;
}

bool Opts::set (const char * opt, int val) {
  for (Opt * o = opts.begin (); o < opts.end (); o++)
    if (!strcmp (o->name, opt)) {
      if (val < o->min) return false;
      if (val > o->max) return false;
      *o->valptr = val;
      return true;
    }
  return false;
}

bool Opts::set (const char * opt, const char * val) {
  if (strcmp (opt, "output")) return false;
  output = val;
  return true;
}

void Opts::add (Mem & mem, const char * n, int v, int * vp, int mi, int ma) {
  opts.push (mem, Opt (n, v, vp, mi, ma));
}

void Opts::printoptions (FILE * file, const char * prfx) const {
  assert (prfx);
  int lenprfx = strlen (prfx);
  fputs (prfx, file); int pos = 0;
  const Opt * o = opts.begin ();
  while (o < opts.end ()) {
    char line[80];
    sprintf (line, " --%s=%d", o->name, *o->valptr);
    int len = strlen (line); assert (len < 80);
    if (len + pos >= 77 - lenprfx) {
      fprintf (file, "\n%s", prfx);
      pos = lenprfx;
    }
    fputs (line, file);
    pos += len;
    o++;
  }
  fputc ('\n', file);

  if (output)
    fprintf (file, "%s\n%s --output=%s\n", prfx, prfx, output);
}

void Solver::initbwsigs () {
  assert (!bwsigs);
  size_t bytes = 2 * size * sizeof *bwsigs;
  bwsigs = (unsigned*) mem.callocate (bytes);
}

void Solver::rszbwsigs (int newsize) {
  assert (bwsigs);
  size_t old_bytes = 2 * size * sizeof *bwsigs;
  size_t new_bytes = 2 * newsize * sizeof *bwsigs;
  bwsigs = (unsigned*) mem.recallocate (bwsigs, old_bytes, new_bytes);
}

void Solver::clrbwsigs () {
  size_t bytes = 2 * size * sizeof *bwsigs;
  memset (bwsigs, 0, bytes);
}

void Solver::delbwsigs () {
  assert (bwsigs);
  size_t bytes = 2 * size * sizeof *bwsigs;
  mem.deallocate (bwsigs, bytes);
  bwsigs = 0;
}

void Solver::initorgs () {
  assert (!orgs);
  size_t bytes = 2 * (maxvar + 1) * sizeof *orgs;
  orgs = (Orgs*) mem.callocate (bytes);
}

void Solver::delorgs () {
  assert (orgs);
  for (int lit = 2; lit <= 2*maxvar+1; lit++) orgs[lit].release (mem);
  size_t bytes = 2 * (maxvar + 1) * sizeof *orgs;
  mem.deallocate (orgs, bytes);
  orgs = 0;
}

void Solver::initfwds () {
  size_t bytes = 2 * (maxvar + 1) * sizeof *fwds;
  fwds = (Fwds*) mem.callocate (bytes);
}

void Solver::delfwds () {
  assert (fwds);
  for (int lit = 2; lit <= 2*maxvar+1; lit++) fwds[lit].release (mem);
  size_t bytes = 2 * (maxvar + 1) * sizeof *fwds;
  mem.deallocate (fwds, bytes);
  fwds = 0;
}

void Solver::initfwsigs () {
  assert (!fwsigs);
  size_t bytes = 2 * (maxvar + 1) * sizeof *fwsigs;
  fwsigs = (unsigned*) mem.callocate (bytes);
}

void Solver::delfwsigs () {
  assert (fwsigs);
  size_t bytes = 2 * (maxvar + 1) * sizeof *fwsigs;
  mem.deallocate (fwsigs, bytes);
  fwsigs = 0;
}

void Solver::initprfx (const char * newprfx) {
  prfx = (char*) mem.allocate (strlen (newprfx) + 1);
  strcpy (prfx, newprfx);
}

void Solver::delprfx () {
  assert (prfx);
  mem.deallocate (prfx, strlen (prfx) + 1);
}

#define OPT(n,v,mi,ma) \
do { opts.add (mem, # n, v, &opts.n, mi, ma); } while (0)

bool Solver::hasterm (void) {
  if (!terminitialized) initerm ();
  return terminal;
}

void Solver::initerm (void) {
  if (opts.terminal < 2) terminal = opts.terminal;
  else terminal = (out == stdout) && isatty(1);
  terminitialized = true;
}

void Solver::init (int initialmaxvar) {
  maxvar = initialmaxvar;
  size = 1;
  while (maxvar >= size) size *= 2;
  queue = 0;
  queue2 = 0;
  level = 0;
  conflict = 0;
  hinc = 128;
  agility = 0;
  typecount = 1;
  lastype = 0;
  out = stdout;
  terminitialized = false;
  measure = true;
  iterating = false;
  simpmode = false;
  elimode = false;
  blkmode = false;
  puremode = false;
  asymode = false;
  autarkmode = false;
  extending = false;
  assert (!initialized);

  iirfs = 0;

  blks = (Rnk*) mem.allocate (2 * size * sizeof *blks);
  jwhs = (int*) mem.callocate (2 * size * sizeof *jwhs);
  occs = (Occs*) mem.callocate (2 * size * sizeof *occs);
  repr = (int*) mem.callocate (2 * size * sizeof *repr);
  vals = (Val*) mem.callocate (2 * size * sizeof *vals);

  elms = (Rnk*) mem.allocate (size * sizeof *elms);
  rnks = (Rnk*) mem.allocate (size * sizeof *rnks);
  vars = (Var*) mem.callocate (size * sizeof *vars);

  for (int i = 1; i <= maxvar; i++)
    vars[i].dlevel = -1;

  for (Rnk * p = rnks + maxvar; p > rnks; p--)
    p->heat = 0, p->pos = -1, schedule.decide.push (mem, p);

  for (Rnk * p = elms + maxvar; p > elms; p--) p->heat = 0, p->pos = -1;
  for (Rnk * p = blks + 2*maxvar+1; p > blks+1; p--) p->heat = 0, p->pos = -1;

  bwsigs = 0;
  initbwsigs ();

  orgs = 0;
  fwds = 0;
  fwsigs = 0;

  frames.push (mem, Frame (trail));

  empty.lits[0] = 0;
  dummy.lits[2] = 0;

  int m = INT_MIN, M = INT_MAX;
  OPT (plain,0,0,1);
  OPT (rtc,0,0,2);
  OPT (quiet,0,0,1);
  OPT (verbose,0,0,2);
  OPT (terminal,2,0,2);
  OPT (print,0,0,1);
  OPT (check,0,0,2);
  OPT (order,2,1,9);
  OPT (simprd,20,0,M); OPT (simpinc,26,0,100); OPT (simprtc,0,0,2);
  OPT (cutrail,1,0,1);
  OPT (merge,1,0,1);
  OPT (dominate,1,0,1);
  OPT (maxdoms,5*1000*1000,0,M);
  OPT (otfs,1,0,1);
  OPT (block,1,0,1);
    OPT (blockrtc,1,0,2); OPT (blockimpl,1,0,1);
    OPT (blockprd,10,1,M); OPT (blockint,300*1000,0,M);
    OPT (blockotfs,1,0,1);
    OPT (blockreward,100,0,10000);
    OPT (blockboost,3,0,100);
  OPT (heatinc,10,0,100);
  OPT (luby,1,0,1);
  OPT (restart,1,0,1); OPT (restartint,10,1,M);
  OPT (restartinner,10,0,1000); OPT (restartouter,10,0,1000);
  OPT (restartminlevel,10,0,1000);
  OPT (rebias,1,0,1); OPT (rebiasint,1000,1,M); OPT (rebiasorgonly,0,0,1);
  OPT (probe,1,0,1);
    OPT (probeint,200*1000,1000,M); OPT (probeprd,5,1,M);
    OPT (probertc,0,0,2); OPT (probereward,100,0,10000);
    OPT (probeboost,6,0,10000);
  OPT (decompose,1,0,1);
  OPT (autark,0,0,1); OPT (autarkdhs,16,0,63);
  OPT (phase,2,0,3);
  OPT (inverse,0,0,1); OPT (inveager,0,0,1);
  OPT (mtfall,0,0,1); OPT (mtfrev,1,0,1);
  OPT (bumpuip,1,0,1);
    OPT (bumpsort,1,0,1); OPT (bumprev,1,0,1);
    OPT (bumpturbo,0,0,1); OPT (bumpbulk,0,0,1);
  OPT (fresh,50,0,100);
  OPT (glue,Cls::MAXGLUE,0,Cls::MAXGLUE);
    OPT (slim,1,0,1); OPT (sticky,2,0,Cls::MAXGLUE);
  OPT (redsub,0,0,M);
  OPT (minimize,2,0,4); OPT (maxdepth,1000,2,10000); OPT (strength,100,0,M);
  OPT (elim,1,0,1);
    OPT (elimgain,0,m/2,M/2);
    OPT (elimint,300*1000,0,M); OPT (elimprd,20,1,M);
    OPT (elimrtc,0,0,2);
    OPT (elimin,-10000,-m,M);
    OPT (elimclim,20,0,M);
    OPT (elimboost,1,0,100);
    OPT (elimreward,100,0,10000);
    OPT (elimasym,3,1,100);
    OPT (elimasymint,100000,100,M);
    OPT (elimasymreward,1000,0,M);
  OPT (dynbw,0,0,1); OPT (fw,1,0,1);
  OPT (fwmaxlen,100,0,M); OPT (bwmaxlen,1000,0,M); OPT (reslim,20,0,M);
  OPT (blkmaxlen,1000,0,M);
  OPT (subst,1,0,1); OPT (ands,1,0,1); OPT (xors,1,0,1); OPT (ites,1,0,1);
  OPT (minlimit,500,10,10000); OPT (maxlimit,3*1000*1000,100,M);
  OPT (dynred,-1,-1,100000);
  OPT (liminitmode,0,0,1);
  OPT (limincmode,2,0,2);
  OPT (liminitconst,5000,0,1000*1000);
  OPT (liminitmax,20000,0,10*1000*1000);
  OPT (liminitpercent,10,0,1000);
  OPT (liminconst1,1000,0,100*1000);
  OPT (liminconst2,1000,0,100*1000);
  OPT (limincpercent,10,0,1000);
  OPT (enlinc,20,0,1000);
  OPT (shrink,2,0,2); OPT (shrinkfactor,100,1,1000);
  OPT (random,1,0,1); OPT (spread,2000,0,M); OPT (seed,0,0,M);
  OPT (skip,25,0,100);
  opts.set ("output", (const char*) 0);

  initprfx ("c ");
  initialized = true;
#ifdef CHECKWITHPICOSAT
  memset (&picosatcheck, 0, sizeof picosatcheck);
  picosat_init ();
  picosatcheck.init++;
#endif
}

void Solver::initiirfs () {
  if (opts.order < 2) return;
  size_t bytes = size * (opts.order - 1) * sizeof *iirfs;
  iirfs =  (int *) mem.allocate (bytes);
  for (Var * v = vars + 1; v <= vars + maxvar; v++)
    for (int i = 1; i < opts.order; i++)
      iirf (v, i) = -1;
}

void Solver::rsziirfs (int newsize) {
  if (opts.order < 2) return;
  size_t old_bytes = size * (opts.order - 1) * sizeof *iirfs;
  size_t new_bytes = newsize * (opts.order - 1) * sizeof *iirfs;
  iirfs =  (int *) mem.reallocate (iirfs, old_bytes, new_bytes);
}

void Solver::deliirfs () {
  assert (iirfs);
  assert (opts.order >= 2);
  size_t bytes = size * (opts.order - 1) * sizeof *iirfs;
  mem.deallocate (iirfs, bytes);
  iirfs = 0;
}

void Solver::delclauses (Anchor<Cls> & anchor) {
  Cls * prev;
  for (Cls * p = anchor.head; p; p = prev) {
    p->bytes (); prev = p->prev; mem.deallocate (p, p->bytes ());
  }
  anchor.head = anchor.tail  = 0;
}

void Solver::reset () {
  assert (initialized);
  initialized = false;
  delprfx ();
  size_t bytes;
  for (int lit = 2; lit <= 2 * maxvar + 1; lit++) occs[lit].bins.release (mem);
  for (int lit = 2; lit <= 2 * maxvar + 1; lit++) occs[lit].large.release (mem);

  bytes = 2 * size * sizeof *blks; mem.deallocate (blks, bytes);
  bytes = 2 * size * sizeof *jwhs; mem.deallocate (jwhs, bytes);
  bytes = 2 * size * sizeof *occs; mem.deallocate (occs, bytes);
  bytes = 2 * size * sizeof *repr; mem.deallocate (repr, bytes);
  bytes = 2 * size * sizeof *vals; mem.deallocate (vals, bytes);

  bytes = size * sizeof *elms; mem.deallocate (elms, bytes);
  bytes = size * sizeof *rnks; mem.deallocate (rnks, bytes);
  bytes = size * sizeof *vars; mem.deallocate (vars, bytes);

  delclauses (original);
  delclauses (binary);
  for (int glue = 0; glue <= opts.glue; glue++)
    delclauses (learned[glue]);
  delclauses (fresh);
  if (iirfs) deliirfs ();
  if (orgs) delorgs ();
  if (fwds) delfwds ();
  if (bwsigs) delbwsigs ();
  if (fwsigs) delfwsigs ();
  schedule.block.release (mem);
  schedule.elim.release (mem);
  schedule.decide.release (mem);
  opts.opts.release (mem);
  trail.release (mem);
  frames.release (mem);
  levels.release (mem);
  trash.release (mem);
  gate.release (mem);
  strnd.release (mem);
  saved.release (mem);
  elits.release (mem);
  plits.release (mem);
  flits.release (mem);
  fclss.release (mem);
#ifdef PRECOCHECK
  check.release (mem);
#endif
  units.release (mem);
  lits.release (mem);
  seen.release (mem);
  assert (!mem);
  memset (this, 0, sizeof *this);
#ifdef CHECKWITHPICOSAT
  picosat_reset ();
#endif
}

void Solver::fxopts () {
  assert (!opts.fixed);
  opts.fixed = true;
  initiirfs ();
  if (!opts.plain) return;
  opts.merge = 0;
  opts.block = 0;
  opts.autark = 0;
  opts.dominate = 0;
  opts.rebias = 0;
  opts.probe = 0;
  opts.decompose = 0;
  opts.minimize = 2;
  opts.elim = 0;
  opts.random = 0;
  opts.otfs = 0;
  opts.dynred = -1;
}

void Solver::propts () {
  assert (opts.fixed);
  opts.printoptions (out, prfx);
  fflush (out);
}

void Solver::prstats () {
  double overalltime = stats.seconds ();
  fprintf (out, "%s%d conflicts, %d decisions, %d random\n", prfx,
           stats.conflicts, stats.decisions, stats.random);
  fprintf (out, "%s%d iterations, %d restarts, %d skipped\n", prfx,
           stats.iter, stats.restart.count, stats.restart.skipped);
  fprintf (out, "%s%d enlarged, %d shrunken, %d rescored, %d rebiased\n", prfx,
           stats.enlarged, stats.shrunken, stats.rescored, stats.rebias.count);
  fprintf (out, "%s%d simplifications, %d reductions\n", prfx,
           stats.simps, stats.reductions);
  fprintf (out, "%s\n", prfx);
  fprintf (out, "%sarty: %.2f ands %.2f xors average arity\n", prfx,
           average (stats.subst.ands.len, stats.subst.ands.count),
           average (stats.subst.xors.len, stats.subst.xors.count));
  fprintf (out, "%sautk: %d autarkies of %.1f avg size\n",
           prfx,
	   stats.autarks.count,
	   average (stats.autarks.size, stats.autarks.count));
  fprintf (out, "%sautk: dhs %d %d %d %d %d %d\n",
           prfx,
	   stats.autarks.dh[0],
	   stats.autarks.dh[1],
	   stats.autarks.dh[2],
	   stats.autarks.dh[3],
	   stats.autarks.dh[4],
	   stats.autarks.dh[5]);
  fprintf (out,
           "%sback: %d track with %.1f avg cuts, %d jumps of %.1f avg len\n",
	   prfx,
	   stats.back.track, average (stats.back.cuts, stats.back.track),
	   stats.back.jump, average (stats.back.dist, stats.back.jump));
  fprintf (out,
           "%sblkd: %lld resolutions, %d phases, %d rounds\n",
	   prfx,
	   stats.blkd.res,
	   stats.blkd.phases, stats.blkd.rounds);
  assert (stats.blkd.all == stats.blkd.impl + stats.blkd.expl);
  fprintf (out,
           "%sblkd: %d = %d implicit + %d explicit\n",
	   prfx,
	   stats.blkd.impl + stats.blkd.expl,
	   stats.blkd.impl, stats.blkd.expl);
  fprintf (out, "%sclss: %d recycled, %d pure, %d autark\n", prfx,
           stats.clauses.gc, stats.clauses.gcpure, stats.clauses.gcautark);
  assert (stats.doms.count >= stats.doms.level1);
  assert (stats.doms.level1 >= stats.doms.probing);
  fprintf (out, "%sdoms: %d dominators, %d high, %d low\n", prfx,
           stats.doms.count,
	   stats.doms.count - stats.doms.level1,
	   stats.doms.level1 - stats.doms.probing);
  fprintf (out, "%selim: %lld resolutions, %d phases, %d rounds\n", prfx,
           stats.elim.resolutions, stats.elim.phases, stats.elim.rounds);
  fprintf (out, "%sextd: %d forced, %d assumed, %d flipped\n", prfx,
           stats.extend.forced, stats.extend.assumed, stats.extend.flipped);
  fprintf (out, "%sglue: %.2f original glue, %.3f slimmed on average\n",
	   prfx,
	   average (stats.glue.orig.sum, stats.glue.orig.count),
	   average (stats.glue.slimmed.sum, stats.glue.slimmed.count));
  long long alllits = stats.lits.added + stats.mins.deleted;
  fprintf (out,
           "%smins: %lld lrnd, %.0f%% del, %lld strng, %lld inv, %d dpth\n",
	   prfx,
           stats.lits.added,
	   percent (stats.mins.deleted, alllits),
	   stats.mins.strong, stats.mins.inverse, stats.mins.depth);
  fprintf (out, "%sotfs: dynamic %d = %d bin + %d trn + %d large\n",
	   prfx,
	   stats.otfs.dyn.bin + stats.otfs.dyn.trn + stats.otfs.dyn.large,
	   stats.otfs.dyn.bin, stats.otfs.dyn.trn, stats.otfs.dyn.large);
  fprintf (out, "%sotfs: static %d = %d bin + %d trn + %d large\n",
	   prfx,
	   stats.otfs.stat.bin + stats.otfs.stat.trn + stats.otfs.stat.large,
	   stats.otfs.stat.bin, stats.otfs.stat.trn, stats.otfs.stat.large);
  fprintf (out, "%sprbe: %d probed, %d phases, %d rounds\n", prfx,
	   stats.probe.variables, stats.probe.phases, stats.probe.rounds);
  fprintf (out, "%sprbe: %d failed, %d lifted, %d merged\n", prfx,
           stats.probe.failed, stats.probe.lifted, stats.probe.merged);
  assert (stats.vars.pure ==
          stats.pure.elim + stats.pure.blkd +
	  stats.pure.expl + stats.pure.autark);
  fprintf (out, "%sprps: %lld srch props, %.2f megaprops per second\n",
           prfx, stats.props.srch,
	  (stats.srchtime>0) ? stats.props.srch/1e6/stats.srchtime : 0);
  fprintf (out, "%spure: %d = %d explicit + %d elim + %d blkd + %d autark\n",
           prfx,
	   stats.vars.pure,
	   stats.pure.expl, stats.pure.elim,
	   stats.pure.blkd, stats.pure.autark);
  assert (stats.vars.zombies ==
 	  stats.zombies.elim + stats.zombies.blkd +
	  stats.zombies.expl + stats.zombies.autark);
  fprintf (out, "%ssbst: %.0f%% subst, "
           "%.1f%% nots, %.1f%% ands, %.1f%% xors, %.1f%% ites\n", prfx,
           percent (stats.vars.subst,stats.vars.elim),
	   percent (stats.subst.nots.count,stats.vars.subst),
	   percent (stats.subst.ands.count,stats.vars.subst),
	   percent (stats.subst.xors.count,stats.vars.subst),
	   percent (stats.subst.ites.count,stats.vars.subst));
  fprintf (out, "%ssccs: %d non trivial, %d fixed, %d merged\n", prfx,
           stats.sccs.nontriv, stats.sccs.fixed, stats.sccs.merged);
#ifndef NSTATSPRECO
  long long l1s = stats.sigs.bw.l1.srch + stats.sigs.fw.l1.srch;
  long long l1h = stats.sigs.bw.l1.hits + stats.sigs.fw.l1.hits;
  long long l2s = stats.sigs.bw.l2.srch + stats.sigs.fw.l2.srch;
  long long l2h = stats.sigs.bw.l2.hits + stats.sigs.fw.l2.hits;
  long long bws = stats.sigs.bw.l1.srch + stats.sigs.bw.l2.srch;
  long long bwh = stats.sigs.bw.l1.hits + stats.sigs.bw.l2.hits;
  long long fws = stats.sigs.fw.l1.srch + stats.sigs.fw.l2.srch;
  long long fwh = stats.sigs.fw.l1.hits + stats.sigs.fw.l2.hits;
  long long hits = bwh + fwh, srch = bws + fws;
  if (opts.verbose > 1) {
    fprintf (out, "%ssigs: %13lld srch %3.0f%% hits, %3.0f%% L1, %3.0f%% L2\n",
	     prfx,
	     srch, percent (hits,srch), percent (l1h,l1s), percent(l2h,l2s));
    fprintf (out,
             "%s  fw: %13lld %3.0f%% %3.0f%% hits, %3.0f%% L1, %3.0f%% L2\n",
	     prfx,
	     fws, percent (fws,srch), percent (fwh,fws),
	     percent (stats.sigs.fw.l1.hits, stats.sigs.fw.l1.srch),
	     percent (stats.sigs.fw.l2.hits, stats.sigs.fw.l2.srch));
    fprintf (out,
             "%s  bw: %13lld %3.0f%% %3.0f%% hits, %3.0f%% L1, %3.0f%% L2\n",
	     prfx,
	     bws, percent (bws,srch), percent (bwh,bws),
	     percent (stats.sigs.bw.l1.hits, stats.sigs.bw.l1.srch),
	     percent (stats.sigs.bw.l2.hits, stats.sigs.bw.l2.srch));
  } else
    fprintf (out,
             "%ssigs: %lld searched, %.0f%% hits, %.0f%% L1, %.0f%% L2\n",
	     prfx,
	     srch, percent (hits,srch), percent (l1h,l1s), percent(l2h,l2s));
#endif
  fprintf (out,
           "%sstrs: %d forward, %d backward, %d dynamic, %d org, %d asym\n",
	   prfx,
	   stats.str.fw, stats.str.bw, stats.str.dyn,
	   stats.str.org, stats.str.asym);
  fprintf (out,
           "%ssubs: %d fw, %d bw, %d dynamic, %d org, %d doms, %d gc\n",
	   prfx,
	   stats.subs.fw, stats.subs.bw,
	   stats.subs.dyn, stats.subs.org,
	   stats.subs.doms, stats.subs.red);
  double othrtime = overalltime - stats.simptime - stats.srchtime;
  fprintf (out, "%stime: "
           "%.1f = "
	   "%.1f srch (%.0f%%) + "
	   "%.1f simp (%.0f%%) + "
	   "%.1f othr (%.0f%%)\n",
           prfx,
	   overalltime,
           stats.srchtime, percent (stats.srchtime, overalltime),
	   stats.simptime, percent (stats.simptime, overalltime),
	   othrtime, percent (othrtime, overalltime));
  fprintf (out, "%svars: %d fxd, %d eq, %d elim, %d pure, %d zmbs, %d autk\n",
           prfx,
           stats.vars.fixed, stats.vars.equiv,
	   stats.vars.elim, stats.vars.pure, stats.vars.zombies,
	   stats.vars.autark);
#ifndef NSTATSPRECO
  long long props = stats.props.srch + stats.props.simp;
  fprintf (out,
           "%svsts: %lld visits, %.2f per prop, %.0f%% blkd, %.0f%% trn\n",
           prfx,
           stats.visits, average (stats.visits, props),
	   percent (stats.blocked, stats.visits),
	   percent (stats.ternaryvisits, stats.visits));
#endif
  fprintf (out, "%szmbs: %d = %d explicit + %d elim + %d blkd + %d autark\n",
           prfx,
	   stats.vars.zombies,
	   stats.zombies.expl, stats.zombies.elim,
	   stats.zombies.blkd, stats.zombies.autark);
  fprintf (out, "%s\n", prfx);
  fprintf (out, "%s%.1f seconds, %.0f MB max, %.0f MB recycled\n", prfx,
           overalltime, mb (mem.getMax ()), mb (stats.collected));
#ifdef CHECKWITHPICOSAT
  fprintf (out, "%s\n%spicosat: %d calls, %d init\n", prfx, prfx,
           picosatcheck.calls, picosatcheck.init);
#endif
  fflush (out);
}

inline void Solver::assign (int lit) {
  assert (!vals [lit]);
  vals[lit] = 1; vals[lit^1] = -1;
  Var & v = vars[lit/2];
  assert ((v.type == ELIM) == extending);
  if (!(v.dlevel = level)) {
    if (v.type == EQUIV) {
      assert (repr[lit]);
      assert (stats.vars.equiv);
      stats.vars.equiv--;
      stats.vars.fixed++;
      v.type = FIXED;
    } else {
      assert (!repr[lit]);
      if (v.type == FREE) {
	stats.vars.fixed++;
	v.type = FIXED;
      } else assert (v.type==ZOMBIE || v.type==PURE || v.type==AUTARK);
    }
    simplified = true;
  }
  if (measure) {
    Val val = lit2val (lit);
    agility -= agility/10000;
    if (v.phase && v.phase != val) agility += 1000;
    v.phase = val;
  }
  v.tlevel = trail;
  trail.push (mem, lit);
#ifndef NLOGPRECO
  printf ("%sLOG assign %d at level %d <=", prfx, lit, level);
  if (v.binary) printf (" %d %d\n", lit, v.reason.lit);
  else if (v.reason.cls)
    printf (" %d %d %d%s\n",
	    v.reason.cls->lits[0],
	    v.reason.cls->lits[1],
	    v.reason.cls->lits[2],
	    v.reason.cls->size > 3 ? " ..." : "");
  else if (!level) printf (" top level\n");
  else printf (" decision\n");
#endif
}

inline void Solver::assume (int lit, bool inclevel) {
  if (inclevel) {
    frames.push (mem, Frame (trail));
    level++;
    LOG ("assume new level " << level);
    assert (level + 1 == frames);
    LOG ("assuming " << lit);
  } else {
    assert (!level);
    LOG ("permanently assume " << lit << " on top level");
  }
  Var & v = vars[lit/2]; v.binary = false; v.reason.cls = 0;
  v.dominator = lit;
  assign (lit);
}

inline void Solver::imply (int lit, int reason) {
  assert (lit/2 != reason/2);
  assert (vals[reason] < 0);
  assert (vars[reason/2].dlevel == level);
  Var & v = vars[lit/2];
  if (level) v.binary = true, v.reason.lit = reason;
  else v.binary = false, v.reason.lit = 0;
  if (level) v.dominator = vars[reason/2].dominator;
  assign (lit);
}

inline int Solver::dominator (int lit, Cls * reason, bool & contained) {
  if (!opts.dominate) return 0;
  if (asymode) return 0; // TODO why is this needed?
  if (autarkmode) return 0; // TODO why is this needed?
  if (opts.maxdoms <= stats.doms.count) return 0;
  contained = false;
  assert (level > 0);
  int vdom = 0, other, oldvdom;
  Var * u;
  for (const int * p = reason->lits; vdom >= 0 && (other = *p); p++) {
    if (other == lit) continue;
    u = vars + (other/2);
    if (!u->dlevel) continue;
    if (u->dlevel < level) { vdom = -1; break; }
    int udom = u->dominator;
    assert (udom);
    if (vdom) {
      assert (vdom > 0);
      if (udom != vdom) vdom = -1;
    } else vdom = udom;
  }
  assert (vdom);
  if (vdom <= 0) return vdom;
  assert (vals[vdom] > 0);
  LOG (vdom << " dominates " << lit);
  for (const int * p = reason->lits; !contained && (other = *p); p++)
    contained = (other^1) == vdom;
  if (contained) goto DONE;
  oldvdom = vdom; vdom = 0;
  for (const int * p = reason->lits; (other = *p); p++) {
    if (other == lit) continue;
    assert (vals[other] < 0);
    u = vars + other/2;
    if (!u->dlevel) continue;
    assert (u->dlevel == level);
    assert (u->dominator == oldvdom);
    other ^= 1;
    assert (other != oldvdom);
    if (other == vdom) continue;
    if (vdom) {
      while (!u->mark &&
	     (other = (assert (u->binary), 1^u->reason.lit)) != oldvdom &&
	     other != vdom) {
	assert (vals[other] > 0);
	u = vars + other/2;
	assert (u->dlevel == level);
	assert (u->dominator == oldvdom);
      }
      while (vdom != other) {
	u = vars + (vdom/2);
	assert (u->mark);
	u->mark = 0;
	assert (u->binary);
	vdom = 1^u->reason.lit;
      }
      if (vdom == oldvdom) break;
    } else {
      vdom = 1^u->reason.lit;
      if (vdom == oldvdom) break;
      assert (vals[vdom] > 0);
      other = vdom;
      do {
	u = vars + other/2;
	assert (u->dlevel == level);
	assert (u->dominator == oldvdom);
	assert (!u->mark);
	u->mark = 1;
	assert (u->binary);
	other = 1^u->reason.lit;
	assert (vals[other] > 0);
      } while (other != oldvdom);
    }
  }
  other = vdom;
  while (other != oldvdom) {
    u = vars + other/2;
    assert (u->dlevel == level);
    assert (u->dominator == oldvdom);
    assert (u->mark);
    u->mark = 0;
    assert (u->binary);
    other = 1^u->reason.lit;
    assert (vals[other] > 0);
  }

  if (vdom == oldvdom) goto DONE;

  assert (vdom);
  LOG (vdom << " also dominates " << lit);
  assert (!contained);
  for (const int * p = reason->lits; !contained && (other = *p); p++)
    contained = (other^1) == vdom;

DONE:
  stats.doms.count++;
  if (level == 1) stats.doms.level1++;
  if (!measure) { assert (level == 1); stats.doms.probing++; }
  if (contained) {
    reason->garbage = true;
    stats.subs.doms++;
    LOG ("dominator clause is subsuming");
  }

  return vdom;
}

inline void Solver::unit (int lit) {
  Var * v;
  Val val = vals[lit];
  assert (!level);
  if (val < 0) {
    LOG ("conflict after adding unit"); conflict = &empty; return;
  }
  if (!val) {
    v = vars + (lit/2);
    v->binary = false, v->reason.cls = 0;
    assign (lit);
  }
  int other = find (lit);
  if (other == lit) return;
  val = vals[other];
  if (val < 0) {
    LOG ("conflict after adding unit"); conflict = &empty; return;
  }
  if (val) return;
  v = vars + (other/2);
  v->binary = false, v->reason.cls = 0;
  assign (other);
}

inline unsigned Solver::gluelits () {
  const int * eol = lits.end ();
  int lit, found = 0;
  unsigned res = 0;
  assert (uip);
  for (const int * p = lits.begin (); p < eol; p++) {
    lit = *p;
    assert (vals[lit] < 0);
    Var * v = vars + (lit/2);
    int dlevel = v->dlevel;
    if (dlevel == level) { assert (!found); found = lit; continue; }
    assert (dlevel > 0);
    Frame * f = &frames[dlevel];
    if (f->contained) continue;
    f->contained = true;
    res++;
  }
  assert (found == uip);
  for (const int * p = lits.begin (); p < eol; p++)
    frames[vars[*p/2].dlevel].contained = false;
  LOG ("calculated glue " << res);
  return res;
}

inline void Solver::slim (Cls * cls) {
  if (!opts.slim) return;
  if (!level) return;
  assert (cls);
  if (!cls->lnd) return;
  assert (!cls->binary);
  unsigned oldglue = cls->glue;
  if (!oldglue) return;
  const int * p = cls->lits;
  unsigned newglue = 0;
  int lit, nonfalse = 0;
  while ((lit = *p++)) {
    Val val = vals[lit];
    if (val >= 0 && nonfalse++) { newglue = oldglue; break; }
    Var * v = vars + (lit/2);
    int dlevel = v->dlevel;
    if (dlevel <= 0) continue;
    assert (dlevel < frames);
    Frame * f = &frames[dlevel];
    if (f->contained) continue;
    if (++newglue >= oldglue) break;
    f->contained = true;
  }
  while (p > cls->lits) {
    lit = *--p;
    if (!lit) continue;
    Var * v = vars + (lit/2);
    int dlevel = v->dlevel;
    if (dlevel <= 0) continue;
    assert (dlevel < frames);
    Frame * f = &frames[dlevel];
    f->contained = false;
  }
  if (cls->glued) {
    assert (oldglue >= newglue);
    if (oldglue == newglue) { stats.glue.slimmed.count++; return; }
    assert (newglue >= 1);
    LOG ("slimmed glue from " << oldglue << " to " << newglue);
  } else LOG ("new glue " << newglue);
  assert (newglue <= (unsigned) opts.glue);
  if (!cls->fresh) dequeue (anchor (cls), cls);
  cls->glue = newglue;
  if (!cls->fresh) push (anchor (cls), cls);
  if (cls->glued) {
    stats.glue.slimmed.count++;
    stats.glue.slimmed.sum += newglue;
  } else cls->glued = true;
}

inline void Solver::force (int lit, Cls * reason) {
  assert (reason);
  assert (!reason->binary);
  Val val = vals[lit];
  if (val < 0) { LOG ("conflict forcing literal"); conflict = reason; }
  if (val) return;
#ifndef NDEBUG
  for (const int * p = reason->lits; *p; p++)
    if (*p != lit) assert (vals[*p] < 0);
#endif
  Var * v = vars + (lit/2);
  int vdom;
  bool sub;
  if (!level) {
    v->binary = false, v->reason.cls = 0;
  } else if (!lits && (vdom = dominator (lit, reason, sub)) > 0)  {
    v->dominator = vars[vdom/2].dominator;
    assert (vals[vdom] > 0);
    vdom ^= 1;
    LOG ("dominating learned clause " << vdom << ' ' << lit);
    assert (!lits);
    lits.push (mem, vdom);
    lits.push (mem, lit);
    bool lnd = reason->lnd || !sub;
    clause (lnd, lnd);
    v->binary = true, v->reason.lit = vdom;
  } else {
    v->binary = false, v->reason.cls = reason;
    reason->locked = true;
    if (reason->lnd) stats.clauses.lckd++;
    v->dominator = lit;
  }
  assign (lit);
}

inline void Solver::jwh (Cls * cls, bool orgonly) {
  if (orgonly && cls->lnd) return;
  int * p;
  for (p = cls->lits; *p; p++)
    ;
  int size = p - cls->lits;
  int inc = logsize (size);
  while (p > cls->lits) {
    int l = *--p;
    jwhs[l] += inc;
    if (jwhs[l] < 0) die ("maximum large JWH score exceeded");
  }
}

int Solver::find (int a) {
  assert (2 <= a && a <= 2 * maxvar + 1);
  int res, tmp;
  for (res = a; (tmp = repr[res]); res = tmp)
    ;
  for (int fix = a; (tmp = repr[fix]) && tmp != res; fix = tmp)
    repr[fix] = res, repr[fix^1] = res^1;
  return res;
}

inline void Solver::merge (int l, int k, int & merged) {
  int a, b;
  if (!opts.merge) return;
  assert (!elimode);
  assert (!blkmode);
  if ((a = find (l)) == (b = find (k))) return;
  assert (a/2 != b/2);
#ifndef NLOGPRECO
  int m = min (a, b);
  LOG ("merge " << l << " and " << k << " to " << m);
  if (k != m) LOG ("learned clause " << k << ' ' << (m^1));
  if (k != m) LOG ("learned clause " << (k^1) << ' ' << m);
  if (l != m) LOG ("learned clause " << l << ' ' << (m^1));
  if (l != m) LOG ("learned clause " << (l^1) << ' ' << m);
#endif
  if (a < b) repr[k] = repr[b] = a, repr[k^1] = repr[b^1] = a^1;
  else       repr[l] = repr[a] = b, repr[l^1] = repr[a^1] = b^1;
  assert (vars[a/2].type == FREE && vars[b/2].type == FREE);
  vars[max (a,b)/2].type = EQUIV;
  stats.vars.merged++;
  stats.vars.equiv++;
  simplified = true;
  merged++;
}

Cls * Solver::clause (bool lnd, unsigned glue) {
  assert (asymode || !elimode || !lnd);
  assert (lnd || !glue);
  Cls * res = 0;
#ifndef NLOGPRECO
  std::cout << prfx << "LOG " << (lnd ? "learned" : "original") << " clause";
  for (const int * p = lits.begin (); p < lits.end (); p++)
    std::cout << ' ' << *p;
  std::cout << std::endl;
#endif
#ifndef NDEBUG
  for (int i = 0; i < lits; i++)
    assert (vars[lits[i]/2].type != ELIM);
#endif
  if (lits == 0) {
    LOG ("conflict after added empty clause");
    conflict = &empty;
  } else if (lits == 1) {
    int lit = lits[0];
    Val val;
    if ((val = vals[lit]) < 0) {
      LOG ("conflict after adding falsified unit clause");
      conflict = &empty;
    } else if (!val) unit (lit);
  } else {
    if (lits >= (int)Cls::MAXSZ) die ("maximal clause size exceeded");
    size_t bytes = Cls::bytes (lits);
    res = (Cls *) mem.callocate (bytes);
    res->lnd = lnd;
    if (!lnd) stats.clauses.irr++;
    res->size = lits;
    int * q = res->lits, * eol = lits.end ();
    for (const int * p = lits.begin (); p < eol; p++)
      *q++ = *p;
    *q = 0;
    if (lits == 2) res->binary = true, stats.clauses.bin++;
    else {
      res->glue = min ((unsigned)opts.glue,glue);
      if (lnd) {
	stats.glue.orig.count++;//TODO redundant?
	stats.glue.orig.sum += glue;
	res->fresh = true;
        stats.clauses.lnd++;
      } else stats.clauses.orig++;
    }
    connect (res);
  }
  lits.shrink ();
  simplified = true;
  return res;
}

inline void Solver::marklits () {
  for (const int * p = lits.begin (); p < lits.end (); p++)
    vars[*p/2].mark = lit2val (*p);
}

inline void Solver::unmarklits () {
  for (const int * p = lits.begin (); p < lits.end (); p++)
    vars[*p/2].mark = 0;
}

inline bool Solver::bwsub (unsigned sig, Cls * c) {
  assert (!c->trash && !c->dirty && !c->garbage);
  limit.budget.bw.sub--;
  int count = lits;
  if (c->size < (unsigned) count) return false;
  INC (sigs.bw.l1.srch);
  if (!sigsubs (sig, c->sig)) { INC (sigs.bw.l1.hits); return false; }
  int lit;
  for (int * p = c->lits; count && (lit = *p); p++) {
    Val u = lit2val (lit), v = vars[lit/2].mark;
    if (u == v) count--;
  }
  return !count;
}

int Solver::bwstr (unsigned sig, Cls * c) {
  assert (!c->trash && !c->dirty && !c->garbage);
  limit.budget.bw.str--;
  int count = lits;
  if (c->size < (unsigned) count) return 0;
  INC (sigs.bw.l1.srch);
  if (!sigsubs (sig, c->sig)) { INC (sigs.bw.l1.hits); return 0; }
  int lit, res = 0;
  for (int * p = c->lits; count && (lit = *p); p++) {
    Val u = lit2val (lit), v = vars[lit/2].mark;
    if (abs (u) != abs (v)) continue;
    if (u == -v) { if (res) return 0; res = lit; }
    count--;
  }
  assert (count >= 0);
  res = count ? 0 : res;

  return res;
}

void Solver::remove (int del, Cls * c) {
  assert (!c->trash && !c->garbage && !c->dirty && !c->gate);
  assert (c->lits[0] && c->lits[1]);
  if (c->binary) {
    int pos = (c->lits[1] == del);
    assert (c->lits[pos] == del);
    LOG ("will not remove " << del << " but will simply produce unit instead");
    unit (c->lits[!pos]);
  } else {
    disconnect (c);
    int * p = c->lits, lit;
    while ((lit = *p) != del) assert (lit), p++;
    while ((lit = *++p)) p[-1] = lit;
    p[-1] = 0;
    assert (p - c->lits >= 3);
    if (p - c->lits == 3) {
      if (c->lnd) {
	assert (stats.clauses.lnd > 0);
	stats.clauses.lnd--;
      } else {
	assert (stats.clauses.orig > 0);
	stats.clauses.orig--;
      }
      c->binary = true;
      stats.clauses.bin++;
    }
    setsig (c);
    connect (c);
    if (elimode || blkmode) touch (del);
    simplified = true;
#ifndef NLOGPRECO
    LOG ("removed " << del << " and got");
    dbgprint ("LOG learned clause ", c);
#endif
  }
}

void Solver::bworgs () {
  if (lits <= 1) return;
  limit.budget.bw.str += 2;
  limit.budget.bw.sub += 4;
  marklits ();
  int first = 0;
  int minlen = INT_MAX;
  for (int i = 0; i < lits; i++) {
    int other = lits[i];
    int len = orgs[other];
    if (len < minlen) first = other, minlen = len;
  }
  unsigned sig = litsig ();
  assert (first);
  INC (sigs.bw.l2.srch);
  if (!sigsubs (sig, bwsigs[first])) INC (sigs.bw.l2.hits);
  else if (orgs[first] <= opts.bwmaxlen)
    for (int i = 0; limit.budget.bw.sub >= 0 && i < orgs[first]; i++) {
      Cls * other = orgs[first][i];
      assert (!other->locked);
      if (other->trash || other->dirty || other->garbage) continue;
      if (!bwsub (sig, other)) continue;
#ifndef NLOGPRECO
      dbgprint ("LOG static backward subsumed clause ", other);
#endif
      stats.subs.bw++;
      limit.budget.bw.sub += 12;
      dump (other);
    }
  int second = 0;
  minlen = INT_MAX;
  for (int i = 0; i < lits; i++) {
    int other = lits[i];
    if (other == first) continue;
    int len = orgs[other];
    if (len < minlen) second = other, minlen = len;
  }
  assert (second);
  if (orgs[first^1] < minlen) second = (first^1);
  for (int round = 0; round <= 1; round++) {
    int start = round ? second : first;
    INC (sigs.bw.l2.srch);
    if (!sigsubs (sig, bwsigs[start])) { INC (sigs.bw.l2.hits); continue; }
    Orgs & org = orgs[start];
    if (org > opts.bwmaxlen) continue;
    for (int i = 0; limit.budget.bw.str >= 0 && i < org; i++) {
      Cls * other = org[i];
      assert (!other->locked);
      if (other->trash || other->dirty || other->garbage) continue;
      int del = bwstr (sig, other);
      if (!del) continue;
      LOG ("static backward strengthened clause by removing " << del);
      stats.str.bw++;
      limit.budget.bw.str += 10;
      remove (del, other);
      assert (litsig () == sig);
    }
  }
  unmarklits ();
}

void Solver::bwoccs (bool & lnd) {
  if (lits <= 1) return;
  limit.budget.bw.sub += lnd ? 30 : 10;
  limit.budget.bw.str += lnd ? 20 : 8;
  marklits ();
  unsigned sig = litsig ();
  for (int i = 0; i < lits; i++) {
    int first = lits[i];
    if (limit.budget.bw.sub >= 0) {
      INC (sigs.bw.l2.srch);
      if (!sigsubs (sig, bwsigs[first])) INC (sigs.bw.l2.hits);
      else if (occs[first].large <= opts.bwmaxlen) {
	for (int i = 0;
	     limit.budget.bw.sub >= 0 && i < occs[first].large; i++) {
	  Cls * other = occs[first].large[i].cls;
	  assert (!other->dirty && !other->trash);
	  if (other->garbage) continue;
	  if (!bwsub (sig, other)) continue;
	  if (!other->lnd && lnd) lnd = false;
	  LOG ((level ? "dynamic" : "static")
	       << " backward subsumed "
	       << (other->lnd ? "learned" : "original")
	       << " clause");
#ifndef NLOGPRECO
	  {
	    std::cout << prfx << "LOG subsumed clause";
	    for (const int * p = other->lits; *p; p++)
	      std::cout << ' ' << *p;
	    std::cout << std::endl;
	  }
#endif
	  if (level) {
	    stats.subs.dyn++;
	    if (!other->lnd) stats.subs.org++;
	  } else stats.subs.bw++;
	  limit.budget.bw.sub += 20;
	  if (other->locked) other->garbage = true; else recycle (other);
	}
      }
    }
    if (limit.budget.bw.str >= 0) {
      INC (sigs.bw.l2.srch);
      if (!sigsubs (sig, bwsigs[first])) INC (sigs.bw.l2.hits);
      else if (occs[first].large <= opts.bwmaxlen) {
	for (int i = 0;
	     limit.budget.bw.sub >= 0 && i < occs[first].large;
	     i++) {
	  Cls* other = occs[first].large[i].cls;
	  assert (!other->dirty && !other->trash);
	  if (other->locked || other->garbage) continue;
	  int del = bwstr (sig, other);
	  if (!del) continue;
	  LOG ((level ? "dynamic" : "static")
	       << " backward strengthened "
	       << (other->lnd ? "learned" : "original")
	       << " clause by removing "
	       << del);
#ifndef NLOGPRECO
	  {
	    std::cout << prfx << "LOG strengthened clause";
	    for (const int * p = other->lits; *p; p++)
	      std::cout << ' ' << *p;
	    std::cout << std::endl;
	  }
#endif
	  if (level) {
	    stats.str.dyn++;
	    if (!other->lnd) stats.str.org++;
	  } else stats.str.bw++;
	  limit.budget.bw.str += 15;
	  remove (del, other);
	  if (level) strcls (other);
	}
      }
    }
  }
  unmarklits ();
}

inline bool Solver::fwsub (unsigned sig, Cls * c) {
  assert (!c->trash && !c->dirty && !c->garbage);
  limit.budget.fw.sub--;
  INC (sigs.fw.l1.srch);
  if (!sigsubs (c->sig, sig)) { INC (sigs.fw.l1.hits); return false; }
  int lit;
  for (const int * p = c->lits; (lit = *p); p++) {
    Val u = lit2val (lit), v = vars[lit/2].mark;
    if (u != v) return false;
  }
  return true;
}

inline int Solver::fwstr (unsigned sig, Cls * c) {
  assert (!c->trash && !c->dirty && !c->garbage);
  limit.budget.fw.str--;
  INC (sigs.fw.l1.srch);
  if (!sigsubs (c->sig, sig)) { INC (sigs.fw.l1.hits); return 0; }
  int res = 0, lit;
  for (const int * p = c->lits; (lit = *p); p++) {
    Val u = lit2val (lit), v = vars[lit/2].mark;
    if (u == v) continue;
    if (u != -v) return 0;
    if (res) return 0;
    res = (lit^1);
  }
  return res;
}

bool Solver::fworgs () {
  if (!opts.fw) return false;
  if (lits <= 1) return false;
  limit.budget.fw.str += 3;
  limit.budget.fw.sub += 5;
  assert (fwds);
  marklits ();
  unsigned sig = litsig ();
  bool res = false;
  if (lits >= 2) {
    for (int i = 0; !res && limit.budget.fw.sub >= 0 && i < lits; i++) {
      int lit = lits[i];
      INC (sigs.fw.l2.srch);
      if (!(fwsigs[lit] & sig)) { INC (sigs.fw.l2.hits); continue; }
      Fwds & f = fwds[lit];
      if (f > opts.fwmaxlen) continue;
      for (int j = 0; !res && limit.budget.fw.sub >= 0 && j < f; j++) {
	Cls * c = f[j];
	if (c->trash || c->dirty || c->garbage) continue;
	res = fwsub (sig, c);
      }
    }
    if (res) {
      LOG ("new clause is subsumed");
      stats.subs.fw++;
      limit.budget.fw.sub += 5;
    }
  }
  if (!res)
    for (int sign = 0; sign <= 1; sign++)
      for (int i = 0; limit.budget.fw.str >= 0 && i < lits; i++) {
	int lit = lits[i];
	INC (sigs.fw.l2.srch);
	if (!(fwsigs[lit] & sig)) { INC (sigs.fw.l2.hits); continue; }
	lit ^= sign;
	Fwds & f = fwds[lit];
        if (f > opts.fwmaxlen) continue;
	int del  = 0;
	for (int j = 0; !del && limit.budget.fw.str >= 0 && j < f; j++) {
	  Cls * c = f[j];
	  if (c->trash || c->dirty || c->garbage) continue;
	  del = fwstr (sig, c);
	}
	if (!del) continue;
	assert (sign || del/2 != lit/2);
	LOG ("strengthen new clause by removing " << del);
	stats.str.fw++;
	limit.budget.fw.str += 8;
	assert (vars[del/2].mark == lit2val (del));
	vars[del/2].mark = 0;
	lits.remove (del);
	sig = litsig ();
	i = -1;
      }
  unmarklits ();
  return res;
}

void Solver::resize (int newmaxvar) {
  assert (maxvar < size);
  assert (newmaxvar > maxvar);
  int newsize = size;
  while (newsize <= newmaxvar)
    newsize *= 2;
  if (newsize > size) {
    size_t o, n;

    ptrdiff_t diff;
    char * c;

    o = 2 * size * sizeof *blks;
    n = 2 * newsize * sizeof *blks;
    c = (char*) blks;
    blks = (Rnk*) mem.recallocate (blks, o, n);
    diff = c - (char*) blks;
    schedule.block.fix (diff);

    o = 2 * size * sizeof *jwhs;
    n = 2 * newsize * sizeof *jwhs;
    jwhs = (int*) mem.recallocate (jwhs, o, n);

    o = 2 * size * sizeof *occs;
    n = 2 * newsize * sizeof *occs;
    occs = (Occs*) mem.recallocate (occs, o, n);

    o = 2 * size * sizeof *repr;
    n = 2 * newsize * sizeof *repr;
    repr = (int*) mem.recallocate (repr, o, n);

    o = 2 * size * sizeof *vals;
    n = 2 * newsize * sizeof *vals;
    vals = (Val*) mem.recallocate (vals, o, n);

    o = size * sizeof *elms;
    n = newsize * sizeof *elms;
    c = (char *) elms;
    elms = (Rnk*) mem.reallocate (elms, o, n);
    diff = c - (char *) elms;
    schedule.elim.fix (diff);

    o = size * sizeof *rnks;
    n = newsize * sizeof *rnks;
    c = (char *) rnks;
    rnks = (Rnk*) mem.reallocate (rnks, o, n);
    diff = c - (char *) rnks;
    schedule.decide.fix (diff);

    o = size * sizeof * vars;
    n = newsize * sizeof * vars;
    vars = (Var *) mem.recallocate (vars, o, n);

    rszbwsigs (newsize);
    rsziirfs (newsize);
    size = newsize;
  }
  assert (newmaxvar < size);
  while (maxvar < newmaxvar) {
    Var * v = vars + ++maxvar;
    v->dlevel = -1;

    for (int i = 1; i < opts.order; i++)
      iirf (v, i) = -1;

    Rnk * r = rnks + maxvar;
    r->heat = 0, r->pos = -1, schedule.decide.push (mem, r);

    Rnk * e = elms + maxvar;
    e->heat = 0, e->pos = -1;
  }
  assert (maxvar < size);
}

void Solver::import () {
  bool trivial = false;
  int * q = lits.begin ();
  const int * p, * eol = lits.end ();
#ifdef CHECKWITHPICOSAT
  for (p = lits.begin (); !trivial && p < eol; p++)
    picosat_add (ulit2ilit (*p));
  picosat_add (0);
#endif
  for (p = lits.begin (); !trivial && p < eol; p++) {
    int lit = *p, v = lit/2;
    assert (1 <= v);
    if (v > maxvar) resize (v);
    Val tmp = vals[lit];
    if (!tmp) {
      int prev = vars[v].mark;
      int val = lit2val (lit);
      if (prev) {
	if (val == prev) continue;
	assert (val == -prev);
	trivial = 1;
      } else {
	vars[v].mark = val;
	*q++ = lit;
      }
    } else if (tmp > 0)
      trivial = true;
  }

  while (p > lits.begin ()) vars[*--p/2].mark = 0;

  if (!trivial) {
    lits.shrink (q);
    bool lnd = false;
    bwoccs (lnd);
    assert (!lnd);
    clause (lnd, lnd);
  }
  lits.shrink ();
}

inline bool Solver::satisfied (const Cls * c) {
  int lit;
  for (const int * p = c->lits; (lit = *p); p++)
    if (val (lit) > 0) return true;
  return false;
}

inline bool Solver::satisfied (Anchor<Cls> & anchor) {
  for (const Cls * p = anchor.head; p; p = p->prev)
    if (!satisfied (p)) return false;
  return true;
}

bool Solver::satisfied () {
  if (!satisfied (binary)) return false;
  if (!satisfied (original)) return false;
#ifndef NDEBUG
  for (int glue = 0; glue <= opts.glue; glue++)
    if (!satisfied (learned[glue])) return false;
  if (!satisfied (fresh)) return false;
#endif
#ifdef PRECOCHECK
  for (const int * p = check.begin (); p < check.end (); p++) {
    bool found = false;
    int lit;
    while ((lit = *p)) {
      if (val (lit) > 0) found = true;
      p++;
    }
    if (!found) return false;
  }
#endif
  return true;
}

inline void Solver::prop2 (int lit) {
  assert (vals[lit] < 0);
  LOG ("prop2 " << lit);
  const Stack<int> & os = occs[lit].bins;
  const int * eos = os.end ();
  for (const int * p = os.begin (); p < eos; p++) {
    int other = *p;
    Val val = vals[other];
    if (val > 0) continue;
    if (!val) { imply (other, lit); continue; }
    LOG ("conflict in binary clause while propagating " << (1^lit));
    conflict = lit2conflict (dummy, lit, other);
    LOG ("conflicting clause " << lit << ' ' << other);
    if (!autarkmode) break;
  }
}

inline void Solver::propl (int lit) {
  assert (vals[lit] < 0);
  LOG ("propl " << lit);
  Stack<Occ> & os = occs[lit].large;
  Occ * a = os.begin (), * t = os.end (), * p = a, * q = a;
CONTINUE_OUTER_LOOP:
  while (p < t) {
    int blit = p->blit;
    Cls * cls = p++->cls;
    *q++ = Occ (blit, cls);
    INC (visits);
    Val val = vals[blit];
    if (val > 0) { INC (blocked); continue; }
#ifndef NSTATSPRECO
    if (cls->lits[2] && !*(cls->lits + 3)) INC (ternaryvisits);
#endif
    int sum = cls->lits[0]^cls->lits[1];
    int other = sum^lit;
    val = vals[other];
    q[-1].blit = other;
    if (val > 0) continue;
    int prev = lit, next, *l, *lits2 = cls->lits+2;
    for (l = lits2; (next = *l); l++) {
      *l = prev; prev = next;
      if (vals[next] < 0) continue;
      occs[next].large.push (mem, Occ (other, cls));
      int pos = (cls->lits[1] == lit);
      cls->lits[pos] = next;
      q--; goto CONTINUE_OUTER_LOOP;
    }
    while (l > lits2) { next = *--l; *l = prev; prev = next; }
    slim (cls);
    if (val) {
      LOG ("conflict in large clause while propagating " << (1^lit));
      conflict = cls;
#ifndef NLOGPRECO
      dbgprint ("LOG conflicting clause ", cls);
#endif
      if (autarkmode) continue;
      break;
    }
    force (other, cls);
    long moved = ((char*)os.begin ()) - (char*)a;
    if (moved) fix (p,moved), fix (q,moved), fix (t,moved), a = os.begin ();
  }
  while (p < t) *q++ = *p++;
  os.shrink (q - a);
}

bool Solver::bcp () {
  if (conflict && !autarkmode) return false;
  int fixed = queue2, old = stats.clauses.orig;
  if (!level && units) flushunits ();
  if (conflict && !autarkmode) return false;
  int lit, props = 0;
  for (;;) {
    if (queue2 < trail) {
      props++;
      lit = 1^trail[queue2++];
      prop2 (lit);
      if (conflict && /*!measure &&*/ !autarkmode) break;
    } /*else*/ if (queue < trail) {
      if (conflict && !autarkmode) break;
      lit = 1^trail[queue++];
      propl (lit);
      if (conflict && !autarkmode) break;
    } else
      break;
  }
  if (measure) stats.props.srch += props;
  else stats.props.simp += props;
  if (conflict) return false;
  if (level) return true;
  while (fixed < trail) {
    int lit = trail[fixed++];
    recycle (lit);
  }
  if (!simpmode) shrink (old);
  assert (!conflict);
  return true;
}

inline bool Solver::needtoflush () const {
  return queue2 < trail;
}

inline int Solver::phase (Var * v) {
  int lit = 2 * (v - vars), notlit = lit + 1, res;
  if (v->phase > 0) res = lit;
  else if (v->phase < 0) res = notlit;
  else if (opts.phase == 0) res = notlit;
  else if (opts.phase == 1) res = lit;
  else if (opts.phase == 2) res = (jwhs[notlit] < jwhs[lit]) ? lit : notlit;
  else res = rng.choose () ? lit : notlit;
  return res;
}

void Solver::extend () {
  report (2, 't');
  extending = true;
  int n = elits;
  while (n > 0) {
    int unblock = elits[--n];
    int lit = elits[--n];
    if (unblock) {
      LOG ("unblocking " << lit);
      Val valit = val (lit);
      assert (valit);
      bool forced = (valit < 0);
      int other;
      while ((other = elits[--n])) {
	if (!forced) continue;
	other = find (other);
	Val v = val (other);
	assert (v);
	forced = (v < 0);
      }
      if (!forced) continue;
      // TODO make this in an assertion ...
      // We have 'in principle' a proof that this can not happen.
      if (repr[lit]) die ("internal error extending assignment");
      LOG ("assign " << (1^lit) << " at level " << level << " <= unblock");
      vals[lit] = 1, vals[lit^1] = -1;
      assert (trail[vars[lit/2].tlevel] == (1^lit));
      trail[vars[lit/2].tlevel] = lit;
      stats.extend.flipped++;
    } else {
      LOG ("extending " << lit);
      assert (!repr [lit]);
      while (elits[n-1]) {
	bool forced = true;
	int other;
	while ((other = elits[--n])) {
	  if (!forced) continue;
	  other = find (other);
	  if (other == lit) continue;
	  if (other == (lit^1)) { forced = false; continue; }
	  Val v = val (other);
	  if (v > 0) forced = false;
	}
	if (!forced) continue;
	Val v = val (lit);
	if (v) assert (v > 0);
	else { assume (lit); stats.extend.forced++; }
      }
      if (!val (lit)) { assume (lit^1); stats.extend.assumed++; }
      n--;
    }
  }
#ifndef NDEBUG
  for (int lit = 2; lit <= 2 * maxvar + 1; lit += 2)
    assert (val (lit));
#endif
  extending = false;
}

bool Solver::decide () {
  Rnk * r;
  for (;;) {
    if (!schedule.decide) { extend (); return false; }
    r = schedule.decide.max ();
    int idx = r - rnks;
    int lit = 2 * idx;
    if (vars[idx].type == FREE && !vals[lit]) break;
    (void) schedule.decide.pop ();
  }
  assert (r);
  stats.decisions++;
  stats.sumheight += level;
  if (opts.random &&
      //agility <= 10 * 100000 &&
      rng.oneoutof (opts.spread)) {
    stats.random++;
    unsigned n = (unsigned) maxvar; assert (n);
    unsigned s = rng.next () % n, d = rng.next () % n;
    while (ggt (n, d) != 1) {
      d++; if (d >= n) d = 1;
    }
    while (vars[s+1].type != FREE || val (2*s+2)) {
      s += d; if (s >= n) s -= n;
    }
    r = rnks + s + 1;
  }
  int lit = phase (var (r));
  assert (!vals[lit]);
  LOG ("decision " << lit);
  assume (lit);
  iterating = false;
  return true;
}

inline bool Solver::min2 (int lit, int other, int depth) {
  assert (lit != other && vals[lit] > 0);
  if (vals[other] >= 0) return false;
  Var * v = vars + (lit/2);
  if (v->binary && v->reason.lit == other) {
    Var * d = vars + (v->dominator/2);
    //assert (d != v);
    assert (depth);
    if (d->removable) return true;
    if (d->mark) return true;
  }
  Var * u = vars + (other/2);
  assert (opts.minimize > Opts::RECUR || u->tlevel < v->tlevel);
  if (u->tlevel > v->tlevel) return false;
  if (u->mark) return true;
  if (u->removable) return true;
  if (u->poison) return false;
  return minimize (u, depth);
}

inline bool Solver::minl (int lit, Cls * cls, int depth) {
  assert (vals[lit] > 0);
  assert (cls->locked || cls->lits[0] == lit || cls->lits[1] == lit);
  Var * v = vars + (lit/2);
  int other;
  for (const int * p = cls->lits; (other = *p); p++) {
    if (other == lit) continue;
    if (vals[other] >= 0) return false;
    Var * u = vars + (other/2);
    assert (opts.minimize > Opts::RECUR || u->tlevel < v->tlevel);
    if (u->tlevel > v->tlevel) {
      if (!opts.inveager) return false;
      if (u != vars + uip/2 && (u->binary || u->reason.cls))
       	return false;
    }
    if (u->mark) continue;
    if (u->removable) continue;
    if (u->poison) return false;
    int l = u->dlevel;
    if (!l) continue;
    if (!frames[l].pulled) return false;
  }
  //int old = pulled;
  for (const int * p = cls->lits; (other = *p); p++) {
    if (other == lit) continue;
    Var * u = vars + other/2;
    if (u->mark) continue;
    if (u->removable) continue;
    if (u->poison) return false;
    int l = u->dlevel;
    if (!l) continue;
    if (!minimize (u, depth)) { /*cleanpulled (old); */ return false; }
  }
  return true;
}

inline bool Solver::strengthen (int lit, int depth) {
  assert (vals[lit] > 0);
  assert (opts.minimize >= Opts::STRONG);
  const Stack<int> & os2 = occs[lit].bins;
  const int * eos2 = os2.end ();
  for (const int * o = os2.begin (); o < eos2 && limit.strength-- >= 0; o++)
    if (min2 (lit, *o, depth)) return true;
  if (opts.minimize < Opts::STRONGER) return false;
  const Stack<Occ> & osl = occs[lit].large;
  const Occ * eosl = osl.end ();
  for (const Occ * o = osl.begin (); o < eosl && limit.strength-- >= 0; o++)
    if (minl (lit, o->cls, depth)) {
      if (opts.mtfall && depth == 1 && o->cls->lnd)
       	mtf (anchor (o->cls), o->cls);
      return true;
    }
  return false;
}

bool Solver::minimize (Var * v, int depth) {
  if (!depth) limit.strength = opts.strength;
  if (depth > stats.mins.depth) stats.mins.depth = depth;
  assert (v->dlevel != level);
  if (v->removable) return true;
  if (depth && v->mark) return true;
  if (v->poison) return false;
  int l = v->dlevel;
  if (!l) return true;
  if (opts.minimize == Opts::NONE) return false;
  if (depth && opts.minimize == Opts::LOCAL) return false;
  if (!v->binary && !v->reason.cls) return false;
  if (!frames[l].pulled) return false;
  if (depth++ >= opts.maxdepth) return false;
  assert (!v->onstack);
  v->onstack = true;
  bool res = false;
  int lit = 2 * (v - vars);
  Val val = vals[lit];
  if (val < 0) lit ^= 1;
  assert (vals[lit] > 0);
  if (v->binary) res = min2 (lit, v->reason.lit, depth);
  else if ((res = minl (lit, v->reason.cls, depth))) {
    if (opts.mtfall && depth == 1 && v->reason.cls->lnd)
      mtf (anchor (v->reason.cls), v->reason.cls);
  }
  if (opts.minimize >= Opts::STRONG && !res)
    if ((res = strengthen (lit, depth)))
      stats.mins.strong++;
  v->onstack = false;
  if (res) v->removable = true;
  else v->poison = true;
  seen.push (mem, v);
  return res;
}

bool Solver::inverse (int lit) {
  assert (vals[lit] > 0);
  Var * v = vars + (lit/2);
  if (v->dlevel != jlevel) return false;
  const Stack<Occ> & osl = occs[lit].large;
  const Occ * eosl = osl.end ();
  bool foundlit = false, founduip = false;
  for (const Occ * o = osl.begin (); o < eosl; o++) {
    Cls * c = o->cls;
    int other = 0;
    for (const int * p = c->lits; (other = *p); p++) {
      if (other == lit) foundlit = true;
      if (other == uip) founduip = true;
      if (other == lit || other == uip) continue;
      if (vals[other] >= 0) break;
      Var * u = vars + (other/2);
      if (u->dlevel >= jlevel) break;
      if (!u->mark) break;
    }
    if (other) continue;
    assert (foundlit);
    assert (founduip);
#ifndef NLOGPRECO
    dbgprint ("LOG inverse arc ", c);
#endif
    if (opts.mtfall && c->lnd) mtf (anchor (c), c);
    return true;
  }
  return false;
}

void Solver::unassign (int lit, bool save) {
  assert (vals[lit] > 0);
  vals[lit] = vals[lit^1] = 0;
  int idx = lit/2;
  Var & v = vars[idx];
  LOG("unassign " << lit <<
      " dlevel " << v.dlevel << " tlevel " << v.tlevel);
  assert (v.dlevel > 0);
  if (save) { LOG ("saving " << lit); saved.push (mem, lit); }
  if (!repr[lit]) {
    Rnk & r = rnks[lit/2];
    if (!schedule.decide.contains (&r))
      schedule.decide.push (mem, &r);
  }
  v.dlevel = -1;
  if (v.binary) return;
  Cls * c = v.reason.cls;
  if (!c) return;
  c->locked = false;
  if (!c->lnd) return;
  assert (stats.clauses.lckd > 0);
  stats.clauses.lckd--;
}

void Solver::undo (int newlevel, bool save) {
  if (newlevel == level) return;
  LOG ("undo " << newlevel);
  assert (newlevel <= level);
  if (save) saved.shrink ();
  int tlevel = frames[newlevel+1].tlevel;
  int cnt = 0;
  while (tlevel < trail) {
    int lit = trail.pop ();
    if (!vals [lit]) continue;
    unassign (lit, save);
    cnt++;
  }
  frames.shrink (newlevel + 1);
  level = newlevel;
  queue = queue2 = trail;
  conflict = 0;
  LOG ("backtracked to new level " << newlevel <<
       " after unassigning " << cnt << " literals");
}

void Solver::cutrail (int newtlevel) {
  LOG ("cutting back trail from " << trail << " to " << newtlevel);
  assert (newtlevel >= 0);
  assert (newtlevel + 1 <= trail);
  assert (!autarkmode);
  queue = queue2 = newtlevel;
  int cnt = 0;
  while (newtlevel + 1 < trail) {
    int lit = trail.top ();
    if (!vars[lit/2].dlevel) break;
    (void) trail.pop ();
    assert (vals[lit]);
    unassign (lit, false);
    cnt++;
  }
  assert (!conflict);
  LOG ("reststarting bcp with " << trail[newtlevel] << " at " << newtlevel);
  stats.back.cuts += cnt;
}

inline int & Solver::iirf (Var * v, int t) {
  assert (1 <= t && t < opts.order && 2 <= opts.order);
  return iirfs[(opts.order - 1) * (v - vars) + (t - 1)];
}

inline void Solver::rescore () {
  stats.rescored++;
  hinc >>= 14;
  for (Rnk * s = rnks + 1; s <= rnks + maxvar; s++) s->heat >>= 14;
  schedule.decide.construct ();
}

inline void Solver::bump (Var * v, int add) {
  assert (stats.conflicts > 0);
  int inc = hinc;
  if (opts.order >= 2) {
    inc /= 2;
    int w = inc, c = stats.conflicts - 1, s = 512, l = 9;
    for (int i = 1; i <= opts.order - 1; i++) {
      w >>= 1, l--;
      for (int j = 1; j <= opts.order - 1; j++) {
	int d = iirf (v, j);
	if (d > c) continue;
	if (d == c) inc += w, s += (1<<l);
	break;
      }
      c--;
    }
    for (int i = opts.order - 1; i > 1; i--)
      iirf (v, i) = iirf (v, i - 1);
    iirf (v, 1) = stats.conflicts;
    assert ((hinc >> (opts.order-1)) <= inc && inc <= hinc);
  }
  Rnk * r = rnk (v);
  r->heat += inc + add;
  LOG ("bump " << 2*(v - vars) << " by " << inc << " new score " << r->heat);
  schedule.decide.up (r);
  assert (hinc < (1<<28));
  if (r->heat >= (1<<24)) {
    if (opts.bumpbulk) needrescore = true;
    else rescore ();
  }
}

inline void Solver::pull (int lit) {
  assert (vals [lit] < 0);
  Var * v = vars + (lit/2);
  assert (v->dlevel && !v->mark);
  LOG ("pulling " << lit << " open " << open);
  v->mark = 1;
  seen.push (mem, v);
  if (v->dlevel == level) open++;
  else {
    lits.push (mem, lit);
    if (!frames[v->dlevel].pulled) {
      frames[v->dlevel].pulled = true;
      levels.push (mem, v->dlevel);
    }
  }
}

void Solver::cleanlevels () {
  while (levels) {
    int l = levels.pop ();
    assert (frames[l].pulled);
    frames[l].pulled = false;
  }
}

void Solver::jump () {
  jlevel = 0;
  int * eolits = lits.end ();
  for (const int * p = lits.begin(); p < eolits; p++) {
    Var * v = vars + (*p/2);
    if (v->dlevel < level && v->dlevel > jlevel)
      jlevel = v->dlevel;
  }
  LOG ("new jump level " << jlevel);
}

void Solver::cleanseen () {
  Var ** eos = seen.end ();
  for (Var ** p = seen.begin(); p < eos; p++) {
    Var * v = *p; v->mark = 0; v->removable = v->poison = false;
  }
  seen.shrink ();
}

void Solver::bump (Cls * c) {
  if (c == &dummy) return;
  assert (c);
  if (c->garbage) return;
  if (!c->lnd) return;
  assert (!c->binary);
  mtf (anchor (c), c);
#ifndef NLOGPRECO
  int lit;
  for (const int * p = c->lits; (lit = *p); p++)
    if (vals[lit] > 0) break;
  if (lit) {
    char buffer[100];
    sprintf (buffer, "LOG bump %d forcing clause ", lit);
    dbgprint (buffer, c);
  } else dbgprint ("LOG bump conflicting clause ", c);
#endif
}

struct RevLtTLevel {
  Var * u, * v;
  RevLtTLevel (Var * a, Var * b) : u (a), v (b) { }
  operator bool () const { return v->tlevel < u->tlevel; }
};

void Solver::bump () {
  assert (conflict);
  Var ** start, ** end, ** bos = seen.begin (), ** eos = seen.end ();
  if (opts.bumpsort) { Sorter<Var*,RevLtTLevel> s (mem); s.sort (bos, seen); }
#ifndef NDEBUG
  for (Var ** p = bos; p + 1 < eos; p++) assert (p[0]->tlevel > p[1]->tlevel);
#endif
  Var * uipvar = vars + (uip/2), * except = opts.bumpuip ? 0 : uipvar;
  int dir;
  if (opts.mtfrev) start = bos,   end = eos,   dir = 1;
  else             start = eos-1, end = bos-1, dir = -1;
  if (opts.mtfrev) bump (conflict);
  for (Var ** p = start; p != end; p += dir) {
    Var * v = *p;
    if (v == uipvar) continue;
    if (v->dlevel < level) continue;
    if (v->binary) continue;
    Cls * r = v->reason.cls;
    if (!r) continue;
    bump (r);
  }
  if (!opts.mtfrev) bump (conflict);
  // NOTE: this case split is actually redundant since
  // we use the variable index order as tie breaker anyhow.
  // TODO not really true, since rescore may happen?
  if (opts.bumpbulk) needrescore = false;
  if (opts.bumprev) start = eos-1, end = bos-1, dir = -1;
  else              start = bos,   end = eos,   dir = 1;
  int turbo = opts.bumpturbo ? eos - bos - 1 : 0;
  for (Var ** p = start; p != end; p += dir) {
    Var * v = *p;
    if (v == except) continue;
    bump (v, turbo);
    if (opts.bumpturbo) turbo--;
  }
  if (opts.bumpbulk && needrescore) rescore ();
}

bool Solver::analyze () {
  int orglevel = level;
RESTART:
  assert (conflict);
  stats.conflicts++;
  if (!level) return false;
  assert (!lits); assert (!seen); assert (!levels);
  int i = trail, lit;
  bool unit = false;
  resolved = open = 0;
  Var * u = 0;
  uip = 0;
  do {
    assert (seen >= resolved);
    int otf = seen - resolved;
    if (uip) {
      LOG ("resolving " << uip);
      resolved++;
    }
    if (!u || !u->binary) {
      Cls * r = (u ? u->reason.cls : conflict); assert (r);
      for (const int * p = r->lits; (lit = *p); p++) {
	Var * v = vars + (lit/2);
	if (!v->dlevel) continue;
	if (!v->mark) { pull (lit); otf = INT_MAX; }
	else if (v->tlevel <= i) otf--;
	else otf = INT_MAX;
      }
      if (level > 0 && opts.otfs && otf <= 0 && r != &dummy) {
#ifndef NLOGPRECO
	printf ("%sLOG on-the-fly strengthened %s clause ",
	       prfx, r->lnd ? "learned" : "original");
	r->print ();
	printf ("%sLOG on-the-fly strengthening resolvent", prfx);
	for (Var ** p = seen.begin (); p < seen.end (); p++)
	  if ((*p)->tlevel <= i) {
	    int lit = 2 * (*p - vars);
	    int val = vals[lit];
	    assert (val);
	    if (val > 0) lit++;
	    printf (" %d", lit);
	  }
	fputc ('\n', stdout);
#endif
#ifndef NDEBUG
	assert (u);
	if (opts.check) {
	  for (Var ** p = seen.begin (); p < seen.end (); p++) {
	    Var * v = *p;
	    assert (v->mark);
	    if (v->tlevel > i) continue;
	    int idx = v - vars, * q;
	    for (q = r->lits; *q; q++) if (*q/2 == idx) break;
	    assert (*q);
	  }
	}
#endif
	lit = 2 * (u - vars);
	int val = vals[lit];
	assert (val);
	if (val < 0) lit++;
	assert (r->locked);
	assert (u->reason.cls == r);
	u->reason.cls = 0;
	r->locked = false;
	if (r->lnd) {
	  assert (stats.clauses.lckd > 0);
	  stats.clauses.lckd--;
	}
	bump ();
	if (r->garbage) r->garbage = false;
	remove (lit, r);
	if (r->binary) {
	  conflict = lit2conflict (dummy, r->lits[0], r->lits[1]);
	  stats.otfs.dyn.trn++;
	} else {
	  conflict = r;
	  slim (r);
	  stats.otfs.dyn.large++;
	}
	cleanlevels ();
	cleanseen ();
	lits.shrink ();
	goto RESTART;
      }
    } else {
      assert (u && u->binary);
      otf--;
      lit = u->reason.lit;
      Var * v = vars + (lit/2);
      if (v->dlevel) {
	if (v->mark) { if (v->tlevel < i) otf--; }
	else { pull (lit); otf = INT_MAX; }
      }

      if (level > 0 && opts.otfs && otf <= 0) {
	stats.otfs.dyn.bin++;
	unit = true;
      }
    }

    for (;;) {
      assert (i > 0);
      uip = trail[--i];
      u = vars + (uip/2);
      if (!u->mark) continue;
      assert (u->dlevel == level);
      assert (open > 0);
      open--;
      break;
    }
  } while (open);
  assert (uip);
  LOG ("uip " << uip);
  uip ^= 1;
  lits.push (mem, uip);

#ifndef NLOGPRECO
  printf ("%sLOG 1st uip clause", prfx);
  for (const int * p = lits.begin (); p < lits.end (); p++)
    printf (" %d", *p);
  fputc ('\n', stdout);
#endif
  bump ();
#ifndef NDEBUG
  for (Var **p = seen.begin (); p < seen.end (); p++) {
    Var * v = *p; assert (v->mark && !v->removable && !v->poison);
  }
#endif
  int * q = lits.begin (), * eolits = lits.end ();
  for (const int * p = q; p < eolits; p++) {
    lit = *p; Var * v = vars + (lit/2);
    assert ((v->dlevel == level) == (lit == uip));
    if (v->dlevel < level && minimize (v, 0)) {
      LOG ("removing 1st uip literal " << lit);
      stats.mins.deleted++;
    } else *q++ = lit;
  }
  lits.shrink (q);
#ifndef NDEBUG
  {
    const int * p;
    for (p = lits.begin (); p < lits.end (); p++)
      if (*p == uip) break;
    assert (p != lits.end ());
  }
#endif
  jump ();
  int cjlevel = 0;
  eolits = lits.end ();
  for (const int * p = lits.begin(); p < eolits; p++) {
    lit = *p;
    Var * v = vars + (lit/2);
    if (v->dlevel != jlevel) continue;
    LOG ("literal " << lit << " on jump level " << jlevel);
    cjlevel++;
  }
  LOG (cjlevel <<
       " variables in minimized 1st UIP clause on jump level " << jlevel);

  if (opts.inverse && cjlevel == 1) {
    q = lits.begin (), eolits = lits.end ();
    {
      int * p;
      for (p = q; p < eolits; p++) {
	if (inverse (1^*p)) {
	  stats.mins.inverse++;
	  stats.mins.deleted++;
	  LOG ("literal " << *p << " removed due to inverse arc");
	  assert (cjlevel > 0);
	  cjlevel--;
	  p++;
	  break;
	} else *q++ = *p;
      }
      while (p < eolits)
	*q++ = *p++;
      if (!cjlevel) {
	LOG ("inverse arc decreases jump level");
	jump ();
      }
    }
    lits.shrink (q);
  }

  unsigned glue = gluelits ();

  cleanseen ();
  cleanlevels ();

  stats.lits.added += lits;
#ifndef NLOGPRECO
  printf ("%sLOG minimized clause", prfx);
  for (const int * p = lits.begin (); p < lits.end (); p++)
    printf (" %d", *p);
  fputc ('\n', stdout);
#endif

#ifndef NDEBUG
  for (const int * p = lits.begin (); p < lits.end (); p++)
    assert (*p == uip || vars[*p/2].dlevel < level);
  if (unit) assert (!jlevel);
#endif
#ifndef NLOGPRECO
  if (jlevel + 1 < level) LOG ("backjumping to level " << jlevel);
  else LOG ("backtracking to level " << jlevel);
#endif
  undo (jlevel);
  if (!jlevel) iterating = true;

#ifndef NLOGPRECO
  {
    int tlevel = -1;
    for (int * p = lits.begin (); p != lits.end (); p++) {
      int lit = *p;
      Val val = vals[lit];
      assert (val <= 0);
      if (!val) { assert (lit == uip); continue; }
      int tmp = vars[lit/2].tlevel;
      assert (tmp >= 0);
      if (tlevel < tmp) tlevel = tmp;
    }
    if (tlevel >= 0)
      LOG ("trail can be cut by for minimized FUIP reason " << trail - tlevel);
  }
#endif

  assert (!conflict);
  bool lnd = true;
  if (opts.dynbw) bwoccs (lnd);
  bool skip = false;
  if (strnd) {
    LOG ("analyzing " << strnd << " strengthened clauses");
    for (Cls ** p = strnd.begin (); p < strnd.end (); p++) {
      Cls * c = *p;
      int countnonfalse = 0, maxlevel = -1, maxtlevel = -1;
      for (int * q = c->lits; (lit = *q); q++) {
	int val = vals[lit];
	if (val > 0) break;
	if (!val && ++countnonfalse >= 2) break;
	if (!val) continue;
	int tmp = vars[lit/2].dlevel;
	if (tmp > maxlevel) maxlevel = tmp;
	tmp = vars[lit/2].tlevel;
	if (tmp > maxtlevel) maxtlevel = tmp;
      }
      if (lit || countnonfalse >= 2) continue;
      assert (maxlevel >= 0);
      assert (maxtlevel >= 0);
      assert (!lit && countnonfalse <= 1);
      if (maxlevel < level) {
	LOG ("strengthening decision level " << maxlevel);
	undo (maxlevel);
      }
      if (maxtlevel < trail) {
	LOG ("strengthened trail level " << maxtlevel);
	cutrail (maxtlevel);
      }
    }
    marklits ();
    unsigned sig = litsig ();
    for (Cls ** p = strnd.begin (); !skip && p < strnd.end (); p++)
      if (fwsub (sig, *p)) skip = true;
    unmarklits ();
    if (skip)
      LOG ("learned clause subsumed by strengthened clause");
    for (Cls ** p = strnd.begin (); p < strnd.end (); p++) {
      Cls * c = *p;
      int unit = 0;
      for (int * q = c->lits; (lit = *q); q++) {
	Val val = vals[lit];
	if (val < 0) continue;
	if (val > 0) break;
	if (unit) break;
	unit = lit;
      }
      if (lit) continue;
      assert (!c->garbage);
      if (unit) {
	if (c->binary) {
	  int other = c->lits[0] + c->lits[1];
	  other -= unit;
	  imply (unit, other);
	} else force (unit, c);
	if (skip) continue;
	skip = true;
	LOG ("learned clause skipped because of forcing strengthened clause");
      } else {
	skip = true;
	LOG ("learned clause skipped because of empty strengthened clause");
      }
    }
    for (Cls ** p = strnd.begin (); p < strnd.end (); p++)
      assert ((*p)->str),  (*p)->str = false;
    strnd.shrink ();
  }
  if (!skip || !lnd) {
    for (Frame * f = frames.begin (); f < frames.end (); f++)
      assert (!f->contained);
    Cls * cls = clause (lnd, lnd ? glue : 0); //TODO: move before if?
    if (!vals[uip] && level == jlevel) {
      if (cls) {
	if (cls->binary) {
	  int other = cls->lits[0] + cls->lits[1];
	  other -= uip;
	  imply (uip, other);
	} else {
	  force (uip, cls);
	  slim (cls);
	}
      }
    }
    if (lnd) {
      while (fresh.count > limit.reduce.fresh && fresh.tail) {
	Cls * f = fresh.tail;
	dequeue (fresh, f);
	assert (f->fresh);
	assert (!f->binary);
	assert (f->lnd);
	f->fresh = false;
	push (anchor (f), f);
      }
      int count = opts.dynred;
      Cls * ctc;
      for (int glue = opts.glue;
	   glue >= opts.sticky && count >= 0 && recycling ();
	   glue--) {
	ctc = learned[glue].tail;
	while (ctc && count-- >= 0 && recycling ()) {
	  Cls * next = ctc->next;
	  assert (!ctc->binary);
	  if (ctc != cls && !ctc->locked) recycle (ctc);
	  ctc = next;
	}
      }
    }
  } else lits.shrink ();

  if (opts.cutrail && level == jlevel && level > 0 && vals[uip]) {
    Var * v = vars + (uip/2);
    int maxtlevel = -1, lit;
    if (v->binary) {
      lit = v->reason.lit;
      maxtlevel = vars[lit/2].tlevel;
    } else {
      Cls * c = v->reason.cls;
      for (int * p = c->lits; (lit = *p); p++) {
	int tmp = vars[lit/2].tlevel;
	if (tmp > maxtlevel) maxtlevel = tmp;
      }
    }
    assert (maxtlevel >= 0);
    LOG ("cutting back UIP reason to " << maxtlevel);
    cutrail (maxtlevel);
  }

  long long tmp = hinc;
  tmp *= 100 + opts.heatinc; tmp /= 100;
  assert (tmp <= INT_MAX);
  hinc = tmp;

  stats.back.track++;
  int dist = orglevel - level;
  assert (dist >= 1);
  if (dist > 1) stats.back.jump++, stats.back.dist += dist;

  return true;
}

int Solver::luby (int i) {
  int k;
  for (k = 1; k < 32; k++)
    if (i == (1 << k) - 1)
      return 1 << (k-1);

  for (k = 1;; k++)
    if ((1 << (k-1)) <= i && i < (1 << k) - 1)
      return luby (i - (1u << (k-1)) + 1);
}

inline double Stats::height () {
  return decisions ? sumheight / (double) decisions : 0.0;
}

void Solver::increp () {
  if ((stats.reports++ % 19)) return;
  fprintf (out,
"%s  .\n"
"%s  ."
"       variables          fixed    eliminated       learned     agility"
"\n"
"%s  ."
" seconds         clauses      equivalent     conflicts      height      MB"
"\n"
"%s  .\n", prfx, prfx, prfx, prfx);
}

int Solver::remvars () const {
  int res = maxvar;
  res -= stats.vars.fixed;
  res -= stats.vars.equiv;
  res -= stats.vars.elim;
  res -= stats.vars.pure;
  res -= stats.vars.zombies;
  res -= stats.vars.autark;
  assert (res >= 0);
  return res;
}

void Solver::report (int v, char type) {
  if (opts.quiet || v > opts.verbose) return;
  char countch[2];
  countch[0] = countch[1] = ' ';
  if (type == lastype && hasterm ())  {
    typecount++;
    if (type != 'e' && type != 'p' && type != 'k') {
      countch[0] = '0' + (typecount % 10);
      if (typecount >= 10) countch[1] = '0' + (typecount % 100)/10;
    }
  } else {
    typecount = 1;
    if (lastype && hasterm ()) fputc ('\n', out);
    increp ();
  }
  assert (maxvar >= stats.vars.fixed + stats.vars.equiv + stats.vars.elim);
  fprintf (out, "%s%c%c%c%7.1f %7d %7d %6d %6d %6d %7d %6d %6.1f %2.0f %4.0f",
          prfx,
	  countch[1], countch[0], type, stats.seconds (),
	  remvars (),
	  stats.clauses.irr,
	  stats.vars.fixed, stats.vars.equiv, stats.vars.elim,
	   type=='e'?schedule.elim:
	  (type=='k'?schedule.block:stats.conflicts),
	  (type=='F'?limit.reduce.fresh:
	    (type=='l' || type=='+' || type == '-') ?
	    limit.reduce.learned : stats.clauses.lnd),
	  stats.height (),
	  agility / 100000.0,
	  mem / (double) (1<<20));
  if (!hasterm () || type=='0' || type=='1' || type=='?') fputc ('\n', out);
  else fputc ('\r', out);
  fflush (out);
  lastype = type;
}

void Solver::restart () {
  int skip = agility >= opts.skip * 100000;
  if (skip) stats.restart.skipped++;
  else stats.restart.count++;
  limit.restart.conflicts = stats.conflicts;
  int delta;
  if (opts.luby) delta = opts.restartint * luby (++limit.restart.lcnt);
  else {
    if (limit.restart.inner >= limit.restart.outer) {
      limit.restart.inner = opts.restartint;
      limit.restart.outer *= 100 + opts.restartouter;
      limit.restart.outer /= 100;
    } else {
      limit.restart.inner *= 100 + opts.restartinner;
      limit.restart.inner /= 100;
    }
    delta = limit.restart.inner;
  }
  limit.restart.conflicts += delta;
  if (!skip) undo (0);
  if (stats.restart.maxdelta < delta) {
    stats.restart.maxdelta = delta;
    report (1, skip ? 'N' : 'R');
  } else
    report (2, skip ? 'n' :'r');
}

void Solver::rebias () {
  limit.rebias.conflicts = stats.conflicts;
  int delta = opts.rebiasint * luby (++limit.rebias.lcnt);
  limit.rebias.conflicts += delta;
  if (!opts.rebias) return;
  stats.rebias.count++;
  for (Var * v = vars + 1; v <= vars + maxvar; v++) v->phase = 0;
  jwh (opts.rebiasorgonly);
  if (stats.rebias.maxdelta >= delta) report (2, 'b');
  else stats.rebias.maxdelta = delta, report (1, 'B');
}

inline int Solver::redundant (Cls * c) {
  assert (!level);
  assert (!c->locked);
  assert (!c->garbage);
#ifndef NDEBUG
  for (const int * p = c->lits; *p; p++) {
    Var * v = vars + (*p/2);
    assert (!v->mark);
    assert (v->type != ELIM);
  }
#endif
  int res = 0, * p, lit;
#ifdef PRECOCHECK
  bool shrink = false;
  for (p = c->lits; !res && (lit = *p); p++) {
    int other =  find (lit);
    Var * v = vars  + (other/2);
    assert ((v->type != ELIM));
    Val val = vals[other];
    if (val > 0 && !v->dlevel) res = 1;
    else if (val < 0 && !v->dlevel) shrink = true;
    else {
      val = lit2val (other);
      if (v->mark == val) { shrink = true; continue; }
      else if (v->mark == -val) res = 1;
      else v->mark = val;
    }
  }
  while (p > c->lits) vars[find (*--p)/2].mark = 0;
#ifndef NDEBUG
  for (p = c->lits; *p; p++) assert (!vars[find (*p)/2].mark);
#endif
  if (shrink || res) {
    for (p = c->lits; *p; p++) check.push (mem, *p);
    check.push (mem, 0);
  }
  if (res) return res;
#endif
  p = c->lits; int * q = p;
  while (!res && (lit = *p)) {
    p++;
    int other = find (lit);
    if (other != lit) simplified = true;
    Var * v = vars + (other/2);
    assert (v->type != ELIM);
    Val val = vals[other];
    if (val && !v->dlevel) {
      if (val > 0) res = 1;
    } else {
      val = lit2val (other);
      if (v->mark == val) continue;
      if (v->mark == -val) res = 1;
      else { assert (!v->mark); v->mark = val; *q++ = other; }
    }
  }
  *q = 0;

  if (!res) assert (!*q && !*p);

  int newsize = q - c->lits;

  if (!res &&
      newsize > 1 &&
      newsize <= opts.redsub &&
      (!c->lnd || c->binary)) {
    limit.budget.red += 10;
    setsig (c);
    for (p = c->lits; !res && (lit = *p) && limit.budget.red >= 0; p++) {
      INC (sigs.fw.l2.srch);
      if (!(fwsigs[lit] & c->sig)) { INC (sigs.fw.l2.hits); continue; }
      if (fwds[lits] > opts.fwmaxlen) continue;
      for (int i = 0; !res && i < fwds[lit] && limit.budget.red >= 0; i++) {
        limit.budget.red--;
	Cls * d = fwds[lit][i];
	assert (!d->trash);
	if (d->garbage) continue;
	if (d->size > (unsigned) newsize) continue;
	if (d->size > (unsigned) opts.redsub) continue;
	if (!c->lnd && d->lnd && !d->binary) continue;
	INC (sigs.fw.l1.srch);
	if (!sigsubs (d->sig, c->sig)) { INC (sigs.fw.l1.hits); continue; };
	int other;
	for (const int * r = d->lits; (other = *r); r++) {
	  Val u = lit2val (other), v = vars[other/2].mark;
	  if (u != v) break;
	}
	if ((res = !other)) {
	  stats.subs.red++;
	  limit.budget.red += 20;
	  if (!c->lnd && d->lnd) {
	    assert (d->binary);
	    d->lnd = false;
	    stats.clauses.irr++;
	  }
	}
      }
    }
  }

  while (q-- > c->lits) {
    assert (vars[*q/2].mark); vars[*q/2].mark = 0;
  }

  if (res) return res;

  if (newsize > 1 && newsize <= opts.redsub && (!c->lnd || c->binary))
    fwds[c->minlit ()].push (mem, c);

  res = newsize <= 1;
  if (!newsize) conflict = &empty;
  else if (newsize == 1) {
    LOG ("learned clause " << c->lits[0]);
    unit (c->lits[0]);
  } else if (newsize == 2 && !c->binary && 2 < c->size) {
    if (c->lnd) assert (stats.clauses.lnd), stats.clauses.lnd--;
    else assert (stats.clauses.orig), stats.clauses.orig--;
    stats.clauses.bin++;
    c->binary = true;
    push (binary, c);
    res = -1;
  } else assert (!res);

  return res;
}

void Solver::checkvarstats () {
#ifndef NDEBUG
  assert (!level);
  int fixed=0, equivalent=0, eliminated=0, pure=0, zombies=0, autarks=0;
  for (Var * v = vars + 1; v <= vars + maxvar; v++) {
    int lit = 2 * (v - vars);
    if (v->type == ELIM) eliminated++;
    else if (v->type == PURE) pure++;
    else if (v->type == ZOMBIE) zombies++;
    else if (v->type == AUTARK) autarks++;
    else if (vals[lit]) { assert (v->type == FIXED); fixed++; }
    else if (repr[lit]) { assert (v->type == EQUIV); equivalent++; }
    else assert (v->type == FREE);
  }
  assert (fixed == stats.vars.fixed);
  assert (eliminated == stats.vars.elim);
  assert (pure == stats.vars.pure);
  assert (zombies == stats.vars.zombies);
  assert (autarks == stats.vars.autark);
  assert (equivalent == stats.vars.equiv);
#endif
}

void Solver::freecls (Cls * c) {
  assert (c && !c->lnd);
  if (c->freed) return;
  int lit;
  for (const int * p = c->lits; (lit = *p); p++)
    if (vals[lit] > 0) break;
  if (lit) return;
  for (const int * p = c->lits; (lit = *p); p++) {
    Val tmp = vals[lit];
    assert (tmp <= 0);
    if (tmp == 0) continue;
    if (!vars[lit/2].dlevel) continue;
    lit ^= 1;
    LOG ("freeing " << lit);
    unassign (lit);
    flits.push (mem, lit);
  }
  fclss.push (mem, c);
  c->freed = true;
#ifndef NLOGPRECO
  dbgprint ("LOG freed clause ", c);
#endif
}

void Solver::autark () {
  if (!opts.autark) return;
  assert (!autarkmode);
  assert (!flits);
  assert (!fclss);
  autarkmode = true;
  measure = false;
  report (2, 'a');
  checkclean ();
  disconnect ();
  initorgs ();
  connect (binary, true);
  connect (original, true);
  int og = stats.clauses.gc, op = stats.clauses.gcpure;
  for (int dhi = stats.conflicts ? 5 : 4; dhi >= 0; dhi--) {
    int dhm = (1 << dhi);
    if (!(dhm & opts.autarkdhs)) continue;
    LOG ("autark strategy " << dhm);
    if (dhm == 16) jwh (true);
    bcp ();
    {
      Stack<LitScore> freevars;
      for (int idx = 1; idx <= maxvar; idx++) {
	Var * v = vars + idx;
	if (vars[idx].type == FREE)  {
	  assert (!vals[2*idx]);
	  int score = INT_MAX, lit = 2*idx;
	  switch (dhm & opts.autarkdhs)
	    {
	      case 1: score = idx; break;
	      case 2: score = -idx; break;
	      case 4: lit ^= 1; score = idx; break;
	      case 8: lit ^= 1; score = -idx; break;
	      case 16:
		{
		  int pjwh = jwhs[lit], njwh = jwhs[lit+1];
		  if (njwh >= pjwh) lit++;
		  score = max (pjwh, njwh);
		}
		break;
	      case 32: lit = phase (v); score = rnks[idx].heat; break;
	      default: continue;
	    }
	  freevars.push (mem, LitScore (lit, score));
	}
      }
      {
	Sorter<LitScore,FreeVarsLt> sorter (mem);
	sorter.sort (freevars.begin (), freevars);
      }
      for (LitScore * p = freevars.begin (); p < freevars.end (); p++) {
	int lit = p->lit;
	if (vals[lit]) continue;
#ifndef NLOGPRECO
	int score = p->score;
	LOG ("autark decide " << lit << " with score " << score);
#endif
	assume (lit);
	bcp ();
      }
      freevars.release (mem);
    }
    for (int i = 0; i <= 1; i++)
      for (Cls * c = i ? original.head : binary.head; c; c = c->prev)
	if (!c->lnd) freecls (c);
    LOG ("autark pruning starts with " << fclss << " conflicts");
    while (flits) {
      int lit = flits.pop ();
      assert (!vals [lit]);
      for (int i = 0; i < orgs[lit]; i++)
	freecls (orgs[lit][i]);
    }
#ifndef NLOGPRECO
    int ot = trail;
#endif
    undo (0, true);
    LOG ("autarky prunning freed " << (ot - trail) - saved <<
	 " literals in " << fclss << " clauses");
#if 0 // this made gcc 4.2.4 with -O3 not terminate (???)
    while (fclss) {
      Cls * c = fclss.pop ();
      assert (c->freed);
      c->freed = false;
    }
#else
    for (int i = 0; i < fclss; i++) {
      Cls * c = fclss[i];
      assert (c->freed);
      c->freed = false;
    }
    fclss.shrink ();
#endif
    int size = 0, nontrivial = 0;
    for (const int * p = saved.begin (); p < saved.end (); p++) {
      int lit = *p;
      assert (!repr[lit]);
      if (!vals[lit]) {
	size++;
	if (orgs[lit^1]) autark (lit), nontrivial++;
	else if (orgs[lit]) pure (lit);
	else zombie (vars + (lit/2));
      } else assert (vals[lit] > 0);
    }
    if (size) {
      LOG ("autarky of size " << size);
      LOG ("autarky with " << nontrivial << " non trivial assignments");
      LOG ("autarky with " << (size - nontrivial) << " trivial assignments");
      assert ((unsigned)dhi < sizeof(stats.autarks.dh)/sizeof(int));
      if (nontrivial) {
	stats.autarks.count++;
	stats.autarks.size += size;
	stats.autarks.dh[dhi]++;
      }
    }
#ifndef NDEBUG
    for (const int * p = saved.begin (); p < saved.end (); p++)
      assert (vals[*p]);
#endif
    saved.shrink ();
  }
  cleans ();
  delorgs ();
  gc ();
  measure = true;
  autarkmode = false;
  stats.clauses.gcautark += stats.clauses.gc - og;
  stats.clauses.gcautark -= stats.clauses.gcpure - op;
  report (2, 'a');
#ifdef CHECKWITHPICOSAT
  if (opts.check) picosatcheck_consistent ();
#endif
}

void Solver::decompose () {
  assert (!level);
  if (!opts.decompose) return;
  report (2, 'd');
  int n = 0;
  size_t bytes = 2 * (maxvar + 1) * sizeof (SCC);
  SCC * sccs = (SCC*) mem.callocate (bytes);
  Stack<int> work;
  saved.shrink ();
  int dfsi = 0;
  LOG ("starting scc decomposition");
  for (int root = 2; !conflict && root <= 2*maxvar + 1; root++) {
    if (sccs[root].idx) { assert (sccs[root].done); continue; }
    assert (!work);
    work.push (mem, root);
    LOG ("new scc root " << root);
    while (!conflict && work) {
      int lit = work.top ();
      if (sccs[lit^1].idx && !sccs[lit^1].done) {
	Val val = vals[lit];
	if (val < 0) {
	  LOG ("conflict while learning unit in scc");
	  conflict = &empty;
	} else if (!val) {
	  LOG ("learned clause " << lit);
	  unit (lit);
	  stats.sccs.fixed++;
	  bcp ();
	}
      }
      if (conflict) break;
      Stack<int> & os = occs[1^lit].bins;
      unsigned i = os;
      if (!sccs[lit].idx) {
	assert (!sccs[lit].done);
	sccs[lit].idx = sccs[lit].min = ++dfsi;
	saved.push (mem, lit);
	while (i) {
	  int other = os[--i];
	  if (sccs[other].idx) continue;
	  work.push (mem, other);
	}
      } else {
	work.pop ();
	if (!sccs[lit].done) {
	  while (i) {
	    int other = os[--i];
	    if (sccs[other].done) continue;
	    sccs[lit].min = min (sccs[lit].min, sccs[other].min);
	  }
	  SCC * scc = sccs + lit;
	  unsigned min = scc->min;
	  if (min != scc->idx) { assert (min < scc->idx); continue; }
	  n++; LOG ("new scc " << n);
	  int m = 0, prev = 0, other = 0;
	  while (!conflict && other != lit) {
	    other = saved.pop ();
	    sccs[other].done = true;
	    LOG ("literal " << other << " added to scc " << n);
	    if (prev) {
	      int a = find (prev), b = find (other);
	      if (a == b) continue;
	      if (a == (1^b)) {
		LOG ("conflict while merging scc");
		conflict = &empty;
	      } else if (val (a) || val (b)) {
		assert (val (a) == val (b));
	      } else {
		merge (prev, other, stats.sccs.merged);
		m++;
	      }
	    } else prev = other;
	  }
	  if (m) stats.sccs.nontriv++;
	}
      }
    }
  }

  LOG ("found " << n << " sccs");
  assert (conflict || dfsi <= 2 * maxvar);
  work.release (mem);
  mem.deallocate (sccs, bytes);
  bcp ();
#ifndef NDEBUG
  checkvarstats ();
#endif
  report (2, 'd');
}

void Solver::disconnect () {
  cleangate (), cleantrash ();
  for (int lit = 2; lit <= 2*maxvar + 1; lit++)
    occs[lit].bins.release (mem), occs[lit].large.release (mem);
  assert (!orgs && !fwds && !fwsigs);
  clrbwsigs ();
}

void Solver::connect (Anchor<Cls> & anchor, bool orgonly) {
  for (Cls * p = anchor.tail; p; p = p->next) {
    assert (&anchor == &this->anchor (p));
    if (!orgonly || !p->lnd) connect (p);
  }
}

void Solver::gc (Anchor<Cls> & anchor, const char * type) {
  limit.budget.red = 10000;
  Cls * p = anchor.tail;
  int collected = 0;
  anchor.head = anchor.tail = 0;
  anchor.count = 0;
  while (p) {
    Cls * next = p->next;
#ifndef NDEBUG
    p->next = p->prev = 0;
#endif
    int red = 1;
    if (!p->garbage) {
      red = redundant (p);
      if (red > 0) p->garbage = true;
    }
    if (p->garbage) { collect (p); collected++; }
    else if (!red) push (anchor, p);
    else assert (connected (binary, p));
    p = next;
  }
#ifndef NLOGPRECO
  if (collected)
    LOG ("collected " << collected << ' ' << type << " clauses");
#else
  (void) type;
#endif
}

inline void Solver::checkclean () {
#ifndef NDEBUG
  if (opts.check) {
    for (int i = 0; i <= 3 + opts.glue; i++) {
      Cls * p;
      if (i == 0) p = original.head;
      if (i == 1) p = binary.head;
      if (i == 2) p = fresh.head;
      if (i >= 3) p = learned[i-3].head;
      while (p) {
	for (int * q = p->lits; *q; q++) {
	  int lit = *q;
	  assert (!repr[lit]);
	  assert (!vals[lit]);
	  Var * v = vars + (lit/2);
	  assert (v->type == FREE);
	}
	p = p->prev;
      }
    }
  }
#endif
}

void Solver::gc () {
#ifndef NLOGPRECO
  size_t old = stats.collected;
#endif
  int round = 0;
  report (2, 'g');
  undo (0);
  disconnect ();
RESTART:
  LOG ("gc round " << round);
  initfwds ();
  initfwsigs ();
  gc (binary, "binary");
  gc (original, "original");
  gc (fresh, "fresh");
  for (int glue = 0; glue <= opts.glue; glue++) {
    char buffer[80];
    sprintf (buffer, "learned[%u]", glue);
    gc (learned[glue], buffer);
  }
  delfwds ();
  delfwsigs ();
  if (needtoflush ()) { if (!bcp ()) return; round++; goto RESTART; }
  connect (binary);
  connect (original);
  for (int glue = 0; glue <= opts.glue; glue++)
    connect (learned[glue]);
  connect (fresh);
  checkclean ();
#ifndef NLOGPRECO
  size_t bytes = stats.collected - old;
  LOG ("collected " << bytes << " bytes");
  dbgprint ();
#endif
  bcp ();
  report (2, 'c');
  stats.gcs++;
#ifdef CHECKWITHPICOSAT
  if (opts.check >= 2) picosatcheck_consistent ();
#endif
}

inline int Solver::recyclelimit () const {
  return limit.reduce.learned + stats.clauses.lckd;
}

inline bool Solver::recycling () const {
  return stats.clauses.lnd >= recyclelimit ();
}

void Solver::reduce () {
  assert (!conflict);
  assert (trail == queue);
  stats.reductions++;
  Cls * f;
  while ((f = fresh.tail)) {
    dequeue (fresh, f);
    assert (f->fresh);
    assert (!f->binary);
    assert (f->lnd);
    f->fresh = false;
    push (anchor (f), f);
  }
  int goal = (stats.clauses.lnd - stats.clauses.lckd)/2, count = 0;
  for (int glue = opts.glue; glue >= opts.sticky && count < goal; glue--) {
    Cls * next;
    for (Cls * p = learned[glue].tail; p && count < goal; p = next) {
      next = p->next;
      if (p->locked) continue;
      p->garbage = true;
      count++;
    }
  }
  gc ();
  jwh (opts.rebiasorgonly);
  if (opts.limincmode == 2) enlarge ();
  else if (count >= goal/2) report (1, '/');
  else report (1, '='), enlarge ();
}

inline void Solver::checkeliminated () {
#ifndef NDEBUG
  if (opts.check) {
    for (int i = 0; i <= 3 + (int)opts.glue; i++) {
      Cls * p;
      if (i == 0) p = original.head;
      if (i == 1) p = binary.head;
      if (i == 1) p = fresh.head;
      if (i >= 3) p = learned[i-3].head;
      while (p) {
	for (int * q = p->lits; *q; q++) {
	  int lit = *q;
	  Var * v = vars + (lit/2);
	  assert (v->type != ELIM);
	}
	p = p->prev;
      }
    }
  }
#endif
}

void Solver::probe () {
  if (!maxvar) return;
  stats.sw2simp ();
  assert (!conflict);
  assert (queue2 == trail);
  undo (0);
  stats.probe.phases++;
  measure = false;
  long long bound;
  if (opts.rtc == 2 || opts.probertc == 2 ||
      ((opts.rtc == 1 || opts.probertc == 1) && !stats.probe.rounds))
    bound = LLONG_MAX;
  else {
    bound = stats.props.simp + opts.probeint;
    for (int i = stats.probe.phases; i <= opts.probeboost; i++)
      bound += opts.probeint;
  }
  int last = -1, first = 0, filled = 0, pos, delta;
FILL:
  filled++;
  assert (maxvar);
  pos = rng.next  () % maxvar;
  delta = rng.next () % maxvar;
  if (!delta) delta++;
  while (gcd (delta, maxvar) != 1)
    if (++delta == maxvar) delta = 1;
  LOG ("probing starting position " << pos << " and delta " << delta);
  report (2, 'p');
  Progress progress (this, 'p');
  while (stats.props.simp < bound && !conflict) {
    assert (!level);
    progress.tick ();
    int idx = pos + 1;
    assert (1 <= idx && idx <= maxvar);
    if (idx == first) {
      stats.probe.rounds++;
      if (last == filled) goto FILL;
      break;
    }
    if (!first) first = idx;
    pos += delta;
    if (pos >= maxvar) pos -= maxvar;
    Var & v = vars[idx];
    if (v.type != FREE && v.type != EQUIV) continue;
    int lit = 2*idx;
    if (!occs[lit].bins || !occs[lit+1].bins) continue;
    assert (!vals[lit]);
    if (repr[lit]) continue;
    assert (!level);
    long long old = stats.props.simp;
    LOG ("probing " << lit);
    stats.probe.variables++;
    assume (lit);
    bool ok = bcp ();
    undo (0, ok);
    if (!ok) goto FAILEDLIT;
    lit ^= 1;
    LOG ("probing " << lit);
    assume (lit);
    stats.probe.variables++;
    if (!bcp ()) { undo (0); goto FAILEDLIT; }
    {
      int * q = saved.begin (), * m = q, * eos = saved.end ();
      for (const int * p = q; p < eos; p++) {
	int other = *p;
	if (other == (lit^1)) continue;
	if (vals[other] < 0) { *q++ = other^1; continue; }
	if (!vals[other]) continue;
	*q++ = *m;
	*m++ = other;
      }
      undo (0);
      if (m == saved.begin ()) continue;
      for (const int * p = m; p < q; p++)
	 merge (lit, *p, stats.probe.merged);
      for (const int * p = saved.begin (); p < m; p++) {
	stats.probe.lifted++;
	last = filled;
	int other = *p;
	LOG ("lifting " << other);
	LOG ("learned clause " << other);
	unit (other);
      }
    }
    goto BCPANDINCBOUND;
FAILEDLIT:
    stats.probe.failed++;
    last = filled;
    LOG ("learned clause " << (1^lit));
    unit (1^lit);
BCPANDINCBOUND:
    bcp ();
    if (bound != LLONG_MAX)
      bound += opts.probereward + (stats.props.simp - old);
  }
  measure = true;
  limit.props.probe = opts.probeprd;
  limit.props.probe *= opts.probeint;
  limit.props.probe += stats.props.srch;
  limit.fixed.probe = stats.vars.fixed;
  checkvarstats ();
  report (2, 'p');
  stats.sw2srch ();
}

bool Solver::andgate (int lit) {
  // TODO L2 sigs?
  assert (vars[lit/2].type != ELIM);
  assert (!gate && !gatestats);
  if (!opts.subst) return false;
  if (!opts.ands) return false;
  if (!orgs[lit] || !orgs[1^lit]) return false;
  Cls * b = 0;
  int other, notlit = (1^lit), bound = 1000;
  for (int i = 0; !b && i < orgs[lit] && bound-- >= 0; i++) {
    Cls * c = orgs[lit][i];
    if (!sigsubs (c->sig, bwsigs[lit^1])) continue;
    assert (!gate);
    gcls (c);
    gatelen = 0;
    bool hit = false;
    for (const int * p = c->lits; (other = *p); p++) {
      if (lit == other) { hit = true; continue; }
      assert (!vars[other/2].mark);
      vars[other/2].mark = -lit2val (other);
      gatelen++;
    }
    assert (hit);
    assert (gatelen);
    int count = gatelen;
    for (int j = 0; j < orgs[notlit] && bound-- >= 0; j++) {
      if (orgs[notlit] - j < count) break;
      Cls * d = orgs[notlit][j];
      if (d->lits[2]) continue;
      int pos = (d->lits[1] == notlit);
      assert (d->lits[pos] == notlit);
      assert (d->binary);
      other = d->lits[!pos];
      if (vars[other/2].mark != lit2val (other)) continue;
      vars[other/2].mark = 0;
      gcls (d);
      if (!--count) break;
    }
    for (const int * p = c->lits; (other = *p); p++)
      vars[other/2].mark = 0;
    if (count) cleangate (); else b = c;
  }
  if (!b) { assert (!gate); return false; }
  assert (gate == gatelen + 1);
  gatestats = (gatelen >= 2) ? &stats.subst.ands : &stats.subst.nots;
  gatepivot = lit;
  posgate = 1;
#ifndef NLOGPRECO
  printf ("%sLOG %s gate %d = ", prfx, (gatelen >= 2) ? "and" : "not", lit);
  for (const int * p = b->lits; (other = *p); p++) {
    if (other == lit) continue;
    printf ("%d", (1^other));
    if (p[1] && (p[1] != lit || p[2])) fputs (" & ", stdout);
  }
  fputc ('\n', stdout);
  dbgprintgate ();
#endif
  return true;
}

bool Solver::xorgate (int lit) {
  // TODO L2 sigs
  assert (vars[lit/2].type != ELIM);
  assert (!gate && !gatestats);
  if (!opts.subst) return false;
  if (!opts.xors) return false;
  if (orgs[lit] < 2 || orgs[1^lit] < 2) return false;
  if (orgs[lit] > orgs[1^lit]) lit ^= 1;
  Cls * b = 0;
  int len = 0, other, bound = 600;
  for (int i = 0; i < orgs[lit] && !b && bound-- >= 0; i++) {
    Cls * c = orgs[lit][i];
    const int maxlen = 20;
    assert (maxlen < 31);
    if (c->size > (unsigned) maxlen) continue;
    if (!c->lits[2]) continue;
    assert (!c->binary);
    if (!sigsubs (c->sig, bwsigs[lit^1])) continue;
    int * p;
    for (p = c->lits; *p; p++)
      ;
    len = p - c->lits;
    assert (len >= 3);
    int required = (1 << (len-1));
    for (p = c->lits; (other = *p); p++) {
      if (other == lit) continue;
      if (orgs[other] < required) break;
      if (orgs[other^1] < required) break;
      if (!sigsubs (c->sig, bwsigs[other])) break;
      if (!sigsubs (c->sig, bwsigs[1^other])) break;
    }
    if (other) continue;
    assert (!gate);
    assert (0 < len && len <= maxlen);
    unsigned signs;
    for (signs = 0; signs < (1u<<len) && bound-- >= 0; signs++) {
      if (parity (signs)) continue;
      int start = 0, startlen = INT_MAX, s = 0;
      for (p = c->lits; (other = *p); p++, s++) {
	if ((signs & (1u<<s))) other ^= 1;
	int tmp = orgs[other];
	if (start && tmp >= startlen) continue;
	startlen = tmp;
	start = other;
      }
      assert (s == len && start && startlen < INT_MAX);
      int j;
      Cls * found = 0;
      for (j = 0; !found && j < orgs[start] && bound-- >= 0; j++) {
	Cls * d = orgs[start][j];
	if (d->sig != c->sig) continue;
	for (p = d->lits; *p; p++)
	  ;
	if (p - d->lits != len) continue;
	bool hit = false;
	s = 0;
	for (p = c->lits; (other = *p); p++, s++) {
	  if ((signs & (1u<<s))) other ^= 1;
	  if (other == start) { hit = true; continue; }
	  if (!d->contains (other)) break;
	}
	assert (other || hit);
	if (!other) found = d;
      }
      assert (bound < 0 || signs || found);
      if (!found) break;
      assert (required);
      required--;
      gcls (found);
    }
    if (signs == (1u<<len)) { assert (!required); b = c; }
    else cleangate ();
  }
  if (!b) { assert (!gate); return false; }
  assert (len >= 3);
  assert (gate == (1<<(len-1)));
  gatepivot = lit;
  gatestats = &stats.subst.xors;
  gatelen = len - 1;
  int pos = -1, neg = gate;
  for (;;) {
    while (pos+1 < neg && gate[pos+1]->contains (lit)) pos++;
    assert (pos >= gate || gate[pos+1]->contains(1^lit));
    if (pos+1 == neg) break;
    while (pos < neg-1 && gate[neg-1]->contains ((1^lit))) neg--;
    assert (neg < 0 || gate[neg-1]->contains(lit));
    if (pos+1 == neg) break;
    assert (pos < neg);
    swap (gate[++pos], gate[--neg]);
  }
  posgate = pos + 1;
#ifndef NLOGPRECO
  printf ("%sLOG %d-ary xor gate %d = ", prfx, len-1, (lit^1));
  for (const int * p = b->lits; (other = *p); p++) {
    if (other == lit) continue;
    printf ("%d", other);
    if (p[1] && (p[1] != lit || p[2])) fputs (" ^ ", stdout);
  }
  fputc ('\n', stdout);
  dbgprintgate ();
#endif
  return true;
}

Cls * Solver::find (int a, int b, int c) {
  unsigned sig = listig (a) | listig (b) | listig (c);
  unsigned all = bwsigs[a] & bwsigs[b] & bwsigs[c];
  if (!sigsubs (sig, all)) return 0;
  int start = a;
  if (orgs[start] > orgs[b]) start = b;
  if (orgs[start] > orgs[c]) start = c;
  for (int i = 0; i < orgs[start]; i++) {
    Cls * res = orgs[start][i];
    if (res->sig != sig) continue;
    assert (res->lits[0] && res->lits[1]);
    if (!res->lits[2]) continue;
    assert (!res->binary);
    if (res->lits[3]) continue;
    int l0 = res->lits[0], l1 = res->lits[1], l2 = res->lits[2];
    if ((a == l0 && b == l1 && c == l2) ||
        (a == l0 && b == l2 && c == l1) ||
        (a == l1 && b == l0 && c == l2) ||
        (a == l1 && b == l2 && c == l0) ||
        (a == l2 && b == l0 && c == l1) ||
        (a == l2 && b == l1 && c == l2)) return res;
  }
  return 0;
}

int Solver::itegate (int lit, int cond, int t) {
  Cls * c = find (lit^1, cond, t^1);
  if (!c) return 0;
  int start = lit, nc = (cond^1), bound = 200;
  if (orgs[nc] < orgs[start]) start = nc;
  unsigned sig = listig (lit) | listig (nc);
  for (int i = 0; i < orgs[start] && --bound >= 0; i++) {
    Cls * d = orgs[start][i];
    if (!sigsubs (sig, d->sig)) continue;
    assert (d->lits[0] && d->lits[1]);
    if (!d->lits[2]) continue;
    assert (!d->binary);
    if (d->lits[3]) continue;
    int l0 = d->lits[0], l1 = d->lits[1], l2 = d->lits[2], res;
         if (l0 == lit && l1 == nc) res = l2;
    else if (l1 == lit && l0 == nc) res = l2;
    else if (l0 == lit && l2 == nc) res = l1;
    else if (l2 == lit && l0 == nc) res = l1;
    else if (l1 == lit && l2 == nc) res = l0;
    else if (l2 == lit && l1 == nc) res = l0;
    else continue;
    Cls * e = find (lit^1, nc, res^1);
    if (!e) continue;
    gcls (d);
    gcls (c);
    gcls (e);
    return res;
  }
  return 0;
}

bool Solver::itegate (int lit) {
  // TODO L2 sigs
  assert (vars[lit/2].type != ELIM);
  assert (!gate && !gatestats);
  if (!opts.subst) return false;
  if (!opts.ites) return false;
  if (orgs[lit] < 2 || orgs[1^lit] < 2) return false;
  if (orgs[lit] > orgs[1^lit]) lit ^= 1;
  Cls * b = 0;
  int cond = 0, t = 0, e = 0;
  for (int i = 0; i < orgs[lit] && !b; i++) {
    Cls * c = orgs[lit][i];
    assert (c->lits[0] && c->lits[1]);
    if (!c->lits[2]) continue;
    assert (!c->binary);
    if (c->lits[3]) continue;
    assert (!gate);
    int o0, o1, l0 = c->lits[0], l1 = c->lits[1], l2 = c->lits[2];
    if (lit == l0) o0 = l1, o1 = l2;
    else if (lit == l1) o0 = l0, o1 = l2;
    else assert (lit == l2), o0 = l0, o1 = l1;
    assert (!gate);
    gcls (c);
    if ((e = itegate (lit, cond = o0, t = o1)) ||
        (e = itegate (lit, cond = o1, t = o0))) b = c;
    else cleangate ();
  }
  if (!b) { assert (!gate); return false; }
  assert (cond && t && e);
  assert (gate == 4);
  gatestats = &stats.subst.ites;
  gatepivot = lit;
  posgate = 2;
#ifndef NLOGPRECO
  LOG ("ite gate " << lit << " = " <<
       (1^cond) << " ? " << (1^t) << " : " << (1^e));
  dbgprintgate ();
#endif
  return true;
}

bool Solver::resolve (Cls * c, int pivot, Cls * d, int tryonly) {
  if (tryonly) resotfs = reslimhit = false;
  assert (tryonly || (c->dirty && d->dirty));
  assert (tryonly != (vars[pivot/2].type == ELIM));
  assert (!vals[pivot] && !repr[pivot]);
  if (!tryonly && gate && c->gate == d->gate) return false;
  if (elimode) stats.elim.resolutions++;
  else { assert (blkmode); stats.blkd.res++; }
  int other, notpivot = (1^pivot), clits = 0, dlits = 0;
  bool found = false, res = true;
  const int * p;
  assert (!lits);
  for (p = c->lits; (other = *p); p++) {
    if (other == pivot) { found = true; continue; }
    assert (other != notpivot);
    assert (find (other) == other);
    assert (other != notpivot);
    if (other == pivot) continue;
    Val v = vals [other];
    if (v < 0) continue;
    if (v > 0) { res = false; goto DONE; }
    assert (!vars[other/2].mark);
    Val u = lit2val (other);
    vars[other/2].mark = u;
    lits.push (mem, other);
    clits++;
  }
  assert (found);
  found = false;
  for (p = d->lits; (other = *p); p++) {
    if (other == notpivot) { found = true; continue; }
    assert (other != pivot);
    assert (find (other) == other);
    assert (other != pivot);
    if (other == notpivot) continue;
    Val v = vals[other];
    if (v < 0) continue;
    if (v > 0) { res = false; goto DONE; }
    dlits++;
    v = vars[other/2].mark;
    Val u = lit2val (other);
    if (!v) {
      vars[other/2].mark = u;
      lits.push (mem, other);
    } else if (v != u) { res = false; goto DONE; }
  }
  assert (found);
  if (tryonly == 1 && opts.otfs) {
#ifndef NLOGPRECO
    bool logresolvent = false;
#endif
    if (lits == clits && (!blkmode || opts.blockotfs)) {
#ifndef NLOGPRECO
      dbgprint ("LOG static on-the-fly strengthened clause ", c);
      dbgprint ("LOG static on-the-fly antecedent ", d);
      logresolvent = true;
#endif
      if (c->gate) cleangate ();
      remove (pivot, c);
      resotfs = true;
      if (clits == 1) stats.otfs.stat.bin++;
      else if (clits == 2) stats.otfs.stat.trn++;
      else stats.otfs.stat.large++;
    }
    if (lits == dlits && (!blkmode || opts.blockotfs)) {
#ifndef NLOGPRECO
      dbgprint ("LOG static on-the-fly strengthened clause ", d);
      dbgprint ("LOG static on-the-fly antecedent ", c);
      logresolvent = true;
#endif
      if (d->gate) cleangate ();
      remove (notpivot, d);
      resotfs = true;
      if (dlits == 1) stats.otfs.stat.bin++;
      else if (dlits == 2) stats.otfs.stat.trn++;
      else stats.otfs.stat.large++;
    }
#ifndef NLOGPRECO
    if (logresolvent) {
      printf ("%sLOG static on-the-fly subsuming resolvent", prfx);
      for (p = lits.begin (); p < lits.end (); p++) printf (" %d", *p);
      fputc ('\n', stdout);
    }
#endif
  }
DONE:
  for (p = lits.begin (); p < lits.end (); p++) {
    Var * u = vars + (*p/2);
    assert (u->mark);
    u->mark = 0;
  }
  if (tryonly <= 1 && res) {
    if (!lits) {
      LOG ("conflict in resolving clauses");
      conflict = &empty;
      res = false;
    } else if (tryonly <= 1 && lits == 1) {
      LOG ("learned clause " << lits[0]);
      unit (lits[0]);
      res = false;
    } else if (!tryonly) {
      if (!fworgs ()) {
	bworgs ();
	clause (false, 0);
      }
    } else {
      if (lits > opts.reslim) reslimhit = true;
    }
  }
  lits.shrink ();
  return res;
}

inline void Solver::block (Cls * c, int lit) {
  assert (opts.block);
#ifndef NLOGPRECO
  dbgprint ("LOG blocked clause ", c);
  LOG ("blocked on literal " << lit);
#endif
  assert (!blklit);
  blklit = lit;
  if (c->gate) cleangate ();
  elits.push (mem, 0);
  bool found = false;
  int other;
  for (const int * p = c->lits; (other = *p); p++) {
    if (other == lit) { found = true; continue; }
    elits.push (mem, other);
  }
  assert (found);
  elits.push (mem, lit);
  elits.push (mem, INT_MAX);
  // NOTE: we can not simply turn binary blocked clauses
  // into learned clauses, which can be removed without
  // any further check in the future, because adding
  // resolvents to the CNF (either through learning,
  // elimination, dominator clauses etc) may unblock
  // clauses.  These binary clauses would need to
  // be treated differently in all operations that
  // remove clauses from the CNF.
  recycle (c);
  blklit = 0;
}

bool Solver::trelim (int idx) {
  assert (!elimvar);
  assert (!conflict);
  assert (queue == trail);
  assert (vars[idx].type == FREE);
RESTART:
  int lit = 2*idx, pos = orgs[lit], neg = orgs[lit^1];
  if (val (lit)) return false;
  if (opts.elimgain <= 1 && (pos <= 1 || neg <= 1)) return true;
  if (opts.elimgain <= 0 && pos <= 2 && neg <= 2) return true;
  LOG ("trelim " << lit << " bound " << elms[idx].heat);
  if (gate) LOG ("actually trelim gate for " << gatepivot);
  for (int sign = 0; sign <= 1; sign++)
    for (int i = 0; i < orgs[lit^sign]; i++)
      if (orgs[lit^sign][i]->size > (unsigned)opts.elimclim)
	return false;
  int gain = pos + neg, l = opts.elimgain, found, i, j;
  Cls * c, * d;
  if (gate) {
    assert (gatepivot/2 == lit/2);
    int piv, s0, e0, s1, e1;
    piv = gatepivot;
    s0 = 0, e0 = posgate;
    s1 = posgate, e1 = gate;
    for (i = s0; !conflict && i < e0 && gain >= l; i++) {
      found = 0;
      c = gate[i];
      for (j = 0; !conflict && gain >= l && j < orgs[piv^1]; j++) {
	d = orgs[piv^1][j];
	if (d->gate) continue;
	if (resolve (c, piv, d, true)) gain--, found++;
	if (needtoflush ()) { if (!bcp ()) return false; goto RESTART; }
	if (val (piv)) return false;
	if (resotfs) goto RESTART;
	if (reslimhit) gain = INT_MIN, found++;
      }
      if (opts.block && opts.blockimpl && !found) { lit = piv; goto BLKD; }
    }
    for (i = s1; !conflict && i < e1 && gain >= l; i++) {
      found = 0;
      c = gate[i];
      for (j = 0; !conflict && gain >= l && j < orgs[piv]; j++) {
	d = orgs[piv][j];
	if (d->gate) continue;
	if (resolve (d, piv, c, true)) gain--, found++;
	if (needtoflush ()) { if (!bcp ()) return false; goto RESTART; }
	if (val (piv)) return false;
	if (resotfs) goto RESTART;
	if (reslimhit) gain = INT_MIN, found++;
      }
      if (opts.block && opts.blockimpl && !found) { lit = 1^piv; goto BLKD; }
    }
  } else {
    for (i = 0; !conflict && gain >= l && i < orgs[lit]; i++) {
      found = 0;
      c = orgs[lit][i];
      for (j = 0; !conflict && gain >= l && j < orgs[lit^1]; j++) {
	d = orgs[lit^1][j];
	if (resolve (c, lit, d, true)) gain--, found++;
	if (needtoflush ()) { if (!bcp ()) return false; goto RESTART; }
	if (val (lit)) return false;
	if (resotfs) goto RESTART;
	if (reslimhit) gain = INT_MIN, found++;
      }
      if (opts.block && opts.blockimpl && !found) {
BLKD:
	block (c, lit);
	stats.blkd.impl++;
	stats.blkd.all++;
	goto RESTART;
      }
    }
  }
  if (conflict) return false;
  LOG ("approximated gain for " << lit << " is " << gain);
  if (!val (lit) && gain < l) {
    measure = false;
    asymode = true;
ASYMMAGAIN:
    for (int sign = 0; sign <= 1; sign++, lit^=1) {
      if (orgs[lit] < 2) goto DONE;
      assert (!val (lit));
      if (orgs[lit] <= opts.elimasym) {
	if (limit.props.asym < 0) goto DONE;
	long long old = stats.props.simp;
	LOG ("trying to eliminate one out of " << orgs[lit] <<
	     " occurences of " << lit);
	for (i = orgs[lit]-1; !conflict && i >= 0; i--) {
	  Cls * c = orgs[lit][i];
	  if (c->gate) continue;
#ifndef NLOGPRECO
	  dbgprint ("LOG trying to eliminate literal from ", c);
#endif
	  assert (c->lits[1]);
	  assume (lit);
	  bool ok = bcp (), fl = true;
	  int other;
	  for (const int * p = c->lits; ok && (other = *p); p++) {
	    if (val (other)) continue;
	    assert (other != lit);
	    assume (other^1);
	    ok = bcp ();
	    fl = false;
	  }
	  undo (0);
	  if (ok) {
	    limit.props.asym -= stats.props.simp - old;
	    continue;
	  }
	  stats.str.asym++;
	  LOG ("asymmetric branching eliminates literal " << lit);
	  if (fl) assign (lit^1); else remove (lit, c);
	  limit.props.asym += opts.elimasymreward;
	  if (val (lit)) goto DONE;
	  if (needtoflush ()) {
	    if (!bcp ()) goto DONE;
	    if (val (lit)) goto DONE;
	    goto ASYMMAGAIN;
	  }
	  if (orgs[lit] < 2) goto DONE;
	  if (orgs[lit] == 2 && orgs[lit^1] == 2) goto DONE;
	}
      }
    }
DONE:
    measure = true;
    asymode = false;
    if (conflict) return false;
    if (vals[lit]) return false;
    if (opts.elimgain <= 1 && (orgs[lit] <= 1 || orgs[1^lit] <= 1))
      return true;
    if (opts.elimgain <= 0 && orgs[lit] <= 2 && orgs[1^lit] <= 2)
      return true;
  }
  return gain >= l;
}

void Solver::elim (int idx) {
  assert (!elimvar);
  elimvar = idx;
  int lit = 2 * idx;
  assert (!conflict);
  assert (!units);
  assert (queue == trail);
  assert (vars[idx].type == FREE);
  assert (!vals[lit]);
  assert (!repr[lit]);
  LOG ("elim " << lit);
  assert (!vals[lit] && !repr[lit]);
  {
    int slit;
    Cls ** begin, ** end;
    if (gate) {
      slit = gatepivot;
      int pos = posgate;
      int neg = gate - pos;
      assert (pos > 0 && neg > 0);
      begin = gate.begin ();
      if (pos < neg) end = begin + posgate;
      else begin += posgate, end = gate.end (), slit ^= 1;
    } else {
      slit = lit;
      int pos = orgs[lit];
      int neg = orgs[lit^1];
      if (!pos && !neg) zombie (vars + idx);
      else if (!pos) pure (lit^1);
      else if (!neg) pure (lit);
      if (!pos || !neg) { elimvar = 0; return; }
      if (pos < neg) begin = orgs[lit].begin (), end = orgs[lit].end ();
      else begin = orgs[1^lit].begin (), end = orgs[1^lit].end (), slit ^= 1;
    }
    elits.push (mem, 0);
    LOG ("elit 0");
    for (Cls ** p = begin; p < end; p++) {
      elits.push (mem, 0);
      LOG ("elit 0");
      Cls * c = *p;
      int other;
      for (int * p = c->lits; (other = *p); p++) {
	if (other == (slit)) continue;
	assert (other != (1^slit));
	elits.push (mem, other);
	LOG ("elit " << other);
      }
    }
    elits.push (mem, slit);
    LOG ("elit " << slit);
    elits.push (mem, 0);
  }

  vars[idx].type = ELIM;
  stats.vars.elim++;

  for (int sign = 0; sign <= 1; sign++)
    for (int i = 0; i < orgs[lit^sign]; i++) {
      Cls *  c = orgs[lit^sign][i];
      c->dirty = true;
    }

  int gatecount = gate;
  if (gatecount) {
    LOG ("actually elim gate for " << gatepivot);
    assert (gatepivot/2 == lit/2);
    int piv = gatepivot;
    for (int i = 0; !conflict && i < posgate; i++)
      for (int j = 0; !conflict && j < orgs[piv^1]; j++)
	resolve (gate[i], piv, orgs[piv^1][j], false);
    for (int i = posgate; !conflict && i < gate; i++)
      for (int j = 0; !conflict && j < orgs[piv]; j++)
	resolve (orgs[piv][j], piv, gate[i], false);
  } else {
    for (int i = 0; !conflict && i < orgs[lit]; i++)
      for (int j = 0; !conflict && j < orgs[lit^1]; j++)
	resolve (orgs[lit][i], lit, orgs[lit^1][j], false);
  }

  for (int sign = 0; sign <= 1; sign++)
    for (int i = 0; i < orgs[lit^sign]; i++)
      orgs[lit^sign][i]->dirty = false;

  assert (gate == gatecount);
  cleangate (), cleantrash ();

  for (int sign = 0; sign <= 1; sign++)
    recycle (lit^sign);

  elimvar = 0;
}

inline long long Solver::clauses () const {
  return stats.clauses.orig + stats.clauses.lnd + stats.clauses.bin;
}

inline bool Solver::hasgate (int idx) {
  assert (0 < idx && idx <= maxvar);
  assert (!gate);
  int lit = 2*idx;
  if (andgate (lit)) return true;
  if (andgate (1^lit)) return true;
  if (xorgate (lit)) return true;
  if (itegate (lit)) return true;
  return false;
}

void Solver::cleans () {
  assert (!level);
  assert (elimode || blkmode || autarkmode);
  assert (orgs);
  for (int i = 0; i <= 3 + (int)opts.glue; i++) {
    Cls * p = 0;
    if (i == 0) p = binary.head;
    if (i == 1) p = original.head;
    if (i == 2) p = fresh.head;
    if (i >= 3) p = learned[i-3].head;
    while (p) {
      assert (!p->locked); assert (!p->garbage);
      Cls * prev = p->prev;
      int other;
      for (int * q = p->lits; (other = *q); q++) {
	Var * v  = vars + (other/2);
	if (v->type == FREE) continue;
	if (v->type == FIXED) continue;
	break;
      }
      if (other) { /*assert (p->lnd);*/ p->garbage = true; }
      p = prev;
    }
  }
}

void Solver::elim () {
  if (!eliminating ()) return;
  stats.elim.phases++;
  elimode = true;
  elimvar = 0;
  checkclean ();
  disconnect ();
  initorgs ();
  initfwds ();
  initfwsigs ();
  clrbwsigs ();
  connect (binary, true);
  connect (original, true);
  if (!bcp ()) return;
  long long bound;
  limit.budget.bw.sub = limit.budget.fw.sub = opts.elimint;
  limit.budget.bw.str = limit.budget.fw.str = opts.elimint;
  if (opts.rtc == 2 || opts.elimrtc == 2 ||
      ((opts.rtc == 1 || opts.elimrtc == 1) && !stats.elim.rounds))
    bound = LLONG_MAX;
  else {
    bound = stats.elim.resolutions + opts.elimint;
    for (int i = stats.elim.phases; i <= opts.elimboost; i++)
      bound += opts.elimint;
  }
  limit.props.asym = opts.elimasymint;
  if (schedule.elim) {
    for (int idx = 1; idx <= maxvar; idx++) {
      int lit = 2*idx;
      //if (pos <= 2 || neg <= 2 || schedule.elim.contains (elms + idx))
       	touch (lit);
    }
  } else {
    for (int idx = 1; idx <= maxvar; idx++) touch (2*idx);
  }
  pure ();
  report (2, 'e');
  Progress progress (this, 'e');
  while (!conflict && schedule.elim) {
    Rnk * e = schedule.elim.max ();
    int idx = e - elms, lit = 2*idx, pos = orgs[lit], neg = orgs[1^lit];
    if (pos > 1 && neg > 1 && (pos > 2 || neg > 2))
      if (stats.elim.resolutions > bound) break;
    progress.tick ();
    (void) schedule.elim.pop ();
    Var * v = vars + idx;
    if (v->type != FREE) continue;
    if (!pos && !neg) zombie (vars + idx);
    else if (!pos) pure (lit^1);
    else if (!neg) pure (lit);
    else {
      if (hasgate (idx)) assert (gatestats);
      bool eliminate = trelim (idx);
      if (needtoflush () && !bcp ()) break;
      if (!eliminate || vals[lit]) { cleangate (); continue; }
      if (gatestats) {
	gatestats->count += 1;
	gatestats->len += gatelen;
	stats.vars.subst++;
      }
      elim (idx);
      if (needtoflush () && !bcp ()) break;
      if (bound != LLONG_MAX) bound += opts.elimreward;
    }
  }

  report (2, 'e');

#ifndef NDEBUG
  for (int idx = 1; !conflict && idx <= maxvar; idx++) {
    if (vars[idx].type != FREE) continue;
    int lit = 2*idx;
    assert (!val (2*idx));
    if (elms[idx].pos == -1) continue;
    int pos = orgs[lit], neg = orgs[lit+1];
    assert (pos >= 2);
    assert (neg >= 2);
    assert (pos != 2 || neg != 2);
  }
#endif

  if (conflict) return;

  assert (!gate);
  assert (!trash);

  if (bound != LLONG_MAX) pure ();
  cleans ();
  delorgs ();
  delfwds ();
  delfwsigs ();
  gc ();
  checkeliminated ();
  elimode = false;

  if (!schedule.elim) {
    stats.elim.rounds++;
    limit.fixed.elim = remvars ();
  }
  limit.props.elim = opts.elimprd;
  limit.props.elim *= opts.elimint;
  limit.props.elim += stats.props.srch;
  checkvarstats ();
}

void Solver::zombie (Var * v) {
  assert (elimode || blkmode || autarkmode);
  assert (!level);
  assert (v->type == FREE);
  int idx = v - vars, lit = 2*idx;
  assert (!val (lit));
  assert (!orgs[lit]);
  assert (!occs[lit].bins);
  assert (!occs[lit].large);
  assert (!orgs[lit^1]);
  assert (!occs[lit^1].bins);
  assert (!occs[lit^1].large);
  assert (!repr [lit]);
  if (puremode) {
    LOG ("explicit zombie literal " << lit);
    stats.zombies.expl++;
  } else if (blkmode) {
    LOG ("blocking zombie literal " << lit);
    stats.zombies.blkd++;
  } else if (autarkmode) {
    LOG ("autark zombie literal " << lit);
    stats.zombies.autark++;
  } else {
    assert (elimode);
    LOG ("eliminating zombie literal " << lit);
    stats.zombies.elim++;
  }
  stats.vars.zombies++;
  v->type = ZOMBIE;
#ifndef NDEBUG
  int old = trail;
#endif
  assume (lit, false);
  assert (old + 1 == trail);
  recycle (lit);
  recycle (lit^1);
  bcp ();
  assert (old + 1 == trail);
  assert (!conflict);
}

#ifdef CHECKWITHPICOSAT
void Solver::picosatcheck_consistent () {
  if (picosat_inconsistent ()) return;
  picosat_reset ();
  picosat_init ();
  picosatcheck.init++;
  for (int i = 0; i <= 1; i++)
    for (Cls * p = (i ? binary : original).tail; p; p = p->next)
      if (!p->lnd) {
	for (int * q = p->lits; *q; q++)
	  picosat_add (ulit2ilit (*q));
	picosat_add (0);
      }
  int res = picosat_sat (-1);
  picosatcheck.calls++;
  picosatcheck.blkd.all = stats.blkd.all;
  if (res == 10) return;
  die ("picosat says original clauses became inconsistent");
}

void Solver::picosatcheck_assume (const char * type, int lit) {
  if (picosat_inconsistent ()) return;
  if (picosatcheck.blkd.all < stats.blkd.all) picosatcheck_consistent ();
  int ilit = ulit2ilit (lit);
  picosat_assume (ilit);
  int res = picosat_sat (-1);
  picosatcheck.calls++;
  if (res != 10)
    die ("picosat checking of %s literal %d (%d) failed", type, lit, ilit);
  picosat_add (ilit), picosat_add (0);
  assert (!picosat_inconsistent ());
}
#endif

void Solver::pure (int lit) {
  assert (elimode || blkmode || autarkmode);
  assert (!level);
  assert (!val (lit));
  assert (!orgs[lit^1]);
  assert (!occs[lit^1].bins);
  assert (!occs[lit^1].large);
  assert (!repr [lit]);
  assert (!needtoflush ());
  Var * v = vars + (lit/2);
  assert (v->type == FREE);
  if (puremode) {
    LOG ("explicit pure literal " << lit);
    stats.pure.expl++;
  } else if (blkmode) {
    LOG ("blocking pure literal " << lit);
    stats.pure.blkd++;
  } else  if (autarkmode) {
    LOG ("autark pure literal " << lit);
    stats.pure.autark++;
  } else {
    assert (elimode);
    LOG ("eliminating pure literal " << lit);
    stats.pure.elim++;
  }
  stats.vars.pure++;
  stats.clauses.gcpure += orgs[lit];
  v->type = PURE;
#ifndef NDEBUG
  int old  = trail;
#endif
  assume (lit, false);
  assert (old + 1 == trail);
  bcp ();
  assert (old + 1 == trail);
  assert (!conflict);
#ifdef CHECKWITHPICOSAT
  if (opts.check >= 2) picosatcheck_assume ("pure", lit);
#endif
}

void Solver::autark (int lit) {
  assert (autarkmode);
  assert (!level);
  assert (!val (lit));
  assert (!vals[lit]);
  Var * v = vars + (lit/2);
  if (v->type == EQUIV) {
    assert (stats.vars.equiv);
    stats.vars.equiv--;
  } else assert (v->type == FREE);
  LOG ("autark literal " << lit);
  stats.vars.autark++;
  v->type = AUTARK;
  assume (lit, false);
  bcp ();
  assert (!conflict);
#ifdef CHECKWITHPICOSAT
  if (opts.check >= 2) picosatcheck_assume ("autark", lit);
#endif
}

void Solver::block () {
  if (!blocking ()) return;
  stats.blkd.phases++;
  blkmode = true;
  blklit = 0;
  checkclean ();
  disconnect ();
  initorgs ();
  connect (binary, true);
  connect (original, true);
  if (!bcp ()) return;
  long long bound;
  if (opts.rtc == 2 || opts.blockrtc == 2 ||
      ((opts.rtc == 1 || opts.blockrtc == 1) && !stats.blkd.rounds)) {
    bound = LLONG_MAX;
  } else {
    bound = stats.blkd.res + opts.blockint;
    for (int i = stats.blkd.phases; i <= opts.blockboost; i++)
      bound += opts.blockint;
  }
  if (schedule.block) {
    for (int lit = 2; lit <= 2*maxvar+1; lit++)
      if (schedule.block.contains (blks + lit)) touchblkd (lit);
  } else {
    for (int lit = 2; lit <= 2*maxvar+1; lit++) touchblkd (lit);
  }
  pure ();
  report (2, 'k');
  Progress progress (this, 'k');
  while (!conflict && schedule.block && stats.blkd.res <= bound) {
    Rnk * b = schedule.block.pop ();
    int lit = b - blks;
    progress.tick ();
RESTART:
    Var * v = vars + (lit/2);
    if (v->type != FREE) continue;
    int pos = orgs[lit], neg = orgs[1^lit];
    if (!pos && !neg) zombie (v);
    else if (!pos) pure (1^lit);
    else if (!neg) pure (lit);
    else if (orgs[lit] <= opts.blkmaxlen || orgs[1^lit] <= opts.blkmaxlen) {
      assert (!val (lit));
      if (orgs[1^lit] > opts.blkmaxlen) continue;
      LOG ("tryblk " << lit);
      for (int i = orgs[lit]-1; !conflict && i >= 0; i--) {
	Cls * c = orgs[lit][i];
	bool blocked = true;
	for (int j = 0; !conflict && blocked && j < orgs[lit^1]; j++) {
	  blocked = !resolve (c, lit, orgs[lit^1][j], true);
	  if (needtoflush ()) { if (!bcp ()) goto DONE; goto RESTART; }
	  if (blocked && resotfs) goto RESTART;
	}
	if (!blocked) continue;
	if (conflict) break;
	if (val (lit)) break;
	block (c, lit);
	stats.blkd.expl++;
	stats.blkd.all++;
	if (bound != LLONG_MAX) bound += opts.blockreward;
      }
    }
  }
DONE:
  report (2, 'k');
  if (conflict) return;
  assert (schedule.block || stats.clauses.irr != 1);
  if (bound != LLONG_MAX) pure ();
  cleans ();
  delorgs ();
  gc ();
  blkmode = false;

  if (!schedule.block) {
    stats.blkd.rounds++;
    limit.fixed.block = remvars ();
  }
  limit.props.block = opts.blockprd;
  limit.props.block *= opts.blockint;
  limit.props.block += stats.props.srch;
  checkvarstats ();
}

void Solver::pure () {
  assert (orgs);
  assert (blkmode || elimode);
  assert (!puremode);
  if (!bcp ()) return;
  puremode = true;
  assert (!plits);
  report (2, 'u');
  for (int idx = 1; idx <= maxvar; idx++) {
    Var * v = vars + idx;
    assert (!v->onplits);
    if (v->type != FREE) continue;
    plits.push (mem, idx);
    v->onplits = true;
  }
  while (plits) {
    int idx = plits.pop (), lit = 2*idx;
    Var * v = vars + idx;
    assert (v->type == FREE);
    assert (v->onplits);
    if (!orgs[lit] && !orgs[lit^1]) zombie (v);
    else if (!orgs[lit]) pure (1^lit);
    else if (!orgs[1^lit]) pure (lit);
    v->onplits = false;
  }
  puremode = false;
  report (2, 'u');
}

void Solver::jwh (bool orgonly) {
  memset (jwhs, 0, 2 * (maxvar + 1) * sizeof *jwhs);
  for (Cls * p = original.head; p; p = p->prev) jwh (p, orgonly);
  for (Cls * p = binary.head; p; p = p->prev) jwh (p, orgonly);
  if (orgonly) return;
  for (Cls * p = fresh.head; p; p = p->prev) jwh (p, orgonly);
  for (int glue = 0; glue <= opts.glue; glue++)
    for (Cls * p = learned[glue].head; p; p = p->prev) jwh (p, orgonly);
}

void Solver::initfresh () {
  limit.reduce.fresh = (opts.fresh * limit.reduce.learned + 99) / 100;
  report (2, 'F');
}

void Solver::initreduce () {
  int l;
  if (opts.liminitmode) l = (opts.liminitpercent*stats.clauses.orig + 99)/100;
  else l = opts.liminitconst;
  if (l > opts.liminitmax) l = opts.liminitmax;
  if (l > opts.maxlimit) l = opts.maxlimit;
  if (l < opts.minlimit) l = opts.minlimit;
  limit.enlarge.conflicts = limit.enlarge.inc = limit.enlarge.init = l;
  limit.reduce.learned = limit.reduce.init =  l;
  initfresh ();
  report (1, 'l');
}

void Solver::enlarge () {
  stats.enlarged++;
  if (opts.limincmode == 1) {
    limit.reduce.learned =
      ((100 + opts.limincpercent) * limit.reduce.learned + 99) / 100;
    limit.enlarge.inc = ((100 + opts.enlinc) * limit.enlarge.inc + 99) / 100;
    limit.enlarge.conflicts = stats.conflicts + limit.enlarge.inc;
  } else if (opts.limincmode == 0) {
    limit.enlarge.inc += opts.liminconst1;
    limit.enlarge.conflicts += limit.enlarge.inc;
    limit.reduce.learned += opts.liminconst2;
  } else {
    assert (opts.limincmode == 2);
    limit.reduce.learned += 2000;
  }
  if (limit.reduce.learned > opts.maxlimit) {
    limit.reduce.learned = opts.maxlimit;
    limit.enlarge.conflicts = INT_MAX;
  }
  initfresh ();
  report (1, '+');
}

void Solver::shrink (int old) {
  if (!opts.shrink) return;
  if (limit.reduce.learned <= opts.minlimit) return;

  int now = stats.clauses.orig;
  if (!now) return;
  old /= 100, now /= 100;
  if (old <= now) return;
  assert (old);

  int red = (100 * (old - now)) / old;
  assert (red <= 100);
  if (!red) return;

  int rl = limit.reduce.learned, drl;
  drl = (opts.shrink < 2) ? limit.reduce.init : rl;
  drl *= (opts.shrinkfactor*red+99)/100;
  rl -= drl/100;
  if (rl < opts.minlimit) rl = opts.minlimit;
  if (limit.reduce.learned == rl) return;

  limit.reduce.learned = rl;
  stats.shrunken++;

  initfresh ();
  report (1, '-');
}

void Solver::simplify () {
  stats.sw2simp ();
  int old = stats.clauses.orig, inc, rv;
  undo (0);
  assert (!simpmode);
  simpmode = true;
RESTART:
  simplified = false;
  decompose ();
  if (conflict) goto DONE;
  gc ();
  if (conflict) goto DONE;
  autark ();
  if (conflict) goto DONE;
  block ();
  if (conflict) goto DONE;
  elim ();
  if (conflict) goto DONE;
  if (simplified &&
      ((opts.rtc == 2 || opts.simprtc == 2) ||
       ((opts.rtc == 1 || opts.simprtc == 1) && !stats.simps)))
     goto RESTART;
  limit.props.simp = stats.props.srch + simprd * clauses ();
  limit.simp = stats.vars.fixed + stats.vars.merged;
  limit.fixed.simp = stats.vars.fixed;
  rv = remvars ();
  inc = opts.simpinc;
  if (rv) while (rv < 1000000) rv *= 10, inc /= 2;
  simprd *= 100 + inc; simprd += 99; simprd /= 100;
  jwh (opts.rebiasorgonly);
  report (1, 's');
  if (stats.simps) shrink (old);
  else initreduce ();
DONE:
  if (opts.print) print (opts.output);
  stats.simps++;
  stats.sw2srch ();
  assert (simpmode);
  simpmode = false;
#ifdef CHECKWITHPICOSAT
  if (opts.check) picosatcheck_consistent ();
#endif
}

void Solver::flushunits () {
  assert (units);
  undo (0);
  while (units && !conflict) unit (units.pop ());
  bcp ();
}

void Solver::initrestart () {
  int delta;
  limit.restart.conflicts = stats.conflicts;
  if (opts.luby) delta = opts.restartint * luby (limit.restart.lcnt = 1);
  else delta = limit.restart.inner = limit.restart.outer = opts.restartint;
  limit.restart.conflicts += delta;
}

void Solver::initbias () {
  jwh (opts.rebiasorgonly);
  limit.rebias.conflicts = opts.rebiasint * luby (limit.rebias.lcnt = 1);
  limit.rebias.conflicts += stats.conflicts;
}

void Solver::iteration () {
  assert (!level);
  assert (iterating);
  stats.iter++;
  initrestart ();//TODO really?
  iterating = false;
  report (2, 'i');
}

inline bool Solver::reducing () const {
  if (opts.limincmode == 2) return stats.clauses.lnd >= limit.reduce.learned;
  int learned_not_locked = stats.clauses.lnd;
  learned_not_locked -= stats.clauses.lckd;
  return 2 * learned_not_locked > 4 * limit.reduce.learned;
}

inline bool Solver::eliminating () const {
  if (!opts.elim) return false;
  if (!stats.elim.phases) return true;
  if (opts.rtc == 2 || opts.simprtc == 2 ||
      ((opts.rtc == 1 || opts.simprtc == 1) && !stats.simps)) return true;
  if (schedule.elim) return true;
  if (stats.props.srch <= limit.props.elim) return false;
  return remvars () < limit.fixed.elim;
}

inline bool Solver::blocking () const {
  if (!opts.block) return false;
  if (!stats.elim.phases) return true;
  if (opts.rtc == 2 || opts.simprtc == 2 ||
      ((opts.rtc == 1 || opts.simprtc == 1) && !stats.simps)) return true;
  if (schedule.block) return true;
  if (stats.props.srch <= limit.props.block) return false;
  return remvars () < limit.fixed.block;
}

inline bool Solver::simplifying () const {
  if (!stats.simps) return true;
  if (stats.props.srch < limit.props.simp) return false;
  return limit.simp < stats.vars.fixed + stats.vars.merged;
}

inline bool Solver::restarting () const {
  if (level < opts.restartminlevel) return 0;
  return limit.restart.conflicts <= stats.conflicts;
}

inline bool Solver::rebiasing () const {
  return limit.rebias.conflicts <= stats.conflicts;
}

inline bool Solver::probing () const {
  if (!opts.probe) return false;
  return (stats.props.srch > limit.props.probe);
}

inline bool Solver::enlarging () const {
  if (opts.limincmode == 2) return false;
  if (limit.reduce.learned >= opts.maxlimit) return false;
  return stats.conflicts >= limit.enlarge.conflicts;
}

inline bool Solver::exhausted () const {
  return stats.decisions >= limit.decisions;
}

int Solver::search () {
  stats.entered2 = seconds ();
  for (;;)
    if (!bcp ()) { if (!analyze ()) return -1; }
    else if (iterating) iteration ();
    else if (units) flushunits ();
    else if (probing ()) probe ();
    else if (simplifying ()) simplify  ();
    else if (rebiasing ()) rebias ();
    else if (restarting ()) restart ();
    else if (reducing ()) reduce ();
    else if (enlarging ()) enlarge ();
    else if (exhausted ()) return 0;
    else if (!decide ()) return 1;
  stats.srchtime += seconds () - stats.entered2;
}

void Solver::initlimit (int decision_limit) {
  memset (&limit, 0, sizeof limit);
  limit.decisions = decision_limit;
  simprd = opts.simprd;
  rng.init (opts.seed);
}

void Solver::initsearch (int decision_limit) {
  assert (opts.fixed);
  initlimit (decision_limit);
  initbias ();
  initrestart ();
#ifdef CHECKWITHPICOSAT
  int res = picosat_sat (-1);
  picosatcheck.calls++;
  if (opts.verbose) {
    printf ("c checking with picosat returns %d\n", res);
    fflush (stdout);
  }
  assert ((res == 20) == picosat_inconsistent ());
#endif
  report (1, '*');
}

int Solver::solve (int decision_limit) {
  initsearch (decision_limit);
  int res = search ();
  report (1, res < 0 ? '0' : (res ? '1' : '?'));
  if (!stats.simps && opts.print) print (opts.output);
  return res;
}

double Stats::now () {
  struct rusage u;
  if (getrusage (RUSAGE_SELF, &u))
    return 0;
  double res = u.ru_utime.tv_sec + 1e-6 * u.ru_utime.tv_usec;
  res += u.ru_stime.tv_sec + 1e-6 * u.ru_stime.tv_usec;
  return res;
}

double Stats::seconds () {
  double t = now (), delta = t - entered;
  delta = (delta < 0) ? 0 : delta;
  time += delta;
  entered = t;
  return time;
}

void Stats::sw2srch () {
  double t = seconds ();
  simptime += t - entered2;
  entered2 = t;
}

void Stats::sw2simp () {
  double t = seconds ();
  srchtime += t - entered2;
  entered2 = t;
}

Stats::Stats () {
  memset (this, 0, sizeof *this);
  entered = now ();
}

Limit::Limit () {
  memset (this, 0, sizeof *this);
  budget.fw.sub = budget.bw.sub = budget.bw.str = budget.fw.str = 10000;
}

void PrecoSat::die (const char * msg, ...) {
  va_list ap;
  fprintf (stdout, "c\nc\nc ");
  va_start (ap, msg);
  vfprintf (stdout, msg, ap);
  va_end (ap);
  fputs ("\nc\nc\n", stdout);
  fflush (stdout);
  abort ();
}

void Solver::print (const char * name) {
  bool close_file;
  FILE * file;
  if (name) {
    file = fopen (name, "w");
    if (!file) die ("can not write '%s'", name);
    close_file = true;
  } else file = stdout, close_file = false;

  assert (!level);

  int m, n;
  if (conflict) fprintf (file, "p cnf 0 1\n0\n"), m=0, n=1;
  else {
    size_t bytes = 2*(maxvar+1) * sizeof (int);
    int * map = (int*) mem.callocate (bytes);
    m = 0;
    for (int idx = 1; idx <= maxvar; idx++)
      if (vars[idx].type == FREE) map[idx] = ++m;

    assert (m == remvars ());

    n = 0;
    for (int i = 0; i <= 1; i++)
      for (Cls * p = (i ? original : binary).tail; p; p = p->next)
	if (!p->lnd && !satisfied (p)) n++;

    fprintf (file, "p cnf %d %d\n", m, n);
    fflush (file);

    for (int i = 0; i <= 1; i++)
      for (Cls * p = (i ? binary : original).tail; p; p = p->next) {
	if (p->lnd) continue;
	if (satisfied (p)) continue;
	int lit;
	for (int * q = p->lits; (lit = *q); q++) {
	  Val tmp = val (lit);
	  if (tmp < 0) continue;
	  assert (!tmp);
	  int ilit = map[lit/2];
	  if (lit&1) ilit = -ilit;
	  fprintf (file, "%d ", ilit);
	}
	fputs ("0\n", file);
      }
    mem.deallocate (map, bytes);
  }
  if (close_file) fclose (file);

  report (1, 'w');
}

void Solver::print () { print (0); }

void Cls::print (const char * prefix) const {
  fputs (prefix, stdout);
  for (const int * p = lits; *p; p++) {
    if (p != lits) fputc (' ', stdout);
    printf ("%d", *p);
  }
  fputc ('\n', stdout);
}

void Solver::dbgprint (const char * type, Anchor<Cls>& anchor) {
  size_t len = strlen (type) + strlen (prfx) + 80;
  char * str = (char*) mem.allocate (len);
  sprintf (str, "%sLOG PRINT %s clause ", prfx, type);
  for (Cls * p = anchor.tail; p; p = p->next) p->print (str);
  mem.deallocate (str, len);
}

void Solver::dbgprint (const char * type, Cls * cls) {
  size_t len = strlen (type) + strlen (prfx) + 1;
  char * str = (char*) mem.allocate (len);
  sprintf (str, "%s%s", prfx, type);
  cls->print (str);
  mem.deallocate (str, len);
}

void Solver::dbgprint () {
  dbgprint ("binary", binary);
  dbgprint ("original", original);
  for (int glue = 0; glue <= opts.glue; glue++) {
    char buffer[80];
    sprintf (buffer, "learned[%u]", glue);
    dbgprint (buffer, learned[glue]);
  }
  dbgprint ("fresh", fresh);
}

void Solver::dbgprintgate () {
  for (Cls ** p = gate.begin(); p < gate.end (); p++) {
    printf ("%sLOG gate clause", prfx);
    Cls * c = *p;
    for (const int * q = c->lits; *q; q++)
      printf (" %d", *q);
    printf ("\n");
  }
}
