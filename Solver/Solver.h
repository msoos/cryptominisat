/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2011, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
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

#ifndef SOLVER_H
#define SOLVER_H

#include <cstdio>
#include <string.h>
#include <stack>
#include <set>
using std::set;

//#define ANIMATE3D

#include "constants.h"
#include "PropBy.h"

#include "Vec.h"
#include "Heap.h"
#include "Alg.h"
#include "MersenneTwister.h"
#include "SolverTypes.h"
#include "Clause.h"
#include "BoundedQueue.h"
#include "SolverConf.h"
#include "ImplCache.h"
#include "ClauseAllocator.h"

//#define VERBOSE_DEBUG_FULLPROP

#ifdef VERBOSE_DEBUG
#define VERBOSE_DEBUG_FULLPROP
#define ENQUEUE_DEBUG
#define DEBUG_ENQUEUE_LEVEL0
#endif

class ThreadControl;
class ClauseAllocator;

//Elimed by which algorithm. NONE = not eliminated
enum ElimedBy {
    ELIMED_NONE = 0
    , ELIMED_VARELIM = 1
    , ELIMED_VARREPLACER = 3
    , ELIMED_QUEUED_VARREPLACER = 4 //Only queued for removal. NOT actually removed
    , ELIMED_DECOMPOSE = 5
};

struct VarData
{
    VarData() :
        level(std::numeric_limits< uint32_t >::max())
        , reason(PropBy())
        , elimed(ELIMED_NONE)
        , polarity(false)
    {}

    ///contains the decision level at which the assignment was made.
    uint32_t level;

    //Reason this got propagated. NULL means decision/toplevel
    PropBy reason;

    ///Whether var has been eliminated (var-elim, different component, etc.)
    char elimed;

    ///The preferred polarity of each variable.
    bool polarity;
};

struct PolaritySorter
{
    PolaritySorter(const vector<VarData>& _varData) :
        varData(_varData)
    {};

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
class Solver
{
public:

    // Constructor/Destructor:
    //
    Solver(
        ClauseAllocator* clAllocator
        , const AgilityData& agilityData
        , const bool _updateGlues
    );

    // Variable mode:
    //
    virtual Var newVar(const bool dvar = true);

    // Read state:
    //
    lbool   value      (const Var x) const;       ///<The current value of a variable.
    lbool   value      (const Lit p) const;       ///<The current value of a literal.
    uint32_t nAssigns   () const;         ///<The current number of assigned literals.
    uint32_t nVars      () const;         ///<The current number of variables.

    //Get state
    bool        okay() const; ///<FALSE means solver is in a conflicting state
    uint32_t    getVerbosity() const;
    uint32_t    getBinWatchSize(const bool alsoLearnt, const Lit lit) const;
    uint32_t    decisionLevel() const;      ///<Returns current decision level
    vector<Lit> getUnitaries() const;       ///<Return the set of unitary clauses
    uint32_t    getNumUnitaries() const;    ///<Return the set of unitary clauses
    uint32_t    countNumBinClauses(const bool alsoLearnt, const bool alsoNonLearnt) const;
    size_t      getTrailSize() const;       ///<Return trail size (MUST be called at decision level 0)
    bool        getStoredPolarity(const Var var);
    void        resetClauseDataStats(size_t clause_num);

protected:

    //Non-categorised functions
    void     cancelZeroLight(); ///<Backtrack until level 0, without updating agility, etc.
    template<class T> uint16_t calcGlue(const T& ps); ///<Calculates the glue of a clause
    bool updateGlues;
    PropStats propStats;


    //Stats for conflicts
    ConflCausedBy lastConflictCausedBy;

    // Solver state:
    //
    ClauseAllocator*    clAllocator;
    bool                ok;               ///< If FALSE, the constraints are already unsatisfiable. No part of the solver state may be used!
    vector<vec<Watched> > watches;        ///< 'watches[lit]' is a list of constraints watching 'lit'
    vector<lbool>       assigns;          ///< The current assignments
    vector<Lit>         trail;            ///< Assignment stack; stores all assigments made in the order they were made.
    vector<uint32_t>    trail_lim;        ///< Separator indices for different decision levels in 'trail'.
    uint32_t            qhead;            ///< Head of queue (as index into the trail)
    Lit                 failBinLit;       ///< Used to store which watches[~lit] we were looking through when conflict occured
    vector<Lit>         assumptions;      ///< Current set of assumptions provided to solve by the user.
    vector<VarData>     varData;          ///< Stores info about variable: polarity, whether it's eliminated, etc.

