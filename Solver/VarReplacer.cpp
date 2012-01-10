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

#include "VarReplacer.h"
#include <iostream>
#include <iomanip>
#include <set>

#include "ThreadControl.h"
#include "ClauseCleaner.h"
#include "time_mem.h"
#include "SolutionExtender.h"
#include "ClauseAllocator.h"

#ifdef VERBOSE_DEBUG
#define REPLACE_STATISTICS
#define VERBOSE_DEBUG_BIN_REPLACER
#endif

//#define VERBOSE_DEBUG
//#define REPLACE_STATISTICS
//#define DEBUG_BIN_REPLACER
//#define VERBOSE_DEBUG_BIN_REPLACER

VarReplacer::VarReplacer(ThreadControl* _control) :
    replacedLits(0)
    , replacedVars(0)
    , lastReplacedVars(0)
    , control(_control)
{
}

VarReplacer::~VarReplacer()
{
}

/**
@brief Replaces variables, clears internal clauses, and reports stats

When replacing, it is imperative not to make variables decision variables
which have been removed by other methods:
\li variable removal at the xor-sphere
\li disconnected component finding and solving
\li variable elimination

NOTE: If any new such algoirhtms are added, this part MUST be updated such
that problems don't creep up
*/
bool VarReplacer::performReplace()
{
    #ifdef VERBOSE_DEBUG
    std::cout << "PerformReplacInternal started." << std::endl;
    //control->printAllClauses();
    #endif
    double time = cpuTime();

    #ifdef REPLACE_STATISTICS
    uint32_t numRedir = 0;
    for (uint32_t i = 0; i < table.size(); i++) {
        if (table[i].var() != i)
            numRedir++;
    }
    std::cout << "c Number of trees:" << reverseTable.size() << std::endl;
    std::cout << "c Number of redirected nodes:" << numRedir << std::endl;
    #endif //REPLACE_STATISTICS

    control->clauseCleaner->removeAndCleanAll();
    if (!control->ok) return false;
    control->testAllClauseAttach();

    #ifdef VERBOSE_DEBUG
    {
        uint32_t i = 0;
        for (vector<Lit>::const_iterator it = table.begin(); it != table.end(); it++, i++) {
            if (it->var() == i) continue;
            std::cout << "Replacing var " << i+1 << " with Lit " << *it << std::endl;
        }
    }
    #endif

    Var var = 0;
    for (vector<Lit>::const_iterator it = table.begin(); it != table.end(); it++, var++) {

        //Was queued for replacement, but it's the top of the tree, so, it's normal again
        if (it->var() == var && control->varData[it->var()].elimed == ELIMED_QUEUED_VARREPLACER)
            control->varData[it->var()].elimed = ELIMED_NONE;

        if (it->var() == var
            || control->varData[it->var()].elimed == ELIMED_DECOMPOSE
            || control->varData[it->var()].elimed == ELIMED_VARELIM
        ) continue;
        control->varData[var].elimed = ELIMED_VARREPLACER;

        #ifdef VERBOSE_DEBUG
        std::cout << "Setting var " << var+1 << " to a non-decision var" << std::endl;
        #endif
        bool wasDecisionVar = control->decision_var[var];
        control->decision_var[var] =  false;
        control->decision_var[it->var()] = true;

        uint32_t& activity1 = control->varData[var].activity;
        uint32_t& activity2 = control->varData[it->var()].activity;
        if (wasDecisionVar && activity1 > activity2) {
            activity2 = activity1;
        }

        activity1 = 0.0;
    }

    uint32_t thisTimeReplaced = replacedVars -lastReplacedVars;
    lastReplacedVars = replacedVars;

    control->testAllClauseAttach();
    assert(control->qhead == control->trail.size());

    control->countNumBinClauses(true, false);
    control->countNumBinClauses(false, true);

    if (!replaceBins()) goto end;
    if (!replace_set(control->clauses)) goto end;
    if (!replace_set(control->learnts)) goto end;

    control->testAllClauseAttach();
    control->checkNoWrongAttach();

end:
    assert(control->qhead == control->trail.size() || !control->ok);

    if (control->conf.verbosity  >= 2) {
        std::cout << "c Replacing "
        << std::setw(8) << thisTimeReplaced << " vars"
        << " Replaced " <<  std::setw(8) << replacedLits<< " lits"
        << " Time: " << std::setw(8) << std::fixed << std::setprecision(2)
        << cpuTime()-time << " s "
        << std::endl;
    }

    replacedLits = 0;

    return control->ok;
}

