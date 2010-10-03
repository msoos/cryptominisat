/****************************************************************************************[Solver.C]
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
CryptoMiniSat -- Copyright (c) 2009 Mate Soos
glucose -- Gilles Audemard, Laurent Simon (2008)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include "Solver.h"
#include <cmath>
#include <string.h>
#include <algorithm>
#include <limits.h>
#include <vector>
#include <iomanip>

#include "Clause.h"
#include "time_mem.h"

#include "VarReplacer.h"
#include "FindUndef.h"
#include "XorFinder.h"
#include "ClauseCleaner.h"
#include "RestartTypeChooser.h"
#include "FailedVarSearcher.h"
#include "Subsumer.h"
#include "PartHandler.h"
#include "XorSubsumer.h"
#include "StateSaver.h"
#include "UselessBinRemover.h"
#include "OnlyNonLearntBins.h"

#ifdef USE_GAUSS
#include "Gaussian.h"
#include "MatrixFinder.h"
#endif //USE_GAUSS

#ifdef _MSC_VER
#define __builtin_prefetch(a,b,c)
//#define __builtin_prefetch(a,b)
#endif //_MSC_VER

//#define DEBUG_UNCHECKEDENQUEUE_LEVEL0
//#define VERBOSE_DEBUG_POLARITIES
//#define DEBUG_DYNAMIC_RESTART

//=================================================================================================
// Constructor/Destructor:


/**
@brief Sets a sane default config and allocates handler classes
*/
Solver::Solver() :
        // Parameters: (formerly in 'SearchParams')
        random_var_freq(0.02)
        , clause_decay (1 / 0.999)
        , restart_first(100), restart_inc(1.5), learntsize_factor((double)1/(double)3), learntsize_inc(1)

        // More parameters:
        //
        , expensive_ccmin  (true)
        , polarity_mode    (polarity_auto)
        , verbosity        (0)
        , restrictedPickBranch(0)
        , findNormalXors   (true)
        , findBinaryXors   (true)
        , regFindBinaryXors(true)
        , doReplace        (true)
        , conglomerateXors (true)
        , heuleProcess     (true)
        , schedSimplification(true)
        , doSubsumption    (true)
        , doXorSubsumption (true)
        , doPartHandler    (true)
        , doHyperBinRes    (true)
        , doBlockedClause  (true)
        , doVarElim        (true)
        , doSubsume1       (true)
        , doAsymmBranch    (true)
        , doAsymmBranchReg (true)
        , doSortWatched    (true)
        , doMinimLearntMore(true)
        , doMinimLMoreRecur(false)
        , failedVarSearch  (true)
        , addExtraBins     (true)
        , remUselessBins   (true)
        , regRemUselessBins(true)
        , subsWNonExistBins(true)
        , regSubsWNonExistBins(true)
        , needToInterrupt  (false)
        , needToDumpLearnts(false)
        , needToDumpOrig   (false)
        , maxDumpLearntsSize(std::numeric_limits<uint32_t>::max())
        , libraryUsage     (true)
        , greedyUnbound    (false)
        , fixRestartType   (auto_restart)

        // Statistics: (formerly in 'SolverStats')
        //
        , starts(0), dynStarts(0), staticStarts(0), fullStarts(0), decisions(0), rnd_decisions(0), propagations(0), conflicts(0)
        , clauses_literals(0), learnts_literals(0), max_literals(0), tot_literals(0)
        , nbGlue2(0), nbBin(0), lastNbBin(0), becameBinary(0), lastSearchForBinaryXor(0), nbReduceDB(0)
        , improvedClauseNo(0), improvedClauseSize(0)
        , numShrinkedClause(0), numShrinkedClauseLits(0)
        , moreRecurMinLDo(0), moreRecurMinLDoLit(0), moreRecurMinLStop (0)

        #ifdef USE_GAUSS
        , sum_gauss_called (0)
        , sum_gauss_confl  (0)
        , sum_gauss_prop   (0)
        , sum_gauss_unit_truths (0)
        #endif //USE_GAUSS
        , ok               (true)
        , var_inc          (128)
        , cla_inc          (1)

        , numCleanedLearnts(1)
        , nbClBeforeRed    (NBCLAUSESBEFOREREDUCE)
        , nbCompensateSubsumer (0)

        , qhead            (0)
        , order_heap       (VarOrderLt(activity))
        , mtrand((unsigned long int)0)
        , maxRestarts(UINT_MAX)
        , MYFLAG           (0)
        #ifdef STATS_NEEDED
        , logger(verbosity)
        , dynamic_behaviour_analysis(false) //do not document the proof as default
        #endif
        , learnt_clause_group(0)
        , libraryCNFFile   (NULL)
        , restartType      (static_restart)
        , lastSelectedRestartType (static_restart)
        , simplifying      (false)
        , simpDB_assigns   (-1)
        , simpDB_props     (0)
{
    varReplacer = new VarReplacer(*this);
    clauseCleaner = new ClauseCleaner(*this);
    failedVarSearcher = new FailedVarSearcher(*this);
    partHandler = new PartHandler(*this);
    subsumer = new Subsumer(*this);
    xorSubsumer = new XorSubsumer(*this);
    restartTypeChooser = new RestartTypeChooser(*this);
    learntsFilename = new char[500];
    learntsFilename[0] = '\0';
    origFilename = new char[500];
    origFilename[0] = '\0';

    #ifdef USE_GAUSS
    matrixFinder = new MatrixFinder(*this);
    #endif //USE_GAUSS

    #ifdef STATS_NEEDED
    logger.setSolver(this);
    #endif
}

/**
@brief Frees clauses and frees all allocated hander classes
*/
Solver::~Solver()
{
    #ifdef USE_GAUSS
    clearGaussMatrixes();
    delete matrixFinder;
    #endif

    #ifndef USE_POOLS
    for (uint32_t i = 0; i != learnts.size(); i++) clauseAllocator.clauseFree(learnts[i]);
    learnts.clear();
    for (uint32_t i = 0; i != clauses.size(); i++) clauseAllocator.clauseFree(clauses[i]);
    clauses.clear();
    for (uint32_t i = 0; i != binaryClauses.size(); i++) clauseAllocator.clauseFree(binaryClauses[i]);
    binaryClauses.clear();
    for (uint32_t i = 0; i != xorclauses.size(); i++) clauseAllocator.clauseFree(xorclauses[i]);
    xorclauses.clear();
    for (uint32_t i = 0; i != freeLater.size(); i++) clauseAllocator.clauseFree(freeLater[i]);
    freeLater.clear();
    #endif //USE_POOLS

    delete varReplacer;
    delete clauseCleaner;
    delete failedVarSearcher;
    delete partHandler;
    delete subsumer;
    delete xorSubsumer;
    delete restartTypeChooser;
    delete [] learntsFilename;

    if (libraryCNFFile)
        fclose(libraryCNFFile);
}

//=================================================================================================
// Minor methods:


/**
@brief Creates a new SAT variable in the solver

This entails making the datastructures large enough to fit the new variable
in all internal datastructures as well as all datastructures used in
classes used inside Solver

@p dvar The new variable should be used as a decision variable?
   NOTE: this has effects on the meaning of a SATISFIABLE result
*/
Var Solver::newVar(bool dvar)
{
    Var v = nVars();
    watches   .push();          // (list for positive literal)
    watches   .push();          // (list for negative literal)
    reason    .push(PropagatedFrom());
    assigns   .push(l_Undef);
    level     .push(-1);
    activity  .push(0);
    seen      .push_back(0);
    seen      .push_back(0);
    seen2     .push_back(0);
    seen2     .push_back(0);
    permDiff  .push(0);

    polarity  .push_back(defaultPolarity());
    #ifdef USE_OLD_POLARITIES
    oldPolarity.push_back(defaultPolarity());
    #endif //USE_OLD_POLARITIES

    decision_var.push_back(dvar);
    insertVarOrder(v);

    varReplacer->newVar();
    if (doPartHandler) partHandler->newVar();
    if (doSubsumption || subsWNonExistBins || regSubsWNonExistBins) subsumer->newVar();
    if (doXorSubsumption) xorSubsumer->newVar();

    insertVarOrder(v);

    #ifdef STATS_NEEDED
    if (dynamic_behaviour_analysis)
        logger.new_var(v);
    #endif

    if (libraryCNFFile)
        fprintf(libraryCNFFile, "c Solver::newVar() called\n");

    return v;
}

/**
@brief Adds an xor clause to the problem

Should ONLY be called from internally. This is different from the extenal
xor clause-adding function addXorClause() in that it assumes that the variables
inside are decision variables, have not been replaced, eliminated, etc.
*/
template<class T>
XorClause* Solver::addXorClauseInt(T& ps, bool xorEqualFalse, const uint32_t group)
{
    assert(qhead == trail.size());
    assert(decisionLevel() == 0);

    if (ps.size() > (0x01UL << 18)) {
        std::cout << "Too long clause!" << std::endl;
        exit(-1);
    }
    std::sort(ps.getData(), ps.getDataEnd());
    Lit p;
    uint32_t i, j;
    for (i = j = 0, p = lit_Undef; i != ps.size(); i++) {
        if (ps[i].var() == p.var()) {
            //added, but easily removed
            j--;
            p = lit_Undef;
            if (!assigns[ps[i].var()].isUndef())
                xorEqualFalse ^= assigns[ps[i].var()].getBool();
        } else if (assigns[ps[i].var()].isUndef()) //just add
            ps[j++] = p = ps[i];
        else //modify xorEqualFalse instead of adding
            xorEqualFalse ^= (assigns[ps[i].var()].getBool());
    }
    ps.shrink(i - j);

    switch(ps.size()) {
        case 0: {
            if (!xorEqualFalse) ok = false;
            return NULL;
        }
        case 1: {
            uncheckedEnqueue(Lit(ps[0].var(), xorEqualFalse));
            ok = (propagate().isNULL());
            return NULL;
        }
        case 2: {
            #ifdef VERBOSE_DEBUG
            cout << "--> xor is 2-long, replacing var " << ps[0].var()+1 << " with " << (!xorEqualFalse ? "-" : "") << ps[1].var()+1 << endl;
            #endif

            ps[0] = ps[0].unsign();
            ps[1] = ps[1].unsign();
            varReplacer->replace(ps, xorEqualFalse, group);
            return NULL;
        }
        default: {
            XorClause* c = clauseAllocator.XorClause_new(ps, xorEqualFalse, group);
            attachClause(*c);
            return c;
        }
    }
}

template XorClause* Solver::addXorClauseInt(vec<Lit>& ps, bool xorEqualFalse, const uint32_t group);
template XorClause* Solver::addXorClauseInt(XorClause& ps, bool xorEqualFalse, const uint32_t group);

