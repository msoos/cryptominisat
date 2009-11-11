#include "VarReplacer.h"

#include "Solver.h"
#include "conglomerate.h"

//#define VERBOSE_DEBUG

#ifdef VERBOSE_DEBUG
#include <iostream>
using std::cout;
using std::endl;
#endif

VarReplacer::VarReplacer(Solver *_S) :
    replaced(0)
    , S(_S)
{
}

void VarReplacer::performReplace()
{
    #ifdef VERBOSE_DEBUG
    cout << "Replacer started." << endl;
    for (map<Var, Lit>::const_iterator it = table.begin(); it != table.end(); it++) {
        cout << "Replacing var " << it->first+1 << " with Lit " << (it->second.sign() ? "-" : "") <<  it->second.var()+1 << endl;
    }
    #endif
    
    if (table.size() == 0) return;
    
    replace_set(S->clauses);
    replace_set(S->learnts);
    
    replace_set(S->xorclauses, true);
    replace_set(S->conglomerate->getCalcAtFinish(), false);
    
    printf("|  Replacing       %8d vars, replaced: %8d                         |\n", table.size(), replaced);
    
    if (S->ok)
        S->ok = (S->propagate() == NULL);
}

void VarReplacer::replace_set(vec<XorClause*>& cs, const bool need_reattach)
{
    XorClause **a = cs.getData();
    XorClause **r = a;
    for (XorClause **end = a + cs.size(); r != end;) {
        XorClause& c = **r;
        
        bool needReattach = false;
        for (Lit *l = &c[0], *lend = l + c.size(); l != lend; l++) {
            const map<Var, Lit>::const_iterator it = table.find(l->var());
            if (it != table.end()) {
                if (need_reattach && !needReattach)
                    S->detachClause(c);
                needReattach = true;
                *l = Lit(it->second.var(), false);
                c.invert(it->second.sign());
                replaced++;
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
            const map<Var, Lit>::const_iterator it = table.find(l->var());
            if (it != table.end()) {
                if (!needReattach) S->detachClause(c);
                needReattach = true;
                *l = Lit(it->second.var(), it->second.sign()^l->sign());
                replaced++;
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

uint VarReplacer::getNumReplaced() const
{
    return replaced;
}

void VarReplacer::extendModel() const
{
    for (map<Var, Lit>::const_iterator it = table.begin(); it != table.end(); it++) {
        assert(S->assigns[it->first] == l_Undef);
        assert(S->assigns[it->second.var()] != l_Undef);
        
        bool val = (S->assigns[it->second.var()] == l_True);
        S->uncheckedEnqueue(Lit(it->first, val ^ it->second.sign()));
    }
}

void VarReplacer::replace(const Var var, Lit lit)
{
    S->setDecisionVar(var, false);
    map<Var, Lit>::iterator it = table.find(lit.var());
    if (it != table.end()) {
        lit = Lit(it->second.var(), it->second.sign() ^ lit.sign());
    }
    
    for(map<Var, Lit>::iterator it = table.begin(); it != table.end(); it++) {
        if (it->second.var() == var) {
            it->second = Lit(lit.var(), it->second.sign() ^ lit.sign());
        }
    }
    
    map<Var, Lit>::iterator it2 = table.find(var);
    if (it2 != table.end()) {
        Var var2 = it2->second.var();
        bool sign = it2->second.sign() ^ lit.sign();
        for(map<Var, Lit>::iterator it = table.begin(); it != table.end(); it++) {
            if (it->second.var() == var2) {
                it->second = Lit(lit.var(), sign ^ lit.sign());
            }
        }
    }
    table[var] = lit;
}
