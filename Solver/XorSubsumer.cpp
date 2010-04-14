/**************************************************************************************************
Originally From: Solver.C -- (C) Niklas Een, Niklas Sorensson, 2004
Substantially modified by: Mate Soos (2010)
**************************************************************************************************/

#include "Solver.h"
#include "XorSubsumer.h"
#include "ClauseCleaner.h"
#include "time_mem.h"
#include "assert.h"
#include <iomanip>
#include "VarReplacer.h"

#ifdef _MSC_VER
#define __builtin_prefetch(a,b,c)
#endif //_MSC_VER

//#define VERBOSE_DEBUG
#ifdef VERBOSE_DEBUG
#define BIT_MORE_VERBOSITY
#endif

//#define BIT_MORE_VERBOSITY
//#define TOUCH_LESS

#ifdef VERBOSE_DEBUG
using std::cout;
using std::endl;
#endif //VERBOSE_DEBUG

XorSubsumer::XorSubsumer(Solver& s):
    solver(s)
{
};

// Will put NULL in 'cs' if clause removed.
void XorSubsumer::subsume0(XorClauseSimp& ps)
{
    assert(solver.xorclauses.size() ==  0);
    #ifdef VERBOSE_DEBUG
    cout << "subsume0 orig clause:";
    ps.clause->plainPrint();
    cout << "pointer:" << &ps << endl;
    #endif
    
    vec<Lit> origClause(ps.clause->size());
    std::copy(ps.clause->getData(), ps.clause->getDataEnd(), origClause.getData());
    const bool origClauseInverted = ps.clause->xor_clause_inverted();
    
    vec<Lit> unmatchedPart;
    bool needUnlinkPS = false;
    
    vec<XorClauseSimp> subs;
    findSubsumed(*ps.clause, subs);
    for (uint32_t i = 0; i < subs.size(); i++){
        #ifdef VERBOSE_DEBUG
        cout << "subsume0 removing:";
        subs[i].clause->plainPrint();
        #endif
        
        XorClause* tmp = subs[i].clause;
        findUnMatched(origClause, *tmp, unmatchedPart);
        if (unmatchedPart.size() == 0) {
            clauses_subsumed++;
            assert(tmp->size() == origClause.size());
            if (origClauseInverted == tmp->xor_clause_inverted()) {
                unlinkClause(subs[i]);
                free(tmp);
            } else {
                solver.ok = false;
                return;
            }
        } else {
            assert(unmatchedPart.size() > 0);
            clauses_cut++;
            //XorClause *c = solver.addXorClauseInt(unmatchedPart, tmp->xor_clause_inverted() ^ origClauseInverted, tmp->getGroup());
            if (!solver.ok) return;
            //if (c != NULL) {
                //linkInClause(*c);
                needUnlinkPS = true;
            //}
        }
        unmatchedPart.clear();
    }
    
    /*if (needUnlinkPS) {
        XorClause* tmp = ps.clause;
        unlinkClause(ps);
        free(tmp);
    }*/
}

void XorSubsumer::findUnMatched(vec<Lit>& A, XorClause& B, vec<Lit>& unmatchedPart)
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

void XorSubsumer::unlinkClause(XorClauseSimp c)
{
    XorClause& cl = *c.clause;
    
    for (uint32_t i = 0; i < cl.size(); i++) {
        maybeRemove(occur[cl[i].var()], &cl);
    }
    
    solver.detachClause(cl);
    
    clauses[c.index].clause = NULL;
}

void XorSubsumer::unlinkModifiedClause(vec<Lit>& origClause, XorClauseSimp c)
{
    for (uint32_t i = 0; i < origClause.size(); i++) {
        maybeRemove(occur[origClause[i].var()], c.clause);
    }
    
    solver.detachModifiedClause(origClause[0].var(), origClause[1].var(), origClause.size(), c.clause);
    
    clauses[c.index].clause = NULL;
}

void XorSubsumer::unlinkModifiedClauseNoDetachNoNULL(vec<Lit>& origClause, XorClauseSimp c)
{
    for (uint32_t i = 0; i < origClause.size(); i++) {
        maybeRemove(occur[origClause[i].var()], c.clause);
    }
}

XorClauseSimp XorSubsumer::linkInClause(XorClause& cl)
{
    XorClauseSimp c(&cl, clauseID++);
    clauses.push(c);
    for (uint32_t i = 0; i < cl.size(); i++) {
        occur[cl[i].var()].push(c);
    }
    
    return c;
}