/**
@brief Adds an xor clause to the problem

Calls addXorClauseInt() for the heavy-lifting. Basically, this does a bit
of debug-related stuff (see "libraryCNFFile"), and then checks if any of the
variables have been eliminated, replaced, etc. If so, it treats it correctly,
and then calls addXorClauseInt() to actually add the xor clause.

@p ps[inout] The VARIABLES in the xor clause. Beware, there must be NO signs
            here: ALL must be unsigned (.sign() == false). Values passed here
            WILL be changed, ordered, removed, etc!
@p xorEqualFalse The xor must be equal to TRUE or false?
*/
template<class T>
bool Solver::addXorClause(T& ps, bool xorEqualFalse, const uint32_t group, const char* group_name)
{
    assert(decisionLevel() == 0);
    if (ps.size() > (0x01UL << 18)) {
        std::cout << "Too long clause!" << std::endl;
        exit(-1);
    }

    if (libraryCNFFile) {
        fprintf(libraryCNFFile, "x");
        for (uint32_t i = 0; i < ps.size(); i++) ps[i].print(libraryCNFFile);
        fprintf(libraryCNFFile, "0\n");
    }

    #ifdef STATS_NEEDED
    if (dynamic_behaviour_analysis) {
        logger.set_group_name(group, group_name);
        learnt_clause_group = std::max(group+1, learnt_clause_group);
    }
    #endif

    if (!ok)
        return false;
    assert(qhead == trail.size());
    #ifndef NDEBUG
    for (Lit *l = ps.getData(), *end = ps.getDataEnd(); l != end; l++) {
        assert(l->var() < nVars() && "Clause inserted, but variable inside has not been declared with newVar()!");
    }
    #endif

    // Check if clause is satisfied and remove false/duplicate literals:
    if (varReplacer->getNumLastReplacedVars() || subsumer->getNumElimed() || xorSubsumer->getNumElimed()) {
        for (uint32_t i = 0; i != ps.size(); i++) {
            if (subsumer->getVarElimed()[ps[i].var()] && !subsumer->unEliminate(ps[i].var()))
                return false;
            else if (xorSubsumer->getVarElimed()[ps[i].var()] && !xorSubsumer->unEliminate(ps[i].var()))
                return false;
            else {
                Lit otherLit = varReplacer->getReplaceTable()[ps[i].var()];
                if (otherLit.var() != ps[i].var()) {
                    ps[i] = Lit(otherLit.var(), false);
                    xorEqualFalse ^= otherLit.sign();
                }
            }
        }
    }

    XorClause* c = addXorClauseInt(ps, xorEqualFalse, group);
    if (c != NULL) xorclauses.push(c);

    return ok;
}

template bool Solver::addXorClause(vec<Lit>& ps, bool xorEqualFalse, const uint32_t group, const char* group_name);
template bool Solver::addXorClause(XorClause& ps, bool xorEqualFalse, const uint32_t group, const char* group_name);

/**
@brief Adds a leant clause to the problem

It is not a very good function, so if really needed, it should be corrected.
It uses addClauseInt() for the heavy-lifting, and then makes the clause learnt.
Not exactly a perfect method. For example, it assumes that the "ps" left by
addClauseInt() conveys some meaningful message, which it might not, for example.

\todo Correct this method, make it more robust
*/
template<class T>
bool Solver::addLearntClause(T& ps, const uint32_t glue, const float miniSatActivity, const uint32_t group)
{
    Clause* c = addClauseInt(ps, group);
    if (c == NULL) return ok;

    //compensate for addClauseInt's attachClause, which doesn't know
    //that this is a learnt clause.
    clauses_literals -= c->size();
    learnts_literals += c->size();

    c->makeLearnt(glue, miniSatActivity);
    if (c->size() > 2) learnts.push(c);
    else {
        nbBin++;
        binaryClauses.push(c);
    }
    return ok;
}
template bool Solver::addLearntClause(Clause& ps, const uint32_t glue, const float miniSatActivity, const uint32_t group);
template bool Solver::addLearntClause(vec<Lit>& ps, const uint32_t glue, const float miniSatActivity, const uint32_t group);

/**
@brief Adds a clause to the problem. Should ONLY be called internally

This code is very specific in that it must NOT be called with varibles in
"ps" that have been replaced, eliminated, etc. Also, it must not be called
when the solver is in an UNSAT (!ok) state, for example. Use it carefully,
and only internally
*/
template <class T>
Clause* Solver::addClauseInt(T& ps, uint32_t group)
{
    assert(ok);

    std::sort(ps.getData(), ps.getDataEnd());
    Lit p = lit_Undef;
    uint32_t i, j;
    for (i = j = 0; i != ps.size(); i++) {
        if (value(ps[i]).getBool() || ps[i] == ~p)
            return NULL;
        else if (value(ps[i]) != l_False && ps[i] != p)
            ps[j++] = p = ps[i];
    }
    ps.shrink(i - j);

    if (ps.size() == 0) {
        ok = false;
        return NULL;
    } else if (ps.size() == 1) {
        uncheckedEnqueue(ps[0]);
        ok = (propagate().isNULL());
        return NULL;
    }

    Clause* c = clauseAllocator.Clause_new(ps, group);
    attachClause(*c);

    return c;
}

template Clause* Solver::addClauseInt(Clause& ps, const uint32_t group);
template Clause* Solver::addClauseInt(vec<Lit>& ps, const uint32_t group);

/**
@brief Adds a clause to the problem. Calls addClauseInt() for heavy-lifting

Does some debug-related stuff (see "libraryCNFFile"), and checks whether the
variables of the literals in "ps" have been eliminated/replaced etc. If so,
it acts on them such that they are correct, and calls addClauseInt() to do
the heavy-lifting
*/
template<class T>
bool Solver::addClause(T& ps, const uint32_t group, const char* group_name)
{
    assert(decisionLevel() == 0);
    if (ps.size() > (0x01UL << 18)) {
        std::cout << "Too long clause!" << std::endl;
        exit(-1);
    }

    if (libraryCNFFile) {
        for (uint32_t i = 0; i != ps.size(); i++) ps[i].print(libraryCNFFile);
        fprintf(libraryCNFFile, "0\n");
    }

    #ifdef STATS_NEEDED
    if (dynamic_behaviour_analysis) {
        logger.set_group_name(group, group_name);
        learnt_clause_group = std::max(group+1, learnt_clause_group);
    }
    #endif

    if (!ok)
        return false;
    assert(qhead == trail.size());
    #ifndef NDEBUG
    for (Lit *l = ps.getData(), *end = ps.getDataEnd(); l != end; l++) {
        assert(l->var() < nVars() && "Clause inserted, but variable inside has not been declared with Solver::newVar() !");
    }
    #endif

    // Check if clause is satisfied and remove false/duplicate literals:
    if (varReplacer->getNumLastReplacedVars() || subsumer->getNumElimed() || xorSubsumer->getNumElimed()) {
        for (uint32_t i = 0; i != ps.size(); i++) {
            ps[i] = varReplacer->getReplaceTable()[ps[i].var()] ^ ps[i].sign();
            if (subsumer->getVarElimed()[ps[i].var()] && !subsumer->unEliminate(ps[i].var()))
                return false;
            if (xorSubsumer->getVarElimed()[ps[i].var()] && !xorSubsumer->unEliminate(ps[i].var()))
                return false;
        }
    }

    Clause* c = addClauseInt(ps, group);
    if (c != NULL) {
        if (c->size() > 2)
            clauses.push(c);
        else
            binaryClauses.push(c);
    }

    return ok;
}

template bool Solver::addClause(vec<Lit>& ps, const uint32_t group, const char* group_name);
template bool Solver::addClause(Clause& ps, const uint32_t group, const char* group_name);

/**
@brief Attaches an xor clause to the watchlists

The xor clause must be larger than 2, since a 2-long XOR clause is a varible
replacement instruction, really.
*/
void Solver::attachClause(XorClause& c)
{
    assert(c.size() > 2);
    #ifdef DEBUG_ATTACH
    assert(assigns[c[0].var()] == l_Undef);
    assert(assigns[c[1].var()] == l_Undef);
    #endif //DEBUG_ATTACH

    watches[Lit(c[0].var(), false).toInt()].push(clauseAllocator.getOffset((Clause*)&c));
    watches[Lit(c[0].var(), true).toInt()].push(clauseAllocator.getOffset((Clause*)&c));
    watches[Lit(c[1].var(), false).toInt()].push(clauseAllocator.getOffset((Clause*)&c));
    watches[Lit(c[1].var(), true).toInt()].push(clauseAllocator.getOffset((Clause*)&c));

    clauses_literals += c.size();
}

/**
@brief Attach normal a clause to the watchlists

Handles 2, 3 and >3 clause sizes differently and specially
*/
void Solver::attachClause(Clause& c)
{
    assert(c.size() > 1);
    uint32_t index0 = (~c[0]).toInt();
    uint32_t index1 = (~c[1]).toInt();

    if (c.size() == 2) {
        watches[index0].push(Watched(c[1]));
        watches[index1].push(Watched(c[0]));
    } else if (c.size() == 3) {
        watches[index0].push(Watched(c[1], c[2]));
        watches[index1].push(Watched(c[0], c[2]));
        watches[(~c[2]).toInt()].push(Watched(c[0], c[1]));
    } else {
        ClauseOffset offset = clauseAllocator.getOffset(&c);
        watches[index0].push(Watched(offset, c[c.size()/2]));
        watches[index1].push(Watched(offset, c[c.size()/2]));
    }

    if (c.learnt())
        learnts_literals += c.size();
    else
        clauses_literals += c.size();
}

/**
@brief Calls detachModifiedClause to do the heavy-lifting
*/
void Solver::detachClause(const XorClause& c)
{
    detachModifiedClause(c[0].var(), c[1].var(), c.size(), &c);
}

/**
@brief Calls detachModifiedClause to do the heavy-lifting
*/
void Solver::detachClause(const Clause& c)
{
    detachModifiedClause(c[0], c[1], (c.size() == 3) ? c[2] : lit_Undef,  c.size(), &c);
}

/**
@brief Detaches a (potentially) modified clause

The first two literals might have chaned through modification, so they are
passed along as arguments -- they are needed to find the correct place where
the clause is
*/
void Solver::detachModifiedClause(const Lit lit1, const Lit lit2, const Lit lit3, const uint32_t origSize, const Clause* address)
{
    assert(origSize > 1);

    if (origSize == 2) {
        assert(findWBin(watches[(~lit1).toInt()], lit2));
        assert(findWBin(watches[(~lit2).toInt()], lit1));
        removeWBin(watches[(~lit1).toInt()], lit2);
        removeWBin(watches[(~lit2).toInt()], lit1);
    } else {
        ClauseOffset offset = clauseAllocator.getOffset(address);
        if (origSize == 3) {
            //The clause might have been longer, and has only recently
            //became 3-long. Check, and detach accordingly
            if (findWCl(watches[(~lit1).toInt()], offset)) goto fullClause;

            removeWTri(watches[(~lit1).toInt()], lit2, lit3);
            removeWTri(watches[(~lit2).toInt()], lit1, lit3);
            removeWTri(watches[(~lit3).toInt()], lit1, lit2);
        } else {
            fullClause:
            assert(findWCl(watches[(~lit1).toInt()], offset));
            assert(findWCl(watches[(~lit2).toInt()], offset));
            removeWCl(watches[(~lit1).toInt()], offset);
            removeWCl(watches[(~lit2).toInt()], offset);
        }
    }

    if (address->learnt())
        learnts_literals -= origSize;
    else
        clauses_literals -= origSize;
}

