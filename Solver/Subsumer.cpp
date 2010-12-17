/*****************************************************************************
SatELite -- (C) Niklas Een, Niklas Sorensson, 2004
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

Original code by SatELite authors are under an MIT licence.
Modifications for CryptoMiniSat are under GPLv3 licence.
******************************************************************************/

#include "Solver.h"
#include "Subsumer.h"
#include "ClauseCleaner.h"
#include "time_mem.h"
#include "assert.h"
#include <iomanip>
#include <cmath>
#include <algorithm>
#include "VarReplacer.h"
#include "XorFinder.h"
#include "CompleteDetachReattacher.h"
#include "OnlyNonLearntBins.h"
#include "UselessBinRemover.h"
#include "DataSync.h"
#include "BothCache.h"

#ifdef _MSC_VER
#define __builtin_prefetch(a,b,c)
#endif //_MSC_VER

//#define VERBOSE_DEBUG
#ifdef VERBOSE_DEBUG
#define BIT_MORE_VERBOSITY
#endif

//#define BIT_MORE_VERBOSITY
//#define TOUCH_LESS

#ifdef VERBOSE_DEBUG
using std::cout;
using std::endl;
#endif //VERBOSE_DEBUG

Subsumer::Subsumer(Solver& s):
    solver(s)
    , totalTime(0.0)
    , numElimed(0)
    , numCalls(1)
    , alsoLearnt(false)
{
};

/**
@brief Extends the model to include eliminated variables

Adds the clauses to the parameter solver2, and then relies on the
caller to call solver2.solve().

@p solver2 The external solver the variables' clauses are added to
*/
void Subsumer::extendModel(Solver& solver2)
{
    #ifdef VERBOSE_DEBUG
    std::cout << "Subsumer::extendModel(Solver& solver2) called" << std::endl;
    #endif

    assert(checkElimedUnassigned());
    vec<Lit> tmp;
    typedef map<Var, vector<vector<Lit> > > elimType;
    for (elimType::iterator it = elimedOutVar.begin(), end = elimedOutVar.end(); it != end; it++) {
        #ifndef NDEBUG
        Var var = it->first;
        #ifdef VERBOSE_DEBUG
        std::cout << "Reinserting elimed var: " << var+1 << std::endl;
        #endif
        assert(!solver.decision_var[var]);
        assert(solver.assigns[var] == l_Undef);
        assert(!solver.order_heap.inHeap(var));
        #endif

        for (vector<vector<Lit> >::const_iterator it2 = it->second.begin(), end2 = it->second.end(); it2 != end2; it2++) {
            tmp.clear();
            tmp.growTo(it2->size());
            std::copy(it2->begin(), it2->end(), tmp.getData());

            #ifdef VERBOSE_DEBUG
            std::cout << "Reinserting elimed clause: " << c << std::endl;;
            #endif

            solver2.addClause(tmp);
            assert(solver2.ok);
        }
    }

    typedef map<Var, vector<std::pair<Lit, Lit> > > elimType2;
    for (elimType2::iterator it = elimedOutVarBin.begin(), end = elimedOutVarBin.end(); it != end; it++) {
        #ifndef NDEBUG
        Var var = it->first;
        #ifdef VERBOSE_DEBUG
        std::cout << "Reinserting elimed var: " << var+1 << std::endl;
        #endif
        assert(!solver.decision_var[var]);
        assert(solver.assigns[var] == l_Undef);
        assert(!solver.order_heap.inHeap(var));
        #endif

        for (vector<std::pair<Lit, Lit> >::iterator it2 = it->second.begin(), end2 = it->second.end(); it2 != end2; it2++) {
            tmp.clear();
            tmp.growTo(2);
            tmp[0] = it2->first;
            tmp[1] = it2->second;

            #ifdef VERBOSE_DEBUG
            std::cout << "Reinserting bin clause: " << it2->first << " , " << it2->second << std::endl;
            #endif

            solver2.addClause(tmp);
            assert(solver2.ok);
        }
    }
}

/**
@brief Adds to the solver the clauses representing variable var

This function is useful if a variable was eliminated, but now needs to be
added back again.

@p var The variable to be added back again
*/
const bool Subsumer::unEliminate(const Var var)
{
    assert(var_elimed[var]);
    vec<Lit> tmp;
    typedef map<Var, vector<vector<Lit> > > elimType;
    typedef map<Var, vector<std::pair<Lit, Lit> > > elimType2;
    elimType::iterator it = elimedOutVar.find(var);
    elimType2::iterator it2 = elimedOutVarBin.find(var);

    //it MUST have been decision var, otherwise we would
    //never have removed it
    solver.setDecisionVar(var, true);
    var_elimed[var] = false;
    numElimed--;
    #ifdef VERBOSE_DEBUG
    std::cout << "Reinserting normal (non-xor) elimed var: " << var+1 << std::endl;
    #endif

    //If the variable was removed because of
    //pure literal removal (by blocked clause
    //elimination, there are no clauses to re-insert
    if (it == elimedOutVar.end() && it2 == elimedOutVarBin.end()) return solver.ok;

    FILE* backup_libraryCNFfile = solver.libraryCNFFile;
    solver.libraryCNFFile = NULL;

    if (it == elimedOutVar.end()) goto next;
    for (vector<vector<Lit> >::iterator itt = it->second.begin(), end2 = it->second.end(); itt != end2; itt++) {
        #ifdef VERBOSE_DEBUG
        std::cout << "Reinserting elimed clause: " << *itt << std::endl;;
        #endif
        tmp.clear();
        tmp.growTo(itt->size());
        std::copy(itt->begin(), itt->end(), tmp.getData());
        solver.addClause(tmp);
    }
    elimedOutVar.erase(it);

    next:
    if (it2 == elimedOutVarBin.end()) goto next2;
    for (vector<std::pair<Lit, Lit> >::iterator itt = it2->second.begin(), end2 = it2->second.end(); itt != end2; itt++) {
        tmp.clear();
        tmp.growTo(2);
        tmp[0] = itt->first;
        tmp[1] = itt->second;
        #ifdef VERBOSE_DEBUG
        std::cout << "Reinserting bin clause: " << itt->first << " , " << itt->second << std::endl;
        #endif
        solver.addClause(tmp);
    }
    elimedOutVarBin.erase(it2);

    next2:
    solver.libraryCNFFile = backup_libraryCNFfile;

    return solver.ok;
}

/**
@brief Backward-subsumption using given clause: helper function

Checks all clauses in the occurrence lists if they are subsumed by ps or not.

The input clause can be learnt. In that case, if it subsumes non-learnt clauses,
it will become non-learnt.

Handles it well if the subsumed clause has a higher activity than the subsuming
clause (will take the max() of the two)

@p ps The clause to use

*/
void Subsumer::subsume0(Clause& ps)
{
    #ifdef VERBOSE_DEBUG
    cout << "subsume0-ing with clause: ";
    ps.plainPrint();
    #endif
    subsume0Happened ret = subsume0Orig(ps, ps.getAbst());

    if (ps.learnt()) {
        if (!ret.subsumedNonLearnt) {
            if (ps.getGlue() > ret.glue)
                ps.setGlue(ret.glue);
            if (ps.getMiniSatAct() < ret.act)
                ps.setMiniSatAct(ret.act);
        } else {
            solver.nbCompensateSubsumer++;
            ps.makeNonLearnt();
        }
    }
}

/**
@brief Backward-subsumption using given clause

@note Use helper function

@param ps The clause to use to backward-subsume
@param[in] abs The abstraction of the clause
@return Subsumed anything? If so, what was the max activity? Was it non-learnt?
*/
template<class T>
Subsumer::subsume0Happened Subsumer::subsume0Orig(const T& ps, uint32_t abs)
{
    subsume0Happened ret;
    ret.subsumedNonLearnt = false;
    ret.glue = std::numeric_limits<uint32_t>::max();
    ret.act = std::numeric_limits< float >::min();

    vec<ClauseSimp> subs;
    findSubsumed(ps, abs, subs);
    for (uint32_t i = 0; i < subs.size(); i++){
        #ifdef VERBOSE_DEBUG
        cout << "-> subsume0 removing:";
        subs[i].clause->plainPrint();
        #endif

        Clause* tmp = subs[i].clause;
        if (tmp->learnt()) {
            ret.glue = std::min(ret.glue, tmp->getGlue());
            ret.act = std::max(ret.act, tmp->getMiniSatAct());
        } else {
            ret.subsumedNonLearnt = true;
        }
        unlinkClause(subs[i]);
    }

    return ret;
}

/**
@brief Backward subsumption and self-subsuming resolution

Performs backward subsumption AND
self-subsuming resolution using backward-subsumption

@param[in] ps The clause to use for backw-subsumption and self-subs. resolution
*/
void Subsumer::subsume1(Clause& ps)
{
    vec<ClauseSimp>    subs;
    vec<Lit>           subsLits;
    #ifdef VERBOSE_DEBUG
    cout << "subsume1-ing with clause:";
    ps.plainPrint();
    #endif

    findSubsumed1(ps, ps.getAbst(), subs, subsLits);
    for (uint32_t j = 0; j < subs.size(); j++) {
        if (subs[j].clause == NULL) continue;
        ClauseSimp c = subs[j];
        if (subsLits[j] == lit_Undef) {
            if (ps.learnt()) {
                if (c.clause->learnt()) ps.takeMaxOfStats(*c.clause);
                else {
                    solver.nbCompensateSubsumer++;
                    ps.makeNonLearnt();
                }
            }
            unlinkClause(c);
        } else {
            strenghten(c, subsLits[j]);
            if (!solver.ok) return;
        }
    }
}

const bool Subsumer::subsume1(vec<Lit>& ps, const bool wasLearnt)
{
    vec<ClauseSimp>    subs;
    vec<Lit>           subsLits;
    bool toMakeNonLearnt = false;

    findSubsumed1(ps, calcAbstraction(ps), subs, subsLits);
    for (uint32_t j = 0; j < subs.size(); j++) {
        if (subs[j].clause == NULL) continue;
        ClauseSimp c = subs[j];
        if (subsLits[j] == lit_Undef) {
            if (wasLearnt && !c.clause->learnt()) toMakeNonLearnt = true;
            unlinkClause(c);
        } else {
            strenghten(c, subsLits[j]);
            if (!solver.ok) return false;
        }
    }

    return toMakeNonLearnt;
}

/**
@brief Removes&free-s a clause from everywhere

Removes clause from occurence lists, from Subsumer::clauses, and iter_sets.

If clause is to be removed because the variable in it is eliminated, the clause
is saved in elimedOutVar[] before it is fully removed.

@param[in] c The clause to remove
@param[in] elim If the clause is removed because of variable elmination, this
parameter is different from var_Undef.
*/
void Subsumer::unlinkClause(ClauseSimp c, const Var elim)
{
    Clause& cl = *c.clause;

    for (uint32_t i = 0; i < cl.size(); i++) {
        if (elim != var_Undef) numMaxElim -= occur[cl[i].toInt()].size()/2;
        else {
            numMaxSubsume0 -= occur[cl[i].toInt()].size()/2;
            numMaxSubsume1 -= occur[cl[i].toInt()].size()/2;
        }
        maybeRemove(occur[cl[i].toInt()], &cl);
        #ifndef TOUCH_LESS
        touch(cl[i], cl.learnt());
        #endif
    }

    // Remove from iterator vectors/sets:
    for (uint32_t i = 0; i < iter_sets.size(); i++) {
        CSet& cs = *iter_sets[i];
        cs.exclude(c);
    }

    // Remove clause from clause touched set:
    cl_touched.exclude(c);

    //Compensate if removing learnt
    if (cl.learnt()) solver.nbCompensateSubsumer++;

    if (elim != var_Undef) {
        assert(!cl.learnt());
        #ifdef VERBOSE_DEBUG
        std::cout << "Eliminating non-bin clause: " << *c.clause << std::endl;
        std::cout << "On variable: " << elim+1 << std::endl;
        #endif //VERBOSE_DEBUG
        vector<Lit> lits(c.clause->size());
        std::copy(c.clause->getData(), c.clause->getDataEnd(), lits.begin());
        elimedOutVar[elim].push_back(lits);
    } else {
        clauses_subsumed++;
    }
    solver.clauseAllocator.clauseFree(c.clause);

    clauses[c.index].clause = NULL;
}


