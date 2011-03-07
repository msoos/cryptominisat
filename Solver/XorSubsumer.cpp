/*****************************************************************************
SatELite -- (C) Niklas Een, Niklas Sorensson, 2004
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

Original code by SatELite authors are under an MIT licence.
Modifications for CryptoMiniSat are under GPLv3.
******************************************************************************/

#include "Solver.h"
#include "XorSubsumer.h"
#include "ClauseCleaner.h"
#include "time_mem.h"
#include "assert.h"
#include <iomanip>
#include "VarReplacer.h"
#include "SolutionExtender.h"

//#define VERBOSE_DEBUG
#ifdef VERBOSE_DEBUG
#define VERBOSE_DEBUGSUBSUME0
#define BIT_MORE_VERBOSITY
#endif

#ifdef VERBOSE_DEBUG
using std::cout;
using std::endl;
#endif //VERBOSE_DEBUG

XorSubsumer::XorSubsumer(Solver& s):
    solver(s)
    , totalTime(0.0)
    , numElimed(0)
    , localSubstituteUseful(0)
{
};

// Will put NULL in 'cs' if clause removed.
void XorSubsumer::subsume0(ClauseSimp ps, XorClause& cl)
{
    #ifdef VERBOSE_DEBUGSUBSUME0
    cout << "subsume0 orig clause:";
    ps.clause->plainPrint();
    #endif

    vec<Lit> unmatchedPart;
    vec<ClauseSimp> subs;

    findSubsumed(cl, ps.index, subs);
    for (uint32_t i = 0; i < subs.size(); i++){
        XorClause* tmp = clauses[subs[i].index];
        findUnMatched(*clauses[ps.index], *tmp, unmatchedPart);
        if (unmatchedPart.size() == 0) {
            #ifdef VERBOSE_DEBUGSUBSUME0
            cout << "subsume0 removing:";
            subs[i].clause->plainPrint();
            #endif
            clauses_subsumed++;
            assert(tmp->size() == clauses[ps.index]->size());
            if (clauses[ps.index]->xorEqualFalse() == tmp->xorEqualFalse()) {
                unlinkClause(subs[i]);
            } else {
                solver.ok = false;
                return;
            }
        } else {
            assert(unmatchedPart.size() > 0);
            clauses_cut++;
            #ifdef VERBOSE_DEBUG
            std::cout << "Cutting xor-clause:";
            subs[i].clause->plainPrint();
            #endif //VERBOSE_DEBUG
            XorClause *c = solver.addXorClauseInt(unmatchedPart, tmp->xorEqualFalse() ^ !clauses[ps.index]->xorEqualFalse(), tmp->getGroup());
            if (c != NULL)
                linkInClause(*c);
            unlinkClause(subs[i]);
            if (!solver.ok) return;
        }
        unmatchedPart.clear();
    }
}

template<class T>
void XorSubsumer::findUnMatched(const T& A, const T& B, vec<Lit>& unmatchedPart)
{
    for (uint32_t i = 0; i != B.size(); i++)
        seen_tmp[B[i].var()] = 1;
    for (uint32_t i = 0; i != A.size(); i++)
        seen_tmp[A[i].var()] = 0;
    for (uint32_t i = 0; i != B.size(); i++) {
        if (seen_tmp[B[i].var()] == 1) {
            unmatchedPart.push(Lit(B[i].var(), false));
            seen_tmp[B[i].var()] = 0;
        }
    }
}

void XorSubsumer::unlinkClause(ClauseSimp c, const Var elim)
{
    XorClause& cl = *clauses[c.index];

    for (uint32_t i = 0; i < cl.size(); i++) {
        removeW(occur[cl[i].var()], c);
    }

    if (elim != var_Undef) {
        XorElimedClause data;
        for (Lit *it = cl.getData(), *end = cl.getDataEnd(); it != end; it++) {
            data.lits.push_back(it->unsign());
        }
        data.xorEqualFalse = cl.xorEqualFalse();
        elimedOutVar[elim].push_back(data);
    }
    solver.detachClause(cl);
    solver.clauseAllocator.clauseFree(clauses[c.index]);

    clauses[c.index] = NULL;
}