/**
@brief Detaches a (potentially) modified xor clause

The first two vars might have chaned through modification, so they are passed
along as arguments.
*/
void Solver::detachModifiedClause(const Var var1, const Var var2, const uint32_t origSize, const XorClause* address)
{
    assert(origSize > 2);

    ClauseOffset offset = clauseAllocator.getOffset(address);
    assert(findWXCl(watches[Lit(var1, false).toInt()], offset));
    assert(findWXCl(watches[Lit(var1, true).toInt()], offset));
    assert(findWXCl(watches[Lit(var2, false).toInt()], offset));
    assert(findWXCl(watches[Lit(var2, true).toInt()], offset));

    removeWXCl(watches[Lit(var1, false).toInt()], offset);
    removeWXCl(watches[Lit(var1, true).toInt()], offset);
    removeWXCl(watches[Lit(var2, false).toInt()], offset);
    removeWXCl(watches[Lit(var2, true).toInt()], offset);

    clauses_literals -= origSize;
}

/**
@brief Revert to the state at given level

Also reverts all stuff in Gass-elimination, as well as resetting the old
default polarities if USE_OLD_POLARITIES is set (which is by default NOT set).
*/
void Solver::cancelUntil(int level)
{
    #ifdef VERBOSE_DEBUG
    cout << "Canceling until level " << level;
    if (level > 0) cout << " sublevel: " << trail_lim[level];
    cout << endl;
    #endif

    if ((int)decisionLevel() > level) {

        #ifdef USE_GAUSS
        for (vector<Gaussian*>::iterator gauss = gauss_matrixes.begin(), end= gauss_matrixes.end(); gauss != end; gauss++)
            (*gauss)->canceling(trail_lim[level]);
        #endif //USE_GAUSS

        for (int sublevel = trail.size()-1; sublevel >= (int)trail_lim[level]; sublevel--) {
            Var var = trail[sublevel].var();
            #ifdef VERBOSE_DEBUG
            cout << "Canceling var " << var+1 << " sublevel: " << sublevel << endl;
            #endif
            #ifdef USE_OLD_POLARITIES
            polarity[var] = oldPolarity[var];
            #endif //USE_OLD_POLARITIES
            assigns[var] = l_Undef;
            insertVarOrder(var);
        }
        qhead = trail_lim[level];
        trail.shrink_(trail.size() - trail_lim[level]);
        trail_lim.shrink_(trail_lim.size() - level);
    }

    #ifdef VERBOSE_DEBUG
    cout << "Canceling finished. (now at level: " << decisionLevel() << " sublevel: " << trail.size()-1 << ")" << endl;
    #endif
}

#ifdef USE_GAUSS
void Solver::clearGaussMatrixes()
{
    for (uint32_t i = 0; i < gauss_matrixes.size(); i++)
        delete gauss_matrixes[i];
    gauss_matrixes.clear();
    for (uint32_t i = 0; i != freeLater.size(); i++)
        clauseAllocator.clauseFree(freeLater[i]);
    freeLater.clear();
}
#endif //USE_GAUSS

/**
@brief Returns what polarity[] should be set as default based on polarity_mode

since polarity is filled with Lit::sign() , "true" here means an inverted
signed-ness, i.e. a FALSE default value. And vice-versa
*/
inline bool Solver::defaultPolarity()
{
    switch(polarity_mode) {
        case polarity_false:
            return true;
        case polarity_true:
            return false;
        case polarity_rnd:
            return mtrand.randInt(1);
        case polarity_auto:
            return true;
        default:
            assert(false);
    }

    return true;
}

/**
@brief Tally votes for a default TRUE or FALSE value for the variable using the Jeroslow-Wang method

@p votes[inout] Votes are tallied at this place for each variable
@p cs The clause to tally votes for
*/
void Solver::tallyVotes(const vec<Clause*>& cs, vector<double>& votes) const
{
    for (const Clause * const*it = cs.getData(), * const*end = it + cs.size(); it != end; it++) {
        const Clause& c = **it;
        if (c.learnt()) continue;

        double divider;
        if (c.size() > 63) divider = 0.0;
        else divider = 1.0/(double)((uint64_t)1<<(c.size()-1));

        for (const Lit *it2 = &c[0], *end2 = it2 + c.size(); it2 != end2; it2++) {
            if (it2->sign()) votes[it2->var()] += divider;
            else votes[it2->var()] -= divider;
        }
    }
}

/**
@brief Tally votes a default TRUE or FALSE value for the variable using the Jeroslow-Wang method

For XOR clause, we simply add some weight for a FALSE default, i.e. being in
xor clauses makes the variabe more likely to be FALSE by default
*/
void Solver::tallyVotes(const vec<XorClause*>& cs, vector<double>& votes) const
{
    for (const XorClause * const*it = cs.getData(), * const*end = it + cs.size(); it != end; it++) {
        const XorClause& c = **it;
        double divider;
        if (c.size() > 63) divider = 0.0;
        else divider = 1.0/(double)((uint64_t)1<<(c.size()-1));

        for (const Lit *it2 = &c[0], *end2 = it2 + c.size(); it2 != end2; it2++)
            votes[it2->var()] += divider;
    }
}

/**
@brief Tallies votes for a TRUE/FALSE default polarity using Jeroslow-Wang

Voting is only used if polarity_mode is "polarity_auto". This is the default.
Uses the tallyVotes() functions to tally the votes
*/
void Solver::calculateDefaultPolarities()
{
    #ifdef VERBOSE_DEBUG_POLARITIES
    std::cout << "Default polarities: " << std::endl;
    #endif

    assert(decisionLevel() == 0);
    if (polarity_mode == polarity_auto) {
        double time = cpuTime();

        vector<double> votes;
        votes.resize(nVars(), 0.0);

        tallyVotes(clauses, votes);
        tallyVotes(binaryClauses, votes);
        tallyVotes(varReplacer->getClauses(), votes);
        tallyVotes(xorclauses, votes);

        Var i = 0;
        uint32_t posPolars = 0;
        uint32_t undecidedPolars = 0;
        for (vector<double>::const_iterator it = votes.begin(), end = votes.end(); it != end; it++, i++) {
            polarity[i] = (*it >= 0.0);
            posPolars += (*it < 0.0);
            undecidedPolars += (*it == 0.0);
            #ifdef VERBOSE_DEBUG_POLARITIES
            std::cout << !defaultPolarities[i] << ", ";
            #endif //VERBOSE_DEBUG_POLARITIES
        }

        if (verbosity >= 2) {
            std::cout << "c Calc default polars - "
            << " time: " << std::fixed << std::setw(6) << std::setprecision(2) << cpuTime()-time << " s"
            << " pos: " << std::setw(7) << posPolars
            << " undec: " << std::setw(7) << undecidedPolars
            << " neg: " << std::setw(7) << nVars()-  undecidedPolars - posPolars
            << std:: endl;
        }
    } else {
        std::fill(polarity.begin(), polarity.end(), defaultPolarity());
    }

    #ifdef VERBOSE_DEBUG_POLARITIES
    std::cout << std::endl;
    #endif //VERBOSE_DEBUG_POLARITIES
}

//=================================================================================================
// Major methods:


/**
@brief Picks a branching variable and its value (True/False)

We do three things here:
-# Try to do random decision (rare, less than 2%)
-# Try acitivity-based decision

Then, we pick a sign (True/False):
\li If we are in search-burst mode ("simplifying" is set), we pick a sign
totally randomly
\li If RANDOM_LOOKAROUND_SEARCHSPACE is set, we take the previously saved
polarity, and with some chance, flip it
\li Otherwise, we simply take the saved polarity
*/
Lit Solver::pickBranchLit()
{
    #ifdef VERBOSE_DEBUG
    cout << "decision level: " << decisionLevel() << " ";
    #endif

    Var next = var_Undef;

    bool random = mtrand.randDblExc() < random_var_freq;

    // Random decision:
    if (random && !order_heap.empty()) {
        if (restrictedPickBranch == 0) next = order_heap[mtrand.randInt(order_heap.size()-1)];
        else next = order_heap[mtrand.randInt(std::min((uint32_t)order_heap.size()-1, restrictedPickBranch))];

        if (assigns[next] == l_Undef && decision_var[next])
            rnd_decisions++;
    }

    // Activity based decision:
    while (next == var_Undef || assigns[next] != l_Undef || !decision_var[next])
        if (order_heap.empty()) {
            next = var_Undef;
            break;
        } else {
            next = order_heap.removeMin();
        }

    //if "simplifying" is set, i.e. if we are in a burst-search mode, then
    //randomly pick a sign. Otherwise, if RANDOM_LOOKAROUND_SEARCHSPACE is
    //defined, we check the default polarity, and we may change it a bit
    //randomly based on the average branch depth. Otherwise, we just go for the
    //polarity that has been saved
    bool sign;
    if (next != var_Undef) {
        if (simplifying && random)
            sign = mtrand.randInt(1);
        #ifdef RANDOM_LOOKAROUND_SEARCHSPACE
        else if (avgBranchDepth.isvalid())
            sign = polarity[next] ^ (mtrand.randInt(avgBranchDepth.getavg() * ((lastSelectedRestartType == static_restart) ? 2 : 1) ) == 1);
        #endif
        else
            sign = polarity[next];
    }

    assert(next == var_Undef || value(next) == l_Undef);

    if (next == var_Undef) {
        #ifdef VERBOSE_DEBUG
        cout << "SAT!" << endl;
        #endif
        return lit_Undef;
    } else {
        Lit lit(next,sign);
        #ifdef VERBOSE_DEBUG
        assert(decision_var[lit.var()]);
        cout << "decided on: " << lit.var()+1 << " to set:" << !lit.sign() << endl;
        #endif
        return lit;
    }
}

/**
@brief Checks subsumption. Used in on-the-fly subsumption code

Assumes 'seen' is cleared (will leave it cleared)
*/
template<class T1, class T2>
bool subset(const T1& A, const T2& B, vector<bool>& seen)
{
    for (uint32_t i = 0; i != B.size(); i++)
        seen[B[i].toInt()] = 1;
    for (uint32_t i = 0; i != A.size(); i++) {
        if (!seen[A[i].toInt()]) {
            for (uint32_t i = 0; i != B.size(); i++)
                seen[B[i].toInt()] = 0;
            return false;
        }
    }
    for (uint32_t i = 0; i != B.size(); i++)
        seen[B[i].toInt()] = 0;
    return true;
}


