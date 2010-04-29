/**************************************************************************************************
Originally From: Solver.C -- (C) Niklas Een, Niklas Sorensson, 2004
Substantially modified by: Mate Soos (2010)
**************************************************************************************************/

#include "Solver.h"
#include "Subsumer.h"
#include "ClauseCleaner.h"
#include "time_mem.h"
#include "assert.h"
#include <iomanip>
#include <cmath>
#include <algorithm>
#include "VarReplacer.h"
#include "Conglomerate.h"
#include "XorFinder.h"

#ifdef _MSC_VER
#define __builtin_prefetch(a,b,c)
#endif //_MSC_VER


//#define VERBOSE_DEBUG
#ifdef VERBOSE_DEBUG
#define BIT_MORE_VERBOSITY
#endif

//#define BIT_MORE_VERBOSITY
//#define HYPER_DEBUG
//#define HYPER_DEBUG2
//#define TOUCH_LESS

#ifdef VERBOSE_DEBUG
using std::cout;
using std::endl;
#endif //VERBOSE_DEBUG

Subsumer::Subsumer(Solver& s):
    solver(s)
    , numCalls(0)
    , numElimed(0)
{
};

Subsumer::~Subsumer()
{
}

void Subsumer::extendModel(Solver& solver2)
{
    vec<Lit> tmp;
    typedef map<Var, vector<vector<Lit> > > elimType;
    for (elimType::iterator it = elimedOutVar.begin(), end = elimedOutVar.end(); it != end; it++) {
        #ifdef VERBOSE_DEBUG
        Var var = it->first;
        std::cout << "Reinserting elimed var: " << var+1 << std::endl;
        #endif
        
        for (vector<vector<Lit> >::iterator it2 = it->second.begin(), end2 = it->second.end(); it2 != end2; it2++) {
            tmp.clear();
            tmp.growTo(it2->size());
            std::copy(it2->begin(), it2->end(), tmp.getData());
            
            #ifdef VERBOSE_DEBUG
            std::cout << "Reinserting Clause: ";
            for (uint32_t i = 0; i < tmp.size(); i++) {
                std::cout << (tmp[i].sign() ? "-" : "")<< tmp[i].var()+1 << " ";
            }
            std::cout << std::endl;
            #endif
            
            solver2.addClause(tmp);
            assert(solver2.ok);
        }
    }
}

const bool Subsumer::unEliminate(const Var var)
{
    vec<Lit> tmp;
    typedef map<Var, vector<vector<Lit> > > elimType;
    elimType::iterator it = elimedOutVar.find(var);
    
    solver.setDecisionVar(var, true);
    var_elimed[var] = false;
    numElimed--;
    
    //If the variable was removed because of
    //pure literal removal (by blocked clause
    //elimination, there are no clauses to re-insert
    if (it == elimedOutVar.end()) return solver.ok;
    
    FILE* backup_libraryCNFfile = solver.libraryCNFFile;
    solver.libraryCNFFile = NULL;
    for (vector<vector<Lit> >::iterator it2 = it->second.begin(), end2 = it->second.end(); it2 != end2; it2++) {
        tmp.clear();
        tmp.growTo(it2->size());
        std::copy(it2->begin(), it2->end(), tmp.getData());
        solver.addClause(tmp);
    }
    solver.libraryCNFFile = backup_libraryCNFfile;
    elimedOutVar.erase(it);
    
    return solver.ok;
}

bool selfSubset(uint32_t A, uint32_t B)
{
    uint32_t B_tmp = B | ((B & 0xAAAAAAAALL) >> 1) | ((B & 0x55555555LL) << 1);
    if ((A & ~B_tmp) == 0){
        uint32_t C = A & ~B;
        return (C & (C-1)) == 0;
    }else
        return false;
}

// Assumes 'seen' is cleared (will leave it cleared)
bool selfSubset(Clause& A, Clause& B, vec<char>& seen)
{
    for (uint32_t i = 0; i < B.size(); i++)
        seen[B[i].toInt()] = 1;
    
    bool    flip = false;
    for (uint32_t i = 0; i < A.size(); i++) {
        if (!seen[A[i].toInt()]) {
            if (flip == true || !seen[(~A[i]).toInt()]) {
                for (uint32_t i = 0; i < B.size(); i++) seen[B[i].toInt()] = 0;
                return false;
            }
            flip = true;
        }
    }
    for (uint32_t i = 0; i < B.size(); i++)
        seen[B[i].toInt()] = 0;
    return flip;
}

// Will put NULL in 'cs' if clause removed.
uint32_t Subsumer::subsume0(Clause& ps)
{
    ps.subsume0Finished();
    ps.unsetVarChanged();
    uint32_t retIndex = std::numeric_limits<uint32_t>::max();
    #ifdef VERBOSE_DEBUG
    cout << "subsume0 orig clause:";
    ps.plainPrint();
    #endif
    
    vec<ClauseSimp> subs;
    findSubsumed(ps, subs);
    for (uint32_t i = 0; i < subs.size(); i++){
        clauses_subsumed++;
        #ifdef VERBOSE_DEBUG
        cout << "subsume0 removing:";
        subs[i].clause->plainPrint();
        #endif
        
        Clause* tmp = subs[i].clause;
        unlinkClause(subs[i]);
        free(tmp);
        retIndex = subs[i].index;
    }
    
    return retIndex;
}

// Will put NULL in 'cs' if clause removed.
uint32_t Subsumer::subsume0(Clause& ps, uint32_t abs)
{
    ps.subsume0Finished();
    ps.unsetVarChanged();
    uint32_t retIndex = std::numeric_limits<uint32_t>::max();
    #ifdef VERBOSE_DEBUG
    cout << "subsume0 orig clause:";
    ps.plainPrint();
    cout << "pointer:" << &ps << endl;
    #endif
    
    vec<ClauseSimp> subs;
    findSubsumed(ps, abs, subs);
    for (uint32_t i = 0; i < subs.size(); i++){
        clauses_subsumed++;
        #ifdef VERBOSE_DEBUG
        cout << "subsume0 removing:";
        subs[i].clause->plainPrint();
        #endif
        
        Clause* tmp = subs[i].clause;
        retIndex = subs[i].index;
        unlinkClause(subs[i]);
        free(tmp);
    }
    
    return retIndex;
}

void Subsumer::unlinkClause(ClauseSimp c, Var elim)
{
    Clause& cl = *c.clause;
    
    if (elim != var_Undef) {
        assert(!cl.learnt());
        io_tmp.clear();
        for (uint32_t i = 0; i < cl.size(); i++)
            io_tmp.push_back(cl[i]);
        elimedOutVar[elim].push_back(io_tmp);
    }
    
    if (updateOccur(cl)) {
        for (uint32_t i = 0; i < cl.size(); i++) {
            maybeRemove(occur[cl[i].toInt()], &cl);
            #ifndef TOUCH_LESS
            touch(cl[i]);
            #endif
        }
    }
    
    solver.detachClause(cl);
    
    // Remove from iterator vectors/sets:
    for (uint32_t i = 0; i < iter_vecs.size(); i++) {
        vec<ClauseSimp>& cs = *iter_vecs[i];
        for (uint32_t j = 0; j < cs.size(); j++)
            if (cs[j].clause == &cl)
                cs[j].clause = NULL;
    }
    for (uint32_t i = 0; i < iter_sets.size(); i++) {
        CSet& cs = *iter_sets[i];
        cs.exclude(c);
    }
    
    // Remove clause from clause touched set:
    if (updateOccur(cl)) {
        cl_touched.exclude(c);
        cl_added.exclude(c);
    }
    
    clauses[c.index].clause = NULL;
}

void Subsumer::unlinkModifiedClause(vec<Lit>& origClause, ClauseSimp c)
{
    if (updateOccur(*c.clause)) {
        for (uint32_t i = 0; i < origClause.size(); i++) {
            maybeRemove(occur[origClause[i].toInt()], c.clause);
            #ifndef TOUCH_LESS
            touch(origClause[i]);
            #endif
        }
    }
    
    solver.detachModifiedClause(origClause[0], origClause[1], origClause.size(), c.clause);
    
    // Remove from iterator vectors/sets:
    for (uint32_t i = 0; i < iter_vecs.size(); i++){
        vec<ClauseSimp>& cs = *iter_vecs[i];
        for (uint32_t j = 0; j < cs.size(); j++)
            if (cs[j].clause == c.clause)
                cs[j].clause = NULL;
    }
    for (uint32_t i = 0; i < iter_sets.size(); i++){
        CSet& cs = *iter_sets[i];
        cs.exclude(c);
    }
    
    // Remove clause from clause touched set:
    if (updateOccur(*c.clause)) {
        cl_touched.exclude(c);
        cl_added.exclude(c);
    }
    
    clauses[c.index].clause = NULL;
}

