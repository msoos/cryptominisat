/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2011, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/

#include "Solver.h"
#include "XorSimplifier.h"
#include "ClauseCleaner.h"
#include "time_mem.h"
#include "assert.h"
#include <iomanip>
#include "VarReplacer.h"
#include "SolutionExtender.h"
#include "CommandControl.h"

//#define VERBOSE_DEBUG
#ifdef VERBOSE_DEBUG
#define VERBOSE_DEBUGSUBSUME0
#define BIT_MORE_VERBOSITY
#endif

XorSimplifier::XorSimplifier(Solver& s):
    solver(s)
    , totalTime(0.0)
    , numElimed(0)
    , localSubstituteUseful(0)
{
}

// Will put NULL in 'cs' if clause removed.
void XorSimplifier::subsume0(ClauseSimp ps, XorClause& cl)
{
    #ifdef VERBOSE_DEBUGSUBSUME0
    cout << "subsume0 orig clause:" << *clauses[ps.index] << endl;
    #endif

    vector<Lit> unmatchedPart;
    vector<ClauseSimp> subs;

    findSubsumed(cl, ps.index, subs);
    //clauses in 'subs' are subsumed by cl

    for (uint32_t i = 0; i < subs.size(); i++){
        XorClause* tmp = clauses[subs[i].index];
        findUnMatched(cl, *tmp, unmatchedPart);

        if (unmatchedPart.size() == 0) {
            #ifdef VERBOSE_DEBUGSUBSUME0
            cout << "subsume0 removing:" << *clauses[subs[i].index] << endl;
            #endif
            clauses_subsumed++;
            assert(tmp->size() == cl.size());
            if (cl.xorEqualFalse() == tmp->xorEqualFalse()) {
                unlinkClause(subs[i]);
            } else {
                solver.ok = false;
                return;
            }
        } else {
            assert(unmatchedPart.size() > 0);
            clauses_cut++;
            #ifdef VERBOSE_DEBUG
            cout << "Cutting xor-clause:" << *clauses[subs[i].index] << endl;
            #endif //VERBOSE_DEBUG
            XorClause *c = solver.addXorClauseInt(unmatchedPart, tmp->xorEqualFalse() ^ !cl.xorEqualFalse());
            if (c != NULL)
                linkInClause(*c);
            unlinkClause(subs[i]);
            if (!solver.ok) return;
        }
        unmatchedPart.clear();
    }
}

template<class T>
void XorSimplifier::findUnMatched(const T& A, const T& B, vector<Lit>& unmatchedPart)
{
    for (uint32_t i = 0; i != B.size(); i++)
        seen_tmp[B[i].var()] = 1;
    for (uint32_t i = 0; i != A.size(); i++)
        seen_tmp[A[i].var()] = 0;
    for (uint32_t i = 0; i != B.size(); i++) {
        if (seen_tmp[B[i].var()] == 1) {
            unmatchedPart.push_back(Lit(B[i].var(), false));
            seen_tmp[B[i].var()] = 0;
        }
    }
}

void XorSimplifier::unlinkClause(ClauseSimp c, const Var elim)
{
    XorClause& cl = *clauses[c.index];

    for (uint32_t i = 0; i < cl.size(); i++) {
        removeW(occur[cl[i].var()], c);
    }

    if (elim != var_Undef) {
        XorElimedClause data;
        for (Lit *it = cl.begin(), *end = cl.end(); it != end; it++) {
            data.lits.push_back(it->unsign());
        }
        data.xorEqualFalse = cl.xorEqualFalse();
        elimedOutVar[elim].push_back(data);
    }
    solver.removeClause(cl);
    clauses[c.index] = NULL;
}

ClauseSimp XorSimplifier::linkInClause(XorClause& cl)
{
    ClauseSimp c(clauses.size());
    clauses.push_back(&cl);
    clauseData.push_back(AbstData(cl, false));
    for (uint32_t i = 0; i < cl.size(); i++) {
        occur[cl[i].var()].push_back(c);
    }

    return c;
}