/**
@brief Cleans clause from false literals

This does NOT re-implement the feature of  ClauseCleaner because
here we need to remove the literals from the occurrence lists as well. Further-
more, we need propagate if needed, which is never assumed to be a need in
ClauseCleaner since there the clauses are always attached to the watch-
lists.

@param ps Clause to be cleaned
*/
const bool Subsumer::cleanClause(Clause& ps)
{
    bool retval = false;

    Lit *i = ps.getData();
    Lit *j = i;
    for (Lit *end = ps.getDataEnd(); i != end; i++) {
        lbool val = solver.value(*i);
        if (val == l_Undef) {
            *j++ = *i;
            continue;
        }
        if (val == l_False) {
            removeW(occur[i->toInt()], &ps);
            numMaxSubsume1 -= occur[i->toInt()].size()/2;
            #ifndef TOUCH_LESS
            touch(*i, ps.learnt());
            #endif
            continue;
        }
        if (val == l_True) {
            *j++ = *i;
            retval = true;
            continue;
        }
        assert(false);
    }
    ps.shrink(i-j);

    return retval;
}

const bool Subsumer::cleanClause(vec<Lit>& ps) const
{
    bool retval = false;

    Lit *i = ps.getData();
    Lit *j = i;
    for (Lit *end = ps.getDataEnd(); i != end; i++) {
        lbool val = solver.value(*i);
        if (val == l_Undef) {
            *j++ = *i;
            continue;
        }
        if (val == l_False)
            continue;
        if (val == l_True) {
            *j++ = *i;
            retval = true;
            continue;
        }
        assert(false);
    }
    ps.shrink(i-j);

    return retval;
}

/**
@brief Removes a literal from a clause

May return with solver.ok being FALSE, and may set&propagate variable values.

@param c Clause to be cleaned of the literal
@param[in] toRemoveLit The literal to be removed from the clause
*/
void Subsumer::strenghten(ClauseSimp& c, const Lit toRemoveLit)
{
    #ifdef VERBOSE_DEBUG
    cout << "-> Strenghtening clause :";
    c.clause->plainPrint();
    cout << " with lit: " << toRemoveLit << std::endl;
    #endif

    literals_removed++;
    c.clause->strengthen(toRemoveLit);
    removeW(occur[toRemoveLit.toInt()], c.clause);
    numMaxSubsume1 -= occur[toRemoveLit.toInt()].size()/2;
    #ifndef TOUCH_LESS
    touch(toRemoveLit, c.clause->learnt());
    #endif
    if (cleanClause(*c.clause)) {
        unlinkClause(c);
        c.clause = NULL;
        return;
    }

    switch (c.clause->size()) {
        case 0:
            #ifdef VERBOSE_DEBUG
            std::cout << "Strenghtened clause to 0-size -> UNSAT"<< std::endl;
            #endif //VERBOSE_DEBUG
            solver.ok = false;
            break;
        case 1: {
            handleSize1Clause((*c.clause)[0]);
            unlinkClause(c);
            c.clause = NULL;
            break;
        }
        case 2: {
            solver.attachBinClause((*c.clause)[0], (*c.clause)[1], (*c.clause).learnt());
            solver.numNewBin++;
            solver.dataSync->signalNewBinClause(*c.clause);
            clBinTouched.push_back(NewBinaryClause((*c.clause)[0], (*c.clause)[1], (*c.clause).learnt()));
            unlinkClause(c);
            c.clause = NULL;
            break;
        }
        default:
            cl_touched.add(c);
    }
}

const bool Subsumer::handleClBinTouched()
{
    assert(solver.ok);
    uint32_t clauses_subsumed_before = clauses_subsumed;
    uint32_t literals_removed_before = literals_removed;
    uint32_t clBinSize = 0;

    vec<Lit> lits(2);
    for (list<NewBinaryClause>::const_iterator it = clBinTouched.begin(); it != clBinTouched.end(); it++) {
        lits[0] = it->lit1;
        lits[1] = it->lit2;
        const bool learnt = it->learnt;

        if (subsume1(lits, learnt)) {
            //if we can't find it, that must be because it has been made non-learnt
            //note: if it has been removed through elimination, it must't
            //be able to subsume any non-learnt clauses, so we never enter here
            if (findWBin(solver.watches, lits[0], lits[1], true)) {
                findWatchedOfBin(solver.watches, lits[0], lits[1], learnt).setLearnt(false);
                findWatchedOfBin(solver.watches, lits[1], lits[0], learnt).setLearnt(false);
            }
        }
        if (!solver.ok) return false;
        clBinSize++;
    }
    clBinTouched.clear();

    if (solver.conf.verbosity >= 3) {
        std::cout << "c subs-w-newbins " << clauses_subsumed - clauses_subsumed_before
        << " lits rem " << literals_removed - literals_removed_before
        << " went through: " << clBinSize << std::endl;
    }

    return true;
}

/**
@brief Handles if a clause became 1-long (unitary)

Either sets&propagates the value, ignores the value (if already set),
or sets solver.ok = FALSE

@param[in] lit The single literal the clause has
*/
inline void Subsumer::handleSize1Clause(const Lit lit)
{
    if (solver.value(lit) == l_False) {
        solver.ok = false;
    } else if (solver.value(lit) == l_Undef) {
        solver.uncheckedEnqueue(lit);
        solver.ok = solver.propagate().isNULL();
    } else {
        assert(solver.value(lit) == l_True);
    }
}

/**
@brief Executes subsume1() recursively on all clauses

This function requires cl_touched to have been set. Then, it manages cl_touched.
The clauses are called to perform subsume1() or subsume0() when appropriate, and
when there is enough numMaxSubume1 and numMaxSubume0 is available.
*/
const bool Subsumer::subsume0AndSubsume1()
{
    CSet s0, s1;

    //uint32_t clTouchedTodo = cl_touched.nElems();
    uint32_t clTouchedTodo = 100000;
    if (addedClauseLits > 1500000) clTouchedTodo /= 2;
    if (addedClauseLits > 3000000) clTouchedTodo /= 2;
    if (addedClauseLits > 10000000) clTouchedTodo /= 2;
    if (alsoLearnt) {
        clTouchedTodo /= 2;
        /*clTouchedTodo = std::max(clTouchedTodo, (uint32_t)20000);
        clTouchedTodo = std::min(clTouchedTodo, (uint32_t)5000);*/
    } else {
        /*clTouchedTodo = std::max(clTouchedTodo, (uint32_t)20000);
        clTouchedTodo = std::min(clTouchedTodo, (uint32_t)5000);*/
    }

    if (!solver.conf.doSubsume1) clTouchedTodo = 0;

    registerIteration(s0);
    registerIteration(s1);
    vec<ClauseSimp> remClTouched;

    // Fixed-point for 1-subsumption:
    #ifdef BIT_MORE_VERBOSITY
    std::cout << "c  cl_touched.nElems() = " << cl_touched.nElems() << std::endl;
    #endif
    do {
        #ifdef VERBOSE_DEBUG
        std::cout << "c  -- subsume0AndSubsume1() round --" << std::endl;
        std::cout << "c  cl_touched.nElems() = " << cl_touched.nElems() << std::endl;
        std::cout << "c  clauses.size() = " << clauses.size() << std::endl;
        std::cout << "c  numMaxSubsume0:" << numMaxSubsume0 << std::endl;
        std::cout << "c  numMaxSubsume1:" << numMaxSubsume1 << std::endl;
        std::cout << "c  numMaxElim:" << numMaxElim << std::endl;
        #endif //VERBOSE_DEBUG

        uint32_t s1Added = 0;
        vec<Lit> setNeg;
        for (CSet::iterator it = cl_touched.begin(), end = cl_touched.end(); it != end; ++it) {

            if (it->clause == NULL) continue;
            Clause& cl = *it->clause;

            bool tooMuch = s1Added >= clTouchedTodo;
            if (numMaxSubsume1 <= 0) tooMuch = false;
            bool addedAnyway = true;

            uint32_t smallestPosSize = std::numeric_limits<uint32_t>::max();
            Lit smallestPos = lit_Undef;

            if (!tooMuch) s1Added += s1.add(*it);
            else if (!s1.alreadyIn(*it)) addedAnyway = false;
            s0.add(*it);

            for (uint32_t j = 0; j < cl.size() && addedAnyway; j++) {
                if (ol_seenPos[cl[j].toInt()] || smallestPos == lit_Error) {
                    smallestPos = lit_Error;
                    goto next;
                }
                if (occur[cl[j].toInt()].size() < smallestPosSize) {
                    smallestPos = cl[j];
                    smallestPosSize = occur[cl[j].toInt()].size();
                }

                next:
                if (ol_seenNeg[(~cl[j]).toInt()]) continue;
                vec<ClauseSimp>& n_occs = occur[(~cl[j]).toInt()];
                for (uint32_t k = 0; k < n_occs.size(); k++) {
                    if (tooMuch && !s1.alreadyIn(n_occs[k])) {
                        addedAnyway = false;
                        goto next2;
                    }
                    s1Added += s1.add(n_occs[k]);
                }
                ol_seenNeg[(~cl[j]).toInt()] = 1;
                setNeg.push(~cl[j]);
            }
            next2:;

            if (smallestPos != lit_Undef && smallestPos != lit_Error && addedAnyway) {
                vec<ClauseSimp>& p_occs = occur[smallestPos.toInt()];
                for (uint32_t k = 0; k < p_occs.size(); k++) {
                    if (tooMuch && !s0.alreadyIn(p_occs[k])) {
                        addedAnyway = false;
                        goto next3;
                    }
                    s0.add(p_occs[k]);
                }
                ol_seenPos[smallestPos.toInt()] = 1;
            }

            next3:
            if (addedAnyway) remClTouched.push(*it);
        }
        //std::cout << "s0.nElems(): " << s0.nElems() << std::endl;
        //std::cout << "s1.nElems(): " << s1.nElems() << std::endl;


        bool doneAll = (numMaxSubsume1 > 0);
        for (CSet::iterator it = s0.begin(), end = s0.end(); it != end; ++it) {
            if (it->clause == NULL) continue;
            subsume0(*it->clause);
            if (!doneAll && numMaxSubsume0 < 0) break;
        }
        s0.clear();

        if (doneAll) {
            for (CSet::iterator it = s1.begin(), end = s1.end(); it != end; ++it) {
                if (it->clause == NULL) continue;
                subsume1(*it->clause);
                /*if (numMaxSubsume1 < 0) {
                    doneAll = false;
                    break;
                }*/
                //s0.exclude(*it);
                if (!solver.ok) goto end;
            }
            s1.clear();
        }

        if (!handleClBinTouched()) goto end;
        for (ClauseSimp *it = remClTouched.getData(), *end = remClTouched.getDataEnd(); it != end; it++) {
            if (it->clause == NULL) continue;
            if (doneAll) it->clause->unsetStrenghtened();
            cl_touched.exclude(*it);
        }
        if (!doneAll) {
            for (Lit *l = setNeg.getData(), *end = setNeg.getDataEnd(); l != end; l++)
                ol_seenNeg[l->toInt()] = 0;
        }

        #ifdef BIT_MORE_VERBOSITY
        if (doneAll)
            std::cout << "c Success with " << remClTouched.size() << " clauses, s1Added: " << s1Added << std::endl;
        else
            std::cout << "c No success with " << remClTouched.size() << " clauses, s1Added: " << s1Added << std::endl;
        #endif //BIT_MORE_VERBOSITY
        remClTouched.clear();
    } while ((cl_touched.nElems() > 100) && numMaxSubsume0 > 0);

    end:
    cl_touched.clear();
    unregisterIteration(s1);
    unregisterIteration(s0);

    return solver.ok;
}

