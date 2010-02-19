/**************************************************************************************************

Solver.C -- (C) Niklas Een, Niklas Sorensson, 2004

A simple Chaff-like SAT-solver with support for incremental SAT.

**************************************************************************************************/

#include "Solver.h"
#include "Simplifier.h"
#include "ClauseCleaner.h"
#include "time_mem.h"
#include "assert.h"

//#define VERBOSE_DEBUG
//#define TOUCH_LESS

#ifdef VERBOSE_DEBUG
using std::cout;
using std::endl;
#endif //VERBOSE_DEBUG

Simplifier::Simplifier(Solver& s):
    solver(s)
    , occur_mode(occ_Permanent)
{};

static bool opt_var_elim = true;

void Simplifier::exclude(vec<ClauseSimp>& cs, Clause* c)
{
    uint i = 0, j = 0;
    for (; i < cs.size(); i++) {
        if (cs[i].clause != c)
            cs[j++] = cs[i];
    }
    cs.shrink(i-j);
}

bool selfSubset(uint64_t A, uint64_t B)
{
    uint64_t B_tmp = B | ((B & 0xAAAAAAAAAAAAAAAALL) >> 1) | ((B & 0x5555555555555555LL) << 1);
    if ((A & ~B_tmp) == 0){
        uint64_t C = A & ~B;
        return (C & (C-1)) == 0;
    }else
        return false;
}

// Assumes 'seen' is cleared (will leave it cleared)
bool selfSubset(Clause& A, Clause& B, vec<bool>& seen)
{
    for (int i = 0; i < B.size(); i++)
        seen[B[i].toInt()] = 1;
    
    bool    flip = false;
    for (int i = 0; i < A.size(); i++) {
        if (!seen[A[i].toInt()]) {
            if (flip == true || !seen[(~A[i]).toInt()]) {
                for (int i = 0; i < B.size(); i++) seen[B[i].toInt()] = 0;
                return false;
            }
            flip = true;
        }
    }
    for (int i = 0; i < B.size(); i++)
        seen[B[i].toInt()] = 0;
    return flip;
}

// Will put NULL in 'cs' if clause removed.
void Simplifier::subsume0(ClauseSimp& ps, int& counter)
{
    #ifdef VERBOSE_DEBUG
    cout << "subsume0 orig clause:";
    ps.clause->plainPrint();
    cout << "pointer:" << ps.clause << endl;
    #endif
    
    vec<ClauseSimp> subs;
    findSubsumed(ps, subs);
    for (int i = 0; i < subs.size(); i++){
        if (&counter != NULL) counter++;
        #ifdef VERBOSE_DEBUG
        cout << "subsume0 removing:";
        subs[i].clause->plainPrint();
        #endif
        
        Clause* tmp = subs[i].clause;
        unlinkClause(subs[i]);
        assert(ps.clause != NULL);
        
        if (ps.clause->learnt() && !tmp->learnt()) {
            //we are making it non-learnt, so in case they where not in cl_added, then we must add them for consistency
            if (!updateOccur(*ps.clause)) {
                cl_added.add(ps);
                Clause& cl = *ps.clause;
                for (int i = 0; i < cl.size(); i++) {
                    occur[cl[i].toInt()].push(ps);
                    touch(cl[i].var());
                }
            }
            ps.clause->makeNonLearnt();
            solver.learnts_literals -= ps.clause->size();
            solver.clauses_literals += ps.clause->size();
        }
        free(tmp);
    }
}

void Simplifier::unlinkClause(ClauseSimp c, Var elim)
{
    Clause& cl = *c.clause;
    
    if (elim != var_Undef) {
        assert(!cl.learnt());
        io_tmp.clear();
        io_tmp.push(Lit(cl.size(), false));
        for (int i = 0; i < cl.size(); i++)
            io_tmp.push(cl[i]);
        fwrite(io_tmp.getData(), 4, io_tmp.size(), elim_out);
    }
    
    if (updateOccur(cl)) {
        for (int i = 0; i < cl.size(); i++) {
            maybeRemove(occur[cl[i].toInt()], &cl);
            #ifndef TOUCH_LESS
            touch(cl[i]);
            #endif
        }
    }
    
    solver.detachClause(cl);
    clauses[c.index].clause = NULL;
    
    // Remove from iterator vectors/sets:
    for (int i = 0; i < iter_vecs.size(); i++) {
        vec<ClauseSimp>& cs = *iter_vecs[i];
        for (int j = 0; j < cs.size(); j++)
            if (cs[j].clause == &cl)
                cs[j].clause = NULL;
    }
    for (int i = 0; i < iter_sets.size(); i++) {
        CSet& cs = *iter_sets[i];
        cs.exclude(c);
    }
    
    // Remove clause from clause touched set:
    if (updateOccur(cl)) {
        cl_touched.exclude(c);
        cl_added.exclude(c);
    }
}