void XorSimplifier::linkInAlreadyClause(ClauseSimp& c)
{
    XorClause& cl = *clauses[c.index];

    for (uint32_t i = 0; i < cl.size(); i++) {
        occur[cl[i].var()].push_back(c);
    }
}

void XorSimplifier::addFromSolver(vector<XorClause*>& cs)
{
    clauses.clear();
    clauseData.clear();
    vector<XorClause*>::iterator i = cs.begin();
    for (vector<XorClause*>::iterator end = cs.end(); i !=  end; i++) {
        if (i+1 != end) __builtin_prefetch(*(i+1));

        linkInClause(**i);
    }
    cs.clear();
}

void XorSimplifier::addBackToSolver()
{
    for (uint32_t i = 0; i < clauses.size(); i++) {
        if (clauses[i] != NULL) {
            solver.xorclauses.push_back(clauses[i]);
            clauses[i]->unsetStrenghtened();
        }
    }
    for (Var var = 0; var < solver.nVars(); var++) {
        occur[var].clear();
    }
}

void XorSimplifier::fillCannotEliminate()
{
    std::fill(cannot_eliminate.begin(), cannot_eliminate.end(), false);
    for (uint32_t i = 0; i < solver.clauses.size(); i++)
        addToCannotEliminate(solver.clauses[i]);

    uint32_t wsLit = 0;
    for (vector<vec2<Watched> >::const_iterator it = solver.watches.begin(), end = solver.watches.end(); it != end; it++, wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        const vec2<Watched>& ws = *it;
        for (vec2<Watched>::const_iterator it2 = ws.begin(), end2 = ws.end(); it2 != end2; it2++) {
            if (it2->isBinary() && !it2->getLearnt()) {
                cannot_eliminate[lit.var()] = true;
                cannot_eliminate[it2->getOtherLit().var()] = true;
            }
        }
    }

    #ifdef VERBOSE_DEBUG
    uint32_t tmpNum = 0;
    for (uint32_t i = 0; i < cannot_eliminate.size(); i++)
        if (cannot_eliminate[i])
            tmpNum++;
        cout << "Cannot eliminate num:" << tmpNum << endl;
    #endif
}

void XorSimplifier::extendModel(SolutionExtender* extender)
{
    #ifdef VERBOSE_DEBUG
    cout << "XorSimplifier::extendModel(Solver& solver2) called" << endl;
    #endif

    assert(checkElimedUnassigned());
    vector<Lit> tmpClause;
    typedef map<Var, vector<XorElimedClause> > elimType;
    for (elimType::iterator it = elimedOutVar.begin(), end = elimedOutVar.end(); it != end; it++) {
        #ifdef VERBOSE_DEBUG
        Var var = it->first;
        cout << "Reinserting elimed var: " << var+1 << endl;
        #endif

        for (vector<XorElimedClause>::iterator it2 = it->second.begin(), end2 = it->second.end(); it2 != end2; it2++) {
            XorElimedClause& c = *it2;
            #ifdef VERBOSE_DEBUG
            cout << "Reinserting Clause: " << c << endl;
            #endif
            tmpClause = c.lits;
            if (!extender->addXorClause(tmpClause, c.xorEqualFalse)) {
                cout << "Error adding eliminated xor-clause while extending" << endl;
                exit(-1);
            }
        }
    }
}