void Subsumer::unlinkModifiedClauseNoDetachNoNULL(vec<Lit>& origClause, ClauseSimp c)
{
    if (updateOccur(*c.clause)) {
        for (uint32_t i = 0; i < origClause.size(); i++) {
            maybeRemove(occur[origClause[i].toInt()], c.clause);
            #ifndef TOUCH_LESS
            touch(origClause[i]);
            #endif
        }
    }
    
    // Remove from iterator vectors/sets:
    for (uint32_t i = 0; i < iter_vecs.size(); i++){
        vec<ClauseSimp>& cs = *iter_vecs[i];
        for (uint32_t j = 0; j < cs.size(); j++)
            if (cs[j].clause == c.clause)
                cs[j].clause = NULL;
    }
    for (uint32_t i = 0; i < iter_sets.size(); i++){
        CSet& cs = *iter_sets[i];
        cs.exclude(c);
    }
    
    // Remove clause from clause touched set:
    if (updateOccur(*c.clause)) {
        cl_touched.exclude(c);
        cl_added.exclude(c);
    }
}

void Subsumer::subsume1(ClauseSimp& ps)
{
    vec<ClauseSimp>    Q;
    vec<ClauseSimp>    subs;
    vec<Lit>        qs;
    uint32_t        q;
    
    ps.clause->unsetStrenghtened();
    registerIteration(Q);
    registerIteration(subs);
    
    Q.push(ps);
    q = 0;
    while (q < Q.size()){
        if (Q[q].clause == NULL) { q++; continue; }
        #ifdef VERBOSE_DEBUG
        cout << "subsume1 orig clause:";
        Q[q].clause->plainPrint();
        #endif
        
        qs.clear();
        for (uint32_t i = 0; i < Q[q].clause->size(); i++)
            qs.push((*Q[q].clause)[i]);
        
        for (uint32_t i = 0; i < qs.size(); i++){
            qs[i] = ~qs[i];
            
            uint32_t abst = calcAbstraction(qs);
            
            findSubsumed(qs, abst, subs);
            for (uint32_t j = 0; j < subs.size(); j++){
                /*#ifndef NDEBUG
                if (&counter != NULL && counter == -1){
                    dump(*subs[j].clause);
                    qs[i] = ~qs[i];
                    dump(qs);
                    printf(L_LIT"\n", L_lit(qs[i]));
                    exit(0);
                }
                #endif*/
                if (subs[j].clause == NULL) continue;
                ClauseSimp c = subs[j];
                Clause& cl = *c.clause;
                #ifdef VERBOSE_DEBUG
                cout << "orig clause    :";
                cl.plainPrint();
                #endif
                unlinkClause(subs[j]);
                
                literals_removed++;
                cl.strengthen(qs[i]);
                Lit *a, *b, *end;
                for (a = b = cl.getData(), end = a + cl.size();  a != end; a++) {
                    lbool val = solver.value(*a);
                    if (val == l_Undef)
                        *b++ = *a;
                    
                    if (val == l_True) {
                        free(&cl);
                        goto endS;
                    }
                }
                cl.shrink(a-b);
                cl.setStrenghtened();
                
                #ifdef VERBOSE_DEBUG
                cout << "strenghtened   :";
                c.clause->plainPrint();
                #endif
                
                if (cl.size() == 0) {
                    solver.ok = false;
                    unregisterIteration(Q);
                    unregisterIteration(subs);
                    free(&cl);
                    return;
                }
                if (cl.size() > 1) {
                    cl.calcAbstraction();
                    linkInAlreadyClause(c);
                    clauses[c.index] = c;
                    solver.attachClause(cl);
                    if (cl.size() == 2) solver.becameBinary++;
                    updateClause(c);
                    Q.push(c);
                } else {
                    assert(cl.size() == 1);
                    solver.uncheckedEnqueue(cl[0]);
                    solver.ok = (solver.propagate() == NULL);
                    if (!solver.ok) {
                        unregisterIteration(Q);
                        unregisterIteration(subs);
                        return;
                    }
                    #ifdef VERBOSE_DEBUG
                    cout << "Found that var " << cl[0].var()+1 << " must be " << std::boolalpha << !cl[0].sign() << endl;
                    #endif
                    free(&cl);
                }
                endS:;
            }
            
            qs[i] = ~qs[i];
            subs.clear();
        }
        q++;
    }
    
    unregisterIteration(Q);
    unregisterIteration(subs);
}

void Subsumer::updateClause(ClauseSimp c)
{
    if (!c.clause->learnt()) subsume0(*c.clause);
    
    cl_touched.add(c);
}

void Subsumer::almost_all_database()
{
    #ifdef BIT_MORE_VERBOSITY
    std::cout << "c Larger database" << std::endl;
    #endif
    // Optimized variant when virtually whole database is involved:
    cl_added  .clear();
    cl_touched.clear();
    
    for (uint32_t i = 0; i < clauses.size(); i++) {
        if (numMaxSubsume1 == 0) break;
        if (clauses[i].clause != NULL && updateOccur(*clauses[i].clause)) {
            subsume1(clauses[i]);
            numMaxSubsume1--;
            if (!solver.ok) return;
        }
    }
    
    assert(solver.ok);
    solver.ok = (solver.propagate() == NULL);
    if (!solver.ok) {
        std::cout << "c (contradiction during subsumption)" << std::endl;
        return;
    }
    solver.clauseCleaner->cleanClausesBewareNULL(clauses, ClauseCleaner::simpClauses, *this);
    
    #ifdef VERBOSE_DEBUG
    cout << "subsume1 part 1 finished" << endl;
    #endif
    
    CSet s1;
    registerIteration(s1);
    while (cl_touched.size() > 0 && numMaxSubsume1 > 0){
        #ifdef VERBOSE_DEBUG
        std::cout << "c cl_touched was > 0, new iteration" << std::endl;
        #endif
        for (CSet::iterator it = cl_touched.begin(), end = cl_touched.end(); it != end; ++it) {
            if (it->clause != NULL)
                s1.add(*it);
        }
        cl_touched.clear();
        
        for (CSet::iterator it = s1.begin(), end = s1.end(); it != end; ++it) {
            if (numMaxSubsume1 == 0) break;
            if (it->clause != NULL) {
                subsume1(*it);
                numMaxSubsume1--;
                if (!solver.ok) return;
            }
        }
        s1.clear();
        
        if (!solver.ok) return;
        solver.ok = (solver.propagate() == NULL);
        if (!solver.ok) {
            printf("c (contradiction during subsumption)\n");
            unregisterIteration(s1);
            return;
        }
        solver.clauseCleaner->cleanClausesBewareNULL(clauses, ClauseCleaner::simpClauses, *this);
    }
    unregisterIteration(s1);
}