void Simplifier::unlinkModifiedClause(vec<Lit>& cl, ClauseSimp c)
{
    if (updateOccur(*c.clause)) {
        for (int i = 0; i < cl.size(); i++) {
            maybeRemove(occur[cl[i].toInt()], c.clause);
            #ifndef TOUCH_LESS
            touch(cl[i]);
            #endif
        }
    }
    
    solver.detachModifiedClause(cl[0], cl[1], cl.size(), c.clause);
    clauses[c.index].clause = NULL;
    
    // Remove from iterator vectors/sets:
    for (int i = 0; i < iter_vecs.size(); i++){
        vec<ClauseSimp>& cs = *iter_vecs[i];
        for (int j = 0; j < cs.size(); j++)
            if (cs[j].clause == c.clause)
                cs[j].clause = NULL;
    }
    for (int i = 0; i < iter_sets.size(); i++){
        CSet& cs = *iter_sets[i];
        cs.exclude(c);
    }
    
    // Remove clause from clause touched set:
    if (updateOccur(*c.clause)) {
        cl_touched.exclude(c);
        cl_added.exclude(c);
    }
}

void Simplifier::subsume1(ClauseSimp& ps, int& counter)
{
    vec<ClauseSimp>    Q;
    vec<ClauseSimp>    subs;
    vec<Lit>        qs;
    int             q;
    
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
        for (int i = 0; i < Q[q].clause->size(); i++)
            qs.push((*Q[q].clause)[i]);
        
        for (int i = 0; i < qs.size(); i++){
            qs[i] = ~qs[i];
            findSubsumed(qs, subs);
            for (int j = 0; j < subs.size(); j++){
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
                if (&counter != NULL) counter++;
                
                #ifdef VERBOSE_DEBUG
                cout << "orig clause    :";
                subs[j].clause->plainPrint();
                #endif
                ClauseSimp c = subs[j];
                Clause& cl = *c.clause;
                unlinkClause(c);
                
                cl.strengthen(qs[i]);
                #ifdef VERBOSE_DEBUG
                cout << "strenghtened   :";
                c.clause->plainPrint();
                #endif
                
                assert(cl.size() > 0);
                if (cl.size() > 1) {
                    solver.attachClause(cl);
                    updateClause(cl, c);
                    Q.push(c);
                } else {
                    if (solver.assigns[cl[0].var()] == l_Undef) {
                        solver.uncheckedEnqueue(cl[0]);
                        #ifdef VERBOSE_DEBUG
                        cout << "Found that var " << cl[0].var()+1 << " must be " << std::boolalpha << !cl[0].sign() << endl;
                        #endif
                    } else {
                        if (solver.assigns[cl[0].var()].getBool() != !cl[0].sign()) {
                            solver.ok = false;
                            unregisterIteration(Q);
                            unregisterIteration(subs);
                            free(&cl);
                            return;
                        }
                    }
                    free(&cl);
                }
            }
            
            qs[i] = ~qs[i];
            subs.clear();
        }
        q++;
    }
    
    unregisterIteration(Q);
    unregisterIteration(subs);
}

void Simplifier::updateClause(Clause& cl, ClauseSimp& c)
{
    clauses[c.index] = c;
    c.abst = calcAbstraction(cl);
    // Update from iterator vectors/sets:
    for (int i = 0; i < iter_vecs.size(); i++) {
        vec<ClauseSimp>& cs = *iter_vecs[i];
        for (uint i2 = 0; i2 < cs.size(); i2++)
            if (cs[i2].clause == &cl)
                cs[i2].abst = c.abst;
    }
    for (int i = 0; i < iter_sets.size(); i++) {
        CSet& cs = *iter_sets[i];
        cs.update(c);
    }
    cl_touched.update(c);
    cl_added.update(c);
    
    // Occur lists:
    if (occur_mode != occ_Off)
        if (!cl.learnt()) subsume0(c);
    
    if (updateOccur(cl)) {
        for (int i2 = 0; i2 < cl.size(); i2++) {
            occur[cl[i2].toInt()].push(c);
            touch(cl[i2].var());
        }
        //if (update)
            cl_touched.add(c);
        //else
        //    cl_added.add(c);
    }
}