bool VarReplacer::replaceBins()
{
    #ifdef DEBUG_BIN_REPLACER
    vector<uint32_t> removed(control->nVars()*2, 0);
    uint32_t replacedLitsBefore = replacedLits;
    #endif

    uint32_t removedLearnt = 0;
    uint32_t removedNonLearnt = 0;
    uint32_t wsLit = 0;
    for (vector<vec<Watched> >::iterator it = control->watches.begin(), end = control->watches.end(); it != end; it++, wsLit++) {
        Lit lit1 = ~Lit::toLit(wsLit);
        vec<Watched>& ws = *it;

        vec<Watched>::iterator i = ws.begin();
        vec<Watched>::iterator j = i;
        for (vec<Watched>::iterator end2 = ws.end(); i != end2; i++) {
            if (!i->isBinary()) {
                *j++ = *i;
                continue;
            }
            //std::cout << "bin: " << lit1 << " , " << i->getOtherLit() << " learnt : " <<  (i->isLearnt()) << std::endl;
            Lit thisLit1 = lit1;
            Lit lit2 = i->getOtherLit();
            assert(thisLit1.var() != lit2.var());

            if (table[lit2.var()].var() != lit2.var()) {
                lit2 = table[lit2.var()] ^ lit2.sign();
                i->setOtherLit(lit2);
                replacedLits++;
            }

            bool changedMain = false;
            if (table[thisLit1.var()].var() != thisLit1.var()) {
                thisLit1 = table[thisLit1.var()] ^ thisLit1.sign();
                replacedLits++;
                changedMain = true;
            }

            if (thisLit1 == lit2) {
                if (control->value(lit2) == l_Undef) {
                    control->enqueue(lit2);
                } else if (control->value(lit2) == l_False) {
                    #ifdef VERBOSE_DEBUG
                    std::cout << "Contradiction during replacement of lits in binary clause" << std::endl;
                    #endif
                    control->ok = false;
                }
                #ifdef DEBUG_BIN_REPLACER
                removed[lit1.toInt()]++;
                removed[origLit2.toInt()]++;
                #endif

                if (i->getLearnt()) removedLearnt++;
                else removedNonLearnt++;
                continue;
            }

            if (thisLit1 == ~lit2) {
                #ifdef DEBUG_BIN_REPLACER
                removed[lit1.toInt()]++;
                removed[origLit2.toInt()]++;
                #endif

                if (i->getLearnt()) removedLearnt++;
                else removedNonLearnt++;
                continue;
            }

            if (changedMain) {
                control->watches[(~thisLit1).toInt()].push(*i);
            } else {
                *j++ = *i;
            }
        }
        ws.shrink_(i-j);
    }

    #ifdef VERBOSE_DEBUG_BIN_REPLACER
    std::cout << "c debug bin replacer start" << std::endl;
    std::cout << "c removedLearnt: " << removedLearnt << std::endl;
    std::cout << "c removedNonLearnt: " << removedNonLearnt << std::endl;
    std::cout << "c debug bin replacer end" << std::endl;
    #endif

    assert(removedLearnt % 2 == 0);
    assert(removedNonLearnt % 2 == 0);
    control->learntsLits -= removedLearnt;
    control->clausesLits -= removedNonLearnt;
    control->numBins -= (removedLearnt + removedNonLearnt)/2;

    if (control->ok) control->ok = (control->propagate().isNULL());
    return control->ok;
}