void Subsumer::smaller_database()
{
    #ifdef BIT_MORE_VERBOSITY
    std::cout << "c Smaller database" << std::endl;
    #endif
    //  Set used in 1-subs:
    //      (1) clauses containing a negated literal of an added clause.
    //      (2) all added or strengthened ("touched") clauses.
    //
    //  Set used in 0-subs:
    //      (1) clauses containing a (non-negated) literal of an added clause, including the added clause itself.
    //      (2) all strenghtened clauses -- REMOVED!! We turned on eager backward subsumption which supersedes this.
    
    #ifdef BIT_MORE_VERBOSITY
    printf("  PREPARING\n");
    #endif
    
    CSet s0, s1;     // 's0' is used for 0-subsumption, 's1' for 1-subsumption
    vec<char>   ol_seen(solver.nVars()*2, 0);
    for (CSet::iterator it = cl_added.begin(), end = cl_added.end(); it != end; ++it) {
        if (it->clause == NULL) continue;
        ClauseSimp& c = *it;
        Clause& cl = *it->clause;
        
        s1.add(c);
        for (uint32_t j = 0; j < cl.size(); j++){
            if (ol_seen[cl[j].toInt()]) continue;
            ol_seen[cl[j].toInt()] = 1;
            
            vec<ClauseSimp>& n_occs = occur[(~cl[j]).toInt()];
            for (uint32_t k = 0; k < n_occs.size(); k++)
                if (n_occs[k].clause != c.clause && n_occs[k].clause->size() <= cl.size() && selfSubset(n_occs[k].clause->getAbst(), c.clause->getAbst()) && selfSubset(*n_occs[k].clause, cl, seen_tmp))
                    s1.add(n_occs[k]);
                
            vec<ClauseSimp>& p_occs = occur[cl[j].toInt()];
            for (uint32_t k = 0; k < p_occs.size(); k++)
                if (subsetAbst(p_occs[k].clause->getAbst(), c.clause->getAbst()))
                    s0.add(p_occs[k]);
        }
    }
    cl_added.clear();
    
    registerIteration(s0);
    registerIteration(s1);
    
    #ifdef BIT_MORE_VERBOSITY
    printf("c FIXED-POINT\n");
    #endif
    
    // Fixed-point for 1-subsumption:
    while (s1.size() > 0 || cl_touched.size() > 0){
        for (CSet::iterator it = cl_touched.begin(), end = cl_touched.end(); it != end; ++it) {
            if (it->clause != NULL) {
                s1.add(*it);
                s0.add(*it);
            }
        }
        
        cl_touched.clear();
        assert(solver.qhead == solver.trail.size());
        
        #ifdef BIT_MORE_VERBOSITY
        printf("c s1.size()=%d  cl_touched.size()=%d\n", s1.size(), cl_touched.size());
        #endif
        
        for (CSet::iterator it = s1.begin(), end = s1.end(); it != end; ++it) {
            if (numMaxSubsume1 == 0) break;
            if (it->clause != NULL) {
                subsume1(*it);
                numMaxSubsume1--;
                if (!solver.ok) return;
            }
        }
        s1.clear();
        
        if (!solver.ok) return;
        solver.ok = (solver.propagate() == NULL);
        if (!solver.ok){
            printf("c (contradiction during subsumption)\n");
            unregisterIteration(s1);
            unregisterIteration(s0);
            return; 
        }
        solver.clauseCleaner->cleanClausesBewareNULL(clauses, ClauseCleaner::simpClauses, *this);
    }
    unregisterIteration(s1);
    
    // Iteration pass for 0-subsumption:
    for (CSet::iterator it = s0.begin(), end = s0.end(); it != end; ++it) {
        if (it->clause != NULL)
            subsume0(*it->clause);
    }
    s0.clear();
    unregisterIteration(s0);
}

ClauseSimp Subsumer::linkInClause(Clause& cl)
{
    ClauseSimp c(&cl, clauseID++);
    clauses.push(c);
    if (updateOccur(cl)) {
        for (uint32_t i = 0; i < cl.size(); i++) {
            occur[cl[i].toInt()].push(c);
            touch(cl[i].var());
        }
    }
    cl_added.add(c);
    
    return c;
}

void Subsumer::linkInAlreadyClause(ClauseSimp& c)
{
    Clause& cl = *c.clause;
    
    if (updateOccur(cl)) {
        for (uint32_t i = 0; i < cl.size(); i++) {
            occur[cl[i].toInt()].push(c);
            touch(cl[i].var());
        }
    }
}

void Subsumer::addFromSolver(vec<Clause*>& cs)
{
    Clause **i = cs.getData();
    Clause **j = i;
    for (Clause **end = i + cs.size(); i !=  end; i++) {
        if (i+1 != end)
            __builtin_prefetch(*(i+1), 1, 1);
        
        if ((*i)->learnt()) {
            *j++ = *i;
            (*i)->setUnsorted();
            continue;
        }
        ClauseSimp c(*i, clauseID++);
        clauses.push(c);
        Clause& cl = *c.clause;
        if (updateOccur(cl)) {
            for (uint32_t i = 0; i < cl.size(); i++) {
                occur[cl[i].toInt()].push(c);
                touch(cl[i].var());
            }
            if (fullSubsume || cl.getVarChanged()) cl_added.add(c);
            else if (cl.getStrenghtened()) cl_touched.add(c);
            
            if (cl.getVarChanged() || cl.getStrenghtened())
                cl.calcAbstraction();
        }
    }
    cs.shrink(i-j);
}

void Subsumer::addBackToSolver()
{
    #ifdef HYPER_DEBUG2
    uint32_t binaryLearntAdded = 0;
    #endif
    
    for (uint32_t i = 0; i < clauses.size(); i++) {
        if (clauses[i].clause != NULL) {
            assert(clauses[i].clause->size() > 1);
            if (clauses[i].clause->size() == 2) {
                #ifdef HYPER_DEBUG2
                if (clauses[i].clause->learnt())
                    binaryLearntAdded++;
                #endif
                solver.binaryClauses.push(clauses[i].clause);
            } else {
                if (clauses[i].clause->learnt())
                    solver.learnts.push(clauses[i].clause);
                else
                    solver.clauses.push(clauses[i].clause);
            }
        }
    }
    
    #ifdef HYPER_DEBUG2
    std::cout << "Binary learnt added:" << binaryLearntAdded << std::endl;
    #endif
}

void Subsumer::removeWrong(vec<Clause*>& cs)
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
                free(&c);
                break;
            }
        }
        if (!remove)
            *j++ = *i;
    }
    cs.shrink(i-j);
}

void Subsumer::fillCannotEliminate()
{
    std::fill(cannot_eliminate.getData(), cannot_eliminate.getDataEnd(), false);
    for (uint32_t i = 0; i < solver.xorclauses.size(); i++) {
        const XorClause& c = *solver.xorclauses[i];
        for (uint32_t i2 = 0; i2 < c.size(); i2++)
            cannot_eliminate[c[i2].var()] = true;
    }
    
    const vec<bool>& tmp2 = solver.conglomerate->getRemovedVars();
    for (uint32_t i = 0; i < tmp2.size(); i++) {
        if (tmp2[i]) cannot_eliminate[i] = true;
    }
    
    const vec<Clause*>& tmp = solver.varReplacer->getClauses();
    for (uint32_t i = 0; i < tmp.size(); i++) {
        const Clause& c = *tmp[i];
        for (uint32_t i2 = 0; i2 < c.size(); i2++)
            cannot_eliminate[c[i2].var()] = true;
    }
    
    #ifdef VERBOSE_DEBUG
    uint32_t tmpNum = 0;
    for (uint32_t i = 0; i < cannot_eliminate.size(); i++)
        if (cannot_eliminate[i])
            tmpNum++;
    std::cout << "Cannot eliminate num:" << tmpNum << std::endl;
    #endif
}

void Subsumer::subsume0LearntSet(vec<Clause*>& cs)
{
    Clause** a = cs.getData();
    Clause** b = a;
    for (Clause** end = a + cs.size(); a != end; a++) {
        if (numMaxSubsume0 > 0 && !(*a)->subsume0IsFinished()) {
            numMaxSubsume0--;
            uint32_t index = subsume0(**a, calcAbstraction(**a));
            if (index != std::numeric_limits<uint32_t>::max()) {
                (*a)->makeNonLearnt();
                clauses[index].clause = *a;
                linkInAlreadyClause(clauses[index]);
                solver.learnts_literals -= (*a)->size();
                solver.clauses_literals += (*a)->size();
                cl_added.add(clauses[index]);
                continue;
            }
            if (numMaxSubsume1 > 0 &&
                (((*a)->size() == 2 && clauses.size() < 3500000) ||
                ((*a)->size() <= 3 && clauses.size() < 300000) ||
                ((*a)->size() <= 4 && clauses.size() < 60000))) {
                ClauseSimp c(*a, clauseID++);
                (*a)->calcAbstraction();
                clauses.push(c);
                subsume1(c);
                numMaxSubsume1--;
                if (!solver.ok) {
                    for (; a != end; a++)
                        *b++ = *a;
                    cs.shrink(a-b);
                    return;
                }
                assert(clauses[c.index].clause != NULL);
                clauses.pop();
                clauseID--;
            }
        }
        *b++  = *a;
    }
    cs.shrink(a-b);
}

