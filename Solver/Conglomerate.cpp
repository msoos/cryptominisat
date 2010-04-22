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
    solver.activity[var] = 0.0;
    solver.order_heap.update(var);
    removedVars[var] = true;
    if (solver.decision_var[var]) {
        madeVarNonDecision.push(var);
        solver.setDecisionVar(var, false);
    }
    found++;
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

const bool Conglomerate::heuleProcessFull()
{
    #ifdef VERBOSE_DEBUG
    cout << "Heule XOR-ing started" << endl;
    #endif
    
    double time = cpuTime();
    found = 0;
    uint oldToReplaceVars = solver.varReplacer->getNewToReplaceVars();
    solver.clauseCleaner->cleanClauses(solver.xorclauses, ClauseCleaner::xorclauses);
    if (solver.ok == false)
        return false;
    
    toRemove.clear();
    toRemove.resize(solver.xorclauses.size(), false);
    blocked.clear();
    blocked.resize(solver.nVars(), false);
    fillVarToXor();
    if (!heuleProcess())
        goto end;
    
    if (solver.verbosity >=1) {
        std::cout << "c |  Heule-processings XORs:" << std::setw(8) << std::setprecision(2) << std::fixed << cpuTime()-time
        << "  Found smaller XOR-s: " << std::setw(6) << found
        << "  New bin anti/eq-s: " << std::setw(3) << solver.varReplacer->getNewToReplaceVars() - oldToReplaceVars
        << std::setw(0) << " |" << std::endl;
    }

end:
    
    clearToRemove();
    
    return solver.ok;
}

const bool Conglomerate::conglomerateXorsFull()
{
    #ifdef VERBOSE_DEBUG
    cout << "Finding conglomerate xors started" << endl;
    #endif
    
    double time = cpuTime();
    found = 0;
    uint32_t origNumClauses = solver.xorclauses.size();
    
    solver.clauseCleaner->cleanClauses(solver.xorclauses, ClauseCleaner::xorclauses);
    if (solver.ok == false)
        return false;
    
    toRemove.clear();
    toRemove.resize(solver.xorclauses.size(), false);
    blocked.clear();
    blocked.resize(solver.nVars(), false);
    blockVars();
    fillVarToXor();
    if (conglomerateXors() == false)
        goto end;
    
end:
    
    clearToRemove();
    assert(origNumClauses >= solver.xorclauses.size());
    if (solver.verbosity >=1) {
        std::cout << "c |  Conglomerating XORs:" << std::setw(8) << std::setprecision(2) << std::fixed << cpuTime()-time << " s   "
        << "   removed " << std::setw(8) << found << " vars"
        << "   removed " << std::setw(8) << origNumClauses-solver.xorclauses.size() << " clauses"
        << std::setw(0) << " |" << std::endl;
    }
    
    clearLearntsFromToRemove();
    solver.order_heap.filter(Solver::VarFilter(solver));
    
    return solver.ok;
}

void Conglomerate::fillNewSet(vector<vector<Lit> >& newSet, vector<pair<XorClause*, uint32_t> >& clauseSet) const
{
    newSet.clear();
    newSet.resize(clauseSet.size());
    
    XorClause& firstXorClause = *(clauseSet[0].first);
    newSet[0].resize(firstXorClause.size());
    memcpy(&newSet[0][0], firstXorClause.getData(), sizeof(Lit)*firstXorClause.size());
    
    for (uint i = 1; i < clauseSet.size(); i++) {
        XorClause& thisXorClause = *clauseSet[i].first;
        newSet[i] = newSet[0];
        newSet[i].resize(firstXorClause.size()+thisXorClause.size());
        memcpy(&newSet[i][firstXorClause.size()], thisXorClause.getData(), sizeof(Lit)*thisXorClause.size());
        clearDouble(newSet[i]);
    }
}

const bool Conglomerate::heuleProcess()
{
    vector<vector<Lit> > newSet;
    while(varToXor.begin() != varToXor.end()) {
        varToXorMap::iterator it = varToXor.begin();
        vector<pair<XorClause*, uint32_t> >& clauseSet = it->second;
        const Var var = it->first;
        
        if (blocked[var] || clauseSet.size() == 1) {
            varToXor.erase(it);
            blocked[var] = true;
            continue;
        }
        blocked[var] = true;
        
        std::sort(clauseSet.begin(), clauseSet.end(), ClauseSetSorter());
        fillNewSet(newSet, clauseSet);
        
        for (uint i = 1; i < newSet.size(); i++) if (newSet[i].size() <= 2) {
            found++;
            XorClause& thisXorClause = *clauseSet[i].first;
            const bool inverted = !clauseSet[0].first->xor_clause_inverted() ^ thisXorClause.xor_clause_inverted();
            const uint old_group = thisXorClause.getGroup();

            #ifdef VERBOSE_DEBUG
            cout << "- XOR1:";
            clauseSet[0].first->plainPrint();
            cout << "- XOR2:";
            thisXorClause.plainPrint();
            #endif
            
            if (!dealWithNewClause(newSet[i], inverted, old_group))
                return false;
            assert(newSet.size() == clauseSet.size());
        }
        
        varToXor.erase(it);
    }
    
    return (solver.ok = (solver.propagate() == NULL));
}