/**
@brief    Analyze conflict and produce a reason clause.

Pre-conditions:
\li  'out_learnt' is assumed to be cleared.
\li Current decision level must be greater than root level.

Post-conditions:
\li 'out_learnt[0]' is the asserting literal at level 'out_btlevel'.

Effect: Will undo part of the trail, upto but not beyond the assumption of the
current decision level.

@return NULL if the conflict doesn't on-the-fly subsume the last clause, and
the pointer of the clause if it does
*/
Clause* Solver::analyze(PropagatedFrom confl, vec<Lit>& out_learnt, int& out_btlevel, uint32_t &nbLevels, const bool update)
{
    int pathC = 0;
    Lit p     = lit_Undef;

    // Generate conflict clause:
    //
    out_learnt.push();      // (leave room for the asserting literal)
    int index   = trail.size() - 1;
    out_btlevel = 0;

    PropagatedFrom oldConfl;

    do {
        assert(!confl.isNULL());          // (otherwise should be UIP)

        if (update && restartType == static_restart && confl.isClause() && confl.getClause()->learnt())
            claBumpActivity(*confl.getClause());

        for (uint32_t j = (p == lit_Undef) ? 0 : 1, size = confl.size(); j != size; j++) {
            Lit q;
            if (j == 0 && !confl.isClause()) q = failBinLit;
            else q = confl[j];
            const Var my_var = q.var();

            if (!seen[my_var] && level[my_var] > 0) {
                varBumpActivity(my_var);
                seen[my_var] = 1;
                assert(level[my_var] <= (int)decisionLevel());
                if (level[my_var] >= (int)decisionLevel()) {
                    pathC++;
                    #ifdef UPDATEVARACTIVITY
                    if (lastSelectedRestartType == dynamic_restart
                        && reason[q.var()].isClause()
                        && !reason[q.var()].isNULL()
                        && reason[q.var()].getClause()->learnt())
                        lastDecisionLevel.push(q.var());
                    #endif
                } else {
                    out_learnt.push(q);
                    if (level[my_var] > out_btlevel)
                        out_btlevel = level[my_var];
                }
            }
        }

        // Select next clause to look at:
        while (!seen[trail[index--].var()]);
        p     = trail[index+1];
        oldConfl = confl;
        confl = reason[p.var()];
        if (confl.isClause()) __builtin_prefetch(confl.getClause(), 1, 0);
        seen[p.var()] = 0;
        pathC--;

    } while (pathC > 0);
    assert(pathC == 0);
    out_learnt[0] = ~p;

    // Simplify conflict clause:
    //
    uint32_t i, j;
    if (expensive_ccmin) {
        uint32_t abstract_level = 0;
        for (i = 1; i < out_learnt.size(); i++)
            abstract_level |= abstractLevel(out_learnt[i].var()); // (maintain an abstraction of levels involved in conflict)

        out_learnt.copyTo(analyze_toclear);
        for (i = j = 1; i < out_learnt.size(); i++)
            if (reason[out_learnt[i].var()].isNULL() || !litRedundant(out_learnt[i], abstract_level))
                out_learnt[j++] = out_learnt[i];
    } else {
        out_learnt.copyTo(analyze_toclear);
        for (i = j = 1; i < out_learnt.size(); i++) {
            PropagatedFrom c(reason[out_learnt[i].var()]);

            for (uint32_t k = 1, size = c.size(); k < size; k++) {
                if (!seen[c[k].var()] && level[c[k].var()] > 0) {
                    out_learnt[j++] = out_learnt[i];
                    break;
                }
            }
        }
    }
    max_literals += out_learnt.size();
    out_learnt.shrink(i - j);
    for (uint32_t j = 0; j != analyze_toclear.size(); j++)
        seen[analyze_toclear[j].var()] = 0;    // ('seen[]' is now cleared)

    if (doMinimLearntMore && out_learnt.size() > 1) minimiseLeartFurther(out_learnt);
    tot_literals += out_learnt.size();

    // Find correct backtrack level:
    //
    if (out_learnt.size() == 1)
        out_btlevel = 0;
    else {
        uint32_t max_i = 1;
        for (uint32_t i = 2; i < out_learnt.size(); i++)
            if (level[out_learnt[i].var()] > level[out_learnt[max_i].var()])
                max_i = i;
        Lit p             = out_learnt[max_i];
        out_learnt[max_i] = out_learnt[1];
        out_learnt[1]     = p;
        out_btlevel       = level[p.var()];
    }

    nbLevels = calcNBLevels(out_learnt);
    if (lastSelectedRestartType == dynamic_restart) {
        #ifdef UPDATEVARACTIVITY
        for(uint32_t i = 0; i != lastDecisionLevel.size(); i++) {
            PropagatedFrom cl = reason[lastDecisionLevel[i]];
            if (cl.isClause() && cl.getClause()->getGlue() < nbLevels)
                varBumpActivity(lastDecisionLevel[i]);
        }
        lastDecisionLevel.clear();
        #endif
    }

    if (out_learnt.size() == 1) return NULL;

    if (oldConfl.isClause() && !oldConfl.getClause()->isXor()
        && out_learnt.size() < oldConfl.getClause()->size()) {
        if (!subset(out_learnt, *oldConfl.getClause(), seen))
            return NULL;
        improvedClauseNo++;
        improvedClauseSize += oldConfl.getClause()->size() - out_learnt.size();
        return oldConfl.getClause();
    }

    return NULL;
}

/**
@brief Performs on-the-fly self-subsuming resolution

Only uses binary and tertiary clauses already in the watchlists in native
form to carry out the forward-self-subsuming resolution
*/
void Solver::minimiseLeartFurther(vec<Lit>& cl)
{
    //80 million is kind of a hack. It seems that the longer the solving
    //the slower this operation gets. So, limiting the "time" with total
    //number of conflict literals is maybe a good way of doing this
    bool thisDoMinLMoreRecur = doMinimLMoreRecur || (cl.size() <= 5 && tot_literals < 80000000);
    if (thisDoMinLMoreRecur) moreRecurMinLDo++;

    //To count the "amount of time" invested in doing transitive on-the-fly
    //self-subsuming resolution
    uint32_t moreRecurProp = 0;

    for (uint32_t i = 0; i < cl.size(); i++) seen[cl[i].toInt()] = 1;
    for (Lit *l = cl.getData(), *end = cl.getDataEnd(); l != end; l++) {
        if (seen[l->toInt()] == 0) continue;

        Lit lit = *l;

        //Recursively self-subsume-resolve all "a OR c" when "a OR b" as well as
        //"~b OR c" exists.
        if (thisDoMinLMoreRecur && moreRecurProp > 450) {
            //std::cout << "switched off"  << std::endl;
            moreRecurMinLStop++;
            thisDoMinLMoreRecur = false;
        }
        if (thisDoMinLMoreRecur) moreRecurMinLDoLit++;
        if (thisDoMinLMoreRecur) {
            //Don't come back to the starting point
            seen2[(~lit).toInt()] = 1;
            allAddedToSeen2.push(~lit);

            toRecursiveProp.push(lit);
            while(!toRecursiveProp.empty()) {
                Lit thisLit = toRecursiveProp.top();
                toRecursiveProp.pop();
                //watched is messed: lit is in watched[~lit]
                vec<Watched>& ws = watches[(~thisLit).toInt()];
                moreRecurProp += ws.size() +10;
                for (Watched* i = ws.getData(), *end = ws.getDataEnd(); i != end; i++) {
                    if (i->isBinary()) {
                        moreRecurProp += 5;
                        Lit otherLit = i->getOtherLit();
                        //don't do indefinite recursion, and don't remove "a" when doing self-subsuming-resolution with 'a OR b'
                        if (seen2[otherLit.toInt()] != 0 || ~otherLit == lit) break;
                        seen2[otherLit.toInt()] = 1;
                        allAddedToSeen2.push(otherLit);
                        seen[(~otherLit).toInt()] = 0;
                        toRecursiveProp.push(~otherLit);
                    } else {
                        break;
                    }
                }
            }
            assert(toRecursiveProp.empty());

            for (Lit *it = allAddedToSeen2.getData(), *end = allAddedToSeen2.getDataEnd(); it != end; it++) {
                seen2[it->toInt()] = 0;
            }
            allAddedToSeen2.clear();
        }

        //watched is messed: lit is in watched[~lit]
        vec<Watched>& ws = watches[(~lit).toInt()];
        for (Watched* i = ws.getData(), *end = ws.getDataEnd(); i != end; i++) {
            if (!thisDoMinLMoreRecur && i->isBinary()) {
                seen[(~i->getOtherLit()).toInt()] = 0;
                continue;
            }

            if (i->isTriClause()) {
                if (seen[(~i->getOtherLit()).toInt()] && seen[i->getOtherLit2().toInt()]) {
                    seen[(~i->getOtherLit()).toInt()] = 0;
                }
                if (seen[(~i->getOtherLit2()).toInt()] && seen[i->getOtherLit().toInt()]) {
                    seen[(~i->getOtherLit2()).toInt()] = 0;
                }
                continue;
            }

            //watches are mostly sorted, so it's more-or-less OK to break
            //  if non-bi or non-tri is encountered
            break;
        }
    }

    uint32_t removedLits = 0;
    Lit *i = cl.getData();
    Lit *j= i;
    for (Lit* end = cl.getDataEnd(); i != end; i++) {
        if (seen[i->toInt()]) *j++ = *i;
        else removedLits++;
        seen[i->toInt()] = 0;
    }
    numShrinkedClause += (removedLits > 0);
    numShrinkedClauseLits += removedLits;
    cl.shrink_(i-j);

    #ifdef VERBOSE_DEBUG
    std::cout << "c Removed further " << toRemove << " lits" << std::endl;
    #endif
}


/**
@brief Check if 'p' can be removed from a learnt clause

'abstract_levels' is used to abort early if the algorithm is
visiting literals at levels that cannot be removed later.
*/
bool Solver::litRedundant(Lit p, uint32_t abstract_levels)
{
    analyze_stack.clear();
    analyze_stack.push(p);
    int top = analyze_toclear.size();
    while (analyze_stack.size() > 0) {
        assert(!reason[analyze_stack.last().var()].isNULL());
        PropagatedFrom c(reason[analyze_stack.last().var()]);

        analyze_stack.pop();

        for (uint32_t i = 1, size = c.size(); i < size; i++) {
            Lit p  = c[i];
            if (!seen[p.var()] && level[p.var()] > 0) {
                if (!reason[p.var()].isNULL() && (abstractLevel(p.var()) & abstract_levels) != 0) {
                    seen[p.var()] = 1;
                    analyze_stack.push(p);
                    analyze_toclear.push(p);
                } else {
                    for (uint32_t j = top; j != analyze_toclear.size(); j++)
                        seen[analyze_toclear[j].var()] = 0;
                    analyze_toclear.shrink(analyze_toclear.size() - top);
                    return false;
                }
            }
        }
    }

    return true;
}