/**
@brief Links in a clause into the occurrence lists and the clauses[]

Increments clauseID

@param[in] cl The clause to link in
*/
ClauseSimp Subsumer::linkInClause(Clause& cl)
{
    ClauseSimp c(&cl, clauseID++);
    clauses.push(c);
    for (uint32_t i = 0; i < cl.size(); i++) {
        occur[cl[i].toInt()].push(c);
        touch(cl[i], cl.learnt());
    }
    cl_touched.add(c);

    return c;
}

/**
@brief Adds clauses from the solver to here, and removes them from the solver

Which clauses are needed can be controlled by the parameters

@param[in] cs The clause-set to use, e.g. solver.binaryClauses, solver.learnts
@param[in] alsoLearnt Also add learnt clauses?
@param[in] addBinAndAddToCL If set to FALSE, binary clauses are not added, and
clauses are never added to the cl_touched set.
*/
const uint64_t Subsumer::addFromSolver(vec<Clause*>& cs)
{
    uint64_t numLitsAdded = 0;
    Clause **i = cs.getData();
    Clause **j = i;
    for (Clause **end = i + cs.size(); i !=  end; i++) {
        if (i+1 != end)
            __builtin_prefetch(*(i+1), 1, 1);

        if (!alsoLearnt && (*i)->learnt()) {
            *j++ = *i;
            continue;
        }

        ClauseSimp c(*i, clauseID++);
        clauses.push(c);
        Clause& cl = *c.clause;
        for (uint32_t i = 0; i < cl.size(); i++) {
            occur[cl[i].toInt()].push(c);
            //if (cl.getStrenghtened()) touch(cl[i], cl.learnt());
        }
        numLitsAdded += cl.size();

        if (cl.getStrenghtened()) cl_touched.add(c);
    }
    cs.shrink(i-j);

    return numLitsAdded;
}

/**
@brief Frees memory occupied by occurrence lists
*/
void Subsumer::freeMemory()
{
    for (uint32_t i = 0; i < occur.size(); i++) {
        occur[i].clear(true);
    }
}

/**
@brief Adds clauses from here, back to the solver
*/
void Subsumer::addBackToSolver()
{
    assert(solver.clauses.size() == 0);
    for (uint32_t i = 0; i < clauses.size(); i++) {
        if (clauses[i].clause == NULL) continue;
        assert(clauses[i].clause->size() > 2);

        if (clauses[i].clause->learnt())
            solver.learnts.push(clauses[i].clause);
        else
            solver.clauses.push(clauses[i].clause);
    }
}

/**
@brief Remove clauses from input that contain eliminated variables

Used to remove learnt clauses that still reference a variable that has been
eliminated.
*/
void Subsumer::removeWrong(vec<Clause*>& cs)
{
    Clause **i = cs.getData();
    Clause **j = i;
    for (Clause **end =  i + cs.size(); i != end; i++) {
        Clause& c = **i;
        if (!c.learnt())  {
            *j++ = *i;
            continue;
        }
        bool remove = false;
        for (Lit *l = c.getData(), *end2 = l+c.size(); l != end2; l++) {
            if (var_elimed[l->var()]) {
                remove = true;
                //solver.detachClause(c);
                solver.clauseAllocator.clauseFree(&c);
                break;
            }
        }
        if (!remove)
            *j++ = *i;
    }
    cs.shrink(i-j);
}

void Subsumer::removeBinsAndTris(const Var var)
{
    uint32_t numRemovedLearnt = 0;

    Lit lit = Lit(var, false);

    numRemovedLearnt += removeBinAndTrisHelper(lit, solver.watches[(~lit).toInt()]);
    numRemovedLearnt += removeBinAndTrisHelper(~lit, solver.watches[lit.toInt()]);

    solver.learnts_literals -= numRemovedLearnt*2;
    solver.numBins -= numRemovedLearnt;

}

const uint32_t Subsumer::removeBinAndTrisHelper(const Lit lit, vec<Watched>& ws)
{
    uint32_t numRemovedLearnt = 0;

    Watched* i = ws.getData();
    Watched* j = i;
    for (Watched *end = ws.getDataEnd(); i != end; i++) {
        if (i->isTriClause()) continue;

        if (i->isBinary()) {
            assert(i->getLearnt());
            removeWBin(solver.watches[(~(i->getOtherLit())).toInt()], lit, i->getLearnt());
            numRemovedLearnt++;
            continue;
        }

        assert(false);
    }
    ws.shrink_(i - j);

    return numRemovedLearnt;
}

void Subsumer::removeWrongBinsAndAllTris()
{
    uint32_t numRemovedHalfLearnt = 0;
    uint32_t wsLit = 0;
    for (vec<Watched> *it = solver.watches.getData(), *end = solver.watches.getDataEnd(); it != end; it++, wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        vec<Watched>& ws = *it;

        Watched* i = ws.getData();
        Watched* j = i;
        for (Watched *end2 = ws.getDataEnd(); i != end2; i++) {
            if (i->isTriClause()) continue;

            if (i->isBinary()
                && (var_elimed[lit.var()] || var_elimed[i->getOtherLit().var()])
                ) {
                assert(i->getLearnt());
                numRemovedHalfLearnt++;
            } else {
                *j++ = *i;
            }
        }
        ws.shrink_(i - j);
    }

    assert(numRemovedHalfLearnt % 2 == 0);
    solver.learnts_literals -= numRemovedHalfLearnt;
    solver.numBins -= numRemovedHalfLearnt/2;
}

/**
@brief Fills the vector cannot_eliminate

Variables that are:
* also present in XOR-clauses, or
* have been replaced
cannot be eliminated. This is enforced by the vector cannot_elimnate
*/
void Subsumer::fillCannotEliminate()
{
    std::fill(cannot_eliminate.getData(), cannot_eliminate.getDataEnd(), false);
    for (uint32_t i = 0; i < solver.xorclauses.size(); i++) {
        const XorClause& c = *solver.xorclauses[i];
        for (uint32_t i2 = 0; i2 < c.size(); i2++)
            cannot_eliminate[c[i2].var()] = true;
    }

    for (Var var = 0; var < solver.nVars(); var++) {
        cannot_eliminate[var] |= solver.varReplacer->cannot_eliminate[var];
    }

    #ifdef VERBOSE_DEBUG
    uint32_t tmpNum = 0;
    for (uint32_t i = 0; i < cannot_eliminate.size(); i++)
        if (cannot_eliminate[i])
            tmpNum++;
    std::cout << "Cannot eliminate num:" << tmpNum << std::endl;
    #endif
}

/**
@brief Subsumes&strenghtens normal clauses with (non-existing) binary clauses

First, it backward-subsumes and performs self-subsuming resolution using binary
clauses on non-binary clauses. Then, it generates non-existing binary clauses
(that could exist, but would be redundant), and performs self-subsuming
resolution with them on the normal clauses using \function subsume0BIN().
*/
const bool Subsumer::subsumeWithBinaries()
{
    //Clearing stats
    double myTime = cpuTime();
    clauses_subsumed = 0;
    literals_removed = 0;
    uint32_t origTrailSize = solver.trail.size();

    vec<Lit> lits(2);
    uint32_t counter = 0;
    uint32_t thisRand = solver.mtrand.randInt();
    for (const vec<Watched> *it = solver.watches.getData(); counter != solver.nVars()*2; counter++) {
        uint32_t wsLit = (counter + thisRand) % (solver.nVars()*2);
        Lit lit = ~Lit::toLit(wsLit);
        lits[0] = lit;
        const vec<Watched> ws_backup = *(it + wsLit);
        for (const Watched *it2 = ws_backup.getData(), *end2 = ws_backup.getDataEnd(); it2 != end2; it2++) {
            if (it2->isBinary() && lit.toInt() < it2->getOtherLit().toInt()) {
                lits[1] = it2->getOtherLit();
                bool toMakeNonLearnt = subsume1(lits, it2->getLearnt());
                if (toMakeNonLearnt) makeNonLearntBin(lit, it2->getOtherLit(), it2->getLearnt());
                if (!solver.ok) return false;
            }
        }
        if (numMaxSubsume1 < 0) break;
    }

    if (solver.conf.verbosity  >= 1) {
        std::cout << "c subs with bin: " << std::setw(8) << clauses_subsumed
        << "  lits-rem: " << std::setw(9) << literals_removed
        << "  v-fix: " << std::setw(4) <<solver.trail.size() - origTrailSize
        << "  time: " << std::setprecision(2) << std::setw(5) <<  cpuTime() - myTime << " s"
        << std::endl;
    }

    return true;
}

void Subsumer::makeNonLearntBin(const Lit lit1, const Lit lit2, const bool learnt)
{
    assert(learnt == true);
    findWatchedOfBin(solver.watches, lit1 ,lit2, learnt).setLearnt(false);
    findWatchedOfBin(solver.watches, lit2 ,lit1, learnt).setLearnt(false);
    solver.learnts_literals -= 2;
    solver.clauses_literals += 2;
}

#define MAX_BINARY_PROP 60000000

