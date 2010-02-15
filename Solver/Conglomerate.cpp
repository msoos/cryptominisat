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

#include "Conglomerate.h"
#include "VarReplacer.h"
#include "ClauseCleaner.h"

#include <utility>
#include <algorithm>
#include <cstring>
#include "time_mem.h"
#include <iomanip>
using std::make_pair;

//#define VERBOSE_DEBUG

#ifdef VERBOSE_DEBUG
#include <iostream>
using std::cout;
using std::endl;
#endif

Conglomerate::Conglomerate(Solver& _solver) :
    found(0)
    , foundBin(0)
    , foundUnit(0)
    , solver(_solver)
{}

Conglomerate::~Conglomerate()
{
    for(uint i = 0; i < calcAtFinish.size(); i++)
        free(calcAtFinish[i]);
}

void Conglomerate::blockVars()
{
    for (Clause *const*it = solver.clauses.getData(), *const*end = it + solver.clauses.size(); it != end; it++) {
        const Clause& c = **it;
        for (const Lit* a = &c[0], *end = a + c.size(); a != end; a++) {
            blocked[a->var()] = true;
        }
    }
    
    for (Clause *const*it = solver.binaryClauses.getData(), *const*end = it + solver.binaryClauses.size(); it != end; it++) {
        const Clause& c = **it;
        blocked[c[0].var()] = true;
        blocked[c[1].var()] = true;
    }
    
    for (Lit* it = &(solver.trail[0]), *end = it + solver.trail.size(); it != end; it++)
        blocked[it->var()] = true;
    
    const vec<Clause*>& clauses = solver.varReplacer->getClauses();
    for (Clause *const*it = clauses.getData(), *const*end = it + clauses.size(); it != end; it++) {
        const Clause& c = **it;
        for (const Lit* a = &c[0], *end = a + c.size(); a != end; a++) {
            blocked[a->var()] = true;
        }
    }
}

void Conglomerate::fillVarToXor()
{
    varToXor.clear();
    
    uint i = 0;
    for (XorClause* const* it = solver.xorclauses.getData(), *const*end = it + solver.xorclauses.size(); it != end; it++, i++) {
        const XorClause& c = **it;
        for (const Lit * a = &c[0], *end = a + c.size(); a != end; a++) {
            if (!blocked[a->var()])
                varToXor[a->var()].push_back(make_pair(*it, i));
        }
    }
}

void Conglomerate::removeVar(const Var var)
{
    solver.setDecisionVar(var, false);
    solver.activity[var] = 0.0;
    solver.order_heap.update(var);
    removedVars[var] = true;
}

void Conglomerate::processClause(XorClause& x, uint32_t num, Var remove_var)
{
    for (const Lit* a = &x[0], *end = a + x.size(); a != end; a++) {
        Var var = a->var();
        if (var != remove_var) {
            varToXorMap::iterator finder = varToXor.find(var);
            if (finder != varToXor.end()) {
                vector<pair<XorClause*, uint32_t> >::iterator it =
                std::find(finder->second.begin(), finder->second.end(), make_pair(&x, num));
                finder->second.erase(it);
            }
        }
    }
}

const bool Conglomerate::conglomerateXors()
{
    if (solver.xorclauses.size() == 0)
        return 0;
    
    #ifdef VERBOSE_DEBUG
    cout << "Finding conglomerate xors started" << endl;
    #endif
    
    double time = cpuTime();
    
    foundBin = foundUnit = 10;
    while (foundBin != 0 || foundUnit != 0) {
        foundBin = foundUnit = 0;
        solver.clauseCleaner->cleanClauses(solver.xorclauses, ClauseCleaner::xorclauses);
        blocked.clear();
        blocked.resize(solver.nVars());
        fillVarToXor();
        if (conglomerateXorsInternal(true) == false)
            return false;
        if (solver.verbosity >=1) {
            std::cout << "c |  Conglomerating XORs:" << std::setw(8) << std::setprecision(2) << std::fixed << cpuTime()-time
            << " found binary XOR-s: " << std::setw(6) << foundBin
            << " found unitary XOR-s: " << std::setw(6) << foundUnit
            << std::setw(3) << " |" << std::endl;
        }
        if (solver.performReplace && solver.varReplacer->performReplace(true) == l_False)
            return false;
    };
    
    solver.clauseCleaner->cleanClauses(solver.xorclauses, ClauseCleaner::xorclauses);
    toRemove.clear();
    toRemove.resize(solver.xorclauses.size(), false);
    blocked.clear();
    blocked.resize(solver.nVars(), false);
    blockVars();
    fillVarToXor();
    if (conglomerateXorsInternal(true) == false)
        return false;
    clearToRemove();
    
    solver.order_heap.filter(Solver::VarFilter(solver));
    
    if (solver.verbosity >=1) {
        std::cout << "c |  Conglomerating XORs:" << std::setw(8) << std::setprecision(2) << std::fixed << cpuTime()-time
        << " s  (removed " << std::setw(8) << found << " vars)"
        << std::setw(31) << "   |" << std::endl;
    }
    
    return solver.ok;
}

