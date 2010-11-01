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
    typedef map<Var, vector<Clause*> > elimType;
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

        for (vector<Clause*>::iterator it2 = it->second.begin(), end2 = it->second.end(); it2 != end2; it2++) {
            Clause& c = **it2;
            tmp.clear();
            tmp.growTo(c.size());
            std::copy(c.getData(), c.getDataEnd(), tmp.getData());

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
    typedef map<Var, vector<Clause*> > elimType;
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
    for (vector<Clause*>::iterator itt = it->second.begin(), end2 = it->second.end(); itt != end2; itt++) {
        #ifdef VERBOSE_DEBUG
        std::cout << "Reinserting elimed clause: " << *itt << std::endl;;
        #endif
        solver.addClause(**itt);
        solver.clauseAllocator.clauseFree(*itt);
    }
    elimedOutVar.erase(it);

    for (vector<std::pair<Lit, Lit> >::iterator itt = it2->second.begin(), end2 = it2->second.end(); itt != end2; itt++) {
        vec<Lit> lits(2);
        lits[0] = itt->first;
        lits[1] = itt->second;
        #ifdef VERBOSE_DEBUG
        std::cout << "Reinserting bin clause: " << itt->first << " , " << itt->second << std::endl;
        #endif
        solver.addClause(lits);
    }
    elimedOutVarBin.erase(it2);

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
            if (ps.getGlue() > ret.activity)
                ps.setGlue(ret.activity);
            if (ps.getMiniSatAct() < ret.oldActivity)
                ps.setMiniSatAct(ret.oldActivity);
        } else {
            solver.nbCompensateSubsumer++;
            ps.makeNonLearnt();
        }
    }
}