/**
@brief Call subsWNonExistBins with randomly picked starting literals

This is the function that overviews the deletion of all clauses that could be
inferred from non-existing binary clauses, and the strenghtening (through self-
subsuming resolution) of clauses that could be strenghtened using non-existent
binary clauses.
*/
const bool Subsumer::subsWNonExistBinsFill()
{
    double myTime = cpuTime();
    for (vec<Watched> *it = solver.watches.getData(), *end = solver.watches.getDataEnd(); it != end; it++) {
        if (it->size() < 2) continue;
        std::sort(it->getData(), it->getDataEnd(), BinSorter2());
    }

    uint32_t oldTrailSize = solver.trail.size();
    uint64_t oldProps = solver.propagations;
    uint64_t maxProp = MAX_BINARY_PROP*7;
    extraTimeNonExist = 0;
    OnlyNonLearntBins* onlyNonLearntBins = NULL;
    if (solver.clauses_literals < 10*1000*1000) {
        onlyNonLearntBins = new OnlyNonLearntBins(solver);
        onlyNonLearntBins->fill();
        solver.multiLevelProp = true;
    }

    doneNumNonExist = 0;
    uint32_t startFrom = solver.mtrand.randInt(solver.order_heap.size());
    for (uint32_t i = 0; i < solver.order_heap.size(); i++) {
        Var var = solver.order_heap[(startFrom + i) % solver.order_heap.size()];
        if (solver.propagations + extraTimeNonExist*150 > oldProps + maxProp) break;
        if (solver.assigns[var] != l_Undef || !solver.decision_var[var]) continue;
        doneNumNonExist++;
        extraTimeNonExist += 5;

        Lit lit(var, true);
        if (onlyNonLearntBins != NULL && onlyNonLearntBins->getWatchSize(lit) == 0) goto next;
        if (!subsWNonExistBinsFillHelper(lit, onlyNonLearntBins)) {
            if (!solver.ok) return false;
            solver.cancelUntilLight();
            solver.uncheckedEnqueue(~lit);
            solver.ok = solver.propagate().isNULL();
            if (!solver.ok) return false;
            continue;
        }
        extraTimeNonExist += 10;
        next:

        //in the meantime it could have got assigned
        if (solver.assigns[var] != l_Undef) continue;
        lit = ~lit;
        if (onlyNonLearntBins != NULL && onlyNonLearntBins->getWatchSize(lit) == 0) continue;
        if (!subsWNonExistBinsFillHelper(lit, onlyNonLearntBins)) {
            if (!solver.ok) return false;
            solver.cancelUntilLight();
            solver.uncheckedEnqueue(~lit);
            solver.ok = solver.propagate().isNULL();
            if (!solver.ok) return false;
            continue;
        }
        extraTimeNonExist += 10;
    }

    if (onlyNonLearntBins) delete onlyNonLearntBins;


    if (solver.conf.verbosity  >= 1) {
        std::cout << "c Calc non-exist non-lernt bins"
        << " v-fix: " << std::setw(5) << solver.trail.size() - oldTrailSize
        << " done: " << std::setw(6) << doneNumNonExist
        << " time: " << std::fixed << std::setprecision(2) << std::setw(5) << (cpuTime() - myTime) << " s"
        << std::endl;
    }
    totalTime += cpuTime() - myTime;

    return true;
}

/**
@brief Subsumes&strenghtens clauses with non-existent binary clauses

Generates binary clauses that could exist, then calls \function subsume0BIN()
with them, thus performing self-subsuming resolution and subsumption on the
clauses.

@param[in] lit This literal is the starting point of this set of non-existent
binary clauses (this literal is the starting point in the binary graph)
@param onlyNonLearntBins This class is initialised before calling this function
and contains all the non-learnt binary clauses
*/
const bool Subsumer::subsWNonExistBinsFillHelper(const Lit& lit, OnlyNonLearntBins* onlyNonLearntBins)
{
    #ifdef VERBOSE_DEBUG
    std::cout << "subsWNonExistBins called with lit " << lit << std::endl;
    #endif //VERBOSE_DEBUG
    solver.newDecisionLevel();
    solver.uncheckedEnqueueLight(lit);
    bool failed;
    if (onlyNonLearntBins == NULL)
        failed = (!solver.propagateNonLearntBin().isNULL());
    else
        failed = !onlyNonLearntBins->propagate();
    if (failed) return false;

    vector<Lit>& thisCache = binNonLearntCache[(~lit).toInt()].lits;
    thisCache.clear();
    binNonLearntCache[(~lit).toInt()].conflictLastUpdated = solver.conflicts;

    assert(solver.decisionLevel() > 0);
    for (int sublevel = solver.trail.size()-1; sublevel > (int)solver.trail_lim[0]; sublevel--) {
        Lit x = solver.trail[sublevel];
        thisCache.push_back(x);
        solver.assigns[x.var()] = l_Undef;
    }
    solver.assigns[solver.trail[solver.trail_lim[0]].var()] = l_Undef;
    solver.qhead = solver.trail_lim[0];
    solver.trail.shrink_(solver.trail.size() - solver.trail_lim[0]);
    solver.trail_lim.shrink_(solver.trail_lim.size());
    //solver.cancelUntilLight();

    return solver.ok;
}
/**
@brief Clears and deletes (almost) everything in this class

Clears touchlists, occurrance lists, clauses, and variable touched lists
*/
void Subsumer::clearAll()
{
    touchedVarsList.clear();
    touchedVars.clear();
    touchedVars.growTo(solver.nVars(), false);
    for (Var var = 0; var < solver.nVars(); var++) {
        if (solver.decision_var[var] && solver.assigns[var] == l_Undef) touch(var);
        occur[2*var].clear();
        occur[2*var+1].clear();
    }
    clauses.clear();
    cl_touched.clear();
    addedClauseLits = 0;
}

const bool Subsumer::eliminateVars()
{
    #ifdef BIT_MORE_VERBOSITY
    std::cout << "c VARIABLE ELIMINIATION -- touchedVarsList size:" << touchedVarsList.size() << std::endl;
    #endif
    vec<Var> init_order;
    orderVarsForElim(init_order);   // (will untouch all variables)

    for (bool first = true; numMaxElim > 0 && numMaxElimVars > 0; first = false) {
        uint32_t vars_elimed = 0;
        vec<Var> order;

        if (first) {
            for (uint32_t i = 0; i < init_order.size(); i++) {
                const Var var = init_order[i];
                if (!cannot_eliminate[var] && solver.decision_var[var])
                    order.push(var);
                //no need to set touched[var] to false -- orderVarsForElim did that already
            }
        } else {
            for (Var *it = touchedVarsList.getData(), *end = touchedVarsList.getDataEnd(); it != end; it++) {
                const Var var = *it;
                if (!cannot_eliminate[var] && solver.decision_var[var])
                    order.push(var);
                touchedVars[var] = false;
            }
            touchedVarsList.clear();
        }
        #ifdef VERBOSE_DEBUG
        std::cout << "Order size:" << order.size() << std::endl;
        #endif

        for (uint32_t i = 0; i < order.size() && numMaxElim > 0 && numMaxElimVars > 0; i++) {
            if (maybeEliminate(order[i])) {
                if (!solver.ok) return false;
                vars_elimed++;
                numMaxElimVars--;
            }
        }
        if (vars_elimed == 0) break;

        numVarsElimed += vars_elimed;
        #ifdef BIT_MORE_VERBOSITY
        std::cout << "c  #var-elim: " << vars_elimed << std::endl;
        #endif
    }

    return true;
}

void Subsumer::subsumeBinsWithBins()
{
    double myTime = cpuTime();
    uint32_t numBinsBefore = solver.numBins;

    uint32_t wsLit = 0;
    for (vec<Watched> *it = solver.watches.getData(), *end = solver.watches.getDataEnd(); it != end; it++, wsLit++) {
        vec<Watched>& ws = *it;
        Lit lit = ~Lit::toLit(wsLit);
        if (ws.size() < 2) continue;

        std::sort(ws.getData(), ws.getDataEnd(), BinSorter());

        Watched* i = ws.getData();
        Watched* j = i;

        Lit lastLit = lit_Undef;
        bool lastLearnt = false;
        for (Watched *end = ws.getDataEnd(); i != end; i++) {
            if (!i->isBinary()) {
                *j++ = *i;
                continue;
            }
            if (i->getOtherLit() == lastLit) {
                //The sorting algorithm prefers non-learnt to learnt, so it is
                //impossible to have non-learnt before learnt
                assert(!(i->getLearnt() == false && lastLearnt == true));

                assert(i->getOtherLit().var() != lit.var());
                removeWBin(solver.watches[(~(i->getOtherLit())).toInt()], lit, i->getLearnt());
                if (i->getLearnt()) solver.learnts_literals -= 2;
                else {
                    solver.clauses_literals -= 2;
                    touch(lit, i->getLearnt());
                    touch(i->getOtherLit(), i->getLearnt());
                }
                solver.numBins--;
            } else {
                lastLit = i->getOtherLit();
                lastLearnt = i->getLearnt();
                *j++ = *i;
            }
        }
        ws.shrink_(i-j);
    }

    if (solver.conf.verbosity  >= 1) {
        std::cout << "c bin-w-bin subsume rem   "
        << std::setw(10) << (numBinsBefore - solver.numBins) << " bins "
        << " time: "
        << std::fixed << std::setprecision(2) << std::setw(5) << (cpuTime() - myTime)
        << " s" << std::endl;
    }
    totalTime += cpuTime() - myTime;
    clauses_subsumed += (numBinsBefore - solver.numBins);
}

