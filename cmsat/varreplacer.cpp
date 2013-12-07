/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.0 of the License, or (at your option) any later version.
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

#include "varreplacer.h"
#include "varupdatehelper.h"
#include <iostream>
#include <iomanip>
#include <set>
using std::cout;
using std::endl;

#include "solver.h"
#include "clausecleaner.h"
#include "time_mem.h"
#include "solutionextender.h"
#include "clauseallocator.h"

#ifdef VERBOSE_DEBUG
#define REPLACE_STATISTICS
#define VERBOSE_DEBUG_BIN_REPLACER
#endif

using namespace CMSat;

//#define VERBOSE_DEBUG
//#define REPLACE_STATISTICS
//#define DEBUG_BIN_REPLACER
//#define VERBOSE_DEBUG_BIN_REPLACER

VarReplacer::VarReplacer(Solver* _solver) :
    solver(_solver)
    , replacedVars(0)
    , lastReplacedVars(0)
{
}

VarReplacer::~VarReplacer()
{
}

void VarReplacer::printReplaceStats() const
{
    uint32_t i = 0;
    for (vector<Lit>::const_iterator
        it = table.begin(); it != table.end()
        ; it++, i++
    ) {
        if (it->var() == i) continue;
        cout << "Replacing var " << i+1 << " with Lit " << *it << endl;
    }
}

void VarReplacer::update_vardata_and_decisionvar(
    const Var orig
    , const Var replaced_with
) {
     //Was queued for replacement, but it's the top of the tree, so
    //it's normal again
    if (orig == replaced_with
        && solver->varData[replaced_with].removed == Removed::queued_replacer
    ) {
        solver->varData[replaced_with].removed = Removed::none;
    }

    //Not replaced_with, or not replaceable, so skip
    if (orig == replaced_with
        || solver->varData[replaced_with].removed == Removed::decomposed
        || solver->varData[replaced_with].removed == Removed::elimed
    ) {
        return;
    }

    //Has already been handled previously, just skip
    if (solver->varData[orig].removed == Removed::replaced) {
        return;
    }

    //Okay, so unset decision, and set the other one decision
    assert(orig != replaced_with);
    solver->varData[orig].removed = Removed::replaced;
    assert(
        (solver->varData[replaced_with].removed == Removed::none
            || solver->varData[replaced_with].removed == Removed::queued_replacer)
        && "It MUST have been queued for varreplacement so top couldn't have been removed/decomposed/etc"
    );
    solver->unsetDecisionVar(orig);
    solver->setDecisionVar(replaced_with);

    //Update activities. Top receives activities of the ones below
    //The activities of the others don't need to be updated -- they are not
    //set to be decision vars anyway
    solver->activities[replaced_with] += solver->activities[orig];
    solver->order_heap.update(orig);
}

bool VarReplacer::enqueueDelayedEnqueue()
{
    for(vector<Lit>::const_iterator
        it = delayedEnqueue.begin(), end = delayedEnqueue.end()
        ; it != end
        ; it++
    ) {
        if (solver->value(*it) == l_Undef) {
            solver->enqueue(*it);
            #ifdef STATS_NEEDED
            solver->propStats.propsUnit++;
            #endif
        } else if (solver->value(*it) == l_False) {
            solver->ok = false;
            break;
        }
    }
    delayedEnqueue.clear();

    if (!solver->ok)
        return false;

    solver->ok = solver->propagate().isNULL();
    return solver->ok;
}

