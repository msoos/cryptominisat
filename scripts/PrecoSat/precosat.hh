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

#ifndef PrecoSat_hh_INCLUDED
#define PrecoSat_hh_INCLUDED

#include <cassert>
#include <cstdlib>
#include <climits>
#include <cstdarg>
#include <cstring>
#include <cstdio>
#include "stddef.h"

namespace PrecoSat {

void die (const char *, ...);

class Mem {
public:
  typedef void * (*NewFun)(void *, size_t);
  typedef void (*DeleteFun)(void *, void*, size_t);
  typedef void * (*ResizeFun)(void *, void *, size_t, size_t);
private:
  size_t cur, max;
  void * emgr; NewFun newfun; DeleteFun deletefun; ResizeFun resizefun;
  void operator += (size_t b) { if ((cur += b) > max) max = cur; }
  void operator -= (size_t b) { assert (cur >= b); cur -= b; }
public:
  Mem () : cur(0), max(0), emgr(0), newfun(0), deletefun(0), resizefun(0) { }
  void set (void * e, NewFun n, DeleteFun d, ResizeFun r) {
    assert (e && n && d && r);
    emgr = e; newfun = n; deletefun = d; resizefun = r;
  }
  operator size_t () const { return cur; }
  size_t getCurrent () const { return cur; }
  size_t getMax () const { return max; }
  void * allocate (size_t bytes) {
    size_t mb = bytes >> 20;
    void * res;
    res = newfun ? newfun (emgr, bytes) : malloc (bytes);
    if (!res) die ("out of memory allocating %d MB", mb);
    *this += bytes;
#ifdef MEMDBGPRECO
    fprintf (stderr, "allocate %p %d %d\n", res, bytes, cur);
#endif
    return res;
  }
  void * callocate (size_t bytes) {
    void * res = allocate (bytes);
    memset (res, 0, bytes);
    return res;
  }
  void * reallocate (void * ptr, size_t old_bytes, size_t new_bytes) {
    size_t mb = new_bytes >> 20;
    *this -= old_bytes;
#ifdef MEMDBGPRECO
    fprintf (stderr, "deallocate %p %d %d\n", ptr, old_bytes, cur);
#endif
    void * res  = resizefun ? resizefun (emgr, ptr, old_bytes, new_bytes) :
                              realloc (ptr, new_bytes);
    if (!res) die ("out of memory reallocating %ld MB", (long) mb);
    *this += new_bytes;
#ifdef MEMDBGPRECO
    fprintf (stderr, "allocate %p %d %d\n", res, new_bytes, cur);
#endif
    return res;
  }
  void * recallocate (void * ptr, size_t o, size_t n) {
    char * res = (char*) reallocate (ptr, o, n);
    if (n > o) memset (res + o, 0, n - o);
    return (void*) res;
  }
  void deallocate (void * ptr, size_t bytes) {
    *this -= bytes;
    if (deletefun) deletefun (emgr, ptr, bytes); 
    else free (ptr);
#ifdef MEMDBGPRECO
    fprintf (stderr, "deallocate %p %d %d\n", ptr, bytes, cur);
#endif
  }
};

template<class T> class Stack {
  T * a, * t, * e;
  int size () const { return e - a; }
public:
  size_t bytes () const { return size () * sizeof (T); }
private:
  void enlarge (Mem & m) {
    assert (t == e);
    size_t ob = bytes ();
    int o = size ();
    int s = o ? 2 * o : 1;
    size_t nb = s * sizeof (T);
    a = (T*) m.reallocate (a, ob, nb);
    t = a + o;
    e = a + s;
  }
public:
  Stack () : a (0), t (0), e (0) { }
  ~Stack () { free (a); }
  operator int () const { return t - a; }
  void push (Mem & m, const T & d) { if (t == e) enlarge (m); *t++ = d; }
  const T & pop () { assert (a < t); return *--t; }
  const T & top () { assert (a < t); return t[-1]; }
  T & operator [] (int i) { assert (i < *this); return a[i]; }
  void shrink (int m = 0) { assert (m <= *this); t = a + m; }
  void shrink (T * nt) { assert (a <= nt); assert (nt <= t); t = nt; }
  void release (Mem & m) { m.deallocate (a, bytes ()); a = t = e = 0; }
  const T * begin () const { return a; }
  const T * end () const { return t; }
  T * begin () { return a; }
  T * end () { return t; }
  void remove (const T & d) {
    assert (t > a);
    T prev = *--t;
    if (prev == d) return;
    T * p = t;
    for (;;) {
      assert (p > a);
      T next = *--p;
      *p = prev;
      if (next == d) return;
      prev = next;
    }
  }
  void trymove (const T & d) {
    T * p = t;
    for (;;) {
      if (p == a) return;
      if (*--p == d) break;
    }
    assert (p < t);
    while (++p < t) p[-1] = p[0];
    t--;
  }
};

template<class T, class L> class Heap {
  Stack<T *> stack;
  static int left (int p) { return 2 * p + 1; }
  static int right (int p) { return 2 * p + 2; }
  static int parent (int c) { return (c - 1) / 2; }
  static void fix (T * & ptr, ptrdiff_t diff) {
    char * charptr = (char *) ptr;
    charptr -= diff;
    ptr = (T *) charptr;
  }
public:
  void release (Mem & mem) { stack.release (mem); }
  operator int () const { return stack; }
  T * operator [] (int i) { return stack[i]; }
  bool contains (T * e) const { return e->pos >= 0; }
  void up (T * e) { 
    int epos = e->pos;
    while (epos > 0) {
      int ppos = parent (epos);
      T * p = stack [ppos];
      if (L (p, e)) break;
      stack [epos] = p, stack [ppos] = e;
      p->pos = epos, epos = ppos;
    }
    e->pos = epos;
  }
  void down (T * e) {
    assert (contains (e));
    int epos = e->pos, size = stack;
    for (;;) {
      int cpos = left (epos);
      if (cpos >= size) break;
      T * c = stack [cpos], * o;
      int opos = right (epos);
      if (L (e, c)) {
	if (opos >= size) break;
	o = stack [opos];
	if (L (e, o)) break;
	cpos = opos, c = o;
      } else if (opos < size) {
	o = stack [opos];
	if (!L (c, o)) { cpos = opos; c = o; }
      }
      stack [cpos] = e, stack [epos] = c;
      c->pos = epos, epos = cpos;
    }
    e->pos = epos;
  }
  void check () {
#ifndef NDEBUG
    for (int ppos = 0; ppos < stack; ppos++) {
      T * p = stack[ppos];
      int lpos = left (ppos);
      if (lpos < stack) {
	T * l = stack[lpos];
	assert (!L (l, p));
      } else break;
      int rpos = right (ppos);
      if (rpos < stack) {
	T * r = stack[rpos];
	assert (!L (r, p));
      } else break;
    }
#endif
  }
  void construct () {
    for (int cpos = parent (stack-1); cpos >= 0; cpos--) {
      T * e = stack[cpos]; assert (cpos >= 0); down (e);
    }
    check ();
  }
  T * max () { assert (stack); return stack[0]; }
  void push (Mem & mem, T * e) {
    assert (!contains (e));
    e->pos = stack;
    stack.push (mem, e);
    up (e);
  }
  T * pop () {
    int size = stack;
    assert (size);
    T * res = stack[0];
    assert (!res->pos);
    if (--size) {
      T * other = stack[size];
      assert (other->pos == size);
      stack[0] = other, other->pos = 0;
      stack.shrink (size);
      down (other);
    } else
      stack.shrink ();
    res->pos = -1;
    return res;
  }
  void remove (T * e) {
    int size = stack;
    assert (size > 0);
    assert (contains (e));
    int epos = e->pos;
    e->pos = -1;
    if (--size == epos) {
      stack.shrink (size);
    } else {
      T * last = stack[size];
      if (size == epos) return;
      stack[last->pos = epos] = last;
      stack.shrink (size);
      up (last);
      down (last);
    }
  }
  void fix (ptrdiff_t diff) {
    for (T ** p = stack.begin (); p < stack.end (); p++)
      fix (*p, diff);
  }
};

template<class A, class E> 
void dequeue (A & anchor, E * e) {
  if (anchor.head == e) { assert (!e->next); anchor.head = e->prev; }
  else { assert (e->next); e->next->prev = e->prev; }
  if (anchor.tail == e) { assert (!e->prev); anchor.tail = e->next; }
  else { assert (e->prev); e->prev->next = e->next; }
  e->prev = e->next = 0;
  assert (anchor.count > 0);
  anchor.count--;
}

template<class A, class E> 
bool connected (A & anchor, E * e) {
  if (e->prev) return true;
  if (e->next) return true;
  return anchor.head == e;
}

template<class A, class E>
void push (A & anchor, E * e) {
  e->next = 0;
  if (anchor.head) anchor.head->next = e;
  e->prev = anchor.head;
  anchor.head = e; 
  if (!anchor.tail) anchor.tail = e;
  anchor.count++;
}

template<class A, class E>
void enqueue (A & anchor, E * e) {
  e->prev = 0;
  if (anchor.tail) anchor.tail->prev = e;
  e->next = anchor.tail;
  anchor.tail = e;
  if (!anchor.head) anchor.head = e;
  anchor.count++;
}

template<class A, class E> 
void mtf (A & a, E * e) { dequeue (a, e); push (a, e); }

struct Cls;

template<class E> struct Anchor { 
  E * head, * tail;
  long count;
  Anchor () : head (0), tail (0), count (0) { }
};

struct Rnk { int heat; int pos; };

struct Hotter {
  Rnk * a, * b;
  Hotter (Rnk * r, Rnk * s) : a (r), b (s) { }
  operator bool () const { 
    if (a->heat > b->heat) return true;
    if (a->heat < b->heat) return false;
    return a < b;
  }
};

typedef signed char Val;

enum Vrt {
  FREE = 0,
  ZOMBIE = 1,
  PURE = 2,
  AUTARK = 3,
  ELIM = 4,
  EQUIV = 5,
  FIXED = 6,
};

typedef enum Vrt Vrt;

struct Var {
  Val phase:2, mark:2;
  bool onstack:1,removable:1,poison:1;
  bool binary:1;
  bool onplits:1;
  Vrt type : 3;
  int tlevel, dlevel, dominator;
  union { Cls * cls; int lit; } reason;
};

struct Cls {
  static const unsigned LDMAXGLUE = 4, MAXGLUE = (1<<LDMAXGLUE)-1;
  bool locked:1,lnd:1,garbage:1;
  bool binary:1,trash:1,dirty:1;
  bool gate:1,str:1,fresh:1,freed:1;
  static const int LDMAXSZ = 32 - 9 - LDMAXGLUE, MAXSZ = (1<<LDMAXSZ)-1;
  bool glued:1;
  unsigned glue:LDMAXGLUE, size:LDMAXSZ, sig;
  Cls * prev, * next;
  int lits[4];
  static size_t bytes (int n);
  size_t bytes () const;
  Cls (int l0 = 0, int l1 = 0, int l2 = 0) 
    { lits[0] = l0; lits[1] = l1; lits[2] = l2; lits[3] = 0; }
  int minlit () const;
  bool contains (int) const;
  void print (const char * prefix = "") const;
};

struct Frame {
  bool pulled : 1, contained:1;
  int tlevel : 30;
  Frame (int t) : pulled (false), contained (false), tlevel (t) { }
};

struct GateStats { int count, len; };

struct Stats {
  struct { int fixed,equiv,elim,subst,zombies,pure,autark,merged; } vars;
  struct { int orig, bin, lnd, irr, lckd, gc, gcpure, gcautark; } clauses;
  struct { int count, size, dh[6]; } autarks;
  struct { long long added; int bssrs, ssrs; } lits;
  struct { long long deleted, strong, inverse; int depth; } mins;
  int conflicts, decisions, random, enlarged, shrunken, rescored, iter;
  struct { int track, jump; long long dist, cuts; } back;
  struct { int count, skipped, maxdelta; } restart;
  struct { int count, level1, probing; } doms;
  struct { GateStats nots, ites, ands, xors; } subst;
  struct { int fw, bw, dyn, org, doms, red; } subs;
  struct { long long res; int all, impl, expl, phases, rounds; } blkd;
  struct { int fw, bw, dyn, org, asym; } str;
  struct { struct { int bin, trn, large; } dyn, stat; } otfs;
  struct { struct { long long sum, count; } slimmed, orig; } glue;
  struct { int count, maxdelta; } rebias;
  struct { int variables, phases, rounds, failed, lifted, merged; } probe;
  struct { int nontriv, fixed, merged; } sccs;
  struct { int forced, assumed, flipped; } extend;
  struct { long long resolutions; int phases, rounds; } elim;
  struct { struct { struct { long long srch,hits; } l1,l2;} fw,bw;} sigs;
  struct { int expl, elim, blkd, autark; } pure;
  struct { int expl, elim, blkd, autark; } zombies;
  struct { long long srch, simp; } props;
  long long visits, ternaryvisits, blocked, sumheight, collected;
  int simps, reductions, gcs, reports, maxdepth, printed;
  double entered, time, simptime, srchtime, entered2;
  static double now ();
  double seconds ();
  double height ();
  void sw2srch ();
  void sw2simp ();
  Stats ();
};

struct Limit {
  int decisions, simp, strength;
  struct { int conflicts, inc, init; } enlarge;
  struct { int conflicts, lcnt, inner, outer; } restart;
  struct { int conflicts, lcnt; } rebias;
  struct { int learned, fresh, init; } reduce;
  struct { int iter, reduce, probe, elim, block, simp; } fixed;
  struct { long long simp, probe, elim, block, asym; } props;
  struct { 
    struct { int sub, str; } fw;
    struct { int sub, str; } bw; 
    int red;
  } budget;
  Limit ();
};

struct Opt {
  const char * name;
  int * valptr, min, max;
  Opt (const char * n, int v, int * vp, int mi, int ma);
};

struct Opts {
  int quiet, verbose, print, terminal;
  int dominate, maxdoms;
  int plain, rtc;
  int merge;
  int otfs;
  int redsub;
  int autark,autarkdhs;
  int phase;
  int block,blockimpl,blockprd,blockint,blockrtc,blockclim,blockotfs;
  int blockreward,blockboost;
  int simprd, simpinc, simprtc;
  int cutrail;
  int probe,probeprd,probeint,probertc,probereward,probeboost;
  int decompose;
  int inverse,inveager,mtfall,mtfrev;
  int bumpuip,bumpsort,bumprev,bumpbulk,bumpturbo;
  int glue, slim, sticky;
  int elim,elimgain,elimin,elimprd,elimint,elimrtc,elimclim;
  int elimreward,elimboost,elimasym,elimasymint,elimasymreward;
  int subst, ands, xors, ites;
  int fw,dynbw,fwmaxlen,bwmaxlen,reslim,blkmaxlen;
  int heatinc;
  int restart, restartint, luby, restartinner, restartouter, restartminlevel;
  int rebias,rebiasint,rebiasorgonly;
  int minlimit, maxlimit;
  int dynred;
  int liminitmode;//0=constant,1=relative
  int liminitmax, liminitconst, liminitpercent;
  int limincmode;//0=constant,1=relative
  int liminconst1, liminconst2;
  int limincpercent;
  int enlinc;
  int shrink,shrinkfactor;
  int fresh;
  int random, spread, seed;
  int order;
  enum MinMode { NONE=0, LOCAL=1, RECUR=2, STRONG=3, STRONGER=4 };
  int minimize, maxdepth, strength;
  int check;
  int skip;
  const char * output;//for 'print'
  bool fixed;
  Stack<Opt> opts;
  Opts () : fixed (false) { }
  bool set (const char *, int);
  bool set (const char *, const char *);
  void add (Mem &, const char * name, int, int *, int, int);
  void printoptions (FILE *, const char *prfx) const;
};

struct SCC { unsigned idx, min : 31; bool done : 1; };

class RNG {
  unsigned state;
public:
  RNG () : state (0) { }
  unsigned next ();
  bool oneoutof (unsigned);
  bool choose ();
  void init (unsigned seed) { state = seed; }
};

struct Occ {
  int blit;
  Cls * cls;
  Occ (int b, Cls * c) : blit (b), cls (c) { }
  bool operator == (const Occ & o) const 
    { return cls ? o.cls == cls : o.blit == blit; }
};

struct Occs {
  Stack<int> bins;
  Stack<Occ> large;
};

typedef Stack<Cls*> Orgs;
typedef Stack<Cls*> Fwds;

class Solver {
  bool initialized;
  Val * vals;
  Var * vars;
  int * jwhs, * repr, * iirfs;
  Occs * occs;
  Orgs * orgs;
  Fwds * fwds;
  unsigned * bwsigs, * fwsigs;
  Rnk * rnks, * prbs, * elms, * blks;
  struct { Heap<Rnk,Hotter> decide, elim, block; } schedule;
  int maxvar, size, queue, queue2, level, jlevel, uip, open, resolved;
  Stack<int> trail, lits, units, levels, saved, elits, plits, flits, check;
  Stack<Frame> frames;
  Stack<Var *> seen;
  Stack<Cls *> trash, gate, strnd, fclss;
  Cls * conflict, empty, dummy;
  Anchor<Cls> original, binary, fresh, learned[Cls::MAXGLUE+1];
  int hinc, simprd, agility, spread, posgate, gatepivot, gatelen, typecount;
  int elimvar, blklit;
  char lastype;
  GateStats * gatestats;
  bool terminal, terminitialized, iterating, blkmode, extending;
  bool resotfs, reslimhit, simplified, needrescore;
  bool measure,simpmode,elimode,bkdmode,puremode,asymode,autarkmode;
  RNG rng;
  Stats stats;
  Limit limit;
  char * prfx;
  FILE * out;
  Opts opts;
  Mem mem;

#ifdef CHECKWITHPICOSAT
  struct { 
    struct { int all; } blkd; 
    int calls, init;
  } picosatcheck;
  void picosatcheck_assume (const char *, int);
  void picosatcheck_consistent ();
#endif