/**
@brief Replaces variables in normal clauses
*/
bool VarReplacer::replace_set(vector<Clause*>& cs)
{
    vector<Clause*>::iterator a = cs.begin();
    vector<Clause*>::iterator r = a;
    for (vector<Clause*>::iterator end = a + cs.size(); r != end; r++) {
        Clause& c = **r;
        assert(c.size() > 2);

        bool changed = false;
        Lit origLit1 = lit_Undef;
        Lit origLit2 = lit_Undef;
        Lit origLit3 = lit_Undef;
        if (c.size() == 3) {
            origLit1 = c[0];
            origLit2 = c[1];
            origLit3 = c[2];
        } else {
            const ClauseData& data = control->clauseData[c.getNum()];
            origLit1 = c[data[0]];
            origLit2 = c[data[1]];
        }

        for (Lit *l = c.begin(), *end2 = l + c.size();  l != end2; l++) {
            if (table[l->var()].var() != l->var()) {
                changed = true;
                *l = table[l->var()] ^ l->sign();
                replacedLits++;
            }
        }

        if (changed && handleUpdatedClause(c, origLit1, origLit2, origLit3)) {
            control->clAllocator->clauseFree(*r);
            if (!control->ok) {
                r++;
                #ifdef VERBOSE_DEBUG
                std::cout << "contradiction while replacing lits in normal clause" << std::endl;
                #endif
                for(;r != end; r++) control->clAllocator->clauseFree(*r);
                cs.resize(cs.size() - (r-a));
                return false;
            }
        } else {
            *a++ = *r;
        }
    }
    cs.resize(cs.size() - (r-a));

    return control->ok;
}