/**
@brief Main function in this class

Performs, recursively:
* backward-subsumption
* self-subsuming resolution
* variable elimination

@param[in] alsoLearnt Should learnt clauses be also hooked into the occurrence
lists? If so, variable elimination cannot take place.
*/
const bool Subsumer::simplifyBySubsumption(const bool _alsoLearnt)
{
    alsoLearnt = _alsoLearnt;
    if (solver.nClauses() > 50000000
        || solver.clauses_literals > 500000000)  return true;

    double myTime = cpuTime();
    clauseID = 0;
    clearAll();

    //if (solver.xorclauses.size() < 30000 && solver.clauses.size() < MAX_CLAUSENUM_XORFIND/10) addAllXorAsNorm();

    if (solver.conf.doReplace && !solver.varReplacer->performReplace(true))
        return false;
    fillCannotEliminate();

    uint32_t expected_size = solver.clauses.size();
    if (alsoLearnt) expected_size += solver.learnts.size();
    clauses.reserve(expected_size);
    cl_touched.reserve(expected_size);

    solver.clauseCleaner->cleanClauses(solver.clauses, ClauseCleaner::clauses);
    if (alsoLearnt) {
        solver.clauseCleaner->cleanClauses(solver.learnts, ClauseCleaner::learnts);

        if (solver.learnts.size() < 10000000)
            std::sort(solver.learnts.getData(), solver.learnts.getDataEnd(), sortBySize());
        addedClauseLits += addFromSolver(solver.learnts);
    } else {
        if (solver.clauses.size() < 10000000)
            std::sort(solver.clauses.getData(), solver.clauses.getDataEnd(), sortBySize());
        addedClauseLits += addFromSolver(solver.clauses);
    }

    CompleteDetachReatacher reattacher(solver);
    reattacher.detachNonBinsNonTris(false);
    totalTime += myTime - cpuTime();

    //Do stuff with binaries
    if (alsoLearnt) {
        numMaxSubsume1 = 500*1000*1000;
        if (solver.conf.doSubsWBins && !subsumeWithBinaries()) return false;
        subsumeNonExist();
        addedClauseLits += addFromSolver(solver.clauses);
    } else {
        if (solver.conf.doBlockedClause) {
            numMaxBlockToVisit = (int64_t)800*1000*1000;
            blockedClauseRemoval();
        }

        subsumeBinsWithBins();
        numMaxSubsume1 = 2*1000*1000*1000;
        if (solver.conf.doSubsWBins && !subsumeWithBinaries()) return false;
        if (solver.conf.doSubsWNonExistBins) {
            if (!subsWNonExistBinsFill()) return false;
            if (!subsumeNonExist()) return false;
        }
        if (!handleClBinTouched()) return false;

        if (solver.conf.doReplace && solver.conf.doRemUselessBins) {
            UselessBinRemover uselessBinRemover(solver);
            if (!uselessBinRemover.removeUslessBinFull()) return false;
        }
    }

    myTime = cpuTime();
    setLimits();
    clauses_subsumed = 0;
    literals_removed = 0;
    numVarsElimed = 0;
    uint32_t origTrailSize = solver.trail.size();

    #ifdef BIT_MORE_VERBOSITY
    std::cout << "c  time until pre-subsume0 clauses and subsume1 2-learnts:" << cpuTime()-myTime << std::endl;
    std::cout << "c  pre-subsumed:" << clauses_subsumed << std::endl;
    std::cout << "c  cl_touched:" << cl_touched.nElems() << std::endl;
    std::cout << "c  clauses:" << clauses.size() << std::endl;
    std::cout << "c  numMaxSubsume0:" << numMaxSubsume0 << std::endl;
    std::cout << "c  numMaxSubsume1:" << numMaxSubsume1 << std::endl;
    std::cout << "c  numMaxElim:" << numMaxElim << std::endl;
    #endif

    /*for (ClauseSimp *it = clauses.getData(), *end = clauses.getDataEnd(); it != end; ++it) {
        if (it->clause == NULL) continue;
        //if (!it->clause->learnt()) continue;
        subsume1(*it->clause);
    }*/
    //setLimits(alsoLearnt);

    if (clauses.size() > 10000000 ||
        (numMaxSubsume1 < 0 && numMaxElim == 0 && numMaxBlockVars == 0))
        goto endSimplifyBySubsumption;

    do {
        if (!subsume0AndSubsume1()) return false;

        if (!solver.conf.doVarElim) break;

        if (!eliminateVars()) return false;

        //subsumeBinsWithBins();
        //if (solver.conf.doSubsWBins && !subsumeWithBinaries()) return false;
        solver.clauseCleaner->removeSatisfiedBins();
    } while (cl_touched.nElems() > 100);
    endSimplifyBySubsumption:

    if (!solver.ok) return false;

    assert(verifyIntegrity());

    removeWrong(solver.learnts);
    removeWrongBinsAndAllTris();
    removeAssignedVarsFromEliminated();

    if (solver.conf.doGateFind && alsoLearnt && !findOrGatesAndTreat()) return false;

    solver.order_heap.filter(Solver::VarFilter(solver));

    addBackToSolver();
    if (!reattacher.reattachNonBins()) return false;

    if (solver.conf.verbosity  >= 1) {
        std::cout << "c lits-rem: " << std::setw(9) << literals_removed
        << "  cl-subs: " << std::setw(8) << clauses_subsumed
        << "  v-elim: " << std::setw(6) << numVarsElimed
        << "  v-fix: " << std::setw(4) <<solver.trail.size() - origTrailSize
        << "  time: " << std::setprecision(2) << std::setw(5) << (cpuTime() - myTime) << " s"
        //<< " blkClRem: " << std::setw(5) << numblockedClauseRemoved
        << std::endl;
    }
    totalTime += cpuTime() - myTime;

    if (!alsoLearnt) {
        BothCache bothCache(solver);
        if (!bothCache.tryBoth(binNonLearntCache)) return false;
    }

    solver.testAllClauseAttach();
    solver.checkNoWrongAttach();
    return true;
}

/**
@brief Calculate limits for backw-subsumption, var elim, etc.

It is important to have limits, otherwise the time taken to perfom these tasks
could be huge. Furthermore, it seems that there is a benefit in doing these
simplifications slowly, instead of trying to use them as much as possible
from the beginning.

@param[in] alsoLearnt Have learnt clauses also been hooked in? If so, variable
elimination must be excluded, for example (i.e. its limit must be 0)
*/
void Subsumer::setLimits()
{
    numMaxSubsume0 = 130*1000*1000;
    numMaxSubsume1 = 80*1000*1000;

    numMaxElim = 500*1000*1000;

    //numMaxElim = 0;
    //numMaxElim = std::numeric_limits<int64_t>::max();

    #ifdef BIT_MORE_VERBOSITY
    std::cout << "c addedClauseLits: " << addedClauseLits << std::endl;
    #endif
    if (addedClauseLits < 10000000) {
        numMaxElim *= 3;
        numMaxSubsume0 *= 3;
        numMaxSubsume1 *= 3;
    }

    if (addedClauseLits < 5000000) {
        numMaxElim *= 4;
        numMaxSubsume0 *= 4;
        numMaxSubsume1 *= 4;
    }

    if (addedClauseLits < 3000000) {
        numMaxElim *= 4;
        numMaxSubsume0 *= 4;
        numMaxSubsume1 *= 4;
    }

    if (addedClauseLits < 1000000) {
        numMaxElim *= 4;
        numMaxSubsume0 *= 4;
        numMaxSubsume1 *= 4;
    }

    numMaxElimVars = (solver.order_heap.size()/3)*numCalls;

    if (solver.order_heap.size() > 200000)
        numMaxBlockVars = (uint32_t)((double)solver.order_heap.size() / 3.5 * (0.8+(double)(numCalls)/4.0));
    else
        numMaxBlockVars = (uint32_t)((double)solver.order_heap.size() / 1.5 * (0.8+(double)(numCalls)/4.0));

    if (!solver.conf.doSubsume1)
        numMaxSubsume1 = 0;

    if (numCalls == 1) {
        numMaxSubsume1 = 10*1000*1000;
    }

    if (alsoLearnt) {
        numMaxElim = 0;
        numMaxElimVars = 0;
        numMaxSubsume0 /= 2;
        numMaxSubsume1 /= 2;
        numMaxBlockVars = 0;
        numMaxBlockToVisit = 0;
    } else {
        numCalls++;
    }

    //For debugging

    //numMaxSubsume0 = 0;
    //numMaxSubsume1 = 0;
    //numMaxSubsume0 = std::numeric_limits<int64_t>::max();
    //numMaxSubsume1 = std::numeric_limits<int64_t>::max();

    //numMaxBlockToVisit = std::numeric_limits<int64_t>::max();
    //numMaxBlockVars = std::numeric_limits<uint32_t>::max();
}

/**
@brief Remove variables from var_elimed if it has been set

While doing, e.g. self-subsuming resolution, it might happen that the variable
that we JUST eliminated has been assigned a value. This could happen for example
if due to clause-cleaning some variable value got propagated that we just set.
Therefore, we must check at the very end if any variables that we eliminated
got set, and if so, the clauses linked to these variables can be fully removed
from elimedOutVar[].
*/
void Subsumer::removeAssignedVarsFromEliminated()
{
    for (Var var = 0; var < var_elimed.size(); var++) {
        if (var_elimed[var] && solver.assigns[var] != l_Undef) {
            var_elimed[var] = false;
            solver.setDecisionVar(var, true);
            numElimed--;

            map<Var, vector<vector<Lit> > >::iterator it = elimedOutVar.find(var);
            if (it != elimedOutVar.end()) elimedOutVar.erase(it);

            map<Var, vector<std::pair<Lit, Lit> > >::iterator it2 = elimedOutVarBin.find(var);
            if (it2 != elimedOutVarBin.end()) elimedOutVarBin.erase(it2);
        }
    }
}

/**
@brief Finds clauses that are backward-subsumed by given clause

Only handles backward-subsumption. Uses occurrence lists

@param[in] ps The clause to backward-subsume with.
@param[in] abs Abstraction of the clause ps
@param[out] out_subsumed The set of clauses subsumed by this clause
*/
template<class T>
void Subsumer::findSubsumed(const T& ps, uint32_t abs, vec<ClauseSimp>& out_subsumed)
{
    #ifdef VERBOSE_DEBUG
    cout << "findSubsumed: " << ps << std::endl;
    #endif

    for (uint32_t i = 0; i != ps.size(); i++)
        seen_tmp[ps[i].toInt()] = 1;

    uint32_t min_i = 0;
    for (uint32_t i = 1; i < ps.size(); i++){
        if (occur[ps[i].toInt()].size() < occur[ps[min_i].toInt()].size())
            min_i = i;
    }

    vec<ClauseSimp>& cs = occur[ps[min_i].toInt()];
    numMaxSubsume0 -= cs.size()*10 + 5;
    for (ClauseSimp *it = cs.getData(), *end = it + cs.size(); it != end; it++){
        if (it+1 != end)
            __builtin_prefetch((it+1)->clause, 1, 1);

        if (it->clause != (Clause*)&ps
            && subsetAbst(abs, it->clause->getAbst())
            && ps.size() <= it->clause->size()) {
                numMaxSubsume0 -= (*it).clause->size() + ps.size();
                if (subset(ps.size(), *it->clause)) {
                out_subsumed.push(*it);
                #ifdef VERBOSE_DEBUG
                cout << "subsumed: ";
                it->clause->plainPrint();
                #endif
            }
        }
    }

    for (uint32_t i = 0; i != ps.size(); i++)
        seen_tmp[ps[i].toInt()] = 0;
}

/**
@brief Checks if clauses are subsumed or could be strenghtened with given clause

Checks if:
* any clause is subsumed with given clause
* the given clause could perform self-subsuming resolution on any other clause

Only takes into consideration clauses that are in the occurrence lists.

@param[in] ps The clause to perform the above listed algos with
@param[in] abs The abstraction of clause ps
@param[out] out_subsumed The clauses that could be modified by ps
@param[out] out_lits Defines HOW these clauses could be modified. By removing
literal, or by subsumption (in this case, there is lit_Undef here)
*/
template<class T>
void Subsumer::findSubsumed1(const T& ps, uint32_t abs, vec<ClauseSimp>& out_subsumed, vec<Lit>& out_lits)
{
    #ifdef VERBOSE_DEBUG
    cout << "findSubsumed1: " << ps << std::endl;
    #endif

    Var minVar = var_Undef;
    uint32_t bestSize = std::numeric_limits<uint32_t>::max();
    for (uint32_t i = 0; i < ps.size(); i++){
        uint32_t newSize = occur[ps[i].toInt()].size()+ occur[(~ps[i]).toInt()].size();
        if (newSize < bestSize) {
            minVar = ps[i].var();
            bestSize = newSize;
        }
    }
    assert(minVar != var_Undef);

    numMaxSubsume1 -= bestSize*10 + 10;
    fillSubs(ps, abs, out_subsumed, out_lits, Lit(minVar, true));
    fillSubs(ps, abs, out_subsumed, out_lits, Lit(minVar, false));
}

/**
@brief Helper function for findSubsumed1

Used to avoid duplication of code
*/
template<class T>
void inline Subsumer::fillSubs(const T& ps, uint32_t abs, vec<ClauseSimp>& out_subsumed, vec<Lit>& out_lits, const Lit lit)
{
    Lit litSub;
    vec<ClauseSimp>& cs = occur[lit.toInt()];
    for (ClauseSimp *it = cs.getData(), *end = it + cs.size(); it != end; it++) {
        if (it+1 != end)
            __builtin_prefetch((it+1)->clause, 1, 1);

        if (it->clause != (Clause*)&ps
            && subsetAbst(abs, it->clause->getAbst())
            && ps.size() <= it->clause->size()) {
            numMaxSubsume1 -= (*it).clause->size() + ps.size();
            litSub = subset1(ps, *it->clause);
            if (litSub != lit_Error) {
                out_subsumed.push(*it);
                out_lits.push(litSub);
                #ifdef VERBOSE_DEBUG
                if (litSub == lit_Undef) cout << "subsume0-d: ";
                else cout << "subsume1-ed (lit: " << litSub << "): ";
                it->clause->plainPrint();
                #endif
            }
        }
    }
}