void XorSubsumer::unlinkModifiedClause(vec<Lit>& origClause, ClauseSimp c)
{
    for (uint32_t i = 0; i < origClause.size(); i++) {
        removeW(occur[origClause[i].var()], c);
    }

    solver.detachModifiedClause(origClause[0].var(), origClause[1].var(), origClause.size(), clauses[c.index]);

    clauses[c.index] = NULL;
}

void XorSubsumer::unlinkModifiedClauseNoDetachNoNULL(vec<Lit>& origClause, ClauseSimp c)
{
    for (uint32_t i = 0; i < origClause.size(); i++) {
        removeW(occur[origClause[i].var()], c);
    }
}

ClauseSimp XorSubsumer::linkInClause(XorClause& cl)
{
    ClauseSimp c(clauses.size());
    clauses.push(&cl);
    clauseData.push(AbstData(cl, false));
    for (uint32_t i = 0; i < cl.size(); i++) {
        occur[cl[i].var()].push(c);
    }

    return c;
}

void XorSubsumer::linkInAlreadyClause(ClauseSimp& c)
{
    XorClause& cl = *clauses[c.index];

    for (uint32_t i = 0; i < cl.size(); i++) {
        occur[cl[i].var()].push(c);
    }
}

void XorSubsumer::addFromSolver(vec<XorClause*>& cs)
{
    clauses.clear();
    clauseData.clear();
    XorClause **i = cs.getData();
    for (XorClause **end = i + cs.size(); i !=  end; i++) {
        if (i+1 != end) __builtin_prefetch(*(i+1));

        linkInClause(**i);
    }
    cs.clear();
    cs.push(NULL); //HACK --to force xor-propagation
}

void XorSubsumer::addBackToSolver()
{
    solver.xorclauses.pop(); //HACK --to force xor-propagation
    for (uint32_t i = 0; i < clauses.size(); i++) {
        if (clauses[i] != NULL) {
            solver.xorclauses.push(clauses[i]);
            clauses[i]->unsetStrenghtened();
        }
    }
    for (Var var = 0; var < solver.nVars(); var++) {
        occur[var].clear();
    }
}

void XorSubsumer::fillCannotEliminate()
{
    std::fill(cannot_eliminate.getData(), cannot_eliminate.getDataEnd(), false);
    for (uint32_t i = 0; i < solver.clauses.size(); i++)
        addToCannotEliminate(solver.clauses[i]);

    uint32_t wsLit = 0;
    for (const vec2<Watched> *it = solver.watches.getData(), *end = solver.watches.getDataEnd(); it != end; it++, wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        const vec2<Watched>& ws = *it;
        for (vec2<Watched>::const_iterator it2 = ws.getData(), end2 = ws.getDataEnd(); it2 != end2; it2++) {
            if (it2->isBinary() && !it2->getLearnt()) {
                cannot_eliminate[lit.var()] = true;
                cannot_eliminate[it2->getOtherLit().var()] = true;
            }
        }
    }

    for (Var var = 0; var < solver.nVars(); var++) {
        cannot_eliminate[var] |= solver.varReplacer->cannot_eliminate[var];
    }

    #ifdef VERBOSE_DEBUG
    uint32_t tmpNum = 0;
    for (uint32_t i = 0; i < cannot_eliminate.size(); i++)
        if (cannot_eliminate[i])
            tmpNum++;
        std::cout << "Cannot eliminate num:" << tmpNum << std::endl;
    #endif
}

