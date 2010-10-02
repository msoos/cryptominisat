/*****************************************************************************
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
******************************************************************************/

#include "VarReplacer.h"
#include <iostream>
#include <iomanip>

#include "ClauseCleaner.h"
#include "PartHandler.h"
#include "time_mem.h"

//#define VERBOSE_DEBUG
//#define DEBUG_REPLACER
//#define REPLACE_STATISTICS

#ifdef VERBOSE_DEBUG
#include <iostream>
using std::cout;
using std::endl;
#endif

VarReplacer::VarReplacer(Solver& _solver) :
    replacedLits(0)
    , replacedVars(0)
    , lastReplacedVars(0)
    , solver(_solver)
{
}

VarReplacer::~VarReplacer()
{
    for (uint32_t i = 0; i != clauses.size(); i++)
        solver.clauseAllocator.clauseFree(clauses[i]);
}

/**
@brief Replaces variables, clears internal clauses, and reports stats

When replacing, it is imperative not to make variables decision variables
which have been removed by other methods:
\li variable removal at the xor-sphere
\li disconnected component finding and solving
\li variable elimination

NOTE: If any new such algoirhtms are added, this part MUST be updated such
that problems don't creep up
*/
const bool VarReplacer::performReplaceInternal()
{
    #ifdef VERBOSE_DEBUG
    cout << "Replacer started." << endl;
    #endif
    double time = cpuTime();

    #ifdef REPLACE_STATISTICS
    uint32_t numRedir = 0;
    for (uint32_t i = 0; i < table.size(); i++) {
        if (table[i].var() != i)
            numRedir++;
    }
    std::cout << "Number of trees:" << reverseTable.size() << std::endl;
    std::cout << "Number of redirected nodes:" << numRedir << std::endl;
    #endif //REPLACE_STATISTICS

    solver.clauseCleaner->removeAndCleanAll(true);
    if (!solver.ok) return false;
    solver.testAllClauseAttach();

    #ifdef VERBOSE_DEBUG
    {
        uint32_t i = 0;
        for (vector<Lit>::const_iterator it = table.begin(); it != table.end(); it++, i++) {
            if (it->var() == i) continue;
            cout << "Replacing var " << i+1 << " with Lit " << (it->sign() ? "-" : "") <<  it->var()+1 << endl;
        }
    }
    #endif

    Var var = 0;
    const vec<char>* removedVars = solver.doXorSubsumption ? &solver.xorSubsumer->getVarElimed() : NULL;
    const vec<lbool>* removedVars2 = solver.doPartHandler ?  &solver.partHandler->getSavedState() : NULL;
    const vec<char>* removedVars3 = solver.doSubsumption ? &solver.subsumer->getVarElimed() : NULL;
    for (vector<Lit>::const_iterator it = table.begin(); it != table.end(); it++, var++) {
        if (it->var() == var
            || (removedVars != NULL && (*removedVars)[it->var()])
            || (removedVars2 != NULL && (*removedVars2)[it->var()] != l_Undef)
            || (removedVars3!= NULL && (*removedVars3)[it->var()])
        ) continue;
        #ifdef VERBOSE_DEBUG
        cout << "Setting var " << var+1 << " to a non-decision var" << endl;
        #endif
        bool wasDecisionVar = solver.decision_var[var];
        solver.setDecisionVar(var, false);
        solver.setDecisionVar(it->var(), true);

        uint32_t& activity1 = solver.activity[var];
        uint32_t& activity2 = solver.activity[it->var()];
        if (wasDecisionVar && activity1 > activity2) {
            activity2 = activity1;
            solver.order_heap.update(it->var());
            solver.polarity[it->var()] = solver.polarity[var]^it->sign();
        }

        activity1 = 0.0;
        solver.order_heap.update(var);
    }
    assert(solver.order_heap.heapProperty());

    uint32_t thisTimeReplaced = replacedVars -lastReplacedVars;
    lastReplacedVars = replacedVars;

    solver.testAllClauseAttach();
    if (!replace_set(solver.binaryClauses, true)) goto end;
    if (!replace_set(solver.clauses, false)) goto end;
    if (!replace_set(solver.learnts, false)) goto end;
    if (!replace_set(solver.xorclauses)) goto end;
    solver.testAllClauseAttach();

end:
    for (uint32_t i = 0; i != clauses.size(); i++)
        solver.removeClause(*clauses[i]);
    clauses.clear();

    if (solver.verbosity >= 2) {
        std::cout << "c Replacing "
        << std::setw(8) << thisTimeReplaced << " vars"
        << " Replaced " <<  std::setw(8) << replacedLits<< " lits"
        << " Time: " << std::setw(8) << std::fixed << std::setprecision(2)
        << cpuTime()-time << " s "
        << std::endl;
    }

    replacedLits = 0;

    solver.order_heap.filter(Solver::VarFilter(solver));

    return solver.ok;
}

