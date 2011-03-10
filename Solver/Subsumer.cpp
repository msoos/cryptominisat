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
#include <algorithm>
#include "PartHandler.h"
#include "SolutionExtender.h"

#ifdef _MSC_VER
#define __builtin_prefetch
#endif //_MSC_VER

//#define VERBOSE_DEBUG
#ifdef VERBOSE_DEBUG
#define BIT_MORE_VERBOSITY
#define VERBOSE_ORGATE_REPLACE
#define VERBOSE_ASYMTE
#endif

//#define BIT_MORE_VERBOSITY
//#define TOUCH_LESS
//#define VERBOSE_ORGATE_REPLACE
//#define VERBOSE_DEBUG_ASYMTE


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

void Subsumer::extendModel(SolutionExtender* extender) const
{
    //go through in reverse order
    for (vector<BlockedClause>::const_reverse_iterator it = blockedClauses.rbegin(), end = blockedClauses.rend(); it != end; it++) {
        extender->addBlockedClause(*it);
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
    //TODO
    assert(false);
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
void Subsumer::unlinkClause(ClauseSimp c, Clause& cl, const Lit elim)
{
    for (uint32_t i = 0; i < cl.size(); i++) {
        if (elim != lit_Undef) numMaxElim -= occur[cl[i].toInt()].size()/2;
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

    if (elim != lit_Undef) {
        assert(!cl.learnt());
        #ifdef VERBOSE_DEBUG
        std::cout << "Eliminating non-bin clause: " << *c.clause << std::endl;
        std::cout << "On variable: " << elim+1 << std::endl;
        #endif //VERBOSE_DEBUG
        vector<Lit> lits(cl.size());
        std::copy(cl.getData(), cl.getDataEnd(), lits.begin());
        blockedClauses.push_back(BlockedClause(elim, lits));
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
const bool Subsumer::strenghten(ClauseSimp& c, Clause& cl, const Lit toRemoveLit)
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

    return handleUpdatedClause(c, cl);
}

const bool Subsumer::handleUpdatedClause(ClauseSimp& c, Clause& cl)
{
    assert(solver.ok);

    if (cleanClause(c, cl)) {
        unlinkClause(c, cl);
        c.index = std::numeric_limits< uint32_t >::max();
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
            solver.enqueue(cl[0]);
            unlinkClause(c, cl);
            c.index = std::numeric_limits< uint32_t >::max();
            solver.propagate<false>();
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

    if (solver.conf.verbosity >= 4) {
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
                    vector<ClauseSimp>& occs = occur[(cl[j]).toInt()];
                    for (uint32_t k = 0; k < occs.size(); k++) {
                        s0.add(occs[k]);
                    }
                }
                ol_seenPos[cl[j].toInt()] = 1;

                if (!ol_seenNeg[(~cl[j]).toInt()]) {
                    vector<ClauseSimp>& occs = occur[(~cl[j]).toInt()];
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
    clauses.push_back(&cl);
    clauseData.push_back(AbstData(cl, false));
    assert(clauseData.size() == clauses.size());

    for (uint32_t i = 0; i < cl.size(); i++) {
        occur[cl[i].toInt()].push_back(c);
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
        vector<ClauseSimp> tmp;
        occur[i].swap(tmp);
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
        if (solver.value(var) == l_Undef
            && !cannot_eliminate[var]
            && !solver.varReplacer->cannot_eliminate[var]
            && solver.decision_var[var]) {
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
    if (expected_size > 10000000) return solver.ok;

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

    #ifdef DEBUG_VAR_ELIM
    for (uint32_t i = 0; i < clauses.size(); i++) {
        if (clauses[i] == NULL) continue;
        const Clause& cl = *clauses[i];
        for (uint32_t i = 0; i < cl.size(); i++) {
            if (var_elimed[cl[i].var()]) {
                std::cout << "Elmied var -- Lit " << cl[i] << " in clause?" << std::endl;
                std::cout << "wrongly left in clause: " << cl << std::endl;
                exit(-1);
            }
        }
    }

    uint32_t wsLit = 0;
    for (const vec2<Watched> *it = solver.watches.getData(), *end = solver.watches.getDataEnd(); it != end; it++, wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        const vec2<Watched>& ws = *it;
        for (vec2<Watched>::const_iterator it2 = ws.getData(), end2 = ws.getDataEnd(); it2 != end2; it2++) {
            if (it2->isBinary()) {
                if (var_elimed[lit.var()] || var_elimed[it2->getOtherLit().var()]) {
                    std::cout << "One var elimed: " << lit << " , " << it2->getOtherLit() << std::endl;
                    exit(-1);
                }
            }
        }
    }
    #endif

    if (solver.conf.doFindXors && !findXors()) return false;

    if (solver.conf.doAsymmTE) AsymmTE();

    do {
        if (!subsume0AndSubsume1()) return false;

        if (!solver.conf.doVarElim) break;

        if (!eliminateVars()) return false;

        //subsumeBinsWithBins();
        //if (solver.conf.doSubsWBins && !subsumeWithBinaries()) return false;
        solver.clauseCleaner->removeSatisfiedBins();
    } while (cl_touched.nElems() > 100);

    if (solver.conf.doCacheNLBins) {
        if (solver.conf.doGateFind
            && !findOrGatesAndTreat()) return false;

        if (solver.conf.doER
            && !extendedResolution()) return false;

        setLimits();
        for (vector<AbstData>::iterator it = clauseData.begin(), end = clauseData.end(); it != end; it++)
            it->defOfOrGate = false;
        if (!subsume0AndSubsume1()) return false;
    }

    assert(solver.ok);
    assert(verifyIntegrity());

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

void Subsumer::AsymmTE()
{
    uint32_t abst;
    vector<Lit> tmpCl;
    const double myTime = cpuTime();
    uint32_t clauses_subsumed_old = clauses_subsumed;
    uint32_t blocked = 0;

    uint32_t index = clauses.size()-1;
    int64_t numToDo = 1ULL*1000ULL*1000ULL;
    for (vector<Clause*>::reverse_iterator it = clauses.rbegin(), end = clauses.rend(); it != end; it++, index--) {
        if (*it == NULL || (**it).learnt()) {
            continue;
        }

        Clause& cl = **it;
        tmpCl.clear();
        for (const Lit *l = cl.getData(), *end = cl.getDataEnd(); l != end; l++) {
            seen[l->toInt()] = true;
            tmpCl.push_back(*l);
        }

        for (const Lit *l = cl.getData(), *end = cl.getDataEnd(); l != end; l++) {
            const vector<LitExtra>& cache = solver.transOTFCache[l->toInt()].lits;
            for (vector<LitExtra>::const_iterator cacheLit = cache.begin(), endCache = cache.end(); cacheLit != endCache; cacheLit++) {
                if (cacheLit->getOnlyNLBin()
                    && !seen[(~cacheLit->getLit()).toInt()]) {
                    Lit toAdd = ~(cacheLit->getLit());
                    tmpCl.push_back(toAdd);
                    seen[toAdd.toInt()] = true;
                }
            }
        }

        //subsumption with non-learnt binary clauses
        bool toRemove = false;
        //for (vector<Lit>::const_iterator l = tmpCl.begin(), end = tmpCl.end(); l != end; l++) {
        for (const Lit* l = cl.getData(), *end = cl.getDataEnd(); l != end; l++) {
            const vector<LitExtra>& cache = solver.transOTFCache[l->toInt()].lits;
            for (vector<LitExtra>::const_iterator cacheLit = cache.begin(), endCache = cache.end(); cacheLit != endCache; cacheLit++) {
                if (cacheLit->getOnlyNLBin()
                    && solver.seen[cacheLit->getLit().toInt()]) {
                    toRemove = true;
                    #ifdef VERBOSE_DEBUG_ASYMTE
                    std::cout << "c AsymLitAdd removing: " << cl << std::endl;
                    #endif
                    goto end;
                }
            }
        }

        if (solver.conf.doBlockedClause) {
            for (const Lit* l = cl.getData(), *end = cl.getDataEnd(); l != end; l++) {
                if (cannot_eliminate[(~*l).var()]) continue;
                //if (solver.varReplacer->cannot_eliminate[(~*l).var()]) continue;

                if (allTautologySlim(~*l)) {
                    vector<Lit> remCl;
                    for (Lit *l2 = cl.getData(), *end2 = cl.getDataEnd(); l2 != end2; l2++)
                        remCl.push_back(*l2);
                    blockedClauses.push_back(BlockedClause(*l, remCl));
                    var_blocked[l->var()] = true;
                    blocked++;
                    toRemove = true;
                    goto end;
                }
            }
        }

        if (numToDo < 0) goto end;
        //subsumption with non-learnt larger clauses
        abst = calcAbstraction(tmpCl);
        for (vector<Lit>::const_iterator it = tmpCl.begin(), end = tmpCl.end(); it != end; it++) {
            const vector<ClauseSimp>& occ = occur[it->toInt()];
            numToDo -= occ.size();
            for (vector<ClauseSimp>::const_iterator it2 = occ.begin(), end2 = occ.end(); it2 != end2; it2++) {
                if (it2->index != index
                    && subsetAbst(clauseData[it2->index].abst, abst)
                    && clauses[it2->index] != NULL
                    && !clauses[it2->index]->learnt()
                    && subsetReverse(*clauses[it2->index])
                )  {
                    #ifdef VERBOSE_DEBUG_ASYMTE
                    std::cout << "c AsymTE removing: " << cl << " -- subsumed by cl: " << *clauses[it2->index] << std::endl;
                    #endif
                    toRemove = true;
                    goto end;
                }
            }
        }

        end:
        if (toRemove) {
            assert(!clauseData[index].defOfOrGate);
            unlinkClause(index, cl);
        }

        for (vector<Lit>::const_iterator l = tmpCl.begin(), end = tmpCl.end(); l != end; l++) {
            seen[l->toInt()] = false;
        }
    }

    if (solver.conf.verbosity >= 1) {
        std::cout << "c AsymmTElim"
        << " cl-rem: " << (clauses_subsumed - clauses_subsumed_old)
        << " blocked: " << blocked
        << " time : " << (cpuTime() - myTime)
        << std::endl;
    }

    clauses_subsumed = clauses_subsumed_old;
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

    numMaxElimVars = (solver.order_heap.size()/3);

    numMaxBlockVars = (uint32_t)((double)solver.order_heap.size() / 1.5 * (0.8+(double)(numCalls)/4.0));

    if (numCalls == 1) {
        numMaxSubsume1 = 3*1000*1000;
    }

    if (!solver.conf.doSubsume1) {
        numMaxSubsume1 = 0;
    }

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
    vector<BlockedClause>::iterator i = blockedClauses.begin();
    vector<BlockedClause>::iterator j = blockedClauses.begin();

    for (vector<BlockedClause>::iterator end = blockedClauses.end(); i != end; i++) {
        if (solver.value(i->blockedOn) != l_Undef) {
            const Var var = i->blockedOn.var();
            if (solver.varData[var].elimed == ELIMED_VARELIM) {
                var_elimed[var] = false;
                solver.varData[var].elimed = ELIMED_NONE;
                solver.setDecisionVar(var, true);
                numElimed--;
            }
        } else {
            *j++ = *i;
        }
    }
    blockedClauses.resize(blockedClauses.size()-(i-j));
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
        seen[ps[i].toInt()] = 1;

    uint32_t min_i = 0;
    for (uint32_t i = 1; i < ps.size(); i++){
        if (occur[ps[i].toInt()].size() < occur[ps[min_i].toInt()].size())
            min_i = i;
    }

    vector<ClauseSimp>& cs = occur[ps[min_i].toInt()];
    numMaxSubsume0 -= cs.size()*10 + 5;
    for (vector<ClauseSimp>::iterator it = cs.begin(), end = cs.end(); it != end; it++){
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
        seen[ps[i].toInt()] = 0;
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
    vector<ClauseSimp>& cs = occur[lit.toInt()];
    for (vector<ClauseSimp>::const_iterator it = cs.begin(), end = cs.end(); it != end; it++) {
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


void Subsumer::removeClausesHelper(vector<ClAndBin>& todo, const Lit lit, std::pair<uint32_t, uint32_t>& removed)
{
     pair<uint32_t, uint32_t>  tmp;
     for (uint32_t i = 0; i < todo.size(); i++) {
        ClAndBin& c = todo[i];
        if (!c.isBin) {
            if (clauses[c.clsimp.index] != NULL)
                unlinkClause(c.clsimp, *clauses[c.clsimp.index], lit);
        } else {
            #ifdef VERBOSE_DEBUG
            std::cout << "Eliminating bin clause: " << c.lit1 << " , " << c.lit2 << std::endl;
            std::cout << "On variable: " << var+1 << std::endl;
            #endif
            assert(lit == c.lit1 || lit == c.lit2);
            tmp = removeWBinAll(solver.watches[(~c.lit1).toInt()], c.lit2);
            //assert(tmp.first > 0 || tmp.second > 0);
            removed.first += tmp.first;
            removed.second += tmp.second;

            tmp = removeWBinAll(solver.watches[(~c.lit2).toInt()], c.lit1);
            //assert(tmp.first > 0 || tmp.second > 0);
            removed.first += tmp.first;
            removed.second += tmp.second;

            vector<Lit> lits;
            lits.push_back(c.lit1);
            lits.push_back(c.lit2);
            blockedClauses.push_back(BlockedClause(lit, lits));
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
void Subsumer::removeClauses(vector<ClAndBin>& posAll, vector<ClAndBin>& negAll, const Var var)
{
    pair<uint32_t, uint32_t> removed;
    removed.first = 0;
    removed.second = 0;

    removeClausesHelper(posAll, Lit(var, false), removed);
    removeClausesHelper(negAll, Lit(var, true), removed);

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

void Subsumer::fillClAndBin(vector<ClAndBin>& all, vector<ClauseSimp>& cs, const Lit lit)
{
    for (uint32_t i = 0; i < cs.size(); i++) {
        if (!clauses[cs[i].index]->learnt()) all.push_back(ClAndBin(cs[i]));
    }

    const vec2<Watched>& ws = solver.watches[(~lit).toInt()];
    for (vec2<Watched>::const_iterator it = ws.getData(), end = ws.getDataEnd(); it != end; it++) {
        if (it->isBinary() &&!it->getLearnt()) all.push_back(ClAndBin(lit, it->getOtherLit()));
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
    assert(!solver.varReplacer->cannot_eliminate[var]);
    assert(solver.decision_var[var]);
    assert(solver.value(var) == l_Undef);

    Lit lit = Lit(var, false);

    //Only exists in binary clauses -- don't delete it then
    /*if (occur[lit.toInt()].size() == 0 && occur[(~lit).toInt()].size() == 0)
        return true;*/

    vector<ClauseSimp>& poss = occur[lit.toInt()];
    vector<ClauseSimp>& negs = occur[(~lit).toInt()];
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
    if (posSize >= 10 && negSize >= 10) return false;

    // Heuristic CUT OFF2:
    if ((posSize >= 4 && negSize >= 4 && before_literals > 300)
        && clauses.size() > 700000)
        return false;
    if ((posSize >= 6 && negSize >= 6 && before_literals > 400)
        && clauses.size() <= 700000 && clauses.size() > 100000)
        return false;
    if ((posSize >= 8 && negSize >= 8 && before_literals > 700)
        && clauses.size() <= 100000)
        return false;

    vector<ClAndBin> posAll, negAll;
    fillClAndBin(posAll, poss, lit);
    fillClAndBin(negAll, negs, ~lit);

    // Count clauses/literals after elimination:
    numMaxElim -= posSize * negSize + before_literals;
    uint32_t before_clauses = posSize + negSize;
    uint32_t after_clauses = 0;
    for (uint32_t i = 0; i < posAll.size(); i++) for (uint32_t j = 0; j < negAll.size(); j++){
        // Merge clauses. If 'y' and '~y' exist, clause will not be created.
        bool ok = merge(posAll[i], negAll[j], lit, ~lit, false);
        if (ok){
            after_clauses++;
            if (after_clauses > before_clauses) return false;
        }
    }

    //Eliminate:
    numMaxElim -= posSize * negSize + before_literals;

    //removing clauses (both non-learnt and learnt)
    vector<ClauseSimp> tmp1 = poss;
    poss.clear();
    for (uint32_t i = 0; i < tmp1.size(); i++) {
        if (clauses[tmp1[i].index]->learnt()) unlinkClause(tmp1[i], *clauses[tmp1[i].index]);
    }
    vector<ClauseSimp> tmp2 = negs;
    negs.clear();
    for (uint32_t i = 0; i < tmp2.size(); i++) {
        if (clauses[tmp2[i].index]->learnt()) unlinkClause(tmp2[i], *clauses[tmp2[i].index]);
    }

    removeClauses(posAll, negAll, var);
    for (uint32_t i = 0; i < posAll.size(); i++) for (uint32_t j = 0; j < negAll.size(); j++){
        bool ok = merge(posAll[i], negAll[j], lit, ~lit, true);
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
        if (!solver.ok) return true;
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
bool Subsumer::merge(const ClAndBin& ps, const ClAndBin& qs, const Lit without_p, const Lit without_q, const bool really)
{
    if (!ps.isBin && clauses[ps.clsimp.index] == NULL) return false;
    if (!qs.isBin && clauses[qs.clsimp.index] == NULL) return false;

    dummy.clear();
    dummy2.clear();
    bool retval = true;
    if (ps.isBin) {
        assert(ps.lit1 == without_p);
        assert(ps.lit2 != without_p);

        seen[ps.lit2.toInt()] = 1;
        dummy.push(ps.lit2);
        dummy2.push_back(ps.lit2);
    } else {
        Clause& c = *clauses[ps.clsimp.index];
        assert(!clauseData[ps.clsimp.index].defOfOrGate);
        numMaxElim -= c.size();
        for (uint32_t i = 0; i < c.size(); i++){
            if (c[i] != without_p){
                seen[c[i].toInt()] = 1;
                dummy.push(c[i]);
                dummy2.push_back(c[i]);
            }
        }
    }

    if (!ps.isBin && really) {
        for (uint32_t i= 0; i < dummy.size(); i++) {
            const vector<LitExtra>& cache = solver.transOTFCache[dummy[i].toInt()].lits;
            for(vector<LitExtra>::const_iterator it = cache.begin(), end = cache.end(); it != end; it++) {
                if (it->getOnlyNLBin()
                    && !seen[(~(it->getLit())).toInt()]
                    ) {
                    Lit toAdd = ~(it->getLit());
                    dummy2.push_back(toAdd);
                    seen[toAdd.toInt()] = 1;
                }
            }
        }
    }

    if (qs.isBin) {
        assert(qs.lit1 == without_q);
        assert(qs.lit2 != without_q);

        if (seen[(~qs.lit2).toInt()]) {
            retval = false;
            goto end;
        }
        if (!seen[qs.lit2.toInt()])
            dummy.push(qs.lit2);
    } else {
        Clause& c = *clauses[qs.clsimp.index];
        assert(!clauseData[qs.clsimp.index].defOfOrGate);
        numMaxElim -= c.size();
        for (uint32_t i = 0; i < c.size(); i++){
            if (c[i] != without_q) {
                if (seen[(~c[i]).toInt()]) {
                    retval = false;
                    goto end;
                }
                if (!seen[c[i].toInt()])
                    dummy.push(c[i]);
            }
            if (really) {
                const vector<LitExtra>& cache = solver.transOTFCache[c[i].toInt()].lits;
                for(vector<LitExtra>::const_iterator it = cache.begin(), end = cache.end(); it != end; it++) {
                    if (it->getOnlyNLBin()
                        && seen[((it->getLit())).toInt()]
                        ) {
                        retval = false;
                        goto end;
                    }
                }
            }
        }
    }

    end:
    for (vector<Lit>::const_iterator it = dummy2.begin(), end = dummy2.end(); it != end; it++) {
        seen[it->toInt()] = 0;
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
        const vector<ClauseSimp>& poss = occur[x.toInt()];
        for (uint32_t i = 0; i < poss.size(); i++)
            if (!clauses[poss[i].index]->learnt()) pos++;

        uint32_t neg = 0;
        const vector<ClauseSimp>& negs = occur[(~x).toInt()];
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

inline const bool Subsumer::allTautologySlim(const Lit lit)
{
    #ifdef VERBOSE_DEBUG
    cout << "allTautology: " << ps << std::endl;
    #endif

    const vector<ClauseSimp>& cs = occur[lit.toInt()];
    const vec2<Watched>& ws = solver.watches[(~lit).toInt()];

    numMaxBlockToVisit--;
    for (vec2<Watched>::const_iterator it = ws.getData(), end = ws.getDataEnd(); it != end; it++) {
        if (!it->isNonLearntBinary()) continue;
        if (seen[(~it->getOtherLit()).toInt()] && it->getOtherLit() != lit) continue;
        else return false;
    }

    for (vector<ClauseSimp>::const_iterator it = cs.begin(), end = cs.end(); it != end; it++){
        const Clause& c = *clauses[it->index];
        numMaxBlockToVisit--;
        if (c.learnt()) continue;
        for (const Lit *l = c.getData(), *end2 = c.getDataEnd(); l != end2; l++) {
            if (seen[(~(*l)).toInt()] && *l != lit) {
                goto next;
            }
        }
        return false;

        next:;
    }

    return true;
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

void Subsumer::createNewVars()
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

    for (vector<OrGate>::iterator it = orGates.begin(), end = orGates.end(); it != end; it++) {
        OrGate& gate = *it;
        for (uint32_t i = 0; i < gate.lits.size(); i++) {
            Lit lit = gate.lits[i];
            gateOcc[lit.toInt()].push_back(&(*it));
        }
    }

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

    clauses_subsumed = old_clauses_subsumed;

    return solver.ok;
}

const bool Subsumer::extendedResolution()
{
    assert(solver.ok);
    bool cannotDoER;
    #pragma omp critical (ERSync)
    cannotDoER = (solver.threadNum != solver.dataSync->getThreadAddingVars()
        || solver.dataSync->getEREnded());

    if (cannotDoER) return true;

    vector<OrGate> backupOrGates = orGates;
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

    createNewVars();
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

    orGates = backupOrGates;

    for (vector<OrGate>::const_iterator it = backupOrGates.begin(), end = backupOrGates.end(); it != end; it++) {
        orGates.push_back(*it);
    }

    return solver.ok;
}

const bool Subsumer::doAllOptimisationWithGates()
{
    assert(solver.ok);

    //sorting
    for (uint32_t i = 0; i < orGates.size(); i++) {
        std::sort(orGates[i].lits.begin(), orGates[i].lits.end());
    }
    std::sort(orGates.begin(), orGates.end(), OrGateSorter2());

    //OR gate treatment
    if (solver.conf.doShortenWithOrGates) {
        for (vector<OrGate>::const_iterator it = orGates.begin(), end = orGates.end(); it != end; it++) {
            if (!shortenWithOrGate(*it)) return false;
        }
    }

    //AND gate treatment
    if (solver.conf.doRemClWithAndGates) {
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
    }

    //EQ gate treatment
    if (solver.conf.doFindEqLitsWithGates
        && !findEqOrGates()) return false;

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
    for (vector<Clause*>::iterator it = clauses.begin(), end = clauses.end(); it != end; it++, num++) {
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
        bool inside = false;

        #ifdef VERBOSE_ORGATE_REPLACE
        std::cout << "OR gate-based cl-shortening" << std::endl;
        std::cout << "Gate used: " << gate << std::endl;
        std::cout << "orig Clause: " << *cl << std::endl;
        #endif

        for (Lit *l = clauses[c.index]->getData(), *end = clauses[c.index]->getDataEnd(); l != end; l++) {
            if (gate.eqLit.var() == l->var()) {
                if (gate.eqLit == *l) {
                    inside = true;
                    break;
                } else {
                    unlinkClause(c, *clauses[c.index]);
                    goto next;
                }
            }
        }

        if (!inside) {
            vec<Lit> lits;
            for (uint32_t i = 0; i < clauses[c.index]->size(); i++) {
                const Lit lit = (*clauses[c.index])[i];
                lits.push(lit);
                ol_seenPos[lit.toInt()] = 0;
                ol_seenNeg[(~lit).toInt()] = 0;
            }
            lits.push(gate.eqLit);
            uint32_t group = clauses[c.index]->getGroup();
            bool learnt = clauses[c.index]->learnt();
            solver.clauseAllocator.clauseFree(clauses[c.index]);

            clauses[c.index] = solver.clauseAllocator.Clause_new(lits, group, learnt);
            gateLitsRemoved--;
            occur[gate.eqLit.toInt()].push_back(c);
            clauseData[c.index] = AbstData(*clauses[c.index], clauseData[c.index].defOfOrGate);
            cl_touched.add(c);
            if (!handleUpdatedClause(c, *clauses[c.index])) return false;
            if (c.index == std::numeric_limits< uint32_t >::max()) continue;
        }

        numOrGateReplaced++;

        for (vector<Lit>::const_iterator it = gate.lits.begin(), end = gate.lits.end(); it != end; it++) {
            Clause& cl = *clauses[c.index];
            gateLitsRemoved++;
            if (std::find(cl.getData(), cl.getDataEnd(), *it) == cl.getDataEnd()) continue;
            if (!strenghten(c, cl, *it)) return false;
            if (c.index == std::numeric_limits< uint32_t >::max()) goto next;
        }

        #ifdef VERBOSE_ORGATE_REPLACE
        std::cout << "new  Clause : " << *cl << std::endl;
        std::cout << "-----------" << std::endl;
        #endif

        next:;
    }

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

    const vector<ClauseSimp>& csOther = occur[(~(gate.lits[1])).toInt()];
    //std::cout << "csother: " << csOther.size() << std::endl;
    uint32_t abstraction = 0;
    uint16_t maxSize = 0;
    for (vector<ClauseSimp>::const_iterator it2 = csOther.begin(), end2 = csOther.end(); it2 != end2; it2++) {
        if (clauseData[it2->index].defOfOrGate) continue;
        const Clause& cl = *clauses[it2->index];
        numOp += cl.size();

        maxSize = std::max(maxSize, cl.size());
        if (sizeSortedOcc.size() < (uint32_t)maxSize+1) sizeSortedOcc.resize(maxSize+1);
        sizeSortedOcc[cl.size()].push_back(*it2);

        for (uint32_t i = 0; i < cl.size(); i++) {
            seen2[cl[i].toInt()] = true;
            abstraction |= 1 << (cl[i].var() & 31);
        }
    }
    abstraction |= 1 << (gate.lits[0].var() & 31);

    vector<ClauseSimp>& cs = occur[(~(gate.lits[0])).toInt()];
    //std::cout << "cs: " << cs.size() << std::endl;
    for (vector<ClauseSimp>::iterator it2 = cs.begin(), end2 = cs.end(); it2 != end2; it2++) {
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
                || !seen2[cl[i].toInt()]
                ) {
                OK = false;
                break;
            }
        }
        if (!OK) continue;

        uint32_t abst2 = 0;
        for (uint32_t i = 0; i < cl.size(); i++) {
            if (cl[i] == ~(gate.lits[0])) continue;
            seen[cl[i].toInt()] = true;
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
            seen[cl[i].toInt()] = false;
        }
    }

    std::fill(seen2.getData(), seen2.getDataEnd(), 0);

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
            if (!seen[cl[i].toInt()]) goto next;
        }
        other = *it;
        return true;

        next:;
    }

    return false;
}

void Subsumer::otfShortenWithGates(vec<Lit>& cl)
{
    bool gateRemSuccess = false;
    vec<Lit> oldCl = cl;
    while (true) {
        for (Lit *l = cl.getData(), *end = cl.getDataEnd(); l != end; l++) {
            const vector<OrGate*>& gates = gateOcc[l->toInt()];
            if (gates.empty()) continue;

            for (vector<OrGate*>::const_iterator it2 = gates.begin(), end2 = gates.end(); it2 != end2; it2++) {
                OrGate& gate = **it2;

                gate.eqLit = solver.varReplacer->getReplaceTable()[gate.eqLit.var()] ^ gate.eqLit.sign();
                if (solver.varData[gate.eqLit.var()].elimed != ELIMED_NONE) continue;

                bool OK = true;
                for (uint32_t i = 0; i < gate.lits.size(); i++) {
                    if (!seen[gate.lits[i].toInt()]) {
                        OK = false;
                        break;
                    }
                }
                if (!OK) continue;

                //Treat gate
                #ifdef VERBOSE_DEBUG_GATE
                std::cout << "Gate: eqLit "  << gate.eqLit << " lits ";
                for (uint32_t i = 0; i < gate.lits.size(); i++) {
                    std::cout << gate.lits[i] << ", ";
                }
                #endif

                bool firstInside = false;
                Lit *lit1 = cl.getData();
                Lit *lit2 = cl.getData();
                bool first = true;
                for (Lit *end3 = cl.getDataEnd(); lit1 != end3; lit1++, first = false) {
                    bool in = false;
                    for (uint32_t i = 0; i < gate.lits.size(); i++) {
                        if (*lit1 == gate.lits[i]) {
                            in = true;
                            if (first) firstInside = true;
                        }
                    }
                    if (in) {
                        seen[lit1->toInt()] = false;
                    } else {
                        *lit2++ = *lit1;
                    }
                }
                if (!seen[gate.eqLit.toInt()]) {
                    *lit2++ = gate.eqLit;
                    seen[gate.eqLit.toInt()] = true;
                }
                assert(!seen[(~gate.eqLit).toInt()]);
                cl.shrink_(lit1-lit2);
                solver.OTFGateRemLits += lit1-lit2;
                #ifdef VERBOSE_DEBUG_GATE
                std::cout << "Old clause: " << oldCl << std::endl;
                for (uint32_t i = 0; i < oldCl.size(); i++) {
                    std::cout << "-> Lit " << oldCl[i] << " lev: " << level[oldCl[i].var()] << " val: " << value(oldCl[i]) <<std::endl;
                }
                std::cout << "New clause: " << cl << std::endl;
                for (uint32_t i = 0; i < cl.size(); i++) {
                    std::cout << "-> Lit " << cl[i] << " lev: " << level[cl[i].var()] << " val: " << value(cl[i]) << std::endl;
                }
                #endif

                if (firstInside) {
                    uint32_t swapWith = std::numeric_limits<uint32_t>::max();
                    for (uint32_t i = 0; i < cl.size(); i++) {
                        if (cl[i] == gate.eqLit) swapWith = i;
                    }
                    std::swap(cl[swapWith], cl[0]);
                }
                #ifdef VERBOSE_DEBUG_GATE
                std::cout << "New clause2: " << cl << std::endl;
                for (uint32_t i = 0; i < cl.size(); i++) {
                    std::cout << "-> Lit " << cl[i] << " lev: " << level[cl[i].var()] << std::endl;
                }
                #endif

                gateRemSuccess = true;

                //Do this recursively, again
                goto next;
            }
        }
        break;
        next:;
    }
    solver.OTFGateRemSucc += gateRemSuccess;
    #ifdef VERBOSE_DEBUG_GATE
    if (gateRemSuccess) std::cout << "--------" << std::endl;
    #endif
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

const bool Subsumer::findXors()
{
    double myTime = cpuTime();
    //Lowest variable <---> XOR map
    xors.clear();
    xorIndex.clear();
    xorIndex.resize(solver.nVars());

    for (vector<Clause*>::iterator it = clauses.begin(), end = clauses.end(); it != end; it++) {
        if (*it == NULL) continue;
        std::sort((*it)->getData(), (*it)->getDataEnd());
    }

    uint32_t i = 0;
    for (vector<Clause*>::iterator it = clauses.begin(), end = clauses.end(); it != end; it++, i++) {
        if (*it == NULL) continue;
        findXor(i);
    }

    uint32_t index = 0;
    uint32_t doneSomething = 0;
    for (vector<Xor>::iterator it = xors.begin(), end = xors.end(); it != end; it++, index++) {
        doneSomething += tryToXor(*it, index);
        if (!solver.ok) goto end;
    }

    end:
    if (solver.conf.verbosity >= 1) {
        std::cout << "c XOR finding finished. Num XORs: " << std::setw(6) << xors.size()
        << " Done something: " << std::setw(6) << doneSomething
        << " Time: " << std::fixed << std::setprecision(2) << (cpuTime() - myTime) << std::setw(6) << std::endl;
    }

    return solver.ok;
}

const uint32_t Subsumer::tryToXor(const Xor& thisXor, const uint32_t thisIndex)
{
    uint32_t doneSomething = 0;
    for (vector<Var>::const_iterator it = thisXor.vars.begin(), end = thisXor.vars.end(); it != end; it++) {
        seen[*it] = 1;
    }

    for (vector<Var>::const_iterator it = thisXor.vars.begin(), end = thisXor.vars.end(); it != end; it++) {
        const vector<uint32_t>& indexes = xorIndex[*it];
        for (vector<uint32_t>::const_iterator it2 = indexes.begin(), end2 = indexes.end(); it2 != end2; it2++) {
            if (*it2 == thisIndex) continue;
            if (xors[*it2].vars.size() <= thisXor.vars.size()) {
                const Xor& otherXor = xors[*it2];
                uint32_t wrong = 0;
                for (vector<Var>::const_iterator it3 = otherXor.vars.begin(), end3 = otherXor.vars.end(); it3 != end3; it3++) {
                    if (!seen[*it3]) {
                        wrong++;
                        if (wrong > 1 || (wrong>0 && otherXor.vars.size() < thisXor.vars.size())) break;
                    }
                }
                if (wrong == 0 || (wrong == 1 && otherXor.vars.size() == thisXor.vars.size())) {
                    doneSomething++;
                    /*std::cout << "Two xors to do something about: " << std::endl;
                    std::cout << thisXor << std::endl;
                    std::cout << otherXor << std::endl;*/
                    vec<Lit> XORofTwo;
                    for (vector<Var>::const_iterator it3 = otherXor.vars.begin(), end3 = otherXor.vars.end(); it3 != end3; it3++) {
                        if (!seen[*it3]) {
                            XORofTwo.push(Lit(*it3, false));
                        }
                    }
                    if (wrong > 0) {
                        for (vector<Var>::const_iterator it3 = otherXor.vars.begin(), end3 = otherXor.vars.end(); it3 != end3; it3++) {
                            seen2[*it3] = 1;
                        }

                        for (vector<Var>::const_iterator it3 = thisXor.vars.begin(), end3 = thisXor.vars.end(); it3 != end3; it3++) {
                            if (!seen2[*it3]) {
                                XORofTwo.push(Lit(*it3, false));
                            }
                        }

                        for (vector<Var>::const_iterator it3 = otherXor.vars.begin(), end3 = otherXor.vars.end(); it3 != end3; it3++) {
                            seen2[*it3] = 0;
                        }
                    }
                    bool finalRHS = thisXor.rhs ^ otherXor.rhs;
                    XorClause* c = solver.addXorClauseInt(XORofTwo, !finalRHS, 0, true);
                    assert(c == NULL);
                    if (!solver.ok) goto end;;
                }
            }
        }
    }

    end:
    for (vector<Var>::const_iterator it = thisXor.vars.begin(), end = thisXor.vars.end(); it != end; it++) {
        seen[*it] = 0;
    }

    return doneSomething;
}

void Subsumer::findXor(ClauseSimp c)
{
    const Clause& cl = *clauses[c.index];
    if (cl.size() >= 10) return; //for speed

    bool rhs = true;
    uint32_t whichOne = 0;
    uint32_t i = 0;
    for (const Lit *l = cl.getData(), *end = cl.getDataEnd(); l != end; l++, i++) {
        seen[l->var()] = 1;
        rhs ^= l->sign();
        whichOne += ((uint32_t)l->sign()) << i;
    }

    FoundXors foundCls(c, cl, clauseData[c.index], rhs, whichOne);
    for (const Lit *l = cl.getData(), *end = cl.getDataEnd(); l != end; l++) {
        findXorMatch(occur[(*l).toInt()], foundCls);
        findXorMatch(occur[(~*l).toInt()], foundCls);
        findXorMatch(solver.watches[(~(*l)).toInt()], *l, foundCls);
        findXorMatch(solver.watches[(*l).toInt()], ~(*l), foundCls);
    }
    //if (foundCls.size() > 3)
    //    std::cout << "size: " << foundCls.size() << std::endl;


    if (foundCls.foundAll()) {
        Xor thisXor(cl, rhs);
        assert(xorIndex.size() > cl[0].var());
        const vector<uint32_t>& whereToFind = xorIndex[cl[0].var()];
        bool found = false;
        for (vector<uint32_t>::const_iterator it = whereToFind.begin(), end = whereToFind.end(); it != end; it++) {
            if (xors[*it] == thisXor) {
                found = true;
                break;
            }
        }
        if (!found) {
            xors.push_back(thisXor);
            uint32_t thisXorIndex = xors.size()-1;
            for (const Lit *l = cl.getData(), *end = cl.getDataEnd(); l != end; l++) {
                xorIndex[l->var()].push_back(thisXorIndex);
            }

            /*if (!foundCls.allTheSameSize(clauseData)) {
                std::cout << "--- XOR---" << std::endl;
                const vector<ClAndBin>& cls = foundCls.getClauses();
                for (uint32_t i = 0; i < cls.size(); i++) {
                    std::cout << cls[i].print(clauses) << std::endl;
                }
                std::cout << "--- XOR END ---" << std::endl;
            }*/
        }
    }

    for (const Lit *l = cl.getData(), *end = cl.getDataEnd(); l != end; l++, i++) {
        seen[l->var()] = 0;
    }
}

void Subsumer::findXorMatch(const vec2<Watched>& ws, const Lit lit, FoundXors& foundCls) const
{
    for (vec2<Watched>::const_iterator it = ws.getData(), end = ws.getDataEnd(); it != end; it++)  {
        if (it->isBinary()
            && seen[it->getOtherLit().var()]
            && !foundCls.alreadyInside(lit, it->getOtherLit()))
        {
            foundCls.add(lit, it->getOtherLit());
        }
    }
}

void Subsumer::findXorMatch(const vector<ClauseSimp>& occ, FoundXors& foundCls) const
{
    for (vector<ClauseSimp>::const_iterator it = occ.begin(), end = occ.end(); it != end; it++) {
        if (clauseData[it->index].size <= foundCls.getSize()
            && ((clauseData[it->index].abst|foundCls.getAbst())== foundCls.getAbst())
            && !foundCls.alreadyInside(*it)
        ) {
            Clause& cl = *clauses[it->index];

            bool rhs = true;
            uint32_t i = 0;
            for (const Lit *l = cl.getData(), *end = cl.getDataEnd(); l != end; l++, i++) {
                if (!seen[l->var()]) goto end;
                rhs ^= l->sign();
            }
            //either the invertedness has to match, or the size must be smaller
            if (rhs != foundCls.getRHS() && cl.size() == foundCls.getSize()) continue;

            foundCls.add(it->index, cl);
            end:;
        }
    }
}