    // Temporaries (to reduce allocation overhead). Each variable is prefixed by the method in which it is
    // used, exept 'seen' wich is used in several places.
    //
    vector<uint16_t>       seen;  ///<Used in multiple places. Contains 2 * numVars() elements, all zeroed out
    vector<uint16_t>       seen2; ///<To reduce temoprary data creation overhead. Used in minimiseLeartFurther()
    vector<Lit>            analyze_stack;
    vector<Lit>            toClear; ///<Temporary, used in some places

    /////////////////
    // Enqueue
    ////////////////
    void  enqueue (const Lit p, const PropBy from = PropBy()); // Enqueue a literal. Assumes value of literal is undefined.
    void  enqueueComplex(const Lit p, const Lit ancestor, const bool learntStep);
    Lit   removeWhich(Lit conflict, Lit thisAncestor, const bool thisStepLearnt);
    bool  isAncestorOf(const Lit conflict, Lit thisAncestor, const bool thisStepLearnt, const bool onlyNonLearnt, const Lit lookingForAncestor);

    vector<Lit> currAncestors;

    //Find lowest common ancestor, once 'currAncestors' has been filled
    Lit deepestCommonAcestor();

    ///Add hyper-binary clause given the ancestor filled in 'currAncestors'
    void  addHyperBin(
        const Lit p
    );

    ///Add hyper-binary clause given this tri-clause
    void  addHyperBin(
        const Lit p
        , const Lit lit1
        , const Lit lit2
    );

    ///Add hyper-binary clause given this large clause
    void  addHyperBin(
        const Lit p
        , const Clause& cl
    );

    ///Find which literal should be set when we have failed
    ///i.e. reached conflict at decision at decision lvl 1
    Lit analyzeFail(PropBy propBy);

    /////////////////
    // Propagating
    ////////////////
    void         newDecisionLevel();                       ///<Begins a new decision level.
    PropBy       propagate(); ///<Perform unit propagation. Returns possibly conflicting clause.
    bool         propBinaryClause(const vec<Watched>::const_iterator i, const Lit p, PropBy& confl); ///<Propagate 2-long clause
    template<bool simple> bool propTriClause   (const vec<Watched>::const_iterator i, const Lit p, PropBy& confl); ///<Propagate 3-long clause
    template<bool simple>
    bool propNormalClause(
        const vec<Watched>::iterator i
        , vec<Watched>::iterator &j
        , const Lit p
        , PropBy& confl
    ); ///<Propagate >3-long clause

    //For hyper-bin and transitive reduction.
    PropBy            propBin(const Lit p, vec<Watched>::const_iterator k);
    Lit               propagateFull();
    bool              enqeuedSomething;         ///<Set if enqueueComplex has been called
    set<BinaryClause> needToAddBinClause;       ///<We store here hyper-binary clauses to be added at the end of propagateFull()
    set<BinaryClause> uselessBin;
    PropBy      propagateNonLearntBin();  ///<For debug purposes, to test binary clause removal

    /////////////////
    // Operations on clauses:
    /////////////////
    virtual void       attachClause        (const Clause& c, const bool checkAttach = true);
    virtual void       attachBinClause     (const Lit lit1, const Lit lit2, const bool learnt, const bool checkUnassignedFirst = true);
    virtual void       detachModifiedClause(const Lit lit1, const Lit lit2, const Lit lit3, const uint32_t origSize, const Clause* address);

    /////////////////////////
    //Classes that must be friends, since they accomplish things on our datastructures
    /////////////////////////
    friend class CompleteDetachReatacher;
    friend class ImplCache;
    friend class ClauseAllocator;

    // Debug & etc:
    void     printAllClauses();
    void     checkLiteralCount();
    void     checkNoWrongAttach() const;
    void     printWatchList(const Lit lit) const;
    bool     satisfied(const BinaryClause& bin);

