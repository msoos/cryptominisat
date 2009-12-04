#include "Conglomerate.h"
#include "Solver.h"
#include "VarReplacer.h"

#include <utility>
#include <algorithm>
using std::make_pair;

//#define VERBOSE_DEBUG

#ifdef VERBOSE_DEBUG
#include <iostream>
using std::cout;
using std::endl;
#endif

Conglomerate::Conglomerate(Solver *_S) :
    S(_S)
{}

Conglomerate::~Conglomerate()
{
    for(uint i = 0; i < calcAtFinish.size(); i++)
        free(calcAtFinish[i]);
}

const vec<XorClause*>& Conglomerate::getCalcAtFinish() const
{
    return calcAtFinish;
}

vec<XorClause*>& Conglomerate::getCalcAtFinish()
{
    return calcAtFinish;
}

void Conglomerate::fillVarToXor()
{
    blocked.clear();
    varToXor.clear();
    
    blocked.resize(S->nVars(), false);
    for (Clause *const*it = S->clauses.getData(), *const*end = it + S->clauses.size(); it != end; it++) {
        const Clause& c = **it;
        for (const Lit* a = &c[0], *end = a + c.size(); a != end; a++) {
            blocked[a->var()] = true;
        }
    }
    
    for (Lit* it = &(S->trail[0]), *end = it + S->trail.size(); it != end; it++)
        blocked[it->var()] = true;
    
    const vec<Clause*>& tmp = S->toReplace->getToRemove();
    for (Clause *const*it = tmp.getData(), *const*end = it + tmp.size(); it != end; it++) {
        const Clause& c = **it;
        for (const Lit* a = &c[0], *end = a + c.size(); a != end; a++) {
            blocked[a->var()] = true;
        }
    }
    
    uint i = 0;
    for (XorClause* const* it = S->xorclauses.getData(), *const*end = it + S->xorclauses.size(); it != end; it++, i++) {
        const XorClause& c = **it;
        for (const Lit * a = &c[0], *end = a + c.size(); a != end; a++) {
            if (!blocked[a->var()])
                varToXor[a->var()].push_back(make_pair(*it, i));
        }
    }
}

void Conglomerate::process_clause(XorClause& x, const uint num, uint var, vec<Lit>& vars) {
    for (const Lit* a = &x[0], *end = a + x.size(); a != end; a++) {
        if (a->var() != var) {
            vars.push(*a);
            varToXorMap::iterator finder = varToXor.find(a->var());
            if (finder != varToXor.end()) {
                vector<pair<XorClause*, uint> >::iterator it =
                    std::find(finder->second.begin(), finder->second.end(), make_pair(&x, num));
                finder->second.erase(it);
            }
        }
    }
}

uint Conglomerate::conglomerateXors()
{
    if (S->xorclauses.size() == 0)
        return 0;
    toRemove.clear();
    toRemove.resize(S->xorclauses.size(), false);
    
    #ifdef VERBOSE_DEBUG
    cout << "Finding conglomerate xors started" << endl;
    #endif
    
    S->removeSatisfied(S->xorclauses);
    S->cleanClauses(S->xorclauses);
    
    fillVarToXor();
    
    uint found = 0;
    while(varToXor.begin() != varToXor.end()) {
        varToXorMap::iterator it = varToXor.begin();
        const vector<pair<XorClause*, uint> >& c = it->second;
        const uint& var = it->first;
        
        //We blocked the var during dealWithNewClause (it was in a 2-long xor-clause)
        if (blocked[var]) {
            varToXor.erase(it);
            continue;
        }
        
        S->setDecisionVar(var, false);
        
        if (c.size() == 0) {
            varToXor.erase(it);
            continue;
        }
        
        #ifdef VERBOSE_DEBUG
        cout << "--- New conglomerate set ---" << endl;
        #endif
        
        XorClause& x = *(c[0].first);
        bool first_inverted = !x.xor_clause_inverted();
        vec<Lit> first_vars;
        process_clause(x, c[0].second, var, first_vars);
        
        #ifdef VERBOSE_DEBUG
        cout << "- Removing: ";
        x.plain_print();
        cout << "Adding var " << var+1 << " to calcAtFinish" << endl;
        #endif
        
        assert(!toRemove[c[0].second]);
        toRemove[c[0].second] = true;
        S->detachClause(x);
        calcAtFinish.push(&x);
        found++;
        
        for (uint i = 1; i < c.size(); i++) {
            vec<Lit> ps(first_vars.size());
            memcpy(ps.getData(), first_vars.getData(), sizeof(Lit)*first_vars.size());
            XorClause& x = *c[i].first;
            process_clause(x, c[i].second, var, ps);
            
            #ifdef VERBOSE_DEBUG
            cout << "- Removing: ";
            x.plain_print();
            #endif
            
            const uint old_group = x.group;
            bool inverted = first_inverted ^ x.xor_clause_inverted();
            assert(!toRemove[c[i].second]);
            toRemove[c[i].second] = true;
            S->removeClause(x);
            found++;
            clearDouble(ps);
            
            if (!dealWithNewClause(ps, inverted, old_group)) {
                clearToRemove();
                S->ok = false;
                return found;
            }
        }
        
        varToXor.erase(it);
    }
    
    clearToRemove();
    
    if (S->ok != false)
        S->ok = (S->propagate() == NULL);
    
    return found;
}