  Rnk * prb (const Rnk *);
  Rnk * rnk (const Var *);
  Var * var (const Rnk *);

  int & iirf (Var *, int);

  Val fixed (int lit) const;

  bool hasterm ();
  void initerm ();
  void initfwds ();
  void initfwsigs ();
  void initbwsigs ();
  void initorgs ();
  void initiirfs ();
  void clrbwsigs ();
  void rszbwsigs (int newsize);
  void rsziirfs (int newsize);
  void connectorgs ();
  void delorgs ();
  void delfwds ();
  void delfwsigs ();
  void delbwsigs ();
  void deliirfs ();
  void delclauses (Anchor<Cls> &);
  Anchor<Cls> & anchor (Cls *);

  void initprfx (const char *);
  void delprfx ();

  void resize (int);

  void initreduce ();
  void initfresh ();
  void initbias ();
  void initrestart ();
  void initlimit (int);
  void initsearch (int);

  long long clauses () const;
  int recyclelimit () const;
  bool recycling () const;
  bool reducing () const;
  bool eliminating () const;
  bool blocking () const;
  bool simplifying () const;
  bool restarting () const;
  bool rebiasing () const;
  bool probing () const;
  bool enlarging () const;
  bool exhausted () const;

  int remvars () const;

  void marklits (Cls *);
  void marklits ();
  void unmarklits ();
  void unmarklits (Cls *);
  bool fworgs ();
  void bworgs ();
  bool fwoccs ();
  void bwoccs (bool & learned);
  Cls * clause (bool learned, unsigned glue);
  unsigned litsig ();
  int redundant (Cls *);
  void recycle (Cls *);
  void recycle (int);
  void setsig (Cls *);
  void slim (Cls *);
  unsigned gluelits ();
  bool clt (int, int) const;
  void connect (Cls *);
  void connect (Anchor<Cls>&, bool orgonly = false);
  void disconnect (Cls *);
  void disconnect ();
  void collect (Cls *);
  int bwstr (unsigned sig, Cls *);
  int fwstr (unsigned sig, Cls *);
  void remove (int lit, Cls *);
  bool fwsub (unsigned sig, Cls *);
  bool bwsub (unsigned sig, Cls *);
  void assign (int l);
  void assume (int l, bool inclevel = true);
  void imply (int l, int reason);
  int dominator (int l, Cls * reason, bool &);
  void force (int l, Cls * reason);
  void unit (int l);
  bool min2 (int lit, int other, int depth);
  bool minl (int lit, Cls *, int depth);
  bool strengthen (int lit, int depth);
  bool inverse (int lit);
  bool minimize (Var *, int depth);
  int luby (int);
  friend class Progress;
  void report (int v, char ch);
  void prop2 (int lit);
  void propl (int lit);
  void flushunits ();
  bool bcp ();
  bool needtoflush () const;
  bool flush ();
  void touchpure (int lit);
  void touchelim (int lit);
  void touchblkd (int lit);
  void touch (int lit);
  void touch (Cls *);
  void rescore ();
  void bump (Cls *);
  void bump (Var *, int add);
  void bump ();
  int phase (Var *);
  bool decide ();
  void extend ();
  void increp ();
  void probe ();
  int find (int lit);
  void merge (int, int, int & merged);
  void shrink (int);
  void enlarge ();
  void checkvarstats ();
  void freecls (Cls *);
  void autark ();
  void decompose ();
  bool resolve (Cls *, int pivot, Cls *, int tryonly);
  bool andgate (int lit);
  Cls * find (int a, int b, int c);
  int itegate (int lit, int cond, int t);
  bool itegate (int lit);
  bool xorgate (int lit);
  bool hasgate (int idx);
  bool trelim (int idx);
  void elim (int idx);
  void elim ();
  void block (Cls *, int lit);
  void block (int lit);
  void block ();
  void zombie (Var*);
  void pure (int lit);
  void autark (int lit);
  void pure ();
  void cleantrash ();
  void cleangate ();
  void cleanlevels ();
  void cleanseen ();
  void jump ();
  void dump (Cls *);
  void gcls (Cls *);
  void strcls (Cls *c);
  void gc (Anchor<Cls> &, const char*);
  void gc ();
  void jwh (Cls *, bool orgonly);
  void jwh (bool orgonly);
  void reduce ();
  void simplify ();
  void iteration ();
  void restart ();
  void rebias ();
  void unassign (int lit, bool save = false);
  void undo (int newlevel, bool save = false);
  void cutrail (int);
  void pull (int lit);
  bool analyze ();