/**
@brief Replaces vars in xorclauses
*/
const bool VarReplacer::replace_set(vec<XorClause*>& cs)
{
    XorClause **a = cs.getData();
    XorClause **r = a;
    for (XorClause **end = a + cs.size(); r != end; r++) {
        XorClause& c = **r;

        bool changed = false;
        Var origVar1 = c[0].var();
        Var origVar2 = c[1].var();

        for (Lit *l = &c[0], *end2 = l + c.size(); l != end2; l++) {
            Lit newlit = table[l->var()];
            if (newlit.var() != l->var()) {
                changed = true;
                *l = Lit(newlit.var(), false);
                c.invert(newlit.sign());
                replacedLits++;
            }
        }

        if (changed && handleUpdatedClause(c, origVar1, origVar2)) {
            if (!solver.ok) {
                for(;r != end; r++) solver.clauseAllocator.clauseFree(*r);
                cs.shrink(r-a);
                return false;
            }
            c.setRemoved();
            solver.freeLater.push(&c);
        } else {
            *a++ = *r;
        }
    }
    cs.shrink(r-a);

    return solver.ok;
}

/**
@brief Helper function for replace_set()
*/
const bool VarReplacer::handleUpdatedClause(XorClause& c, const Var origVar1, const Var origVar2)
{
    uint32_t origSize = c.size();
    std::sort(c.getData(), c.getDataEnd());
    Lit p;
    uint32_t i, j;
    for (i = j = 0, p = lit_Undef; i != c.size(); i++) {
        if (c[i].var() == p.var()) {
            //added, but easily removed
            j--;
            p = lit_Undef;
            if (!solver.assigns[c[i].var()].isUndef())
                c.invert(solver.assigns[c[i].var()].getBool());
        } else if (solver.assigns[c[i].var()].isUndef()) //just add
            c[j++] = p = c[i];
        else c.invert(solver.assigns[c[i].var()].getBool()); //modify xorEqualFalse instead of adding
    }
    c.shrink(i - j);
    c.setStrenghtened();

    #ifdef VERBOSE_DEBUG
    cout << "xor-clause after replacing: ";
    c.plainPrint();
    #endif

    switch (c.size()) {
    case 0:
        solver.detachModifiedClause(origVar1, origVar2, origSize, &c);
        if (!c.xorEqualFalse())
            solver.ok = false;
        return true;
    case 1:
        solver.detachModifiedClause(origVar1, origVar2, origSize, &c);
        solver.uncheckedEnqueue(Lit(c[0].var(), c.xorEqualFalse()));
        solver.ok = (solver.propagate().isNULL());
        return true;
    case 2: {
        solver.detachModifiedClause(origVar1, origVar2, origSize, &c);
        c[0] = c[0].unsign();
        c[1] = c[1].unsign();
        addBinaryXorClause(c, c.xorEqualFalse(), c.getGroup(), true);
        return true;
    }
    default:
        solver.detachModifiedClause(origVar1, origVar2, origSize, &c);
        solver.attachClause(c);
        return false;
    }

    assert(false);
    return false;
}