/*_________________________________________________________________________________________________
|
|  analyzeFinal : (p : Lit)  ->  [void]
|
|  Description:
|    Specialized analysis procedure to express the final conflict in terms of assumptions.
|    Calculates the (possibly empty) set of assumptions that led to the assignment of 'p', and
|    stores the result in 'out_conflict'.
|________________________________________________________________________________________________@*/
void Solver::analyzeFinal(Lit p, vec<Lit>& out_conflict)
{
    out_conflict.clear();
    out_conflict.push(p);

    if (decisionLevel() == 0)
        return;

    seen[p.var()] = 1;

    for (int32_t i = (int32_t)trail.size()-1; i >= (int32_t)trail_lim[0]; i--) {
        Var x = trail[i].var();
        if (seen[x]) {
            if (reason[x].isNULL()) {
                assert(level[x] > 0);
                out_conflict.push(~trail[i]);
            } else {
                PropagatedFrom c = reason[x];
                for (uint32_t j = 1, size = c.size(); j < size; j++)
                    if (level[c[j].var()] > 0)
                        seen[c[j].var()] = 1;
            }
            seen[x] = 0;
        }
    }

    seen[p.var()] = 0;
}


/**
@brief Enqueues&sets a new fact that has been found

Call this when a fact has been found. Sets the value, enqueues it for
propagation, sets its level, sets why it was propagated, saves the polarity,
and does some logging if logging is enabled. May also save the "old" polarity
(i.e. polarity that was in polarities[] at p.var()] of the variable if
USE_OLD_POLARITIES is set

@p p the fact to enqueue
@p from Why was it propagated (binary clause, tertiary clause, normal clause)
*/
void Solver::uncheckedEnqueue(const Lit p, const PropagatedFrom& from)
{

    #ifdef DEBUG_UNCHECKEDENQUEUE_LEVEL0
    #ifndef VERBOSE_DEBUG
    if (decisionLevel() == 0)
    #endif //VERBOSE_DEBUG
    std::cout << "uncheckedEnqueue var " << p.var()+1 << " to " << !p.sign() << " level: " << decisionLevel() << " sublevel: " << trail.size() << std::endl;
    #endif //DEBUG_UNCHECKEDENQUEUE_LEVEL0

    //assert(decisionLevel() == 0 || !subsumer->getVarElimed()[p.var()]);

    assert(assigns[p.var()].isUndef());
    const Var v = p.var();
    assigns [v] = boolToLBool(!p.sign());//lbool(!sign(p));  // <<== abstract but not uttermost effecient
    level   [v] = decisionLevel();
    reason  [v] = from;
    #ifdef USE_OLD_POLARITIES
    oldPolarity[p.var()] = polarity[p.var()];
    #endif //USE_OLD_POLARITIES
    polarity[p.var()] = p.sign();
    trail.push(p);

    #ifdef STATS_NEEDED
    if (dynamic_behaviour_analysis)
        logger.propagation(p, from);
    #endif
}

/*_________________________________________________________________________________________________
|
|  propagate : [void]  ->  [Clause*]
|
|  Description:
|    Propagates all enqueued facts. If a conflict arises, the conflicting clause is returned,
|    otherwise NULL.
|
|    Post-conditions:
|      * the propagation queue is empty, even if there was a conflict.
|________________________________________________________________________________________________@*/
/**
@brief Propagates a binary clause

Need to be somewhat tricky if the clause indicates that current assignement
is incorrect (i.e. both literals evaluate to FALSE). If conflict if found,
sets failBinLit
*/
inline void Solver::propBinaryClause(Watched* &i, Watched* &j, const Watched *end, const Lit& p, PropagatedFrom& confl)
{
    *j++ = *i;
    lbool val = value(i->getOtherLit());
    if (val.isUndef()) {
        uncheckedEnqueue(i->getOtherLit(), PropagatedFrom(p));
    } else if (val == l_False) {
        confl = PropagatedFrom(p);
        failBinLit = i->getOtherLit();
        qhead = trail.size();
        while (++i < end)
            *j++ = *i;
        i--;
    }
}

/**
@brief Propagates a tertiary (3-long) clause

Need to be somewhat tricky if the clause indicates that current assignement
is incorrect (i.e. all 3 literals evaluate to FALSE). If conflict is found,
sets failBinLit
*/
inline void Solver::propTriClause(Watched* &i, Watched* &j, const Watched *end, const Lit& p, PropagatedFrom& confl)
{
    *j++ = *i;
    lbool val = value(i->getOtherLit());
    lbool val2 = value(i->getOtherLit2());
    if (val.isUndef() && val2 == l_False) {
        uncheckedEnqueue(i->getOtherLit(), PropagatedFrom(p, i->getOtherLit2()));
    } else if (val == l_False && val2.isUndef()) {
        uncheckedEnqueue(i->getOtherLit2(), PropagatedFrom(p, i->getOtherLit()));
    } else if (val == l_False && val2 == l_False) {
        confl = PropagatedFrom(p, i->getOtherLit2());
        failBinLit = i->getOtherLit();
        qhead = trail.size();
        while (++i < end)
            *j++ = *i;
        i--;
    }
}

/**
@brief Propagates a tertiary (3-long) clause

We have blocked literals in this case in the watchlist. That must be checked
and updated.
*/
inline void Solver::propNormalClause(Watched* &i, Watched* &j, const Watched *end, const Lit& p, PropagatedFrom& confl, const bool update)
{
    if (value(i->getBlockedLit()).getBool()) {
        // Clause is sat
        *j++ = *i;
        return;
    }
    Clause& c = *clauseAllocator.getPointer(i->getOffset());

    // Make sure the false literal is data[1]:
    const Lit false_lit(~p);
    if (c[0] == false_lit) {
        c[0] = c[1];
        c[1] = false_lit;
    }

    assert(c[1] == false_lit);

    // If 0th watch is true, then clause is already satisfied.
    const Lit& first = c[0];
    if (value(first).getBool()) {
        j->setClause();
        j->setOffset(i->getOffset());
        j->setBlockedLit(first);
        j++;
    } else {
        // Look for new watch:
        for (Lit *k = &c[2], *end2 = c.getDataEnd(); k != end2; k++) {
            if (value(*k) != l_False) {
                c[1] = *k;
                *k = false_lit;
                watches[(~c[1]).toInt()].push(Watched(i->getOffset(), c[0]));
                return;
            }
        }

        // Did not find watch -- clause is unit under assignment:
        *j++ = *i;
        if (value(first) == l_False) {
            confl = PropagatedFrom(&c);
            qhead = trail.size();
            // Copy the remaining watches:
            while (++i < end)
                *j++ = *i;
            i--;
        } else {
            uncheckedEnqueue(first, &c);
            #ifdef DYNAMICNBLEVEL
            if (update && c.learnt() && c.getGlue() > 2) { // GA
                uint32_t nbLevels = calcNBLevels(c);
                if (nbLevels+1 < c.getGlue())
                    c.setGlue(nbLevels);
            }
            #endif
        }
    }
}

/**
@brief Propagates a tertiary (3-long) clause

Strangely enough, we need to have 4 literals in the wathclists:
for the first two varialbles, BOTH negations (v and ~v). This means quite some
pain, since we need to remove v when moving ~v and vica-versa. However, it means
better memory-accesses since the watchlist is already in the memory...

\todo maybe not worth it, and a variable-based watchlist should be used
*/
inline void Solver::propXorClause(Watched* &i, Watched* &j, const Watched *end, const Lit& p, PropagatedFrom& confl)
{
    XorClause& c = *(XorClause*)clauseAllocator.getPointer(i->getOffset());

    // Make sure the false literal is data[1]:
    if (c[0].var() == p.var()) {
        Lit tmp(c[0]);
        c[0] = c[1];
        c[1] = tmp;
    }
    assert(c[1].var() == p.var());

    bool final = c.xorEqualFalse();
    for (uint32_t k = 0, size = c.size(); k != size; k++ ) {
        const lbool& val = assigns[c[k].var()];
        if (val.isUndef() && k >= 2) {
            Lit tmp(c[1]);
            c[1] = c[k];
            c[k] = tmp;
            removeWXCl(watches[(~p).toInt()], i->getOffset());
            watches[Lit(c[1].var(), false).toInt()].push(i->getOffset());
            watches[Lit(c[1].var(), true).toInt()].push(i->getOffset());
            return;
        }

        c[k] = c[k].unsign() ^ val.getBool();
        final ^= val.getBool();
    }

    // Did not find watch -- clause is unit under assignment:
    *j++ = *i;

    if (assigns[c[0].var()].isUndef()) {
        c[0] = c[0].unsign()^final;
        uncheckedEnqueue(c[0], (Clause*)&c);
    } else if (!final) {
        confl = PropagatedFrom((Clause*)&c);
        qhead = trail.size();
        // Copy the remaining watches:
        while (++i < end)
            *j++ = *i;
        i--;
    } else {
        Lit tmp(c[0]);
        c[0] = c[1];
        c[1] = tmp;
    }
}

/**
@brief Does the propagation

Basically, it goes through the watchlists recursively, and calls the appropirate
propagaton function
*/
PropagatedFrom Solver::propagate(const bool update)
{
    PropagatedFrom confl;
    uint32_t num_props = 0;

    #ifdef VERBOSE_DEBUG
    cout << "Propagation started" << endl;
    #endif

    while (qhead < trail.size()) {
        Lit            p   = trail[qhead++];     // 'p' is enqueued fact to propagate.
        vec<Watched>&  ws  = watches[p.toInt()];
        Watched        *i, *i2, *j;
        num_props += ws.size()/2 + 1;

        #ifdef VERBOSE_DEBUG
        cout << "Propagating lit " << (p.sign() ? '-' : ' ') << p.var()+1 << endl;
        #endif

        i = i2 = j = ws.getData();
        i2++;
        for (Watched *end = ws.getDataEnd(); i != end; i++, i2++) {
            if (i->isBinary()) {
                propBinaryClause(i, j, end, p, confl);
                goto FoundWatch;
            } //end BINARY

            if (i->isTriClause()) {
                propTriClause(i, j, end, p, confl);
                goto FoundWatch;
            } //end TRICLAUSE

            if (i->isClause()) {
                propNormalClause(i, j, end, p, confl, update);
                goto FoundWatch;
            } //end CLAUSE

            if (i->isXorClause()) {
                propXorClause(i, j, end, p, confl);
                goto FoundWatch;
            } //end XORCLAUSE
FoundWatch:
            ;
        }
        ws.shrink_(i - j);
    }
    propagations += num_props;
    simpDB_props -= num_props;

    #ifdef VERBOSE_DEBUG
    cout << "Propagation ended." << endl;
    #endif

    return confl;
}

