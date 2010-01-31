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
#include <iostream>
#include <iomanip>

#include "Conglomerate.h"
#include "ClauseCleaner.h"
#include "time_mem.h"

//#define VERBOSE_DEBUG
//#define DEBUG_REPLACER

#ifdef VERBOSE_DEBUG
#include <iostream>
using std::cout;
using std::endl;
#endif

VarReplacer::VarReplacer(Solver& _solver) :
    replacedLits(0)
    , lastReplacedLits(0)
    , replacedVars(0)
    , lastReplacedVars(0)
    , addedNewClause(false)
    , solver(_solver)
{
}

VarReplacer::~VarReplacer()
{
    for (uint i = 0; i != clauses.size(); i++)
        //binaryClausePool.free(clauses[i]);
        free(clauses[i]);
}

const lbool VarReplacer::performReplaceInternal()
{
    #ifdef VERBOSE_DEBUG
    cout << "Replacer started." << endl;
    #endif
    double time = cpuTime();
    
    solver.clauseCleaner->cleanClauses(solver.clauses, ClauseCleaner::clauses);
    solver.clauseCleaner->cleanClauses(solver.learnts, ClauseCleaner::learnts);
    solver.clauseCleaner->cleanClauses(solver.xorclauses, ClauseCleaner::xorclauses);
    solver.clauseCleaner->removeSatisfied(solver.binaryClauses, ClauseCleaner::binaryClauses);
    
    #ifdef VERBOSE_DEBUG
    {
        uint i = 0;
        for (vector<Lit>::const_iterator it = table.begin(); it != table.end(); it++, i++) {
            if (it->var() == i) continue;
            cout << "Replacing var " << i+1 << " with Lit " << (it->sign() ? "-" : "") <<  it->var()+1 << endl;
        }
    }
    #endif
    
    Var var = 0;
    const vector<bool>& removedVars = solver.conglomerate->getRemovedVars();
    for (vector<Lit>::const_iterator it = table.begin(); it != table.end(); it++, var++) {
        if (it->var() == var || removedVars[it->var()]) continue;
        #ifdef VERBOSE_DEBUG
        cout << "Setting var " << i+1 << " to a non-decision var" << endl;
        #endif
        bool wasDecisionVar = solver.decision_var[var];
        solver.setDecisionVar(var, false);
        solver.setDecisionVar(it->var(), true);
        
        double& activity1 = solver.activity[var];
        double& activity2 = solver.activity[it->var()];
        if (wasDecisionVar && activity1 > activity2) {
            activity2 = activity1;
            solver.order_heap.update(it->var());
            solver.polarity[it->var()] = solver.polarity[var]^it->sign();
        }
        
        activity1 = 0.0;
        solver.order_heap.update(var);
    }
    assert(solver.order_heap.heapProperty());
    
    replace_set(solver.clauses);
    replace_set(solver.learnts);
    replace_set(solver.binaryClauses);
    
    replace_set(solver.xorclauses, true);
    replace_set(solver.conglomerate->getCalcAtFinish(), false);
    
    for (uint i = 0; i != clauses.size(); i++)
        solver.removeClause(*clauses[i]);
    clauses.clear();
    
    if (solver.verbosity >=1) {
        std::cout << "c |  Replacing   " << std::setw(8) << replacedVars-lastReplacedVars << " vars"
        << "     Replaced " <<  std::setw(8) << replacedLits-lastReplacedLits << " lits"
        << "     Time: " << std::setw(8) << std::fixed << std::setprecision(2) << cpuTime()-time << " s "
        << std::setw(12) <<  " |" << std::endl;
    }
    
    addedNewClause = false;
    lastReplacedVars = replacedVars;
    lastReplacedLits = replacedLits;
    
    if (!solver.ok)
        return l_False;
    
    solver.ok = (solver.propagate() == NULL);
    if (!solver.ok)
        return l_False;
    
    solver.order_heap.filter(Solver::VarFilter(solver));
    
    return l_Undef;
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
            solver.freeLater.push(&c);
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
            if (!solver.assigns[c[i].var()].isUndef())
                c.invert(solver.assigns[c[i].var()].getBool());
            solver.clauses_literals -= 2;
        } else if (solver.assigns[c[i].var()].isUndef()) //just add
            c[j++] = p = c[i];
        else c.invert(solver.assigns[c[i].var()].getBool()); //modify xor_clause_inverted instead of adding
    }
    c.shrink(i - j);
    
    #ifdef VERBOSE_DEBUG
    cout << "xor-clause after replacing: ";
    c.plainPrint();
    #endif
    
    switch (c.size()) {
    case 0:
        solver.detachModifiedClause(origVar1, origVar2, origSize, &c);
        if (!c.xor_clause_inverted())
            solver.ok = false;
        return true;
    case 1:
        solver.detachModifiedClause(origVar1, origVar2, origSize, &c);
        solver.uncheckedEnqueue(c[0] ^ c.xor_clause_inverted());
        return true;
    case 2: {
        solver.detachModifiedClause(origVar1, origVar2, origSize, &c);
        vec<Lit> ps(2);
        ps[0] = c[0];
        ps[1] = c[1];
        addBinaryXorClause(ps, c.xor_clause_inverted(), c.getGroup(), true);
        return true;
    }
    default:
        if (origVar1 != c[0].var() || origVar2 != c[1].var()) {
            solver.detachModifiedClause(origVar1, origVar2, origSize, &c);
            solver.attachClause(c);
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
        if (solver.value(c[i]) == l_True || c[i] == ~p) {
            satisfied = true;
            break;
        }
        else if (solver.value(c[i]) != l_False && c[i] != p)
            c[j++] = p = c[i];
        else
            solver.clauses_literals--;
    }
    c.shrink(i - j);
    
    if (satisfied) {
        solver.detachModifiedClause(origLit1, origLit2, origSize, &c);
        return true;
    }
    
    switch(c.size()) {
    case 0:
        solver.detachModifiedClause(origLit1, origLit2, origSize, &c);
        solver.ok = false;
        return true;
    case 1 :
        solver.detachModifiedClause(origLit1, origLit2, origSize, &c);
        solver.uncheckedEnqueue(c[0]);
        return true;
    case 2:
        solver.detachModifiedClause(origLit1, origLit2, origSize, &c);
        solver.attachClause(c);
        return false;
    default:
        if (origLit1 != c[0] || origLit2 != c[1]) {
            solver.detachModifiedClause(origLit1, origLit2, origSize, &c);
            solver.attachClause(c);
        }
        return false;
    }
    
    assert(false);
    return false;
}

