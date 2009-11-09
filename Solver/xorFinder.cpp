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

//#define VERBOSE_DEBUG

#ifdef VERBOSE_DEBUG
#include <iostream>
using std::cout;
using std::endl;
#endif

using std::make_pair;

XorFinder::XorFinder(Solver* _S, vec<Clause*>& _cls, vec<XorClause*>& _xorcls, const uint _minSize) :
    S(_S)
    , cls(_cls)
    , xorcls(_xorcls)
    , minSize(_minSize)
    , maxSize(10)
{
}

uint XorFinder::doByPart(uint& sumLengths)
{
    uint sumNonParitionClauses = 0;
    uint sumUsage = 0;
    vector<uint> varUsage(S->nVars(), 0);
    for (Clause **it = cls.getData(), **end = it + cls.size(); it != end; it++) {
        const uint size = (*it)->size();
        if ( size > maxSize || size < minSize) continue;
        
        for (const Lit *l = &(**it)[0], *end = l + size; l != end; l++) {
            varUsage[l->var()]++;
            sumUsage++;
        }
        sumNonParitionClauses++;
    }
    
    uint sumNumClauses = 0;
    uint found = 0;
    sumLengths = 0;
    
    const uint limit = 400000;
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
        #endif
        
        uint numClauses = 0;
        table.clear();
        table.resize(estimate/2);
        uint i = 0;
        for (Clause **it = cls.getData(), **end = it + cls.size(); it != end; it++, i++) {
            const uint size = (*it)->size();
            if ( size > maxSize || size < minSize) continue;
            
            for (Lit *l = &(**it)[0], *end = l + size; l != end; l++) {
                if (l->var() >= from  && l->var() <= until) {
                    table[*it].push_back(make_pair(*it, i));
                    numClauses++;
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
    
    S->replace(toReplace);
    
    #ifdef VERBOSE_DEBUG
    cout << "Overdone work due to partitioning:" << (double)sumNumClauses/(double)sumNonParitionClauses << "x" << endl;
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
    vector<bool> toRemove(cls.size(), false);
    
    nextXor = table.begin();
    const vector<pair<Clause*, uint> >* myclauses;
    vector<Lit> lits;
    bool impair;
    while ((myclauses = getNextXor(impair)) != NULL) {
        const Clause& c = *((*myclauses)[0].first);
        lits.clear();
        for (const Lit *it = &c[0], *end = it+c.size() ; it != end; it++) {
            lits.push_back(Lit(it->var(), false));
        }
        uint old_group = c.group;
        
        #ifdef VERBOSE_DEBUG
        cout << "- Found clauses:" << endl;
        #endif
        
        for (const pair<Clause*, uint> *it = &(myclauses->at(0)), *end = it + myclauses->size() ; it != end; it++) {
            #ifdef VERBOSE_DEBUG
            it->first->plain_print();
            #endif
            toRemove[it->second] = true;
            S->detachClause(*it->first);
            free(it->first);
        }
        
        XorClause* x = XorClause_new(lits, impair, old_group);
        assert(x->size() > 1);
        if (x->size() == 2) {
            toReplace[lits[0].var()] = Lit(lits[1].var(), !impair);
            S->calcAtFinish.push_back(make_pair(x, lits[0].var()));
        } else {
            xorcls.push(x);
            S->attachClause(*x);
            #ifdef VERBOSE_DEBUG
            cout << "- Final xor-clause: ";
            x->plain_print();
            #endif
        }
        
        foundXors++;
        sumLengths += lits.size();
    }
    
    Clause **a = cls.getData();
    Clause **r = cls.getData();
    Clause **end = cls.getData() + cls.size();
    for (uint i = 0; r != end; i++) {
        if (!toRemove[i])
            *a++ = *r++;
        else
            r++;
    }
    cls.shrink(r-a);
    
    return foundXors;
}

const vector<pair<Clause*, uint> >* XorFinder::getNextXor(bool& impair)
{
    for (; nextXor != table.end(); nextXor++) {
        if (isXor(nextXor->second, impair)) {
            ClauseTable::iterator tmp = nextXor;
            nextXor++;
            return &(tmp->second);
        }
    }
    
    return NULL;
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

bool XorFinder::isXor(vector<pair<Clause*, uint> >& clauses, bool& impair)
{
    uint size = clauses.size();
    assert(size > 0);
    const uint requiredSize = 1 << (clauses[0].first->size()-1);
    
    if (size < 4 || size < requiredSize)
        return false;
    
    clause_sorter clause_sorter_object;
    std::sort(clauses.begin(), clauses.end(), clause_sorter_object);
    
    uint numPair = 0;
    uint numImpair = 0;
    countImpairs(clauses, numImpair, numPair);
    
    if (numImpair < 4 && numPair < 4)
        return false;
    
    if (numImpair == requiredSize) {
        impair = true;
        if (numImpair != clauses.size())
            cleanNotRightImPair(clauses, impair);
        
        return true;
    }
    
    if (numPair == requiredSize) {
        impair = false;
        if (numPair != clauses.size()) 
            cleanNotRightImPair(clauses, impair);
        
        return true;
    }
    
    return false;
}

void XorFinder::cleanNotRightImPair(vector<pair<Clause*, uint> >& clauses, const bool impair) const
{
    pair<Clause*, uint>* a = &(clauses[0]);
    pair<Clause*, uint>* r = a;
    pair<Clause*, uint>* end = a + clauses.size();
    
    for (; r != end;) {
        if (impairSigns(*(r->first)) != impair) {
            r++;
        } else {
            *a++ = *r++;
        }
    }
    clauses.resize(clauses.size()-(r-a));
}

void XorFinder::countImpairs(const vector<pair<Clause*, uint> >& clauses, uint& numImpair, uint& numPair) const
{
    numImpair = 0;
    numPair = 0;
    
    vector<pair<Clause*, uint> >::const_iterator it = clauses.begin();
    vector<pair<Clause*, uint> >::const_iterator it2 = it;
    it2++;
    
    bool impair = impairSigns(*it->first);
    numImpair += impair;
    numPair += !impair;
    
    for (; it2 != clauses.end();) {
        if (!clauseEqual(*it->first, *it2->first)) {
            bool impair = impairSigns(*it2->first);
            numImpair += impair;
            numPair += !impair;
        }
        it++;
        it2++;
    }
}