void Subsumer::removeClausesHelper(vec<ClAndBin>& todo, const Var var, std::pair<uint32_t, uint32_t>& removed)
{
     pair<uint32_t, uint32_t>  tmp;
     for (uint32_t i = 0; i < todo.size(); i++) {
        ClAndBin& c = todo[i];
        if (!c.isBin) {
            unlinkClause(c.clsimp, var);
        } else {
            #ifdef VERBOSE_DEBUG
            std::cout << "Eliminating bin clause: " << c.lit1 << " , " << c.lit2 << std::endl;
            std::cout << "On variable: " << var+1 << std::endl;
            #endif
            assert(var == c.lit1.var() || var == c.lit2.var());
            tmp = removeWBinAll(solver.watches[(~c.lit1).toInt()], c.lit2);
            //assert(tmp.first > 0 || tmp.second > 0);
            removed.first += tmp.first;
            removed.second += tmp.second;

            tmp = removeWBinAll(solver.watches[(~c.lit2).toInt()], c.lit1);
            //assert(tmp.first > 0 || tmp.second > 0);
            removed.first += tmp.first;
            removed.second += tmp.second;

            elimedOutVarBin[var].push_back(std::make_pair(c.lit1, c.lit2));
            #ifndef TOUCH_LESS
            touch(c.lit1, false);
            touch(c.lit2, false);
            #endif
        }
    }
}

/**
@brief Used for variable elimination

Migrates clauses in poss to ps, and negs to ns
Also unlinks ass clauses is ps and ns. This is special unlinking, since it
actually saves the clauses for later re-use when extending the model, or re-
introducing the eliminated variables.

@param[in] poss The occurrence list of var where it is positive
@param[in] negs The occurrence list of var where it is negavite
@param[out] ps Where thre clauses from poss have been moved
@param[out] ns Where thre clauses from negs have been moved
@param[in] var The variable that is being eliminated
*/
void Subsumer::removeClauses(vec<ClAndBin>& posAll, vec<ClAndBin>& negAll, const Var var)
{
    pair<uint32_t, uint32_t> removed;
    removed.first = 0;
    removed.second = 0;

    removeClausesHelper(posAll, var, removed);
    removeClausesHelper(negAll, var, removed);

    solver.learnts_literals -= removed.first;
    solver.clauses_literals -= removed.second;
    solver.numBins -= (removed.first + removed.second)/2;
}

const uint32_t Subsumer::numNonLearntBins(const Lit lit) const
{
    uint32_t num = 0;
    const vec<Watched>& ws = solver.watches[(~lit).toInt()];
    for (const Watched *it = ws.getData(), *end = ws.getDataEnd(); it != end; it++) {
        if (it->isBinary() && !it->getLearnt()) num++;
    }

    return num;
}

void Subsumer::fillClAndBin(vec<ClAndBin>& all, vec<ClauseSimp>& cs, const Lit lit)
{
    for (uint32_t i = 0; i < cs.size(); i++)
        all.push(ClAndBin(cs[i]));

    const vec<Watched>& ws = solver.watches[(~lit).toInt()];
    for (const Watched *it = ws.getData(), *end = ws.getDataEnd(); it != end; it++) {
        if (it->isBinary() &&!it->getLearnt()) all.push(ClAndBin(lit, it->getOtherLit()));
    }
}

/**
@brief Tries to eliminate variable

Tries to eliminate a variable. It uses heuristics to decide whether it's a good
idea to eliminate a variable or not.

@param[in] var The variable that is being eliminated
@return TRUE if variable was eliminated
*/
bool Subsumer::maybeEliminate(const Var var)
{
    assert(!var_elimed[var]);
    assert(!cannot_eliminate[var]);
    assert(solver.decision_var[var]);
    if (solver.value(var) != l_Undef) return false;

    Lit lit = Lit(var, false);

    //Only exists in binary clauses -- don't delete it then
    /*if (occur[lit.toInt()].size() == 0 && occur[(~lit).toInt()].size() == 0)
        return false;*/

    const uint32_t numNonLearntPos = numNonLearntBins(lit);
    const uint32_t posSize = occur[lit.toInt()].size() + numNonLearntPos;
    const uint32_t numNonLearntNeg = numNonLearntBins(~lit);
    const uint32_t negSize = occur[(~lit).toInt()].size() + numNonLearntNeg;
    vec<ClauseSimp>& poss = occur[lit.toInt()];
    vec<ClauseSimp>& negs = occur[(~lit).toInt()];
    numMaxElim -= posSize + negSize;

    // Heuristic CUT OFF:
    if (posSize >= 10 && negSize >= 10) return false;

    // Count clauses/literals before elimination:
    uint32_t before_literals = numNonLearntNeg*2 + numNonLearntPos*2;
    for (uint32_t i = 0; i < poss.size(); i++) before_literals += poss[i].clause->size();
    for (uint32_t i = 0; i < negs.size(); i++) before_literals += negs[i].clause->size();

    // Heuristic CUT OFF2:
    if ((posSize >= 3 && negSize >= 3 && before_literals > 300)
        && clauses.size() > 700000)
        return false;
    if ((posSize >= 5 && negSize >= 5 && before_literals > 400)
        && clauses.size() <= 700000 && clauses.size() > 100000)
        return false;
    if ((posSize >= 8 && negSize >= 8 && before_literals > 700)
        && clauses.size() <= 100000)
        return false;

    vec<ClAndBin> posAll, negAll;
    fillClAndBin(posAll, poss, lit);
    fillClAndBin(negAll, negs, ~lit);
    assert(posAll.size() == posSize);
    assert(negAll.size() == negSize);

    // Count clauses/literals after elimination:
    numMaxElim -= posSize * negSize + before_literals;
    uint32_t before_clauses = posSize + negSize;
    uint32_t after_clauses = 0;
    vec<Lit> dummy; //to reduce temporary data allocation
    for (uint32_t i = 0; i < posAll.size(); i++) for (uint32_t j = 0; j < negAll.size(); j++){
        // Merge clauses. If 'y' and '~y' exist, clause will not be created.
        dummy.clear();
        bool ok = merge(posAll[i], negAll[j], lit, ~lit, dummy);
        if (ok){
            after_clauses++;
            if (after_clauses > before_clauses) return false;
        }
    }

    //Eliminate:
    numMaxElim -= posSize * negSize + before_literals;
    poss.clear();
    negs.clear();
    //addLearntBinaries(var);
    removeClauses(posAll, negAll, var);

    #ifndef NDEBUG
    for (uint32_t i = 0; i < solver.watches[lit.toInt()].size(); i++) {
        Watched& w = solver.watches[lit.toInt()][i];
        assert(w.isTriClause() || (w.isBinary() && w.getLearnt()));
    }
    for (uint32_t i = 0; i < solver.watches[(~lit).toInt()].size(); i++) {
        Watched& w = solver.watches[(~lit).toInt()][i];
        assert(w.isTriClause() || (w.isBinary() && w.getLearnt()));
    }
    #endif

    for (uint32_t i = 0; i < posAll.size(); i++) for (uint32_t j = 0; j < negAll.size(); j++){
        dummy.clear();
        bool ok = merge(posAll[i], negAll[j], lit, ~lit, dummy);
        if (!ok) continue;

        uint32_t group_num = 0;
        #ifdef STATS_NEEDED
        group_num = solver.learnt_clause_group++;
        if (solver.dynamic_behaviour_analysis) {
            string name = solver.logger.get_group_name(ps[i].clause->getGroup()) + " " + solver.logger.get_group_name(ns[j].clause->getGroup());
            solver.logger.set_group_name(group_num, name);
        }
        #endif

        if (cleanClause(dummy)) continue;
        #ifdef VERBOSE_DEBUG
        std::cout << "Adding new clause due to varelim: " << dummy << std::endl;
        #endif
        switch (dummy.size()) {
            case 0:
                solver.ok = false;
                break;
            case 1: {
                handleSize1Clause(dummy[0]);
                break;
            }
            case 2: {
                if (findWBin(solver.watches, dummy[0], dummy[1])) {
                    Watched& w = findWatchedOfBin(solver.watches, dummy[0], dummy[1]);
                    if (w.getLearnt()) {
                        w.setLearnt(false);
                        findWatchedOfBin(solver.watches, dummy[1], dummy[0], true).setLearnt(false);
                        solver.learnts_literals -= 2;
                        solver.clauses_literals += 2;
                    }
                } else {
                    solver.attachBinClause(dummy[0], dummy[1], false);
                    solver.numNewBin++;
                    solver.dataSync->signalNewBinClause(dummy);
                }
                subsume1(dummy, false);
                break;
            }
            default: {
                Clause* cl = solver.clauseAllocator.Clause_new(dummy, group_num);
                ClauseSimp c = linkInClause(*cl);
                if (numMaxSubsume1 > 0) subsume1(*c.clause);
                else subsume0(*c.clause);
            }
        }
        if (!solver.ok) return true;
    }

    //removeBinsAndTris(var);

    assert(occur[lit.toInt()].size() == 0 &&  occur[(~lit).toInt()].size() == 0);
    var_elimed[var] = true;
    numElimed++;
    solver.setDecisionVar(var, false);
    return true;
}

void Subsumer::addLearntBinaries(const Var var)
{
    vec<Lit> tmp;
    Lit lit = Lit(var, false);
    const vec<Watched>& ws = solver.watches[lit.toInt()];
    const vec<Watched>& ws2 = solver.watches[(~lit).toInt()];

    for (const Watched *w1 = ws.getData(), *end1 = ws.getDataEnd(); w1 != end1; w1++) {
        if (!w1->isBinary()) continue;
        const bool numOneIsLearnt = w1->getLearnt();
        const Lit lit1 = w1->getOtherLit();
        if (solver.value(lit1) != l_Undef || var_elimed[lit1.var()]) continue;

        for (const Watched *w2 = ws2.getData(), *end2 = ws2.getDataEnd(); w2 != end2; w2++) {
            if (!w2->isBinary()) continue;
            const bool numTwoIsLearnt = w2->getLearnt();
            if (!numOneIsLearnt && !numTwoIsLearnt) {
                //At least one must be learnt
                continue;
            }

            const Lit lit2 = w2->getOtherLit();
            if (solver.value(lit2) != l_Undef || var_elimed[lit2.var()]) continue;

            tmp.clear();
            tmp.growTo(2);
            tmp[0] = lit1;
            tmp[1] = lit2;
            Clause* tmpOK = solver.addClauseInt(tmp, 0, true);
            release_assert(tmpOK == NULL);
            release_assert(solver.ok);
        }
    }
    assert(solver.value(lit) == l_Undef);
}