const vector<Var> VarReplacer::getReplacingVars() const
{
    vector<Var> replacingVars;
    
    for(map<Var, vector<Var> >::const_iterator it = reverseTable.begin(), end = reverseTable.end(); it != end; it++) {
        replacingVars.push_back(it->first);
    }
    
    return replacingVars;
}

void VarReplacer::extendModel() const
{
    uint i = 0;
    for (vector<Lit>::const_iterator it = table.begin(); it != table.end(); it++, i++) {
        if (it->var() == i) continue;
        
        #ifdef VERBOSE_DEBUG
        cout << "Extending model: var "; solver.printLit(Lit(i, false));
        cout << " to "; solver.printLit(*it);
        cout << endl;
        #endif
        
        assert(solver.assigns[it->var()] != l_Undef);
        if (solver.assigns[i] == l_Undef) {
            bool val = (solver.assigns[it->var()] == l_False);
            solver.uncheckedEnqueue(Lit(i, val ^ it->sign()));
        } else {
            assert(solver.assigns[i].getBool() == (solver.assigns[it->var()].getBool() ^ it->sign()));
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
    assert(solver.assigns[ps[0].var()].isUndef());
    assert(solver.assigns[ps[1].var()].isUndef());
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
                solver.ok = false;
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
        solver.binaryClauses.push(c);
        solver.becameBinary++;
    } else
        clauses.push(c);
    solver.attachClause(*c);
    
    ps[0] ^= true;
    ps[1] ^= true;
    c = Clause_new(ps, group, false);
    if (internal) {
        solver.binaryClauses.push(c);
        solver.becameBinary++;
    } else
        clauses.push(c);
    solver.attachClause(*c);
}

bool VarReplacer::alreadyIn(const Var var, const Lit lit)
{
    Lit lit2 = table[var];
    if (lit2.var() == lit.var()) {
        if (lit2.sign() != lit.sign()) {
            #ifdef VERBOSE_DEBUG
            cout << "Inverted cycle in var-replacement -> UNSAT" << endl;
            #endif
            solver.ok = false;
        }
        return true;
    }
    
    lit2 = table[lit.var()];
    if (lit2.var() == var) {
        if (lit2.sign() != lit.sign()) {
            #ifdef VERBOSE_DEBUG
            cout << "Inverted cycle in var-replacement -> UNSAT" << endl;
            #endif
            solver.ok = false;
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