bool VarReplacer::performReplace()
{
    assert(solver->ok);
    checkUnsetSanity();

    //Set up stats
    runStats.clear();
    runStats.numCalls = 1;
    const double myTime = cpuTime();
    const size_t origTrailSize = solver->trail.size();

    #ifdef REPLACE_STATISTICS
    uint32_t numRedir = 0;
    for (uint32_t i = 0; i < table.size(); i++) {
        if (getVarReplacedWith(i) != i)
            numRedir++;
    }
    cout << "c Number of trees:" << reverseTable.size() << endl;
    cout << "c Number of redirected nodes:" << numRedir << endl;
    #endif //REPLACE_STATISTICS

    solver->clauseCleaner->removeAndCleanAll();
    solver->testAllClauseAttach();

    //Printing stats
    if (solver->conf.verbosity >= 5)
        printReplaceStats();

    Var var = 0;
    for (vector<Lit>::const_iterator
        it = table.begin(); it != table.end()
        ; it++, var++
    ) {
       update_vardata_and_decisionvar(var, it->var());
    }

    runStats.actuallyReplacedVars = replacedVars -lastReplacedVars;
    lastReplacedVars = replacedVars;

    solver->testAllClauseAttach();
    assert(solver->qhead == solver->trail.size());

    #ifdef DEBUG_IMPLICIT_STATS
    solver->checkImplicitStats();
    #endif

    //Replace implicits
    if (!replaceImplicit()) {
        goto end;
    }

    //While replacing the implicit clauses
    //we cannot enqueue literals, so we do it now
    if (!enqueueDelayedEnqueue())
        goto end;

    //Replace longs
    if (!replace_set(solver->longIrredCls)) {
        goto end;
    }
    if (!replace_set(solver->longRedCls)) {
        goto end;
    }

    //Update assumptions
    for(Lit& lit: solver->assumptions) {
        solver->assumptionsSet[lit.var()] = false;
        lit = getLitReplacedWith(lit);
        solver->assumptionsSet[lit.var()] = true;
    }

    solver->testAllClauseAttach();
    solver->checkNoWrongAttach();
    solver->checkStats();

end:
    assert(solver->qhead == solver->trail.size() || !solver->ok);
    if (solver->okay()) {
        checkUnsetSanity();
    }

    //Update stamp dominators
    solver->stamp.updateDominators(this);

    //Update stats
    runStats.zeroDepthAssigns += solver->trail.size() - origTrailSize;
    runStats.cpu_time = cpuTime() - myTime;
    globalStats += runStats;
    if (solver->conf.verbosity  >= 1) {
        if (solver->conf.verbosity  >= 3)
            runStats.print(solver->nVars());
        else
            runStats.printShort();
    }

    return solver->ok;
}

void VarReplacer::newBinClause(
    Lit origLit1
    , Lit origLit2
    , Lit origLit3
    , Lit lit1
    , Lit lit2
    , bool red
) {
    //Only attach once
    if (origLit1 < origLit2
        && origLit2 < origLit3
    ){
        delayedAttach.push_back(BinaryClause(lit1, lit2, red));
        (*solver->drup) << lit1 << lit2 << fin;
    }
}

void VarReplacer::updateTri(
    watch_subarray::iterator& i
    , watch_subarray::iterator& j
    , const Lit origLit1
    , const Lit origLit2
    , Lit lit1
    , Lit lit2
) {
    Lit lit3 = i->lit3();
    Lit origLit3 = lit3;
    assert(origLit1.var() != origLit3.var());
    assert(origLit2.var() != origLit3.var());
    assert(origLit2 < origLit3);
    assert(solver->value(origLit3) == l_Undef);

    //Update lit3
    if (getVarReplacedWith(lit3) != lit3.var()) {
        lit3 = getLitReplacedWith(lit3);
        i->setLit3(lit3);
        runStats.replacedLits++;
    }

    bool remove = false;

    //Tautology, remove
    if (lit1 == ~lit2
        || lit1 == ~lit3
        || lit2 == ~lit3
    ) {
        remove = true;
    }

    //All 3 lits are the same
    if (!remove
        && lit1 == lit2
        && lit2 == lit3
    ) {
        delayedEnqueue.push_back(lit1);
        (*solver->drup) << lit1 << fin;
        remove = true;
    }

    //1st and 2nd lits are the same
    if (!remove
        && lit1 == lit2
    ) {
        newBinClause(origLit1, origLit2, origLit3, lit1, lit3, i->red());
        remove = true;
    }

    //1st and 3rd lits  OR 2nd and 3rd lits are the same
    if (!remove
        && (lit1 == lit3 || (lit2 == lit3))
    ) {
        newBinClause(origLit1, origLit2, origLit3, lit1, lit2, i->red());
        remove = true;
    }

    if (remove) {
        impl_tmp_stats.remove(*i);

        //Drup -- Only delete once
        if (origLit1 < origLit2
            && origLit2 < origLit3
        ) {
            (*solver->drup)
            << del
            << origLit1
            << origLit2
            << origLit3
            << fin;
        }

        return;
    }

    //Order literals
    orderLits(lit1, lit2, lit3);

    //Now make into the order this TRI was in
    if (origLit1 > origLit2
        && origLit1 < origLit3
    ) {
        std::swap(lit1, lit2);
    }
    if (origLit1 > origLit2
        && origLit1 > origLit3
    ) {
        std::swap(lit1, lit3);
        std::swap(lit2, lit3);
    }
    i->setLit2(lit2);
    i->setLit3(lit3);

    //Drup
    if (//Changed
        (lit1 != origLit1
            || lit2 != origLit2
            || lit3 != origLit3
        )
        //Remove&attach only once
        && (origLit1 < origLit2
            && origLit2 < origLit3
        )
    ) {
        (*solver->drup)
        << lit1 << lit2 << lit3 << fin
        << del << origLit1 << origLit2  << origLit3 << fin;
    }

    if (lit1 != origLit1) {
        solver->watches[lit1.toInt()].push(*i);
    } else {
        *j++ = *i;
    }

    return;
}