const bool Conglomerate::conglomerateXors()
{
    vector<vector<Lit> > newSet;
    while(varToXor.begin() != varToXor.end()) {
        varToXorMap::iterator it = varToXor.begin();
        vector<pair<XorClause*, uint32_t> >& clauseSet = it->second;
        const Var var = it->first;
        
        //We blocked the var during dealWithNewClause (it was in a 2-long xor-clause)
        if (blocked[var]) {
            varToXor.erase(it);
            continue;
        }
        
        if (clauseSet.size() == 0) {
            removeVar(var);
            varToXor.erase(it);
            continue;
        }
        
        std::sort(clauseSet.begin(), clauseSet.end(), ClauseSetSorter());
        fillNewSet(newSet, clauseSet);
        
        int diff = 0;
        for (size_t i = 1; i < newSet.size(); i++)
            diff += (int)newSet[i].size()-(int)clauseSet[i].first->size();
        
        if (newSet.size() > 2) {
            blocked[var] = true;
            varToXor.erase(it);
            continue;
        }
        
        XorClause& firstXorClause = *(clauseSet[0].first);
        bool first_inverted = !firstXorClause.xor_clause_inverted();
        removeVar(var);
        
        #ifdef VERBOSE_DEBUG
        cout << "--- New conglomerate set ---" << endl;
        cout << "- Removing: ";
        firstXorClause.plainPrint();
        cout << "Adding var " << var+1 << " to calcAtFinish" << endl;
        #endif
        
        assert(!toRemove[clauseSet[0].second]);
        toRemove[clauseSet[0].second] = true;
        processClause(firstXorClause, clauseSet[0].second, var);
        solver.detachClause(firstXorClause);
        calcAtFinish.push(&firstXorClause);
        
        for (uint i = 1; i < clauseSet.size(); i++) {
            XorClause& thisXorClause = *clauseSet[i].first;
            #ifdef VERBOSE_DEBUG
            cout << "- Removing: ";
            thisXorClause.plainPrint();
            #endif
            
            const uint old_group = thisXorClause.getGroup();
            const bool inverted = first_inverted ^ thisXorClause.xor_clause_inverted();
            assert(!toRemove[clauseSet[i].second]);
            toRemove[clauseSet[i].second] = true;
            processClause(thisXorClause, clauseSet[i].second, var);
            solver.removeClause(thisXorClause);
            
            if (!dealWithNewClause(newSet[i], inverted, old_group))
                return false;
            assert(newSet.size() == clauseSet.size());
        }
        
        varToXor.erase(it);
    }
    
    return (solver.ok = (solver.propagate() == NULL));
}

bool Conglomerate::dealWithNewClause(vector<Lit>& ps, const bool inverted, const uint old_group)
{
    switch(ps.size()) {
        case 0: {
            #ifdef VERBOSE_DEBUG
            cout << "--> xor is 0-long" << endl;
            #endif
            
            if  (!inverted) {
                solver.ok = false;
                return false;
            }
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
                solver.ok = false;
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
            cout << "--> xor is 2-long, must later replace variable" << endl;
            XorClause* newX = XorClause_new(ps, inverted, old_group);
            newX->plainPrint();
            free(newX);
            #endif
            
            vec<Lit> tmpPS(2);
            tmpPS[0] = ps[0];
            tmpPS[1] = ps[1];
            if (solver.varReplacer->replace(tmpPS, inverted, old_group) == false)
                return false;
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
            r++;
        }
    }
    solver.xorclauses.shrink(r-a);
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

void Conglomerate::extendModel(Solver& solver2)
{
    #ifdef VERBOSE_DEBUG
    cout << "Executing doCalcAtFinish" << endl;
    #endif
    
    vec<Lit> ps;
    for (int i = (int)(calcAtFinish.size())-1; i >= 0; i--) {
        XorClause& c = *calcAtFinish[i];
        assert(c.size() > 2);
        
        ps.clear();
        for (Lit *l = c.getData(), *end = c.getDataEnd(); l != end; l++) {
            ps.push(l->unsign());
        }
        
        solver2.addXorClause(ps, c.xor_clause_inverted(), c.getGroup());
        assert(solver2.ok);
    }
}

const bool Conglomerate::addRemovedClauses()
{
    #ifdef VERBOSE_DEBUG
    cout << "Executing addRemovedClauses" << endl;
    #endif
    
    for(XorClause **it = calcAtFinish.getData(), **end = calcAtFinish.getDataEnd(); it != end; it++)
    {
        XorClause& c = **it;
        #ifdef VERBOSE_DEBUG
        cout << "readding already removed (conglomerated) clause: ";
        c.plainPrint();
        #endif
        
        for(Lit *l = c.getData(), *end2 = c.getDataEnd(); l != end2 ; l++)
            *l = l->unsign();
        
        FILE* backup_libraryCNFfile = solver.libraryCNFFile;
        solver.libraryCNFFile = NULL;
        if (!solver.addXorClause(c, c.xor_clause_inverted(), c.getGroup())) {
            for (;it != end; it++) free(*it);
            calcAtFinish.clear();
            return false;
        }
        solver.libraryCNFFile = backup_libraryCNFfile;
        free(&c);
    }
    calcAtFinish.clear();
    
    std::fill(removedVars.getData(), removedVars.getDataEnd(), false);
    for (Var *v = madeVarNonDecision.getData(), *end =  madeVarNonDecision.getDataEnd(); v != end; v++) {
        solver.setDecisionVar(*v, true);
    }
    madeVarNonDecision.clear();
    
    return true;
}