/**
@brief Resolves two clauses on a variable

Clause ps must contain without_p
Clause ps must contain without_q
And without_p = ~without_q

@note: 'seen' is assumed to be cleared.

@param[in] var The variable that is being eliminated
@return FALSE if clause is always satisfied ('out_clause' should not be used)
*/
bool Subsumer::merge(const ClAndBin& ps, const ClAndBin& qs, const Lit without_p, const Lit without_q, vec<Lit>& out_clause)
{
    bool retval = true;
    if (ps.isBin) {
        assert(ps.lit1 == without_p);
        assert(ps.lit2 != without_p);

        seen_tmp[ps.lit2.toInt()] = 1;
        out_clause.push(ps.lit2);
    } else {
        Clause& c = *ps.clsimp.clause;
        numMaxElim -= c.size();
        for (uint32_t i = 0; i < c.size(); i++){
            if (c[i] != without_p){
                seen_tmp[c[i].toInt()] = 1;
                out_clause.push(c[i]);
            }
        }
    }

    if (qs.isBin) {
        assert(qs.lit1 == without_q);
        assert(qs.lit2 != without_q);

        if (seen_tmp[(~qs.lit2).toInt()]) {
            retval = false;
            goto end;
        }
        if (!seen_tmp[qs.lit2.toInt()])
            out_clause.push(qs.lit2);
    } else {
        Clause& c = *qs.clsimp.clause;
        numMaxElim -= c.size();
        for (uint32_t i = 0; i < c.size(); i++){
            if (c[i] != without_q) {
                if (seen_tmp[(~c[i]).toInt()]) {
                    retval = false;
                    goto end;
                }
                if (!seen_tmp[c[i].toInt()])
                    out_clause.push(c[i]);
            }
        }
    }

    end:
    if (ps.isBin) {
        seen_tmp[ps.lit2.toInt()] = 0;
    } else {
        Clause& c = *ps.clsimp.clause;
        numMaxElim -= c.size();
        for (uint32_t i = 0; i < c.size(); i++)
            seen_tmp[c[i].toInt()] = 0;
    }

    return retval;
}

/**
@brief Orders variables for elimination

Variables are ordered according to their occurrances. If a variable occurs far
less than others, it should be prioritised for elimination. The more difficult
variables are OK to try later.

@note: Will untouch all variables.

@param[out] order The order to try to eliminate the variables
*/
void Subsumer::orderVarsForElim(vec<Var>& order)
{
    order.clear();
    vec<pair<int, Var> > cost_var;
    for (uint32_t i = 0; i < touchedVarsList.size(); i++){
        Lit x = Lit(touchedVarsList[i], false);
        touchedVars[x.var()] = false;
        //this is not perfect -- solver.watches[] is an over-approximation
        uint32_t pos = occur[x.toInt()].size();
        uint32_t neg = occur[(~x).toInt()].size();
        uint32_t cost = pos * neg * 2 + numNonLearntBins(x) * neg + numNonLearntBins(~x) * pos;
        cost_var.push(std::make_pair(cost, x.var()));
    }
    touchedVarsList.clear();

    std::sort(cost_var.getData(), cost_var.getData()+cost_var.size(), myComp());
    for (uint32_t x = 0; x < cost_var.size(); x++) {
        if (cost_var[x].first != 0)
            order.push(cost_var[x].second);
    }
}

/**
@brief Verifies that occurrence lists are OK

Calculates how many occurences are of the varible in clauses[], and if that is
less than occur[var].size(), returns FALSE

@return TRUE if they are OK
*/
const bool Subsumer::verifyIntegrity()
{
    vector<uint32_t> occurNum(solver.nVars()*2, 0);

    for (uint32_t i = 0; i < clauses.size(); i++) {
        if (clauses[i].clause == NULL) continue;
        Clause& c = *clauses[i].clause;
        for (uint32_t i2 = 0; i2 < c.size(); i2++)
            occurNum[c[i2].toInt()]++;
    }

    for (uint32_t i = 0; i < occurNum.size(); i++) {
        #ifdef VERBOSE_DEBUG
        std::cout << "occurNum[i]:" << occurNum[i]
        << " occur[i]:" << occur[i].size()
        << "  --- i:" << i << std::endl;
        #endif //VERBOSE_DEBUG

        if (occurNum[i] != occur[i].size()) return false;
    }

    return true;
}

template<class T>
const bool Subsumer::allTautology(const T& ps, const Lit lit)
{
    #ifdef VERBOSE_DEBUG
    cout << "allTautology: ";
    for (uint32_t i = 0; i < ps.size(); i++) {
        if (ps[i].sign()) printf("-");
        printf("%d ", ps[i].var() + 1);
    }
    printf("0\n");
    #endif

    numMaxBlockToVisit -= ps.size()*2;
    for (const Lit *l = ps.getData(), *end = ps.getDataEnd(); l != end; l++) {
        if (*l != ~lit) seen_tmp[l->toInt()] = true;
    }

    bool allIsTautology = true;
    const vec<ClauseSimp>& cs = occur[lit.toInt()];
    const vec<Watched>& ws = solver.watches[(~lit).toInt()];

    for (const ClauseSimp *it = cs.getData(), *end = cs.getDataEnd(); it != end; it++){
        if (it+1 != end) __builtin_prefetch((it+1)->clause, 1, 1);

        const Clause& c = *it->clause;
        numMaxBlockToVisit -= c.size();
        for (const Lit *l = c.getData(), *end2 = c.getDataEnd(); l != end2; l++) {
            if (seen_tmp[(~(*l)).toInt()]) {
                goto next;
            }
        }
        allIsTautology = false;
        break;

        next:;
    }
    if (!allIsTautology) goto end;

    numMaxBlockToVisit -= ws.size();
    for (const Watched *it = ws.getData(), *end = ws.getDataEnd(); it != end; it++) {
        if (!it->isNonLearntBinary()) continue;
        if (seen_tmp[(~it->getOtherLit()).toInt()]) continue;
        else {
            allIsTautology = false;
            break;
        }
    }

    end:
    for (const Lit *l = ps.getData(), *end = ps.getDataEnd(); l != end; l++) {
        seen_tmp[l->toInt()] = false;
    }

    return allIsTautology;
}

void Subsumer::blockedClauseRemoval()
{
    if (numMaxBlockToVisit < 0) return;
    if (solver.order_heap.size() < 1) return;

    double myTime = cpuTime();
    numblockedClauseRemoved = 0;
    uint32_t numElimedBefore = numElimed;

    touchedBlockedVars = priority_queue<VarOcc, vector<VarOcc>, MyComp>();
    touchedBlockedVarsBool.clear();
    touchedBlockedVarsBool.growTo(solver.nVars(), false);
    for (uint32_t i =  0; i < solver.order_heap.size(); i++) {
        //if (solver.order_heap.size() < 1) break;
        //touchBlockedVar(solver.order_heap[solver.mtrand.randInt(solver.order_heap.size()-1)]);
        touchBlockedVar(solver.order_heap[i]);
    }

    uint32_t triedToBlock = 0;
    while (numMaxBlockToVisit > 0 && !touchedBlockedVars.empty() && numMaxElimVars > 0) {
        VarOcc vo = touchedBlockedVars.top();
        touchedBlockedVars.pop();
        touchedBlockedVarsBool[vo.var] = false;

        if (solver.assigns[vo.var] != l_Undef || !solver.decision_var[vo.var] || cannot_eliminate[vo.var])
            continue;

        triedToBlock++;
        Lit lit = Lit(vo.var, false);

        //if (!tryOneSetting(lit)) {
            tryOneSetting(lit);
       // }
    }

    if (solver.conf.verbosity >= 1) {
        std::cout
        << "c blocked clauses removed: " << std::setw(8) << numblockedClauseRemoved
        << " vars: " << std::setw(6) << numElimed - numElimedBefore
        << " tried: " << std::setw(11) << triedToBlock
        << " T: " << std::fixed << std::setprecision(2) << std::setw(4) << cpuTime() - myTime
        << " s" << std::endl;
    }
}

const bool Subsumer::tryOneSetting(const Lit lit)
{
    numMaxBlockToVisit -= occur[lit.toInt()].size();
    for(ClauseSimp *it = occur[lit.toInt()].getData(), *end = occur[lit.toInt()].getDataEnd(); it != end; it++) {
        if (!allTautology(*it->clause, ~lit)) {
            return false;
        }
    }

    vec<Lit> lits(1);
    const vec<Watched>& ws = solver.watches[(~lit).toInt()];
    numMaxBlockToVisit -= ws.size();
    for (const Watched *it = ws.getData(), *end = ws.getDataEnd(); it != end; it++) {
        if (!it->isNonLearntBinary()) continue;
        lits[0] = it->getOtherLit();
        if (!allTautology(lits, ~lit)) return false;
    }

    blockedClauseElimAll(lit);
    blockedClauseElimAll(~lit);

    var_elimed[lit.var()] = true;
    numElimed++;
    numMaxElimVars--;
    solver.setDecisionVar(lit.var(), false);

    return true;
}

void Subsumer::blockedClauseElimAll(const Lit lit)
{
    vec<ClauseSimp> toRemove(occur[lit.toInt()]);
    for (ClauseSimp *it = toRemove.getData(), *end = toRemove.getDataEnd(); it != end; it++) {
        #ifdef VERBOSE_DEBUG
        std::cout << "Next varelim because of block clause elim" << std::endl;
        #endif //VERBOSE_DEBUG
        unlinkClause(*it, lit.var());
        numblockedClauseRemoved++;
    }

    uint32_t removedNum = 0;
    vec<Watched>& ws = solver.watches[(~lit).toInt()];
    Watched *i = ws.getData();
    Watched *j = i;
    for (Watched *end = ws.getDataEnd(); i != end; i++) {
        if (!i->isNonLearntBinary()) {
            *j++ = *i;
            continue;
        }
        removeWBin(solver.watches[(~i->getOtherLit()).toInt()], lit, i->getLearnt());
        elimedOutVarBin[lit.var()].push_back(std::make_pair(lit, i->getOtherLit()));
        touch(i->getOtherLit(), false);
        removedNum++;
    }
    ws.shrink_(i-j);

    solver.clauses_literals -= removedNum*2;
    solver.numBins -= removedNum;
}

/**
@brief Checks if eliminated variables are unassigned

If there is a variable that has been assigned even though it's been eliminated
that means that there were clauses that contained that variable, and where some-
how inserted into the watchlists. That would be a grave bug, since that would
mean that not all clauses containing the eliminated variable were removed during
the running of this class.

@return TRUE if they are all unassigned
*/
const bool Subsumer::checkElimedUnassigned() const
{
    uint32_t checkNumElimed = 0;
    for (uint32_t i = 0; i < var_elimed.size(); i++) {
        if (var_elimed[i]) {
            checkNumElimed++;
            assert(solver.assigns[i] == l_Undef);
            if (solver.assigns[i] != l_Undef) return false;
        }
    }
    assert(numElimed == checkNumElimed);

    return true;
}