/**
@brief Only propagates binary clauses

This is used in special algorithms outside the main Solver class
*/
PropagatedFrom Solver::propagateBin()
{
    while (qhead < trail.size()) {
        Lit p   = trail[qhead++];
        vec<Watched> & wbin = watches[p.toInt()];
        propagations += wbin.size()/2;
        for(Watched *k = wbin.getData(), *end = wbin.getDataEnd(); k != end; k++) {
            if (!k->isBinary()) continue;

            lbool val = value(k->getOtherLit());
            if (val.isUndef()) {
                //uncheckedEnqueue(k->impliedLit, k->clause);
                uncheckedEnqueueLight(k->getOtherLit());
            } else if (val == l_False) {
                return PropagatedFrom(p);
            }
        }
    }

    return PropagatedFrom();
}

/**
@brief Calculates the glue of a clause

Used to calculate the Glue of a new clause, or to update the glue of an
existing clause. Only used if the glue-based activity heuristic is enabled,
i.e. if we are in GLUCOSE mode (not MiniSat mode)
*/
template<class T>
inline const uint32_t Solver::calcNBLevels(const T& ps)
{
    MYFLAG++;
    uint32_t nbLevels = 0;
    for(const Lit *l = ps.getData(), *end = ps.getDataEnd(); l != end; l++) {
        int32_t lev = level[l->var()];
        if (permDiff[lev] != MYFLAG) {
            permDiff[lev] = MYFLAG;
            nbLevels++;
        }
    }
    return nbLevels;
}

/*_________________________________________________________________________________________________
|
|  reduceDB : ()  ->  [void]
|
|  Description:
|    Remove half of the learnt clauses, minus the clauses locked by the current assignment. Locked
|    clauses are clauses that are reason to some assignment. Binary clauses are never removed.
|________________________________________________________________________________________________@*/
bool  reduceDB_ltMiniSat::operator () (const Clause* x, const Clause* y) {
    const uint32_t xsize = x->size();
    const uint32_t ysize = y->size();

    // First criteria
    if (xsize > 2 && ysize == 2) return 1;
    if (ysize > 2 && xsize == 2) return 0;

    if (x->getMiniSatAct() == y->getMiniSatAct())
        return xsize > ysize;
    else return x->getMiniSatAct() < y->getMiniSatAct();
}

bool  reduceDB_ltGlucose::operator () (const Clause* x, const Clause* y) {
    const uint32_t xsize = x->size();
    const uint32_t ysize = y->size();

    // First criteria
    if (xsize > 2 && ysize == 2) return 1;
    if (ysize > 2 && xsize == 2) return 0;

    if (x->getGlue() > y->getGlue()) return 1;
    if (x->getGlue() < y->getGlue()) return 0;
    return xsize > ysize;
}

/**
@brief Removes learnt clauses that have been found not to be too good

Either based on glue or MiniSat-style learnt clause activities, the clauses are
sorted and then removed
*/
void Solver::reduceDB()
{
    uint32_t     i, j;

    nbReduceDB++;
    if (lastSelectedRestartType == dynamic_restart)
        std::sort(learnts.getData(), learnts.getData()+learnts.size(), reduceDB_ltGlucose());
    else
        std::sort(learnts.getData(), learnts.getData()+learnts.size(), reduceDB_ltMiniSat());

    #ifdef VERBOSE_DEBUG
    std::cout << "Cleaning clauses" << std::endl;
    for (uint32_t i = 0; i != learnts.size(); i++) {
        std::cout << "activity:" << learnts[i]->activity()
        << " \toldActivity:" << learnts[i]->oldActivity()
        << " \tsize:" << learnts[i]->size() << std::endl;
    }
    #endif


    const uint32_t removeNum = (double)learnts.size() / (double)RATIOREMOVECLAUSES;
    for (i = j = 0; i != removeNum; i++){
        if (i+1 < removeNum)
            __builtin_prefetch(learnts[i+1], 0, 0);
        if (learnts[i]->size() > 2 && !locked(*learnts[i]) && (lastSelectedRestartType == static_restart || learnts[i]->getGlue() > 2)) {
            removeClause(*learnts[i]);
        } else
            learnts[j++] = learnts[i];
    }
    for (; i < learnts.size(); i++) {
        learnts[j++] = learnts[i];
    }
    learnts.shrink(i - j);

    clauseAllocator.consolidate(this);
}

inline int64_t abs64(int64_t a)
{
    if (a < 0) return -a;
    return a;
}

/**
@brief Simplify the clause database according to the current top-level assigment.

We remove satisfied clauses, clean clauses from assigned literals, find
binary xor-clauses and replace variables with one another. Heuristics are
used to check if we need to find binary xor clauses or not.
*/
const bool Solver::simplify()
{
    testAllClauseAttach();
    assert(decisionLevel() == 0);

    if (!ok || !propagate().isNULL()) {
        ok = false;
        return false;
    }

    if (simpDB_props > 0) {
        return true;
    }

    double slowdown = (100000.0/(double)binaryClauses.size());
    slowdown = std::min(3.5, slowdown);
    slowdown = std::max(0.2, slowdown);

    double speedup = 50000000.0/(double)(propagations-lastSearchForBinaryXor);
    speedup = std::min(3.5, speedup);
    speedup = std::max(0.2, speedup);

    /*std::cout << "new:" << nbBin - lastNbBin + becameBinary << std::endl;
    std::cout << "left:" << ((double)(nbBin - lastNbBin + becameBinary)/BINARY_TO_XOR_APPROX) * slowdown  << std::endl;
    std::cout << "right:" << (double)order_heap.size() * PERCENTAGEPERFORMREPLACE * speedup << std::endl;*/

    if (findBinaryXors && regFindBinaryXors &&
        (((double)abs64((int64_t)nbBin - (int64_t)lastNbBin + (int64_t)becameBinary)/BINARY_TO_XOR_APPROX) * slowdown) >
        ((double)order_heap.size() * PERCENTAGEPERFORMREPLACE * speedup)) {
        lastSearchForBinaryXor = propagations;

        clauseCleaner->cleanClauses(clauses, ClauseCleaner::clauses);
        clauseCleaner->cleanClauses(learnts, ClauseCleaner::learnts);
        clauseCleaner->removeSatisfied(binaryClauses, ClauseCleaner::binaryClauses);
        if (!ok) return false;

        XorFinder xorFinder(*this, binaryClauses, ClauseCleaner::binaryClauses);
        if (!xorFinder.fullFindXors(2, 2)) return false;

        lastNbBin = nbBin;
        becameBinary = 0;
    }

    // Remove satisfied clauses:
    clauseCleaner->removeAndCleanAll();
    if (!ok) return false;

    if (doReplace && !varReplacer->performReplace())
        return false;

    // Remove fixed variables from the variable heap:
    order_heap.filter(VarFilter(*this));

    #ifdef USE_GAUSS
    for (vector<Gaussian*>::iterator gauss = gauss_matrixes.begin(), end = gauss_matrixes.end(); gauss != end; gauss++) {
        if (!(*gauss)->full_init()) return false;
    }
    #endif //USE_GAUSS

    simpDB_assigns = nAssigns();
    simpDB_props   = clauses_literals + learnts_literals;   // (shouldn't depend on stats really, but it will do for now)

    testAllClauseAttach();
    return true;
}

/**
@brief Used to dump claues and exit

Only called when a clean interrupt has been asked for, and we are at a point
where we can cleanly dump the datastructures (i.e. they are not in the middle
of being updated, for example)
*/
void Solver::interruptCleanly()
{
    if (needToDumpLearnts) {
        dumpSortedLearnts(learntsFilename, maxDumpLearntsSize);
        std::cout << "c Sorted learnt clauses dumped to file '" << learntsFilename << "'" << std::endl;
    }
    if (needToDumpOrig) {
        dumpOrigClauses(origFilename);
        std::cout << "c Simplified original clauses dumped to file '" << origFilename << "'" << std::endl;
    }
    exit(0);
}


/**
@brief Search for a model

Limits: must be below the specified number of conflicts and must keep the
number of learnt clauses below the provided limit

Use negative value for 'nof_conflicts' or 'nof_learnts' to indicate infinity.

Output: 'l_True' if a partial assigment that is consistent with respect to the
clauseset is found. If all variables are decision variables, this means
that the clause set is satisfiable. 'l_False' if the clause set is
unsatisfiable. 'l_Undef' if the bound on number of conflicts is reached.
*/
lbool Solver::search(int nof_conflicts, int nof_conflicts_fullrestart, const bool update)
{
    assert(ok);
    int         conflictC = 0;
    vec<Lit>    learnt_clause;
    llbool      ret;

    starts++;
    if (restartType == static_restart)
        staticStarts++;
    else
        dynStarts++;

    #ifdef USE_GAUSS
    for (vector<Gaussian*>::iterator gauss = gauss_matrixes.begin(), end = gauss_matrixes.end(); gauss != end; gauss++) {
        if (!(*gauss)->full_init())
            return l_False;
    }
    #endif //USE_GAUSS

    testAllClauseAttach();
    findAllAttach();
    for (;;) {
        if (needToInterrupt) interruptCleanly();
        PropagatedFrom confl = propagate(update);

        if (!confl.isNULL()) {
            ret = handle_conflict(learnt_clause, confl, conflictC, update);
            if (ret != l_Nothing) return ret;
        } else {
            #ifdef USE_GAUSS
            bool at_least_one_continue = false;
            for (vector<Gaussian*>::iterator gauss = gauss_matrixes.begin(), end= gauss_matrixes.end(); gauss != end; gauss++) {
                ret = (*gauss)->find_truths(learnt_clause, conflictC);
                if (ret == l_Continue) at_least_one_continue = true;
                else if (ret != l_Nothing) return ret;
            }
            if (at_least_one_continue) continue;
            #endif //USE_GAUSS
            ret = new_decision(nof_conflicts, nof_conflicts_fullrestart, conflictC);
            if (ret != l_Nothing) return ret;
        }
    }
}