const bool Subsumer::treatLearnts()
{
    subsume0LearntSet(solver.learnts);
    if (!solver.ok) return false;
    subsume0LearntSet(solver.binaryClauses);
    if (!solver.ok) return false;
    solver.ok = (solver.propagate() == NULL);
    if (!solver.ok){
        printf("c (contradiction during subsumption)\n");
        return false;
    }
    solver.clauseCleaner->cleanClausesBewareNULL(clauses, ClauseCleaner::simpClauses, *this);
    return true;
}

const bool Subsumer::simplifyBySubsumption()
{
    if (solver.nClauses() > 20000000)  return true;
    
    double myTime = cpuTime();
    uint32_t origTrailSize = solver.trail.size();
    clauses_subsumed = 0;
    literals_removed = 0;
    numblockedClauseRemoved = 0;
    numCalls++;
    clauseID = 0;
    numVarsElimed = 0;
    blockTime = 0.0;
    
    //if (solver.xorclauses.size() < 30000 && solver.clauses.size() < MAX_CLAUSENUM_XORFIND/10) addAllXorAsNorm();
    
    //For VE
    touched_list.clear();
    touched.clear();
    touched.growTo(solver.nVars(), false);
    for (Var var = 0; var < solver.nVars(); var++) {
        if (solver.decision_var[var] && solver.assigns[var] == l_Undef) touch(var);
        occur[2*var].clear();
        occur[2*var+1].clear();
    }
    
    if (solver.performReplace && !solver.varReplacer->performReplace(true))
        return false;
    fillCannotEliminate();
    
    clauses.clear();
    cl_added.clear();
    cl_touched.clear();
    
    clauses.reserve(solver.clauses.size() + solver.binaryClauses.size());
    cl_added.reserve(solver.clauses.size() + solver.binaryClauses.size());
    cl_touched.reserve(solver.clauses.size() + solver.binaryClauses.size());
    
    if (clauses.size() < 200000)
        fullSubsume = true;
    else
        fullSubsume = false;
    
    solver.clauseCleaner->cleanClauses(solver.clauses, ClauseCleaner::clauses);
    addFromSolver(solver.clauses);
    solver.clauseCleaner->cleanClauses(solver.binaryClauses, ClauseCleaner::binaryClauses);
    addFromSolver(solver.binaryClauses);
    
    //Limits
    if (clauses.size() > 3500000)
        numMaxSubsume0 = 900000 * (1+numCalls/2);
    else
        numMaxSubsume0 = 2000000 * (1+numCalls/2);
    
    if (solver.doSubsume1) {
        if (clauses.size() > 3500000)
            numMaxSubsume1 = 100000 * (1+numCalls/2);
        else
            numMaxSubsume1 = 500000 * (1+numCalls/2);
    } else {
        numMaxSubsume1 = 0;
    }
    
    if (clauses.size() > 3500000)
        numMaxElim = (uint32_t)((double)solver.order_heap.size() / 5.0 * (0.8+(double)(numCalls)/4.0));
    else
        numMaxElim = (uint32_t)((double)solver.order_heap.size() / 2.0 * (0.8+(double)(numCalls)/4.0));
    
    if (clauses.size() > 3500000)
        numMaxBlockToVisit = (int64_t)(30000.0 * (0.8+(double)(numCalls)/3.0));
    else
        numMaxBlockToVisit = (int64_t)(50000.0 * (0.8+(double)(numCalls)/3.0));
    
    if (solver.order_heap.size() > 200000)
        numMaxBlockVars = (uint32_t)((double)solver.order_heap.size() / 3.5 * (0.8+(double)(numCalls)/4.0));
    else
        numMaxBlockVars = (uint32_t)((double)solver.order_heap.size() / 1.5 * (0.8+(double)(numCalls)/4.0));
    
    //For debugging post-c32s-gcdm16-22.cnf --- an instance that is turned SAT to UNSAT if a bug is in the code
    //numMaxBlockToVisit = std::numeric_limits<int64_t>::max();
    //numMaxElim = std::numeric_limits<uint32_t>::max();
    //numMaxSubsume0 = std::numeric_limits<uint32_t>::max();
    //numMaxSubsume1 = std::numeric_limits<uint32_t>::max();
    //numMaxBlockVars = std::numeric_limits<uint32_t>::max();
    
    #ifdef BIT_MORE_VERBOSITY
    std::cout << "c  num clauses:" << clauses.size() << std::endl;
    std::cout << "c  time to link in:" << cpuTime()-myTime << std::endl;
    #endif
    
    for (uint32_t i = 0; i < clauses.size(); i++) {
        if (numMaxSubsume0 == 0) break;
        if (clauses[i].clause != NULL && 
            (fullSubsume
            || !clauses[i].clause->subsume0IsFinished())
            ) {
            subsume0(*clauses[i].clause);
            numMaxSubsume0--;
        }
    }
    
    origNClauses = clauses.size();
    uint32_t origNLearnts = solver.learnts.size();
    
    if (!treatLearnts()) return false;
    
    #ifdef BIT_MORE_VERBOSITY
    std::cout << "c  time until pre-subsume0 clauses and subsume1 2-learnts:" << cpuTime()-myTime << std::endl;
    #endif
    
    if (!solver.ok) return false;
    #ifdef VERBOSE_DEBUG
    std::cout << "c   pre-subsumed:" << clauses_subsumed << std::endl;
    std::cout << "c   cl_added:" << cl_added.size() << std::endl;
    std::cout << "c   cl_touched:" << cl_touched.size() << std::endl;
    std::cout << "c   clauses:" << clauses.size() << std::endl;
    std::cout << "c   origNClauses:" << origNClauses << std::endl;
    #endif
    
    if (clauses.size() > 10000000)  goto endSimplifyBySubsumption;
    if (solver.doBlockedClause && numCalls % 3 == 1) blockedClauseRemoval();
    do{
        #ifdef BIT_MORE_VERBOSITY
        std::cout << "c time before the start of almost_all/smaller: " << cpuTime() - myTime << std::endl;
        #endif
        if (numMaxSubsume0 > 0) {
            if (cl_added.size() > origNClauses / 2) {
                almost_all_database();
                if (!solver.ok) return false;
            } else {
                smaller_database();
                if (!solver.ok) return false;
            }
        }
        cl_added.clear();
        assert(cl_added.size() == 0);
        assert(solver.ok);
        solver.ok = (solver.propagate() == NULL);
        if (!solver.ok) {
            printf("c (contradiction during subsumption)\n");
            return false;
        }
        solver.clauseCleaner->cleanClausesBewareNULL(clauses, ClauseCleaner::simpClauses, *this);
        
        #ifdef BIT_MORE_VERBOSITY
        std::cout << "c time until the end of almost_all/smaller: " << cpuTime() - myTime << std::endl;
        #endif
        
        if (!solver.doVarElim) break;
        
        #ifdef BIT_MORE_VERBOSITY
        printf("c VARIABLE ELIMINIATION\n");
        std::cout << "c  toucheds list size:" << touched_list.size() << std::endl;
        #endif
        vec<Var> init_order;
        orderVarsForElim(init_order);   // (will untouch all variables)
        
        for (bool first = true; numMaxElim > 0; first = false){
            uint32_t vars_elimed = 0;
            vec<Var> order;
            
            if (first) {
                //init_order.copyTo(order);
                for (uint32_t i = 0; i < init_order.size(); i++) {
                    const Var var = init_order[i];
                    if (!cannot_eliminate[var] && solver.decision_var[var])
                        order.push(var);
                }
            } else {
                for (uint32_t i = 0; i < touched_list.size(); i++) {
                    const Var var = touched_list[i];
                    if (!cannot_eliminate[var] && solver.decision_var[var])
                        order.push(var);
                    touched[var] = false;
                }
                touched_list.clear();
            }
            #ifdef VERBOSE_DEBUG
            std::cout << "Order size:" << order.size() << std::endl;
            #endif
            
            assert(solver.qhead == solver.trail.size());
            for (uint32_t i = 0; i < order.size() && numMaxElim > 0; i++, numMaxElim--) {
                if (maybeEliminate(order[i])) {
                    if (!solver.ok) {
                        printf("c (contradiction during subsumption)\n");
                        return false;
                    }
                    vars_elimed++;
                }
            }
            assert(solver.qhead == solver.trail.size());
            
            if (vars_elimed == 0) break;
            
            numVarsElimed += vars_elimed;
            #ifdef BIT_MORE_VERBOSITY
            printf("c  #var-elim: %d\n", vars_elimed);
            std::cout << "c time until the end of varelim: " << cpuTime() - myTime << std::endl;
            #endif
        }
    }while (cl_added.size() > 100 && numMaxElim > 0);
    endSimplifyBySubsumption:
    
    if (!solver.ok) return false;
    solver.ok = (solver.propagate() == NULL);
    if (!solver.ok) {
        printf("c (contradiction during subsumption)\n");
        return false;
    }
    
    #ifndef NDEBUG
    verifyIntegrity();
    #endif
    
    //vector<char> var_merged = merge();
    removeWrong(solver.learnts);
    removeWrong(solver.binaryClauses);
    
    solver.clauseCleaner->cleanClausesBewareNULL(clauses, ClauseCleaner::simpClauses, *this);
//     if (solver.doHyperBinRes && clauses.size() < 1000000 && numCalls > 1 && !hyperBinRes())
//         return false;
//     
//     solver.ok = (solver.propagate() == NULL);
//     if (!solver.ok) return false;
//     solver.clauseCleaner->cleanClausesBewareNULL(clauses, ClauseCleaner::simpClauses, *this);
    
    solver.order_heap.filter(Solver::VarFilter(solver));
    
    addBackToSolver();
    solver.nbCompensateSubsumer += origNLearnts-solver.learnts.size();
    
    if (solver.verbosity >= 1) {
        std::cout << "c |  lits-rem: " << std::setw(9) << literals_removed
        << "  cl-subs: " << std::setw(8) << clauses_subsumed
        << "  v-elim: " << std::setw(6) << numVarsElimed
        << "  v-fix: " << std::setw(4) <<solver.trail.size() - origTrailSize
        << "  time: " << std::setprecision(2) << std::setw(5) << (cpuTime() - myTime) << " s"
        //<< " blkClRem: " << std::setw(5) << numblockedClauseRemoved
        << "   |" << std::endl;
        
        if (numblockedClauseRemoved > 0 || blockTime > 0.0) {
            std::cout 
            << "c |  Blocked clauses removed: " << std::setw(8) << numblockedClauseRemoved 
            << "    Time: " << std::fixed << std::setprecision(2) << std::setw(4) << blockTime << " s" 
            << "                                    |" << std::endl;
        }
    }
    
    return true;
}