const bool Conglomerate::conglomerateXorsInternal(const bool noblock)
{
    vector<vector<Lit> > newSet;
    while(varToXor.begin() != varToXor.end()) {
        varToXorMap::iterator it = varToXor.begin();
        vector<pair<XorClause*, uint32_t> >& clauseSet = it->second;
        const Var var = it->first;
        
        //We blocked the var during dealWithNewClause (it was in a 2-long xor-clause)
        if (!noblock && blocked[var]) {
            varToXor.erase(it);
            continue;
        }
        
        if (clauseSet.size() == 0) {
            if (!noblock) removeVar(var);
            varToXor.erase(it);
            continue;
        }
        
        std::sort(clauseSet.begin(), clauseSet.end(), ClauseSetSorter());
        
        newSet.clear();
        newSet.resize(clauseSet.size());
        
        XorClause& firstXorClause = *(clauseSet[0].first);
        bool first_inverted = !firstXorClause.xor_clause_inverted();
        newSet[0].resize(firstXorClause.size());
        memcpy(&newSet[0][0], firstXorClause.getData(), sizeof(Lit)*firstXorClause.size());
        
        for (uint i = 1; i < clauseSet.size(); i++) {
            XorClause& thisXorClause = *clauseSet[i].first;
            newSet[i] = newSet[0];
            newSet[i].resize(firstXorClause.size()+thisXorClause.size());
            memcpy(&newSet[i][firstXorClause.size()], thisXorClause.getData(), sizeof(Lit)*thisXorClause.size());
            clearDouble(newSet[i]);
        }
        
        if (noblock) {
            for (uint i = 1; i < newSet.size(); i++) if (newSet[i].size() <= 2) {
                XorClause& thisXorClause = *clauseSet[i].first;
                if (newSet[i].size() == 2)
                    foundBin++;
                else if (newSet.size() == 1)
                    foundUnit++;
                
                if (!dealWithNewClause(newSet[i], first_inverted ^ thisXorClause.xor_clause_inverted(), thisXorClause.getGroup())) {
                    solver.ok = false;
                    goto end;
                }
            }
        }
        
        int diff = 0;
        for (size_t i = 1; i < newSet.size(); i++)
            diff += (int)newSet[i].size()-(int)clauseSet[i].first->size();
        
        if (noblock || (newSet.size() > 2 && diff > 0)) {
            blocked[var] = true;
            varToXor.erase(it);
            continue;
        }
        
        #ifdef VERBOSE_DEBUG
        cout << "--- New conglomerate set ---" << endl;
        cout << "- Removing: ";
        firstXorClause.plainPrint();
        cout << "Adding var " << var+1 << " to calcAtFinish" << endl;
        #endif
        removeVar(var);
        
        assert(!toRemove[clauseSet[0].second]);
        toRemove[clauseSet[0].second] = true;
        processClause(firstXorClause, clauseSet[0].second, var);
        solver.detachClause(firstXorClause);
        calcAtFinish.push(&firstXorClause);
        found++;
        
        for (uint i = 1; i < clauseSet.size(); i++) {
            XorClause& thisXorClause = *clauseSet[i].first;
            #ifdef VERBOSE_DEBUG
            cout << "- Removing: ";
            thisXorClause.plainPrint();
            #endif
            
            const uint old_group = thisXorClause.getGroup();
            bool inverted = first_inverted ^ thisXorClause.xor_clause_inverted();
            assert(!toRemove[clauseSet[i].second]);
            toRemove[clauseSet[i].second] = true;
            processClause(thisXorClause, clauseSet[i].second, var);
            solver.removeClause(thisXorClause);
            
            if (!dealWithNewClause(newSet[i], inverted, old_group)) {
                solver.ok = false;
                goto end;
            }
            assert(newSet.size() == clauseSet.size());
        }
        
        varToXor.erase(it);
    }
    
    solver.ok = (solver.propagate() == NULL);
    
end:
    
    return solver.ok;
}