const bool XorSimplifier::localSubstitute()
{
    vector<Lit> tmp;
    for (Var var = 0; var < occur.size(); var++) {
        vector<ClauseSimp>& occ = occur[var];

        if (occ.size() <= 1) continue;
        for (uint32_t i = 0; i < occ.size(); i++) {
            XorClause& c1 = *clauses[occ[i].index];
            for (uint32_t i2 = i+1; i2 < occ.size(); i2++) {
                XorClause& c2 = *clauses[occ[i2].index];
                tmp.clear();
                xorTwoClauses(c1, c2, tmp);
                if (tmp.size() <= 2) {
                    #ifdef VERBOSE_DEBUG
                    cout << "Local substiuting. Clause1:" << c1 << endl;
                    cout << "Clause 2:" << c2 << endl;
                    #endif //VERBOSE_DEBUG
                    localSubstituteUseful++;
                    XorClause* ret = solver.addXorClauseInt(tmp, c1.xorEqualFalse() ^ !c2.xorEqualFalse());
                    release_assert(ret == NULL);
                    if (!solver.ok) {
                        #ifdef VERBOSE_DEBUG
                        cout << "solver.ok is false after local substitution" << endl;
                        #endif //VERBOSE_DEBUG
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

template<class T>
void XorSimplifier::xorTwoClauses(const T& c1, const T& c2, vector<Lit>& xored)
{
    for (uint32_t i = 0; i != c1.size(); i++)
        seen_tmp[c1[i].var()] = 1;
    for (uint32_t i = 0; i != c2.size(); i++)
        seen_tmp[c2[i].var()] ^= 1;

    for (uint32_t i = 0; i != c1.size(); i++) {
        if (seen_tmp[c1[i].var()] == 1) {
            xored.push_back(Lit(c1[i].var(), false));
            seen_tmp[c1[i].var()] = 0;
        }
    }
    for (uint32_t i = 0; i != c2.size(); i++) {
        if (seen_tmp[c2[i].var()] == 1) {
            xored.push_back(Lit(c2[i].var(), false));
            seen_tmp[c2[i].var()] = 0;
        }
    }
}

void XorSimplifier::removeWrong(vector<Clause*>& cs)
{
    vector<Clause*>::iterator i = cs.begin();
    vector<Clause*>::iterator j = i;
    for (vector<Clause*>::iterator end =  i + cs.size(); i != end; i++) {
        Clause& c = **i;
        if (!c.learnt())  {
            *j++ = *i;
            continue;
        }
        bool remove = false;
        for (Lit *l = c.begin(), *end2 = l+c.size(); l != end2; l++) {
            if (var_elimed[l->var()]) {
                remove = true;
                solver.detachClause(c);
                solver.clAllocator.clauseFree(&c);
                break;
            }
        }
        if (!remove)
            *j++ = *i;
    }
    cs.resize(cs.size() - (i-j));
}

void XorSimplifier::removeWrongBins()
{
    uint32_t numRemovedHalfLearnt = 0;
    uint32_t wsLit = 0;
    for (vector<vec2<Watched> >::iterator it = solver.watches.begin(), end = solver.watches.end(); it != end; it++, wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        vec2<Watched>& ws = *it;

        vec2<Watched>::iterator i = ws.begin();
        vec2<Watched>::iterator j = i;
        for (vec2<Watched>::iterator end2 = ws.end(); i != end2; i++) {
            if (i->isBinary()
                && i->getLearnt()
                && (var_elimed[lit.var()] || var_elimed[i->getOtherLit().var()])
                ) {
                numRemovedHalfLearnt++;
            } else {
                assert(!i->isBinary() || (!var_elimed[lit.var()] && !var_elimed[i->getOtherLit().var()]));
                *j++ = *i;
            }
        }
        ws.shrink_(i - j);
    }

    assert(numRemovedHalfLearnt % 2 == 0);
    solver.learnts_literals -= numRemovedHalfLearnt;
    solver.numBins -= numRemovedHalfLearnt/2;
}


const bool XorSimplifier::removeDependent()
{
    for (Var var = 0; var < solver.nVars(); var++) {
        if (cannot_eliminate[var]
            || solver.value(var) != l_Undef
            || solver.varData[var].elimed != ELIMED_NONE) continue;

        vector<ClauseSimp>& occ = occur[var];

        if (occ.size() == 1) {
            #ifdef VERBOSE_DEBUG
            cout << "Eliminating dependent var " << var + 1 << endl;
            cout << "-> Removing dependent clause " << *clauses[occ[0].index] << endl;;
            #endif //VERBOSE_DEBUG
            unlinkClause(occ[0], var);
            solver.setDecisionVar(var, false);
            var_elimed[var] = true;
            solver.varData[var].elimed = ELIMED_XORVARELIM;
            numElimed++;
        } else if (occ.size() == 2) {
            vector<Lit> lits;
            XorClause& c1 = *clauses[occ[0].index];
            lits.resize(c1.size());
            std::copy(c1.begin(), c1.end(), lits.begin());
            bool inverted = c1.xorEqualFalse();

            XorClause& c2 = *clauses[occ[1].index];
            lits.resize(lits.size() + c2.size());
            std::copy(c2.begin(), c2.end(), lits.begin() + c1.size());
            inverted ^= !c2.xorEqualFalse();
            uint32_t ret = removeAll(lits, var);
            release_assert(ret == 2);

            #ifdef VERBOSE_DEBUG
            cout << "Eliminating var " << var + 1 << " present in 2 xor-clauses" << endl;
            cout << "-> Removing xor clause " << *clauses[occ[0].index] << endl;
            cout << "-> Removing xor clause " << *clauses[occ[1].index] << endl;
            #endif //VERBOSE_DEBUG
            ClauseSimp toUnlink0 = occ[0];
            ClauseSimp toUnlink1 = occ[1];
            unlinkClause(toUnlink0);
            unlinkClause(toUnlink1, var);
            solver.setDecisionVar(var, false);
            var_elimed[var] = true;
            solver.varData[var].elimed = ELIMED_XORVARELIM;
            numElimed++;

            for (uint32_t i = 0; i < lits.size(); i++)
                cannot_eliminate[lits[i].var()] = true;
            XorClause* c = solver.addXorClauseInt(lits, inverted);
            #ifdef VERBOSE_DEBUG
            if (c != NULL) {
                cout << "-> Added combined xor clause:" << c << endl;;
            } else
                cout << "-> Combined xor clause is NULL" << endl;
            #endif
            if (c != NULL) linkInClause(*c);
            if (!solver.ok) {
                #ifdef VERBOSE_DEBUG
                cout << "solver.ok is false after var-elim through xor" << endl;
                #endif //VERBOSE_DEBUG
                return false;
            }
        }
    }

    return true;
}

inline void XorSimplifier::addToCannotEliminate(Clause* it)
{
    const Clause& c = *it;
    for (uint32_t i2 = 0; i2 < c.size(); i2++)
        cannot_eliminate[c[i2].var()] = true;
}

const bool XorSimplifier::unEliminate(const Var var, CommandControl* ccsolver)
{
    assert(var_elimed[var]);
    assert(solver.varData[var].elimed == ELIMED_XORVARELIM);
    vector<Lit> tmp;
    typedef map<Var, vector<XorElimedClause> > elimType;
    elimType::iterator it = elimedOutVar.find(var);

    //MUST set to decision, since it would never have been eliminated
    //had it not been decision var
    solver.setDecisionVar(var, true);
    var_elimed[var] = false;
    solver.varData[var].elimed = ELIMED_NONE;
    numElimed--;
    assert(it != elimedOutVar.end());
    #ifdef VERBOSE_DEBUG
    cout << "Reinserting xor elimed var: " << var+1 << endl;
    #endif

    for (vector<XorElimedClause>::iterator it2 = it->second.begin(), end2 = it->second.end(); it2 != end2; it2++) {
        XorElimedClause& c = *it2;
        #ifdef VERBOSE_DEBUG
        cout << "Reinserting elimed XOR clause: " << c.lits << " xorIsFalse:" << c.xorEqualFalse << endl;
        #endif
        ccsolver->addXorClause(c.lits, c.xorEqualFalse);
    }
    elimedOutVar.erase(it);

    return solver.ok;
}


const bool XorSimplifier::simplifyBySubsumption()
{
    double myTime = cpuTime();
    uint32_t origTrailSize = solver.trail.size();
    clauses_subsumed = 0;
    clauses_cut = 0;
    uint32_t lastNumElimed = numElimed;
    localSubstituteUseful = 0;
    for (Var var = 0; var < solver.nVars(); var++) {
        occur[var].clear();
    }

    //Cleaning
    if (!solver.varReplacer->performReplace()) return false;
    solver.clauseCleaner->cleanClauses(solver.xorclauses, ClauseCleaner::xorclauses);
    if (!solver.ok) return false;

    //Add from solver to here
    addFromSolver(solver.xorclauses);
    origNClauses = clauses.size();

    #ifdef VERBOSE_DEBUG
    cout << "c   clauses:" << clauses.size() << endl;
    #endif

    bool propagated = true;
    while (propagated) {
        propagated = false;
        for (uint32_t i = 0; i < clauses.size(); i++) {
            if (clauses[i] != NULL) {
                subsume0(i, *clauses[i]);
                if (!solver.ok) {
                    addBackToSolver();
                    return false;
                }
            }
        }

        propagated = (solver.qhead != solver.trail.size());
        solver.ok = (solver.propagate().isNULL());
        if (!solver.ok) {
            addBackToSolver();
            return false;
        }

        fillCannotEliminate();
        if (solver.conf.doConglXors && !removeDependent()) {
            addBackToSolver();
            return false;
        }

        if (solver.conf.doHeuleProcess && !localSubstitute()) {
            addBackToSolver();
            return false;
        }
    }

    solver.order_heap.filter(Solver::VarFilter(solver));

    removeWrong(solver.learnts);
    removeWrongBins();
    addBackToSolver();
    removeAssignedVarsFromEliminated();

    if (solver.conf.verbosity >= 1) {
        cout << "c x-sub: " << std::setw(5) << clauses_subsumed
        << " x-cut: " << std::setw(6) << clauses_cut
        << " vfix: " << std::setw(6) <<solver.trail.size() - origTrailSize
        << " v-elim: " <<std::setw(6) << numElimed - lastNumElimed
        << " locsubst:" << std::setw(6) << localSubstituteUseful
        << " time: " << std::setw(6) << std::setprecision(2) << (cpuTime() - myTime)
        << endl;
    }
    totalTime += cpuTime() - myTime;

    solver.testAllClauseAttach();
    return true;
}

void XorSimplifier::findSubsumed(XorClause& ps, uint32_t index, vector<ClauseSimp>& out_subsumed)
{
    #ifdef VERBOSE_DEBUGSUBSUME0
    cout << "findSubsumed: ";
    for (uint32_t i = 0; i < ps.size(); i++) {
        if (ps[i].sign()) printf("-");
        printf("%d ", ps[i].var() + 1);
    }
    printf("0\n");
    #endif

    uint32_t min_i = 0;
    for (uint32_t i = 1; i < ps.size(); i++){
        if (occur[ps[i].var()].size() < occur[ps[min_i].var()].size())
            min_i = i;
    }

    vector<ClauseSimp>& cs = occur[ps[min_i].var()];
    for (vector<ClauseSimp>::iterator it = cs.begin(), end = it + cs.size(); it != end; it++){
        if (it->index != index
            && subsetAbst(clauseData[index].abst, clauseData[it->index].abst)
            && ps.size() <= clauseData[it->index].size
            && subset(ps, *clauses[it->index])
           ) {
            out_subsumed.push_back(*it);
            #ifdef VERBOSE_DEBUGSUBSUME0
            cout << "subsumed: " << *clauses[it->index] << endl;
            #endif
        }
    }
}

const bool XorSimplifier::checkElimedUnassigned() const
{
    uint32_t checkNumElimed = 0;
    for (uint32_t i = 0; i < var_elimed.size(); i++) {
        if (var_elimed[i]) {
            checkNumElimed++;
            assert(solver.assigns[i] == l_Undef);
            if (solver.assigns[i] != l_Undef) return false;
        }
    }
    assert(numElimed == checkNumElimed);

    return true;
}

void XorSimplifier::removeAssignedVarsFromEliminated()
{
    for (Var var = 0; var < var_elimed.size(); var++) {
        if (var_elimed[var] && solver.assigns[var] != l_Undef) {
            assert(solver.varData[var].elimed == ELIMED_XORVARELIM);

            var_elimed[var] = false;
            solver.varData[var].elimed = ELIMED_NONE;
            solver.setDecisionVar(var, true);
            numElimed--;
            map<Var, vector<XorElimedClause> >::iterator it = elimedOutVar.find(var);
            if (it != elimedOutVar.end()) elimedOutVar.erase(it);
        }
    }
}