void Simplifier::almost_all_database(int& clauses_subsumed, int& literals_removed)
{
    std::cout << "c Larger database" << std::endl;
    // Optimized variant when virtually whole database is involved:
    cl_added  .clear();
    cl_touched.clear();
    
    for (int i = 0; i < clauses.size(); i++) {
        if (clauses[i].clause != NULL)
            subsume1(clauses[i], literals_removed);
    }
    if (!solver.ok) return;
    solver.ok = solver.propagate() == NULL;
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
    while (cl_touched.size() > 0){
        std::cout << "c cl_touched was > 0, new iteration" << std::endl;
        for (CSet::iterator it = cl_touched.begin(), end = cl_touched.end(); it != end; ++it) {
            if (it->clause != NULL)
                s1.add(*it);
        }
        cl_touched.clear();
        
        for (CSet::iterator it = s1.begin(), end = s1.end(); it != end; ++it) {
            if (it->clause != NULL)
                subsume1(*it, literals_removed);
        }
        s1.clear();
        
        if (!solver.ok) return;
        solver.ok = solver.propagate() == NULL;
        if (!solver.ok) {
            printf("(contradiction during subsumption)\n");
            unregisterIteration(s1);
            return;
        }
        solver.clauseCleaner->cleanClausesBewareNULL(clauses, ClauseCleaner::simpClauses, *this);
    }
    unregisterIteration(s1);
    
    printf("c final stage: subsume0\n");
    
    for (int i = 0; i < clauses.size(); i++) {
        assert(clauses[i].index == i);
        if (clauses[i].clause != NULL)
            subsume0(clauses[i], clauses_subsumed);
    }
}

void Simplifier::smaller_database(int& clauses_subsumed, int& literals_removed)
{
    std::cout << "Smaller database" << std::endl;
    //  Set used in 1-subs:
    //      (1) clauses containing a negated literal of an added clause.
    //      (2) all added or strengthened ("touched") clauses.
    //
    //  Set used in 0-subs:
    //      (1) clauses containing a (non-negated) literal of an added clause, including the added clause itself.
    //      (2) all strenghtened clauses -- REMOVED!! We turned on eager backward subsumption which supersedes this.
    
    printf("  PREPARING\n");
    CSet s0, s1;     // 's0' is used for 0-subsumption, 's1' for 1-subsumption
    vec<char>   ol_seen(solver.nVars()*2, 0);
    for (CSet::iterator it = cl_added.begin(), end = cl_added.end(); it != end; ++it) {
        if (it->clause != NULL) continue;
        ClauseSimp& c = *it;
        Clause& cl = *it->clause;
        
        s1.add(c);
        for (int j = 0; j < cl.size(); j++){
            if (ol_seen[cl[j].toInt()]) continue;
            ol_seen[cl[j].toInt()] = 1;
            
            vec<ClauseSimp>& n_occs = occur[~cl[j].toInt()];
            for (int k = 0; k < n_occs.size(); k++)
                if (n_occs[k].clause != c.clause && n_occs[k].clause->size() <= cl.size() && selfSubset(n_occs[k].abst, c.abst) && selfSubset(*n_occs[k].clause, cl, seen_tmp))
                    s1.add(n_occs[k]);
                
            vec<ClauseSimp>& p_occs = occur[cl[j].toInt()];
            for (int k = 0; k < p_occs.size(); k++)
                if (subsetAbst(p_occs[k].abst, c.abst))
                    s0.add(p_occs[k]);
        }
    }
    cl_added.clear();
    
    registerIteration(s0);
    registerIteration(s1);
    
    printf("  FIXED-POINT\n");
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
        printf("s1.size()=%d  cl_touched.size()=%d\n", s1.size(), cl_touched.size());
        
        for (CSet::iterator it = s1.begin(), end = s1.end(); it != end; ++it) {
            if (it->clause != NULL)
                subsume1(*it, literals_removed);
        }
        s1.clear();
        
        if (!solver.ok) return;
        solver.ok = solver.propagate() == NULL;
        if (!solver.ok){
            printf("(contradiction during subsumption)\n");
            unregisterIteration(s1);
            unregisterIteration(s0);
            return; 
        }
        solver.clauseCleaner->cleanClausesBewareNULL(clauses, ClauseCleaner::simpClauses, *this);
        assert(cl_added.size() == 0);
    }
    unregisterIteration(s1);
    
    // Iteration pass for 0-subsumption:
    for (CSet::iterator it = s0.begin(), end = s0.end(); it != end; ++it) {
        if (it->clause != NULL)
            subsume0(*it, clauses_subsumed);
    }
    s0.clear();
    unregisterIteration(s0);
}