/**
@brief Picks a new decision variable to branch on

@returns l_Undef if it should restart instead. l_False if it reached UNSAT
         (through simplification)
*/
llbool Solver::new_decision(const int& nof_conflicts, const int& nof_conflicts_fullrestart, int& conflictC)
{

    // Reached bound on number of conflicts?
    switch (restartType) {
    case dynamic_restart:
        if (glueHistory.isvalid() &&
            ((glueHistory.getavg()) > (totalSumOfGlue / (double)(conflicts - compTotSumGlue)))) {

            #ifdef DEBUG_DYNAMIC_RESTART
            if (glueHistory.isvalid()) {
                std::cout << "glueHistory.getavg():" << glueHistory.getavg() <<std::endl;
                std::cout << "totalSumOfGlue:" << totalSumOfGlue << std::endl;
                std::cout << "conflicts:" << conflicts<< std::endl;
                std::cout << "compTotSumGlue:" << compTotSumGlue << std::endl;
                std::cout << "conflicts-compTotSumGlue:" << conflicts-compTotSumGlue<< std::endl;
            }
            #endif

            glueHistory.fastclear();
            #ifdef STATS_NEEDED
            if (dynamic_behaviour_analysis)
                progress_estimate = progressEstimate();
            #endif
            cancelUntil(0);
            return l_Undef;
        }
        break;
    case static_restart:
        if (nof_conflicts >= 0 && conflictC >= nof_conflicts) {
            #ifdef STATS_NEEDED
            if (dynamic_behaviour_analysis)
                progress_estimate = progressEstimate();
            #endif
            cancelUntil(0);
            return l_Undef;
        }
        break;
    case auto_restart:
        assert(false);
        break;
    }
    if (nof_conflicts_fullrestart >= 0 && (int)conflicts >= nof_conflicts_fullrestart)  {
        #ifdef STATS_NEEDED
        if (dynamic_behaviour_analysis)
            progress_estimate = progressEstimate();
        #endif
        cancelUntil(0);
        return l_Undef;
    }

    // Simplify the set of problem clauses:
    if (decisionLevel() == 0 && !simplify()) {
        return l_False;
    }

    // Reduce the set of learnt clauses:
    if (conflicts >= numCleanedLearnts * nbClBeforeRed + nbCompensateSubsumer) {
        numCleanedLearnts ++;
        reduceDB();
        nbClBeforeRed += 500;
    }

    Lit next = lit_Undef;
    while (decisionLevel() < assumptions.size()) {
        // Perform user provided assumption:
        Lit p = assumptions[decisionLevel()];
        if (value(p) == l_True) {
            // Dummy decision level:
            newDecisionLevel();
        } else if (value(p) == l_False) {
            analyzeFinal(~p, conflict);
            return l_False;
        } else {
            next = p;
            break;
        }
    }

    if (next == lit_Undef) {
        // New variable decision:
        decisions++;
        next = pickBranchLit();

        if (next == lit_Undef)
            return l_True;
    }

    // Increase decision level and enqueue 'next'
    assert(value(next) == l_Undef);
    newDecisionLevel();
    uncheckedEnqueue(next);

    return l_Nothing;
}

/**
@brief Handles a conflict that we reached through propagation

Handles on-the-fly subsumption: the OTF subsumption check is done in
conflict analysis, but this is the code that actually replaces the original
clause with that of the shorter one
@returns l_False if UNSAT
*/
llbool Solver::handle_conflict(vec<Lit>& learnt_clause, PropagatedFrom confl, int& conflictC, const bool update)
{
    #ifdef VERBOSE_DEBUG
    cout << "Handling conflict: ";
    for (uint32_t i = 0; i < learnt_clause.size(); i++)
        cout << learnt_clause[i].var()+1 << ",";
    cout << endl;
    #endif

    int backtrack_level;
    uint32_t glue;

    conflicts++;
    conflictC++;
    if (decisionLevel() == 0)
        return l_False;
    learnt_clause.clear();
    Clause* c = analyze(confl, learnt_clause, backtrack_level, glue, update);
    if (update) {
        #ifdef RANDOM_LOOKAROUND_SEARCHSPACE
        avgBranchDepth.push(decisionLevel());
        #endif //RANDOM_LOOKAROUND_SEARCHSPACE
        if (restartType == dynamic_restart)
            glueHistory.push(glue);

        totalSumOfGlue += glue;
    } else {
        compTotSumGlue++;
    }

    #ifdef STATS_NEEDED
    if (dynamic_behaviour_analysis)
        logger.conflict(Logger::simple_confl_type, backtrack_level, confl->getGroup(), learnt_clause);
    #endif
    cancelUntil(backtrack_level);

    #ifdef VERBOSE_DEBUG
    cout << "Learning:";
    for (uint32_t i = 0; i < learnt_clause.size(); i++) printLit(learnt_clause[i]), cout << " ";
    cout << endl;
    cout << "reverting var " << learnt_clause[0].var()+1 << " to " << !learnt_clause[0].sign() << endl;
    #endif

    assert(value(learnt_clause[0]) == l_Undef);
    //Unitary learnt
    if (learnt_clause.size() == 1) {
        uncheckedEnqueue(learnt_clause[0]);
        assert(backtrack_level == 0 && "Unit clause learnt, so must cancel until level 0, right?");

        #ifdef VERBOSE_DEBUG
        cout << "Unit clause learnt." << endl;
        #endif
    //Normal learnt
    } else {
        if (c) { //On-the-fly subsumption
            uint32_t origSize = c->size();
            detachClause(*c);
            for (uint32_t i = 0; i != learnt_clause.size(); i++)
                (*c)[i] = learnt_clause[i];
            c->shrink(origSize - learnt_clause.size());
            if (c->learnt()) {
                if (c->getGlue() > glue)
                    c->setGlue(glue); // LS
                if (c->size() == 2)
                    nbBin++;
                learnts_literals -= origSize - learnt_clause.size();
            } else {
                clauses_literals -= origSize - learnt_clause.size();
            }
        } else {  //no on-the-fly subsumption
            c = clauseAllocator.Clause_new(learnt_clause, learnt_clause_group++, true);
            #ifdef STATS_NEEDED
            if (dynamic_behaviour_analysis)
                logger.set_group_name(c->getGroup(), "learnt clause");
            #endif
            if (c->size() > 2) {
                learnts.push(c);
                c->setGlue(glue); // LS
            } else {
                binaryClauses.push(c);
                nbBin++;
            }
        }
        attachClause(*c);
        uncheckedEnqueue(learnt_clause[0], c);
    }

    varDecayActivity();
    if (update && restartType == static_restart) claDecayActivity();

    return l_Nothing;
}

/**
@brief After a full restart, determines which restar type to use

Uses class RestartTypeChooser to do the heavy-lifting
*/
const bool Solver::chooseRestartType(const uint32_t& lastFullRestart)
{
    uint32_t relativeStart = starts - lastFullRestart;

    if (relativeStart > RESTART_TYPE_DECIDER_FROM  && relativeStart < RESTART_TYPE_DECIDER_UNTIL) {
        if (fixRestartType == auto_restart)
            restartTypeChooser->addInfo();

        if (relativeStart == (RESTART_TYPE_DECIDER_UNTIL-1)) {
            RestartType tmp;
            if (fixRestartType == auto_restart)
                tmp = restartTypeChooser->choose();
            else
                tmp = fixRestartType;

            if (tmp == dynamic_restart) {
                glueHistory.fastclear();
                glueHistory.initSize(100);
                if (verbosity >= 3)
                    std::cout << "c Decided on dynamic restart strategy"
                    << std::endl;
            } else  {
                if (verbosity >= 3)
                    std::cout << "c Decided on static restart strategy"
                    << std::endl;

                #ifdef USE_GAUSS
                if (!matrixFinder->findMatrixes()) return false;
                #endif //USE_GAUSS
            }
            lastSelectedRestartType = tmp;
            restartType = tmp;
            restartTypeChooser->reset();
        }
    }

    return true;
}

inline void Solver::setDefaultRestartType()
{
    if (fixRestartType != auto_restart) restartType = fixRestartType;
    else restartType = static_restart;

    if (restartType == dynamic_restart) {
        glueHistory.fastclear();
        glueHistory.initSize(100);
    }

    lastSelectedRestartType = restartType;
}

/**
@brief The function that brings together almost all CNF-simplifications

It burst-searches for given number of conflicts, then it tries all sorts of
things like variable elimination, subsumption, failed literal probing, etc.
to try to simplifcy the problem at hand.
*/
const lbool Solver::simplifyProblem(const uint32_t numConfls)
{
    testAllClauseAttach();
    #ifdef USE_GAUSS
    bool gauss_was_cleared = (gauss_matrixes.size() == 0);
    clearGaussMatrixes();
    #endif //USE_GAUSS

    StateSaver savedState(*this);;

    #ifdef BURST_SEARCH
    if (verbosity >= 3)
        std::cout << "c " << std::setw(24) << " "
        << "Simplifying problem for " << std::setw(8) << numConfls << " confls"
        << std::endl;
    random_var_freq = 1;
    simplifying = true;
    uint64_t origConflicts = conflicts;
    #endif //BURST_SEARCH

    lbool status = l_Undef;

    #ifdef BURST_SEARCH
    restartType = static_restart;

    printRestartStat("S");
    while(status == l_Undef && conflicts-origConflicts < numConfls) {
        status = search(100, -1, false);
        starts--;
    }
    printRestartStat("S");
    if (status != l_Undef) goto end;
    #endif //BURST_SEARCH

    if (doXorSubsumption && !xorSubsumer->simplifyBySubsumption()) goto end;

    if (failedVarSearch && !failedVarSearcher->search()) goto end;

    if (doReplace && (regRemUselessBins || regSubsWNonExistBins)) {
        OnlyNonLearntBins onlyNonLearntBins(*this);
        if (!onlyNonLearntBins.fill()) goto end;
        if (regSubsWNonExistBins
            && !subsumer->subsumeWithBinaries(&onlyNonLearntBins)) goto end;
        if (regRemUselessBins) {
            UselessBinRemover uselessBinRemover(*this, onlyNonLearntBins);
            if (!uselessBinRemover.removeUslessBinFull()) goto end;
        }
    }

    if (doSubsumption && !subsumer->simplifyBySubsumption(false)) goto end;

    if (doSubsumption && !subsumer->simplifyBySubsumption(true)) goto end;

    /*if (findNormalXors && xorclauses.size() > 200 && clauses.size() < MAX_CLAUSENUM_XORFIND/8) {
        XorFinder xorFinder(*this, clauses, ClauseCleaner::clauses);
        if (!xorFinder.doNoPart(3, 7)) {
            status = l_False;
            goto end;
        }
    } else*/ if (xorclauses.size() <= 200 && xorclauses.size() > 0 && nClauses() > 10000) {
        XorFinder x(*this, clauses, ClauseCleaner::clauses);
        x.addAllXorAsNorm();
    }

    if (doAsymmBranchReg && !failedVarSearcher->asymmBranch()) goto end;

    //addSymmBreakClauses();

    if (doSortWatched) sortWatched();

end:
    #ifdef BURST_SEARCH
    if (verbosity >= 3)
        std::cout << "c Simplifying finished" << std::endl;
    #endif //#ifdef BURST_SEARCH

    savedState.restore();
    simplifying = false;

    #ifdef USE_GAUSS
    if (status == l_Undef && !gauss_was_cleared && !matrixFinder->findMatrixes())
        status = l_False;
    #endif //USE_GAUSS

    testAllClauseAttach();

    if (!ok) return l_False;
    return status;
}

/**
@brief Should we perform a full restart?

If so, we also do the things to be done if the full restart is effected.
Currently, this means we try to find disconnected components and solve
them with sub-solvers using class PartHandler
*/
const bool Solver::checkFullRestart(int& nof_conflicts, int& nof_conflicts_fullrestart, uint32_t& lastFullRestart)
{
    if (nof_conflicts_fullrestart > 0 && (int)conflicts >= nof_conflicts_fullrestart) {
        #ifdef USE_GAUSS
        clearGaussMatrixes();
        #endif //USE_GAUSS
        nof_conflicts = restart_first + (double)restart_first*restart_inc;
        nof_conflicts_fullrestart = (double)nof_conflicts_fullrestart * FULLRESTART_MULTIPLIER_MULTIPLIER;
        restartType = static_restart;
        lastFullRestart = starts;

        if (verbosity >= 3)
            std::cout << "c Fully restarting" << std::endl;
        printRestartStat("F");

        /*if (findNormalXors && clauses.size() < MAX_CLAUSENUM_XORFIND) {
            XorFinder xorFinder(this, clauses, ClauseCleaner::clauses);
            if (!xorFinder.doNoPart(3, 10))
                return false;
        }*/

        if (doPartHandler && !partHandler->handle())
            return false;

        //calculateDefaultPolarities();

        fullStarts++;
    }

    return true;
}

