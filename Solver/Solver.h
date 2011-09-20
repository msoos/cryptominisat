/*****************************************************************************
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
glucose -- Gilles Audemard, Laurent Simon (2008)
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

Original code by MiniSat and glucose authors are under an MIT licence.
Modifications for CryptoMiniSat are under LGPLv3 licence
******************************************************************************/

#ifndef SOLVER_H
#define SOLVER_H

#include <cstdio>
#include <string.h>
#include <stack>
#include <set>
using std::set;

//#define ANIMATE3D

#include "constants.h"
#include "PropByShared.h"

#include "Vec.h"
#include "Heap.h"
#include "Alg.h"
#include "MersenneTwister.h"
#include "SolverTypes.h"
#include "Clause.h"
#include "BoundedQueue.h"
#include "SolverConf.h"
#include "ImplCache.h"

//#define VERBOSE_DEBUG_FULLPROP
//#define VERBOSE_DEBUG_OTF_GATE_SHORTEN

#ifdef VERBOSE_DEBUG
#define VERBOSE_DEBUG_FULLPROP
#define ENQUEUE_DEBUG
#define DEBUG_ENQUEUE_LEVEL0
#define VERBOSE_DEBUG_OTF_GATE_SHORTEN
#endif

class ThreadControl;
class ClauseAllocator;

enum ElimedBy {ELIMED_NONE = 0, ELIMED_VARELIM = 1, ELIMED_VARREPLACER = 3, ELIMED_QUEUED_VARREPLACER = 4, ELIMED_DECOMPOSE = 5};

struct VarData
{
    VarData() :
        level(std::numeric_limits< uint32_t >::max())
        , activity(0)
        , elimed(ELIMED_NONE)
        , polarity(Polarity())
        , reason(PropBy())
    {}

    ///contains the decision level at which the assignment was made.
    uint32_t level;

    ///A heuristic measurement of the activity of a variable.
    uint32_t activity;

    ///Whether var has been eliminated (var-elim, different component, etc.)
    char elimed;

    ///The preferred polarity of each variable.
    Polarity polarity;

    PropBy reason;
};



struct PolaritySorter
{
    PolaritySorter(const vector<VarData>& _varData) :
        varData(_varData)
    {};

