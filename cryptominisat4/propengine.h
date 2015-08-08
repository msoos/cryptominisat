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
        , bool* _needToInterrupt
    );
    ~PropEngine();

    // Read state:
    //
    uint32_t nAssigns   () const;         ///<The current number of assigned literals.

    //Get state
    uint32_t    decisionLevel() const;      ///<Returns current decision level
    size_t      getTrailSize() const;       ///<Return trail size (MUST be called at decision level 0)
    bool        getStoredPolarity(const Var var);
    size_t trail_size() const {
        return trail.size();
    }
    bool propagate_occur();
    PropStats propStats;
    template<bool update_bogoprops = true>
    void enqueue(const Lit p, const PropBy from = PropBy());
    void new_decision_level();
    bool update_polarity_and_activity = true;

protected:
    virtual Lit find_good_blocked_lit(const Clause& c) const  = 0;
    void new_var(const bool bva, const Var orig_outer) override;
    void new_vars(const size_t n) override;
    void save_on_var_memory();
    template<class T> uint32_t calc_glue_using_seen2(const T& ps);
    template<class T> uint32_t calc_glue_using_seen2_upper_bit_no_zero_lev(const T& ps);

    //Stats for conflicts
    ConflCausedBy lastConflictCausedBy;

    // Solver state:
    //
    vector<Lit>         trail;            ///< Assignment stack; stores all assigments made in the order they were made.
    vector<uint32_t>    trail_lim;        ///< Separator indices for different decision levels in 'trail'.
    uint32_t            qhead;            ///< Head of queue (as index into the trail)
    Lit                 failBinLit;       ///< Used to store which watches[lit] we were looking through when conflict occured

    template<bool update_bogoprops>
    PropBy propagate_any_order();
    PropBy propagate_strict_order(
        #ifdef STATS_NEEDED
        AvgCalc<size_t>* watchListSizeTraversed = NULL
        #endif
    );
    PropBy propagateIrredBin();  ///<For debug purposes, to test binary clause removal
    PropResult prop_normal_helper(
        Clause& c
        , ClOffset offset
        , watch_subarray::iterator &j
        , const Lit p
    );
    PropResult handle_normal_prop_fail(Clause& c, ClOffset offset, PropBy& confl);
    PropResult handle_prop_tri_fail(
        watch_subarray_const::const_iterator i
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
    void     check_wrong_attach() const;
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
        watch_subarray_const::const_iterator i
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
        watch_subarray_const::const_iterator i
        , const Lit p
        , PropBy& confl
    );
    template<bool update_bogoprops = true>
    bool prop_tri_cl_any_order(
        watch_subarray_const::const_iterator i
        , const Lit lit1
        , PropBy& confl
    );

    ///Propagate >3-long clause
    PropResult prop_long_cl_strict_order(
        watch_subarray_const::const_iterator i
        , watch_subarray::iterator &j
        , const Lit p
        , PropBy& confl
    );
    template<bool update_bogoprops>
    bool prop_long_cl_any_order(
        watch_subarray_const::const_iterator i
        , watch_subarray::iterator &j
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
uint32_t PropEngine::calc_glue_using_seen2(const T& ps)
{
    uint32_t nbLevels = 0;
    for(auto lit: ps) {
        const uint32_t lev = varData[lit.var()].level;
        if (lev != 0 && !seen2[lev]) {
            nbLevels++;
            seen2[lev] = 1;
        }
    }

    for(auto lit: ps) {
        uint32_t lev = varData[lit.var()].level;
        seen2[lev] = 0;
    }
    return nbLevels;
}

template<class T> inline
uint32_t PropEngine::calc_glue_using_seen2_upper_bit_no_zero_lev(const T& ps)
{
    uint32_t nbLevels = 0;

    for(const Lit lit: ps) {
        const uint32_t lev = varData[lit.var()].level;
        if (lev == 0) {
            continue;
        }
        if (!(seen2[lev] & 2)) {
            nbLevels++;
            seen2[lev] |= 2;
        }
    }

    for(const Lit lit: ps) {
        const uint32_t lev = varData[lit.var()].level;
        seen2[lev] &= 1;
    }
    return nbLevels;
}


inline bool PropEngine::getStoredPolarity(const Var var)
{
    return varData[var].polarity;
}

} //end namespace

#endif //__PROPENGINE_H__
