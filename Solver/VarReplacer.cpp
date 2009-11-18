#include "VarReplacer.h"

#include "Solver.h"
#include "Conglomerate.h"

//#define VERBOSE_DEBUG

#ifdef VERBOSE_DEBUG
#include <iostream>
using std::cout;
using std::endl;
#endif

VarReplacer::VarReplacer(Solver *_S) :
    replacedLits(0)
    , replacedVars(0)
    , S(_S)
{
}

void VarReplacer::performReplace()
{
    #ifdef VERBOSE_DEBUG
    cout << "Replacer started." << endl;
    uint i = 0;
    for (vector<Lit>::const_iterator it = table.begin(); it != table.end(); it++, i++) {
        if (it->var() == i) continue;
        cout << "Replacing var " << i+1 << " with Lit " << (it->sign() ? "-" : "") <<  it->var()+1 << endl;
    }
    #endif
    
    if (replacedVars == 0) return;
    
    replace_set(S->clauses);
    replace_set(S->learnts);
    
    replace_set(S->xorclauses, true);
    replace_set(S->conglomerate->getCalcAtFinish(), false);
    
    printf("|  Replacing   %8d vars, replaced %8d lits                          |\n", replacedVars, replacedLits);
    
    replacedVars = 0;
    replacedLits = 0;
    
    if (S->ok)
        S->ok = (S->propagate() == NULL);
    
    S->order_heap.filter(Solver::VarFilter(*S));
}

void VarReplacer::replace_set(vec<XorClause*>& cs, const bool need_reattach)
{
    XorClause **a = cs.getData();
    XorClause **r = a;
    for (XorClause **end = a + cs.size(); r != end;) {
        XorClause& c = **r;
        
        bool needReattach = false;
        for (Lit *l = &c[0], *lend = l + c.size(); l != lend; l++) {
            Lit newlit = table[l->var()];
            if (newlit.var() != l->var()) {
                if (need_reattach && !needReattach)
                    S->detachClause(c);
                needReattach = true;
                *l = Lit(newlit.var(), false);
                c.invert(newlit.sign());
                replacedLits++;
            }
        }
        
        if (need_reattach && needReattach) {
            std::sort(c.getData(), c.getData() + c.size());
            Lit p;
            int i, j;
            for (i = j = 0, p = lit_Undef; i < c.size(); i++) {
                c[i] = c[i].unsign();
                if (c[i] == p) {
                    //added, but easily removed
                    j--;
                    p = lit_Undef;
                    if (!S->assigns[c[i].var()].isUndef())
                        c.invert(S->assigns[c[i].var()].getBool());
                } else if (S->value(c[i]) == l_Undef) //just add
                    c[j++] = p = c[i];
                else c.invert(S->value(c[i]) == l_True); //modify xor_clause_inverted instead of adding
            }
            c.shrink(i - j);
            
            switch (c.size()) {
            case 0: {
                if (!c.xor_clause_inverted())
                    S->ok = false;
                free(&c);
                r++;
                break;
            }
            case 1: {
                S->uncheckedEnqueue(Lit(c[0].var(), !c.xor_clause_inverted()));
                free(&c);
                r++;
                break;
            }
            default: {
                S->attachClause(c);
                *a++ = *r++;
                break;
            }
            }
        } else {
            *a++ = *r++;
        }
    }
    cs.shrink(r-a);
}

void VarReplacer::replace_set(vec<Clause*>& cs)
{
    Clause **a = cs.getData();
    Clause **r = a;
    for (Clause **end = a + cs.size(); r != end; ) {
        Clause& c = **r;
        bool needReattach = false;
        for (Lit *l = c.getData(), *end = l + c.size();  l != end; l++) {
            Lit newlit = table[l->var()];
            if (newlit.var() != l->var()) {
                if (!needReattach) S->detachClause(c);
                needReattach = true;
                *l = Lit(newlit.var(), newlit.sign()^l->sign());
                replacedLits++;
            }
        }
        
        bool skip = false;
        if (needReattach) {
            std::sort(c.getData(), c.getData() + c.size());
            Lit p;
            int i, j;
            for (i = j = 0, p = lit_Undef; i < c.size(); i++) {
                if (S->value(c[i]) == l_True || c[i] == ~p) {
                    skip = true;
                    break;
                }
                else if (S->value(c[i]) != l_False && c[i] != p)
                    c[j++] = p = c[i];
            }
            c.shrink(i - j);
            
            if (skip) {
                free(&c);
                r++;
                continue;
            }
            
            switch(c.size()) {
            case 1 : {
                S->uncheckedEnqueue(c[0]);
                free(&c);
                r++;
                break;
            }
            default: {
                S->attachClause(c);
                *a++ = *r++;
                break;
            }
            }
        } else {
            *a++ = *r++;
        }
    }
    cs.shrink(r-a);
}

const uint VarReplacer::getNumReplacedLits() const
{
    return replacedLits;
}

const uint VarReplacer::getNumReplacedVars() const
{
    return replacedVars;
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
        
        assert(S->assigns[i] == l_Undef);
        assert(S->assigns[it->var()] != l_Undef);
        
        bool val = (S->assigns[it->var()] == l_False);
        S->uncheckedEnqueue(Lit(i, val ^ it->sign()));
    }
}

void VarReplacer::replace(Var var, Lit lit)
{
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
        if (lit2.var() != var) {
            setAllThatPointsHereTo(lit1.var(), Lit(lit.var(), lit1.sign()));
            table[lit1.var()] = Lit(lit.var(), lit1.sign());
            reverseTable[lit.var()].push_back(lit1.var());
            S->setDecisionVar(lit1.var(), false);
            
            setAllThatPointsHereTo(lit2.var(), lit ^ lit2.sign());
            table[lit2.var()] = lit ^ lit2.sign();
            reverseTable[lit.var()].push_back(lit2.var());
            S->setDecisionVar(lit2.var(), false);
            
            table[lit.var()] = Lit(lit.var(), false);
            S->setDecisionVar(lit.var(), true);
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
    S->setDecisionVar(var, false);
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