void VarReplacer::updateBin(
    watch_subarray::iterator& i
    , watch_subarray::iterator& j
    , const Lit origLit1
    , const Lit origLit2
    , Lit lit1
    , Lit lit2
) {
    bool remove = false;

    //Two lits are the same in BIN
    if (lit1 == lit2) {
        delayedEnqueue.push_back(lit2);
        (*solver->drup) << lit2 << fin;
        remove = true;
    }

    //Tautology
    if (lit1 == ~lit2)
        remove = true;

    if (remove) {
        impl_tmp_stats.remove(*i);

        //Drup -- Delete only once
        if (origLit1 < origLit2) {
            (*solver->drup) << del << origLit1 << origLit2 << fin;
        }

        return;
    }

    //Drup
    if (//Changed
        (lit1 != origLit1
            || lit2 != origLit2)
        //Delete&attach only once
        && (origLit1 < origLit2)
    ) {
        (*solver->drup)
        << lit1 << lit2 << fin
        << del << origLit1 << origLit2 << fin;
    }

    if (lit1 != origLit1) {
        solver->watches[lit1.toInt()].push(*i);
    } else {
        *j++ = *i;
    }
}

void VarReplacer::updateStatsFromImplStats()
{
    assert(impl_tmp_stats.removedRedBin % 2 == 0);
    solver->binTri.redBins -= impl_tmp_stats.removedRedBin/2;

    assert(impl_tmp_stats.removedIrredBin % 2 == 0);
    solver->binTri.irredBins -= impl_tmp_stats.removedIrredBin/2;

    assert(impl_tmp_stats.removedRedTri % 3 == 0);
    solver->binTri.redTris -= impl_tmp_stats.removedRedTri/3;

    assert(impl_tmp_stats.removedIrredTri % 3 == 0);
    solver->binTri.irredTris -= impl_tmp_stats.removedIrredTri/3;

    #ifdef DEBUG_IMPLICIT_STATS
    solver->checkImplicitStats();
    #endif

    runStats.removedBinClauses += impl_tmp_stats.removedRedBin/2 + impl_tmp_stats.removedIrredBin/2;
    runStats.removedTriClauses += impl_tmp_stats.removedRedTri/3 + impl_tmp_stats.removedIrredTri/3;

    impl_tmp_stats.clear();
}

