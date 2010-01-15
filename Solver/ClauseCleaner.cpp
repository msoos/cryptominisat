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

void ClauseCleaner::removeSatisfied(vec<XorClause*>& cs, ClauseSetType type, const uint limit)
{
    #ifdef DEBUG_CLEAN
    assert(solver.decisionLevel() == 0);
    #endif
    
    if (lastNumUnitarySat[type] + limit >= solver.get_unitary_learnts_num())
        return;
    
    int i,j;
    for (i = j = 0; i < cs.size(); i++) {
        if (satisfied(*cs[i]))
            solver.removeClause(*cs[i]);
        else
            cs[j++] = cs[i];
    }
    cs.shrink_(i - j);
    
    lastNumUnitarySat[type] = solver.get_unitary_learnts_num();
}

void ClauseCleaner::removeSatisfied(vec<Clause*>& cs, ClauseSetType type, const uint limit)
{
    #ifdef DEBUG_CLEAN
    assert(solver.decisionLevel() == 0);
    #endif
    
    if (lastNumUnitarySat[type] + limit >= solver.get_unitary_learnts_num())
        return;
    
    int i,j;
    for (i = j = 0; i != cs.size(); i++) {
        __builtin_prefetch(cs[i+1], 0, 0);
        if (satisfied(*cs[i]))
            solver.removeClause(*cs[i]);
        else
            cs[j++] = cs[i];
    }
    cs.shrink_(i - j);
    
    lastNumUnitarySat[type] = solver.get_unitary_learnts_num();
}

inline const bool ClauseCleaner::cleanClause(Clause& c)
{
    Lit origLit1 = c[0];
    Lit origLit2 = c[1];
    uint32_t origSize = c.size();
    
    Lit *i, *j, *end;
    for (i = j = c.getData(), end = i + c.size();  i != end; i++) {
        lbool val = solver.value(*i);
        if (val == l_Undef) {
            *j = *i;
            j++;
        }
        if (val == l_True) {
            solver.detachModifiedClause(origLit1, origLit2, origSize, &c);
            return true;
        }
    }
    
    if ((c.size() > 2) && (c.size() - (i-j) == 2)) {
        solver.detachModifiedClause(origLit1, origLit2, c.size(), &c);
        c.shrink(i-j);
        solver.attachClause(c);
    } else {
        c.shrink(i-j);
        if (c.learnt())
            solver.learnts_literals -= i-j;
        else
            solver.clauses_literals -= i-j;
    }
    
    return false;
}

void ClauseCleaner::cleanClauses(vec<Clause*>& cs, ClauseSetType type, const uint limit)
{
    #ifdef DEBUG_CLEAN
    assert(solver.decisionLevel() == 0);
    #endif
    
    if (lastNumUnitaryClean[type] + limit >= solver.get_unitary_learnts_num())
        return;
    
    Clause **s, **ss, **end;
    for (s = ss = cs.getData(), end = s + cs.size();  s != end;) {
        __builtin_prefetch((s+1), 1, 0);
        if (cleanClause(**s)) {
            clauseFree(*s);
            s++;
        } else if (type != ClauseCleaner::binaryClauses && (*s)->size() == 2) {
            solver.binaryClauses.push(*s);
            solver.becameBinary++;
            s++;
        } else {
            *ss++ = *s++;
        }
    }
    cs.shrink(s-ss);
    
    lastNumUnitaryClean[type] = solver.get_unitary_learnts_num();
    
    #ifdef VERBOSE_DEBUG
    cout << "cleanClauses(Clause) useful ?? Removed: " << s-ss << endl;
    #endif
}

void ClauseCleaner::cleanClauses(vec<XorClause*>& cs, ClauseSetType type, const uint limit)
{
    #ifdef DEBUG_CLEAN
    assert(solver.decisionLevel() == 0);
    #endif
    
    if (lastNumUnitaryClean[type] + limit >= solver.get_unitary_learnts_num())
        return;
    
    XorClause **s, **ss, **end;
    for (s = ss = cs.getData(), end = s + cs.size();  s != end;) {
        __builtin_prefetch((s+1), 1, 0);
        if (cleanClause(**s)) {
            (**s).mark(1);
            solver.freeLater.push(*s);
            s++;
        } else {
            *ss++ = *s++;
        }
    }
    cs.shrink(s-ss);
    
    lastNumUnitaryClean[type] = solver.get_unitary_learnts_num();
    
    #ifdef VERBOSE_DEBUG
    cout << "cleanClauses(XorClause) useful: ?? Removed: " << s-ss << endl;
    #endif
}

inline const bool ClauseCleaner::cleanClause(XorClause& c)
{
    Lit *i, *j, *end;
    Var origVar1 = c[0].var();
    Var origVar2 = c[1].var();
    uint32_t origSize = c.size();
    for (i = j = c.getData(), end = i + c.size();  i != end; i++) {
        const lbool& val = solver.assigns[i->var()];
        if (val.isUndef()) {
            *j = *i;
            j++;
        } else c.invert(val.getBool());
    }
    c.shrink(i-j);
    
    switch (c.size()) {
        case 0: {
            solver.detachModifiedClause(origVar1, origVar2, origSize, &c);
            return true;
        }
        case 2: {
            vec<Lit> ps(2);
            ps[0] = c[0].unsign();
            ps[1] = c[1].unsign();
            solver.varReplacer->replace(ps, c.xor_clause_inverted(), c.getGroup());
            solver.detachModifiedClause(origVar1, origVar2, origSize, &c);
            return true;
        }
        default:
            solver.clauses_literals -= i-j;
            return false;
    }
}

bool ClauseCleaner::satisfied(const Clause& c) const
{
    for (uint i = 0; i != c.size(); i++)
        if (solver.value(c[i]) == l_True)
            return true;
        return false;
}

bool ClauseCleaner::satisfied(const XorClause& c) const
{
    bool final = c.xor_clause_inverted();
    for (uint k = 0; k != c.size(); k++ ) {
        const lbool& val = solver.assigns[c[k].var()];
        if (val.isUndef()) return false;
        final ^= val.getBool();
    }
    return final;
}