void XorSubsumer::linkInAlreadyClause(XorClauseSimp& c)
{
    XorClause& cl = *c.clause;
    
    for (uint32_t i = 0; i < c.clause->size(); i++) {
        occur[cl[i].var()].push(c);
    }
}

void XorSubsumer::addFromSolver(vec<XorClause*>& cs)
{
    XorClause **i = cs.getData();
    for (XorClause **end = i + cs.size(); i !=  end; i++) {
        if (i+1 != end)
            __builtin_prefetch(*(i+1), 1, 1);
        
        linkInClause(**i);
        if ((*i)->getVarChanged() || (*i)->getStrenghtened())
            (*i)->calcXorAbstraction();
    }
    cs.clear();
}

void XorSubsumer::addBackToSolver()
{
    for (uint32_t i = 0; i < clauses.size(); i++) {
        if (clauses[i].clause != NULL) {
            solver.xorclauses.push(clauses[i].clause);
            clauses[i].clause->unsetStrenghtened();
            clauses[i].clause->unsetVarChanged();
        }
    }
    for (Var var = 0; var < solver.nVars(); var++) {
        occur[var].clear();
    }
    clauses.clear();
    clauseID = 0;
}

const bool XorSubsumer::simplifyBySubsumption(const bool doFullSubsume)
{
    double myTime = cpuTime();
    uint32_t origTrailSize = solver.trail.size();
    clauses_subsumed = 0;
    clauses_cut = 0;
    clauseID = 0;
    
    for (Var var = 0; var < solver.nVars(); var++) {
        //occur[var].clear(true);
        newVar();
    }
    
    while (solver.performReplace && solver.varReplacer->needsReplace()) {
        if (!solver.varReplacer->performReplace())
            return false;
    }
    
    solver.clauseCleaner->cleanClauses(solver.xorclauses, ClauseCleaner::xorclauses);
    if (!solver.ok) return false;
    
    clauses.clear();
    clauses.reserve(solver.xorclauses.size());
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
            if (clauses[i].clause != NULL) {
                subsume0(clauses[i]);
                if (!solver.ok) return false;
            }
        }
        
        if (solver.qhead != solver.trail.size()) propagated = true;
        solver.ok = solver.propagate() == NULL;
        if (!solver.ok) {
            std::cout << "c (contradiction during subsumption)" << std::endl;
            return false;
        }
        solver.clauseCleaner->cleanXorClausesBewareNULL(clauses, ClauseCleaner::xorSimpClauses, *this);
        if (!solver.ok) return false;
        
        if (solver.performReplace && solver.varReplacer->needsReplace()) {
            addBackToSolver();
            while (solver.performReplace && solver.varReplacer->needsReplace()) {
                replaced = true;
                if (!solver.varReplacer->performReplace())
                    return false;
            }
            addFromSolver(solver.xorclauses);
        }
    }
    
    if (solver.trail.size() - origTrailSize > 0)
        solver.order_heap.filter(Solver::VarFilter(solver));
    
    addBackToSolver();
    
    if (solver.verbosity >= 1) {
        std::cout << "c | xorclauses-subsumed: " << std::setw(9) << clauses_subsumed
        << " xorclauses-cut: " << std::setw(9) << clauses_cut
        << " vars fixed: " << std::setw(3) <<solver.trail.size() - origTrailSize
        << " time: " << std::setw(5) << std::setprecision(2) << (cpuTime() - myTime)
        << "  |" << std::endl;
    }
    
    return true;
}

void XorSubsumer::findSubsumed(XorClause& ps, vec<XorClauseSimp>& out_subsumed)
{
    #ifdef VERBOSE_DEBUG
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
    
    vec<XorClauseSimp>& cs = occur[ps[min_i].var()];
    for (XorClauseSimp *it = cs.getData(), *end = it + cs.size(); it != end; it++){
        if (it+1 != end)
            __builtin_prefetch((it+1)->clause, 1, 1);
        
        if (it->clause != &ps && subsetAbst(ps.getAbst(), it->clause->getAbst()) && ps.size() <= it->clause->size() && subset(ps, *it->clause)) {
            out_subsumed.push(*it);
            #ifdef VERBOSE_DEBUG
            cout << "subsumed: ";
            it->clause->plainPrint();
            #endif
        }
    }
}


