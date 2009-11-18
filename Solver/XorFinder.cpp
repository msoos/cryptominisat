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

#include "xorFinder.h"
#include <algorithm>
#include <utility>
#include <iostream>
#include "Solver.h"
#include "VarReplacer.h"

//#define VERBOSE_DEBUG

#ifdef VERBOSE_DEBUG
#include <iostream>
using std::cout;
using std::endl;
#endif

using std::make_pair;

XorFinder::XorFinder(Solver* _S, vec<Clause*>& _cls, vec<XorClause*>& _xorcls) :
    cls(_cls)
    , xorcls(_xorcls)
    , S(_S)
{
}

uint XorFinder::doNoPart(uint& sumLengths, const uint minSize, const uint maxSize)
{
    toRemove.clear();
    toRemove.resize(cls.size(), false);
    
    vector<bool> toRemove(cls.size(), false);
    uint found = 0;
    table.clear();
    table.reserve(cls.size()/2);
    uint i = 0;
    for (Clause **it = cls.getData(), **end = it + cls.size(); it != end; it++, i++) {
        const uint size = (*it)->size();
        if ( size > maxSize || size < minSize) continue;
        table.push_back(make_pair(*it, i));
    }
    
    found += findXors(sumLengths);
    clearToRemove();
    
    S->toReplace->performReplace();
    if (S->ok == false) return found;
    S->ok = (S->propagate() == NULL);
    
    return found;
}

uint XorFinder::doByPart(uint& sumLengths, const uint minSize, const uint maxSize)
{
    toRemove.clear();
    toRemove.resize(cls.size(), false);
    
    uint sumUsage = 0;
    vector<uint> varUsage(S->nVars(), 0);
    for (Clause **it = cls.getData(), **end = it + cls.size(); it != end; it++) {
        const uint size = (*it)->size();
        if ( size > maxSize || size < minSize) continue;
        
        for (const Lit *l = &(**it)[0], *end = l + size; l != end; l++) {
            varUsage[l->var()]++;
            sumUsage++;
        }
    }
    
    uint found = 0;
    #ifdef VERBOSE_DEBUG
    uint sumNumClauses = 0;
    #endif
    
    const uint limit = 800000;
    uint from = 0;
    uint until = 0;
    while (until < varUsage.size()) {
        uint estimate = 0;
        for (; until < varUsage.size(); until++) {
            estimate += varUsage[until];
            if (estimate >= limit) break;
        }
        #ifdef VERBOSE_DEBUG
        printf("Xor-finding: Vars from: %d, until: %d\n", from, until);
        uint numClauses = 0;
        #endif
        
        table.clear();
        table.reserve(estimate/2);
        uint i = 0;
        for (Clause **it = cls.getData(), **end = it + cls.size(); it != end; it++, i++) {
            if (toRemove[i]) continue;
            const uint size = (*it)->size();
            if ( size > maxSize || size < minSize) continue;
            
            for (Lit *l = &(**it)[0], *end = l + size; l != end; l++) {
                if (l->var() >= from  && l->var() <= until) {
                    table.push_back(make_pair(*it, i));
                    #ifdef VERBOSE_DEBUG
                    numClauses++;
                    #endif
                    break;
                }
            }
        }
        #ifdef VERBOSE_DEBUG
        printf("numClauses in range: %d\n", numClauses);
        sumNumClauses += numClauses;
        #endif
        
        uint lengths;
        found += findXors(lengths);
        sumLengths += lengths;
        #ifdef VERBOSE_DEBUG
        printf("Found in this range: %d\n", found);
        #endif
        
        from = until+1;
    }
    
    clearToRemove();
    
    S->toReplace->performReplace();
    if (S->ok == false) return found;
    S->ok = (S->propagate() == NULL);
    
    #ifdef VERBOSE_DEBUG
    cout << "Overdone work due to partitioning:" << (double)sumNumClauses/(double)cls.size() << "x" << endl;
    #endif
    
    return found;
}