/**
@brief Replaces variables in normal clauses
*/
const bool VarReplacer::replace_set(vec<Clause*>& cs, const bool binClauses)
{
    Clause **a = cs.getData();
    Clause **r = a;
    for (Clause **end = a + cs.size(); r != end; r++) {
        Clause& c = **r;
        bool changed = false;
        Lit origLit1 = c[0];
        Lit origLit2 = c[1];
        Lit origLit3 = (c.size() == 3) ? c[2] : lit_Undef;
        for (Lit *l = c.getData(), *end2 = l + c.size();  l != end2; l++) {
            if (table[l->var()].var() != l->var()) {
                changed = true;
                *l = table[l->var()] ^ l->sign();
                replacedLits++;
            }
        }

        if (changed && handleUpdatedClause(c, origLit1, origLit2, origLit3)) {
            if (!solver.ok) {
                for(;r != end; r++) solver.clauseAllocator.clauseFree(*r);
                cs.shrink(r-a);
                return false;
            }
        } else {
            if (!binClauses && c.size() == 2) {
                solver.detachClause(c);
                Clause *c2 = solver.clauseAllocator.Clause_new(c);
                solver.clauseAllocator.clauseFree(&c);
                solver.attachClause(*c2);
                solver.becameBinary++;
                solver.binaryClauses.push(c2);
            } else
                *a++ = *r;
        }
    }
    cs.shrink(r-a);

    return solver.ok;
}

/**
@brief Helper function for replace_set()
*/
const bool VarReplacer::handleUpdatedClause(Clause& c, const Lit origLit1, const Lit origLit2, const Lit origLit3)
{
    bool satisfied = false;
    std::sort(c.getData(), c.getData() + c.size());
    Lit p;
    uint32_t i, j;
    const uint32_t origSize = c.size();
    for (i = j = 0, p = lit_Undef; i != origSize; i++) {
        if (solver.value(c[i]) == l_True || c[i] == ~p) {
            satisfied = true;
            break;
        }
        else if (solver.value(c[i]) != l_False && c[i] != p)
            c[j++] = p = c[i];
    }
    c.shrink(i - j);
    c.setStrenghtened();

    solver.detachModifiedClause(origLit1, origLit2, origLit3, origSize, &c);

    if (satisfied) return true;

    switch(c.size()) {
    case 0:
        solver.ok = false;
        return true;
    case 1 :
        solver.uncheckedEnqueue(c[0]);
        solver.ok = (solver.propagate().isNULL());
        return true;
    default:
        solver.attachClause(c);

        return false;
    }

    assert(false);
    return false;
}

/**
@brief Returns variables that have been replaced
*/
const vector<Var> VarReplacer::getReplacingVars() const
{
    vector<Var> replacingVars;

    for(map<Var, vector<Var> >::const_iterator it = reverseTable.begin(), end = reverseTable.end(); it != end; it++) {
        replacingVars.push_back(it->first);
    }

    return replacingVars;
}

/**
@brief Given a partial model, it tries to extend it to variables that have been replaced

Cannot extend it fully, because it might be that a replaced d&f, but a was
later variable-eliminated. That's where extendModelImpossible() comes in
*/
void VarReplacer::extendModelPossible() const
{
    #ifdef VERBOSE_DEBUG
    std::cout << "extendModelPossible() called" << std::endl;
    #endif //VERBOSE_DEBUG
    uint32_t i = 0;
    for (vector<Lit>::const_iterator it = table.begin(); it != table.end(); it++, i++) {
        if (it->var() == i) continue;

        #ifdef VERBOSE_DEBUG
        cout << "Extending model: var "; solver.printLit(Lit(i, false));
        cout << " to "; solver.printLit(*it);
        cout << endl;
        #endif

        if (solver.assigns[it->var()] != l_Undef) {
            if (solver.assigns[i] == l_Undef) {
                bool val = (solver.assigns[it->var()] == l_False);
                solver.uncheckedEnqueue(Lit(i, val ^ it->sign()));
            } else {
                assert(solver.assigns[i].getBool() == (solver.assigns[it->var()].getBool() ^ it->sign()));
            }
        }
        solver.ok = (solver.propagate().isNULL());
        assert(solver.ok);
    }
}