void Subsumer::findSubsumed(Clause& ps, vec<ClauseSimp>& out_subsumed)
{
    #ifdef VERBOSE_DEBUG
    cout << "findSubsumed: ";
    for (uint32_t i = 0; i < ps.size(); i++) {
        if (ps[i].sign()) printf("-");
        printf("%d ", ps[i].var() + 1);
    }
    printf("0\n");
    #endif
    
    int min_i = 0;
    for (uint32_t i = 1; i < ps.size(); i++){
        if (occur[ps[i].toInt()].size() < occur[ps[min_i].toInt()].size())
            min_i = i;
    }
    
    vec<ClauseSimp>& cs = occur[ps[min_i].toInt()];
    for (ClauseSimp *it = cs.getData(), *end = it + cs.size(); it != end; it++){
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

void Subsumer::findSubsumed(Clause& ps, uint32_t abs, vec<ClauseSimp>& out_subsumed)
{
    #ifdef VERBOSE_DEBUG
    cout << "findSubsumed: ";
    for (uint32_t i = 0; i < ps.size(); i++) {
        if (ps[i].sign()) printf("-");
        printf("%d ", ps[i].var() + 1);
    }
    printf("0\n");
    #endif
    
    int min_i = 0;
    for (uint32_t i = 1; i < ps.size(); i++){
        if (occur[ps[i].toInt()].size() < occur[ps[min_i].toInt()].size())
            min_i = i;
    }
    
    vec<ClauseSimp>& cs = occur[ps[min_i].toInt()];
    for (ClauseSimp *it = cs.getData(), *end = it + cs.size(); it != end; it++){
        if (it+1 != end)
            __builtin_prefetch((it+1)->clause, 1, 1);
        
        if (it->clause != &ps && subsetAbst(abs, it->clause->getAbst()) && ps.size() <= it->clause->size() && subset(ps, *it->clause)) {
            out_subsumed.push(*it);
            #ifdef VERBOSE_DEBUG
            cout << "subsumed: ";
            it->clause->plainPrint();
            #endif
        }
    }
}

void Subsumer::findSubsumed(const vec<Lit>& ps, const uint32_t abst, vec<ClauseSimp>& out_subsumed)
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
        if (occur[ps[i].toInt()].size() < occur[ps[min_i].toInt()].size())
            min_i = i;
    }
    
    vec<ClauseSimp>& cs = occur[ps[min_i].toInt()];
    for (ClauseSimp *it = cs.getData(), *end = it + cs.size(); it != end; it++){
        if (it+1 != end)
            __builtin_prefetch((it+1)->clause, 1, 1);
        
        if (subsetAbst(abst, it->clause->getAbst()) && ps.size() <= it->clause->size() && subset(ps, *it->clause)) {
            out_subsumed.push(*it);
            #ifdef VERBOSE_DEBUG
            cout << "subsumed: ";
            it->clause->plainPrint();
            #endif
        }
    }
}

void inline Subsumer::MigrateToPsNs(vec<ClauseSimp>& poss, vec<ClauseSimp>& negs, vec<ClauseSimp>& ps, vec<ClauseSimp>& ns, const Var x)
{
    poss.moveTo(ps);
    negs.moveTo(ns);
    
    for (uint32_t i = 0; i < ps.size(); i++)
        unlinkClause(ps[i], x);
    for (uint32_t i = 0; i < ns.size(); i++)
        unlinkClause(ns[i], x);
}

void inline Subsumer::DeallocPsNs(vec<ClauseSimp>& ps, vec<ClauseSimp>& ns)
{
    for (uint32_t i = 0; i < ps.size(); i++) {
        clauses[ps[i].index].clause = NULL;
        free(ps[i].clause);
    }
    for (uint32_t i = 0; i < ns.size(); i++) {
        clauses[ns[i].index].clause = NULL;
        free(ns[i].clause);
    }
}

// Returns TRUE if variable was eliminated.
bool Subsumer::maybeEliminate(const Var x)
{
    assert(solver.qhead == solver.trail.size());
    assert(!var_elimed[x]);
    assert(!cannot_eliminate[x]);
    assert(solver.decision_var[x]);
    if (solver.value(x) != l_Undef) return false;
    if (occur[Lit(x, false).toInt()].size() == 0 && occur[Lit(x, true).toInt()].size() == 0)
        return false;
    
    vec<ClauseSimp>&   poss = occur[Lit(x, false).toInt()];
    vec<ClauseSimp>&   negs = occur[Lit(x, true).toInt()];
    
    // Heuristic CUT OFF:
    if (poss.size() >= 10 && negs.size() >= 10)
        return false;
    
    // Count clauses/literals before elimination:
    int before_clauses  = poss.size() + negs.size();
    uint32_t before_literals = 0;
    for (uint32_t i = 0; i < poss.size(); i++) before_literals += poss[i].clause->size();
    for (uint32_t i = 0; i < negs.size(); i++) before_literals += negs[i].clause->size();
    
    // Heuristic CUT OFF2:
    if ((poss.size() >= 3 && negs.size() >= 3 && before_literals > 300)
        && clauses.size() > 1500000)
        return false;
    if ((poss.size() >= 5 && negs.size() >= 5 && before_literals > 400)
        && clauses.size() <= 1500000 && clauses.size() > 200000)
        return false;
    if ((poss.size() >= 8 && negs.size() >= 8 && before_literals > 700)
        && clauses.size() <= 200000)
        return false;
    
    // Count clauses/literals after elimination:
    int after_clauses  = 0;
    vec<Lit>  dummy;
    for (uint32_t i = 0; i < poss.size(); i++) for (uint32_t j = 0; j < negs.size(); j++){
        // Merge clauses. If 'y' and '~y' exist, clause will not be created.
        dummy.clear();
        bool ok = merge(*poss[i].clause, *negs[j].clause, Lit(x, false), Lit(x, true), dummy);
        if (ok){
            after_clauses++;
            if (after_clauses > before_clauses) goto Abort;
        }
    }
    Abort:;
    
    // Maybe eliminate:
    if (after_clauses  <= before_clauses) {
        vec<ClauseSimp> ps, ns;
        MigrateToPsNs(poss, negs, ps, ns, x);
        for (uint32_t i = 0; i < ps.size(); i++) for (uint32_t j = 0; j < ns.size(); j++){
            dummy.clear();
            bool ok = merge(*ps[i].clause, *ns[j].clause, Lit(x, false), Lit(x, true), dummy);
            if (ok){
                Clause* cl = solver.addClauseInt(dummy, 0);
                if (cl != NULL) {
                    ClauseSimp c = linkInClause(*cl);
                    subsume0(*cl);
                }
                if (!solver.ok) return true;
            }
        }
        DeallocPsNs(ps, ns);
        goto Eliminated;
    }
    
    return false;
    
    Eliminated:
    assert(occur[Lit(x, false).toInt()].size() + occur[Lit(x, true).toInt()].size() == 0);
    var_elimed[x] = 1;
    numElimed++;
    solver.setDecisionVar(x, false);
    return true;
}

