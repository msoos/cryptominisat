/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.0 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/

#ifndef __PROPENGINE_H__
#define __PROPENGINE_H__

#include <cstdio>
#include <string.h>
#include <stack>
#include <set>

//#define ANIMATE3D

#include "constants.h"
#include "propby.h"

#include "avgcalc.h"
#include "propby.h"
#include "heap.h"
#include "alg.h"
#include "clause.h"
#include "boundedqueue.h"
#include "cnf.h"

namespace CMSat {

using std::set;
class Solver;
class SQLStats;

//#define VERBOSE_DEBUG_FULLPROP
//#define DEBUG_STAMPING
//#define VERBOSE_DEBUG

#ifdef VERBOSE_DEBUG
#define VERBOSE_DEBUG_FULLPROP
#define ENQUEUE_DEBUG
#define DEBUG_ENQUEUE_LEVEL0
#endif

class Solver;
class ClauseAllocator;
class Gaussian;

enum PropResult {
    PROP_FAIL = 0
    , PROP_NOTHING = 1
    , PROP_SOMETHING = 2
    , PROP_TODO = 3
};

/**
@brief The propagating and conflict generation class

Handles watchlists, conflict analysis, propagation, variable settings, etc.
*/
class PropEngine: public CNF
{
public:

    // Constructor/Destructor:
    //
    PropEngine(
        const SolverConf* _conf
        , std::atomic<bool>* _must_interrupt_inter
    );
    ~PropEngine();

    // Read state:
    //
    uint32_t nAssigns   () const;         ///<The current number of assigned literals.

    //Get state
    uint32_t    decisionLevel() const;      ///<Returns current decision level
    size_t      getTrailSize() const;       ///<Return trail size (MUST be called at decision level 0)
    size_t trail_size() const {
        return trail.size();
    }
    bool propagate_occur();
    PropStats propStats;
    template<bool update_bogoprops = true>
    void enqueue(const Lit p, const PropBy from = PropBy());
    void new_decision_level();

protected:
    void new_var(const bool bva, const uint32_t orig_outer) override;
    void new_vars(const size_t n) override;
    void save_on_var_memory();
    template<class T> uint32_t calc_glue(const T& ps);

    //For state saving
    void save_state(SimpleOutFile& f) const;
    void load_state(SimpleInFile& f);

    //Stats for conflicts
    ConflCausedBy lastConflictCausedBy;

    // Solver state:
    //
    vector<Lit>         trail;            ///< Assignment stack; stores all assigments made in the order they were made.
    vector<uint32_t>    trail_lim;        ///< Separator indices for different decision levels in 'trail'.
    uint32_t            qhead;            ///< Head of queue (as index into the trail)
    Lit                 failBinLit;       ///< Used to store which watches[lit] we were looking through when conflict occured

    friend class Gaussian;

    template<bool update_bogoprops>
    PropBy propagate_any_order();
    PropBy propagate_strict_order();
    /*template<bool update_bogoprops>
    bool handle_xor_cl(
        Watched*& i
        , Watched*& &j
        , const Lit p
        , PropBy& confl
    );*/
    PropBy propagateIrredBin();  ///<For debug purposes, to test binary clause removal
    PropResult prop_normal_helper(
        Clause& c
        , ClOffset offset
        , Watched*& j
        , const Lit p
    );
    PropResult handle_normal_prop_fail(Clause& c, ClOffset offset, PropBy& confl);
    PropResult handle_prop_tri_fail(
        Watched* i
        , Lit lit1
        , PropBy& confl
    );

    /////////////////
    // Operations on clauses:
    /////////////////
    virtual void attachClause(
        const Clause& c
        , const bool checkAttach = true
    );
    virtual void detach_tri_clause(
        Lit lit1
        , Lit lit2
        , Lit lit3
        , bool red
        , bool allow_empty_watch = false
    );
    virtual void detach_bin_clause(
        Lit lit1
        , Lit lit2
        , bool red
        , bool allow_empty_watch = false
    );
    virtual void attach_bin_clause(
        const Lit lit1
        , const Lit lit2
        , const bool red
        , const bool checkUnassignedFirst = true
    );
    virtual void attach_tri_clause(
        const Lit lit1
        , const Lit lit2
        , const Lit lit3
        , const bool red
    );
    virtual void detach_modified_clause(
        const Lit lit1
        , const Lit lit2
        , const uint32_t origSize
        , const Clause* address
    );

    // Debug & etc:
    void     print_all_clauses();
    void     printWatchList(const Lit lit) const;
    bool     satisfied(const BinaryClause& bin);
    void     print_trail();

    //Var selection, activity, etc.
    void sortWatched();
    void updateVars(
        const vector<uint32_t>& outerToInter
        , const vector<uint32_t>& interToOuter
        , const vector<uint32_t>& interToOuter2
    );
    void updateWatch(watch_subarray ws, const vector<uint32_t>& outerToInter);

    size_t mem_used() const
    {
        size_t mem = 0;
        mem += CNF::mem_used();
        mem += trail.capacity()*sizeof(Lit);
        mem += trail_lim.capacity()*sizeof(uint32_t);
        mem += toClear.capacity()*sizeof(Lit);
        return mem;
    }

private:
    bool propagate_tri_clause_occur(const Watched& ws);
    bool propagate_binary_clause_occur(const Watched& ws);
    bool propagate_long_clause_occur(const ClOffset offset);

    template<bool update_bogoprops = true>
    bool prop_bin_cl(
        const Watched* i
        , const Lit p
        , PropBy& confl
    ); ///<Propagate 2-long clause