/**
@brief Used when a variable was eliminated, but it replaced some other variables

This function will add to solver2 clauses that represent the relationship of
the variables to their replaced cousins. Then, calling solver2.solve() should
take care of everything
*/
void VarReplacer::extendModelImpossible(Solver& solver2) const
{

    #ifdef VERBOSE_DEBUG
    std::cout << "extendModelImpossible() called" << std::endl;
    #endif //VERBOSE_DEBUG

    vec<Lit> tmpClause;
    uint32_t i = 0;
    for (vector<Lit>::const_iterator it = table.begin(); it != table.end(); it++, i++) {
        if (it->var() == i) continue;
        if (solver.assigns[it->var()] == l_Undef) {
            assert(solver.assigns[it->var()] == l_Undef);
            assert(solver.assigns[i] == l_Undef);

            tmpClause.clear();
            tmpClause.push(Lit(it->var(), true));
            tmpClause.push(Lit(i, it->sign()));
            solver2.addClause(tmpClause);
            assert(solver2.ok);

            tmpClause.clear();
            tmpClause.push(Lit(it->var(), false));
            tmpClause.push(Lit(i, it->sign()^true));
            solver2.addClause(tmpClause);
            assert(solver2.ok);
        }
    }
}

/**
@brief Replaces two two vars in "ps" with one another. xorEqualFalse defines anti/equivalence

It can be tricky to do this. For example, if:

\li a replaces: b, c
\li f replaces: f, h
\li we just realised that c = h
This is the most difficult case, but there are other cases, e.g. if we already
know that c=h, in which case we don't do anything

@p ps must contain 2 variables(!), i.e literals with no sign
@p xorEqualFalse if True, the two variables are equivalent. Otherwise, they are antivalent
@p group of clause they have been inspired from. Sometimes makes no sense...
*/
template<class T>
const bool VarReplacer::replace(T& ps, const bool xorEqualFalse, const uint32_t group)
{
    #ifdef VERBOSE_DEBUG
    std::cout << "replace() called with var " << ps[0].var()+1 << " and var " << ps[1].var()+1 << " with xorEqualFalse " << xorEqualFalse << std::endl;
    #endif

    assert(ps.size() == 2);
    assert(!ps[0].sign());
    assert(!ps[1].sign());
    #ifdef DEBUG_REPLACER
    assert(solver.assigns[ps[0].var()].isUndef());
    assert(solver.assigns[ps[1].var()].isUndef());
    #endif

    Var var = ps[0].var();
    Lit lit = Lit(ps[1].var(), !xorEqualFalse);
    assert(var != lit.var());

    //Detect circle
    if (alreadyIn(var, lit)) return solver.ok;

    Lit lit1 = table[var];
    bool inverted = false;

    //This pointer is already set, try to invert
    if (lit1.var() != var) {
        Var tmp_var = var;

        var = lit.var();
        lit = Lit(tmp_var, lit.sign());
        inverted = true;
    }

    if (inverted) {
        Lit lit2 = table[var];

        //Inversion is also set, triangular cycle
        //A->B, A->C, B->C. There is nothing to add
        if (lit1.var() == lit2.var()) {
            if ((lit1.sign() ^ lit2.sign()) != lit.sign()) {
                #ifdef VERBOSE_DEBUG
                cout << "Inverted cycle in var-replacement -> UNSAT" << endl;
                #endif
                return (solver.ok = false);
            }
            return true;
        }

        //Inversion is also set
        if (lit2.var() != var) {
            assert(table[lit1.var()].var() == lit1.var());
            setAllThatPointsHereTo(lit1.var(), Lit(lit.var(), lit1.sign()));

            assert(table[lit2.var()].var() == lit2.var());
            setAllThatPointsHereTo(lit2.var(), lit ^ lit2.sign());

            table[lit.var()] = Lit(lit.var(), false);
            replacedVars++;
            addBinaryXorClause(ps, xorEqualFalse, group);
            return true;
        }
    }

    //Follow forwards
    Lit litX = table[lit.var()];
    if (litX.var() != lit.var())
        lit = litX ^ lit.sign();

    //Follow backwards
    setAllThatPointsHereTo(var, lit);
    replacedVars++;
    addBinaryXorClause(ps, xorEqualFalse, group);

    return true;
}

template const bool VarReplacer::replace(vec<Lit>& ps, const bool xorEqualFalse, const uint32_t group);
template const bool VarReplacer::replace(XorClause& ps, const bool xorEqualFalse, const uint32_t group);