bool Conglomerate::dealWithNewClause(vector<Lit>& ps, const bool inverted, const uint old_group)
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
            
            if (solver.assigns[ps[0].var()] == l_Undef) {
                assert(solver.decisionLevel() == 0);
                blocked[ps[0].var()] = true;
                solver.uncheckedEnqueue(Lit(ps[0].var(), inverted));
            } else if (solver.assigns[ps[0].var()] != boolToLBool(!inverted)) {
                #ifdef VERBOSE_DEBUG
                cout << "Conflict. Aborting.";
                #endif
                return false;
            } else {
                #ifdef VERBOSE_DEBUG
                cout << "Variable already set to correct value";
                #endif
            }
            break;
        }
        
        case 2: {
            #ifdef VERBOSE_DEBUG
            cout << "--> xor is 2-long, must later replace variable, adding var " << ps[0].var() + 1 << " to calcAtFinish:" << endl;
            XorClause* newX = XorClause_new(ps, inverted, old_group);
            newX->plainPrint();
            free(newX);
            #endif
            
            vec<Lit> tmpPS(2);
            tmpPS[0] = ps[0];
            tmpPS[1] = ps[1];
            solver.varReplacer->replace(tmpPS, inverted, old_group);
            blocked[ps[0].var()] = true;
            blocked[ps[1].var()] = true;
            break;
        }
        
        default: {
            XorClause* newX = XorClause_new(ps, inverted, old_group);
            
            #ifdef VERBOSE_DEBUG
            cout << "- Adding: ";
            newX->plainPrint();
            #endif
            
            solver.xorclauses.push(newX);
            toRemove.push_back(false);
            solver.attachClause(*newX);
            for (const Lit * a = &((*newX)[0]), *end = a + newX->size(); a != end; a++) {
                if (!blocked[a->var()])
                    varToXor[a->var()].push_back(make_pair(newX, (uint32_t)(toRemove.size()-1)));
            }
            break;
        }
    }
    
    return true;
}

void Conglomerate::clearDouble(vector<Lit>& ps) const
{    
    std::sort(ps.begin(), ps.end());
    Lit p;
    uint32_t i, j;
    for (i = j = 0, p = lit_Undef; i != ps.size(); i++) {
        ps[i] = ps[i].unsign();
        if (ps[i] == p) {
            //added, but easily removed
            j--;
            p = lit_Undef;
        } else
            ps[j++] = p = ps[i];
    }
    ps.resize(ps.size() - (i - j));
}

void Conglomerate::clearToRemove()
{
    assert(toRemove.size() == solver.xorclauses.size());
    
    XorClause **a = solver.xorclauses.getData();
    XorClause **r = a;
    XorClause **end = a + solver.xorclauses.size();
    for (uint i = 0; r != end; i++) {
        if (!toRemove[i])
            *a++ = *r++;
        else {
            (**a).mark(1);
            r++;
        }
    }
    solver.xorclauses.shrink(r-a);
    
    clearLearntsFromToRemove();
}

void Conglomerate::clearLearntsFromToRemove()
{
    Clause **a = solver.learnts.getData();
    Clause **r = a;
    Clause **end = a + solver.learnts.size();
    for (; r != end;) {
        const Clause& c = **r;
        bool inside = false;
        if (!solver.locked(c)) {
            for (uint i = 0; i < c.size(); i++) {
                if (removedVars[c[i].var()]) {
                    inside = true;
                    break;
                }
            }
        }
        if (!inside)
            *a++ = *r++;
        else {
            solver.removeClause(**r);
            r++;
        }
    }
    solver.learnts.shrink(r-a);
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
        c.plainPrint();
        #endif
        
        bool final = c.xor_clause_inverted();
        for (int k = 0, size = c.size(); k < size; k++ ) {
            const lbool& val = solver.assigns[c[k].var()];
            if (val == l_Undef)
                toAssign.push_back(c[k].var());
            else
                final ^= val.getBool();
        }
        #ifdef VERBOSE_DEBUG
        if (toAssign.size() == 0) {
            cout << "ERROR: toAssign.size() == 0 !!" << endl;
            for (int k = 0, size = c.size(); k < size; k++ ) {
                cout << "Var: " << c[k].var() + 1 << " Level: " << solver.level[c[k].var()] << endl;
            }
        }
        if (toAssign.size() > 1) {
            cout << "Double assign!" << endl;
            for (uint i = 1; i < toAssign.size(); i++) {
                cout << "-> extra Var " << toAssign[i]+1 << endl;
            }
        }
        #endif
        assert(toAssign.size() > 0);
        
        for (uint i = 1; i < toAssign.size(); i++) {
            solver.uncheckedEnqueue(Lit(toAssign[i], true), &c);
        }
        solver.uncheckedEnqueue(Lit(toAssign[0], final), &c);
        assert(solver.clauseCleaner->satisfied(c));
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
        c.plainPrint();
        #endif
        
        ps.clear();
        for(uint i2 = 0; i2 != c.size() ; i2++) {
            ps.push(Lit(c[i2].var(), false));
        }
        solver.addXorClause(ps, c.xor_clause_inverted(), c.getGroup(), tmp, true);
        free(&c);
    }
    calcAtFinish.clear();
    for (uint i = 0; i < removedVars.size(); i++) {
        if (removedVars[i]) {
            removedVars[i] = false;
            solver.setDecisionVar(i, true);
            #ifdef VERBOSE_DEBUG
            std::cout << "Inserting Var " << i+1 << " back into the order_heap" << std::endl;
            #endif //VERBOSE_DEBUG
        }
    }
}

void Conglomerate::newVar()
{
    removedVars.resize(removedVars.size()+1, false);
}
