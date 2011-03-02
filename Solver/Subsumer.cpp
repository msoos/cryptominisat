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
#include "UselessBinRemover.h"
#include "DataSync.h"
#include <set>
#include "PartHandler.h"

#ifdef _MSC_VER
#define __builtin_prefetch
#endif //_MSC_VER

//#define VERBOSE_DEBUG
#ifdef VERBOSE_DEBUG
#define BIT_MORE_VERBOSITY
#define VERBOSE_ORGATE_REPLACE
#endif

//#define BIT_MORE_VERBOSITY
//#define TOUCH_LESS
//#define VERBOSE_ORGATE_REPLACE


#ifdef VERBOSE_DEBUG
using std::cout;
using std::endl;
#endif //VERBOSE_DEBUG

Subsumer::Subsumer(Solver& s):
    solver(s)
    , totalTime(0.0)
    , numElimed(0)
    , numERVars(0)
    , finishedAddingVars(false)
    , numCalls(0)
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
            std::cout << "Reinserting elimed clause: " << tmp << std::endl;;
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
    assert(solver.varData[var].elimed == ELIMED_VARELIM);
    vec<Lit> tmp;
    typedef map<Var, vector<vector<Lit> > > elimType;
    typedef map<Var, vector<std::pair<Lit, Lit> > > elimType2;
    elimType::iterator it = elimedOutVar.find(var);
    elimType2::iterator it2 = elimedOutVarBin.find(var);

    //it MUST have been decision var, otherwise we would
    //never have removed it
    solver.setDecisionVar(var, true);
    var_elimed[var] = false;
    solver.varData[var].elimed = ELIMED_NONE;
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
        std::cout << "Reinserting elimed clause: ";
        for (uint32_t i = 0; i < itt->size(); i++) {
            std::cout << (*itt)[i] << " , ";
        }
        std::cout << std::endl;
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
void Subsumer::subsume0(ClauseSimp c, Clause& cl)
{
    #ifdef VERBOSE_DEBUG
    cout << "subsume0-ing with clause: ";
    ps.plainPrint();
    #endif
    Sub0Ret ret = subsume0(c.index, cl, clauseData[c.index].abst);

    if (cl.learnt()) {
        if (!ret.subsumedNonLearnt) {
            if (cl.getGlue() > ret.glue)
                cl.setGlue(ret.glue);
        } else {
            solver.nbCompensateSubsumer++;
            cl.makeNonLearnt();
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
Subsumer::Sub0Ret Subsumer::subsume0(const uint32_t index, const T& ps, uint32_t abs)
{
    Sub0Ret ret;
    ret.subsumedNonLearnt = false;
    ret.glue = std::numeric_limits<uint32_t>::max();

    vec<ClauseSimp> subs;
    findSubsumed(index, ps, abs, subs);
    for (uint32_t i = 0; i < subs.size(); i++){
        #ifdef VERBOSE_DEBUG
        cout << "-> subsume0 removing:";
        subs[i].clause->plainPrint();
        #endif

        Clause& tmp = *clauses[subs[i].index];
        if (tmp.learnt()) {
            ret.glue = std::min(ret.glue, tmp.getGlue());
        } else {
            ret.subsumedNonLearnt = true;
        }
        unlinkClause(subs[i], tmp);
    }

    return ret;
}

/**
@brief Backward subsumption and self-subsuming resolution

Performs backward subsumption AND
self-subsuming resolution using backward-subsumption

@param[in] ps The clause to use for backw-subsumption and self-subs. resolution
*/
void Subsumer::subsume1(ClauseSimp c, Clause& ps)
{
    vec<ClauseSimp>    subs;
    vec<Lit>           subsLits;
    #ifdef VERBOSE_DEBUG
    cout << "subsume1-ing with clause:";
    ps.plainPrint();
    #endif

    findSubsumed1(c.index, ps, clauseData[c.index].abst, subs, subsLits);
    for (uint32_t j = 0; j < subs.size(); j++) {
        ClauseSimp c = subs[j];
        Clause& cl = *clauses[c.index];
        if (subsLits[j] == lit_Undef) {
            if (ps.learnt()) {
                if (cl.learnt()) ps.takeMaxOfStats(cl);
                else {
                    solver.nbCompensateSubsumer++;
                    ps.makeNonLearnt();
                }
            }
            unlinkClause(c, cl);
        } else {
            strenghten(c, cl, subsLits[j]);
            if (!solver.ok) return;
        }
    }
}

const bool Subsumer::subsume1(vec<Lit>& ps, const bool wasLearnt)
{
    vec<ClauseSimp>    subs;
    vec<Lit>           subsLits;
    bool toMakeNonLearnt = false;

    findSubsumed1(std::numeric_limits< uint32_t >::max(), ps, calcAbstraction(ps), subs, subsLits);
    for (uint32_t j = 0; j < subs.size(); j++) {
        ClauseSimp c = subs[j];
        Clause& cl = *clauses[c.index];
        if (subsLits[j] == lit_Undef) {
            if (wasLearnt && !cl.learnt()) toMakeNonLearnt = true;
            unlinkClause(c, cl);
        } else {
            strenghten(c, cl, subsLits[j]);
            if (!solver.ok) return false;
        }
    }

    return toMakeNonLearnt;
}

/**
@brief Removes&free-s a clause from everywhere

Removes clause from occurence lists, from Subsumer::clauses

If clause is to be removed because the variable in it is eliminated, the clause
is saved in elimedOutVar[] before it is fully removed.

@param[in] c The clause to remove
@param[in] elim If the clause is removed because of variable elmination, this
parameter is different from var_Undef.
*/
void Subsumer::unlinkClause(ClauseSimp c, Clause& cl, const Var elim)
{
    assert(!clauseData[c.index].defOfOrGate);

    for (uint32_t i = 0; i < cl.size(); i++) {
        if (elim != var_Undef) numMaxElim -= occur[cl[i].toInt()].size()/2;
        else {
            numMaxSubsume0 -= occur[cl[i].toInt()].size()/2;
            numMaxSubsume1 -= occur[cl[i].toInt()].size()/2;
        }
        maybeRemove(occur[cl[i].toInt()], c);
        touchedVars.touch(cl[i], cl.learnt());
    }

    // Remove from iterator vectors/sets:
    s0.exclude(c);
    s1.exclude(c);
    cl_touched.exclude(c);

    //Compensate if removing learnt
    if (cl.learnt()) solver.nbCompensateSubsumer++;

    if (elim != var_Undef) {
        assert(!cl.learnt());
        #ifdef VERBOSE_DEBUG
        std::cout << "Eliminating non-bin clause: " << *c.clause << std::endl;
        std::cout << "On variable: " << elim+1 << std::endl;
        #endif //VERBOSE_DEBUG
        vector<Lit> lits(cl.size());
        std::copy(cl.getData(), cl.getDataEnd(), lits.begin());
        elimedOutVar[elim].push_back(lits);
    } else {
        clauses_subsumed++;
        solver.clauseAllocator.clauseFree(&cl);
        clauses[c.index] = NULL;
    }
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
const bool Subsumer::cleanClause(ClauseSimp& c, Clause& ps)
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
            removeW(occur[i->toInt()], c);
            numMaxSubsume1 -= occur[i->toInt()].size()/2;
            touchedVars.touch(*i, ps.learnt());
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

/**
@brief Removes a literal from a clause

May return with solver.ok being FALSE, and may set&propagate variable values.

@param c Clause to be cleaned of the literal
@param[in] toRemoveLit The literal to be removed from the clause
*/
void Subsumer::strenghten(ClauseSimp& c, Clause& cl, const Lit toRemoveLit)
{
    #ifdef VERBOSE_DEBUG
    cout << "-> Strenghtening clause :";
    c.clause->plainPrint();
    cout << " with lit: " << toRemoveLit << std::endl;
    #endif

    literals_removed++;
    cl.strengthen(toRemoveLit);
    removeW(occur[toRemoveLit.toInt()], c);
    numMaxSubsume1 -= occur[toRemoveLit.toInt()].size()/2;
    touchedVars.touch(toRemoveLit, cl.learnt());

    handleUpdatedClause(c, cl);
}

const bool Subsumer::handleUpdatedClause(ClauseSimp& c, Clause& cl)
{
    if (cleanClause(c, cl)) {
        unlinkClause(c, cl);
        c.index = NULL;
        return true;
    }

    switch (cl.size()) {
        case 0:
            #ifdef VERBOSE_DEBUG
            std::cout << "Strenghtened clause to 0-size -> UNSAT"<< std::endl;
            #endif //VERBOSE_DEBUG
            solver.ok = false;
            return false;
        case 1: {
            solver.uncheckedEnqueue(cl[0]);
            unlinkClause(c, cl);
            c.index = std::numeric_limits< uint32_t >::max();
            return solver.ok;
        }
        case 2: {
            solver.attachBinClause(cl[0], cl[1], cl.learnt());
            solver.numNewBin++;
            solver.dataSync->signalNewBinClause(cl);
            clBinTouched.push_back(NewBinaryClause(cl[0], cl[1], cl.learnt()));
            unlinkClause(c, cl);
            c.index = std::numeric_limits< uint32_t >::max();
            return solver.ok;
        }
        default:
            if (cl.size() == 3) solver.dataSync->signalNewTriClause(cl);
            cl_touched.add(c);
    }
    clauseData[c.index] = AbstData(cl, clauseData[c.index].defOfOrGate);

    return true;
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
@brief Executes subsume1() recursively on all clauses

This function requires cl_touched to have been set. Then, it manages cl_touched.
The clauses are called to perform subsume1() or subsume0() when appropriate, and
when there is enough numMaxSubume1 and numMaxSubume0 is available.
*/
const bool Subsumer::subsume0AndSubsume1()
{
    s0.clear();
    s1.clear();
    //uint32_t clTouchedTodo = cl_touched.nElems();

    uint32_t clTouchedTodo = 10000;
    if (addedClauseLits > 3000000) clTouchedTodo /= 2;
    if (addedClauseLits > 10000000) clTouchedTodo /= 2;

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
        const bool doSubs1Next = (numMaxSubsume1 > 0);
        for (CSet::iterator it = cl_touched.begin(), end = cl_touched.end(); it != end; ++it) {
            if (it->index == std::numeric_limits< uint32_t >::max()) continue;
            Clause& cl = *clauses[it->index];

            if (s1Added >= clTouchedTodo) break;
            s0.add(*it);
            s1Added += s1.add(*it);

            for (uint32_t j = 0; j < cl.size(); j++) {
                if (!ol_seenPos[cl[j].toInt()]) {
                    vec<ClauseSimp>& occs = occur[(cl[j]).toInt()];
                    for (uint32_t k = 0; k < occs.size(); k++) {
                        s0.add(occs[k]);
                    }
                }
                ol_seenPos[cl[j].toInt()] = 1;

                if (!ol_seenNeg[(~cl[j]).toInt()]) {
                    vec<ClauseSimp>& occs = occur[(~cl[j]).toInt()];
                    for (uint32_t k = 0; k < occs.size(); k++) {
                        s1Added += s1.add(occs[k]);
                    }
                }
                ol_seenNeg[(~cl[j]).toInt()] = 1;
            }

            remClTouched.push(*it);
            cl.unsetStrenghtened();
            if (doSubs1Next) cl.unsetChanged();
        }
        //std::cout << "s0.nElems(): " << s0.nElems() << std::endl;
        //std::cout << "s1.nElems(): " << s1.nElems() << std::endl;

        for (uint32_t i = 0; i < remClTouched.size(); i++) {
            cl_touched.exclude(remClTouched[i]);
        }
        remClTouched.clear();

        for (CSet::iterator it = s0.begin(), end = s0.end(); it != end; ++it) {
            if (it->index == std::numeric_limits< uint32_t >::max()) continue;
            subsume0(it->index, *clauses[it->index]);
            //if (!doneAll && numMaxSubsume0 < 0) break;
        }
        s0.clear();

        if (doSubs1Next) {
            for (CSet::iterator it = s1.begin(), end = s1.end(); it != end; ++it) {
                if (it->index == std::numeric_limits< uint32_t >::max()) continue;
                subsume1(*it, *clauses[it->index]);
                if (!solver.ok) goto end;
            }
            s1.clear();
        }

        if (!handleClBinTouched()) goto end;
    } while ((cl_touched.nElems() > 100) && numMaxSubsume0 > 0);
    end:

    return solver.ok;
}

/**
@brief Links in a clause into the occurrence lists and the clauses[]

Increments clauseID

@param[in] cl The clause to link in
*/
ClauseSimp Subsumer::linkInClause(Clause& cl)
{
    ClauseSimp c(clauses.size());
    clauses.push(&cl);
    clauseData.push(AbstData(cl, false));
    assert(clauseData.size() == clauses.size());

    for (uint32_t i = 0; i < cl.size(); i++) {
        occur[cl[i].toInt()].push(c);
        touchedVars.touch(cl[i], cl.learnt());
        if (cl.getChanged()) {
            ol_seenPos[cl[i].toInt()] = 0;
            ol_seenNeg[(~cl[i]).toInt()] = 0;
        }
    }
    if (cl.getStrenghtened() || cl.getChanged())
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
        if (i+1 != end) __builtin_prefetch(*(i+1));

        ClauseSimp c = linkInClause(**i);
        numLitsAdded += (*i)->size();
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
        if (clauses[i] == NULL) continue;
        assert(clauses[i]->size() > 2);

        if (clauses[i]->learnt())
            solver.learnts.push(clauses[i]);
        else
            solver.clauses.push(clauses[i]);
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
    for (vec2<Watched> *it = solver.watches.getData(), *end = solver.watches.getDataEnd(); it != end; it++, wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        vec2<Watched>& ws = *it;

        vec2<Watched>::iterator i = ws.getData();
        vec2<Watched>::iterator j = i;
        for (vec2<Watched>::iterator end2 = ws.getDataEnd(); i != end2; i++) {
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
    for (const vec2<Watched> *it = solver.watches.getData(); counter != solver.nVars()*2; counter++) {
        uint32_t wsLit = (counter + thisRand) % (solver.nVars()*2);
        Lit lit = ~Lit::toLit(wsLit);
        lits[0] = lit;
        const vec2<Watched> ws_backup = *(it + wsLit);
        for (vec2<Watched>::const_iterator it2 = ws_backup.getData(), end2 = ws_backup.getDataEnd(); it2 != end2; it2++) {
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

/**
@brief Clears and deletes (almost) everything in this class

Clears touchlists, occurrance lists, clauses, and variable touched lists
*/
void Subsumer::clearAll()
{
    touchedVars.clear();
    clauses.clear();
    cl_touched.clear();
    addedClauseLits = 0;
    for (Var var = 0; var < solver.nVars(); var++) {
        occur[2*var].clear();
        occur[2*var+1].clear();
        ol_seenNeg[2*var    ] = 1;
        ol_seenNeg[2*var + 1] = 1;
        ol_seenPos[2*var    ] = 1;
        ol_seenPos[2*var + 1] = 1;
    }
    clauses.clear();
    clauseData.clear();
    cl_touched.clear();
    addedClauseLits = 0;
    numLearntBinVarRemAdded = 0;
}

const bool Subsumer::eliminateVars()
{
    #ifdef BIT_MORE_VERBOSITY
    std::cout << "c VARIABLE ELIMINIATION -- touchedVars size:" << touchedVars.size() << std::endl;
    #endif
    uint32_t vars_elimed = 0;

    vec<Var> order;
    orderVarsForElim(order);

    #ifdef VERBOSE_DEBUG
    std::cout << "c #order size:" << order.size() << std::endl;
    #endif

    uint32_t numtry = 0;
    for (uint32_t i = 0; i < order.size() && numMaxElim > 0 && numMaxElimVars > 0; i++) {
        Var var = order[i];
        if (!cannot_eliminate[var] && solver.decision_var[var]) {
            numtry++;
            if (maybeEliminate(order[i])) {
                if (!solver.ok) return false;
                vars_elimed++;
                numMaxElimVars--;
            }
        }
    }
    numVarsElimed += vars_elimed;

    #ifdef BIT_MORE_VERBOSITY
    std::cout << "c  #try to eliminate: " << numtry << std::endl;
    std::cout << "c  #var-elim: " << vars_elimed << std::endl;
    #endif

    return true;
}

void Subsumer::subsumeBinsWithBins()
{
    double myTime = cpuTime();
    uint32_t numBinsBefore = solver.numBins;

    uint32_t wsLit = 0;
    for (vec2<Watched> *it = solver.watches.getData(), *end = solver.watches.getDataEnd(); it != end; it++, wsLit++) {
        vec2<Watched>& ws = *it;
        Lit lit = ~Lit::toLit(wsLit);
        if (ws.size() < 2) continue;

        std::sort(ws.getData(), ws.getDataEnd(), BinSorter());

        vec2<Watched>::iterator i = ws.getData();
        vec2<Watched>::iterator j = i;

        Lit lastLit = lit_Undef;
        bool lastLearnt = false;
        for (vec2<Watched>::iterator end = ws.getDataEnd(); i != end; i++) {
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
                    touchedVars.touch(lit, i->getLearnt());
                    touchedVars.touch(i->getOtherLit(), i->getLearnt());
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

*/
const bool Subsumer::simplifyBySubsumption()
{
    if (solver.nClauses() > 50000000
        || solver.clauses_literals > 500000000)  return true;

    double myTime = cpuTime();
    clearAll();
    numCalls++;

    //touch all variables
    for (Var var = 0; var < solver.nVars(); var++) {
        if (solver.decision_var[var] && solver.assigns[var] == l_Undef) touchedVars.touch(var);
    }

    //if (solver.xorclauses.size() < 30000 && solver.clauses.size() < MAX_CLAUSENUM_XORFIND/10) addAllXorAsNorm();

    if (solver.conf.doReplace && !solver.varReplacer->performReplace(true))
        return false;
    fillCannotEliminate();

    uint32_t expected_size = solver.clauses.size() + solver.learnts.size();
    if (expected_size > 10000000 ||
        (numMaxSubsume1 < 0 && numMaxElim == 0 && numMaxBlockVars == 0))
        return solver.ok;

    clauses.reserve(expected_size);
    cl_touched.reserve(expected_size);

    solver.clauseCleaner->cleanClauses(solver.clauses, ClauseCleaner::clauses);
    solver.clauseCleaner->cleanClauses(solver.learnts, ClauseCleaner::learnts);

    if (solver.clauses.size() < 10000000)
        std::sort(solver.clauses.getData(), solver.clauses.getDataEnd(), sortBySize());
    addedClauseLits += addFromSolver(solver.clauses);

    if (solver.learnts.size() < 300000)
        std::sort(solver.learnts.getData(), solver.learnts.getDataEnd(), sortBySize());
    addedClauseLits += addFromSolver(solver.learnts);

    CompleteDetachReatacher reattacher(solver);
    reattacher.detachNonBinsNonTris(false);
    totalTime += myTime - cpuTime();

    //Do stuff with binaries
    subsumeBinsWithBins();
    numMaxSubsume1 = 500*1000*1000;
    if (solver.conf.doSubsWBins && !subsumeWithBinaries()) return false;
    if ((solver.conf.doBlockedClause)
        && solver.conf.doVarElim) {
        numMaxBlockToVisit = (int64_t)800*1000*1000;
        blockedClauseRemoval();
    }

    if (solver.conf.doReplace && solver.conf.doRemUselessBins) {
        UselessBinRemover uselessBinRemover(solver);
        if (!uselessBinRemover.removeUslessBinFull()) return false;
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

    do {
        if (!subsume0AndSubsume1()) return false;

        if (!solver.conf.doVarElim) break;

        if (!eliminateVars()) return false;

        //subsumeBinsWithBins();
        //if (solver.conf.doSubsWBins && !subsumeWithBinaries()) return false;
        solver.clauseCleaner->removeSatisfiedBins();
    } while (cl_touched.nElems() > 100);

    if (solver.conf.doGateFind
        && solver.conf.doCacheNLBins
        && numCalls > 3
        && !findOrGatesAndTreat()) return false;

    assert(solver.ok);
    assert(verifyIntegrity());

    removeWrong(solver.learnts);
    removeWrongBinsAndAllTris();
    removeAssignedVarsFromEliminated();

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

        std::cout << "c learnt bin added due to v-elim: " << numLearntBinVarRemAdded << std::endl;
    }
    totalTime += cpuTime() - myTime;

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
*/
void Subsumer::setLimits()
{
    numMaxSubsume0 = 130*1000*1000;
    numMaxSubsume1 = 20*1000*1000;
    numMaxSubsume0 *= 3;

    numMaxElim = 100*1000*1000;

    //numMaxElim = 0;
    //numMaxElim = std::numeric_limits<int64_t>::max();

    #ifdef BIT_MORE_VERBOSITY
    std::cout << "c addedClauseLits: " << addedClauseLits << std::endl;
    #endif
    if (addedClauseLits < 10000000) {
        numMaxElim *= 3;
        numMaxSubsume0 *= 3;
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
        numMaxElim *= 3;
        numMaxSubsume0 *= 3;
        numMaxSubsume1 *= 3;
    }

    numMaxElimVars = (solver.order_heap.size()/3)*numCalls;

    numMaxBlockVars = (uint32_t)((double)solver.order_heap.size() / 1.5 * (0.8+(double)(numCalls)/4.0));

    if (!solver.conf.doSubsume1)
        numMaxSubsume1 = 0;

    if (numCalls == 1) {
        numMaxSubsume1 = 3*1000*1000;
    }

    numCalls++;

    //For debugging

    //numMaxSubsume0 = 0;
    //numMaxSubsume1 = 0;
    //numMaxSubsume0 = std::numeric_limits<int64_t>::max();
    //numMaxSubsume1 = std::numeric_limits<int64_t>::max();
    //numMaxElimVars = std::numeric_limits<int32_t>::max();
    //numMaxElim     = std::numeric_limits<int64_t>::max();

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
            assert(solver.varData[var].elimed == ELIMED_VARELIM);
            var_elimed[var] = false;
            solver.varData[var].elimed = ELIMED_NONE;
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
void Subsumer::findSubsumed(const uint32_t index, const T& ps, uint32_t abs, vec<ClauseSimp>& out_subsumed)
{
    #ifdef VERBOSE_DEBUG
    cout << "findSubsumed: ";
    for (uint32_t i = 0; i < ps.size(); i++) {
        std::cout << ps[i] << " , ";
    }
    std::cout << std::endl;
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
        if (it->index != index
            && subsetAbst(abs, clauseData[it->index].abst)
            && ps.size() <= clauseData[it->index].size) {
                numMaxSubsume0 -= clauseData[it->index].size + ps.size();
                if (subset(ps.size(), *clauses[it->index])) {
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
void Subsumer::findSubsumed1(uint32_t index, const T& ps, uint32_t abs, vec<ClauseSimp>& out_subsumed, vec<Lit>& out_lits)
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
    fillSubs(ps, index, abs, out_subsumed, out_lits, Lit(minVar, true));
    fillSubs(ps, index, abs, out_subsumed, out_lits, Lit(minVar, false));
}

/**
@brief Helper function for findSubsumed1

Used to avoid duplication of code
*/
template<class T>
void inline Subsumer::fillSubs(const T& ps, const uint32_t index, uint32_t abs, vec<ClauseSimp>& out_subsumed, vec<Lit>& out_lits, const Lit lit)
{
    Lit litSub;
    vec<ClauseSimp>& cs = occur[lit.toInt()];
    for (ClauseSimp *it = cs.getData(), *end = it + cs.size(); it != end; it++) {
        if (it->index != index
            && subsetAbst(abs, clauseData[it->index].abst)
            && ps.size() <= clauseData[it->index].size) {
            numMaxSubsume1 -= clauseData[it->index].size + ps.size();
            litSub = subset1(ps, *clauses[it->index]);
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
            if (clauses[c.clsimp.index] != NULL)
                unlinkClause(c.clsimp, *clauses[c.clsimp.index], var);
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
            touchedVars.touch(c.lit1, false);
            touchedVars.touch(c.lit2, false);
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
    const vec2<Watched>& ws = solver.watches[(~lit).toInt()];
    for (vec2<Watched>::const_iterator it = ws.getData(), end = ws.getDataEnd(); it != end; it++) {
        if (it->isBinary() && !it->getLearnt()) num++;
    }

    return num;
}

void Subsumer::fillClAndBin(vec<ClAndBin>& all, vec<ClauseSimp>& cs, const Lit lit)
{
    for (uint32_t i = 0; i < cs.size(); i++) {
        if (!clauses[cs[i].index]->learnt()) all.push(ClAndBin(cs[i]));
    }

    const vec2<Watched>& ws = solver.watches[(~lit).toInt()];
    for (vec2<Watched>::const_iterator it = ws.getData(), end = ws.getDataEnd(); it != end; it++) {
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
    assert(solver.ok);
    assert(!var_elimed[var]);
    assert(solver.varData[var].elimed == ELIMED_NONE);
    assert(!cannot_eliminate[var]);
    assert(solver.decision_var[var]);
    if (solver.value(var) != l_Undef) return true;

    Lit lit = Lit(var, false);

    //Only exists in binary clauses -- don't delete it then
    /*if (occur[lit.toInt()].size() == 0 && occur[(~lit).toInt()].size() == 0)
        return true;*/

    vec<ClauseSimp>& poss = occur[lit.toInt()];
    vec<ClauseSimp>& negs = occur[(~lit).toInt()];
    const uint32_t numNonLearntPos = numNonLearntBins(lit);
    const uint32_t numNonLearntNeg = numNonLearntBins(~lit);
    uint32_t before_literals = numNonLearntNeg*2 + numNonLearntPos*2;

    uint32_t posSize = 0;
    for (uint32_t i = 0; i < poss.size(); i++)
        if (!clauses[poss[i].index]->learnt()) {
            posSize++;
            before_literals += clauses[poss[i].index]->size();
        }
    posSize += numNonLearntPos;

    uint32_t negSize = 0;
    for (uint32_t i = 0; i < negs.size(); i++)
        if (!clauses[negs[i].index]->learnt()) {
            negSize++;
            before_literals += clauses[negs[i].index]->size();
        }
    negSize += numNonLearntNeg;

    numMaxElim -= posSize + negSize;

    // Heuristic CUT OFF:
    if (posSize >= 10 && negSize >= 10) return true;

    // Heuristic CUT OFF2:
    if ((posSize >= 4 && negSize >= 4 && before_literals > 300)
        && clauses.size() > 700000)
        return false;
    if ((posSize >= 6 && negSize >= 6 && before_literals > 400)
        && clauses.size() <= 700000 && clauses.size() > 100000)
        return true;
    if ((posSize >= 8 && negSize >= 8 && before_literals > 700)
        && clauses.size() <= 100000)
        return true;

    vec<ClAndBin> posAll, negAll;
    fillClAndBin(posAll, poss, lit);
    fillClAndBin(negAll, negs, ~lit);

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
            if (after_clauses > (before_clauses+ ((bool)(solver.order_heap.size() < 25000)) )) return false;
        }
    }

    //Eliminate:
    numMaxElim -= posSize * negSize + before_literals;

    //removing clauses (both non-learnt and learnt)
    vec<ClauseSimp> tmp1 = poss;
    poss.clear();
    for (uint32_t i = 0; i < tmp1.size(); i++) {
        if (clauses[tmp1[i].index]->learnt()) unlinkClause(tmp1[i], *clauses[tmp1[i].index]);
    }
    vec<ClauseSimp> tmp2 = negs;
    negs.clear();
    for (uint32_t i = 0; i < tmp2.size(); i++) {
        if (clauses[tmp2[i].index]->learnt()) unlinkClause(tmp2[i], *clauses[tmp2[i].index]);
    }

    removeClauses(posAll, negAll, var);

    //check watchlists
    #ifndef NDEBUG
    const vec2<Watched>& ws1 = solver.watches[lit.toInt()];
    for (vec2<Watched>::const_iterator i = ws1.getData(), end = ws1.getDataEnd(); i != end; i++) {
        assert(i->isTriClause() || (i->isBinary() && i->getLearnt()));
    }
    const vec2<Watched>& ws2 = solver.watches[(~lit).toInt()];
    for (vec2<Watched>::const_iterator i = ws2.getData(), end = ws2.getDataEnd(); i != end; i++) {
        assert(i->isTriClause() || (i->isBinary() && i->getLearnt()));
    }
    #endif

    removeClauses(posAll, negAll, var);
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

        #ifdef VERBOSE_DEBUG
        std::cout << "Adding new clause due to varelim: " << dummy << std::endl;
        #endif
        Clause* newCl = solver.addClauseInt(dummy, group_num, false, 0, false, false);
        if (newCl != NULL) {
            ClauseSimp newClSimp = linkInClause(*newCl);
            if (numMaxSubsume1 > 0) subsume1(newClSimp, *newCl);
            else subsume0(newClSimp, *newCl);
        } else {
            if (dummy.size() == 2) subsume0(std::numeric_limits< uint32_t >::max(), dummy, calcAbstraction(dummy));
        }
        if (!solver.ok) return false;
    }
    for(uint32_t i = 0; i < posAll.size(); i++) {
        if (!posAll[i].isBin && clauses[posAll[i].clsimp.index] != NULL) {
            solver.clauseAllocator.clauseFree(clauses[posAll[i].clsimp.index]);
            clauses[posAll[i].clsimp.index] = NULL;
        }
    }
    for(uint32_t i = 0; i < negAll.size(); i++) {
        if (!negAll[i].isBin && clauses[negAll[i].clsimp.index] != NULL) {
            solver.clauseAllocator.clauseFree(clauses[negAll[i].clsimp.index]);
            clauses[negAll[i].clsimp.index] = NULL;
        }
    }

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

    //removeBinsAndTris(var);

    assert(occur[lit.toInt()].size() == 0 &&  occur[(~lit).toInt()].size() == 0);
    var_elimed[var] = true;
    solver.varData[var].elimed = ELIMED_VARELIM;
    numElimed++;
    numMaxElimVars--;
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
            Clause* tmpOK = solver.addClauseInt(tmp, 0, true, 2, true);
            numLearntBinVarRemAdded++;
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
    if (!ps.isBin && clauses[ps.clsimp.index] == NULL) return false;
    if (!qs.isBin && clauses[qs.clsimp.index] == NULL) return false;

    bool retval = true;
    if (ps.isBin) {
        assert(ps.lit1 == without_p);
        assert(ps.lit2 != without_p);

        seen_tmp[ps.lit2.toInt()] = 1;
        out_clause.push(ps.lit2);
    } else {
        Clause& c = *clauses[ps.clsimp.index];
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
        Clause& c = *clauses[qs.clsimp.index];
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
        Clause& c = *clauses[ps.clsimp.index];
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
    for (vector<Var>::const_iterator it = touchedVars.begin(), end = touchedVars.end(); it != end ; it++){
        Lit x = Lit(*it, false);
        uint32_t pos = 0;
        const vec<ClauseSimp>& poss = occur[x.toInt()];
        for (uint32_t i = 0; i < poss.size(); i++)
            if (!clauses[poss[i].index]->learnt()) pos++;

        uint32_t neg = 0;
        const vec<ClauseSimp>& negs = occur[(~x).toInt()];
        for (uint32_t i = 0; i < negs.size(); i++)
            if (!clauses[negs[i].index]->learnt()) neg++;

        uint32_t nNonLPos = numNonLearntBins(x);
        uint32_t nNonLNeg = numNonLearntBins(~x);
        uint32_t cost = pos*neg/4 +  nNonLPos*neg*2 + nNonLNeg*pos*2 + nNonLNeg*nNonLPos*6;
        cost_var.push(std::make_pair(cost, x.var()));
    }
    touchedVars.clear();

    std::sort(cost_var.getData(), cost_var.getDataEnd(), myComp());
    for (uint32_t x = 0; x < cost_var.size(); x++) {
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
        if (clauses[i] == NULL) continue;
        Clause& c = *clauses[i];
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
    cout << "allTautology: " << ps << std::endl;
    #endif

    numMaxBlockToVisit -= ps.size()*2;
    for (const Lit *l = ps.getData(), *end = ps.getDataEnd(); l != end; l++) {
        if (*l != ~lit) seen_tmp[l->toInt()] = true;
    }

    bool allIsTautology = true;
    const vec<ClauseSimp>& cs = occur[lit.toInt()];
    const vec2<Watched>& ws = solver.watches[(~lit).toInt()];

    for (const ClauseSimp *it = cs.getData(), *end = cs.getDataEnd(); it != end; it++){
        const Clause& c = *clauses[it->index];
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
    for (vec2<Watched>::const_iterator it = ws.getData(), end = ws.getDataEnd(); it != end; it++) {
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
    if (solver.order_heap.empty()) return;

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
    while (numMaxBlockToVisit > 0 && !touchedBlockedVars.empty()) {
        VarOcc vo = touchedBlockedVars.top();
        touchedBlockedVars.pop();
        touchedBlockedVarsBool[vo.var] = false;

        if (solver.value(vo.var) != l_Undef
            || !solver.decision_var[vo.var]
            || cannot_eliminate[vo.var]
            || dontElim[vo.var])
            continue;

        triedToBlock++;
        Lit lit = Lit(vo.var, false);

        //if (!tryOneSetting(lit)) {
            tryOneSetting(lit);
       // }
    }

    if (solver.conf.verbosity >= 1) {
        std::cout
        << "c spec. var-rem cls: " << std::setw(8) << numblockedClauseRemoved
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
        if (!allTautology(*clauses[it->index], ~lit)) {
            return false;
        }
    }

    vec<Lit> lits(1);
    const vec2<Watched>& ws = solver.watches[(~lit).toInt()];
    numMaxBlockToVisit -= ws.size();
    for (vec2<Watched>::const_iterator it = ws.getData(), end = ws.getDataEnd(); it != end; it++) {
        if (!it->isNonLearntBinary()) continue;
        lits[0] = it->getOtherLit();
        if (!allTautology(lits, ~lit)) return false;
    }

    blockedClauseElimAll(lit);
    blockedClauseElimAll(~lit);

    var_elimed[lit.var()] = true;
    solver.varData[lit.var()].elimed = ELIMED_VARELIM;
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
        unlinkClause(*it, *clauses[it->index], lit.var());
        solver.clauseAllocator.clauseFree(clauses[it->index]);
        clauses[it->index] = NULL;
        numblockedClauseRemoved++;
    }

    uint32_t removedNum = 0;
    vec2<Watched>& ws = solver.watches[(~lit).toInt()];
    vec2<Watched>::iterator i = ws.getData();
    vec2<Watched>::iterator j = i;
    for (vec2<Watched>::iterator end = ws.getDataEnd(); i != end; i++) {
        if (!i->isNonLearntBinary()) {
            *j++ = *i;
            continue;
        }
        assert(!i->getLearnt());
        removeWBin(solver.watches[(~i->getOtherLit()).toInt()], lit, false);
        elimedOutVarBin[lit.var()].push_back(std::make_pair(lit, i->getOtherLit()));
        touchedVars.touch(i->getOtherLit(), false);
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

class NewGateData
{
    public:
        NewGateData(const Lit _lit1, const Lit _lit2, const uint32_t _numLitRem, const uint32_t _numClRem) :
            lit1(_lit1)
            , lit2(_lit2)
            , numLitRem(_numLitRem)
            , numClRem(_numClRem)
        {}
        const bool operator<(const NewGateData& n2) const
        {
            uint32_t value1 = numClRem*ANDGATEUSEFUL + numLitRem;
            uint32_t value2 = n2.numClRem*ANDGATEUSEFUL + n2.numLitRem;
            if (value1 != value2) return(value1 > value2);
            if (lit1 != n2.lit1) return(lit1 > n2.lit1);
            if (lit2 != n2.lit2) return(lit2 > n2.lit2);
            return false;
        }
        const bool operator==(const NewGateData& n2) const
        {
            return(lit1 == n2.lit1
                && lit2 == n2.lit2
                && numLitRem == n2.numLitRem
                && numClRem == n2.numClRem);
        }
        Lit lit1;
        Lit lit2;
        uint32_t numLitRem;
        uint32_t numClRem;
};

struct SecondSorter
{
    const bool operator() (const std::pair<Var, uint32_t> p1, const std::pair<Var, uint32_t> p2)
    {
        return p1.second > p2.second;
    }
};

void Subsumer::createNewVar()
{
    double myTime = cpuTime();
    vector<NewGateData> newGates;
    vec<Lit> lits;
    vec<ClauseSimp> subs;
    numMaxSubsume0 = 300*1000*1000;
    uint64_t numOp = 0;

    if (solver.negPosDist.size() == 0) return;
    uint32_t size = std::min((uint32_t)solver.negPosDist.size()-1, 400U);


    uint32_t tries = 0;
    for (; tries < std::min(100000U, size*size/2); tries++) {
        Var var1, var2;
        var1 = solver.negPosDist[solver.mtrand.randInt(size)].var;
        var2 = solver.negPosDist[solver.mtrand.randInt(size)].var;
        if (var1 == var2) continue;

        if (solver.value(var1) != l_Undef
            || solver.subsumer->getVarElimed()[var1]
            || solver.xorSubsumer->getVarElimed()[var1]
            || solver.partHandler->getSavedState()[var1] != l_Undef
            ) continue;

        if (solver.value(var2) != l_Undef
            || solver.subsumer->getVarElimed()[var2]
            || solver.xorSubsumer->getVarElimed()[var2]
            || solver.partHandler->getSavedState()[var2] != l_Undef
            ) continue;

        Lit lit1 = Lit(var1, solver.mtrand.randInt(1));
        Lit lit2 = Lit(var2, solver.mtrand.randInt(1));
        if (lit1 > lit2) std::swap(lit1, lit2);

        lits.clear();
        lits.push(lit1);
        lits.push(lit2);

        subs.clear();
        findSubsumed(std::numeric_limits< uint32_t >::max(), lits, calcAbstraction(lits), subs);

        uint32_t potential = 0;
        if (numOp < 100*1000*1000) {
            OrGate gate;
            gate.eqLit = Lit(0,false);
            gate.lits.push_back(lit1);
            gate.lits.push_back(lit2);
            treatAndGate(gate, false, potential, numOp);
        }

        if (potential > 5 || subs.size() > 100
            || (potential > 1 && subs.size() > 50)) {
            newGates.push_back(NewGateData(lit1, lit2, subs.size(), potential));
        }
    }

    std::sort(newGates.begin(), newGates.end());
    newGates.erase(std::unique(newGates.begin(), newGates.end() ), newGates.end() );

    uint32_t addedNum = 0;
    for (uint32_t i = 0; i < newGates.size(); i++) {
        const NewGateData& n = newGates[i];
        if ((i > 50 && n.numLitRem < 1000 && n.numClRem < 25)
            || i > ((double)solver.order_heap.size()*0.01)
            || i > 100) break;

        const Var newVar = solver.newVar();
        dontElim[newVar] = true;
        const Lit newLit = Lit(newVar, false);
        OrGate orGate;
        orGate.eqLit = newLit;
        orGate.lits.push_back(n.lit1);
        orGate.lits.push_back(n.lit2);
        orGates.push_back(orGate);

        lits.clear();
        lits.push(newLit);
        lits.push(~n.lit1);
        Clause* cl = solver.addClauseInt(lits, 0);
        assert(cl == NULL);
        assert(solver.ok);

        lits.clear();
        lits.push(newLit);
        lits.push(~n.lit2);
        cl = solver.addClauseInt(lits, 0);
        assert(cl == NULL);
        assert(solver.ok);

        lits.clear();
        lits.push(~newLit);
        lits.push(n.lit1);
        lits.push(n.lit2);
        cl = solver.addClauseInt(lits, 0, false, 0, false, false);
        assert(cl != NULL);
        assert(solver.ok);
        ClauseSimp c = linkInClause(*cl);
        clauseData[c.index].defOfOrGate = true;

        addedNum++;
        numERVars++;
    }
    finishedAddingVars = true;

    if (solver.conf.verbosity >= 1) {
        std::cout << "c Added " << addedNum << " vars "
        << " tried: " << tries
        << " time: " << (cpuTime() - myTime) << std::endl;
    }
    //std::cout << "c Added " << addedNum << " vars "
    //<< " time: " << (cpuTime() - myTime) << " numThread: " << solver.threadNum << std::endl;
}

const bool Subsumer::findOrGatesAndTreat()
{
    assert(solver.ok);
    uint32_t old_clauses_subsumed = clauses_subsumed;

    double myTime = cpuTime();
    orGates.clear();
    for (size_t i = 0; i < gateOcc.size(); i++) gateOcc[i].clear();

    totalOrGateSize = 0;
    uint32_t oldNumVarToReplace = solver.varReplacer->getNewToReplaceVars();
    uint32_t oldNumBins = solver.numBins;
    gateLitsRemoved = 0;
    numOrGateReplaced = 0;

    findOrGates(false);
    doAllOptimisationWithGates();

    if (solver.conf.verbosity >= 1) {
        std::cout << "c ORs : " << std::setw(6) << orGates.size()
        << " avg-s: " << std::fixed << std::setw(4) << std::setprecision(1) << ((double)totalOrGateSize/(double)orGates.size())
        << " cl-sh: " << std::setw(5) << numOrGateReplaced
        << " l-rem: " << std::setw(6) << gateLitsRemoved
        << " b-add: " << std::setw(6) << (solver.numBins - oldNumBins)
        << " v-rep: " << std::setw(3) << (solver.varReplacer->getNewToReplaceVars() - oldNumVarToReplace)
        << " cl-rem: " << andGateNumFound
        << " avg s: " << ((double)andGateTotalSize/(double)andGateNumFound)
        << " T: " << std::fixed << std::setw(7) << std::setprecision(2) <<  (cpuTime() - myTime) << std::endl;
    }
    if (!solver.ok) return false;

    bool cannotDoER;
    #pragma omp critical (ERSync)
    cannotDoER = (solver.threadNum != solver.dataSync->getThreadAddingVars()
        || solver.dataSync->getEREnded());


    if (solver.conf.doER && !cannotDoER) {
        vector<OrGate> backupOrGates = orGates;
        if (!carryOutER()) return false;
        for (vector<OrGate>::const_iterator it = backupOrGates.begin(), end = backupOrGates.end(); it != end; it++) {
            orGates.push_back(*it);
        }
    }

    for (vector<OrGate>::iterator it = orGates.begin(), end = orGates.end(); it != end; it++) {
        OrGate& gate = *it;
        for (uint32_t i = 0; i < gate.lits.size(); i++) {
            Lit lit = gate.lits[i];
            gateOcc[lit.toInt()].push_back(&(*it));
        }
    }

    clauses_subsumed = old_clauses_subsumed;

    return solver.ok;
}

const bool Subsumer::carryOutER()
{
    double myTime = cpuTime();
    orGates.clear();
    totalOrGateSize = 0;
    uint32_t oldNumVarToReplace = solver.varReplacer->getNewToReplaceVars();
    uint32_t oldNumBins = solver.numBins;
    gateLitsRemoved = 0;
    numOrGateReplaced = 0;
    for (uint32_t i = 0; i < clauseData.size(); i++) {
        clauseData[i].defOfOrGate = false;
    }

    createNewVar();
    doAllOptimisationWithGates();

    if (solver.conf.verbosity >= 1) {
        std::cout << "c ORs : " << std::setw(6) << orGates.size()
        << " avg-s: " << std::fixed << std::setw(4) << std::setprecision(1) << ((double)totalOrGateSize/(double)orGates.size())
        << " cl-sh: " << std::setw(5) << numOrGateReplaced
        << " l-rem: " << std::setw(6) << gateLitsRemoved
        << " b-add: " << std::setw(6) << (solver.numBins - oldNumBins)
        << " v-rep: " << std::setw(3) << (solver.varReplacer->getNewToReplaceVars() - oldNumVarToReplace)
        << " cl-rem: " << andGateNumFound
        << " avg s: " << ((double)andGateTotalSize/(double)andGateNumFound)
        << " T: " << std::fixed << std::setw(7) << std::setprecision(2) <<  (cpuTime() - myTime) << std::endl;
    }
    return solver.ok;
}

const bool Subsumer::doAllOptimisationWithGates()
{
    assert(solver.ok);
    for (uint32_t i = 0; i < orGates.size(); i++) {
        std::sort(orGates[i].lits.begin(), orGates[i].lits.end());
    }
    std::sort(orGates.begin(), orGates.end(), OrGateSorter2());

    for (vector<OrGate>::const_iterator it = orGates.begin(), end = orGates.end(); it != end; it++) {
        if (!shortenWithOrGate(*it)) return false;
    }

    if (!treatAndGates()) return false;
    if (!findEqOrGates()) return false;

    return true;
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
    uint32_t num = 0;
    for (Clause **it = clauses.getData(), **end = clauses.getDataEnd(); it != end; it++, num++) {
        if (*it == NULL) continue;
        const Clause& cl = **it;
        if (!learntGatesToo && cl.learnt()) continue;

        uint8_t numSizeZeroCache = 0;
        Lit which = lit_Undef;
        for (const Lit *l = cl.getData(), *end2 = cl.getDataEnd(); l != end2; l++) {
            Lit lit = *l;
            const vector<LitExtra>& cache = solver.transOTFCache[(~lit).toInt()].lits;

            if (cache.size() == 0) {
                numSizeZeroCache++;
                if (numSizeZeroCache > 1) break;
                which = lit;
            }
        }
        if (numSizeZeroCache > 1) continue;

        if (numSizeZeroCache == 1) {
            findOrGate(~which, ClauseSimp(num), learntGatesToo);
        } else {
            for (const Lit *l = cl.getData(), *end2 = cl.getDataEnd(); l != end2; l++)
                findOrGate(~*l, ClauseSimp(num), learntGatesToo);
        }
    }
}

void Subsumer::findOrGate(const Lit eqLit, const ClauseSimp& c, const bool learntGatesToo)
{
    Clause& cl = *clauses[c.index];
    bool isEqual = true;
    for (const Lit *l2 = cl.getData(), *end3 = cl.getDataEnd(); l2 != end3; l2++) {
        if (*l2 == ~eqLit) continue;
        Lit otherLit = *l2;
        const vector<LitExtra>& cache = solver.transOTFCache[(~otherLit).toInt()].lits;

        bool OK = false;
        for (vector<LitExtra>::const_iterator cacheLit = cache.begin(), endCache = cache.end(); cacheLit != endCache; cacheLit++) {
            if ((learntGatesToo || cacheLit->getOnlyNLBin()) &&
                cacheLit->getLit() == eqLit) {
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
            if (*l2 == ~eqLit) continue;
            gate.lits.push_back(*l2);
        }
        //std::sort(gate.lits.begin(), gate.lits.end());
        gate.eqLit = eqLit;
        //gate.num = orGates.size();
        orGates.push_back(gate);
        totalOrGateSize += gate.lits.size();
        clauseData[c.index].defOfOrGate = true;

        #ifdef VERBOSE_ORGATE_REPLACE
        std::cout << "Found gate : " << gate << std::endl;
        #endif
    }
}

const bool Subsumer::shortenWithOrGate(const OrGate& gate)
{
    assert(solver.ok);

    vec<ClauseSimp> subs;
    findSubsumed(std::numeric_limits< uint32_t >::max(), gate.lits, calcAbstraction(gate.lits), subs);
    for (uint32_t i = 0; i < subs.size(); i++) {
        ClauseSimp c = subs[i];
        if (clauseData[c.index].defOfOrGate) continue;
        Clause& cl = *clauses[c.index];

        bool inside = false;
        for (Lit *l = cl.getData(), *end = cl.getDataEnd(); l != end; l++) {
            if (gate.eqLit.var() == l->var()) {
                inside = true;
                break;
            }
        }
        if (inside) continue;

        #ifdef VERBOSE_ORGATE_REPLACE
        std::cout << "OR gate-based cl-shortening" << std::endl;
        std::cout << "Gate used: " << gate << std::endl;
        std::cout << "orig Clause: " << *cl << std::endl;
        #endif

        numOrGateReplaced++;

        for (vector<Lit>::const_iterator it = gate.lits.begin(), end = gate.lits.end(); it != end; it++) {
            gateLitsRemoved++;
            cl.strengthen(*it);
            maybeRemove(occur[it->toInt()], c);
            touchedVars.touch(*it, cl.learnt());
        }

        gateLitsRemoved--;
        cl.add(gate.eqLit); //we can add, because we removed above. Otherwise this is segfault
        occur[gate.eqLit.toInt()].push(c);
        clauseData[c.index] = AbstData(cl, clauseData[c.index].defOfOrGate);

        #ifdef VERBOSE_ORGATE_REPLACE
        std::cout << "new  Clause : " << *cl << std::endl;
        std::cout << "-----------" << std::endl;
        #endif

        if (!handleUpdatedClause(c, cl)) return false;
    }

    return solver.ok;
}

const bool Subsumer::treatAndGates()
{
    assert(solver.ok);

    andGateNumFound = 0;
    vec<Lit> lits;
    andGateTotalSize = 0;
    uint32_t foundPotential;
    //double myTime = cpuTime();
    uint64_t numOp = 0;

    for (vector<OrGate>::const_iterator it = orGates.begin(), end = orGates.end(); it != end; it++) {
        const OrGate& gate = *it;
        if (gate.lits.size() > 2) continue;
        if (!treatAndGate(gate, true, foundPotential, numOp)) return false;
    }

    //std::cout << "c andgate time : " << (cpuTime() - myTime) << std::endl;

    return true;
}

const bool Subsumer::treatAndGate(const OrGate& gate, const bool reallyRemove, uint32_t& foundPotential, uint64_t& numOp)
{
    assert(gate.lits.size() == 2);
    std::set<ClauseSimp> clToUnlink;
    ClauseSimp other;
    foundPotential = 0;

    if (occur[(~(gate.lits[0])).toInt()].empty() || occur[(~(gate.lits[1])).toInt()].empty())
        return true;

    for (uint32_t i = 0; i < sizeSortedOcc.size(); i++)
        sizeSortedOcc[i].clear();

    const vec<ClauseSimp>& csOther = occur[(~(gate.lits[1])).toInt()];
    //std::cout << "csother: " << csOther.size() << std::endl;
    uint32_t abstraction = 0;
    uint16_t maxSize = 0;
    for (const ClauseSimp *it2 = csOther.getData(), *end2 = csOther.getDataEnd(); it2 != end2; it2++) {
        if (clauseData[it2->index].defOfOrGate) continue;
        const Clause& cl = *clauses[it2->index];
        numOp += cl.size();

        maxSize = std::max(maxSize, cl.size());
        if (sizeSortedOcc.size() < (uint32_t)maxSize+1) sizeSortedOcc.resize(maxSize+1);
        sizeSortedOcc[cl.size()].push_back(*it2);

        for (uint32_t i = 0; i < cl.size(); i++) {
            seen_tmp2[cl[i].toInt()] = true;
            abstraction |= 1 << (cl[i].var() & 31);
        }
    }
    abstraction |= 1 << (gate.lits[0].var() & 31);

    vec<ClauseSimp>& cs = occur[(~(gate.lits[0])).toInt()];
    //std::cout << "cs: " << cs.size() << std::endl;
    for (ClauseSimp *it2 = cs.getData(), *end2 = cs.getDataEnd(); it2 != end2; it2++) {
        if (clauseData[it2->index].defOfOrGate
            || (clauseData[it2->index].abst | abstraction) != abstraction
            || clauseData[it2->index].size > maxSize
            || sizeSortedOcc[clauseData[it2->index].size].empty()) continue;
        numOp += clauseData[it2->index].size;

        const Clause& cl = *clauses[it2->index];
        bool OK = true;
        for (uint32_t i = 0; i < cl.size(); i++) {
            if (cl[i] == ~(gate.lits[0])) continue;
            if (   cl[i].var() == ~(gate.lits[1].var())
                || cl[i].var() == gate.eqLit.var()
                || !seen_tmp2[cl[i].toInt()]
                ) {
                OK = false;
                break;
            }
        }
        if (!OK) continue;

        uint32_t abst2 = 0;
        for (uint32_t i = 0; i < cl.size(); i++) {
            if (cl[i] == ~(gate.lits[0])) continue;
            seen_tmp[cl[i].toInt()] = true;
            abst2 |= 1 << (cl[i].var() & 31);
        }
        abst2 |= 1 << ((~(gate.lits[1])).var() & 31);

        numOp += sizeSortedOcc[cl.size()].size()*5;
        const bool foundOther = findAndGateOtherCl(sizeSortedOcc[cl.size()], ~(gate.lits[1]), abst2, other);
        foundPotential += foundOther;
        if (reallyRemove && foundOther) {
            assert(other.index != it2->index);
            clToUnlink.insert(other.index);
            clToUnlink.insert(it2->index);
            if (!treatAndGateClause(other, gate, cl)) return false;
        }

        for (uint32_t i = 0; i < cl.size(); i++) {
            seen_tmp[cl[i].toInt()] = false;
        }
    }

    std::fill(seen_tmp2.getData(), seen_tmp2.getDataEnd(), 0);

    for(std::set<ClauseSimp>::const_iterator it2 = clToUnlink.begin(), end2 = clToUnlink.end(); it2 != end2; it2++) {
        unlinkClause(*it2, *clauses[it2->index]);
    }
    clToUnlink.clear();

    return true;
}

const bool Subsumer::treatAndGateClause(const ClauseSimp& other, const OrGate& gate, const Clause& cl)
{
    vec<Lit> lits;

    #ifdef VERBOSE_ORGATE_REPLACE
    std::cout << "AND gate-based cl rem" << std::endl;
    std::cout << "clause 1: " << cl << std::endl;
    std::cout << "clause 2: " << *other.clause << std::endl;
    std::cout << "gate : " << gate << std::endl;
    #endif

    lits.clear();
    for (uint32_t i = 0; i < cl.size(); i++) {
        if (cl[i] != ~(gate.lits[0])) lits.push(cl[i]);

        assert(cl[i].var() != gate.eqLit.var());
    }
    andGateNumFound++;
    andGateTotalSize += cl.size();

    lits.push(~(gate.eqLit));
    Clause& otherCl = *clauses[other.index];
    bool learnt = otherCl.learnt() && cl.learnt();
    uint32_t glue;
    if (!learnt) glue = 0;
    else glue = std::min(otherCl.getGlue(), cl.getGlue());

    #ifdef VERBOSE_ORGATE_REPLACE
    std::cout << "new clause:" << lits << std::endl;
    std::cout << "-----------" << std::endl;
    #endif

    Clause* c = solver.addClauseInt(lits, cl.getGroup(), learnt, glue, false, false);
    if (c != NULL) linkInClause(*c);
    if (!solver.ok) return false;

    return true;
}

inline const bool Subsumer::findAndGateOtherCl(const vector<ClauseSimp>& sizeSortedOcc, const Lit lit, const uint32_t abst2, ClauseSimp& other)
{
    for (vector<ClauseSimp>::const_iterator it = sizeSortedOcc.begin(), end = sizeSortedOcc.end(); it != end; it++) {
        if (clauseData[it->index].defOfOrGate
            || clauseData[it->index].abst != abst2) continue;

        const Clause& cl = *clauses[it->index];
        for (uint32_t i = 0; i < cl.size(); i++) {
            if (cl[i] == lit) continue;
            if (!seen_tmp[cl[i].toInt()]) goto next;
        }
        other = *it;
        return true;

        next:;
    }

    return false;
}

void Subsumer::makeAllBinsNonLearnt()
{
    uint32_t changedHalfToNonLearnt = 0;
    uint32_t wsLit = 0;
    for (vec<Watched> *it = solver.watches.getData(), *end = solver.watches.getDataEnd(); it != end; it++, wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        vec<Watched>& ws = *it;

        for (Watched *i = ws.getData(), *end2 = ws.getDataEnd(); i != end2; i++) {
            if (i->isBinary() && i->getLearnt()) {
                i->setLearnt(false);
                changedHalfToNonLearnt++;
            }
        }
    }

    assert(changedHalfToNonLearnt % 2 == 0);
    solver.learnts_literals -= changedHalfToNonLearnt;
    solver.clauses_literals += changedHalfToNonLearnt;
}