    //Var selection, activity, etc.
    AgilityData agility;
    void sortWatched();
    void updateVars(
        const vector<uint32_t>& outerToInter
        , const vector<uint32_t>& interToOuter
        , const vector<uint32_t>& interToOuter2
    );
    void updateWatch(vec<Watched>& ws, const vector<uint32_t>& outerToInter);
};


///////////////////////////////////////
// Implementation of inline methods:

inline void Solver::newDecisionLevel()
{
    trail_lim.push_back(trail.size());
    #ifdef VERBOSE_DEBUG
    cout << "New decision level: " << trail_lim.size() << endl;
    #endif
}

inline uint32_t Solver::decisionLevel() const
{
    return trail_lim.size();
}

inline lbool Solver::value (const Var x) const
{
    return assigns[x];
}

inline lbool Solver::value (const Lit p) const
{
    return assigns[p.var()] ^ p.sign();
}

inline uint32_t Solver::nAssigns() const
{
    return trail.size();
}

inline uint32_t Solver::nVars() const
{
    return assigns.size();
}

inline bool Solver::okay() const
{
    return ok;
}

/**
@brief Enqueues&sets a new fact that has been found

Call this when a fact has been found. Sets the value, enqueues it for
propagation, sets its level, sets why it was propagated, saves the polarity,
and does some logging if logging is enabled

@p p the fact to enqueue
@p from Why was it propagated (binary clause, tertiary clause, normal clause)
*/
inline void Solver::enqueue(const Lit p, const PropBy from)
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
    assert(decisionLevel() == 0 || varData[p.var()].elimed != ELIMED_VARELIM);
    #endif

    const Var v = p.var();
    assert(value(v).isUndef());
    if (watches[p.toInt()].size() > 0)
        __builtin_prefetch(watches[p.toInt()].begin());

    assigns[v] = boolToLBool(!p.sign());
    trail.push_back(p);
    propStats.propagations++;

    varData[v].reason = from;
    varData[v].level = decisionLevel();
    agility.update(varData[v].polarity != !p.sign());
    varData[v].polarity = !p.sign();

    #ifdef ANIMATE3D
    std::cerr << "s " << v << " " << p.sign() << endl;
    #endif
}

inline void Solver::enqueueComplex(
    const Lit p
    , const Lit ancestor
    , const bool learntStep
) {
    assert(value(p.var()) == l_Undef);

    //Prefetch next watchlist
    if (watches[p.toInt()].size() > 0)
        __builtin_prefetch(watches[p.toInt()].begin());

    #ifdef VERBOSE_DEBUG_FULLPROP
    cout << "Enqueing " << p
    << " with ancestor: " << ancestor
    << " learntStep: " << learntStep << endl;
     #endif

    const Var var = p.var();
    assigns[var] = boolToLBool(!p.sign());
    trail.push_back(p);
    propStats.propagations++;
    varData[var].reason = PropBy(~ancestor, learntStep, false, false);
    varData[var].level = decisionLevel();
    varData[var].polarity = !p.sign();
    enqeuedSomething = true;
}

/**
We can try both ways: either binary clause can be removed. Try to remove one, then the other
Return which one is to be removed
*/
inline Lit Solver::removeWhich(Lit conflict, Lit thisAncestor, bool thisStepLearnt)
{
    const PropBy& data = varData[conflict.var()].reason;

    bool onlyNonLearnt = !data.getLearntStep();
    Lit lookingForAncestor = data.getAncestor();
    if (isAncestorOf(conflict, thisAncestor, thisStepLearnt, onlyNonLearnt, lookingForAncestor))
        return thisAncestor;

    onlyNonLearnt = !thisStepLearnt;
    thisStepLearnt = data.getLearntStep();
    std::swap(lookingForAncestor, thisAncestor);
    if (isAncestorOf(conflict, thisAncestor, thisStepLearnt, onlyNonLearnt, lookingForAncestor))
        return thisAncestor;

    return lit_Undef;
}

