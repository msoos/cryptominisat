/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
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
#include "vec.h"
#include "heap.h"
#include "alg.h"
#include "MersenneTwister.h"
#include "clause.h"
#include "boundedqueue.h"
#include "cnf.h"

namespace CMSat {

using std::set;
class Solver;
class SQLStats;

//#define VERBOSE_DEBUG_FULLPROP
//#define DEBUG_STAMPING

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

struct PolaritySorter
{
    PolaritySorter(const vector<VarData>& _varData) :
        varData(_varData)
    {}

    bool operator()(const Lit lit1, const Lit lit2) {
        const bool value1 = varData[lit1.var()].polarity ^ lit1.sign();
        const bool value2 = varData[lit2.var()].polarity ^ lit2.sign();

        //Strongly prefer TRUE value at the beginning
        if (value1 == true && value2 == false)
            return true;

        if (value1 == false && value2 == true)
            return false;

        //Tie 2: last level
        /*assert(pol1 == pol2);
        if (pol1 == true) return varData[lit1.var()].level < varData[lit2.var()].level;
        else return varData[lit1.var()].level > varData[lit2.var()].level;*/

        return false;
    }

    const vector<VarData>& varData;
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
        const SolverConf& _conf
        , bool* _needToInterrupt
    );
    ~PropEngine();

    // Read state:
    //
    uint32_t nAssigns   () const;         ///<The current number of assigned literals.

    //Get state
    uint32_t    getBinWatchSize(const bool alsoRed, const Lit lit) const;
    uint32_t    decisionLevel() const;      ///<Returns current decision level
    vector<Lit> getUnitaries() const;       ///<Return the set of unitary clauses
    uint32_t    getNumUnitaries() const;    ///<Return the set of unitary clauses
    size_t      getTrailSize() const;       ///<Return trail size (MUST be called at decision level 0)
    bool        getStoredPolarity(const Var var);
    void        resetClauseDataStats(size_t clause_num);
    size_t trail_size() const {
        return trail.size();
    }
    void cancelZeroLight();
    bool propagate_occur();
    PropStats propStats;
    void enqueue(const Lit p, const PropBy from = PropBy());
    void newDecisionLevel();

protected:
    void new_var(const bool bva, const Var orig_outer) override;
    void new_vars(const size_t n) override;
    void saveVarMem();
    template<class T> uint32_t calcGlue(const T& ps); ///<Calculates the glue of a clause

    //Stats for conflicts
    ConflCausedBy lastConflictCausedBy;

    // Solver state:
    //
    vector<Lit>         trail;            ///< Assignment stack; stores all assigments made in the order they were made.
    vector<uint32_t>    trail_lim;        ///< Separator indices for different decision levels in 'trail'.
    uint32_t            qhead;            ///< Head of queue (as index into the trail)
    Lit                 failBinLit;       ///< Used to store which watches[lit] we were looking through when conflict occured