    ///Propagate 3-long clause
    PropResult propTriHelperSimple(
        const Lit lit1
        , const Lit lit2
        , const Lit lit3
        , const bool red
    );
    template<bool update_bogoprops = true>
    void propTriHelperAnyOrder(
        const Lit lit1
        , const Lit lit2
        , const Lit lit3
        , const bool red
    );
    void update_glue(Clause& c);

    PropResult prop_tri_cl_strict_order (
        Watched* i
        , const Lit p
        , PropBy& confl
    );
    template<bool update_bogoprops = true>
    bool prop_tri_cl_any_order(
        Watched* i
        , const Lit lit1
        , PropBy& confl
    );

    ///Propagate >3-long clause
    PropResult prop_long_cl_strict_order(
        Watched* i
        , Watched*& j
        , const Lit p
        , PropBy& confl
    );
    template<bool update_bogoprops>
    bool prop_long_cl_any_order(
        Watched* i
        , Watched*& j
        , const Lit p
        , PropBy& confl
    );
};


///////////////////////////////////////
// Implementation of inline methods:

inline void PropEngine::new_decision_level()
{
    trail_lim.push_back(trail.size());
    #ifdef VERBOSE_DEBUG
    cout << "New decision level: " << trail_lim.size() << endl;
    #endif
}

inline uint32_t PropEngine::decisionLevel() const
{
    return trail_lim.size();
}

inline uint32_t PropEngine::nAssigns() const
{
    return trail.size();
}

inline size_t PropEngine::getTrailSize() const
{
    assert(decisionLevel() == 0);

    return trail.size();
}

inline bool PropEngine::satisfied(const BinaryClause& bin)
{
    return ((value(bin.getLit1()) == l_True)
            || (value(bin.getLit2()) == l_True));
}

template<class T> inline
uint32_t PropEngine::calc_glue(const T& ps)
{
    MYFLAG++;
    uint32_t nblevels = 0;
    for (Lit lit: ps) {
        int l = varData[lit.var()].level;
        if (l != 0 && permDiff[l] != MYFLAG) {
            permDiff[l] = MYFLAG;
            nblevels++;
            if (nblevels >= 50) {
                return nblevels;
            }
        }
    }
    return nblevels;
}

inline PropResult PropEngine::prop_normal_helper(
    Clause& c
    , ClOffset offset
    , Watched*& j
    , const Lit p
) {
    #ifdef STATS_NEEDED
    c.stats.clause_looked_at++;
    #endif

    // Make sure the false literal is data[1]:
    if (c[0] == ~p) {
        std::swap(c[0], c[1]);
    }

    assert(c[1] == ~p);

    // If 0th watch is true, then clause is already satisfied.
    if (value(c[0]) == l_True) {
        *j = Watched(offset, c[0]);
        j++;
        return PROP_NOTHING;
    }

    // Look for new watch:
    for (Lit *k = c.begin() + 2, *end2 = c.end()
        ; k != end2
        ; k++
    ) {
        //Literal is either unset or satisfied, attach to other watchlist
        if (value(*k) != l_False) {
            c[1] = *k;
            *k = ~p;
            watches[c[1]].push(Watched(offset, c[0]));
            return PROP_NOTHING;
        }
    }

    return PROP_TODO;
}


inline PropResult PropEngine::handle_normal_prop_fail(
    Clause&
    #ifdef STATS_NEEDED
    c
    #endif
    , ClOffset offset
    , PropBy& confl
) {
    confl = PropBy(offset);
    #ifdef VERBOSE_DEBUG_FULLPROP
    cout << "Conflict from ";
    for(size_t i = 0; i < c.size(); i++) {
        cout  << c[i] << " , ";
    }
    cout << endl;
    #endif //VERBOSE_DEBUG_FULLPROP

    //Update stats
    #ifdef STATS_NEEDED
    c.stats.conflicts_made++;
    c.stats.sum_of_branch_depth_conflict += decisionLevel() + 1;
    if (c.red())
        lastConflictCausedBy = ConflCausedBy::longred;
    else
        lastConflictCausedBy = ConflCausedBy::longirred;*/
    #endif

    qhead = trail.size();
    return PROP_FAIL;
}

template<bool update_bogoprops>
void PropEngine::enqueue(const Lit p, const PropBy from)
{
    #ifdef DEBUG_ENQUEUE_LEVEL0
    #ifndef VERBOSE_DEBUG
    if (decisionLevel() == 0)
    #endif //VERBOSE_DEBUG
    cout << "enqueue var " << p.var()+1
    << " to val " << !p.sign()
    << " level: " << decisionLevel()
    << " sublevel: " << trail.size()
    << " by: " << from << endl;
    #endif //DEBUG_ENQUEUE_LEVEL0

    #ifdef ENQUEUE_DEBUG
    assert(trail.size() <= nVarsOuter());
    assert(varData[p.var()].removed == Removed::none);
    #endif

    const uint32_t v = p.var();
    assert(value(v) == l_Undef);
    /*if (!watches[~p].empty()) {
        watches.prefetch((~p).toInt());
    }*/

    const bool sign = p.sign();
    assigns[v] = boolToLBool(!sign);
    varData[v].reason = from;
    varData[v].level = decisionLevel();
    varData[v].polarity = !sign;
    trail.push_back(p);

    if (update_bogoprops) {
        propStats.bogoProps += 1;
    }

    #ifdef STATS_NEEDED
    if (sign) {
        propStats.varSetNeg++;
    } else {
        propStats.varSetPos++;
    }
    #endif

    #ifdef ANIMATE3D
    std::cerr << "s " << v << " " << p.sign() << endl;
    #endif
}

} //end namespace

#endif //__PROPENGINE_H__