/**
hop backwards from thisAncestor until:
1) we reach ancestor of 'conflict' -- at this point, we return TRUE
2) we reach an invalid point. Either root, or an invalid hop. We return FALSE.
*/
inline bool Solver::isAncestorOf(
    const Lit conflict
    , Lit thisAncestor
    , const bool thisStepLearnt
    , const bool onlyNonLearnt
    , const Lit lookingForAncestor
) {
    #ifdef VERBOSE_DEBUG_FULLPROP
    cout << "isAncestorOf."
    << "conflict: " << conflict
    << " thisAncestor: " << thisAncestor
    << " thisStepLearnt: " << thisStepLearnt
    << " onlyNonLearnt: " << onlyNonLearnt
    << " lookingForAncestor: " << lookingForAncestor << endl;
    #endif

    //Was propagated at level 0 -- clauseCleaner will remove the clause
    if (lookingForAncestor == lit_Undef) return false;

    if (lookingForAncestor == thisAncestor) {
        #ifdef VERBOSE_DEBUG_FULLPROP
        cout << "Last position inside prop queue is not saved during propFull" << endl
        << "This may be the same exact binary clause -- not removing" << endl;
        #endif
        return false;
    }

    #ifdef VERBOSE_DEBUG_FULLPROP
    cout << "Looking for ancestor of " << conflict << " : " << lookingForAncestor << endl;
    cout << "This step is learnt? " << (thisStepLearnt ? "yes" : "false") << endl;
    cout << "Only non-learnt is acceptable?" << (onlyNonLearnt ? "yes" : "no") << endl;
    cout << "This step would be learnt?" << (thisStepLearnt ? "yes" : "no") << endl;
    #endif

    if (onlyNonLearnt && thisStepLearnt) {
        #ifdef VERBOSE_DEBUG_FULLPROP
        cout << "This step doesn't work -- is learnt but needs non-learnt" << endl;
        #endif
        return false;
    }

    propStats.bogoProps += 1;
    while(thisAncestor != lit_Undef) {
        #ifdef VERBOSE_DEBUG_FULLPROP
        cout << "Current acestor: " << thisAncestor
        << " its learnt-ness: " << varData[thisAncestor.var()].reason.getLearntStep()
        << endl;
        #endif

        if (thisAncestor == conflict) {
            #ifdef VERBOSE_DEBUG_FULLPROP
            cout << "We are trying to step over the conflict."
            << " That would create a loop." << endl;
            #endif
            return false;
        }

        if (thisAncestor == lookingForAncestor) {
            #ifdef VERBOSE_DEBUG_FULLPROP
            cout << "Ancestor found" << endl;
            #endif
            return true;
        }

        const PropBy& data = varData[thisAncestor.var()].reason;
        if ((onlyNonLearnt && data.getLearntStep())
            || data.getHyperbinNotAdded()
        ) {
            #ifdef VERBOSE_DEBUG_FULLPROP
            cout << "Wrong kind of hop would be needed" << endl;
            #endif
            return false;  //reached learnt hop (but this is non-learnt)
        }

        thisAncestor = data.getAncestor();
    }

    #ifdef VERBOSE_DEBUG_FULLPROP
    cout << "Exit, reached root" << endl;
    #endif

    return false;
}

inline void Solver::addHyperBin(const Lit p, const Lit lit1, const Lit lit2)
{
    assert(value(p.var()) == l_Undef);

    #ifdef VERBOSE_DEBUG_FULLPROP
    cout << "Enqueing " << p
    << " with ancestor 3-long clause: " << p << " , " << lit1 << " , " << lit2
    << endl;
    #endif

    assert(value(lit1) == l_False);
    assert(value(lit2) == l_False);

    currAncestors.clear();;
    if (varData[lit1.var()].level != 0)
        currAncestors.push_back(~lit1);

    if (varData[lit2.var()].level != 0)
        currAncestors.push_back(~lit2);

    addHyperBin(p);
}

inline void Solver::addHyperBin(const Lit p, const Clause& cl)
{
    assert(value(p.var()) == l_Undef);

    #ifdef VERBOSE_DEBUG_FULLPROP
    cout << "Enqueing " << p
    << " with ancestor clause: " << cl
    << endl;
     #endif

    currAncestors.clear();
    size_t i = 0;
    for (Clause::const_iterator it = cl.begin(), end = cl.end(); it != end; it++, i++) {
        if (*it != p) {
            assert(value(*it) == l_False);
            if (varData[it->var()].level != 0)
                currAncestors.push_back(~*it);
        }
    }

    addHyperBin(p);
}

//Add binary clause to deepest common ancestor
inline void Solver::addHyperBin(const Lit p)
{
    propStats.bogoProps += 1;
    Lit deepestAncestor = lit_Undef;
    bool hyperBinNotAdded = true;
    if (currAncestors.size() > 1) {
        deepestAncestor = deepestCommonAcestor();

        #ifdef VERBOSE_DEBUG_FULLPROP
        cout << "Adding hyper-bin clause: " << p << " , " << ~deepestAncestor << endl;
        #endif
        needToAddBinClause.insert(BinaryClause(p, ~deepestAncestor, true));
        hyperBinNotAdded = false;
    } else {
        //0-level propagation is NEVER made by propFull
        assert(currAncestors.size() > 0);

        #ifdef VERBOSE_DEBUG_FULLPROP
        cout
        << "Not adding hyper-bin because only ONE lit is not set at"
        << "level 0 in long clause, but that long clause needs to be cleaned"
        << endl;
        #endif
        deepestAncestor = currAncestors[0];
        hyperBinNotAdded = true;
    }

    enqueueComplex(p, deepestAncestor, true);
    varData[p.var()].reason.setHyperbin(true);
    varData[p.var()].reason.setHyperbinNotAdded(hyperBinNotAdded);
}