    PropBy propagateAnyOrder();
    PropBy propagateBinFirst(
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
    virtual void detachTriClause(
        Lit lit1
        , Lit lit2
        , Lit lit3
        , bool red
        , bool allow_empty_watch = false
    );
    virtual void detachBinClause(
        Lit lit1
        , Lit lit2
        , bool red
        , bool allow_empty_watch = false
    );
    virtual void attachBinClause(
        const Lit lit1
        , const Lit lit2
        , const bool red
        , const bool checkUnassignedFirst = true
    );
    virtual void attachTriClause(
        const Lit lit1
        , const Lit lit2
        , const Lit lit3
        , const bool red
    );
    virtual void detachModifiedClause(
        const Lit lit1
        , const Lit lit2
        , const uint32_t origSize
        , const Clause* address
    );

    // Debug & etc:
    void     printAllClauses();
    void     checkNoWrongAttach() const;
    void     printWatchList(const Lit lit) const;
    bool     satisfied(const BinaryClause& bin);
    void     print_trail();

    //Var selection, activity, etc.
    AgilityData agility;
    void sortWatched();
    void updateVars(
        const vector<uint32_t>& outerToInter
        , const vector<uint32_t>& interToOuter
        , const vector<uint32_t>& interToOuter2
    );
    void updateWatch(watch_subarray ws, const vector<uint32_t>& outerToInter);

    virtual size_t memUsed() const
    {
        size_t mem = 0;
        mem += trail.capacity()*sizeof(Lit);
        mem += trail_lim.capacity()*sizeof(uint32_t);
        mem += toClear.capacity()*sizeof(Lit);
        return mem;
    }

private:
    bool propagate_tri_clause_occur(const Watched& ws);
    bool propagate_binary_clause_occur(const Watched& ws);
    bool propagate_long_clause_occur(const ClOffset offset);

    bool propBinaryClause(
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
    void propTriHelperAnyOrder(
        const Lit lit1
        , const Lit lit2
        , const Lit lit3
        #ifdef STATS_NEEDED
        , const bool red
        #endif
    );
    void lazy_hyper_bin_resolve(Lit lit1, Lit lit2);
    bool can_do_lazy_hyper_bin(Lit lit1, Lit lit2, Lit lit3);
    void update_glue(Clause& c);

    PropResult propTriClause (
        watch_subarray_const::const_iterator i
        , const Lit p
        , PropBy& confl
    );
    bool propTriClauseAnyOrder(
        watch_subarray_const::const_iterator i
        , const Lit lit1
        , PropBy& confl
    );

    ///Propagate >3-long clause
    PropResult propNormalClause(
        watch_subarray_const::const_iterator i
        , watch_subarray::iterator &j
        , const Lit p
        , PropBy& confl
    );
    bool propNormalClauseAnyOrder(
        watch_subarray_const::const_iterator i
        , watch_subarray::iterator &j
        , const Lit p
        , PropBy& confl
    );
    void lazy_hyper_bin_resolve(
        const Clause& c
        , ClOffset offset
    );
};


///////////////////////////////////////
// Implementation of inline methods:

inline void PropEngine::newDecisionLevel()
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

/**
@brief Enqueues&sets a new fact that has been found

Call this when a fact has been found. Sets the value, enqueues it for
propagation, sets its level, sets why it was propagated, saves the polarity,
and does some logging if logging is enabled

@p p the fact to enqueue
@p from Why was it propagated (binary clause, tertiary clause, normal clause)
*/
inline void PropEngine::enqueue(const Lit p, const PropBy from)
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
    assert(decisionLevel() == 0 || varData[p.var()].removed != Removed::elimed);
    #endif

    const Var v = p.var();
    assert(value(v) == l_Undef);
    if (!watches[(~p).toInt()].empty()) {
        watches.prefetch((~p).toInt());
    }

    assigns[v] = boolToLBool(!p.sign());
    #ifdef STATS_NEEDED_EXTRA
    varData[v].stats.trailLevelHist.push(trail.size());
    varData[v].stats.decLevelHist.push(decisionLevel());
    #endif
    varData[v].reason = from;
    varData[v].level = decisionLevel();

    trail.push_back(p);
    propStats.propagations++;
    propStats.bogoProps += 1;

    if (p.sign()) {
        #ifdef STATS_NEEDED_EXTRA
        varData[v].stats.negPolarSet++;
        #endif
        propStats.varSetNeg++;
    } else {
        #ifdef STATS_NEEDED_EXTRA
        varData[v].stats.posPolarSet++;
        #endif
        propStats.varSetPos++;
    }

    if (varData[v].polarity != !p.sign()) {
        agility.update(true);
        #ifdef STATS_NEEDED_EXTRA
        varData[v].stats.flippedPolarity++;
        #endif
        propStats.varFlipped++;
    } else {
        agility.update(false);
    }

    //Only update non-decision: this way, flipped decisions don't get saved
    if (from != PropBy()) {
        varData[v].polarity = !p.sign();
    }

    #ifdef ANIMATE3D
    std::cerr << "s " << v << " " << p.sign() << endl;
    #endif
}

inline void PropEngine::cancelZeroLight()
{
    assert((int)decisionLevel() > 0);

    for (int sublevel = trail.size()-1; sublevel >= (int)trail_lim[0]; sublevel--) {
        Var var = trail[sublevel].var();
        assigns[var] = l_Undef;
    }
    qhead = trail_lim[0];
    trail.resize(trail_lim[0]);
    trail_lim.clear();
}

inline uint32_t PropEngine::getNumUnitaries() const
{
    if (decisionLevel() > 0)
        return trail_lim[0];
    else
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

/**
@brief Calculates the glue of a clause

Used to calculate the Glue of a new clause, or to update the glue of an
existing clause. Only used if the glue-based activity heuristic is enabled,
i.e. if we are in GLUCOSE mode (not MiniSat mode)
*/
template<class T>
uint32_t PropEngine::calcGlue(const T& ps)
{
    uint32_t nbLevels = 0;
    typename T::const_iterator l, end;

    for(l = ps.begin(), end = ps.end(); l != end; l++) {
        uint32_t lev = varData[l->var()].level;
        if (!seen2[lev]) {
            nbLevels++;
            seen2[lev] = 1;
        }
    }

    for(l = ps.begin(), end = ps.end(); l != end; l++) {
        uint32_t lev = varData[l->var()].level;
        seen2[lev] = 0;
    }
    return nbLevels;
}


inline bool PropEngine::getStoredPolarity(const Var var)
{
    return varData[var].polarity;
}

} //end namespace

#endif //__PROPENGINE_H__
