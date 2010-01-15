/***********************************************************************************
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************************/

#include "VarReplacer.h"

#include "Solver.h"
#include "Conglomerate.h"
#include "ClauseCleaner.h"

//#define VERBOSE_DEBUG
//#define DEBUG_REPLACER

#ifdef VERBOSE_DEBUG
#include <iostream>
using std::cout;
using std::endl;
#endif

VarReplacer::VarReplacer(Solver *_s) :
    replacedLits(0)
    , lastReplacedLits(0)
    , replacedVars(0)
    , lastReplacedVars(0)
    , addedNewClause(false)
    , S(_s)
{
}

VarReplacer::~VarReplacer()
{
    for (uint i = 0; i != clauses.size(); i++)
        //binaryClausePool.free(clauses[i]);
        free(clauses[i]);
}

void VarReplacer::performReplace()
{
    #ifdef VERBOSE_DEBUG
    cout << "Replacer started." << endl;
    #endif
    
    S->clauseCleaner->cleanClauses(S->clauses, ClauseCleaner::clauses);
    S->clauseCleaner->cleanClauses(S->learnts, ClauseCleaner::learnts);
    S->clauseCleaner->cleanClauses(S->xorclauses, ClauseCleaner::xorclauses);
    S->clauseCleaner->removeSatisfied(S->binaryClauses, ClauseCleaner::binaryClauses);
    
    if (replacedVars == lastReplacedVars) return;
    
    #ifdef VERBOSE_DEBUG
    {
        uint i = 0;
        for (vector<Lit>::const_iterator it = table.begin(); it != table.end(); it++, i++) {
            if (it->var() == i) continue;
            cout << "Replacing var " << i+1 << " with Lit " << (it->sign() ? "-" : "") <<  it->var()+1 << endl;
        }
    }
    #endif
    
    uint i = 0;
    const vector<bool>& removedVars = S->conglomerate->getRemovedVars();
    for (vector<Lit>::const_iterator it = table.begin(); it != table.end(); it++, i++) {
        if (it->var() == i) continue;
        #ifdef VERBOSE_DEBUG
        cout << "Setting var " << i+1 << " to a non-decision var" << endl;
        #endif
        S->setDecisionVar(i, false);
        if (!removedVars[it->var()])
            S->setDecisionVar(it->var(), true);
        
        double& activity1 = S->activity[i];
        double& activity2 = S->activity[it->var()];
        if (activity1 > activity2) {
            activity2 = activity1;
            S->order_heap.update(it->var());
        }
        activity1 = 0.0;
        S->order_heap.update(i);
    }
    assert(S->order_heap.heapProperty());
    
    replace_set(S->clauses);
    replace_set(S->learnts);
    replace_set(S->binaryClauses);
    
    replace_set(S->xorclauses, true);
    replace_set(S->conglomerate->getCalcAtFinish(), false);
    
    for (uint i = 0; i != clauses.size(); i++)
        S->removeClause(*clauses[i]);
    clauses.clear();
    
    if (S->verbosity >=1)
        printf("c |  Replacing   %8d vars, replaced %8d lits                          |\n", replacedVars-lastReplacedVars, replacedLits-lastReplacedLits);
    
    addedNewClause = false;
    lastReplacedVars = replacedVars;
    lastReplacedLits = replacedLits;
    
    if (S->ok)
        S->ok = (S->propagate() == NULL);
    
    S->order_heap.filter(Solver::VarFilter(*S));
}

void VarReplacer::replace_set(vec<XorClause*>& cs, const bool isAttached)
{
    XorClause **a = cs.getData();
    XorClause **r = a;
    for (XorClause **end = a + cs.size(); r != end;) {
        XorClause& c = **r;
        
        bool changed = false;
        Var origVar1 = c[0].var();
        Var origVar2 = c[1].var();
        for (Lit *l = &c[0], *lend = l + c.size(); l != lend; l++) {
            Lit newlit = table[l->var()];
            if (newlit.var() != l->var()) {
                changed = true;
                *l = Lit(newlit.var(), false);
                c.invert(newlit.sign());
                replacedLits++;
            }
        }
        
        if (isAttached && changed && handleUpdatedClause(c, origVar1, origVar2)) {
            c.mark(1);
            S->freeLater.push(&c);
            r++;
        } else {
            *a++ = *r++;
        }
    }
    cs.shrink(r-a);
}