bool VarReplacer::replaceImplicit()
{
    impl_tmp_stats.clear();
    delayedEnqueue.clear();
    delayedAttach.clear();

    size_t wsLit = 0;
    for (auto
        it = solver->watches.begin(), end = solver->watches.end()
        ; it != end
        ; ++it, wsLit++
    ) {
        const Lit origLit1 = Lit::toLit(wsLit);
        watch_subarray ws = *it;

        watch_subarray::iterator i = ws.begin();
        watch_subarray::iterator j = i;
        for (watch_subarray::iterator end2 = ws.end(); i != end2; i++) {
            //Don't bother clauses
            if (i->isClause()) {
                *j++ = *i;
                continue;
            }

            const Lit origLit2 = i->lit2();
            assert(solver->value(origLit1) == l_Undef);
            assert(solver->value(origLit2) == l_Undef);
            assert(origLit1.var() != origLit2.var());

            //Update main lit
            Lit lit1 = origLit1;
            if (getVarReplacedWith(lit1) != lit1.var()) {
                lit1 = getLitReplacedWith(lit1);
                runStats.replacedLits++;
            }

            //Update lit2
            Lit lit2 = origLit2;
            if (getVarReplacedWith(lit2) != lit2.var()) {
                lit2 = getLitReplacedWith(lit2);
                i->setLit2(lit2);
                runStats.replacedLits++;
            }

            if (i->isTri()) {
                updateTri(i, j, origLit1, origLit2, lit1, lit2);
            } else {
                assert(i->isBinary());
                updateBin(i, j, origLit1, origLit2, lit1, lit2);
            }
        }
        ws.shrink_(i-j);
    }

    for(vector<BinaryClause>::const_iterator
        it = delayedAttach.begin(), end = delayedAttach.end()
        ; it != end
        ; it++
    ) {
        solver->attachBinClause(it->getLit1(), it->getLit2(), it->isRed());
    }
    delayedAttach.clear();

    #ifdef VERBOSE_DEBUG_BIN_REPLACER
    cout << "c debug bin replacer start" << endl;
    cout << "c debug bin replacer end" << endl;
    #endif

    updateStatsFromImplStats();

    return solver->ok;
}

/**
@brief Replaces variables in normal clauses
*/
bool VarReplacer::replace_set(vector<ClOffset>& cs)
{
    vector<ClOffset>::iterator i = cs.begin();
    vector<ClOffset>::iterator j = i;
    for (vector<ClOffset>::iterator end = cs.end(); i != end; i++) {
        Clause& c = *solver->clAllocator.getPointer(*i);
        assert(c.size() > 3);

        bool changed = false;
        Lit origLit1 = c[0];
        Lit origLit2 = c[1];

        (*solver->drup) << deldelay << c << fin;

        for (Lit& l: c) {
            if (isReplaced(l)) {
                changed = true;
                l = getLitReplacedWith(l);
                runStats.replacedLits++;
            }
        }

        if (changed && handleUpdatedClause(c, origLit1, origLit2)) {
            solver->clAllocator.clauseFree(*i);
            runStats.removedLongClauses++;
            if (!solver->ok) {
                return false;
            }
        } else {
            *j++ = *i;
        }
    }
    cs.resize(cs.size() - (i-j));

    return solver->ok;
}

/**
@brief Helper function for replace_set()
*/
bool VarReplacer::handleUpdatedClause(
    Clause& c
    , const Lit origLit1
    , const Lit origLit2
) {
    bool satisfied = false;
    std::sort(c.begin(), c.end());
    Lit p;
    uint32_t i, j;
    const uint32_t origSize = c.size();
    for (i = j = 0, p = lit_Undef; i != origSize; i++) {
        assert(solver->varData[c[i].var()].removed == Removed::none);
        if (solver->value(c[i]) == l_True || c[i] == ~p) {
            satisfied = true;
            break;
        }
        else if (solver->value(c[i]) != l_False && c[i] != p)
            c[j++] = p = c[i];
    }
    c.shrink(i - j);
    c.setChanged();

    solver->detachModifiedClause(origLit1, origLit2, origSize, &c);

    #ifdef VERBOSE_DEBUG
    cout << "clause after replacing: " << c << endl;
    #endif

    if (satisfied) {
        (*solver->drup) << findelay;
        return true;
    }
    (*solver->drup) << c << fin << findelay;

    switch(c.size()) {
    case 0:
        solver->ok = false;
        return true;
    case 1 :
        solver->enqueue(c[0]);
        #ifdef STATS_NEEDED
        solver->propStats.propsUnit++;
        #endif
        solver->ok = (solver->propagate().isNULL());
        runStats.removedLongLits += origSize;
        return true;
    case 2:
        solver->attachBinClause(c[0], c[1], c.red());
        runStats.removedLongLits += origSize;
        return true;

    case 3:
        solver->attachTriClause(c[0], c[1], c[2], c.red());
        runStats.removedLongLits += origSize;
        return true;

    default:
        solver->attachClause(c);
        runStats.removedLongLits += origSize - c.size();
        return false;
    }

    assert(false);
    return false;
}