bool Conglomerate::dealWithNewClause(vec<Lit>& ps, const bool inverted, const uint old_group)
{
    switch(ps.size()) {
        case 0: {
            #ifdef VERBOSE_DEBUG
            cout << "--> xor is 0-long" << endl;
            #endif
            
            if  (!inverted)
                return false;
            break;
        }
        case 1: {
            #ifdef VERBOSE_DEBUG
            cout << "--> xor is 1-long, attempting to set variable " << ps[0].var()+1 << endl;
            #endif
            
            if (S->assigns[ps[0].var()] == l_Undef) {
                assert(S->decisionLevel() == 0);
                S->uncheckedEnqueue(Lit(ps[0].var(), inverted));
                ps[0] = Lit(ps[0].var(), inverted);
                Clause* newC = Clause_new(ps, old_group);
                S->unitary_learnts.push(newC);
            } else if (S->assigns[ps[0].var()] != boolToLBool(!inverted)) {
                #ifdef VERBOSE_DEBUG
                cout << "Conflict. Aborting.";
                #endif
                return false;
            }
            break;
        }
        
        case 2: {
            #ifdef VERBOSE_DEBUG
            cout << "--> xor is 2-long, must later replace variable, adding var " << ps[0].var() + 1 << " to calcAtFinish:" << endl;
            XorClause* newX = XorClause_new(ps, inverted, old_group);
            newX->plain_print();
            free(newX);
            #endif
            
            S->toReplace->replace(ps, inverted, old_group);
            blocked[ps[0].var()] = true;
            blocked[ps[1].var()] = true;
            break;
        }
        
        default: {
            XorClause* newX = XorClause_new(ps, inverted, old_group);
            
            #ifdef VERBOSE_DEBUG
            cout << "- Adding: ";
            newX->plain_print();
            #endif
            
            S->xorclauses.push(newX);
            toRemove.push_back(false);
            S->attachClause(*newX);
            for (const Lit * a = &((*newX)[0]), *end = a + newX->size(); a != end; a++) {
                if (!blocked[a->var()])
                    varToXor[a->var()].push_back(make_pair(newX, toRemove.size()-1));
            }
            break;
        }
    }
    
    return true;
}

void Conglomerate::clearDouble(vec<Lit>& ps) const
{
    std::sort(ps.getData(), ps.getData() + ps.size());
    Lit p;
    uint i, j;
    for (i = j = 0, p = lit_Undef; i < ps.size(); i++) {
        if (ps[i] == p) {
            //added, but easily removed
            j--;
            p = lit_Undef;
        } else //just add
            ps[j++] = p = ps[i];
    }
    ps.shrink(i - j);
}

void Conglomerate::clearToRemove()
{
    XorClause **a = S->xorclauses.getData();
    XorClause **r = a;
    XorClause **end = a + S->xorclauses.size();
    for (uint i = 0; r != end; i++) {
        if (!toRemove[i])
            *a++ = *r++;
        else
            r++;
    }
    S->xorclauses.shrink(r-a);
}

void Conglomerate::doCalcAtFinish()
{
    #ifdef VERBOSE_DEBUG
    cout << "Executing doCalcAtFinish" << endl;
    #endif
    
    vector<Var> toAssign;
    for (XorClause** it = calcAtFinish.getData() + calcAtFinish.size()-1; it != calcAtFinish.getData()-1; it--) {
        toAssign.clear();
        XorClause& c = **it;
        assert(c.size() > 2);
        
        #ifdef VERBOSE_DEBUG
        cout << "doCalcFinish for xor-clause:";
        S->printClause(c); cout << endl;
        #endif
        
        bool final = c.xor_clause_inverted();
        for (int k = 0, size = c.size(); k < size; k++ ) {
            const lbool& val = S->assigns[c[k].var()];
            if (val == l_Undef)
                toAssign.push_back(c[k].var());
            else
                final ^= val.getBool();
        }
        #ifdef VERBOSE_DEBUG
        if (toAssign.size() == 0) {
            cout << "ERROR: toAssign.size() == 0 !!" << endl;
            for (int k = 0, size = c.size(); k < size; k++ ) {
                cout << "Var: " << c[k].var() + 1 << " Level: " << S->level[c[k].var()] << endl;
            }
        }
        if (toAssign.size() > 1) {
            cout << "Double assign!" << endl;
        }
        #endif
        assert(toAssign.size() > 0);
        
        for (uint i = 1; i < toAssign.size(); i++) {
            S->uncheckedEnqueue(Lit(toAssign[i], false), &c);
        }
        S->uncheckedEnqueue(Lit(toAssign[0], final), &c);
    }
}

void Conglomerate::addRemovedClauses()
{
    #ifdef VERBOSE_DEBUG
    cout << "Executing addRemovedClauses" << endl;
    #endif
    
    char tmp[100];
    tmp[0] = '\0';
    vec<Lit> ps;
    for(uint i = 0; i < calcAtFinish.size(); i++)
    {
        XorClause& c = *calcAtFinish[i];
        #ifdef VERBOSE_DEBUG
        cout << "readding already removed (conglomerated) clause: ";
        c.plain_print();
        #endif
        
        ps.clear();
        for(uint i2 = 0; i2 != c.size() ; i2++) {
            ps.push(c[i2]);
            S->setDecisionVar(c[i2].var(), true);
        }
        S->addXorClause(ps, c.xor_clause_inverted(), c.group, tmp);
        free(&c);
    }
    calcAtFinish.clear();
}