/**
@brief Backward-subsumption using given clause

@note Use helper function

@param ps The clause to use to backward-subsume (doesn't handle learnt clauses)
@param[in] abs The abstraction of the clause
@return Subsumed anything? If so, what was the max activity? Was it non-learnt?
*/
template<class T>
Subsumer::subsume0Happened Subsumer::subsume0Orig(const T& ps, uint32_t abs)
{
    subsume0Happened ret;
    ret.subsumedNonLearnt = false;
    ret.activity = std::numeric_limits<uint32_t>::max();
    ret.oldActivity = std::numeric_limits< float >::min();

    vec<ClauseSimp> subs;
    findSubsumed(ps, abs, subs);
    for (uint32_t i = 0; i < subs.size(); i++){
        #ifdef VERBOSE_DEBUG
        cout << "-> subsume0 removing:";
        subs[i].clause->plainPrint();
        #endif

        Clause* tmp = subs[i].clause;
        if (tmp->learnt()) {
            solver.nbCompensateSubsumer++;
            ret.activity = std::min(ret.activity, tmp->getGlue());
            ret.oldActivity = std::max(ret.oldActivity, tmp->getMiniSatAct());
        } else {
            ret.subsumedNonLearnt = true;
        }
        unlinkClause(subs[i]);
    }

    return ret;
}
/**
@brief Backward-subsumption&self-subsuming resolution for binary clause sets

Takes in a set of binary clauses:
lit1 OR lits[0]
lit1 OR lits[1]
...
and backward-subsumes clauses in the occurence lists with it, as well as
performing self-subsuming resolution using these binary clauses on clauses in
the occurrence lists.

@param[in] lit1 As defined above
@param[in] lits The abstraction of the clause
*/
void Subsumer::subsume0BIN(const Lit lit1, const vec<char>& lits)
{
    vec<ClauseSimp> subs;
    vec<ClauseSimp> subs2;
    vec<Lit> subs2Lit;

    vec<ClauseSimp>& cs = occur[lit1.toInt()];
    for (ClauseSimp *it = cs.getData(), *end = it + cs.size(); it != end; it++){
        if (it+1 != end) __builtin_prefetch((it+1)->clause, 0, 1);
        if (it->clause == NULL) continue;

        Clause& c = *it->clause;
        extraTimeNonExist += c.size()*3;
        bool removed = false;
        for (uint32_t i = 0; i < c.size(); i++) {
            if (lits[c[i].toInt()]) {
                subs.push(*it);
                removed = true;
                break;
            }
        }
        if (removed) continue;

        for (uint32_t i = 0; i < c.size(); i++) {
            if (lits[(~c[i]).toInt()]) {
                subs2.push(*it);
                subs2Lit.push(c[i]);
                break;
            }
        }
    }

    for (uint32_t i = 0; i < subs.size(); i++){
        unlinkClause(subs[i]);
    }

    for (uint32_t i = 0; i < subs2.size(); i++) {
        strenghten(subs2[i], subs2Lit[i]);
        if (!solver.ok) break;
    }

    #ifdef VERBOSE_DEBUG
    if (!solver.ok) {
        std::cout << "solver.ok is false when returning from subsume0BIN()" << std::endl;
    }
    #endif //VERBOSE_DEBUG
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
    ps.unsetStrenghtened();
    for (uint32_t j = 0; j < subs.size(); j++) {
        if (subs[j].clause == NULL) continue;
        ClauseSimp c = subs[j];
        if (subsLits[j] == lit_Undef) {
            if (ps.learnt()) {
                if (c.clause->learnt()) {
                    if (c.clause->getGlue() < ps.getGlue())
                        ps.setGlue(c.clause->getGlue());
                    if (c.clause->getMiniSatAct() > ps.getMiniSatAct())
                        ps.setMiniSatAct(c.clause->getMiniSatAct());
                } else {
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
        maybeRemove(occur[cl[i].toInt()], &cl);
        #ifndef TOUCH_LESS
        touch(cl[i]);
        #endif
    }

    // Remove from iterator vectors/sets:
    for (uint32_t i = 0; i < iter_sets.size(); i++) {
        CSet& cs = *iter_sets[i];
        cs.exclude(c);
    }

    // Remove clause from clause touched set:
    cl_touched.exclude(c);

    if (elim != var_Undef) {
        assert(!cl.learnt());
        #ifdef VERBOSE_DEBUG
        std::cout << "Eliminating non-bin clause: " << *c.clause << std::endl;
        std::cout << "On variable: " << elim+1 << std::endl;
        #endif //VERBOSE_DEBUG
        elimedOutVar[elim].push_back(c.clause);
    } else {
        clauses_subsumed++;
        solver.clauseAllocator.clauseFree(c.clause);
    }

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
            #ifndef TOUCH_LESS
            touch(*i);
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
    #ifndef TOUCH_LESS
    touch(toRemoveLit);
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
            solver.signalNewBinClause(*c.clause);
            unlinkClause(c);
            c.clause = NULL;
            break;
        }
        default:
            cl_touched.add(c);
    }
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

    if (cl_touched.size() < clauses.size() / 2) {
        vec<char> ol_seenPos(solver.nVars()*2, 0);
        vec<char> ol_seenNeg(solver.nVars()*2, 0);
        for (CSet::iterator it = cl_touched.begin(), end = cl_touched.end(); it != end; ++it) {
            if (it->clause == NULL) continue;
            ClauseSimp& c = *it;
            Clause& cl = *it->clause;

            uint32_t smallestPosSize = std::numeric_limits<uint32_t>::max();
            Lit smallestPos = lit_Undef;
            s1.add(c);
            for (uint32_t j = 0; j < cl.size(); j++) {
                if (ol_seenPos[cl[j].toInt()] || smallestPos == lit_Error) {
                    smallestPos = lit_Error;
                    goto next;
                }
                if (occur[cl[j].toInt()].size() < smallestPosSize) {
                    smallestPos = cl[j];
                    smallestPosSize = occur[cl[j].toInt()].size();
                }

                next:
                if (ol_seenNeg[cl[j].toInt()]) continue;
                ol_seenNeg[cl[j].toInt()] = 1;

                vec<ClauseSimp>& n_occs = occur[(~cl[j]).toInt()];
                for (uint32_t k = 0; k < n_occs.size(); k++) s1.add(n_occs[k]);
            }

            if (smallestPos != lit_Undef && smallestPos != lit_Error) {
                ol_seenPos[smallestPos.toInt()] = 1;
                vec<ClauseSimp>& p_occs = occur[smallestPos.toInt()];
                for (uint32_t k = 0; k < p_occs.size(); k++) s0.add(p_occs[k]);
            }
        }
    } else {
        for (uint32_t i = 0; i < clauses.size(); i++) {
            if (clauses[i].clause != NULL) {
                s0.add(clauses[i]);
                s1.add(clauses[i]);
            }
        }
    }
    cl_touched.clear();

    registerIteration(s0);
    registerIteration(s1);

    // Fixed-point for 1-subsumption:
    do {
        for (CSet::iterator it = s0.begin(), end = s0.end(); it != end; ++it) {
            if (it->clause == NULL) continue;
            if (numMaxSubsume0 == 0) break;
            if (it->clause != NULL) {
                subsume0(*it->clause);
                numMaxSubsume0--;
            }
        }
        s0.clear();

        for (CSet::iterator it = s1.begin(), end = s1.end(); it != end; ++it) {
            if (numMaxSubsume1 == 0) break;
            if (it->clause != NULL) {
                subsume1(*it->clause);
                s0.exclude(*it);
                numMaxSubsume1--;
                if (!solver.ok) goto end;
            }
        }
        s1.clear();

        for (CSet::iterator it = cl_touched.begin(), end = cl_touched.end(); it != end; ++it) {
            if (it->clause != NULL) {
                s1.add(*it);
                s0.add(*it);
            }
        }
        cl_touched.clear();
    } while (s1.size() > 100 || s0.size() > 100);
    assert(cl_touched.size() == 0);

    end:
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
        touch(cl[i].var());
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
void Subsumer::addFromSolver(vec<Clause*>& cs, const bool alsoLearnt, const bool addBinAndAddToCL)
{
    Clause **i = cs.getData();
    Clause **j = i;
    for (Clause **end = i + cs.size(); i !=  end; i++) {
        if (i+1 != end)
            __builtin_prefetch(*(i+1), 1, 1);

        if (!alsoLearnt && (*i)->learnt()) {
            *j++ = *i;
            continue;
        }

        if (!addBinAndAddToCL && (*i)->size() == 2) {
            //don't add binary clauses in this case
            *j++ = *i;
            continue;
        }

        ClauseSimp c(*i, clauseID++);
        clauses.push(c);
        Clause& cl = *c.clause;
        for (uint32_t i = 0; i < cl.size(); i++) {
            occur[cl[i].toInt()].push(c);
            touch(cl[i]);
        }

        if (addBinAndAddToCL) {
            if (cl.getStrenghtened()) cl_touched.add(c);
        }
    }
    cs.shrink(i-j);
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

void Subsumer::removeWrongBins()
{
    uint32_t numRemovedHalfLearnt = 0;
    uint32_t wsLit = 0;
    for (vec<Watched> *it = solver.watches.getData(), *end = solver.watches.getDataEnd(); it != end; it++, wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        vec<Watched>& ws = *it;

        Watched* i = ws.getData();
        Watched* j = i;
        for (Watched *end2 = ws.getDataEnd(); i != end2; i++) {
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
    solver.clauses_literals -= numRemovedHalfLearnt;
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

@param onlyNonLearntBins This class is initialised before calling this function
and contains all the non-learnt binary clauses
*/
const bool Subsumer::subsumeWithBinaries()
{
    //Clearing stats
    double myTime = cpuTime();
    clauses_subsumed = 0;
    literals_removed = 0;
    uint32_t origTrailSize = solver.trail.size();

    vec<Lit> lits(2);
    uint32_t wsLit = 0;
    for (const vec<Watched> *it = solver.watches.getData(), *end = solver.watches.getDataEnd(); it != end; it++, wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        lits[0] = lit;
        const vec<Watched> ws_backup = *it;
        for (const Watched *it2 = ws_backup.getData(), *end2 = ws_backup.getDataEnd(); it2 != end2; it2++) {
            if (it2->isBinary() && lit.toInt() < it2->getOtherLit().toInt()) {
                lits[1] = it2->getOtherLit();
                bool toMakeNonLearnt = subsume1(lits, it2->getLearnt());
                if (toMakeNonLearnt) makeNonLearntBin(lit, it2->getOtherLit(), it2->getLearnt());
                if (!solver.ok) return false;
                numMaxSubsume0--;
            }
        }
        if (numMaxSubsume0 < 0) break;
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

const bool Subsumer::subsWNonExitsBinsFullFull()
{
    double myTime = cpuTime();
    clauses_subsumed = 0;
    literals_removed = 0;

    uint32_t oldTrailSize = solver.trail.size();
    if (!subsWNonExistBinsFull()) return false;

    if (solver.conf.verbosity  >= 1) {
        std::cout << "c Subs w/ non-existent bins: " << std::setw(6) << clauses_subsumed
        << " l-rem: " << std::setw(6) << literals_removed
        << " v-fix: " << std::setw(5) << solver.trail.size() - oldTrailSize
        << " done: " << std::setw(6) << doneNum
        << " time: " << std::fixed << std::setprecision(2) << std::setw(5) << (cpuTime() - myTime) << " s"
        << std::endl;
    }

    totalTime += cpuTime() - myTime;
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

@param onlyNonLearntBins This class is initialised before calling this function
and contains all the non-learnt binary clauses
*/
const bool Subsumer::subsWNonExistBinsFull()
{
    uint64_t oldProps = solver.propagations;
    uint64_t maxProp = MAX_BINARY_PROP;
    //if (clauses.size() > 2000000) maxProp /= 2;
    toVisitAll.growTo(solver.nVars()*2, false);
    extraTimeNonExist = 0;

    doneNum = 0;
    uint32_t startFrom = solver.mtrand.randInt(solver.order_heap.size());
    for (uint32_t i = 0; i < solver.order_heap.size(); i++) {
        Var var = solver.order_heap[(i+startFrom)%solver.order_heap.size()];
        if (solver.propagations + extraTimeNonExist - oldProps > maxProp) break;
        if (solver.assigns[var] != l_Undef || !solver.decision_var[var]) continue;
        doneNum++;
        extraTimeNonExist += 5;

        Lit lit(var, true);
        if (!subsWNonExistBins(lit)) {
            if (!solver.ok) return false;
            solver.cancelUntil(0);
            solver.uncheckedEnqueue(~lit);
            solver.ok = solver.propagate().isNULL();
            if (!solver.ok) return false;
            continue;
        }
        extraTimeNonExist += 10;

        //in the meantime it could have got assigned
        if (solver.assigns[var] != l_Undef) continue;
        lit = ~lit;
        if (!subsWNonExistBins(lit)) {
            if (!solver.ok) return false;
            solver.cancelUntil(0);
            solver.uncheckedEnqueue(~lit);
            solver.ok = solver.propagate().isNULL();
            if (!solver.ok) return false;
            continue;
        }
        extraTimeNonExist += 10;
    }

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
const bool Subsumer::subsWNonExistBins(const Lit& lit)
{
    #ifdef VERBOSE_DEBUG
    std::cout << "subsWNonExistBins called with lit " << lit << std::endl;
    #endif //VERBOSE_DEBUG
    toVisit.clear();
    solver.newDecisionLevel();
    solver.uncheckedEnqueueLight(lit);
    bool failed = (!solver.propagateBin(false).isNULL());
    if (failed) return false;

    assert(solver.decisionLevel() > 0);
    for (int c = solver.trail.size()-1; c > (int)solver.trail_lim[0]; c--) {
        Lit x = solver.trail[c];
        toVisit.push(x);
        toVisitAll[x.toInt()] = true;
    }
    solver.cancelUntil(0);

    if (toVisit.size() <= solver.getBinWatchSize(false, lit)) {
        //This has been performed above, with subsume1Partial of binary clauses:
        //this toVisit.size()<=1, there mustn't have been more than 1 binary
        //clause in the watchlist, so this has been performed above.
    } else {
        subsume0BIN(~lit, toVisitAll);
    }

    for (uint32_t i = 0; i < toVisit.size(); i++)
        toVisitAll[toVisit[i].toInt()] = false;

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
}

const bool Subsumer::eliminateVars()
{
    #ifdef BIT_MORE_VERBOSITY
    std::cout << "c VARIABLE ELIMINIATION -- touched_list size:" << touched_list.size() << std::endl;
    #endif
    vec<Var> init_order;
    orderVarsForElim(init_order);   // (will untouch all variables)

    for (bool first = true; numMaxElim > 0; first = false) {
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
            for (uint32_t i = 0; i < touchedVarsList.size(); i++) {
                const Var var = touchedVarsList[i];
                if (!cannot_eliminate[var] && solver.decision_var[var])
                    order.push(var);
                touchedVars[var] = false;
            }
            touchedVarsList.clear();
        }
        #ifdef VERBOSE_DEBUG
        std::cout << "Order size:" << order.size() << std::endl;
        #endif

        for (uint32_t i = 0; i < order.size() && numMaxElim > 0; i++, numMaxElim--) {
            if (maybeEliminate(order[i])) {
                if (!solver.ok) return false;
                vars_elimed++;
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
            assert(i->isBinary());
            if (i->getOtherLit() == lastLit) {
                //The sorting algorithm prefers non-learnt to learnt, so it is
                //impossible to have non-learnt before learnt
                assert(!(i->getLearnt() == false && lastLearnt == true));

                assert(i->getOtherLit().var() != lit.var());
                removeWBin(solver.watches[(~(i->getOtherLit())).toInt()], lit, i->getLearnt());
                if (i->getLearnt()) solver.learnts_literals -= 2;
                else {
                    solver.clauses_literals -= 2;
                    touch(lit);
                    touch(i->getOtherLit());
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

void Subsumer::addExternTouchVars()
{
    for (uint32_t i = 0; i < touchedVarsExtList.size(); i++) {
        Var var = touchedVarsExtList[i];
        touchedVarsExt[var] = false;
        touch(var);
    }
    touchedVarsExtList.clear();
}

void Subsumer::addRemainingTouchedToExt()
{
    for (uint32_t i = 0; i < touchedVarsList.size(); i++) {
        Var var = touchedVars[i];
        touchExternal(var);
    }
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
const bool Subsumer::simplifyBySubsumption(const bool alsoLearnt)
{
    if (solver.nClauses() > 20000000)  return true;

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

    if (alsoLearnt) {
        solver.clauseCleaner->cleanClauses(solver.learnts, ClauseCleaner::learnts);
        std::sort(solver.learnts.getData(), solver.learnts.getDataEnd(), sortBySize());
        addFromSolver(solver.learnts, alsoLearnt);
    }
    solver.clauseCleaner->cleanClauses(solver.clauses, ClauseCleaner::clauses);
    addFromSolver(solver.clauses, alsoLearnt);
    if (!alsoLearnt) addExternTouchVars();

    CompleteDetachReatacher reattacher(solver);
    reattacher.detachNonBins();
    totalTime += myTime - cpuTime();

    //Do stuff with binaries
    if (!alsoLearnt) {
        subsumeBinsWithBins();
        numMaxSubsume0 = 1000000*numCalls;
        if (solver.conf.doSubsWBins && !subsumeWithBinaries()) return false;
        if (solver.conf.doSubsWNonExistBins && !subsWNonExitsBinsFullFull()) return false;
    }

    myTime = cpuTime();
    setLimits(alsoLearnt);
    clauses_subsumed = 0;
    literals_removed = 0;
    numblockedClauseRemoved = 0;
    numVarsElimed = 0;
    blockTime = 0.0;
    uint32_t origTrailSize = solver.trail.size();

    for (uint32_t i = 0; i < clauses.size(); i++) {
        if (numMaxSubsume0 == 0) break;
        if (clauses[i].clause != NULL) {
            subsume0(*clauses[i].clause);
            numMaxSubsume0--;
        }
    }

    #ifdef BIT_MORE_VERBOSITY
    std::cout << "c  time until pre-subsume0 clauses and subsume1 2-learnts:" << cpuTime()-myTime << std::endl;
    std::cout << "c  pre-subsumed:" << clauses_subsumed << std::endl;
    std::cout << "c  cl_touched:" << cl_touched.size() << std::endl;
    std::cout << "c  clauses:" << clauses.size() << std::endl;
    #endif

    if (clauses.size() > 10000000 ||
        (numMaxSubsume1 == 0 && numMaxElim == 0 && numMaxBlockVars == 0))
        goto endSimplifyBySubsumption;

    //if (solver.doBlockedClause && numCalls % 3 == 1) blockedClauseRemoval();
    do {
        if (!subsume0AndSubsume1()) return false;

        if (!solver.conf.doVarElim) break;

        if (!eliminateVars()) return false;

        //subsumeBinsWithBins();
        //if (solver.conf.doSubsWBins && !subsumeWithBinaries()) return false;
        solver.clauseCleaner->removeSatisfiedBins();
    } while (cl_touched.size() > 100);
    endSimplifyBySubsumption:

    if (!solver.ok) return false;

    assert(verifyIntegrity());

    removeWrong(solver.learnts);
    removeWrongBins();
    removeAssignedVarsFromEliminated();
    if (!alsoLearnt) addRemainingTouchedToExt();

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

        if (numblockedClauseRemoved > 0 || blockTime > 0.1) {
            std::cout
            << "c Blocked clauses removed: " << std::setw(8) << numblockedClauseRemoved
            << "    Time: " << std::fixed << std::setprecision(2) << std::setw(4) << blockTime << " s"
            << std::endl;
        }
    }
    totalTime += cpuTime() - myTime;

    solver.testAllClauseAttach();
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
void Subsumer::setLimits(const bool alsoLearnt)
{
    if (clauses.size() > 3500000) {
        numMaxSubsume0 = 900000 * (1+numCalls/2);
        numMaxElim = (uint32_t)((double)solver.order_heap.size() / 6.0 * (0.8+(double)(numCalls)/4.0));
        numMaxSubsume1 = 100000 * (1+numCalls/2);
        numMaxBlockToVisit = (int64_t)(30000.0 * (0.8+(double)(numCalls)/3.0));
    }
    if (clauses.size() <= 3500000 && clauses.size() > 1500000) {
        numMaxSubsume0 = 2000000 * (1+numCalls/2);
        numMaxElim = (uint32_t)((double)solver.order_heap.size() / 3.0 * (0.8+(double)(numCalls)/4.0));
        numMaxSubsume1 = 300000 * (1+numCalls/2);
        numMaxBlockToVisit = (int64_t)(50000.0 * (0.8+(double)(numCalls)/3.0));
    }
    if (clauses.size() <= 1500000) {
        numMaxSubsume0 = 4000000 * (1+numCalls/2);
        numMaxElim = (uint32_t)((double)solver.order_heap.size() / 3.0 * (0.8+(double)(numCalls)/2.0));
        numMaxSubsume1 = 400000 * (1+numCalls/2);
        numMaxBlockToVisit = (int64_t)(80000.0 * (0.8+(double)(numCalls)/3.0));
    }
    if (solver.order_heap.size() > 200000)
        numMaxBlockVars = (uint32_t)((double)solver.order_heap.size() / 3.5 * (0.8+(double)(numCalls)/4.0));
    else
        numMaxBlockVars = (uint32_t)((double)solver.order_heap.size() / 1.5 * (0.8+(double)(numCalls)/4.0));

    if (!solver.conf.doSubsume1 || numCalls == 1)
        numMaxSubsume1 = 0;
    if (alsoLearnt) {
        numMaxElim = 0;
        numMaxSubsume1 = std::min(numMaxSubsume1, (uint32_t)10000);
        numMaxBlockVars = 0;
        numMaxBlockToVisit = 0;
    } else {
        numCalls++;
    }

    //For debugging
    //numMaxBlockToVisit = std::numeric_limits<int64_t>::max();
    //numMaxElim = std::numeric_limits<uint32_t>::max();
    //numMaxSubsume0 = std::numeric_limits<uint32_t>::max();
    //numMaxSubsume1 = std::numeric_limits<uint32_t>::max();
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
            map<Var, vector<Clause*> >::iterator it = elimedOutVar.find(var);
            if (it != elimedOutVar.end()) {
                for (uint32_t i = 0; i < it->second.size(); i++) {
                    solver.clauseAllocator.clauseFree(it->second[i]);
                }
                elimedOutVar.erase(it);
            }
            map<Var, vector<std::pair<Lit, Lit> > >::iterator it2 = elimedOutVarBin.find(var);
            if (it2 != elimedOutVarBin.end()) {
                elimedOutVarBin.erase(it2);
            }
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
    for (ClauseSimp *it = cs.getData(), *end = it + cs.size(); it != end; it++){
        if (it+1 != end)
            __builtin_prefetch((it+1)->clause, 1, 1);

        if (it->clause != (Clause*)&ps
            && subsetAbst(abs, it->clause->getAbst())
            && ps.size() <= it->clause->size()
            && subset(ps.size(), *it->clause)) {
            out_subsumed.push(*it);
            #ifdef VERBOSE_DEBUG
            cout << "subsumed: ";
            it->clause->plainPrint();
            #endif
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
            touch(c.lit1);
            touch(c.lit2);
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
        assert(it->isBinary());
        if (!it->getLearnt()) num++;
    }

    return num;
}

void Subsumer::fillClAndBin(vec<ClAndBin>& all, vec<ClauseSimp>& cs, const Lit lit)
{
    for (uint32_t i = 0; i < cs.size(); i++)
        all.push(ClAndBin(cs[i]));

    const vec<Watched>& ws = solver.watches[(~lit).toInt()];
    for (const Watched *it = ws.getData(), *end = ws.getDataEnd(); it != end; it++) {
        assert(it->isBinary());
        if (!it->getLearnt()) all.push(ClAndBin(lit, it->getOtherLit()));
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

    // Heuristic CUT OFF:
    if (posSize >= 10 && negSize >= 10) return false;

    // Count clauses/literals before elimination:
    uint32_t before_literals = numNonLearntNeg*2 + numNonLearntPos*2;
    for (uint32_t i = 0; i < poss.size(); i++) before_literals += poss[i].clause->size();
    for (uint32_t i = 0; i < negs.size(); i++) before_literals += negs[i].clause->size();

    // Heuristic CUT OFF2:
    if ((posSize >= 3 && negSize >= 3 && before_literals > 300)
        && clauses.size() > 1500000)
        return false;
    if ((posSize >= 5 && negSize >= 5 && before_literals > 400)
        && clauses.size() <= 1500000 && clauses.size() > 200000)
        return false;
    if ((posSize >= 8 && negSize >= 8 && before_literals > 700)
        && clauses.size() <= 200000)
        return false;

    vec<ClAndBin> posAll, negAll;
    fillClAndBin(posAll, poss, lit);
    fillClAndBin(negAll, negs, ~lit);
    assert(posAll.size() == posSize);
    assert(negAll.size() == negSize);

    // Count clauses/literals after elimination:
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
    poss.clear();
    negs.clear();
    removeClauses(posAll, negAll, var);

    #ifndef NDEBUG
    for (uint32_t i = 0; i < solver.watches[lit.toInt()].size(); i++) {
        Watched& w = solver.watches[lit.toInt()][i];
        assert(w.isBinary() && w.getLearnt());
    }
    for (uint32_t i = 0; i < solver.watches[(~lit).toInt()].size(); i++) {
        Watched& w = solver.watches[(~lit).toInt()][i];
        assert(w.isBinary() && w.getLearnt());
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
                    solver.signalNewBinClause(dummy);
                }
                subsume1(dummy, false);
                break;
            }
            default: {
                Clause* cl = solver.clauseAllocator.Clause_new(dummy, group_num);
                ClauseSimp c = linkInClause(*cl);
                subsume1(*c.clause);
            }
        }
        if (!solver.ok) return true;
    }

    assert(occur[lit.toInt()].size() == 0 &&  occur[(~lit).toInt()].size() == 0);
    var_elimed[var] = true;
    numElimed++;
    solver.setDecisionVar(var, false);
    return true;
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
        uint32_t pos = occur[x.toInt()].size() + solver.watches[(~x).toInt()].size();
        uint32_t neg = occur[x.toInt()].size() + solver.watches[x.toInt()].size();
        cost_var.push(std::make_pair(pos * neg, x.var()));
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

const bool Subsumer::allTautology(const vec<Lit>& ps, const Lit lit)
{
    #ifdef VERBOSE_DEBUG
    cout << "allTautology: ";
    for (uint32_t i = 0; i < ps.size(); i++) {
        if (ps[i].sign()) printf("-");
        printf("%d ", ps[i].var() + 1);
    }
    printf("0\n");
    #endif

    vec<ClauseSimp>& cs = occur[lit.toInt()];
    if (cs.size() == 0) return true;

    for (const Lit *l = ps.getData(), *end = ps.getDataEnd(); l != end; l++) {
        seen_tmp[l->toInt()] = true;
    }

    bool allIsTautology = true;
    for (ClauseSimp *it = cs.getData(), *end = cs.getDataEnd(); it != end; it++){
        if (it+1 != end)
            __builtin_prefetch((it+1)->clause, 1, 1);

        Clause& c = *it->clause;
        for (Lit *l = c.getData(), *end2 = l + c.size(); l != end2; l++) {
            if (*l != lit && seen_tmp[(~(*l)).toInt()]) {
                goto next;
            }
        }
        allIsTautology = false;
        break;

        next:;
    }

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
    vec<ClauseSimp> toRemove;

    touchedBlockedVars = priority_queue<VarOcc, vector<VarOcc>, MyComp>();
    touchedBlockedVarsBool.clear();
    touchedBlockedVarsBool.growTo(solver.nVars(), false);
    for (uint32_t i =  0; i < solver.order_heap.size() && i < numMaxBlockVars; i++) {
        if (solver.order_heap.size() < 1) break;
        touchBlockedVar(solver.order_heap[solver.mtrand.randInt(solver.order_heap.size()-1)]);
    }

    while (touchedBlockedVars.size() > 100  && numMaxBlockToVisit > 0) {
        VarOcc vo = touchedBlockedVars.top();
        touchedBlockedVars.pop();

        if (solver.assigns[vo.var] != l_Undef || !solver.decision_var[vo.var] || cannot_eliminate[vo.var])
            continue;
        touchedBlockedVarsBool[vo.var] = false;
        Lit lit = Lit(vo.var, false);
        Lit negLit = Lit(vo.var, true);

        numMaxBlockToVisit -= (int64_t)occur[lit.toInt()].size();
        numMaxBlockToVisit -= (int64_t)occur[negLit.toInt()].size();
        //if (!tryOneSetting(lit, negLit)) {
            tryOneSetting(negLit, lit);
       // }
    }
    blockTime += cpuTime() - myTime;

    #ifdef BIT_MORE_VERBOSITY
    std::cout << "c  Total fime for block until now: " << blockTime << std::endl;
    #endif
}

const bool Subsumer::tryOneSetting(const Lit lit, const Lit negLit)
{
    uint32_t toRemove = 0;
    bool returnVal = false;
    vec<Lit> cl;

    for(ClauseSimp *it = occur[lit.toInt()].getData(), *end = occur[lit.toInt()].getDataEnd(); it != end; it++) {
        cl.clear();
        cl.growTo(it->clause->size()-1);
        for (uint32_t i = 0, i2 = 0; i < it->clause->size(); i++) {
            if ((*it->clause)[i] != lit) {
                cl[i2] = (*it->clause)[i];
                i2++;
            }
        }

        if (allTautology(cl, negLit)) {
            toRemove++;
        } else {
            break;
        }
    }

    if (toRemove == occur[lit.toInt()].size()) {
        var_elimed[lit.var()] = true;
        solver.setDecisionVar(lit.var(), false);
        vec<ClauseSimp> toRemove(occur[lit.toInt()]);
        for (ClauseSimp *it = toRemove.getData(), *end = toRemove.getDataEnd(); it != end; it++) {
            #ifdef VERBOSE_DEBUG
            std::cout << "Next varelim because of block clause elim" << std::endl;
            #endif //VERBOSE_DEBUG
            unlinkClause(*it, lit.var());
            numblockedClauseRemoved++;
        }

        vec<ClauseSimp> toRemove2(occur[negLit.toInt()]);
        for (ClauseSimp *it = toRemove2.getData(), *end = toRemove2.getDataEnd(); it != end; it++) {
            #ifdef VERBOSE_DEBUG
            std::cout << "Next varelim because of block clause elim" << std::endl;
            #endif //VERBOSE_DEBUG
            unlinkClause(*it, lit.var());
            numblockedClauseRemoved++;
        }
        returnVal = true;
    }

    return returnVal;
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
