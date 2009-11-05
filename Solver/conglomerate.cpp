#include "conglomerate.h"
#include "Solver.h"

#include <utility>
#include <algorithm>
using std::make_pair;

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
    
    #ifdef VERBOSE_DEBUG
    cout << "Finding conglomerate xors started" << endl;
    #endif
    
    fillVarToXor();
    
    uint found = 0;
    vector<bool> toRemove(S->xorclauses.size(), false);
    while(varToXor.begin() != varToXor.end()) {
        varToXorMap::iterator it = varToXor.begin();
        const vector<pair<XorClause*, uint> >& c = it->second;
        const uint& var = it->first;
        
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
        calcAtFinish.push_back(make_pair(&x, var));
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
            sort(ps.begin(), ps.end());
            Lit* a = &ps[0], *r = a;
            r++;
            for (Lit *end = a + ps.size(); r != end;) {
                if (a->var() != r->var()) {
                    a++;
                    *a = *r++;
                } else {
                    a--;
                    r++;
                }
            }
            ps.resize(ps.size()-(r-a)+1);
            
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
        }
        
        varToXor.erase(it);
    }
    
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
    
    return found;
}

void Conglomerate::doCalcAtFinish()
{
    for (vector<pair<XorClause*, Var> >::iterator it = calcAtFinish.begin(); it != calcAtFinish.end(); it++) {
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