void Simplifier::addFromSolver(vec<Clause*>& cs)
{
    for (uint i = 0; i < cs.size(); i++) {
        ClauseSimp c(cs[i], clauses.size());
        clauses.push(c);
        Clause& cl = *c.clause;
        if (updateOccur(cl)) {
            for (int i = 0; i < cl.size(); i++) {
                occur[cl[i].toInt()].push(c);
                touch(cl[i].var());
            }
            cl_added.add(c);
        }
    }
    cs.clear();
}

const bool Simplifier::simplifyBySubsumption(bool with_var_elim)
{
    double myTime = cpuTime();
    
    for (Var i = 0; i < solver.nVars(); i++) {
        occur       .push();
        occur       .push();
        seen_tmp    .push(0);       // (one for each polarity)
        seen_tmp    .push(0);
        touched     .push(1);
        touched_list.push(i);
        var_elimed  .push(0);
    }
    
    solver.clauseCleaner->cleanClauses(solver.clauses, ClauseCleaner::clauses);
    addFromSolver(solver.clauses);
    solver.clauseCleaner->cleanClauses(solver.binaryClauses, ClauseCleaner::binaryClauses);
    addFromSolver(solver.binaryClauses);
    uint32_t origLearntsSize = solver.learnts.size();
    solver.clauseCleaner->cleanClauses(solver.learnts, ClauseCleaner::learnts);
    addFromSolver(solver.learnts);
    
    int     clauses_subsumed = 0, literals_removed = 0;
    int     orig_n_clauses  = solver.nClauses();
    int     orig_n_literals = solver.nLiterals();
    uint    origTrailSize = solver.trail.size();
    
    for (int i = 0; i < clauses.size(); i++) {
        assert(clauses[i].index == i);
        if (clauses[i].clause != NULL)
            subsume0(clauses[i], clauses_subsumed);
    }
    if (!solver.ok) return false;
    std::cout << "c   pre-subsumed:" << clauses_subsumed << std::endl;
    
    do{
        // SUBSUMPTION:
        //
        #ifndef SAT_LIVE
        std::cout << "c   -- subsuming" << std::endl;
        #endif
        
        if (cl_added.size() > solver.nClauses() / 2) {
            almost_all_database(clauses_subsumed, literals_removed);
            if (!solver.ok) return false;
        } else {
            smaller_database(clauses_subsumed, literals_removed);
            if (!solver.ok) return false;
        }
        
        // VARIABLE ELIMINATION:
        //
        if (!with_var_elim || !opt_var_elim) break;
        
        
        /*printf("VARIABLE ELIMINIATION\n");
        vec<Var>    init_order;
        orderVarsForElim(init_order);   // (will untouch all variables)
        
        for (bool first = true;; first = false){
            int vars_elimed = 0;
            int clauses_before = solver.nClauses();
            vec<Var>    order;
            
            if (first)
                init_order.copyTo(order);
            else{
                for (int i = 0; i < touched_list.size(); i++)
                    if (!var_elimed[touched_list[i]]) {
                        order.push(touched_list[i]);
                        touched[touched_list[i]] = 0;
                    }
                    touched_list.clear();
            }
            
            assert(solver.qhead == solver.trail.size());
            for (int i = 0; i < order.size(); i++){
                #ifndef SAT_LIVE
                if (i % 1000 == 999 || i == order.size()-1)
                    printf("  -- var.elim.:  %d/%d          \r", i+1, order.size());
                #endif
                if (maybeEliminate(order[i])){
                    vars_elimed++;
                    solver.ok = solver.propagate() == NULL;
                    if (!solver.ok) {
                        printf("(contradiction during subsumption)\n");
                        return;
                    }
                }
            }
            assert(solver.qhead == solver.trail.size());
            
            if (vars_elimed == 0)
                break;
            
            printf("  #clauses-removed: %-8d #var-elim: %d\n", clauses_before - solver.nClauses(), vars_elimed);
            
        }*/
    }while (cl_added.size() > 100);
    
    if (!solver.ok) return false;
    solver.ok = (solver.propagate() == NULL);
    if (!solver.ok) {
        printf("(contradiction during subsumption)\n");
        return false;
    }
    
    std::cout << "c #literals-removed: " << literals_removed 
    << "  #clauses-subsumed: " << clauses_subsumed 
    << " #vars fixed: " << solver.trail.size() - origTrailSize
    << std::endl;
    
    if (solver.trail.size() - origTrailSize > 0)
        solver.order_heap.filter(Solver::VarFilter(solver));
    
    for (uint i = 0; i < clauses.size(); i++) {
        if (clauses[i].clause != NULL) {
            if (clauses[i].clause->size() == 2)
                solver.binaryClauses.push(clauses[i].clause);
            else {
                if (clauses[i].clause->learnt())
                    solver.learnts.push(clauses[i].clause);
                else
                    solver.clauses.push(clauses[i].clause);
            }
        }
    }
    clauses.clear();
    solver.nbCompensateSimplifier += origLearntsSize-solver.learnts.size();
    
     /*for (i = 0; i < solver.clauses.size(); i++) {
         cout << "detaching:"; solver.clauses[i]->plainPrint();
         cout << "pointer:" << solver.clauses[i] << endl;
         solver.detachClause(*solver.clauses[i]);
     }
     for (i = 0; i < solver.binaryClauses.size(); i++) {
         cout << "detaching:"; solver.binaryClauses[i]->plainPrint();
         cout << "pointer:" << solver.binaryClauses[i] << endl;
         solver.detachClause(*solver.binaryClauses[i]);
     }*/
    
    std::cout << "c Simplification time:" << (cpuTime() - myTime) << std::endl;
    return true;
}