// Returns FALSE if clause is always satisfied ('out_clause' should not be used). 'seen' is assumed to be cleared.
bool Subsumer::merge(Clause& ps, Clause& qs, Lit without_p, Lit without_q, vec<Lit>& out_clause)
{
    for (uint32_t i = 0; i < ps.size(); i++){
        if (ps[i] != without_p){
            seen_tmp[ps[i].toInt()] = 1;
            out_clause.push(ps[i]);
        }
    }
    
    for (uint32_t i = 0; i < qs.size(); i++){
        if (qs[i] != without_q){
            if (seen_tmp[(~qs[i]).toInt()]){
                for (uint32_t i = 0; i < ps.size(); i++)
                    seen_tmp[ps[i].toInt()] = 0;
                return false;
            }
            if (!seen_tmp[qs[i].toInt()])
                out_clause.push(qs[i]);
        }
    }
    
    for (uint32_t i = 0; i < ps.size(); i++)
        seen_tmp[ps[i].toInt()] = 0;
    
    return true;
}

struct myComp {
    bool operator () (const pair<int, Var>& x, const pair<int, Var>& y) {
        return x.first < y.first ||
            (!(y.first < x.first) && x.second < y.second);
    }
};
    
// Side-effect: Will untouch all variables.
void Subsumer::orderVarsForElim(vec<Var>& order)
{
    order.clear();
    vec<pair<int, Var> > cost_var;
    for (uint32_t i = 0; i < touched_list.size(); i++){
        Var x = touched_list[i];
        touched[x] = 0;
        cost_var.push(std::make_pair( occur[Lit(x, false).toInt()].size() * occur[Lit(x, true).toInt()].size() , x ));
    }
    
    touched_list.clear();
    std::sort(cost_var.getData(), cost_var.getData()+cost_var.size(), myComp());
    
    for (uint32_t x = 0; x < cost_var.size(); x++) {
        if (cost_var[x].first != 0)
            order.push(cost_var[x].second);
    }
}

const bool Subsumer::hyperUtility(vec<ClauseSimp>& iter, const Lit lit, BitArray& inside, vec<ClauseSimp>& addToClauses, uint32_t& hyperBinAdded, uint32_t& hyperBinUnitary)
{
    for (ClauseSimp *it = iter.getData(), *end = it + iter.size() ; it != end; it++) {
        if (it->clause == NULL) continue;
        uint32_t notIn = 0;
        Lit notInLit = Lit(0,false);
        
        Clause& cl2 = *it->clause;
        for (uint32_t i = 0; i < cl2.size(); i++) {
            if (cl2[i].var() == lit.var()) {
                notIn = 2;
                break;
            }
            if (!inside[cl2[i].toInt()]) {
                notIn++;
                notInLit = cl2[i];
            }
            if (notIn > 1) break;
        }
        
        if (notIn == 0) {
            if (solver.assigns[lit.var()] == l_Undef) {
                solver.uncheckedEnqueue(lit);
                solver.ok = (solver.propagate() == NULL);
                if (!solver.ok) return false;
                hyperBinUnitary++;
            } else if (solver.assigns[lit.var()] != boolToLBool(!lit.sign())) {
                solver.ok = false;
                return false;
            }
        }
        
        if (notIn == 1 && !inside[(~notInLit).toInt()]) {
            vec<Lit> cs(2);
            cs[0] = lit;
            cs[1] = notInLit;
            Clause *cl3 = Clause_new(cs, 0);
            uint32_t index = subsume0(*cl3);
            if (index != std::numeric_limits<uint32_t>::max()) {
                ClauseSimp c(cl3, index);
                addToClauses.push(c);
                inside.setBit((~notInLit).toInt());
                #ifdef HYPER_DEBUG
                std::cout << "HyperBinRes adding clause: ";
                cl3->plainPrint();
                #endif
                hyperBinAdded++;
            }
        }
    }
    
    return true;
}

const bool Subsumer::hyperBinRes()
{
    double myTime = cpuTime();
    
    BitArray inside;
    inside.resize(solver.nVars()*2, 0);
    uint32_t hyperBinAdded = 0;
    uint32_t hyperBinUnitary = 0;
    vec<ClauseSimp> addToClauses;
    uint64_t totalClausesChecked = 0;

    vec<Var> varsToCheck;
    
    if (clauses.size() > 100000 || solver.order_heap.size() > 30000) {
        Heap<Solver::VarOrderLt> tmp(solver.order_heap);
        uint32_t thisTopX = std::min(tmp.size(), 1000U);
        for (uint32_t i = 0; i != thisTopX; i++)
            varsToCheck.push(tmp.removeMin());
    } else {
        for (Var i = 0; i < solver.nVars(); i++)
            varsToCheck.push(i);
    }
    
    for (Var test = 0; test < 2*varsToCheck.size(); test++) if (solver.assigns[test/2] == l_Undef && solver.decision_var[test/2]) {
        if (totalClausesChecked > 5000000)
            break;
        
        inside.setZero();
        Lit lit(varsToCheck[test/2], test&1);
        #ifdef HYPER_DEBUG
        std::cout << "Resolving with literal:" << (lit.sign() ? "-" : "") << lit.var()+1 << std::endl;
        #endif
        
        vec<Lit> addedToInside;
        vec<ClauseSimp>& set = occur[lit.toInt()];
        for (ClauseSimp *it = set.getData(), *end = it + set.size() ; it != end; it++) {
            if (it->clause == NULL) continue;
            Clause& cl2 = *it->clause;
            if (cl2.size() > 2) continue;
            assert(cl2[0] == lit || cl2[1] == lit);
            if (cl2[0] == lit) {
                inside.setBit((~cl2[1]).toInt());
                addedToInside.push(~cl2[1]);
            } else {
                inside.setBit((~cl2[0]).toInt());
                addedToInside.push(~cl2[0]);
            }
        }
        
        uint32_t sum = 0;
        for (uint32_t add = 0; add < addedToInside.size(); add++) {
            sum += occur[addedToInside[add].toInt()].size();
        }
        
        if (sum < clauses.size()) {
            totalClausesChecked += sum;
            for (uint32_t add = 0; add < addedToInside.size(); add++) {
                vec<ClauseSimp>& iter = occur[addedToInside[add].toInt()];
                if (!hyperUtility(iter, lit, inside, addToClauses, hyperBinAdded, hyperBinUnitary))
                    return false;
            }
        } else {
            totalClausesChecked += clauses.size();
            if (!hyperUtility(clauses, lit, inside, addToClauses, hyperBinAdded, hyperBinUnitary))
                return false;
        }
        
        for (uint32_t i = 0; i < addToClauses.size(); i++) {
            Clause *c = solver.addClauseInt(*addToClauses[i].clause, 0);
            free(addToClauses[i].clause);
            if (c != NULL) {
                ClauseSimp cc(c, addToClauses[i].index);
                clauses[cc.index] = cc;
                linkInAlreadyClause(cc);
                solver.becameBinary++; //since this binary did not exist, and now it exists.
                subsume1(cc);
            }
            if (!solver.ok) return false;
        }
        addToClauses.clear();
    }
    
    if (solver.verbosity >= 1) {
        std::cout << "c |  Hyper-binary res binary added: " << std::setw(5) << hyperBinAdded 
        << " unitaries: " << std::setw(5) << hyperBinUnitary 
        << " time: " << std::setprecision(2) << std::setw(5)<< cpuTime() - myTime << " s" 
        << "                  |"  << std::endl;
    }
    
    return true;
}