const bool VarReplacer::handleUpdatedClause(XorClause& c, const Var origVar1, const Var origVar2)
{
    uint origSize = c.size();
    std::sort(c.getData(), c.getData() + c.size());
    Lit p;
    uint32_t i, j;
    for (i = j = 0, p = lit_Undef; i != c.size(); i++) {
        c[i] = c[i].unsign();
        if (c[i] == p) {
            //added, but easily removed
            j--;
            p = lit_Undef;
            if (!S->assigns[c[i].var()].isUndef())
                c.invert(S->assigns[c[i].var()].getBool());
        } else if (S->assigns[c[i].var()].isUndef()) //just add
            c[j++] = p = c[i];
        else c.invert(S->assigns[c[i].var()].getBool()); //modify xor_clause_inverted instead of adding
    }
    c.shrink(i - j);
    
    #ifdef VERBOSE_DEBUG
    cout << "xor-clause after replacing: ";
    c.plainPrint();
    #endif
    
    switch (c.size()) {
    case 0:
        S->detachModifiedClause(origVar1, origVar2, origSize, &c);
        if (!c.xor_clause_inverted())
            S->ok = false;
        return true;
    case 1:
        S->detachModifiedClause(origVar1, origVar2, origSize, &c);
        S->uncheckedEnqueue(c[0] ^ c.xor_clause_inverted());
        return true;
    case 2: {
        S->detachModifiedClause(origVar1, origVar2, origSize, &c);
        vec<Lit> ps(2);
        ps[0] = c[0];
        ps[1] = c[1];
        addBinaryXorClause(ps, c.xor_clause_inverted(), c.getGroup(), true);
        return true;
    }
    default:
        if (origVar1 != c[0].var() || origVar2 != c[1].var()) {
            S->detachModifiedClause(origVar1, origVar2, origSize, &c);
            S->attachClause(c);
        }
        return false;
    }
    
    assert(false);
    return false;
}

void VarReplacer::replace_set(vec<Clause*>& cs)
{
    Clause **a = cs.getData();
    Clause **r = a;
    for (Clause **end = a + cs.size(); r != end; ) {
        Clause& c = **r;
        bool changed = false;
        Lit origLit1 = c[0];
        Lit origLit2 = c[1];
        for (Lit *l = c.getData(), *end = l + c.size();  l != end; l++) {
            if (table[l->var()].var() != l->var()) {
                changed = true;
                *l = table[l->var()] ^ l->sign();
                replacedLits++;
            }
        }
        
        if (changed && handleUpdatedClause(c, origLit1, origLit2)) {
            clauseFree(&c);
            r++;
        } else {
            *a++ = *r++;
        }
    }
    cs.shrink(r-a);
}

const bool VarReplacer::handleUpdatedClause(Clause& c, const Lit origLit1, const Lit origLit2)
{
    bool satisfied = false;
    std::sort(c.getData(), c.getData() + c.size());
    Lit p;
    uint32_t i, j;
    const uint origSize = c.size();
    for (i = j = 0, p = lit_Undef; i != origSize; i++) {
        if (S->value(c[i]) == l_True || c[i] == ~p) {
            satisfied = true;
            break;
        }
        else if (S->value(c[i]) != l_False && c[i] != p)
            c[j++] = p = c[i];
    }
    c.shrink(i - j);
    
    if (satisfied) {
        S->detachModifiedClause(origLit1, origLit2, origSize, &c);
        return true;
    }
    
    switch(c.size()) {
    case 0:
        S->detachModifiedClause(origLit1, origLit2, origSize, &c);
        S->ok = false;
        return true;
    case 1 :
        S->detachModifiedClause(origLit1, origLit2, origSize, &c);
        S->uncheckedEnqueue(c[0]);
        return true;
    case 2:
        S->detachModifiedClause(origLit1, origLit2, origSize, &c);
        S->attachClause(c);
        return false;
    default:
        if (origLit1 != c[0] || origLit2 != c[1]) {
            S->detachModifiedClause(origLit1, origLit2, origSize, &c);
            S->attachClause(c);
        }
        return false;
    }
    
    assert(false);
    return false;
}

const uint VarReplacer::getNumReplacedLits() const
{
    return replacedLits;
}

const uint VarReplacer::getNumReplacedVars() const
{
    return replacedVars;
}

const uint VarReplacer::getNumLastReplacedVars() const
{
    return lastReplacedVars;
}

const uint VarReplacer::getNewToReplaceVars() const
{
    return replacedVars-lastReplacedVars;
}

const vector<Lit>& VarReplacer::getReplaceTable() const
{
    return table;
}

const vector<Var> VarReplacer::getReplacingVars() const
{
    vector<Var> replacingVars;
    
    for(map<Var, vector<Var> >::const_iterator it = reverseTable.begin(), end = reverseTable.end(); it != end; it++) {
        replacingVars.push_back(it->first);
    }
    
    return replacingVars;
}

const vec<Clause*>& VarReplacer::getClauses() const
{
    return clauses;
}

void VarReplacer::extendModel() const
{
    uint i = 0;
    for (vector<Lit>::const_iterator it = table.begin(); it != table.end(); it++, i++) {
        if (it->var() == i) continue;
        
        #ifdef VERBOSE_DEBUG
        cout << "Extending model: var "; S->printLit(Lit(i, false));
        cout << " to "; S->printLit(*it);
        cout << endl;
        #endif
        
        assert(S->assigns[it->var()] != l_Undef);
        if (S->assigns[i] == l_Undef) {
            bool val = (S->assigns[it->var()] == l_False);
            S->uncheckedEnqueue(Lit(i, val ^ it->sign()));
        } else {
            assert(S->assigns[i].getBool() == (S->assigns[it->var()].getBool() ^ it->sign()));
        }
    }
}