void Simplifier::findSubsumed(ClauseSimp& ps, vec<ClauseSimp>& out_subsumed)
{
    #ifdef VERBOSE_DEBUG
    cout << "findSubsumed: ";
    for (uint i = 0; i < ps.clause->size(); i++) {
        if ((*ps.clause)[i].sign()) printf("-");
        printf("%d ", (*ps.clause)[i].var() + 1);
    }
    printf("0\n");
    #endif
    
    Clause& cl = *ps.clause;
    int min_i = 0;
    for (int i = 1; i < cl.size(); i++){
        if (occur[cl[i].toInt()].size() < occur[cl[min_i].toInt()].size())
            min_i = i;
    }
    
    vec<ClauseSimp>& cs = occur[cl[min_i].toInt()];
    for (int i = 0; i < cs.size(); i++){
        if (i+1 < cs.size())
            __builtin_prefetch(cs[i+1].clause, 1, 1);
        
        if (cs[i].clause != ps.clause && subsetAbst(ps.abst, cs[i].abst) && cl.size() <= cs[i].clause->size() && subset(cl, *cs[i].clause, seen_tmp)) {
            out_subsumed.push(cs[i]);
            #ifdef VERBOSE_DEBUG
            cout << "subsumed: ";
            cs[i].clause->plainPrint();
            #endif
        }
    }
}