uint XorFinder::findXors(uint& sumLengths)
{
    #ifdef VERBOSE_DEBUG
    cout << "Finding Xors started" << endl;
    #endif
    
    uint foundXors = 0;
    sumLengths = 0;
    std::sort(table.begin(), table.end(), clause_sorter_primary());
    
    ClauseTable::iterator begin = table.begin();
    ClauseTable::iterator end = table.begin();
    vector<Lit> lits;
    bool impair;
    while (getNextXor(begin,  end, impair)) {
        const Clause& c = *(begin->first);
        lits.clear();
        for (const Lit *it = &c[0], *cend = it+c.size() ; it != cend; it++) {
            lits.push_back(Lit(it->var(), false));
        }
        uint old_group = c.group;
        
        #ifdef VERBOSE_DEBUG
        cout << "- Found clauses:" << endl;
        #endif
        
        for (ClauseTable::iterator it = begin; it != end; it++) {
            #ifdef VERBOSE_DEBUG
            it->first->plain_print();
            #endif
            toRemove[it->second] = true;
            S->removeClause(*it->first);
        }
        
        switch(lits.size()) {
        case 2: {
            S->toReplace->replace(lits[0].var(), Lit(lits[1].var(), !impair));
            
            #ifdef VERBOSE_DEBUG
            XorClause* x = XorClause_new(lits, impair, old_group);
            cout << "- Final 2-long xor-clause: ";
            x->plain_print();
            free(x);
            #endif
            break;
        }
        default: {
            XorClause* x = XorClause_new(lits, impair, old_group);
            xorcls.push(x);
            S->attachClause(*x);
            
            #ifdef VERBOSE_DEBUG
            cout << "- Final xor-clause: ";
            x->plain_print();
            #endif
        }
        }
        
        foundXors++;
        sumLengths += lits.size();
    }
    
    return foundXors;
}

void XorFinder::clearToRemove()
{
    Clause **a = cls.getData();
    Clause **r = cls.getData();
    Clause **cend = cls.getData() + cls.size();
    for (uint i = 0; r != cend; i++) {
        if (!toRemove[i])
            *a++ = *r++;
        else
            r++;
    }
    cls.shrink(r-a);
}

bool XorFinder::getNextXor(ClauseTable::iterator& begin, ClauseTable::iterator& end, bool& impair)
{
    ClauseTable::iterator tableEnd = table.end();

    while(begin != tableEnd && end != tableEnd) {
        begin = end;
        end++;
        while(end != tableEnd && clause_vareq(begin->first, end->first))
            end++;
        if (isXor(begin, end, impair))
            return true;
    }
    
    return false;
}

bool XorFinder::clauseEqual(const Clause& c1, const Clause& c2) const
{
    assert(c1.size() == c2.size());
    for (uint i = 0, size = c1.size(); i < size; i++)
        if (c1[i].sign() !=  c2[i].sign()) return false;
    
    return true;
}

bool XorFinder::impairSigns(const Clause& c) const
{
    uint num = 0;
    for (const Lit *it = &c[0], *end = it + c.size(); it != end; it++)
        num += it->sign();
        
    return num % 2;
}

bool XorFinder::isXor(const ClauseTable::iterator& begin, const ClauseTable::iterator& end, bool& impair)
{
    uint size = &(*begin) - &(*end);
    assert(size > 0);
    const uint requiredSize = 1 << (begin->first->size()-1);
    
    if (size < requiredSize)
        return false;
    
    std::sort(begin, end, clause_sorter_secondary());
    
    uint numPair = 0;
    uint numImpair = 0;
    countImpairs(begin, end, numImpair, numPair);
    
    if (numImpair == requiredSize) {
        impair = true;
        
        return true;
    }
    
    if (numPair == requiredSize) {
        impair = false;
        
        return true;
    }
    
    return false;
}

void XorFinder::countImpairs(const ClauseTable::iterator& begin, const ClauseTable::iterator& end, uint& numImpair, uint& numPair) const
{
    numImpair = 0;
    numPair = 0;
    
    ClauseTable::const_iterator it = begin;
    ClauseTable::const_iterator it2 = begin;
    it2++;
    
    bool impair = impairSigns(*it->first);
    numImpair += impair;
    numPair += !impair;
    
    for (; it2 != end;) {
        if (!clauseEqual(*it->first, *it2->first)) {
            bool impair = impairSigns(*it2->first);
            numImpair += impair;
            numPair += !impair;
        }
        it++;
        it2++;
    }
}

