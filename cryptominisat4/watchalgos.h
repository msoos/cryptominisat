#ifndef __WATCHALGOS_H__
#define __WATCHALGOS_H__

#include "watched.h"
#include "watcharray.h"

namespace CMSat {
using namespace CMSat;

/**
@brief Orders the watchlists such that the order is binary, tertiary, normal, xor
*/
struct WatchedSorter
{
    bool operator () (const Watched& x, const Watched& y);
};

inline bool  WatchedSorter::operator () (const Watched& x, const Watched& y)
{
    if (y.isBinary()) return false;
    //y is not binary, but x is, so x must be first
    if (x.isBinary()) return true;

    //from now on, none is binary.
    if (y.isTri()) return false;
    if (x.isTri()) return true;

    //from now on, none is binary or tertiary
    //don't bother sorting these
    return false;
}

//////////////////
// NORMAL Clause
//////////////////
static inline bool findWCl(watch_subarray_const ws, const ClOffset c)
{
    watch_subarray_const::const_iterator i = ws.begin(), end = ws.end();
    for (; i != end && (!i->isClause() || i->getOffset() != c); i++);
    return i != end;
}

static inline void removeWCl(watch_subarray ws, const ClOffset c)
{
    watch_subarray::iterator i = ws.begin(), end = ws.end();
    for (; i != end && (!i->isClause() || i->getOffset() != c); i++);
    assert(i != end);
    watch_subarray::iterator j = i;
    i++;
    for (; i != end; j++, i++) *j = *i;
    ws.shrink_(1);
}

//////////////////
// TRI Clause
//////////////////

static inline Watched& findWatchedOfTri(
    watch_array& wsFull
    , const Lit lit1
    , const Lit lit2
    , const Lit lit3
    , const bool red
) {
    watch_subarray ws = wsFull[lit1.toInt()];
    for (watch_subarray::iterator i = ws.begin(), end = ws.end(); i != end; i++) {
        if (i->isTri()
            && i->lit2() == lit2
            && i->lit3() == lit3
            && i->red() == red
        ) {
            return *i;
        }
    }

    assert(false);
    return *ws.begin();
}

static inline bool findWTri(
    const watch_array& wsFull
    , const Lit lit1
    , const Lit lit2
    , const Lit lit3
    , const bool red
) {
    watch_subarray_const ws = wsFull[lit1.toInt()];
    for (watch_subarray_const::const_iterator
        it = ws.begin(), end = ws.end()
        ; it != end
        ; it++
    ) {
        if (it->isTri()
            && it->lit2() == lit2
            && it->lit3() == lit3
            && it->red() == red
        ) {
            return true;
        }
    }

    return false;
}

static inline const Watched& findWatchedOfTri(
    const watch_array& wsFull
    , const Lit lit1
    , const Lit lit2
    , const Lit lit3
    , const bool red
) {
    watch_subarray_const ws = wsFull[lit1.toInt()];
    for (watch_subarray_const::const_iterator
        it = ws.begin(), end = ws.end()
        ; it != end
        ; it++
    ) {
        if (it->isTri()
            && it->lit2() == lit2
            && it->lit3() == lit3
            && it->red() == red
        ) {
            return *it;
        }
    }

    assert(false);
    return *ws.begin();
}

static inline void removeWTri(
    watch_array& wsFull
    , const Lit lit1
    , const Lit lit2
    , const Lit lit3
    , const bool red
) {
    assert(lit2 < lit3);

    watch_subarray ws = wsFull[lit1.toInt()];
    watch_subarray::iterator i = ws.begin(), end = ws.end();
    for (; i != end && (
        !i->isTri()
        || i->lit2() != lit2
        || i->lit3() != lit3
        || i->red() != red
    ); i++);

    assert(i != end);
    watch_subarray::iterator j = i;
    i++;
    for (; i != end; j++, i++) *j = *i;
    ws.shrink_(1);
}

inline void removeTriAllButOne(
    watch_array& wsFull
    , const Lit lit
    , const Lit* lits
    , const bool red
) {
    if (lit != lits[0])
        removeWTri(wsFull, lits[0], lits[1], lits[2], red);
    if (lit != lits[1])
        removeWTri(wsFull, lits[1], lits[0], lits[2], red);
    if (lit != lits[2])
        removeWTri(wsFull, lits[2], lits[0], lits[1], red);
}

//////////////////
// BINARY Clause
//////////////////

inline bool findWBin(
    const watch_array& wsFull
    , const Lit lit1
    , const Lit lit2
) {
    watch_subarray_const::const_iterator i = wsFull[lit1.toInt()].begin();
    watch_subarray_const::const_iterator end = wsFull[lit1.toInt()].end();
    for (; i != end && (!i->isBinary() || i->lit2() != lit2); i++);
    return i != end;
}

inline bool findWBin(
    const watch_array& wsFull
    , const Lit lit1
    , const Lit lit2
    , const bool red
) {
    watch_subarray_const::const_iterator i = wsFull[lit1.toInt()].begin();
    watch_subarray_const::const_iterator end = wsFull[lit1.toInt()].end();
    for (; i != end && (
        !i->isBinary()
        || i->lit2() != lit2
        || i->red() != red
    ); i++);

    return i != end;
}

inline void removeWBin(
    watch_array &wsFull
    , const Lit lit1
    , const Lit lit2
    , const bool red
) {
    watch_subarray ws = wsFull[lit1.toInt()];
    watch_subarray::iterator i = ws.begin(), end = ws.end();
    for (; i != end && (
        !i->isBinary()
        || i->lit2() != lit2
        || i->red() != red
    ); i++);

    assert(i != end);
    watch_subarray::iterator j = i;
    i++;
    for (; i != end; j++, i++) *j = *i;
    ws.shrink_(1);
}

inline Watched& findWatchedOfBin(
    watch_array& wsFull
    , const Lit lit1
    , const Lit lit2
    , const bool red
) {
    watch_subarray ws = wsFull[lit1.toInt()];
    for (watch_subarray::iterator i = ws.begin(), end = ws.end(); i != end; i++) {
        if (i->isBinary() && i->lit2() == lit2 && i->red() == red)
            return *i;
    }

    assert(false);
    return *ws.begin();
}

} //end namespace


#endif //__WATCHALGOS_H__