void Simplifier::findSubsumed(vec<Lit>& ps, vec<ClauseSimp>& out_subsumed)
{
    #ifdef VERBOSE_DEBUG
    cout << "findSubsumed: ";
    for (uint i = 0; i < ps.size(); i++) {
        if (ps[i].sign()) printf("-");
        printf("%d ", ps[i].var() + 1);
    }
    printf("0\n");
    #endif
    
    uint32_t abst = calcAbstraction(ps);
    
    uint32_t min_i = 0;
    for (uint32_t i = 1; i < ps.size(); i++){
        if (occur[ps[i].toInt()].size() < occur[ps[min_i].toInt()].size())
            min_i = i;
    }
    
    vec<ClauseSimp>& cs = occur[ps[min_i].toInt()];
    for (uint32_t i = 0; i < cs.size(); i++){
        if (i+1 < cs.size())
            __builtin_prefetch(cs[i+1].clause, 1, 1);
        
        if (subsetAbst(abst, cs[i].abst) && ps.size() <= cs[i].clause->size() && subset(ps, *cs[i].clause, seen_tmp)) {
            out_subsumed.push(cs[i]);
            #ifdef VERBOSE_DEBUG
            cout << "subsumed: ";
            cs[i].clause->plainPrint();
            #endif
        }
    }
}

