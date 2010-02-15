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

#include "XorFinder.h"

#include <algorithm>
#include <utility>
#include <iostream>
#include "Solver.h"
#include "VarReplacer.h"
#include "ClauseCleaner.h"
#include "time_mem.h"

//#define VERBOSE_DEBUG

#ifdef VERBOSE_DEBUG
#include <iostream>
using std::cout;
using std::endl;
#endif

using std::make_pair;

XorFinder::XorFinder(Solver* _s, vec<Clause*>& _cls, ClauseCleaner::ClauseSetType _type) :
    cls(_cls)
    , type(_type)
    , S(_s)
{
}

const bool XorFinder::doNoPart(const uint minSize, const uint maxSize)
{
    uint sumLengths = 0;
    double time = cpuTime();
    foundXors = 0;
    S->clauseCleaner->cleanClauses(cls, type);
    if (S->ok == false)
        return false;
    
    toRemove.clear();
    toRemove.resize(cls.size(), false);
    
    table.clear();
    table.reserve(cls.size()/2);
    uint i = 0;
    for (Clause **it = cls.getData(), **end = it + cls.size(); it != end; it++, i++) {
        const uint size = (*it)->size();
        if ( size > maxSize || size < minSize) continue;
        table.push_back(make_pair(*it, i));
    }
    
    if (findXors(sumLengths) == false)
        return false;
    
    if (S->verbosity >=1) {
        if (minSize == maxSize && minSize == 2)
            printf("c |  Finding binary XORs:        %5.2lf s (found: %7d, avg size: %3.1lf)                  |\n", cpuTime()-time, foundXors, (double)sumLengths/(double)foundXors);
        else
            printf("c |  Finding non-binary XORs:    %5.2lf s (found: %7d, avg size: %3.1lf)                  |\n", cpuTime()-time, foundXors, (double)sumLengths/(double)foundXors);
    }
    
    if (foundXors > 0)
        clearToRemove();
    
    if (type == ClauseCleaner::binaryClauses) {
        for (uint i = 0, j = 0, size = table.size(); i != size; i++) {
            if (!toRemove[table[i].second])
                S->binaryClauses[j++] = table[i].first;
        }
    }
    
    return S->ok = (S->propagate() == NULL);
}

const bool XorFinder::findXors(uint& sumLengths)
{
    #ifdef VERBOSE_DEBUG
    cout << "Finding Xors started" << endl;
    #endif
    
    sumLengths = 0;
    std::sort(table.begin(), table.end(), clause_sorter_primary());
    
    ClauseTable::iterator begin = table.begin();
    ClauseTable::iterator end = table.begin();
    vec<Lit> lits;
    bool impair;
    while (getNextXor(begin,  end, impair)) {
        const Clause& c = *(begin->first);
        lits.clear();
        for (const Lit *it = &c[0], *cend = it+c.size() ; it != cend; it++) {
            lits.push(Lit(it->var(), false));
        }
        uint old_group = c.getGroup();
        
        #ifdef VERBOSE_DEBUG
        cout << "- Found clauses:" << endl;
        #endif
        
        for (ClauseTable::iterator it = begin; it != end; it++)
            if (impairSigns(*it->first) == impair){
            #ifdef VERBOSE_DEBUG
            it->first->plainPrint();
            #endif
            toRemove[it->second] = true;
            S->removeClause(*it->first);
        }
        
        switch(lits.size()) {
        case 2: {
            if (S->varReplacer->replace(lits, impair, old_group) == false)
                return false;
            
            #ifdef VERBOSE_DEBUG
            XorClause* x = XorClause_new(lits, impair, old_group);
            cout << "- Final 2-long xor-clause: ";
            x->plainPrint();
            free(x);
            #endif
            break;
        }
        default: {
            XorClause* x = XorClause_new(lits, impair, old_group);
            S->xorclauses.push(x);
            S->attachClause(*x);
            
            #ifdef VERBOSE_DEBUG
            cout << "- Final xor-clause: ";
            x->plainPrint();
            #endif
        }
        }
        
        foundXors++;
        sumLengths += lits.size();
    }
    
    return true;
}

void XorFinder::clearToRemove()
{
    assert(toRemove.size() == cls.size());
    
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