/**
@brief Performs a set of pre-optimisations before the beggining of solving

This is somewhat different than the set of optimisations carried out during
solving in simplifyProblem(). For instance, binary xors are searched fully
here, while there, no search for them is carried out. Also, the ordering
is different.

\todo experiment to use simplifyProblem() instead of this, with the only
addition of binary clause search. Maybe it will do just as good (or better).
*/
void Solver::performStepsBeforeSolve()
{
    assert(qhead == trail.size());
    testAllClauseAttach();

    if (doReplace && !varReplacer->performReplace()) return;

    if (doAsymmBranch && !libraryUsage
        && !failedVarSearcher->asymmBranch()) return;

    if (doReplace) {
        OnlyNonLearntBins onlyNonLearntBins(*this);
        if (!onlyNonLearntBins.fill()) return;
        if (subsWNonExistBins && !libraryUsage
            && !subsumer->subsumeWithBinaries(&onlyNonLearntBins)) return;
    }

    if (doSubsumption
        && !libraryUsage
        && clauses.size() + binaryClauses.size() + learnts.size() < 4800000
        && !subsumer->simplifyBySubsumption())
        return;

    if (findBinaryXors && binaryClauses.size() < MAX_CLAUSENUM_XORFIND) {
        XorFinder xorFinder(*this, binaryClauses, ClauseCleaner::binaryClauses);
        if (!xorFinder.fullFindXors(2, 2)) return;
        if (doReplace && !varReplacer->performReplace(true)) return;
    }

    if (findNormalXors && clauses.size() < MAX_CLAUSENUM_XORFIND) {
        XorFinder xorFinder(*this, clauses, ClauseCleaner::clauses);
        if (!xorFinder.fullFindXors(3, 7)) return;
    }

    if (xorclauses.size() > 1) {
        if (doXorSubsumption && !xorSubsumer->simplifyBySubsumption())
            return;

        if (doReplace && !varReplacer->performReplace())
            return;
    }

    if (doSortWatched) sortWatched();
    testAllClauseAttach();
}

/**
@brief Initialises model, restarts, learnt cluause cleaning, burst-search, etc.
*/
void Solver::initialiseSolver()
{
    //Clear up previous stuff like model, final conflict, matrixes
    model.clear();
    conflict.clear();
    #ifdef USE_GAUSS
    clearGaussMatrixes();
    #endif //USE_GAUSS

    //Initialise restarts & dynamic restart datastructures
    setDefaultRestartType();
    totalSumOfGlue = 0;
    compTotSumGlue = conflicts;

    //Initialise avg. branch depth
    #ifdef RANDOM_LOOKAROUND_SEARCHSPACE
    avgBranchDepth.fastclear();
    avgBranchDepth.initSize(500);
    #endif //RANDOM_LOOKAROUND_SEARCHSPACE

    //Initialise number of restarts&full restarts
    starts = 0;
    fullStarts = 0;

    if (nClauses() * learntsize_factor < nbClBeforeRed) {
        if (nClauses() * learntsize_factor < nbClBeforeRed/2)
            nbClBeforeRed /= 4;
        else
            nbClBeforeRed = (nClauses() * learntsize_factor)/2;
    }
    testAllClauseAttach();
    findAllAttach();
}

/**
@brief The main solve loop that glues everything together

We clear everything needed, pre-simplify the problem, calculate default
polarities, and start the loop. Finally, we either report UNSAT or extend the
found solution with all the intermediary simplifications (e.g. variable
elimination, etc.) and output the solution.
*/
lbool Solver::solve(const vec<Lit>& assumps)
{
#ifdef VERBOSE_DEBUG
    std::cout << "Solver::solve() called" << std::endl;
#endif
    if (!ok) return l_False;
    assert(qhead == trail.size());

    if (libraryCNFFile)
        fprintf(libraryCNFFile, "c Solver::solve() called\n");

    assumps.copyTo(assumptions);
    initialiseSolver();
    int       nof_conflicts = restart_first; //Geometric restart policy, start with this many
    int       nof_conflicts_fullrestart = restart_first * FULLRESTART_MULTIPLIER + conflicts; //at this point, do a full restart
    uint32_t  lastFullRestart = starts; //last time a full restart was made was at this number of restarts
    lbool     status = l_Undef; //Current status
    uint64_t  nextSimplify = restart_first * SIMPLIFY_MULTIPLIER + conflicts; //Do simplifyProblem() at this number of conflicts

    if (conflicts == 0) {
        performStepsBeforeSolve();
        if (!ok) return l_False;
    }
    calculateDefaultPolarities();

    printStatHeader();
    printRestartStat("B");
    uint64_t lastConflPrint = conflicts;
    // Search:
    while (status == l_Undef && starts < maxRestarts) {
        #ifdef DEBUG_VARELIM
        assert(subsumer->checkElimedUnassigned());
        assert(xorSubsumer->checkElimedUnassigned());
        #endif //DEBUG_VARELIM

        if ((conflicts - lastConflPrint) > std::max(conflicts/100*6, (uint64_t)4000)) {
            printRestartStat("N");
            lastConflPrint = conflicts;
        }

        if (schedSimplification && conflicts >= nextSimplify) {
            status = simplifyProblem(500);
            printRestartStat();
            lastConflPrint = conflicts;
            nextSimplify = conflicts * 1.5;
            if (status != l_Undef) break;
        }

        #ifdef STATS_NEEDED
        if (dynamic_behaviour_analysis) {
            logger.end(Logger::restarting);
            logger.begin();
        }
        #endif

        status = search(nof_conflicts, nof_conflicts_fullrestart);
        nof_conflicts = (double)nof_conflicts * restart_inc;
        if (status != l_Undef) break;
        if (!checkFullRestart(nof_conflicts, nof_conflicts_fullrestart, lastFullRestart))
            return l_False;
        if (!chooseRestartType(lastFullRestart))
            return l_False;
        #ifdef RANDOM_LOOKAROUND_SEARCHSPACE
        //if (avgBranchDepth.isvalid())
        //    std::cout << "avg branch depth:" << avgBranchDepth.getavg() << std::endl;
        #endif //RANDOM_LOOKAROUND_SEARCHSPACE
    }
    printEndSearchStat();

    #ifdef USE_GAUSS
    clearGaussMatrixes();
    #endif //USE_GAUSS

    #ifdef VERBOSE_DEBUG
    if (status == l_True)
        std::cout << "Solution  is SAT" << std::endl;
    else if (status == l_False)
        std::cout << "Solution is UNSAT" << std::endl;
    else
        std::cout << "Solutions is UNKNOWN" << std::endl;
    #endif //VERBOSE_DEBUG

    if (status == l_True) handleSATSolution();
    else if (status == l_False) handleUNSATSolution();

    #ifdef STATS_NEEDED
    if (dynamic_behaviour_analysis) {
        if (status == l_True)
            logger.end(Logger::model_found);
        else if (status == l_False)
            logger.end(Logger::unsat_model_found);
        else if (status == l_Undef)
            logger.end(Logger::restarting);
    }
    #endif

    cancelUntil(0);
    if (doPartHandler && status != l_False) partHandler->readdRemovedClauses();
    restartTypeChooser->reset();

    #ifdef VERBOSE_DEBUG
    std::cout << "Solver::solve() finished" << std::endl;
    #endif
    return status;
}

/**
@brief Extends a SAT solution to the full solution

variable elimination, variable replacement, sub-part solving, etc. all need to
be handled correctly to arrive at a solution that is a solution to ALL of the
original problem, not just of what remained of it at the end inside this class
(i.e. we need to combine things from the helper classes)
*/
void Solver::handleSATSolution()
{

    if (greedyUnbound) {
        double time = cpuTime();
        FindUndef finder(*this);
        const uint32_t unbounded = finder.unRoll();
        if (verbosity >= 1)
            printf("c Greedy unbounding     :%5.2lf s, unbounded: %7d vars\n", cpuTime()-time, unbounded);
    }

    partHandler->addSavedState();
    varReplacer->extendModelPossible();
    checkSolution();

    if (subsumer->getNumElimed() || xorSubsumer->getNumElimed()) {
        if (verbosity >= 1) {
            std::cout << "c Solution needs extension. Extending." << std::endl;
        }
        Solver s;
        s.doSubsumption = false;
        s.doReplace = false;
        s.findBinaryXors = false;
        s.findNormalXors = false;
        s.failedVarSearch = false;
        s.conglomerateXors = false;
        s.subsWNonExistBins = false;
        s.regSubsWNonExistBins = false;
        s.remUselessBins = false;
        s.regRemUselessBins = false;
        s.doAsymmBranch = false;
        s.doAsymmBranchReg = false;
        s.greedyUnbound = greedyUnbound;

        vec<Lit> tmp;
        for (Var var = 0; var < nVars(); var++) {
            s.newVar(decision_var[var] || subsumer->getVarElimed()[var] || varReplacer->varHasBeenReplaced(var) || xorSubsumer->getVarElimed()[var]);

            //assert(!(xorSubsumer->getVarElimed()[var] && (decision_var[var] || subsumer->getVarElimed()[var] || varReplacer->varHasBeenReplaced(var))));

            if (value(var) != l_Undef) {
                tmp.clear();
                tmp.push(Lit(var, value(var) == l_False));
                s.addClause(tmp);
            }
        }
        varReplacer->extendModelImpossible(s);
        subsumer->extendModel(s);
        xorSubsumer->extendModel(s);

        lbool status = s.solve();
        release_assert(status == l_True && "c ERROR! Extension of model failed!");
#ifdef VERBOSE_DEBUG
        std::cout << "Solution extending finished." << std::endl;
#endif
        for (Var var = 0; var < nVars(); var++) {
            if (assigns[var] == l_Undef && s.model[var] != l_Undef) uncheckedEnqueue(Lit(var, s.model[var] == l_False));
        }
        ok = (propagate().isNULL());
        release_assert(ok && "c ERROR! Extension of model failed!");
    }
    checkSolution();
    //Copy model:
    model.growTo(nVars());
    for (Var var = 0; var != nVars(); var++) model[var] = value(var);
}

/**
@brief When problem is decided to be UNSAT, this is called

There is basically nothing to be handled for the moment, but this could be
made extensible
*/
void Solver::handleUNSATSolution()
{
    if (conflict.size() == 0)
        ok = false;
}