const bool Subsumer::findOrGatesAndTreat()
{
    assert(solver.ok);
    double myTime = cpuTime();
    orGates.clear();
    totalOrGateSize = 0;
    uint32_t oldNumVarToReplace = solver.varReplacer->getNewToReplaceVars();
    uint32_t oldNumBins = solver.numBins;
    gateLitsRemoved = 0;
    numOrGateReplaced = 0;
    defOfOrGate.clear();
    defOfOrGate.growTo(clauses.size(), false);
    uint32_t old_clauses_subsumed = clauses_subsumed;

    findOrGates(false);
    for (uint32_t i = 0; i < orGates.size(); i++) {
        std::sort(orGates[i].lits.begin(), orGates[i].lits.end());
    }
    std::sort(orGates.begin(), orGates.end(), OrGateSorter2());

    for (vector<OrGate>::const_iterator it = orGates.begin(), end = orGates.end(); it != end; it++) {
        if (!shortenWithOrGate(*it)) goto end;
    }

    if (!findEqOrGates()) goto end;

    end:
    if (solver.conf.verbosity >= 1) {
        std::cout << "c gates found : " << std::setw(6) << orGates.size()
        << " avg size: " << std::fixed << std::setw(4) << std::setprecision(1) << ((double)totalOrGateSize/(double)orGates.size())
        << " cl-shorten: " << std::setw(5) << numOrGateReplaced
        << " lits-rem: " << std::setw(6) << gateLitsRemoved
        << " bin-add: " << std::setw(6) << (solver.numBins - oldNumBins)
        << " var-repl: " << std::setw(3) << (solver.varReplacer->getNewToReplaceVars() - oldNumVarToReplace)
        << " T: " << std::fixed << std::setw(7) << std::setprecision(2) <<  (cpuTime() - myTime) << std::endl;
    }

    /*for (uint32_t i = 0; i < orGates.size(); i++) {
        const OrGate& v = orGates[i];
        std::cout << " " << v.eqLit << " = ";
        for (uint32_t i2 = 0; i2 < v.lits.size(); i2++) {
            std::cout << v.lits[i2] << " , ";
        }
        std::cout << std::endl;
    }

    myTime = cpuTime();
    orGates.clear();
    totalOrGateSize = 0;
    leftHandOfGate.clear();
    leftHandOfGate.growTo(solver.nVars()*2, false);
    oldNumVarToReplace = solver.varReplacer->getNewToReplaceVars();
    oldNumBins = solver.numBins;
    gateLitsRemoved = 0;
    numOrGateReplaced = 0;
    findOrGates(true);
    if (solver.conf.verbosity >= 1) {
        std::cout << "c gates found : " << std::setw(6) << orGates.size()
        << " avg size: " << std::fixed << std::setw(4) << std::setprecision(1) << ((double)totalOrGateSize/(double)orGates.size())
        << std::endl;
    }*/

    clauses_subsumed = old_clauses_subsumed;

    return solver.ok;
}

const bool Subsumer::findEqOrGates()
{
    assert(solver.ok);

    vec<Lit> tmp(2);
    for (uint32_t i = 1; i < orGates.size(); i++) {
        const OrGate& gate1 = orGates[i-1];
        const OrGate& gate2 = orGates[i];
        if (gate1.lits == gate2.lits
            && gate1.eqLit.var() != gate2.eqLit.var()
           ) {
            tmp[0] = gate1.eqLit.unsign();
            tmp[1] = gate2.eqLit.unsign();
            const bool sign = true ^ gate1.eqLit.sign() ^ gate2.eqLit.sign();
            Clause *c = solver.addXorClauseInt(tmp, sign, 0);
            tmp.clear();
            tmp.growTo(2);
            assert(c == NULL);
            if (!solver.ok) return false;
        }
    }

    return true;
}

void Subsumer::findOrGates(const bool learntGatesToo)
{
    for (const ClauseSimp *it = clauses.getData(), *end = clauses.getDataEnd(); it != end; it++) {
        if (it->clause == NULL) continue;
        const Clause& cl = *it->clause;
        if (!learntGatesToo && cl.learnt()) continue;

        uint8_t numSizeZeroCache = 0;
        Lit which = lit_Undef;
        for (const Lit *l = cl.getData(), *end2 = cl.getDataEnd(); l != end2; l++) {
            Lit lit = *l;
            vector<Lit> const* cache;
            if (!learntGatesToo) cache = &binNonLearntCache[(~lit).toInt()].lits;
            else cache = &solver.transOTFCache[(~lit).toInt()].lits;

            if (cache->size() == 0) {
                numSizeZeroCache++;
                if (numSizeZeroCache > 1) break;
                which = lit;
            }
        }
        if (numSizeZeroCache > 1) continue;

        if (numSizeZeroCache == 1) {
            findOrGate(which, *it, learntGatesToo);
        } else {
            for (const Lit *l = cl.getData(), *end2 = cl.getDataEnd(); l != end2; l++)
                findOrGate(*l, *it, learntGatesToo);
        }
    }
}

void Subsumer::findOrGate(const Lit eqLit, const ClauseSimp& c, const bool learntGatesToo)
{
    Clause& cl = *c.clause;
    bool isEqual = true;
    for (const Lit *l2 = cl.getData(), *end3 = cl.getDataEnd(); l2 != end3; l2++) {
        if (*l2 == eqLit) continue;
        Lit otherLit = *l2;
        vector<Lit> const* cache;
        if (!learntGatesToo) cache = &binNonLearntCache[(~otherLit).toInt()].lits;
        else cache = &solver.transOTFCache[(~otherLit).toInt()].lits;

        bool OK = false;
        for (vector<Lit>::const_iterator cacheLit = cache->begin(), endCache = cache->end(); cacheLit != endCache; cacheLit++) {
            if (*cacheLit == ~eqLit) {
                OK = true;
                break;
            }
        }
        if (!OK) {
            isEqual = false;
            break;
        }
    }

    if (isEqual) {
        OrGate gate;
        for (const Lit *l2 = cl.getData(), *end3 = cl.getDataEnd(); l2 != end3; l2++) {
            if (*l2 == eqLit) continue;
            gate.lits.push_back(*l2);
        }
        gate.eqLit = ~eqLit;
        orGates.push_back(gate);
        totalOrGateSize += gate.lits.size();
        assert(c.index < defOfOrGate.size());
        defOfOrGate[c.index] = true;
    }
}

const bool Subsumer::shortenWithOrGate(const OrGate& gate)
{
    assert(solver.ok);

    vec<ClauseSimp> subs;
    findSubsumed(gate.lits, calcAbstraction(gate.lits), subs);
    for (uint32_t i = 0; i < subs.size(); i++) {
        ClauseSimp c = subs[i];
        Clause* cl = subs[i].clause;

        #ifdef VERBOSE_ORGATE_REPLACE
        std::cout << "gate used: eqLit " << gate.eqLit << std::endl;
        for (uint32_t i = 0; i < gate.lits.size(); i++) {
            std::cout << "lit: " << gate.lits[i] << " , ";
        }
        std::cout << std::endl;
        std::cout << "origClause: " << *cl << std::endl;
        #endif

        bool replaceDef = false;
        bool exactReplace = false;
        //Let's say there are two gates:
        // a V b V -f
        // c V d V -g
        // now, if we replace (a V b) with g and we replace (c V d) with f, then
        // we have just lost the definition of the gates!!
        if (c.index < defOfOrGate.size() && defOfOrGate[c.index] && cl->size() == gate.lits.size() + 1) {
            replaceDef = true;
            for (Lit *l = cl->getData(), *end = cl->getDataEnd(); l != end; l++) {
                if (gate.eqLit == ~(*l)) exactReplace = true;
            }
        }
        if (exactReplace) continue;
        if (replaceDef) {
            if (cl->size() != 3) continue;
            vec<Lit> lits(cl->size());
            std::copy(cl->getData(), cl->getDataEnd(), lits.getData());
            for (vector<Lit>::const_iterator it = gate.lits.begin(), end = gate.lits.end(); it != end; it++) {
                remove(lits, *it);
            }
            lits.push(gate.eqLit);
            if (lits[0].var() != lits[1].var()
                && (solver.cacheContainsBinCl(lits[0], lits[1])
                    || solver.cacheContainsBinCl(lits[1], lits[0]))
                ) continue;

            #ifdef VERBOSE_ORGATE_REPLACE
            std::cout << "Adding learnt clause " << lits << " due to gate replace-avoid" << std::endl;
            #endif

            Clause* c2 = solver.addClauseInt(lits, 0, true);
            assert(c2 == NULL);
            if (!solver.ok)  return false;
            continue;
        }

        numOrGateReplaced++;
        #ifdef VERBOSE_ORGATE_REPLACE
        std::cout << "Cl changed (due to gate) : " << *cl << std::endl;
        #endif

        for (vector<Lit>::const_iterator it = gate.lits.begin(), end = gate.lits.end(); it != end; it++) {
            gateLitsRemoved++;
            cl->strengthen(*it);
            maybeRemove(occur[it->toInt()], cl);
            /*#ifndef TOUCH_LESS
            touch(*it, cl.learnt());
            #endif*/
        }

        bool inside = false;
        bool invertInside = false;
        for (Lit *l = cl->getData(), *end = cl->getDataEnd(); l != end; l++) {
            if (gate.eqLit == (*l)) {
                inside = true;
                break;
            }
            if (gate.eqLit == ~(*l)) {
                invertInside = true;
                break;
            }
        }
        if (invertInside) {
            unlinkClause(c);
            continue;
        }

        if (!inside) {
            gateLitsRemoved--;
            cl->add(gate.eqLit); //we can add, because we removed above. Otherwise this is segfault
            occur[gate.eqLit.toInt()].push(c);
        }

        //Clean the clause
        if (cleanClause(*c.clause)) {
            unlinkClause(c);
            c.clause = NULL;
            continue;
        }

        //Handle clause
        switch (c.clause->size()) {
            case 0:
                solver.ok = false;
                return false;
                break;
            case 1: {
                handleSize1Clause((*c.clause)[0]);
                unlinkClause(c);
                if (!solver.ok) return false;
                c.clause = NULL;
                break;
            }
            case 2: {
                solver.attachBinClause((*c.clause)[0], (*c.clause)[1], (*c.clause).learnt());
                solver.numNewBin++;
                solver.dataSync->signalNewBinClause(*c.clause);
                clBinTouched.push_back(NewBinaryClause((*c.clause)[0], (*c.clause)[1], (*c.clause).learnt()));
                unlinkClause(c);
                c.clause = NULL;
                break;
            }
            default:
                if (!cl->learnt()) {
                    for (Lit *i2 = cl->getData(), *end2 = cl->getDataEnd(); i2 != end2; i2++)
                    touchChangeVars(*i2);
                }
                cl_touched.add(c);
        }
    }

    return solver.ok;
}

const bool Subsumer::subsumeNonExist()
{
    double myTime = cpuTime();

    clauses_subsumed = 0;
    for (const ClauseSimp *it = clauses.getData(), *end = clauses.getDataEnd(); it != end; it++) {
        if (it->clause == NULL) continue;
        const Clause& cl = *it->clause;


        for (const Lit *l = cl.getData(), *end = cl.getDataEnd(); l != end; l++) {
            seen_tmp[l->toInt()] = true;
        }

        bool toRemove = false;
        for (const Lit *l = cl.getData(), *end = cl.getDataEnd(); l != end; l++) {
            vector<Lit>& cache = binNonLearntCache[l->toInt()].lits;
            for (vector<Lit>::const_iterator cacheLit = cache.begin(), endCache = cache.end(); cacheLit != endCache; cacheLit++) {
                if (seen_tmp[cacheLit->toInt()]) {
                    toRemove = true;
                    break;
                }
            }
            if (toRemove) break;
        }

        for (const Lit *l = cl.getData(), *end = cl.getDataEnd(); l != end; l++) {
            seen_tmp[l->toInt()] = false;
        }

        if (toRemove) {
            // std::cout << "cl-rem subs w/ non-existent: " << cl << std::endl;
            unlinkClause(*it);
        }
    }

    if (solver.conf.verbosity  >= 1) {
        std::cout << "c Subs w/ non-existent bins: " << std::setw(6) << clauses_subsumed
        << " time: " << std::fixed << std::setprecision(2) << std::setw(5) << (cpuTime() - myTime) << " s"
        << std::endl;
    }
    totalTime += cpuTime() - myTime;

    return true;
}