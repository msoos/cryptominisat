#include "VarReplacer.h"

#include "Solver.h"

VarReplacer::VarReplacer(Solver *_S) :
    replaced(0)
    , S(_S)
{
}

void VarReplacer::replace(const map<Var, Lit>& toReplace)
{
    if (toReplace.size() == 0) return;
    
    replace_set(toReplace, S->clauses);
    replace_set(toReplace, S->learnts);
    
    replace_set(toReplace, S->xorclauses);
    
    printf("|  Replacing       %8d vars, replaced: %8d                         |\n", toReplace.size(), replaced);
}

void VarReplacer::replace_set(const map<Var, Lit>& toReplace, vec<XorClause*>& set)
{
    XorClause **a = set.getData();
    XorClause **r = a;
    for (XorClause **end = a + set.size(); r != end;) {
        XorClause& c = **r;
        bool needReattach = false;
        for (Lit *l = &c[0], *lend = l + c.size(); l != lend; l++) {
            const map<Var, Lit>::const_iterator it = toReplace.find(l->var());
            if (it != toReplace.end()) {
                if (!needReattach)
                    S->detachClause(c);
                needReattach = true;
                *l = Lit(it->second.var(), false);
                c.invert(it->second.sign());
                replaced++;
            }
        }
        
        if (needReattach) {
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
    set.shrink(r-a);
}

void VarReplacer::replace_set(const map<Var, Lit>& toReplace, vec<Clause*>& cs)
{
    Clause **a = cs.getData();
    Clause **r = a;
    for (Clause **end = a + cs.size(); r != end; ) {
        Clause& c = **r;
        bool needReattach = false;
        for (Lit *l = c.getData(), *end = l + c.size();  l != end; l++) {
            const map<Var, Lit>::const_iterator it = toReplace.find(l->var());
            if (it != toReplace.end()) {
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

uint VarReplacer::getReplaced() const
{
    return replaced;
}
