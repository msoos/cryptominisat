#include "conglomerate.h"
#include "Solver.h"

#include <utility>
#include <algorithm>
using std::make_pair;

//#define VERBOSE_DEBUG

#ifdef VERBOSE_DEBUG
#include <iostream>
using std::cout;
using std::endl;
#endif

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
    
    uint i = 0;
    for (XorClause* const* it = S->xorclauses.getData(), *const*end = it + S->xorclauses.size(); it != end; it++, i++) {
        const XorClause& c = **it;
        for (const Lit * a = &c[0], *end = a + c.size(); a != end; a++) {
            if (!blocked[a->var()])
                varToXor[a->var()].push_back(make_pair(*it, i));
        }
    }
}

void Conglomerate::process_clause(XorClause& x, const uint num, uint var, vector<Lit>& vars) {
    for (const Lit* a = &x[0], *end = a + x.size(); a != end; a++) {
        if (a->var() != var) {
            vars.push_back(*a);
            varToXorMap::iterator finder = varToXor.find(a->var());
            if (finder != varToXor.end()) {
                vector<pair<XorClause*, uint> >::iterator it =
                    std::find(finder->second.begin(), finder->second.end(), make_pair(&x, num));
                finder->second.erase(it);
            }
        }
    }
}

uint Conglomerate::conglomerateXors(Solver* _S)
{
    S = _S;
    toRemove.resize(S->xorclauses.size(), false);
    
    #ifdef VERBOSE_DEBUG
    cout << "Finding conglomerate xors started" << endl;
    #endif
    
    fillVarToXor();
    
    uint found = 0;
    while(varToXor.begin() != varToXor.end()) {
        varToXorMap::iterator it = varToXor.begin();
        const vector<pair<XorClause*, uint> >& c = it->second;
        const uint& var = it->first;
        S->decision_var[var] = false;
        
        if (c.size() == 0) {
            varToXor.erase(it);
            continue;
        }
        
        #ifdef VERBOSE_DEBUG
        cout << "--- New conglomerate set ---" << endl;
        #endif
        
        XorClause& x = *(c[0].first);
        bool first_inverted = !x.xor_clause_inverted();
        vector<Lit> first_vars;
        process_clause(x, c[0].second, var, first_vars);
        
        #ifdef VERBOSE_DEBUG
        cout << "- Removing: ";
        x.plain_print();
        #endif
        
        assert(!toRemove[c[0].second]);
        toRemove[c[0].second] = true;
        S->detachClause(x);
        S->calcAtFinish.push_back(make_pair(&x, var));
        found++;
        
        vector<Lit> ps;
        for (uint i = 1; i < c.size(); i++) {
            ps = first_vars;
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
            S->detachClause(x);
            free(&x);
            found++;
            clearDouble(ps);
            
                }
            }
            ps.resize(ps.size()-(r-a)+1);
            
            XorClause* newX = XorClause_new(ps, inverted, old_group);
            
            #ifdef VERBOSE_DEBUG
            cout << "- Adding: ";
            newX->plain_print();
            #endif
            
            if (ps.size() == 1) {
                #ifdef VERBOSE_DEBUG
                cout << "--> xor is 1-long, attempting to set variable" << endl;
                #endif
                
                if (S->assigns[ps[0].var()] == l_Undef)
                    S->uncheckedEnqueue(Lit(ps[0].var(), inverted));
                else if (S->assigns[ps[0].var()] != boolToLBool(!inverted)) {
                        #ifdef VERBOSE_DEBUG
                        cout << "Conflict. Aborting.";
                        #endif
                        S->ok = false;
                        return found;
                }
            } else {
                S->xorclauses.push(newX);
                toRemove.push_back(false);
                S->attachClause(*newX);
                for (const Lit * a = &((*newX)[0]), *end = a + newX->size(); a != end; a++) {
                    if (!blocked[a->var()])
                        varToXor[a->var()].push_back(make_pair(newX, toRemove.size()-1));
                }
            }
        }
        
        varToXor.erase(it);
    }
    
    clearToRemove();
    S->replace(toReplace);
    
    return found;
}

void Conglomerate::clearDouble(vector<Lit>& ps) const
{
    std::sort(ps.begin(), ps.end());
    Lit p;
    int i, j;
    for (i = j = 0, p = lit_Undef; i < ps.size(); i++) {
        if (ps[i] == p) {
            //added, but easily removed
            j--;
            p = lit_Undef;
        } else //just add
            ps[j++] = p = ps[i];
    }
    ps.resize(ps.size()-(i - j));
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
    
    varToXorMap tmp;
    tmp.swap(varToXor);
    
    return found;
}

void Conglomerate::doCalcAtFinish(Solver* S)
{
    for (vector<pair<XorClause*, Var> >::reverse_iterator it = S->calcAtFinish.rbegin(); it != S->calcAtFinish.rend(); it++) {
        XorClause& c = *it->first;
        bool final = c.xor_clause_inverted();
        for (int k = 0, size = c.size(); k < size; k++ ) {
            const lbool& val = S->assigns[c[k].var()];
            if (c[k].var() != it->second)
                final ^= val.getBool();
        }
        S->assigns[it->second] = final ? l_False : l_True;
        free(it->first);
    }
}