/**
@brief Returns variables that have been replaced
*/
vector<Var> VarReplacer::getReplacingVars() const
{
    vector<Var> replacingVars;

    for(map<Var, vector<Var> >::const_iterator
        it = reverseTable.begin(), end = reverseTable.end()
        ; it != end
        ; it++
    ) {
        replacingVars.push_back(it->first);
    }

    return replacingVars;
}

void VarReplacer::set_sub_var_during_solution_extension(Var var, Var sub_var)
{
    lbool to_set = solver->model[var] ^ table[sub_var].sign();
    if (solver->model[sub_var] != l_Undef) {
        assert(solver->varData[sub_var].removed == Removed::queued_replacer
            || solver->varData[sub_var].removed == Removed::replaced
        );
        assert(solver->model[sub_var] == l_Undef
            || solver->varData[sub_var].level == 0
            || solver->varData[sub_var].removed == Removed::queued_replacer
        );
        assert(solver->model[sub_var] == to_set);
    } else {
        solver->model[sub_var] = to_set;
    }
}

void VarReplacer::extendModel(const Var var)
{
    assert(solver->model[var] != l_Undef);
    map<Var, vector<Var> >::const_iterator it
        = reverseTable.find(var);

    if (it == reverseTable.end())
        return;

    assert(it->first == var);
    for(const Var sub_var: it->second)
    {
        set_sub_var_during_solution_extension(var, sub_var);
    }
}

void VarReplacer::extendModel()
{
    #ifdef VERBOSE_DEBUG
    cout << "c VarReplacer::extendModel() called" << endl;
    #endif //VERBOSE_DEBUG

    assert(solver->model.size() == solver->nVarsReal());
    for (map<Var, vector<Var> >::const_iterator
        it = reverseTable.begin() , end = reverseTable.end()
        ; it != end
        ; it++
    ) {
        if (solver->model[it->first] == l_Undef)
            continue;

        for(const Var sub_var: it->second)
        {
            set_sub_var_during_solution_extension(it->first, sub_var);
        }
    }
}

void VarReplacer::replaceChecks(const Lit lit1, const Lit lit2) const
{

    assert(solver->ok);
    assert(solver->decisionLevel() == 0);
    assert(!lit1.sign());
    assert(!lit2.sign());
    assert(solver->value(lit1.var()) == l_Undef);
    assert(solver->value(lit2.var()) == l_Undef);

    assert(solver->varData[lit1.var()].removed == Removed::none
            || solver->varData[lit1.var()].removed == Removed::queued_replacer);
    assert(solver->varData[lit2.var()].removed == Removed::none
            || solver->varData[lit2.var()].removed == Removed::queued_replacer);
}

bool VarReplacer::handleAlreadyReplaced(const Lit lit1, const Lit lit2)
{
    //OOps, already inside, but with inverse polarity, UNSAT
    if (lit1.sign() != lit2.sign()) {
        //Drup
        (*solver->drup)
        << ~lit1 << lit2 << fin
        << lit1 << ~lit2 << fin
        << lit1 << fin
        << ~lit1 << fin;

        solver->ok = false;
        return false;
    }

    //Already inside in the correct way, return
    return true;
}

bool VarReplacer::replace_vars_already_set(
    const Lit lit1
    , const lbool val1
    , const Lit lit2
    , const lbool val2
) {
    if (val1 != val2) {
        (*solver->drup)
        << ~lit1 << fin
        << lit1 << fin;

        solver->ok = false;
    }

    //Already set, return with correct code
    return solver->ok;
}

