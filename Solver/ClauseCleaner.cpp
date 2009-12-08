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

#include "ClauseCleaner.h"

//#define DEBUG_CLEAN

ClauseCleaner::ClauseCleaner(Solver& _solver) :
    solver(_solver)
{
    for (uint i = 0; i < 4; i++) {
        lastNumUnitarySat[i] = solver.get_unitary_learnts_num();
        lastNumUnitaryClean[i] = solver.get_unitary_learnts_num();
    }
}

void ClauseCleaner::removeSatisfied(vec<XorClause*>& cs, ClauseSetType type)
{
    #ifdef DEBUG_CLEAN
    assert(solver.decisionLevel() == 0);
    #endif
    
    if (lastNumUnitarySat[type] == solver.get_unitary_learnts_num())
        return;
    
    int i,j;
    for (i = j = 0; i < cs.size(); i++) {
        if (satisfied(*cs[i]))
            solver.removeClause(*cs[i]);
        else
            cs[j++] = cs[i];
    }
    cs.shrink(i - j);
    
    lastNumUnitarySat[type] = solver.get_unitary_learnts_num();
}

void ClauseCleaner::removeSatisfied(vec<Clause*>& cs, ClauseSetType type)
{
    #ifdef DEBUG_CLEAN
    assert(solver.decisionLevel() == 0);
    #endif
    
    if (lastNumUnitarySat[type] == solver.get_unitary_learnts_num())
        return;
    
    int i,j;
    for (i = j = 0; i < cs.size(); i++) {
        if (satisfied(*cs[i]))
            solver.removeClause(*cs[i]);
        else
            cs[j++] = cs[i];
    }
    cs.shrink(i - j);
    
    lastNumUnitarySat[type] = solver.get_unitary_learnts_num();
}

bool ClauseCleaner::cleanClause(Clause& c)
{
    assert(c.size() >= 2);
    Lit first = c[0];
    Lit second = c[1];
    
    Lit *i, *j, *end;
    uint at = 0;
    for (i = j = c.getData(), end = i + c.size();  i != end; i++, at++) {
        if (solver.value(*i) == l_Undef) {
            *j = *i;
            j++;
        } else assert(at > 1);
        assert(solver.value(*i) != l_True);
    }
    if ((c.size() > 2) && (c.size() - (i-j) == 2)) {
        solver.detachModifiedClause(first, second, c.size(), &c);
        c.shrink(i-j);
        solver.attachClause(c);
    } else
        c.shrink(i-j);
    
    assert(c.size() > 1);
    
    return (i-j > 0);
}

void ClauseCleaner::cleanClauses(vec<Clause*>& cs, ClauseSetType type)
{
    #ifdef DEBUG_CLEAN
    assert(solver.decisionLevel() == 0);
    #endif
    
    if (lastNumUnitaryClean[type] == solver.get_unitary_learnts_num())
        return;
    
    uint useful = 0;
    for (int s = 0; s < cs.size(); s++)
        useful += cleanClause(*cs[s]);
    
    lastNumUnitaryClean[type] = solver.get_unitary_learnts_num();
    
    #ifdef VERBOSE_DEBUG
    cout << "cleanClauses(Clause) useful:" << useful << endl;
    #endif
}

void ClauseCleaner::cleanClauses(vec<XorClause*>& cs, ClauseSetType type)
{
    #ifdef DEBUG_CLEAN
    assert(solver.decisionLevel() == 0);
    #endif
    
    if (lastNumUnitaryClean[type] == solver.get_unitary_learnts_num())
        return;
    
    uint useful = 0;
    XorClause **s, **ss, **end;
    for (s = ss = cs.getData(), end = s + cs.size();  s != end;) {
        XorClause& c = **s;
        #ifdef VERBOSE_DEBUG
        std::cout << "Cleaning clause:";
        c.plain_print();
        solver.printClause(c);std::cout << std::endl;
        #endif
        
        Lit *i, *j, *end;
        uint at = 0;
        for (i = j = c.getData(), end = i + c.size();  i != end; i++, at++) {
            const lbool& val = solver.assigns[i->var()];
            if (val.isUndef()) {
                *j = *i;
                j++;
            } else /*assert(at>1),*/ c.invert(val.getBool());
        }
        c.shrink(i-j);
        if (i-j > 0) useful++;
        
        if (c.size() == 2) {
            vec<Lit> ps(2);
            ps[0] = c[0].unsign();
            ps[1] = c[1].unsign();
            solver.toReplace->replace(ps, c.xor_clause_inverted(), c.group);
            solver.removeClause(c);
            s++;
        } else
            *ss++ = *s++;
        
        #ifdef VERBOSE_DEBUG
        std::cout << "Cleaned clause:";
        c.plain_print();
        solver.printClause(c);std::cout << std::endl;
        #endif
        assert(c.size() > 1);
    }
    cs.shrink(s-ss);
    
    lastNumUnitaryClean[type] = solver.get_unitary_learnts_num();
    
    #ifdef VERBOSE_DEBUG
    cout << "cleanClauses(XorClause) useful:" << useful << endl;
    #endif
}

bool ClauseCleaner::satisfied(const Clause& c) const
{
    for (uint i = 0; i < c.size(); i++)
        if (solver.value(c[i]) == l_True)
            return true;
        return false;
}

bool ClauseCleaner::satisfied(const XorClause& c) const
{
    bool final = c.xor_clause_inverted();
    for (uint k = 0; k < c.size(); k++ ) {
        const lbool& val = solver.assigns[c[k].var()];
        if (val.isUndef()) return false;
        final ^= val.getBool();
    }
    return final;
}