    const bool operator()(const Lit lit1, const Lit lit2) {
        const bool pol1 = !varData[lit1.var()].polarity.getLastVal() ^ lit1.sign();
        const bool pol2 = !varData[lit2.var()].polarity.getLastVal() ^ lit2.sign();

        //Tie 1: polarity
        if (pol1 == true && pol2 == false) return true;
        if (pol1 == false && pol2 == true) return false;
        return false;

        //Tie 2: last level
        /*assert(pol1 == pol2);
        if (pol1 == true) return varData[lit1.var()].level < varData[lit2.var()].level;
        else return varData[lit1.var()].level > varData[lit2.var()].level;*/
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
    Solver(ClauseAllocator* clAllocator, const AgilityData& agilityData);

    // Variable mode:
    //
    void setPolarity(const Var v, const bool b);      ///<Declare which polarity the decision heuristic should use for a variable.
    virtual const Var newVar(const bool dvar = true);

    // Read state:
    //
    const lbool   value      (const Var x) const;       ///<The current value of a variable.
    const lbool   value      (const Lit p) const;       ///<The current value of a literal.
    const uint32_t nAssigns   () const;         ///<The current number of assigned literals.
    const uint32_t nVars      () const;         ///<The current number of variables.

    //Get state
    const bool        okay() const; ///<FALSE means solver is in a conflicting state
    const uint32_t    getVerbosity() const;
    const uint32_t    getBinWatchSize(const bool alsoLearnt, const Lit lit) const;
    const uint32_t    decisionLevel() const;      ///<Returns current decision level
    const vector<Lit> getUnitaries() const;       ///<Return the set of unitary clauses
    const uint32_t    getNumUnitaries() const;    ///<Return the set of unitary clauses
    const uint32_t    countNumBinClauses(const bool alsoLearnt, const bool alsoNonLearnt) const;
    const size_t      getTrailSize() const;       ///<Return trail size (MUST be called at decision level 0)

protected:

    // Mode of operation:
    //

    void     cancelZeroLight(); ///<Backtrack until level 0, without updating agility, etc.

    uint64_t propagations; ///<Number of propagations made
    uint64_t bogoProps;    ///<An approximation of time

    // Solver state:
    //
    ClauseAllocator*    clAllocator;
    bool                ok;               ///< If FALSE, the constraints are already unsatisfiable. No part of the solver state may be used!
    vector<vec<Watched> > watches;        ///< 'watches[lit]' is a list of constraints watching 'lit'
    vector<ClauseData>  clauseData;       ///< Which lit is watched in clause
    vector<lbool>       assigns;          ///< The current assignments
    vector<Lit>         trail;            ///< Assignment stack; stores all assigments made in the order they were made.
    vector<uint32_t>    trail_lim;        ///< Separator indices for different decision levels in 'trail'.
    vector<PropData>    propData;         ///< Used during hyper-bin resolution & useless bin removal
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
    void       enqueue (const Lit p, const PropBy from = PropBy()); // Enqueue a literal. Assumes value of literal is undefined.
    void       enqueueComplex(const Lit p, const Lit ancestor, const bool learntStep);
    const Lit  removeWhich(Lit conflict, Lit thisAncestor, const bool thisStepLearnt);
    const bool isAncestorOf(const Lit conflict, Lit thisAncestor, const bool thisStepLearnt, const bool onlyNonLearnt, const Lit lookingForAncestor);
    void       addHyperBin(vector<Lit>& currAncestors, const Lit p); ///<Add hyper-binary clause given these ancestors
    void       addHyperBin(const Lit lit1, const Lit lit2, const Lit p); ///<Add hyper-binary clause given this tri-clause
    void       addHyperBin(const Clause& cl, const Lit p); ///<Add hyper-binary clause given this large clause

    /////////////////
    // Propagating
    ////////////////
    void         newDecisionLevel();                       ///<Begins a new decision level.
    const PropBy propagate(const bool update = true); ///<Perform unit propagation. Returns possibly conflicting clause.
    const bool   propBinaryClause(const vec<Watched>::const_iterator i, const Lit p, PropBy& confl); ///<Propagate 2-long clause
    template<bool simple> const bool propTriClause   (const vec<Watched>::const_iterator i, const Lit p, PropBy& confl); ///<Propagate 3-long clause
    template<bool simple> const bool propNormalClause(vec<Watched>::iterator &i, vec<Watched>::iterator &j, const Lit p, PropBy& confl); ///<Propagate >3-long clause

    //For hyper-bin, binary clause removal, etc.
    const PropBy      propBin(const Lit p, vec<Watched>::iterator k, set<BinaryClause>& uselessBin);
    const PropBy      propagateFull(set<BinaryClause>& uselessBin);
    bool              enqeuedSomething;         ///<Set if enqueueComplex has been called
    set<BinaryClause> needToAddBinClause;       ///<We store here hyper-binary clauses to be added at the end of propagateFull()
    const PropBy      propagateNonLearntBin();  ///<For debug purposes, to test binary clause removal

    /////////////////
    // Operations on clauses:
    /////////////////
    virtual void       attachClause        (const Clause& c, const uint16_t point1 = 0, const uint16_t point2 = 1, const bool checkAttach = true);
    virtual void       attachBinClause     (const Lit lit1, const Lit lit2, const bool learnt, const bool checkUnassignedFirst = true);
    void       detachClause        (const Clause& c);       ///<Detach clause from watch lists
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
};


///////////////////////////////////////
// Implementation of inline methods:

inline void     Solver::newDecisionLevel()
{
    trail_lim.push_back(trail.size());
    #ifdef VERBOSE_DEBUG
    std::cout << "New decision level: " << trail_lim.size() << std::endl;
    #endif
}

inline const uint32_t      Solver::decisionLevel()      const
{
    return trail_lim.size();
}

inline const lbool    Solver::value         (const Var x) const
{
    return assigns[x];
}

inline const lbool    Solver::value         (const Lit p) const
{
    return assigns[p.var()] ^ p.sign();
}

inline const uint32_t      Solver::nAssigns      ()      const
{
    return trail.size();
}

inline const uint32_t Solver::nVars         ()      const
{
    return assigns.size();
}

inline void     Solver::setPolarity   (Var v, bool b)
{
    assert(v < nVars());
    varData[v].polarity.setForced(!b);
}

inline const bool     Solver::okay          ()      const
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
inline void  Solver::enqueue(const Lit p, const PropBy from)
{
    #ifdef DEBUG_ENQUEUE_LEVEL0
    #ifndef VERBOSE_DEBUG
    if (decisionLevel() == 0)
    #endif //VERBOSE_DEBUG
    std::cout << "enqueue var " << p.var()+1
    << " to val " << !p.sign()
    << " level: " << decisionLevel()
    << " sublevel: " << trail.size()
    << " by: " << from << std::endl;
    #endif //DEBUG_ENQUEUE_LEVEL0

    #ifdef ENQUEUE_DEBUG
    assert(decisionLevel() == 0 || varData[p.var()].elimed != ELIMED_VARELIM);
    #endif

    const Var v = p.var();
    assert(value(v).isUndef());
    if (watches[p.toInt()].size() > 0) __builtin_prefetch(watches[p.toInt()].begin());

    assigns [v] = boolToLBool(!p.sign());
    #ifdef ANIMATE3D
    std::cerr << "s " << v << " " << p.sign() << std::endl;
    #endif
    varData[v].reason = from;
    trail.push_back(p);
    propagations++;

    varData[v].level = decisionLevel();
    agility.update(varData[v].polarity.getLastVal() != p.sign());
    varData[v].polarity.setLastVal(p.sign());
}

inline void Solver::enqueueComplex(const Lit p, const Lit ancestor, const bool learntStep)
{
    assert(value(p.var()) == l_Undef);
    if (watches[p.toInt()].size() > 0) __builtin_prefetch(watches[p.toInt()].begin());

    #ifdef VERBOSE_DEBUG_FULLPROP
    std::cout << "Enqueing " << p
    << " with ancestor: " << ancestor
    << " learntStep: " << learntStep << std::endl;
     #endif

    const Var var = p.var();
    assigns [var] = boolToLBool(!p.sign());//lbool(!sign(p));  // <<== abstract but not uttermost effecient
    trail.push_back(p);
    propagations++;

    propData[var].ancestor = ancestor;
    propData[var].learntStep = learntStep;
    propData[var].hyperBin = false;
    propData[var].hyperBinNotAdded = false;
    enqeuedSomething = true;
}

/**
We can try both ways: either binary clause can be removed. Try to remove one, then the other
Return which one is to be removed
*/
inline const Lit Solver::removeWhich(Lit conflict, Lit thisAncestor, bool thisStepLearnt)
{
    const PropData& data = propData[conflict.var()];

    bool onlyNonLearnt = !data.learntStep;
    Lit lookingForAncestor = data.ancestor;
    if (isAncestorOf(conflict, thisAncestor, thisStepLearnt, onlyNonLearnt, lookingForAncestor))
        return thisAncestor;

    onlyNonLearnt = !thisStepLearnt;
    thisStepLearnt = data.learntStep;
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
inline const bool Solver::isAncestorOf(const Lit conflict, Lit thisAncestor, const bool thisStepLearnt, const bool onlyNonLearnt, const Lit lookingForAncestor)
{
    #ifdef VERBOSE_DEBUG_FULLPROP
    std::cout << "isAncestorOf."
    << "conflict: " << conflict
    << " thisAncestor: " << thisAncestor
    << " thisStepLearnt: " << thisStepLearnt
    << " onlyNonLearnt: " << onlyNonLearnt
    << " lookingForAncestor: " << lookingForAncestor << std::endl;
    #endif

    //Was propagated at level 0 -- clauseCleaner will remove the clause
    if (lookingForAncestor == lit_Undef) return false;

    if (lookingForAncestor == thisAncestor) {
        #ifdef VERBOSE_DEBUG_FULLPROP
        std::cout << "Last position inside prop queue is not saved during propFull" << std::endl
        << "This may be the same exact binary clause -- not removing" << std::endl;
        #endif
        return false;
    }

    #ifdef VERBOSE_DEBUG_FULLPROP
    std::cout << "Looking for ancestor of " << conflict << " : " << lookingForAncestor << std::endl;
    std::cout << "This step is learnt? " << (thisStepLearnt ? "yes" : "false") << std::endl;
    std::cout << "Only non-learnt is acceptable?" << (onlyNonLearnt ? "yes" : "no") << std::endl;
    std::cout << "This step would be learnt?" << (thisStepLearnt ? "yes" : "no") << std::endl;
    #endif

    if (onlyNonLearnt && thisStepLearnt) {
        #ifdef VERBOSE_DEBUG_FULLPROP
        std::cout << "This step doesn't work -- is learnt but needs non-learnt" << std::endl;
        #endif
        return false;
    }

    bogoProps += 1;
    while(thisAncestor != lit_Undef) {
        #ifdef VERBOSE_DEBUG_FULLPROP
        std::cout << "Current acestor: " << thisAncestor
        << " its learnt-ness: " << propData[thisAncestor.var()].learntStep
        << std::endl;
        #endif

        if (thisAncestor == conflict) {
            #ifdef VERBOSE_DEBUG_FULLPROP
            std::cout << "We are trying to step over the conflict."
            << " That would create a loop." << std::endl;
            #endif
            return false;
        }

        if (thisAncestor == lookingForAncestor) {
            #ifdef VERBOSE_DEBUG_FULLPROP
            std::cout << "Ancestor found" << std::endl;
            #endif
            return true;
        }

        const PropData& data = propData[thisAncestor.var()];
        if ((onlyNonLearnt && data.learntStep)
            || data.hyperBinNotAdded
        ) {
            #ifdef VERBOSE_DEBUG_FULLPROP
            std::cout << "Wrong kind of hop would be needed" << std::endl;
            #endif
            return false;  //reached learnt hop (but this is non-learnt)
        }

        thisAncestor = data.ancestor;
    }

    #ifdef VERBOSE_DEBUG_FULLPROP
    std::cout << "Exit, reached root" << std::endl;
    #endif

    return false;
}

inline void Solver::addHyperBin(const Lit lit1, const Lit lit2, const Lit p)
{
    assert(value(p.var()) == l_Undef);
    #ifdef VERBOSE_DEBUG_FULLPROP
    std::cout << "Enqueing " << p
    << " with ancestor 3-long clause: " << lit1 << " , " << lit2 << " , " << p
    << std::endl;
    #endif

    assert(value(lit1) == l_False);
    assert(value(lit2) == l_False);

    vector<Lit> currAncestors;
    if (varData[lit1.var()].level != 0) currAncestors.push_back(~lit1);
    if (varData[lit2.var()].level != 0) currAncestors.push_back(~lit2);

    addHyperBin(currAncestors, p);
}

inline void Solver::addHyperBin(const Clause& cl, const Lit p)
{
    assert(value(p.var()) == l_Undef);
    #ifdef VERBOSE_DEBUG_FULLPROP
    std::cout << "Enqueing " << p
    << " with ancestor clause: " << cl
    << std::endl;
     #endif

    vector<Lit> currAncestors;
    size_t i = 0;
    for (Clause::const_iterator it = cl.begin(), end = cl.end(); it != end; it++, i++) {
        if (*it != p) {
            assert(value(*it) == l_False);
            if (varData[it->var()].level != 0) currAncestors.push_back(~*it);
        }
    }
    addHyperBin(currAncestors, p);
}

inline void Solver::addHyperBin(vector<Lit>& currAncestors, const Lit p)
{
    bogoProps += 1;
    Lit foundLit = lit_Undef;
    if (currAncestors.size() > 1) {
        //Number each node with the number of paths going through it.
        //The one that attains cl->size() the first is the lowest common ancestor
        toClear.clear();
        while(foundLit == lit_Undef) {
            for (vector<Lit>::iterator it = currAncestors.begin(), end = currAncestors.end(); it != end; it++) {
                if (*it == lit_Undef) continue;
                seen[it->toInt()]++;
                if (seen[it->toInt()] == 1)
                    toClear.push_back(*it);

                #ifdef VERBOSE_DEBUG_FULLPROP
                std::cout << "seen " << *it << " : " << seen[it->toInt()] << std::endl;
                #endif

                if (seen[it->toInt()] == currAncestors.size()) {
                    foundLit = *it;
                    break;
                }
                *it = propData[it->var()].ancestor;
            }
        }
        assert(foundLit != lit_Undef);

        //Clear node numbers we have assigned
        for(std::vector<Lit>::const_iterator it = toClear.begin(), end = toClear.end(); it != end; it++) {
            seen[it->toInt()] = 0;
        }

        #ifdef VERBOSE_DEBUG_FULLPROP
        std::cout << "Adding hyper-bin clause: " << p << " , " << foundLit << std::endl;
        #endif
        needToAddBinClause.insert(BinaryClause(p, ~foundLit, true));
    } else {
        //0-level propagation is NEVER made by propFull
        assert(currAncestors.size() > 0);

        #ifdef VERBOSE_DEBUG_FULLPROP
        std::cout << "Not adding hyper-bin because only ONE lit is not set at level 0 in long clause" << std::endl;
        #endif
        foundLit = currAncestors[0];
    }
    enqueueComplex(p, foundLit, true);
    propData[p.var()].hyperBin = true;
    propData[p.var()].hyperBinNotAdded = (currAncestors.size() == 1);
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

inline const uint32_t Solver::getNumUnitaries() const
{
    if (decisionLevel() > 0)
        return trail_lim[0];
    else
        return trail.size();
}

inline const size_t Solver::getTrailSize() const
{
    assert(decisionLevel() == 0);

    return trail.size();
}

inline bool Solver::satisfied(const BinaryClause& bin)
{
    return ((value(bin.getLit1()) == l_True) || (value(bin.getLit2()) == l_True));
}

#endif //SOLVER_H