bool VarReplacer::handleOneSet(
    const Lit lit1
    , const lbool val1
    , const Lit lit2
    , const lbool val2
) {
    if (solver->ok) {
        Lit toEnqueue;
        if (val1 != l_Undef) {
            toEnqueue = lit2 ^ (val1 == l_False);
        } else {
            toEnqueue = lit1 ^ (val2 == l_False);
        }
        solver->enqueue(toEnqueue);
        (*solver->drup) << toEnqueue << fin;

        #ifdef STATS_NEEDED
        solver->propStats.propsUnit++;
        #endif

        solver->ok = (solver->propagate().isNULL());
    }
    return solver->ok;
}

/**
@brief Replaces two two lits with one another
*/
bool VarReplacer::replace(
    Lit lit1
    , Lit lit2
    , const bool xorEqualFalse
    , bool addLaterAsTwoBins
)
{
    #ifdef VERBOSE_DEBUG
    cout << "replace() called with var " << lit1 << " and var " << lit2 << " with xorEqualFalse " << xorEqualFalse << endl;
    #endif

    replaceChecks(lit1, lit2);

    #ifdef DRUP_DEBUG
    (*solver->drup)
    << ~lit1 << " " << (lit2 ^!xorEqualFalse) fin
    << lit1 << " " << (~lit2 ^!xorEqualFalse) fin
    ;
    #endif

    //Move forward circle
    lit1 = table[lit1.var()];
    lit2 = table[lit2.var()] ^ !xorEqualFalse;

    //Already inside?
    if (lit1.var() == lit2.var()) {
        return handleAlreadyReplaced(lit1, lit2);
    }

    //Not already inside
    (*solver->drup)
    << ~lit1 << lit2 << fin
    << lit1 << ~lit2 << fin;

    //None should be removed, only maybe queued for replacement
    assert(solver->varData[lit1.var()].removed == Removed::none
            || solver->varData[lit1.var()].removed == Removed::queued_replacer);
    assert(solver->varData[lit2.var()].removed == Removed::none
            || solver->varData[lit2.var()].removed == Removed::queued_replacer);

    lbool val1 = solver->value(lit1);
    lbool val2 = solver->value(lit2);

    //Both are set
    if (val1 != l_Undef && val2 != l_Undef) {
        return replace_vars_already_set(lit1, val1, lit2, val2);
    }

    //exactly one set
    if ((val1 != l_Undef && val2 == l_Undef)
        || (val2 != l_Undef && val1 == l_Undef)
    ) {
        return handleOneSet(lit1, val1, lit2, val2);
    }

    assert(val1 == l_Undef && val2 == l_Undef);

    if (addLaterAsTwoBins)
        laterAddBinXor.push_back(LaterAddBinXor(lit1, lit2^true));

    solver->varData[lit1.var()].removed = Removed::queued_replacer;
    solver->varData[lit2.var()].removed = Removed::queued_replacer;
    if (reverseTable.find(lit1.var()) == reverseTable.end()) {
        reverseTable[lit2.var()].push_back(lit1.var());
        table[lit1.var()] = lit2 ^ lit1.sign();
        replacedVars++;
        return true;
    }

    if (reverseTable.find(lit2.var()) == reverseTable.end()) {
        reverseTable[lit1.var()].push_back(lit2.var());
        table[lit2.var()] = lit1 ^ lit2.sign();
        replacedVars++;
        return true;
    }

    //both have children
    setAllThatPointsHereTo(lit1.var(), lit2 ^ lit1.sign()); //erases reverseTable[lit1.var()]
    replacedVars++;
    return true;
}

/**
@brief Returns if we already know that var = lit

Also checks if var = ~lit, in which it sets solver->ok = false
*/
bool VarReplacer::alreadyIn(const Var var, const Lit lit)
{
    Lit lit2 = table[var];
    if (lit2.var() == lit.var()) {
        if (lit2.sign() != lit.sign()) {
            #ifdef VERBOSE_DEBUG
            cout << "Inverted cycle in var-replacement -> UNSAT" << endl;
            #endif
            solver->ok = false;
        }
        return true;
    }

    lit2 = table[lit.var()];
    if (lit2.var() == var) {
        if (lit2.sign() != lit.sign()) {
            #ifdef VERBOSE_DEBUG
            cout << "Inverted cycle in var-replacement -> UNSAT" << endl;
            #endif
            solver->ok = false;
        }
        return true;
    }

    return false;
}