/**
@brief Adds a binary xor to the internal/external clause set

It is added externally ONLY if we are in the middle of replacing clauses,
and a new binary xor just came up. That is a very strange and unfortunate
experience, as we cannot change the datastructures in the middle of replacement
so we add this to the binary clauses of Solver, and we recover it next time.

\todo Clean this messy internal/external thing using a better datastructure.
*/
template<class T>
void VarReplacer::addBinaryXorClause(T& ps, const bool xorEqualFalse, const uint32_t group, const bool internal)
{
    assert(internal || (replacedVars > lastReplacedVars));
    #ifdef DEBUG_REPLACER
    assert(!ps[0].sign());
    assert(!ps[1].sign());
    #endif

    Clause* c;
    ps[0] ^= xorEqualFalse;

    c = solver.clauseAllocator.Clause_new(ps, group, false);
    if (internal) {
        solver.binaryClauses.push(c);
        solver.becameBinary++;
    } else
        clauses.push(c);
    solver.attachClause(*c);

    ps[0] ^= true;
    ps[1] ^= true;
    c = solver.clauseAllocator.Clause_new(ps, group, false);
    if (internal) {
        solver.binaryClauses.push(c);
        solver.becameBinary++;
    } else
        clauses.push(c);
    solver.attachClause(*c);
}

template void VarReplacer::addBinaryXorClause(vec<Lit>& ps, const bool xorEqualFalse, const uint32_t group, const bool internal);
template void VarReplacer::addBinaryXorClause(XorClause& ps, const bool xorEqualFalse, const uint32_t group, const bool internal);

/**
@brief Returns if we already know that var = lit

Also checks if var = ~lit, in which it sets solver.ok = false
*/
bool VarReplacer::alreadyIn(const Var var, const Lit lit)
{
    Lit lit2 = table[var];
    if (lit2.var() == lit.var()) {
        if (lit2.sign() != lit.sign()) {
            #ifdef VERBOSE_DEBUG
            cout << "Inverted cycle in var-replacement -> UNSAT" << endl;
            #endif
            solver.ok = false;
        }
        return true;
    }

    lit2 = table[lit.var()];
    if (lit2.var() == var) {
        if (lit2.sign() != lit.sign()) {
            #ifdef VERBOSE_DEBUG
            cout << "Inverted cycle in var-replacement -> UNSAT" << endl;
            #endif
            solver.ok = false;
        }
        return true;
    }

    return false;
}

/**
@brief Changes internal graph to set everything that pointed to var to point to lit
*/
void VarReplacer::setAllThatPointsHereTo(const Var var, const Lit lit)
{
    map<Var, vector<Var> >::iterator it = reverseTable.find(var);
    if (it != reverseTable.end()) {
        for(vector<Var>::const_iterator it2 = it->second.begin(), end = it->second.end(); it2 != end; it2++) {
            assert(table[*it2].var() == var);
            if (lit.var() != *it2) {
                table[*it2] = lit ^ table[*it2].sign();
                reverseTable[lit.var()].push_back(*it2);
            }
        }
        reverseTable.erase(it);
    }
    table[var] = lit;
    reverseTable[lit.var()].push_back(var);
}

void VarReplacer::newVar()
{
    table.push_back(Lit(table.size(), false));
}

/**
@brief Attaches internal clauses to the watchlist of solver

Sometimes we need to completely detach every clause. The fastest way to do
that is to clear the watchlists. Then, we need to re-attach the clauses. This
function is used in this last stage.

Re-attaching is done, e.g. if many unitary clauses have been found through
failed var searching
*/
void VarReplacer::reattachInternalClauses()
{
    Clause **i = clauses.getData();
    Clause **j = i;
    for (Clause **end = clauses.getDataEnd(); i != end; i++) {
        if (solver.value((**i)[0]) == l_Undef &&
            solver.value((**i)[1]) == l_Undef) {
            solver.attachClause(**i);
            *j++ = *i;
        }

        if (solver.value((**i)[0]) == l_False &&
            solver.value((**i)[1]) == l_Undef) {
            solver.uncheckedEnqueue((**i)[1]);
        }

        if (solver.value((**i)[0]) == l_Undef &&
            solver.value((**i)[1]) == l_False) {
            solver.uncheckedEnqueue((**i)[0]);
        }

        if (solver.value((**i)[0]) == l_False &&
            solver.value((**i)[1]) == l_False) {
            solver.ok = false;
        }

        //either is l_True, it is ignored
    }
    clauses.shrink(i-j);
}
