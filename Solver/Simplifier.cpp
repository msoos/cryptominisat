/**************************************************************************************************

Solver.C -- (C) Niklas Een, Niklas Sorensson, 2004

A simple Chaff-like SAT-solver with support for incremental SAT.

**************************************************************************************************/

#include "Solver.h"
#include "Simplifier.h"
#include "ClauseCleaner.h"
#include "time_mem.h"

//#define VERBOSE_DEBUG

#ifdef VERBOSE_DEBUG
using std::cout;
using std::endl;
#endif //VERBOSE_DEBUG

Simplifier::Simplifier(Solver& s):
    solver(s)
    , occur_mode(occ_Permanent)
{};

static bool opt_var_elim = true;

void Simplifier::exclude(vec<Clause*>& cs, Clause* c)
{
    uint i = 0, j = 0;
    for (; i < cs.size(); i++) {
        if (cs[i] != c)
            cs[j++] = cs[i];
    }
    cs.shrink(i-j);
}

inline bool subset(uint64_t A, uint64_t B) {
    return (A & ~B) == 0;
}

// Assumes 'seen' is cleared (will leave it cleared)
template<class T1, class T2>
static bool subset(T1& A, T2& B, vec<char>& seen)
{
    for (int i = 0; i < B.size(); i++)
        seen[B[i].toInt()] = 1;
    for (int i = 0; i < A.size(); i++){
        if (!seen[A[i].toInt()]){
            for (int i = 0; i < B.size(); i++) 
                seen[B[i].toInt()] = 0;
            return false;
        }
    }
    for (int i = 0; i < B.size(); i++) 
        seen[B[i].toInt()] = 0;
    return true;
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
bool selfSubset(Clause& A, Clause& B, vec<char>& seen)
{
    for (int i = 0; i < B.size(); i++)
        seen[B[i].toInt()] = 1;
    
    bool    flip = false;
    for (int i = 0; i < A.size(); i++){
        if (!seen[A[i].toInt()]){
            if (flip == true || !seen[(~A[i]).toInt()]){
                for (int i = 0; i < B.size(); i++) seen[B[i].toInt()] = 0;
                return false;
            }
            flip = true;
        }
    }
    for (int i = 0; i < B.size(); i++) seen[B[i].toInt()] = 0;
    return flip;
}

// Will put NULL in 'cs' if clause removed.
void Simplifier::subsume0(Clause* ps, int& counter)
{
    #ifdef VERBOSE_DEBUG
    cout << "subsume0 orig clause:";
    ps->plainPrint();
    cout << "pointer:" << ps << endl;
    #endif
    
    vec<Clause*> subs;
    findSubsumed(*ps, subs);
    for (int i = 0; i < subs.size(); i++){
        if (&counter != NULL) counter++;
        #ifdef VERBOSE_DEBUG
        cout << "subsume0 removing:";
        subs[i]->plainPrint();
        #endif
        Clause* tmp = subs[i];
        unlinkClause(*subs[i]);
        free(tmp);
    }
}

void Simplifier::unlinkClause(Clause& c, bool reallyRemove, Var elim)
{
    if (elim != var_Undef){
        assert(!c.learnt());
        io_tmp.clear();
        io_tmp.push(Lit(c.size(), false));
        for (int i = 0; i < c.size(); i++)
            io_tmp.push(c[i]);
        fwrite(io_tmp.getData(), 4, io_tmp.size(), elim_out);
    }
    
    if (updateOccur(c)){
        for (int i = 0; i < c.size(); i++){
            maybeRemove(occur[c[i].toInt()], &c);
            #ifndef TOUCH_LESS
            touch(c[i]);
            #endif
        }
    }
    
    solver.detachClause(c);
    if (reallyRemove) {
        uint pos;
        if (c.learnt()) {
            for (pos = 0; pos < solver.learnts.size(); pos++)
                if (solver.learnts[pos] == &c) {
                    solver.learnts[pos] = NULL;
                    break;
                }
        } else {
            for (pos = 0; pos < solver.clauses.size(); pos++)
                if (solver.clauses[pos] == &c) {
                    solver.clauses[pos] = NULL;
                    break;
                }
        }
    }
    
    // Remove from iterator vectors/sets:
    for (int i = 0; i < iter_vecs.size(); i++){
        vec<Clause*>& cs = *iter_vecs[i];
        for (int j = 0; j < cs.size(); j++)
            if (cs[j] == &c)
                cs[j] = NULL;
    }
    for (int i = 0; i < iter_sets.size(); i++){
        vec<Clause*>& cs = *iter_sets[i];
        exclude(cs, &c);
    }
    
    // Remove clause from clause touched set:
    if (updateOccur(c)) {
        exclude(cl_touched, &c);
        exclude(cl_added, &c);
    }
}

void Simplifier::subsume1(Clause& ps, int& counter)
{
    vec<Clause*>    Q;
    vec<Clause*>    subs;
    vec<Lit>        qs;
    int             q;
    
    registerIteration(Q);
    registerIteration(subs);
    
    Q.push(&ps);
    q = 0;
    while (q < Q.size()){
        if (Q[q] == NULL) { q++; continue; }
        #ifdef VERBOSE_DEBUG
        cout << "subsume1 orig clause:";
        Q[q]->plainPrint();
        #endif
        
        qs.clear();
        for (int i = 0; i < Q[q]->size(); i++)
            qs.push((*Q[q])[i]);
        
        for (int i = 0; i < qs.size(); i++){
            qs[i] = ~qs[i];
            findSubsumed(qs, subs);
            for (int j = 0; j < subs.size(); j++){
                #ifndef NDEBUG
                if (&counter != NULL && counter == -1){
                    dump(*subs[j]);
                    qs[i] = ~qs[i];
                    dump(qs);
                    printf(L_LIT"\n", L_lit(qs[i]));
                    exit(0);
                }
                #endif
                if (subs[j] == NULL) continue;
                if (&counter != NULL) counter++;
                
                #ifdef VERBOSE_DEBUG
                cout << "orig clause    :";
                subs[j]->plainPrint();
                #endif
                Clause& c = *subs[j];
                unlinkClause(c, false);
                
                c.strengthen(qs[i]);
                #ifdef VERBOSE_DEBUG
                cout << "strenghtened   :";
                c.plainPrint();
                #endif
                
                assert(c.size() > 0);
                if (c.size() > 1) {
                    solver.attachClause(c);
                    // Occur lists:
                    if (updateOccur(c)){
                        for (int i2 = 0; i2 < c.size(); i2++) {
                            occur[c[i2].toInt()].push(&c);
                            touch(c[i2].var());
                        }
                        cl_added.push(&c);
                    }
                    Q.push(&c);
                } else {
                    uint pos;
                    if (c.learnt()) {
                        for (pos = 0; pos < solver.learnts.size(); pos++)
                            if (solver.learnts[pos] == &c) {
                                solver.learnts[pos] = NULL;
                                break;
                            }
                    } else {
                        for (pos = 0; pos < solver.clauses.size(); pos++)
                            if (solver.clauses[pos] == &c) {
                                solver.clauses[pos] = NULL;
                                break;
                            }
                    }
                    if (solver.assigns[c[0].var()] == l_Undef) {
                        solver.uncheckedEnqueue(c[0]);
                        #ifdef VERBOSE_DEBUG
                        cout << "Found that var " << c[0].var()+1 << " must be " << std::boolalpha << !c[0].sign() << endl;
                        #endif
                    } else {
                        if (solver.assigns[c[0].var()].getBool() != !c[0].sign()) {
                            solver.ok = false;
                            return;
                        }
                    }
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

void Simplifier::almost_all_database(int& clauses_subsumed, int& literals_removed)
{
    std::cout << "Larger database" << std::endl;
    // Optimized variant when virtually whole database is involved:
    cl_added  .clear();
    cl_touched.clear();
    
    for (int i = 0; i < solver.clauses.size(); i++) {
        if (solver.clauses[i] != NULL)
            subsume1(*solver.clauses[i], literals_removed);
    }
    
    if (!solver.ok)
        return;
    solver.ok = solver.propagate() == NULL;
    if (!solver.ok) {
        printf("(contradiction during subsumption)\n");
        return;
    }
    solver.clauseCleaner->cleanClausesBewareNULL(solver.clauses, ClauseCleaner::clauses, *this);
    
    #ifdef VERBOSE_DEBUG
    cout << "subsume1 part 1 finished" << endl;
    #endif
    
    vec<Clause*> s1; //TODO this should be CSet!!
    registerIteration(s1);
    while (cl_touched.size() > 0){
        for (int i = 0; i < cl_touched.size(); i++)
            if (cl_touched[i] != NULL)
                s1.push(cl_touched[i]);
        cl_touched.clear();
        
        for (int i = 0; i < s1.size(); i++)
            if (s1[i] != NULL)
                subsume1(*s1[i], literals_removed);
        s1.clear();
        
        solver.ok = solver.propagate() == NULL;
        if (!solver.ok) {
            printf("(contradiction during subsumption)\n");
            unregisterIteration(s1);
            return;
        }
        solver.clauseCleaner->cleanClausesBewareNULL(solver.clauses, ClauseCleaner::clauses, *this);
    }
    unregisterIteration(s1);
    
    for (int i = 0; i < solver.clauses.size(); i++) {
        if (solver.clauses[i] != NULL) {
            assert(solver.clauses[i]->size() > 1);
            subsume0(solver.clauses[i], clauses_subsumed);
        }
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
    vec<Clause*> s0, s1;     // 's0' is used for 0-subsumption, 's1' for 1-subsumption
    vec<char>   ol_seen(solver.nVars()*2, 0);
    for (int i = 0; i < cl_added.size(); i++){
        if (cl_added[i] != NULL) continue;
        Clause& c = *cl_added[i];
        
        s1.push(&c);
        for (int j = 0; j < c.size(); j++){
            if (ol_seen[c[j].toInt()]) continue;
            ol_seen[c[j].toInt()] = 1;
            
            vec<Clause*>& n_occs = occur[~c[j].toInt()];
            for (int k = 0; k < n_occs.size(); k++)
                if (n_occs[k] != &c && n_occs[k]->size() <= c.size() && selfSubset(n_occs[k]->abst(), c.abst()) && selfSubset(*n_occs[k], c, seen_tmp))
                    s1.push(n_occs[k]);
                
            vec<Clause*>& p_occs = occur[c[j].toInt()];
            for (int k = 0; k < p_occs.size(); k++)
                if (subset(p_occs[k]->abst(), c.abst()))
                    s0.push(p_occs[k]);
        }
    }
    cl_added.clear();
    
    registerIteration(s0);
    registerIteration(s1);
    
    printf("  FIXED-POINT\n");
    // Fixed-point for 1-subsumption:
    while (s1.size() > 0 || cl_touched.size() > 0){
        for (int i = 0; i < cl_touched.size(); i++)
            if (cl_touched[i] != NULL)
                s1.push(cl_touched[i]),
            s0.push(cl_touched[i]);
        
        cl_touched.clear();
        assert(solver.qhead == solver.trail.size());
        printf("s1.size()=%d  cl_touched.size()=%d\n", s1.size(), cl_touched.size());
        for (int i = 0; i < s1.size(); i++)
            if (s1[i] != NULL)
                subsume1(*s1[i], literals_removed);
            s1.clear();
        
        solver.ok = solver.propagate() == NULL;
        if (!solver.ok){
            printf("(contradiction during subsumption)\n");
            unregisterIteration(s0);
            unregisterIteration(s1);
            return; 
        }
        solver.clauseCleaner->cleanClausesBewareNULL(solver.clauses, ClauseCleaner::clauses, *this);
        assert(cl_added.size() == 0);
    }
    unregisterIteration(s1);
    
    // Iteration pass for 0-subsumption:
    for (int i = 0; i < s0.size(); i++)
        if (s0[i] != NULL)
            subsume0(s0[i], clauses_subsumed);
    s0.clear();
    unregisterIteration(s0);
}

void Simplifier::simplifyBySubsumption(bool with_var_elim)
{
    double time = cpuTime();
    
    for (Var i = 0; i < solver.nVars(); i++) {
        occur       .push();
        occur       .push();
        seen_tmp    .push(0);       // (one for each polarity)
        seen_tmp    .push(0);
        touched     .push(1);
        touched_list.push(i);
        var_elimed  .push(0);
    }
    
    for (uint i = 0; i < solver.binaryClauses.size(); i++) {
        if (solver.binaryClauses[i]->learnt())
            solver.learnts.push(solver.binaryClauses[i]);
        else
            solver.clauses.push(solver.binaryClauses[i]);
    }
    solver.binaryClauses.clear();
    solver.clauseCleaner->cleanClauses(solver.clauses, ClauseCleaner::clauses);
    
    for (uint i = 0; i < solver.clauses.size(); i++) {
        Clause& c = *solver.clauses[i];
        cl_added.push(&c);
        if (updateOccur(c)){
            for (int i = 0; i < c.size(); i++)
                occur[c[i].toInt()].push(&c),
                touch(c[i].var());
                cl_touched.push(&c);
        }
    }
    
    int     orig_n_clauses  = solver.nClauses();
    int     orig_n_literals = solver.nLiterals();
    do{
        // SUBSUMPTION:
        //
        #ifndef SAT_LIVE
        printf("  -- subsuming                       \n");
        #endif
        int     clauses_subsumed = 0, literals_removed = 0;
        
        if (cl_added.size() > solver.nClauses() / 2) {
            almost_all_database(clauses_subsumed, literals_removed);
            if (!solver.ok) return;
        }else{
            smaller_database(clauses_subsumed, literals_removed);
            if (!solver.ok) return;
        }
        
        if (literals_removed > 0 || clauses_subsumed > 0)
            printf("  #literals-removed: %d    #clauses-subsumed: %d\n", literals_removed, clauses_subsumed);
        
        
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
    
    if (orig_n_clauses != solver.nClauses() || orig_n_literals != solver.nLiterals()) {
        printf("| %9d | %7d %8d | %7s %7d %8s %7s | %6s   | %d/%d\n",
               (int)solver.conflicts,
               solver.nClauses(),
               solver.nLiterals(),
               "--", solver.nLearnts(), "--", "--", "--",
               solver.nClauses() - orig_n_clauses,
               solver.nLiterals() - orig_n_literals);
    }
    
    if (!solver.ok)
        return;
    
    solver.ok = (solver.propagate() == NULL);
    if (!solver.ok) {
        printf("(contradiction during subsumption)\n");
        return;
    }
    
    uint i, j;
    for (i = j = 0; i < solver.clauses.size(); i++) {
        if (solver.clauses[i] != NULL) {
            if (solver.clauses[i]->size() == 2)
                solver.binaryClauses.push(solver.clauses[i]);
            else
                solver.clauses[j++] = solver.clauses[i];
        }
    }
    solver.clauses.shrink(i-j);
    solver.clauseCleaner->cleanClauses(solver.clauses, ClauseCleaner::clauses);
    
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
    
    printf("DONE!\n");
    std::cout << " Simplification time:" << cpuTime() - time << std::endl;
}

void Simplifier::findSubsumed(Clause& ps, vec<Clause*>& out_subsumed)
{
    #ifdef VERBOSE_DEBUG
    cout << "findSubsumed: ";
    for (uint i = 0; i < ps.size(); i++) {
        if (ps[i].sign()) printf("-");
        printf("%d ", ps[i].var() + 1);
    }
    printf("0\n");
    #endif
    
    uint64_t  abst;
    abst = calcAbstraction(ps);
    
    int min_i = 0;
    for (int i = 1; i < ps.size(); i++){
        if (occur[ps[i].toInt()].size() < occur[ps[min_i].toInt()].size())
            min_i = i;
    }
    
    vec<Clause*>& cs = occur[ps[min_i].toInt()];
    for (int i = 0; i < cs.size(); i++){
        if (cs[i] != &ps && ps.size() <= cs[i]->size() && subset(abst, cs[i]->abst()) && subset(ps, *cs[i], seen_tmp)) {
            out_subsumed.push(cs[i]);
            #ifdef VERBOSE_DEBUG
            cout << "subsumed: ";
            cs[i]->plainPrint();
            #endif
        }
    }
}

void Simplifier::findSubsumed(vec<Lit>& ps, vec<Clause*>& out_subsumed)
{
    #ifdef VERBOSE_DEBUG
    cout << "findSubsumed: ";
    for (uint i = 0; i < ps.size(); i++) {
        if (ps[i].sign()) printf("-");
        printf("%d ", ps[i].var() + 1);
    }
    printf("0\n");
    #endif
    
    uint64_t  abst;
    abst = calcAbstraction(ps);
    
    int min_i = 0;
    for (int i = 1; i < ps.size(); i++){
        if (occur[ps[i].toInt()].size() < occur[ps[min_i].toInt()].size())
            min_i = i;
    }
    
    vec<Clause*>& cs = occur[ps[min_i].toInt()];
    for (int i = 0; i < cs.size(); i++){
        if (ps.size() <= cs[i]->size() && subset(abst, cs[i]->abst()) && subset(ps, *cs[i], seen_tmp)) {
            out_subsumed.push(cs[i]);
            #ifdef VERBOSE_DEBUG
            cout << "subsumed: ";
            cs[i]->plainPrint();
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