/**
@brief Changes internal graph to set everything that pointed to var to point to lit
*/
void VarReplacer::setAllThatPointsHereTo(const Var var, const Lit lit)
{
    map<Var, vector<Var> >::iterator it = reverseTable.find(var);
    if (it != reverseTable.end()) {
        for(vector<Var>::const_iterator it2 = it->second.begin(), end = it->second.end(); it2 != end; it2++) {
            assert(table[*it2].var() == var);
            if (lit.var() != *it2) {
                table[*it2] = lit ^ table[*it2].sign();
                reverseTable[lit.var()].push_back(*it2);
            }
        }
        reverseTable.erase(it);
    }
    table[var] = lit;
    reverseTable[lit.var()].push_back(var);
}

void VarReplacer::newVar()
{
    table.push_back(Lit(table.size(), false));
}

void VarReplacer::updateVars(
    const std::vector< uint32_t >& outerToInter
    , const std::vector< uint32_t >& interToOuter
) {
    assert(laterAddBinXor.empty());

    updateArray(table, interToOuter);
    updateLitsMap(table, outerToInter);
    map<Var, vector<Var> > newReverseTable;
    for(map<Var, vector<Var> >::iterator
        it = reverseTable.begin(), end = reverseTable.end()
        ; it != end
        ; it++
    ) {
        updateArrayMapCopy(it->second, outerToInter);
        newReverseTable[outerToInter.at(it->first)] = it->second;
    }
    reverseTable.swap(newReverseTable);
}

void VarReplacer::checkUnsetSanity()
{
    for(size_t i = 0; i < solver->nVars(); i++) {
        const Lit repLit = getLitReplacedWith(Lit(i, false));
        const Var repVar = getVarReplacedWith(i);

        if ((solver->varData[i].removed == Removed::none
                || solver->varData[i].removed == Removed::queued_replacer)
            && (solver->varData[repVar].removed == Removed::none
                || solver->varData[repVar].removed == Removed::queued_replacer)
            && solver->value(i) != solver->value(repLit)
        ) {
            cout
            << "Variable " << (i+1)
            << " has been set to " << solver->value(i)
            << " but it has been replaced with lit "
            << getLitReplacedWith(Lit(i, false))
            << " and that has been set to "
            << solver->value(getLitReplacedWith(Lit(i, false)))
            << endl;

            assert(solver->value(i) == solver->value(repLit));
            exit(-1);
        }
    }
}

bool VarReplacer::addLaterAddBinXor()
{
    assert(solver->ok);

    vector<Lit> ps(2);
    for(vector<LaterAddBinXor>::const_iterator
        it = laterAddBinXor.begin(), end = laterAddBinXor.end()
        ; it != end
        ; it++
    ) {
        ps[0] = it->lit1;
        ps[1] = it->lit2;
        solver->addClauseInt(ps);
        if (!solver->ok)
            return false;

        ps[0] ^= true;
        ps[1] ^= true;
        solver->addClauseInt(ps);
        if (!solver->ok)
            return false;
    }
    laterAddBinXor.clear();

    return true;
}

size_t VarReplacer::memUsed() const
{
    size_t b = 0;
    b += delayedEnqueue.capacity()*sizeof(Lit);
    b += laterAddBinXor.capacity()*sizeof(LaterAddBinXor);
    b += table.capacity()*sizeof(Lit);
    for(map<Var, vector<Var> >::const_iterator
        it = reverseTable.begin(), end = reverseTable.end()
        ; it != end
        ; it++
    ) {
        b += it->second.capacity()*sizeof(Lit);
    }
    b += reverseTable.size()*(sizeof(Var) + sizeof(vector<Var>)); //TODO under-counting

    return b;
}