  void checkeliminated ();
  void cleans ();
  void checkclean ();

  void import ();
  int search ();
  bool satisfied (const Cls*);
  bool satisfied (Anchor<Cls> &);

  void print (const char * name);
  void print ();

  void dbgprint (const char *, Cls *);
  void dbgprint (const char *, Anchor<Cls> &);
  void dbgprint ();
  void dbgprintgate ();

public:

  Solver () : initialized (false) { }

  void set (void * e, Mem::NewFun n, Mem::DeleteFun d, Mem::ResizeFun r) {
    mem.set (e, n, d, r);
  }
  void setprfx (const char* newprfx) { delprfx (); initprfx (newprfx); }
  bool set (const char * option, int arg) { return opts.set (option, arg); }
  bool set (const char * o, const char * a) { return opts.set (o, a); }
  void set (FILE * file) { out = file; initerm (); }

  void init (int initial_maxvar = 0);
  void fxopts ();

  void add (int lit) { if (lit) lits.push (mem, lit); else import (); }
  int next () { int res = maxvar + 1; resize (res); return res; }
  int solve (int decision_limit = INT_MAX);
  int val (int l) { return vals[find (l)]; }
  double seconds () { return stats.seconds (); }
  bool satisfied ();
  void prstats ();
  void propts ();
  void reset ();
  int getMaxVar () const { return maxvar; }
  operator bool () const { return initialized; }
};

};

#endif