class varDataStruct
{
    public:
        varDataStruct() : 
            numPosClauses (0)
            , numNegClauses (0)
            , sumPosClauseSize (0)
            , sumNegClauseSize (0)
            , posHash(0)
            , negHash(0)
        {}
        const bool operator < (const varDataStruct& other) const
        {
            if (numPosClauses < other.numPosClauses) return true;
            if (numPosClauses > other.numPosClauses) return false;
            
            if (numNegClauses < other.numNegClauses)  return true;
            if (numNegClauses > other.numNegClauses)  return false;
            
            if (sumPosClauseSize < other.sumPosClauseSize) return true;
            if (sumPosClauseSize > other.sumPosClauseSize) return false;
            
            if (sumNegClauseSize < other.sumNegClauseSize) return true;
            if (sumNegClauseSize > other.sumNegClauseSize) return false;
            
            if (posHash < other.posHash) return true;
            if (posHash > other.posHash) return false;
            
            if (negHash < other.negHash) return true;
            if (negHash > other.negHash) return false;
            
            return false;
        }
        
        const bool operator == (const varDataStruct& other) const
        {
            if (numPosClauses == other.numPosClauses &&
                numNegClauses == other.numNegClauses &&
                sumPosClauseSize == other.sumPosClauseSize &&
                sumNegClauseSize == other.sumNegClauseSize &&
                posHash == other.posHash &&
                negHash == other.negHash)
                return true;
            
            return false;
        }
        
        void reversePolarity()
        {
            std::swap(numPosClauses, numNegClauses);
            std::swap(sumPosClauseSize, sumNegClauseSize);
            std::swap(posHash, negHash);
        }
        
        uint32_t numPosClauses;
        uint32_t numNegClauses;
        uint32_t sumPosClauseSize;
        uint32_t sumNegClauseSize;
        int posHash;
        int negHash;
};

int hash32shift(int key)
{
    key = ~key + (key << 15); // key = (key << 15) - key - 1;
    key = key ^ (key >> 12);
    key = key + (key << 2);
    key = key ^ (key >> 4);
    key = key * 2057; // key = (key + (key << 3)) + (key << 11);
    key = key ^ (key >> 16);
    return key;
}

void Subsumer::verifyIntegrity()
{
    vector<uint> occurNum(solver.nVars()*2, 0);
    
    for (uint32_t i = 0; i < clauses.size(); i++) {
        if (clauses[i].clause == NULL) continue;
        Clause& c = *clauses[i].clause;
        for (uint32_t i2 = 0; i2 < c.size(); i2++)
            occurNum[c[i2].toInt()]++;
    }
    
    for (uint32_t i = 0; i < occurNum.size(); i++) {
        assert(occurNum[i] == occur[i].size());
    }
}

const bool Subsumer::allTautology(const vec<Lit>& ps, const Lit lit)
{
    #ifdef VERBOSE_DEBUG
    cout << "allTautology: ";
    for (uint32_t i = 0; i < ps.size(); i++) {
        if (ps[i].sign()) printf("-");
        printf("%d ", ps[i].var() + 1);
    }
    printf("0\n");
    #endif
    
    vec<ClauseSimp>& cs = occur[lit.toInt()];
    if (cs.size() == 0) return true;
    
    for (const Lit *l = ps.getData(), *end = ps.getDataEnd(); l != end; l++) {
        seen_tmp[l->toInt()] = true;
    }
    
    bool allIsTautology = true;
    for (ClauseSimp *it = cs.getData(), *end = cs.getDataEnd(); it != end; it++){
        if (it+1 != end)
            __builtin_prefetch((it+1)->clause, 1, 1);
        
        Clause& c = *it->clause;
        for (Lit *l = c.getData(), *end2 = l + c.size(); l != end2; l++) {
            if (*l != lit && seen_tmp[(~(*l)).toInt()]) {
                goto next;
            }
        }
        allIsTautology = false;
        break;
        
        next:;
    }
    
    for (const Lit *l = ps.getData(), *end = ps.getDataEnd(); l != end; l++) {
        seen_tmp[l->toInt()] = false;
    }
    
    return allIsTautology;
}

void Subsumer::blockedClauseRemoval()
{
    if (numMaxBlockToVisit < 0) return;
    if (solver.order_heap.size() < 1) return;
    double myTime = cpuTime();
    vec<ClauseSimp> toRemove;
    
    touchedBlockedVars = priority_queue<VarOcc, vector<VarOcc>, MyComp>();
    touchedBlockedVarsBool.clear();
    touchedBlockedVarsBool.growTo(solver.nVars(), false);
    for (uint32_t i =  0; i < solver.order_heap.size() && i < numMaxBlockVars; i++) {
        touchBlockedVar(solver.order_heap[solver.mtrand.randInt(solver.order_heap.size()-1)]);
    }
    
    while (touchedBlockedVars.size() > 100  && numMaxBlockToVisit > 0) {
        VarOcc vo = touchedBlockedVars.top();
        touchedBlockedVars.pop();
        
        if (solver.assigns[vo.var] != l_Undef || !solver.decision_var[vo.var] || cannot_eliminate[vo.var])
            continue;
        touchedBlockedVarsBool[vo.var] = false;
        Lit lit = Lit(vo.var, false);
        Lit negLit = Lit(vo.var, true);
        
        numMaxBlockToVisit -= (int64_t)occur[lit.toInt()].size();
        numMaxBlockToVisit -= (int64_t)occur[negLit.toInt()].size();
        //if (!tryOneSetting(lit, negLit)) {
            tryOneSetting(negLit, lit);
       // }
    }
    blockTime += cpuTime() - myTime;
    
    #ifdef BIT_MORE_VERBOSITY
    std::cout << "c  Total fime for block until now: " << blockTime << std::endl;
    #endif
}

const bool Subsumer::tryOneSetting(const Lit lit, const Lit negLit)
{
    uint32_t toRemove = 0;
    bool returnVal = false;
    vec<Lit> cl;
    
    for(ClauseSimp *it = occur[lit.toInt()].getData(), *end = occur[lit.toInt()].getDataEnd(); it != end; it++) {
        cl.clear();
        cl.growTo(it->clause->size()-1);
        for (uint32_t i = 0, i2 = 0; i < it->clause->size(); i++) {
            if ((*it->clause)[i] != lit) {
                cl[i2] = (*it->clause)[i];
                i2++;
            }
        }
        
        if (allTautology(cl, negLit)) {
            toRemove++;
        } else {
            break;
        }
    }
    
    if (toRemove == occur[lit.toInt()].size()) {
        var_elimed[lit.var()] = true;
        solver.setDecisionVar(lit.var(), false);
        vec<ClauseSimp> toRemove(occur[lit.toInt()]);
        for (ClauseSimp *it = toRemove.getData(), *end = toRemove.getDataEnd(); it != end; it++) {
            unlinkClause(*it, lit.var());
            numblockedClauseRemoved++;
        }
        
        vec<ClauseSimp> toRemove2(occur[negLit.toInt()]);
        for (ClauseSimp *it = toRemove2.getData(), *end = toRemove2.getDataEnd(); it != end; it++) {
            unlinkClause(*it, lit.var());
            numblockedClauseRemoved++;
        }
        returnVal = true;
    }
    
    return returnVal;
}