/*
// Returns TRUE if variable was eliminated.
bool Solver::maybeEliminate(Var x)
{
    assert(propQ.size() == 0);
    assert(!var_elimed[x]);
    if (value(x) != l_Undef) return false;
    if (occur[index(Lit(x))].size() == 0 && occur[index(~Lit(x))].size() == 0) return false;
    
    vec<Clause>&   poss = occur[index( Lit(x))];
    vec<Clause>&   negs = occur[index(~Lit(x))];
    vec<Clause>    new_clauses;
    
    int before_clauses  = -1;
    int before_literals = -1;
    
    bool    elimed     = false;
    bool    tried_elim = false;
    
    #define MigrateToPsNs vec<Clause> ps; poss.moveTo(ps); vec<Clause> ns; negs.moveTo(ns); for (int i = 0; i < ps.size(); i++) unlinkClause(ps[i], x); for (int i = 0; i < ns.size(); i++) unlinkClause(ns[i], x);
    #define DeallocPsNs   for (int i = 0; i < ps.size(); i++) deallocClause(ps[i]); for (int i = 0; i < ns.size(); i++) deallocClause(ns[i]);
    
    // Find 'x <-> p':
    if (opt_unit_def){
        Lit p = findUnitDef(x, poss, negs);
        if (p == lit_Undef) propagateToplevel(); if (!ok) return true;
        if (p != lit_Undef){
            //printf("DEF: x%d = "L_LIT"\n", x, L_lit(p));
            //BEG
            //printf("POS:\n"); for (int i = 0; i < poss.size(); i++) printf("  "), dump(poss[i]);
            //printf("NEG:\n"); for (int i = 0; i < negs.size(); i++) printf("  "), dump(negs[i]);
            //printf("DEF: x%d = "L_LIT"\n", x, L_lit(p));
            //hej = true;
            //END
            Clause_t    def; def.push(p);
            MigrateToPsNs
            substitute(Lit(x), def, ps, ns, new_clauses);
            //BEG
            //hej = false;
            //printf("NEW:\n");
            //for (int i = 0; i < new_clauses.size(); i++)
            //printf("  "), dump(new_clauses[i]);
            //END
            propagateToplevel(); if (!ok) return true;
            DeallocPsNs
            goto Eliminated;
        }
    }
    
    
    // Heuristic:
    if (poss.size() >= 8 && negs.size() >= 8)      // <<== CUT OFF
        //  if (poss.size() >= 7 && negs.size() >= 7)      // <<== CUT OFF
        //  if (poss.size() >= 6 && negs.size() >= 6)      // <<== CUT OFF
        return false;
    
    // Count clauses/literals before elimination:
    before_clauses  = poss.size() + negs.size();
    before_literals = 0;
    for (int i = 0; i < poss.size(); i++) before_literals += poss[i].size();
    for (int i = 0; i < negs.size(); i++) before_literals += negs[i].size();
    
    if (poss.size() >= 3 && negs.size() >= 3 && before_literals > 300)  // <<== CUT OFF
        return false;
    
    // Check for definitions:
    if (opt_def_elim || opt_hyper1_res){
        //if (poss.size() > 1 || negs.size() > 1){
   if (poss.size() > 2 || negs.size() > 2){
       Clause_t def;
       int      result_size;
       if (findDef(Lit(x), poss, negs, def)){
           if (def.size() == 0){ enqueue(~Lit(x)); return true; }  // Hyper-1-resolution
               result_size = substitute(Lit(x), def, poss, negs);
               if (result_size <= poss.size() + negs.size()){  // <<= elimination threshold (maybe subst. should return literal count as well)
               MigrateToPsNs
               substitute(Lit(x), def, ps, ns, new_clauses);
               propagateToplevel(); if (!ok) return true;
               DeallocPsNs
               goto Eliminated;
               }else
                   tried_elim = true,
                   def.clear();
       }
       if (!elimed && findDef(~Lit(x), negs, poss, def)){
           if (def.size() == 0){ enqueue(Lit(x)); return true; }  // Hyper-1-resolution
               result_size = substitute(~Lit(x), def, negs, poss);
               if (result_size <= poss.size() + negs.size()){  // <<= elimination threshold
                   MigrateToPsNs
                   substitute(~Lit(x), def, ns, ps, new_clauses);
                   propagateToplevel(); if (!ok) return true;
                   DeallocPsNs
                   goto Eliminated;
               }else
                   tried_elim = true;
       }
   }
    }
    
    if (!tried_elim){
        // Count clauses/literals after elimination:
        int after_clauses  = 0;
        int after_literals = 0;
        Clause_t  dummy;
        for (int i = 0; i < poss.size(); i++){
            for (int j = 0; j < negs.size(); j++){
                // Merge clauses. If 'y' and '~y' exist, clause will not be created.
                dummy.clear();
                bool ok = merge(poss[i], negs[j], Lit(x), ~Lit(x), seen_tmp, dummy.asVec());
                if (ok){
                    after_clauses++;
                    if (after_clauses > before_clauses) goto Abort;
                    after_literals += dummy.size(); }
            }
        }
        Abort:;
        
        // Maybe eliminate:
        if ((!opt_niver && after_clauses  <= before_clauses)
            ||  ( opt_niver && after_literals <= before_literals)
            ){
            MigrateToPsNs
            for (int i = 0; i < ps.size(); i++){
                for (int j = 0; j < ns.size(); j++){
                    dummy.clear();
                    bool ok = merge(ps[i], ns[j], Lit(x), ~Lit(x), seen_tmp, dummy.asVec());
                    if (ok){
                        Clause c = addClause(dummy.asVec());
                        if (!c.null()){
                            new_clauses.push(c); }
                            propagateToplevel(); if (!ok) return true;
                    }
                }
            }
            DeallocPsNs
            goto Eliminated;
        }
        
        //****TEST*****
        // Try to remove 'x' from clauses:
        bool    ran = false;
        if (poss.size() < 10){ ran = true; asymmetricBranching( Lit(x)); if (!ok) return true; }
        if (negs.size() < 10){ ran = true; asymmetricBranching(~Lit(x)); if (!ok) return true; }
        if (value(x) != l_Undef) return false;
        if (!ran) return false;
        
        {
            // Count clauses/literals after elimination:
            int after_clauses  = 0;
            int after_literals = 0;
            Clause_t  dummy;
            for (int i = 0; i < poss.size(); i++){
                for (int j = 0; j < negs.size(); j++){
                    // Merge clauses. If 'y' and '~y' exist, clause will not be created.
                    dummy.clear();
                    bool ok = merge(poss[i], negs[j], Lit(x), ~Lit(x), seen_tmp, dummy.asVec());
                    if (ok){
                        after_clauses++;
                        after_literals += dummy.size(); }
                }
            }
            
            // Maybe eliminate:
            if ((!opt_niver && after_clauses  <= before_clauses)
                ||  ( opt_niver && after_literals <= before_literals)
                ){
                MigrateToPsNs
                for (int i = 0; i < ps.size(); i++){
                    for (int j = 0; j < ns.size(); j++){
                        dummy.clear();
                        bool ok = merge(ps[i], ns[j], Lit(x), ~Lit(x), seen_tmp, dummy.asVec());
                        if (ok){
                            Clause c = addClause(dummy.asVec());
                            if (!c.null()){
                                new_clauses.push(c); }
                                propagateToplevel(); if (!ok) return true;
                        }
                    }
                }
                DeallocPsNs
                goto Eliminated;
            }
        }
        //*****END TEST****
    }
    
    return false;
    
    Eliminated:
    assert(occur[index(Lit(x))].size() + occur[index(~Lit(x))].size() == 0);
    var_elimed[x] = 1;
    assigns   [x] = toInt(l_Error);
    return true;
}
*/