void XorSubsumer::extendModel(SolutionExtender* extender)
{
    #ifdef VERBOSE_DEBUG
    std::cout << "XorSubsumer::extendModel(Solver& solver2) called" << std::endl;
    #endif

    assert(checkElimedUnassigned());
    vector<Lit> tmpClause;
    typedef map<Var, vector<XorElimedClause> > elimType;
    for (elimType::iterator it = elimedOutVar.begin(), end = elimedOutVar.end(); it != end; it++) {
        #ifdef VERBOSE_DEBUG
        Var var = it->first;
        std::cout << "Reinserting elimed var: " << var+1 << std::endl;
        #endif

        for (vector<XorElimedClause>::iterator it2 = it->second.begin(), end2 = it->second.end(); it2 != end2; it2++) {
            XorElimedClause& c = *it2;
            #ifdef VERBOSE_DEBUG
            std::cout << "Reinserting Clause: ";
            c.plainPrint();
            #endif
            tmpClause = c.lits;
            extender->addXorClause(tmpClause, c.xorEqualFalse);
        }
    }
}

const bool XorSubsumer::localSubstitute()
{
    vec<Lit> tmp;
    for (Var var = 0; var < occur.size(); var++) {
        vec<ClauseSimp>& occ = occur[var];

        if (occ.size() <= 1) continue;
        for (uint32_t i = 0; i < occ.size(); i++) {
            XorClause& c1 = *clauses[occ[i].index];
            for (uint32_t i2 = i+1; i2 < occ.size(); i2++) {
                XorClause& c2 = *clauses[occ[i2].index];
                tmp.clear();
                xorTwoClauses(c1, c2, tmp);
                if (tmp.size() <= 2) {
                    #ifdef VERBOSE_DEBUG
                    std::cout << "Local substiuting. Clause1:"; c1.plainPrint();
                    std::cout << "Clause 2:"; c2.plainPrint();
                    #endif //VERBOSE_DEBUG
                    localSubstituteUseful++;
                    XorClause* ret = solver.addXorClauseInt(tmp, c1.xorEqualFalse() ^ !c2.xorEqualFalse(), c1.getGroup());
                    release_assert(ret == NULL);
                    if (!solver.ok) {
                        #ifdef VERBOSE_DEBUG
                        std::cout << "solver.ok is false after local substitution" << std::endl;
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
void XorSubsumer::xorTwoClauses(const T& c1, const T& c2, vec<Lit>& xored)
{
    for (uint32_t i = 0; i != c1.size(); i++)
        seen_tmp[c1[i].var()] = 1;
    for (uint32_t i = 0; i != c2.size(); i++)
        seen_tmp[c2[i].var()] ^= 1;

    for (uint32_t i = 0; i != c1.size(); i++) {
        if (seen_tmp[c1[i].var()] == 1) {
            xored.push(Lit(c1[i].var(), false));
            seen_tmp[c1[i].var()] = 0;
        }
    }
    for (uint32_t i = 0; i != c2.size(); i++) {
        if (seen_tmp[c2[i].var()] == 1) {
            xored.push(Lit(c2[i].var(), false));
            seen_tmp[c2[i].var()] = 0;
        }
    }
}

void XorSubsumer::removeWrong(vec<Clause*>& cs)
{
    Clause **i = cs.getData();
    Clause **j = i;
    for (Clause **end =  i + cs.size(); i != end; i++) {
        Clause& c = **i;
        if (!c.learnt())  {
            *j++ = *i;
            continue;
        }
        bool remove = false;
        for (Lit *l = c.getData(), *end2 = l+c.size(); l != end2; l++) {
            if (var_elimed[l->var()]) {
                remove = true;
                solver.detachClause(c);
                solver.clauseAllocator.clauseFree(&c);
                break;
            }
        }
        if (!remove)
            *j++ = *i;
    }
    cs.shrink(i-j);
}

void XorSubsumer::removeWrongBins()
{
    uint32_t numRemovedHalfLearnt = 0;
    uint32_t wsLit = 0;
    for (vec2<Watched> *it = solver.watches.getData(), *end = solver.watches.getDataEnd(); it != end; it++, wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        vec2<Watched>& ws = *it;

        vec2<Watched>::iterator i = ws.getData();
        vec2<Watched>::iterator j = i;
        for (vec2<Watched>::iterator end2 = ws.getDataEnd(); i != end2; i++) {
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


const bool XorSubsumer::removeDependent()
{
    for (Var var = 0; var < occur.size(); var++) {
        if (cannot_eliminate[var] || !solver.decision_var[var] || solver.assigns[var] != l_Undef) continue;
        vec<ClauseSimp>& occ = occur[var];

        if (occ.size() == 1) {
            #ifdef VERBOSE_DEBUG
            std::cout << "Eliminating dependent var " << var + 1 << std::endl;
            std::cout << "-> Removing dependent clause "; occ[0].clause->plainPrint();
            #endif //VERBOSE_DEBUG
            unlinkClause(occ[0], var);
            assert(solver.varData[var].elimed == ELIMED_NONE);
            solver.setDecisionVar(var, false);
            var_elimed[var] = true;
            solver.varData[var].elimed = ELIMED_XORVARELIM;
            numElimed++;
        } else if (occ.size() == 2) {
            vec<Lit> lits;
            XorClause& c1 = *clauses[occ[0].index];
            lits.growTo(c1.size());
            std::copy(c1.getData(), c1.getDataEnd(), lits.getData());
            bool inverted = c1.xorEqualFalse();

            XorClause& c2 = *clauses[occ[1].index];
            lits.growTo(lits.size() + c2.size());
            std::copy(c2.getData(), c2.getDataEnd(), lits.getData() + c1.size());
            inverted ^= !c2.xorEqualFalse();
            uint32_t group = c2.getGroup();
            uint32_t ret = removeAll(lits, var);
            release_assert(ret == 2);

            #ifdef VERBOSE_DEBUG
            std::cout << "Eliminating var " << var + 1 << " present in 2 xor-clauses" << std::endl;
            std::cout << "-> Removing xor clause "; occ[0].clause->plainPrint();
            std::cout << "-> Removing xor clause "; occ[1].clause->plainPrint();
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
            XorClause* c = solver.addXorClauseInt(lits, inverted, group);
            #ifdef VERBOSE_DEBUG
            if (c != NULL) {
                std::cout << "-> Added combined xor clause:"; c->plainPrint();
            } else
                std::cout << "-> Combined xor clause is NULL" << std::endl;
            #endif
            if (c != NULL) linkInClause(*c);
            if (!solver.ok) {
                #ifdef VERBOSE_DEBUG
                std::cout << "solver.ok is false after var-elim through xor" << std::endl;
                #endif //VERBOSE_DEBUG
                return false;
            }
        }
    }

    return true;
}

inline void XorSubsumer::addToCannotEliminate(Clause* it)
{
    const Clause& c = *it;
    for (uint32_t i2 = 0; i2 < c.size(); i2++)
        cannot_eliminate[c[i2].var()] = true;
}

const bool XorSubsumer::unEliminate(const Var var)
{
    assert(var_elimed[var]);
    assert(solver.varData[var].elimed = ELIMED_XORVARELIM);
    vec<Lit> tmp;
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
    std::cout << "Reinserting xor elimed var: " << var+1 << std::endl;
    #endif

    for (vector<XorElimedClause>::iterator it2 = it->second.begin(), end2 = it->second.end(); it2 != end2; it2++) {
        XorElimedClause& c = *it2;
        tmp.clear();
        tmp.growTo(c.lits.size());
        std::copy(c.lits.begin(), c.lits.end(), tmp.getData());
        #ifdef VERBOSE_DEBUG
        std::cout << "Reinserting elimed clause: " << tmp << std::endl;;
        #endif
        solver.addXorClause(tmp, c.xorEqualFalse);
    }
    elimedOutVar.erase(it);

    return solver.ok;
}


const bool XorSubsumer::simplifyBySubsumption()
{
    double myTime = cpuTime();
    uint32_t origTrailSize = solver.trail.size();
    clauses_subsumed = 0;
    clauses_cut = 0;
    uint32_t lastNumElimed = numElimed;
    localSubstituteUseful = 0;
    while (solver.conf.doReplace && solver.varReplacer->needsReplace()) {
        if (!solver.varReplacer->performReplace())
            return false;
    }

    for (Var var = 0; var < solver.nVars(); var++) {
        occur[var].clear();
    }
    solver.findAllAttach();

    solver.clauseCleaner->cleanClauses(solver.xorclauses, ClauseCleaner::xorclauses);
    if (!solver.ok) return false;

    addFromSolver(solver.xorclauses);
    #ifdef BIT_MORE_VERBOSITY
    std::cout << "c time to link in:" << cpuTime()-myTime << std::endl;
    #endif

    origNClauses = clauses.size();

    if (!solver.ok) return false;
    #ifdef VERBOSE_DEBUG
    std::cout << "c   clauses:" << clauses.size() << std::endl;
    #endif

    bool replaced = true;
    bool propagated = false;
    while (replaced || propagated) {
        replaced = propagated = false;
        for (uint32_t i = 0; i < clauses.size(); i++) {
            if (clauses[i] != NULL) {
                subsume0(i, *clauses[i]);
                if (!solver.ok) {
                    addBackToSolver();
                    return false;
                }
            }
        }

        propagated =  (solver.qhead != solver.trail.size());
        solver.ok = (solver.propagate<false>().isNULL());
        if (!solver.ok) {
            return false;
        }
        if (!solver.ok) return false;

        fillCannotEliminate();
        if (solver.conf.doConglXors && !removeDependent()) {
            addBackToSolver();
            return false;
        }

        if (solver.conf.doHeuleProcess && !localSubstitute()) {
            addBackToSolver();
            return false;
        }

        /*if (solver.doReplace && solver.varReplacer->needsReplace()) {
            addBackToSolver();
            while (solver.doReplace && solver.varReplacer->needsReplace()) {
                replaced = true;
                if (!solver.varReplacer->performReplace())
                    return false;
            }
            addFromSolver(solver.xorclauses);
        }*/
    }

    solver.order_heap.filter(Solver::VarFilter(solver));

    removeWrong(solver.learnts);
    removeWrongBins();
    addBackToSolver();
    removeAssignedVarsFromEliminated();

    if (solver.conf.verbosity >= 1) {
        std::cout << "c x-sub: " << std::setw(5) << clauses_subsumed
        << " x-cut: " << std::setw(6) << clauses_cut
        << " vfix: " << std::setw(6) <<solver.trail.size() - origTrailSize
        << " v-elim: " <<std::setw(6) << numElimed - lastNumElimed
        << " locsubst:" << std::setw(6) << localSubstituteUseful
        << " time: " << std::setw(6) << std::setprecision(2) << (cpuTime() - myTime)
        << std::endl;
    }
    totalTime += cpuTime() - myTime;

    solver.testAllClauseAttach();
    return true;
}

void XorSubsumer::findSubsumed(XorClause& ps, uint32_t index, vec<ClauseSimp>& out_subsumed)
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

    vec<ClauseSimp>& cs = occur[ps[min_i].var()];
    for (ClauseSimp *it = cs.getData(), *end = it + cs.size(); it != end; it++){
        if (it->index != index
            && subsetAbst(clauseData[index].abst, clauseData[it->index].abst)
            && ps.size() <= clauseData[it->index].size
            && subset(ps, *clauses[it->index])
           ) {
            out_subsumed.push(*it);
            #ifdef VERBOSE_DEBUGSUBSUME0
            cout << "subsumed: ";
            it->clause->plainPrint();
            #endif
        }
    }
}

const bool XorSubsumer::checkElimedUnassigned() const
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

void XorSubsumer::removeAssignedVarsFromEliminated()
{
    for (Var var = 0; var < var_elimed.size(); var++) {
        if (var_elimed[var] && solver.assigns[var] != l_Undef) {
            assert(solver.varData[var].elimed = ELIMED_XORVARELIM);

            var_elimed[var] = false;
            solver.varData[var].elimed = ELIMED_NONE;
            solver.setDecisionVar(var, true);
            numElimed--;
            map<Var, vector<XorElimedClause> >::iterator it = elimedOutVar.find(var);
            if (it != elimedOutVar.end()) elimedOutVar.erase(it);
        }
    }
}