/*vector<char> Subsumer::merge()
{
    vector<char> var_merged(solver.nVars(), false);
    double myTime = cpuTime();
    
    vector<varDataStruct> varData(solver.nVars());
    
    for (Var var = 0; var < solver.nVars(); var++) if (solver.decision_var[var] && solver.assigns[var] == l_Undef && !cannot_eliminate[var]) {
        varDataStruct thisVar;
        
        vec<ClauseSimp>& toCountPos = occur[Lit(var, false).toInt()];
        thisVar.numPosClauses = toCountPos.size();
        for (uint32_t i2 = 0; i2 < toCountPos.size(); i2++) {
            thisVar.sumPosClauseSize += toCountPos[i2].clause->size();
            for (Lit *l = toCountPos[i2].clause->getData(), *end = l + toCountPos[i2].clause->size(); l != end; l++) {
                if (l->var() != var) thisVar.posHash ^= hash32shift(l->toInt());
            }
        }
        
        vec<ClauseSimp>& toCountNeg = occur[Lit(var, true).toInt()];
        thisVar.numNegClauses = toCountNeg.size();
        for (uint32_t i2 = 0; i2 < toCountNeg.size(); i2++) {
            thisVar.sumNegClauseSize += toCountNeg[i2].clause->size();
            for (Lit *l = toCountNeg[i2].clause->getData(), *end = l + toCountNeg[i2].clause->size(); l != end; l++) {
                if (l->var() != var) thisVar.negHash ^= hash32shift(l->toInt());
            }
        }
        
        varData[var] = thisVar;
    }
    
    map<varDataStruct, vector<Var> > dataToVar;
    for (Var var = 0; var < solver.nVars(); var++) if (solver.decision_var[var] && solver.assigns[var] == l_Undef) {
        dataToVar[varData[var]].push_back(var);
        assert(dataToVar[varData[var]].size() > 0);
    }
    
    for (map<varDataStruct, vector<Var> >::iterator it = dataToVar.begin(); it != dataToVar.end(); it++) {
        //std::cout << "size: " << it->second.size() << std::endl;
        for (uint i = 0; i < it->second.size()-1; i++) {
            assert(it->second.size() > i+1);
            assert(varData[it->second[i]] == varData[it->second[i+1]]);
        }
    }
    
    uint64_t checked = 0;
    uint64_t replaced = 0;
    for (Var var = 0; var < solver.nVars(); var++) if (solver.decision_var[var] && solver.assigns[var] == l_Undef && !cannot_eliminate[var]) {
        varDataStruct tmp = varData[var];
        assert(dataToVar[tmp].size() > 0);
        
        if (dataToVar[tmp].size() > 0) {
            map<varDataStruct, vector<Var> >::iterator it = dataToVar.find(tmp);
            for (uint i = 0; i < it->second.size(); i++) if (it->second[i] != var) {
                checked ++;
                if (checkIfSame(Lit(var, false), Lit(it->second[i], false)) &&
                    (checkIfSame(Lit(var, true), Lit(it->second[i], true))))
                {
                    vec<Lit> ps(2);
                    ps[0] = Lit(var, false);
                    ps[1] = Lit(it->second[i], false);
                    solver.varReplacer->replace(ps, true, 0);
                    replaced++;
                    goto next;
                    
                }
            }
        }
        
        tmp.reversePolarity();
        if (dataToVar[tmp].size() > 0) {
            map<varDataStruct, vector<Var> >::iterator it = dataToVar.find(tmp);
            for (uint i = 0; i < it->second.size(); i++) if (it->second[i] != var) {
                checked ++;
                if (checkIfSame(Lit(var, true), Lit(it->second[i], false)) &&
                    (checkIfSame(Lit(var, false), Lit(it->second[i], true))))
                {
                    vec<Lit> ps(2);
                    ps[0] = Lit(var, false);
                    ps[1] = Lit(it->second[i], false);
                    solver.varReplacer->replace(ps, false, 0);
                    replaced++;
                    goto next;
                }
            }
        }
        
        next:;
    }
        
    std::cout << "c |  Merging vars in same clauses.  checked: " << std::setw(5) << checked << " replaced: " << std::setw(5) << replaced << " time: " << std::setprecision(2) << std::setw(5) << cpuTime()-myTime << std::endl;
    
    return var_merged;
}*/

/*const bool Subsumer::checkIfSame(const Lit lit1, const Lit lit2)
{
    assert(lit1.var() != lit2.var());
    #ifdef VERBOSE_DEBUG
    std::cout << "checking : " << lit1.var()+1 << " , " << lit2.var()+1 << std::endl;
    #endif
    
    vec<ClauseSimp>& occSet1 = occur[lit1.toInt()];
    vec<ClauseSimp>& occSet2 = occur[lit2.toInt()];
    vec<Lit> tmp;
    
    for (ClauseSimp *it = occSet1.getData(), *end = it + occSet1.size(); it != end; it++) {
        bool found = false;
        tmp.clear();
        uint32_t clauseSize = it->clause->size();
        
        for (Lit *l = it->clause->getData(), *end2 = l + it->clause->size(); l != end2; l++) {
            if (l->var() != lit1.var()) {
                tmp.push(*l);
                seen_tmp[l->toInt()] = true;
            }
        }
        #ifdef VERBOSE_DEBUG
        std::cout << "orig: ";
        it->clause->plainPrint();
        #endif
        
        for (ClauseSimp *it2 = occSet2.getData(), *end2 = it2 + occSet2.size(); (it2 != end2 && !found); it2++) {
            if (it2->clause->size() != clauseSize) continue;
            
            for (Lit *l = it2->clause->getData(), *end3 = l + tmp.size(); l != end3; l++) if (l->var() != lit1.var()) {
                if (!seen_tmp[l->toInt()]) goto next;
            }
            found = true;
            
            #ifdef VERBOSE_DEBUG
            std::cout << "OK:   ";
            it2->clause->plainPrint();
            #endif
            next:;
        }
        
        for (Lit *l = tmp.getData(), *end2 = l + tmp.size(); l != end2; l++) {
            seen_tmp[l->toInt()] = false;
        }
        
        if (!found) return false;
    }
    #ifdef VERBOSE_DEBUG
    std::cout << "OK" << std::endl;
    #endif
    
    return true;
}*/

/*void Subsumer::pureLiteralRemoval()
{
    for (Var var = 0; var < solver.nVars(); var++) if (solver.decision_var[var] && solver.assigns[var] == l_Undef && !cannot_eliminate[var] && !var_elimed[var] && !solver.varReplacer->varHasBeenReplaced(var)) {
        uint32_t numPosClauses = occur[Lit(var, false).toInt()].size();
        uint32_t numNegClauses = occur[Lit(var, true).toInt()].size();
        if (numNegClauses > 0 && numPosClauses > 0) continue;
        
        if (numNegClauses == 0 && numPosClauses == 0) {
            if (solver.decision_var[var]) madeVarNonDecision.push(var);
            solver.setDecisionVar(var, false);
            pureLitRemoved[var] = true;
            pureLitsRemovedNum++;
            continue;
        }
        
        if (numPosClauses == 0 && numNegClauses > 0) {
            if (solver.decision_var[var]) madeVarNonDecision.push(var);
            solver.setDecisionVar(var, false);
            pureLitRemoved[var] = true;
            assignVar.push(Lit(var, true));
            vec<ClauseSimp> occ(occur[Lit(var, true).toInt()]);
            for (ClauseSimp *it = occ.getData(), *end = occ.getDataEnd(); it != end; it++) {
                unlinkClause(*it);
                pureLitClauseRemoved.push(it->clause);
            }
            pureLitsRemovedNum++;
            continue;
        }
        
        if (numNegClauses == 0 && numPosClauses > 0) {
            if (solver.decision_var[var]) madeVarNonDecision.push(var);
            solver.setDecisionVar(var, false);
            pureLitRemoved[var] = true;
            assignVar.push(Lit(var, false));
            vec<ClauseSimp> occ(occur[Lit(var, false).toInt()]);
            for (ClauseSimp *it = occ.getData(), *end = occ.getDataEnd(); it != end; it++) {
                unlinkClause(*it);
                pureLitClauseRemoved.push(it->clause);
            }
            pureLitsRemovedNum++;
            continue;
        }
    }
}*/

/*void Subsumer::undoPureLitRemoval()
{
    assert(solver.ok);
    for (uint32_t i = 0; i < madeVarNonDecision.size(); i++) {
        assert(!solver.decision_var[madeVarNonDecision[i]]);
        assert(solver.assigns[madeVarNonDecision[i]] == l_Undef);
        solver.setDecisionVar(madeVarNonDecision[i], true);
        pureLitRemoved[madeVarNonDecision[i]] = false;
    }
    madeVarNonDecision.clear();
    
    for (Lit *l = assignVar.getData(), *end = assignVar.getDataEnd(); l != end; l++) {
        solver.uncheckedEnqueue(*l);
        solver.ok = (solver.propagate() == NULL);
        assert(solver.ok);
    }
    assignVar.clear();
}

void Subsumer::reAddPureLitClauses()
{
    for (Clause **it = pureLitClauseRemoved.getData(), **end = pureLitClauseRemoved.getDataEnd(); it != end; it++) {
        solver.addClause(**it, (*it)->getGroup());
        free(*it);
        assert(solver.ok);
    }
    pureLitClauseRemoved.clear();
}*/