void VarReplacer::replace(vec<Lit>& ps, const bool xor_clause_inverted, const uint group)
{
    #ifdef VERBOSE_DEBUG
    cout << "replace() called with var " << ps[0].var()+1 << " and var " << ps[1].var()+1 << " with xor_clause_inverted " << xor_clause_inverted << endl;
    #endif
    
    #ifdef DEBUG_REPLACER
    assert(ps.size() == 2);
    assert(!ps[0].sign());
    assert(!ps[1].sign());
    assert(S->assigns[ps[0].var()].isUndef());
    assert(S->assigns[ps[1].var()].isUndef());
    #endif
    
    
    addBinaryXorClause(ps, xor_clause_inverted, group);
    Var var = ps[0].var();
    Lit lit = Lit(ps[1].var(), !xor_clause_inverted);
    assert(var != lit.var());
    
    //Detect circle
    if (alreadyIn(var, lit)) return;
    replacedVars++;
    
    Lit lit1 = table[var];
    bool inverted = false;
    
    //This pointer is already set, try to invert
    if (lit1.var() != var) {
        Var tmp_var = var;
        
        var = lit.var();
        lit = Lit(tmp_var, lit.sign());
        inverted = true;
    }
    
    if (inverted) {
        //Inversion is also set
        Lit lit2 = table[var];
        
        //triangular cycle
        if (lit1.var() == lit2.var()) {
            if ((lit1.sign() ^ lit2.sign()) != lit.sign()) {
                #ifdef VERBOSE_DEBUG
                cout << "Inverted cycle in var-replacement -> UNSAT" << endl;
                #endif
                S->ok = false;
            }
            return;
        }
        
        if (lit2.var() != var) {
            setAllThatPointsHereTo(lit1.var(), Lit(lit.var(), lit1.sign()));
            table[lit1.var()] = Lit(lit.var(), lit1.sign());
            reverseTable[lit.var()].push_back(lit1.var());
            
            setAllThatPointsHereTo(lit2.var(), lit ^ lit2.sign());
            table[lit2.var()] = lit ^ lit2.sign();
            reverseTable[lit.var()].push_back(lit2.var());
            
            table[lit.var()] = Lit(lit.var(), false);
            return;
        }
    }
    
    //Follow forwards
    Lit lit2 = table[lit.var()];
    if (lit2.var() != lit.var())
        lit = lit2 ^ lit.sign();
    
    //Follow backwards
    setAllThatPointsHereTo(var, lit);
    
    table[var] = lit;
    reverseTable[lit.var()].push_back(var);
}

void VarReplacer::addBinaryXorClause(vec<Lit>& ps, const bool xor_clause_inverted, const uint group, const bool internal)
{
    #ifdef DEBUG_REPLACER
    assert(!ps[0].sign());
    assert(!ps[1].sign());
    #endif
    
    Clause* c;
    ps[0] ^= xor_clause_inverted;
    
    c = Clause_new(ps, group, false);
    if (internal) {
        S->binaryClauses.push(c);
        S->becameBinary++;
    } else
        clauses.push(c);
    S->attachClause(*c);
    
    ps[0] ^= true;
    ps[1] ^= true;
    c = Clause_new(ps, group, false);
    if (internal) {
        S->binaryClauses.push(c);
        S->becameBinary++;
    } else
        clauses.push(c);
    S->attachClause(*c);
}

bool VarReplacer::alreadyIn(const Var var, const Lit lit)
{
    Lit lit2 = table[var];
    if (lit2.var() == lit.var()) {
        if (lit2.sign() != lit.sign()) {
            #ifdef VERBOSE_DEBUG
            cout << "Inverted cycle in var-replacement -> UNSAT" << endl;
            #endif
            S->ok = false;
        }
        return true;
    }
    
    lit2 = table[lit.var()];
    if (lit2.var() == var) {
        if (lit2.sign() != lit.sign()) {
            #ifdef VERBOSE_DEBUG
            cout << "Inverted cycle in var-replacement -> UNSAT" << endl;
            #endif
            S->ok = false;
        }
        return true;
    }
    
    return false;
}

void VarReplacer::setAllThatPointsHereTo(const Var var, const Lit lit)
{
    map<Var, vector<Var> >::iterator it = reverseTable.find(var);
    if (it == reverseTable.end())
        return;
    
    for(vector<Var>::const_iterator it2 = it->second.begin(), end = it->second.end(); it2 != end; it2++) {
        assert(table[*it2].var() == var);
        table[*it2] = lit ^ table[*it2].sign();
        if (lit.var() != *it2)
            reverseTable[lit.var()].push_back(*it2);
    }
    reverseTable.erase(it);
}

void VarReplacer::newVar()
{
    table.push_back(Lit(table.size(), false));
}

void VarReplacer::newClause()
{
    addedNewClause = true;
}