/**
@brief Helper function for replace_set()
*/
bool VarReplacer::handleUpdatedClause(Clause& c, const Lit origLit1, const Lit origLit2, const Lit origLit3)
{
    bool satisfied = false;
    std::sort(c.begin(), c.begin() + c.size());
    Lit p;
    uint32_t i, j;
    const uint32_t origSize = c.size();
    for (i = j = 0, p = lit_Undef; i != origSize; i++) {
        assert(control->varData[c[i].var()].elimed == ELIMED_NONE);
        if (control->value(c[i]) == l_True || c[i] == ~p) {
            satisfied = true;
            break;
        }
        else if (control->value(c[i]) != l_False && c[i] != p)
            c[j++] = p = c[i];
    }
    c.shrink(i - j);
    c.setChanged();

    control->detachModifiedClause(origLit1, origLit2, origLit3, origSize, &c);

    #ifdef VERBOSE_DEBUG
    std::cout << "clause after replacing: " << c << std::endl;
    #endif

    if (satisfied) return true;

    switch(c.size()) {
    case 0:
        control->ok = false;
        return true;
    case 1 :
        control->enqueue(c[0]);
        control->ok = (control->propagate().isNULL());
        return true;
    case 2:
        control->attachBinClause(c[0], c[1], c.learnt());
        return true;
    case 3:
        control->clAllocator->releaseClauseNum(&c);
        //NO BREAK, since 3-long is just like a long one, except
        //we might need to free the clause number
    default:
        control->attachClause(c);
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

    for(map<Var, vector<Var> >::const_iterator it = reverseTable.begin(), end = reverseTable.end(); it != end; it++) {
        replacingVars.push_back(it->first);
    }

    return replacingVars;
}

/**
@brief Used when a variable was eliminated, but it replaced some other variables

This function will add to solver2 clauses that represent the relationship of
the variables to their replaced cousins. Then, calling solver2.solve() should
take care of everything
*/
void VarReplacer::extendModel(SolutionExtender* extender) const
{

    #ifdef VERBOSE_DEBUG
    std::cout << "c VarReplacer::extendModel() called" << std::endl;
    #endif //VERBOSE_DEBUG

    vector<Lit> tmpClause;
    uint32_t i = 0;
    for (vector<Lit>::const_iterator it = table.begin(); it != table.end(); it++, i++) {
        if (it->var() == i) continue;
        tmpClause.clear();
        tmpClause.push_back(Lit(it->var(), true));
        tmpClause.push_back(Lit(i, it->sign()));
        bool OK = extender->addClause(tmpClause);
        assert(OK);

        tmpClause.clear();
        tmpClause.push_back(Lit(it->var(), false));
        tmpClause.push_back(Lit(i, it->sign()^true));
        OK = extender->addClause(tmpClause);
        assert(OK);
    }
}

/**
@brief Replaces two two vars in "ps" with one another. xorEqualFalse defines anti/equivalence

It can be tricky to do this. For example, if:

\li a replaces: b, c
\li f replaces: f, h
\li we just realised that c = h
This is the most difficult case, but there are other cases, e.g. if we already
know that c=h, in which case we don't do anything

@p ps must contain 2 variables(!), i.e literals with no sign
@p xorEqualFalse if True, the two variables are equivalent. Otherwise, they are antivalent
*/
bool VarReplacer::replace(Lit lit1, Lit lit2, const bool xorEqualFalse)
{
    #ifdef VERBOSE_DEBUG
    std::cout << "replace() called with var " << lit1 << " and var " << lit2 << " with xorEqualFalse " << xorEqualFalse << std::endl;
    #endif

    assert(control->ok);
    assert(control->decisionLevel() == 0);
    assert(!lit1.sign());
    assert(!lit2.sign());
    assert(control->value(lit1.var()) == l_Undef);
    assert(control->value(lit2.var()) == l_Undef);

    assert(control->varData[lit1.var()].elimed == ELIMED_NONE
            || control->varData[lit1.var()].elimed == ELIMED_QUEUED_VARREPLACER);
    assert(control->varData[lit2.var()].elimed == ELIMED_NONE
            || control->varData[lit2.var()].elimed == ELIMED_QUEUED_VARREPLACER);

    //Move forward circle
    lit1 = table[lit1.var()];
    lit2 = table[lit2.var()] ^ !xorEqualFalse;

    //Already inside?
    if (lit1.var() == lit2.var()) {
        if (lit1.sign() != lit2.sign()) {
            control->ok = false;
            return false;
        }
        return true;
    }

    //Even the moved-forward version must be unelimed
    assert(control->varData[lit1.var()].elimed == ELIMED_NONE
            || control->varData[lit1.var()].elimed == ELIMED_QUEUED_VARREPLACER);
    assert(control->varData[lit2.var()].elimed == ELIMED_NONE
            || control->varData[lit2.var()].elimed == ELIMED_QUEUED_VARREPLACER);

    lbool val1 = control->value(lit1);
    lbool val2 = control->value(lit2);
    if (val1 != l_Undef && val2 != l_Undef) {
        if (val1 != val2) control->ok = false;
        return control->ok;
    }

    if ((val1 != l_Undef && val2 == l_Undef) || (val2 != l_Undef && val1 == l_Undef)) {
        //exactly one l_Undef, exectly one l_True/l_False
        if (val1 != l_Undef) control->enqueue(lit2 ^ (val1 == l_False));
        else control->enqueue(lit1 ^ (val2 == l_False));

        if (control->ok) control->ok = (control->propagate().isNULL());
        return control->ok;
    }

    assert(val1 == l_Undef && val2 == l_Undef);

    laterAddBinXor.push_back(LaterAddBinXor(lit1, lit2^true));

    control->varData[lit1.var()].elimed = ELIMED_QUEUED_VARREPLACER;
    control->varData[lit2.var()].elimed = ELIMED_QUEUED_VARREPLACER;
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

Also checks if var = ~lit, in which it sets control->ok = false
*/
bool VarReplacer::alreadyIn(const Var var, const Lit lit)
{
    Lit lit2 = table[var];
    if (lit2.var() == lit.var()) {
        if (lit2.sign() != lit.sign()) {
            #ifdef VERBOSE_DEBUG
            std::cout << "Inverted cycle in var-replacement -> UNSAT" << std::endl;
            #endif
            control->ok = false;
        }
        return true;
    }

    lit2 = table[lit.var()];
    if (lit2.var() == var) {
        if (lit2.sign() != lit.sign()) {
            #ifdef VERBOSE_DEBUG
            std::cout << "Inverted cycle in var-replacement -> UNSAT" << std::endl;
            #endif
            control->ok = false;
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

bool VarReplacer::addLaterAddBinXor()
{
    assert(control->ok);

    vector<Lit> ps(2);
    for(vector<LaterAddBinXor>::const_iterator it = laterAddBinXor.begin(), end = laterAddBinXor.end(); it != end; it++) {
        ps[0] = it->lit1;
        ps[1] = it->lit2;
        control->addClauseInt(ps, false);
        if (!control->ok)
            return false;

        ps[0] ^= true;
        ps[1] ^= true;
        control->addClauseInt(ps, false);
        if (!control->ok)
            return false;
    }
    laterAddBinXor.clear();

    return true;
}