//Analyze why did we fail at decision level 1
inline Lit Solver::analyzeFail(const PropBy propBy)
{
    //Clear out the datastructs we will be usin
    currAncestors.clear();

    //First, we set the ancestors, based on the clause
    //Each literal in the clause is an ancestor. So just 'push' them inside the
    //'currAncestors' variable
    switch(propBy.getType()) {
        case tertiary_t : {
            const Lit lit = ~propBy.getOtherLit2();
            if (varData[lit.var()].level != 0)
                currAncestors.push_back(lit);
            //intentionally falling through here
            //i.e. there is no 'break' here for a reason
        }
        case binary_t: {
            const Lit lit = ~propBy.getOtherLit();
            if (varData[lit.var()].level != 0)
                currAncestors.push_back(lit);

            if (varData[failBinLit.var()].level != 0)
                currAncestors.push_back(~failBinLit);

            break;
        }

        case clause_t: {
            const uint32_t offset = propBy.getClause();
            const Clause& cl = *clAllocator->getPointer(offset);
            for(size_t i = 0; i < cl.size(); i++) {
                if (varData[cl[i].var()].level != 0)
                    currAncestors.push_back(~cl[i]);
            }
            break;
        }

        case null_clause_t:
            assert(false);
            break;
    }

    Lit foundLit = deepestCommonAcestor();

    return foundLit;
}

inline Lit Solver::deepestCommonAcestor()
{
    //Then, we go back on each ancestor recursively, and exit on the first one
    //that unifies ALL the previous ancestors. That is the lowest common ancestor
    toClear.clear();
    Lit foundLit = lit_Undef;
    while(foundLit == lit_Undef) {
        #ifdef VERBOSE_DEBUG_FULLPROP
        cout << "LEVEL analyzeFail" << endl;
        #endif
        size_t num_lit_undef = 0;
        for (vector<Lit>::iterator
            it = currAncestors.begin(), end = currAncestors.end()
            ; it != end
            ; it++
        ) {

            //We have reached the top of the graph, the other 'threads' that
            //are still stepping back will find which literal is the lowest
            //common ancestor
            if (*it == lit_Undef) {
                #ifdef VERBOSE_DEBUG_FULLPROP
                cout << "seen lit_Undef" << endl;
                #endif
                num_lit_undef++;
                assert(num_lit_undef != currAncestors.size());
                continue;
            }

            //Increase path count
            seen[it->toInt()]++;

            //Visited counter has to be cleared later, so add it to the
            //to-be-cleared set
            if (seen[it->toInt()] == 1)
                toClear.push_back(*it);

            #ifdef VERBOSE_DEBUG_FULLPROP
            cout << "seen " << *it << " : " << seen[it->toInt()] << endl;
            #endif

            //Is this point where all the 'threads' that are stepping backwards
            //reach each other? If so, we have found what we were looking for!
            //We can exit, and return 'foundLit'
            if (seen[it->toInt()] == currAncestors.size()) {
                foundLit = *it;
                break;
            }

            //Update ancestor to its own ancestor, i.e. step up this 'thread'
            *it = varData[it->var()].reason.getAncestor();
        }
    }
    #ifdef VERBOSE_DEBUG_FULLPROP
    cout << "END" << endl;
    #endif
    assert(foundLit != lit_Undef);

    //Clear nodes we have visited
    for(std::vector<Lit>::const_iterator
        it = toClear.begin(), end = toClear.end()
        ; it != end
        ; it++
    ) {
        seen[it->toInt()] = 0;
    }

    return foundLit;
}

inline void Solver::cancelZeroLight()
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

inline uint32_t Solver::getNumUnitaries() const
{
    if (decisionLevel() > 0)
        return trail_lim[0];
    else
        return trail.size();
}

inline size_t Solver::getTrailSize() const
{
    assert(decisionLevel() == 0);

    return trail.size();
}

inline bool Solver::satisfied(const BinaryClause& bin)
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
uint16_t Solver::calcGlue(const T& ps)
{
    uint32_t nbLevels = 0;
    typename T::const_iterator l, end;

    for(l = ps.begin(), end = ps.end(); l != end; l++) {
        int32_t lev = varData[l->var()].level;
        if (!seen2[lev]) {
            nbLevels++;
            seen2[lev] = 1;
        }
    }

    for(l = ps.begin(), end = ps.end(); l != end; l++) {
        int32_t lev = varData[l->var()].level;
        seen2[lev] = 0;
    }
    return nbLevels;
}


inline bool Solver::getStoredPolarity(const Var var)
{
    return varData[var].polarity;
}
#endif //SOLVER_H
