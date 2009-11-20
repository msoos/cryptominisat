/**************************************************************************************************

Solver.C -- (C) Niklas Een, Niklas Sörensson, 2004

A simple Chaff-like SAT-solver with support for incremental SAT.

**************************************************************************************************/

#include "Solver.h"
#include "Main.h"
#include "Sort.h"
#include <cmath>

#include "Solver_clause.iC"    // (purely for efficiency reasons!)


//=================================================================================================
// Minor methods:


// Creates a new SAT variable in the solver. If 'decision_var' is cleared, variable will not be
// used as a decision variable (NOTE! This has effects on the meaning of a SATISFIABLE result).
//
Var Solver::newVar(void)
{
    int     index;
    index = nVars();
    watches     .push();        // (list for positive literal)
    watches     .push();        // (list for negative literal)
    occur       .push();
    occur       .push();
    n_occurs    .push(0);
    n_occurs    .push(0);
    reason      .push(Clause_NULL);
    assigns     .push(toInt(l_Undef));
    level       .push(-1);
    activity    .push(0);
  #ifdef VAR_ORDER_2
    lt_activity .push(0);
  #endif
    order       .newVar();
    seen_tmp    .push(0);       // (one for each polarity)
    seen_tmp    .push(0);
    touched     .push(1);
    touched_list.push(index);
    touched_tmp .push(0);
    var_elimed  .push(0);
    frozen      .push(0);
    return index;
}


// Can only be used if no variables have been created (no previous calls to 'newVar()' or 'setVars()').
//
void Solver::setVars(int n_vars)
{
    // Hmm, doesn't work very well... Debug later
    assert(nVars() == 0);
    watches     .growTo(2*n_vars);
    occur       .growTo(2*n_vars);
    n_occurs    .growTo(2*n_vars, 0);
    reason      .growTo(n_vars  , Clause_NULL);
    assigns     .growTo(n_vars  , toInt(l_Undef));
    level       .growTo(n_vars  , -1);
    activity    .growTo(n_vars, 0);
  #ifdef VAR_ORDER_2
    lt_activity .growTo(n_vars, 0);
  #endif
    seen_tmp    .growTo(2*n_vars, 0);
    touched     .growTo(n_vars, 1);
    touched_list.growTo(n_vars); for (int i = 0; i < n_vars; i++) touched_list[i] = i;
    touched_tmp .growTo(n_vars, 0);
    var_elimed  .growTo(n_vars, 0);
    frozen      .growTo(n_vars, 0);

    for (int i = 0; i < n_vars; i++)
        order.newVar();
}


// Returns FALSE if immediate conflict.
bool Solver::assume(Lit p) {
    assert(propQ.size() == 0);
    if (verbosity >= 2) reportf(L_IND"assume("L_LIT")\n", L_ind, L_lit(p));
    trail_lim.push(trail.size());
    return enqueue(p); }


// Revert one variable binding on the trail.
//
inline void Solver::undoOne(void)
{
    if (verbosity >= 2){ Lit p = trail.last(); reportf(L_IND"unbind("L_LIT")\n", L_ind, L_lit(p)); }
    Lit     p  = trail.last(); trail.pop();
    Var     x  = var(p);
    assigns[x] = toInt(l_Undef);
    reason [x] = Clause_NULL;
    level  [x] = -1;
    order.undo(x);
}


// Reverts to the state before last 'assume()'.
//
void Solver::cancel(void)
{
    assert(propQ.size() == 0);
    if (verbosity >= 2){ if (trail.size() != trail_lim.last()){ Lit p = trail[trail_lim.last()]; reportf(L_IND"cancel("L_LIT")\n", L_ind, L_lit(p)); } }
    for (int c = trail.size() - trail_lim.last(); c != 0; c--)
        undoOne();
    trail_lim.pop();
}


// Revert to the state at given level.
//
void Solver::cancelUntil(int level) {
    while (decisionLevel() > level) cancel(); }


// Record a clause and drive backtracking. 'clause[0]' must contain the asserting literal.
//
void Solver::record(const vec<Lit>& clause)
{
    assert(clause.size() != 0);
    Clause c = addClause(clause, true); assert(ok);
    check(enqueue(clause[0], c));
}


//=================================================================================================
// Major methods:


Solver::~Solver(void)
{
    assert(iter_vecs.size() == 0); assert(iter_sets.size() == 0);
    for (int i = 0; i < constrs.size(); i++) if (!constrs[i].null()) deallocClause(constrs[i], true);
    for (int i = 0; i < learnts.size(); i++) if (!learnts[i].null()) deallocClause(learnts[i], true);
    deleteTmpFiles();
}


/*_________________________________________________________________________________________________
|                                                                                                  
|  analyze : (confl : Clause) (out_learnt : vec<Lit>&) (out_btlevel : int&)  ->  [void]            
|                                                                                                  
|  Description:                                                                                    
|    Analyze conflict and produce a reason clause.                                                 
|                                                                                                  
|    Pre-conditions:                                                                               
|      * 'out_learnt' is assumed to be cleared.                                                    
|      * Current decision level must be greater than root level.                                   
|                                                                                                  
|    Post-conditions:                                                                              
|      * 'out_learnt[0]' is the asserting literal at level 'out_btlevel'.                          
|                                                                                                  
|  Effect:                                                                                         
|    Will undo part of the trail, upto but not beyond the assumption of the current decision level.
|________________________________________________________________________________________________@*/
void Solver::analyze(Clause confl, vec<Lit>& out_learnt, int& out_btlevel)
{
    vec<char>&  seen = seen_tmp;
    int         pathC    = 0;
    Lit         p = lit_Undef;
    vec<Lit>    p_reason;

    // Generate conflict clause:
    //
#if 1
// Niklas Sörensson's version
    // Generate conflict clause:
    //
    out_learnt.push();      // (leave room for the asserting literal)
    out_btlevel = 0;
    int index = trail.size()-1;
    do{
        assert(confl != Clause_NULL);          // (otherwise should be UIP)
        if (confl.learnt()) claBumpActivity(confl);

        for (int j = p == lit_Undef ? 0 : 1; j < confl.size(); j++){
            Lit q = confl(j);
            if (!seen[var(q)] && level[var(q)] > 0){
                seen[var(q)] = 1;
                varBumpActivity(q);
                if (level[var(q)] == decisionLevel())
                    pathC++;
                else{
                    out_learnt.push(q),
                    out_btlevel = max(out_btlevel, level[var(q)]);
                }
            }
        }

        // Select next clause to look at:
        while (!seen[var(trail[index--])]);
        p     = trail[index+1];
        confl = reason[var(p)];
        seen[var(p)] = 0;
        pathC--;

    }while (pathC > 0);
    out_learnt[0] = ~p;

#else
    out_learnt.push();      // (leave room for the asserting literal)
    out_btlevel = 0;
    do{
        assert(!confl.null());          // (otherwise should be UIP)

        p_reason.clear();
        calcReason(confl, p, p_reason);

        for (int j = 0; j < p_reason.size(); j++){
            Lit q = p_reason[j];
            if (!seen[var(q)] && level[var(q)] > 0){
              #ifdef BUMP_MORE
                varBumpActivity(q);
              #endif
                seen[var(q)] = 1;
                if (level[var(q)] == decisionLevel())
                    pathC++;
                else{
                    out_learnt.push(~q),
                    out_btlevel = max(out_btlevel, level[var(q)]);
                }
            }
        }

        // Select next clause to look at:
        do{
            p = trail.last();
            confl = reason[var(p)];
            undoOne();
        }while (!seen[var(p)]);
        pathC--;
        seen[var(p)] = 0;
    }while (pathC > 0);
    out_learnt[0] = ~p;
#endif

  #ifndef BUMP_MORE
    // Bump variables:
    for (int i = 0; i < out_learnt.size(); i++)
        varBumpActivity(out_learnt[i]);
  #endif

    // Remove literals:
//#define DEBUG_CSK
  #ifdef DEBUG_CSK
    vec<char>   seen_copy; seen.copyTo(seen_copy);
    vec<Lit>    out_learnt_copy; out_learnt.copyTo(out_learnt_copy);
    vec<Lit>    candidate;
  #endif

    vec<Var>    to_clear(out_learnt.size()); for (int i = 0; i < out_learnt.size(); i++) to_clear[i] = var(out_learnt[i]);
    if (opt_confl_ksub){
        vec<int>    levels;
        for (int i = 1; i < out_learnt.size(); i++)
            levels.push(level[var(out_learnt[i])]);
        sortUnique(levels);

        for (int k = 0; k < levels.size(); k++){
            assert(levels[k] > 0);
            int from = trail_lim[levels[k]-1];
            int to   = (levels[k] >= trail_lim.size()) ? trail.size() : trail_lim[levels[k]];
            for (int i = from; i < to; i++){
                Var     x = var(trail[i]);
                Clause  r = reason[x];
                if (!r.null()){
                    for (int j = 1; j < r.size(); j++)
                        if (seen[var(r[j])] == 0 && level[var(r[j])] != 0)
                            goto NoConsequence;
                    seen[x] = 2;
                    to_clear.push(x);
                  NoConsequence:;
                }
            }
        }
        for (int i = 0; i < out_learnt.size(); i++)
            if (seen[var(out_learnt[i])] == 2)
                out_learnt[i] = lit_Undef;

      #ifdef DEBUG_CSK
        for (int i = 0; i < out_learnt.size(); i++)
            if (out_learnt[i] != lit_Undef)
                candidate.push(out_learnt[i]);
        seen_copy.copyTo(seen);
        out_learnt_copy.copyTo(out_learnt);

        for (int i = 0; i < out_learnt.size(); i++){
            Clause c = reason[var(out_learnt[i])];
            if (!c.null()){
                assert(c[0] == ~out_learnt[i]);
                for (int j = 1; j < c.size(); j++){
                    if (seen[var(c[j])] != 1)
                        goto Done2;
                }
                out_learnt[i] = lit_Undef;
              Done2:;
            }
        }
      #endif

    }else if (opt_confl_1sub){
        for (int i = 0; i < out_learnt.size(); i++){
            Clause c = reason[var(out_learnt[i])];
            if (!c.null()){
                assert(c[0] == ~out_learnt[i]);
                for (int j = 1; j < c.size(); j++){
                    if (seen[var(c[j])] != 1)
                        goto Done;
                }
                out_learnt[i] = lit_Undef;
              Done:;
            }
        }
    }
    if (opt_confl_1sub || opt_confl_ksub){
        int new_sz = 0;
        for (int i = 0; i < out_learnt.size(); i++)
            if (out_learnt[i] != lit_Undef)
                out_learnt[new_sz++] = out_learnt[i];
        out_learnt.shrink(out_learnt.size() - new_sz);
    }
  #ifdef DEBUG_CSK
    if (candidate.size() > out_learnt.size()){
        reportf("csk: "), dump(candidate);
        reportf("cs1: "), dump(out_learnt);
        exit(0);
    }
  #endif


    // Clear 'seen':
    for (int j = 0; j < to_clear.size(); j++) seen[to_clear[j]] = 0;    // ('seen[]' is now cleared)

    if (verbosity >= 2){
        reportf(L_IND"Learnt {", L_ind);
        for (int i = 0; i < out_learnt.size(); i++) reportf(" "L_LIT, L_lit(out_learnt[i]));
        reportf(" } at level %d\n", out_btlevel); }
}


/*_________________________________________________________________________________________________
|                                                                                                  
|  enqueue : (p : Lit) (from : Clause)  ->  [bool]                                                 
|                                                                                                  
|  Description:                                                                                    
|    Puts a new fact on the propagation queue as well as immediately updating the variable's value.
|    Should a conflict arise, FALSE is returned.                                                   
|                                                                                                  
|  Input:                                                                                          
|    p    - The fact to enqueue                                                                    
|    from - [Optional] Fact propagated from this (currently) unit clause. Stored in 'reason[]'.    
|           Default value is NULL (no reason).                                                     
|                                                                                                  
|  Output:                                                                                         
|    TRUE if fact was enqueued without conflict, FALSE otherwise.                                  
|________________________________________________________________________________________________@*/
bool Solver::enqueue(Lit p, Clause from)
{
    if (value(p) != l_Undef){
      #ifdef RELEASE
        return value(p) != l_False;
      #else
        if (value(p) == l_False){
            // Conflicting enqueued assignment
            assert(decisionLevel() > 0);
            return false;
        }else if (value(p) == l_True){
            // Existing consistent assignment -- don't enqueue
            return true;
        }else{
            assert(value(p) == l_Error);
            // Do nothing -- clause should be removed.
            return true;
        }
      #endif
    }else{
        // New fact -- store it.
      #ifndef RELEASE
        if (verbosity >= 2) reportf(L_IND"bind("L_LIT")\n", L_ind, L_lit(p));
      #endif
        assigns[var(p)] = toInt(lbool(!sign(p)));
        level  [var(p)] = decisionLevel();
        reason [var(p)] = from;
        trail.push(p);
        propQ.insert(p);
        return true;
    }
}


/*_________________________________________________________________________________________________
|                                                                                                  
|  propagateToplevel : [void]  ->  [void]                                                          
|                                                                                                  
|  Description:                                                                                    
|    Destructively update clause database with enqueued top-level facts.                           
|________________________________________________________________________________________________@*/
void Solver::propagateToplevel(void)
{
    assert(decisionLevel() == 0);

    for (int i = 0; i < units.size(); i++){
        if (!enqueue(units[i])){
            propQ.clear();
            return; } }
    units.clear();

    while (propQ.size() > 0){
        Lit            p = propQ.dequeue();
        vec<Clause>    cs;

        // Remove satisfied clauses:
        occur[index(p)].moveTo(cs);
        for (int i = 0; i < cs.size(); i++)
            removeClause(cs[i]);

        // Remove false literals from clauses:
        occur[index(~p)].moveTo(cs);
        registerIteration(cs);
        for (int i = 0; i < cs.size(); i++){
            if (!cs[i].null())
                strengthenClause(cs[i], ~p);    // (may enqueue new facts to propagate)
        }
        unregisterIteration(cs);
    }
    propQ.clear();
}


/*_________________________________________________________________________________________________
|                                                                                                  
|  propagate : [void]  ->  [Clause]                                                                
|                                                                                                  
|  Description:                                                                                    
|    Propagates all enqueued facts. If a conflict arises, the conflicting clause is returned,      
|    otherwise NULL.                                                                               
|                                                                                                  
|    Post-conditions:                                                                              
|      * the propagation queue is empty, even if there was a conflict.                             
|________________________________________________________________________________________________@*/
#if 0
// Standard
Clause Solver::propagate(void)
{
    if (decisionLevel() == 0 && occur_mode != occ_Off){
        propagateToplevel();
        return Clause_NULL;
    }

    Clause confl;
    while (propQ.size() > 0){
        stats.propagations++;
        Lit           p  = propQ.dequeue();        // 'p' is enqueued fact to propagate.
        vec<Clause>&  ws = watches[index(p)];
        bool          keep_watch;
        int           i, j;
        for (i = j = 0; confl.null() && i < ws.size(); i++){
            stats.inspects++;
            keep_watch = false;
            if (!propagateClause(ws[i], p, keep_watch))
                confl = ws[i],
                propQ.clear();
            if (keep_watch)
                ws[j++] = ws[i];
        }

        // Copy the remaining watches:
        while (i < ws.size())
            ws[j++] = ws[i++];

        watches[index(p)].shrink(i - j);
    }

    return confl;
}
#endif

#if 0
// Optimized
Clause Solver::propagate(void)
{
    if (decisionLevel() == 0 && occur_mode != occ_Off){
        propagateToplevel();
        return Clause_NULL;
    }

    Clause confl;
    while (propQ.size() > 0){
        stats.propagations++;
        Lit           p  = propQ.dequeue();        // 'p' is enqueued fact to propagate.
        vec<Clause>&  ws = watches[index(p)];
        int           i, j;
        for (i = j = 0; i < ws.size(); i++){
            stats.inspects++;

            // Make sure the false literal is c[1]:
            Clause& c = ws[i];
            Lit     false_lit = ~p;
            if (c(0) == false_lit)
                c(0) = c(1), c(1) = false_lit;
            assert(c(1) == false_lit);

            // If 0th watch is true, then clause is already satisfied.
            if (value(c(0)) == l_True){
                ws[j++] = ws[i];
                goto Continue; }

            // Look for new watch:
            for (int k = 2; k < c.size(); k++){
                if (value(c(k)) != l_False){
                    c(1) = c(k), c(k) = false_lit;
                    watches[index(~c(1))].push(c);
                    goto Continue; } }

            // Clause is unit under assignment:
            ws[j++] = ws[i];
            if (!enqueue(c(0), c)){
                confl = ws[i],
                propQ.clear();
                i++; break;
            }

          Continue:;
        }

        // Copy the remaining watches:
        while (i < ws.size())
            ws[j++] = ws[i++];

        watches[index(p)].shrink(i - j);
    }

    return confl;
}
#endif

#if 1
// Borrowed from Niklas Sörensson -- uses "unsafe" type casts to achieve maximum performance
Clause Solver::propagate(void)
{
    if (decisionLevel() == 0 && occur_mode != occ_Off){
        propagateToplevel();
        return Clause_NULL;
    }
    assert(watches_setup);

    Clause confl = Clause_NULL;
    while (propQ.size() > 0){
        stats.propagations++;
        Lit           p  = propQ.dequeue();        // 'p' is enqueued fact to propagate.
        vec<Clause>&  ws = watches[index(p)];
        Clause_t      **i, **j, **end = (Clause_t**)(Clause*)ws + ws.size();
        for (i = j = (Clause_t**)(Clause*)ws; confl == NULL && i < end; ){
            stats.inspects++;
            Clause_t& c = **i;

            // Make sure the false literal is data[1]:
            Lit false_lit = ~p;
            if (c(0) == false_lit)
                c(0) = c(1), c(1) = false_lit;

            assert(c(1) == false_lit);

            // If 0th watch is true, then clause is already satisfied.
            if (value(c(0)) == l_True)
                *j++ = *i;
            else{
                // Look for new watch:
                for (int k = 2; k < c.size(); k++)
                    if (value(c(k)) != l_False){
                        c(1) = c(k); c(k) = false_lit;
                        watches[index(~c(1))].push(&c);
                        goto next; }

                // Clause is unit under assignment:
                *j++ = *i;
                if (!enqueue(c(0), &c)){
                    confl = *(Clause*)i;
                    propQ.clear();
                }
            }
        next:
            i++;
        }

        // Copy the remaining watches:
        while (i < end)
            *j++ = *i++;

        ws.shrink(i - j);
    }

    return confl;
}
#endif


/*_________________________________________________________________________________________________
|                                                                                                  
|  reduceDB : ()  ->  [void]                                                                       
|                                                                                                  
|  Description:                                                                                    
|    Remove half of the learnt clauses, minus the clauses locked by the current assignment. Locked 
|    clauses are clauses that are reason to a some assignment.                                     
|________________________________________________________________________________________________@*/
bool satisfied(Solver& S, Clause c)
{
    for (int i = 0; i < c.size(); i++){
        if ((S.value(c[i]) == l_True && S.level[var(c[i])] == 0) || S.value(c[i]) == l_Error)    // (l_Error means variable is eliminated)
            return true; }
    return false;
}

struct reduceDB_lt { bool operator () (Clause x, Clause y) { return x.activity() < y.activity(); } };
void Solver::reduceDB(void)
{
/**/if (occur_mode == occ_All) return;  // <<= Temporary fix

    // <<= BAD IMPLEMENTATION! FIX!
    stats.reduceDBs++;
    int     i;
    double  extra_lim = cla_inc / learnts.size();    // Remove any clause below this activity
    vec<Clause> ls;
    for (int i = 0; i < learnts.size(); i++)
        if (!learnts[i].null())
             ls.push(learnts[i]);

    sort(ls,
    reduceDB_lt());
    for (i = 0; i < ls.size() / 3; i++){
        if (!locked(ls[i]))
            removeClause(ls[i]);
    }
    for (; i < ls.size(); i++){
//      if (!locked(ls[i]) && ls[i].activity() < extra_lim)
        if (!locked(ls[i]) && (ls[i].activity() < extra_lim || satisfied(*this, ls[i])))
            removeClause(ls[i]);
    }
}


void Solver::compressDB(void)
{
    // UNFINISHED
    vec<Pair<int,Var> > n_occurs(2*nVars());

    for (int i = 0; i < n_occurs.size(); i++)
        n_occurs[i].fst = 0,
        n_occurs[i].snd = i;

    for (int i = 0; i < learnts.size(); i++){
        Clause  c = learnts[i];
        if (c == Clause_NULL) goto Skip;
        for (int j = 0; j < c.size(); j++)
            if (value(c[j]) == l_Error) goto Skip;

     for (int j = 0; j < c.size(); j++)
            n_occurs[index(c[j])].fst--;
        Skip:;
    }

    sort(n_occurs);

    /**/for (int i = 0; i < n_occurs.size(); i++) if (n_occurs[i].fst != 0) reportf("%d ", n_occurs[i].fst); reportf("\n"); exit(0);
}


/*_________________________________________________________________________________________________
|                                                                                                  
|  simplifyDB : [void]  ->  [bool]                                                                 
|                                                                                                  
|  Description:                                                                                    
|    Simplify all constraints according to the current top-level assigment (redundant constraints  
|    may be removed altogether).                                                                   
|________________________________________________________________________________________________@*/
void Solver::simplifyDB(bool subsume)
{
    if (!ok) return;    // GUARD (public method)
    assert(decisionLevel() == 0);

    // temporary placement -- put at end of solve?  <<= flytta till 'propagateToplevel()'
    // end

    if (!propagate().null()){   // (cannot use 'propagateToplevel()' here since it behaves different for 'occur_mode == occ_Off')
        assert(ok == false);
        return; }

    //**/if (occur_mode != occ_Off) clauseReduction();

    if (nAssigns() == last_simplify)
        return;
    last_simplify = nAssigns();

    // Subsumption simplification:
    //*HACK!*/if (!subsume && opt_var_elim){ opt_var_elim = false; if (opt_repeated_sub) reportf("                                                                                (var.elim. off)\r"); }
    if (occur_mode != occ_Off){
        if (!opt_repeated_sub){
            if (subsume) simplifyBySubsumption();
        }else
            simplifyBySubsumption();
    }

    // Removed satisfied clauses from the learnt clause database:
    if (stats.inspects - last_inspects > nLearnts() * 32){
        last_inspects = stats.inspects;
        for (int i = 0; i < learnts.size(); i++) if (!learnts[i].null() && satisfied(*this, learnts[i])) removeClause(learnts[i]);
    }

    //**/if (occur_mode != occ_Off) clauseReduction();
}


/*_________________________________________________________________________________________________
|                                                                                                  
|  search : (nof_conflicts : int) (nof_learnts : int) (params : const SearchParams&)  ->  [lbool]  
|                                                                                                  
|  Description:                                                                                    
|    Search for a model the specified number of conflicts, keeping the number of learnt clauses    
|    below the provided limit. NOTE! Use negative value for 'nof_conflicts' or 'nof_learnts' to    
|    indicate infinity.                                                                            
|                                                                                                  
|  Output:                                                                                         
|    'l_True' if a partial assigment that is consistent with respect to the clauseset is found. If 
|    all variables are decision variables, this means that the clause set is satisfiable. 'l_False'
|    if the clause set is unsatisfiable. 'l_Undef' if the bound on number of conflicts is reached. 
|________________________________________________________________________________________________@*/
lbool Solver::search(int nof_conflicts, int nof_learnts, const SearchParams& params)
{
    if (!ok) return l_False;    // GUARD (public method)
    assert(root_level == decisionLevel());

    stats.starts++;
    int     conflictC = 0;
    var_decay = 1 / params.var_decay;
    cla_decay = 1 / params.clause_decay;
    model.clear();

    for (;;){
        Clause confl = propagate();
        if (!confl.null()){
            // CONFLICT

            if (verbosity >= 2) reportf(L_IND"**CONFLICT**\n", L_ind);
            stats.conflicts++; conflictC++;
            vec<Lit>    learnt_clause;
            int         backtrack_level;
            if (decisionLevel() == root_level)
                return l_False;
            analyze(confl, learnt_clause, backtrack_level);
            cancelUntil(max(backtrack_level, root_level));
            record(learnt_clause);
            varDecayActivity(); claDecayActivity();

        }else{
            // NO CONFLICT

            if (nof_conflicts >= 0 && conflictC >= nof_conflicts){
                // Reached bound on number of conflicts:
                progress_estimate = progressEstimate();
                propQ.clear();
                cancelUntil(root_level);
                return l_Undef; }

            if (decisionLevel() == 0 && params.simplify){
                // Simplify the set of problem clauses:
                simplifyDB();
                if (!ok) return l_False;
            }

            if (nof_learnts >= 0 && nLearnts()-nAssigns() >= nof_learnts)
                // Reduce the set of learnt clauses:
                reduceDB();

            // New variable decision:
            stats.decisions++;
            Var next = order.select(params.random_var_freq);

            if (next == var_Undef){
                // Model found:
                model.growTo(nVars());
                if (occur_mode == occ_Off)
                    for (int i = 0; i < nVars(); i++) model[i] = value(i);
                else{
                  #ifdef WATCH_OPTIMIZATION
                    watches.clear(true); watches_setup = false;
                  #endif
                    extendModel();
                }
                cancelUntil(root_level);
                return l_True;
            }

            check(assume(~Lit(next)));
        }
    }
}


/*_________________________________________________________________________________________________
|                                                                                                  
|  extendModel : [void]  ->  [void]                                                                
|                                                                                                  
|  Description:                                                                                    
|    Extend the partial model of the current SAT environment to a full model, reading back         
|    eliminated clauses from the temporary file.                                                   
|________________________________________________________________________________________________@*/
void Solver::extendModel(void)
{
    if (verbosity >= 1){
        reportf("==============================================================================\n");
        reportf("Extending model [cpu-time: %g s]\n", cpuTime()); }

    //**/int64 mem0 = memUsed();
  #if 1
    // READING FILE FORWARDS (simple)
    Solver  S(occ_Off);
    for (int i = 0; i < nVars(); i++){
        S.newVar();
        if (!var_elimed[i] && value(i) != l_Undef)
            S.addUnit(((value(i) == l_True) ? Lit(i) : ~Lit(i)));
    }

    fflush(elim_out);
    rewind(elim_out);
    for(;;){
        Lit     p;
        int     n = fread(&p, 4, 1, elim_out);
        if (n == 0)
            break;
        int     size = index(p);

        io_tmp.clear();
        io_tmp.growTo(size);
        fread((Lit*)io_tmp, 4, size, elim_out);

        S.addClause(io_tmp);
        /**/if (!S.ok) reportf("PANIC! False clause read back: "), dump(S, io_tmp);
        assert(S.ok);
    }
    fflush(elim_out);

    check(S.solve());
    S.model.moveTo(model);

  #else
    // READING FILE BACKWARDS  
    Solver  S(occ_Off);
    S.setupWatches();
    for (int i = 0; i < nVars(); i++){
        S.newVar();
        if (!var_elimed[i] && value(i) != l_Undef)
            S.addUnit(((value(i) == l_True) ? Lit(i) : ~Lit(i)));
    }
    check(S.propagate() == Clause_NULL);

    fflush(elim_out);
    const int       chunk_size = 1000;
    vec<long>       offsets;
    vec<vec<Lit> >  tmps(chunk_size);
    Lit     p;
    int     n, size, c;

    rewind(elim_out);
    offsets.push(ftell(elim_out));
    c = 0;
    for(;;){
        n = fread(&p, 4, 1, elim_out); if (n == 0) break; size = index(p);              // (read size of clause or abort if no more clauses)
        io_tmp.clear(); io_tmp.growTo(size); fread((Lit*)io_tmp, 4, size, elim_out);    // (read clause)
        c++;
        if (c == chunk_size)
            offsets.push(ftell(elim_out)),
            c = 0;
    }

    assert(S.constrs.size() == 0);
    assert(S.propQ.size() == 0);
    rewind(elim_out);
    for (int i = offsets.size() - 1; i >= 0; i--){
        fseek(elim_out, SEEK_SET, offsets[i]);
        for(c = 0; c < chunk_size; c++){
            n = fread(&p, 4, 1, elim_out); if (n == 0) break; size = index(p);              // (read size of clause or abort if no more clauses)
            tmps[c].clear(); tmps[c].growTo(size); fread((Lit*)tmps[c], 4, size, elim_out); // (read clause)
        }
        for (; c > 0; c--){
            assert(S.watches_setup);
            S.addClause(tmps[c-1]); /*DEBUG*/if (!S.ok) reportf("PANIC! False clause read back: "), dump(S, io_tmp); assert(S.ok);
            check(S.propagate() == Clause_NULL);
        }
    }
    fseek(elim_out, SEEK_END, 0);
    fflush(elim_out);

    check(S.solve());
    S.model.moveTo(model);
  #endif

    //**/reportf("MEM USED for extending model: %g MB\n", (memUsed() - mem0) / (1024*1024.0));
}


// Return search-space coverage. Not extremely reliable.
//
double Solver::progressEstimate(void)
{
    int n_vars = 0; for (int i = 0; i < nVars(); i++) n_vars += !var_elimed[i];
    double  progress = 0;
    double  F = 1.0 / n_vars;
    for (int i = 0; i < nVars(); i++)
        if (value(i) != l_Undef && !var_elimed[i])
            progress += pow(F, level[i]);
    return progress / n_vars;
}


// Divide all variable activities by 1e100.
//
void Solver::varRescaleActivity(void)
{
    for (int i = 0; i < nVars(); i++)
        activity[i] *= 1e-100;
    var_inc *= 1e-100;
}


// Divide all constraint activities by 1e100.
//
void Solver::claRescaleActivity(void)
{
    for (int i = 0; i < learnts.size(); i++)
        if (!learnts[i].null())
            learnts[i].activity() *= 1e-20;
    cla_inc *= 1e-20;
}


/*_________________________________________________________________________________________________
|                                                                                                  
|  solve : (assumps : const vec<Lit>&)  ->  [bool]                                                 
|                                                                                                  
|  Description:                                                                                    
|    Top-level solve. If using assumptions (non-empty 'assumps' vector), you must call             
|    'simplifyDB()' first to see that no top-level conflict is present (which would put the solver 
|    in an undefined state).                                                                       
|________________________________________________________________________________________________@*/
bool Solver::solve(const vec<Lit>& assumps)
{
    if (verbosity >= 1){
        reportf("==============================================================================\n");
        reportf("|           |     ORIGINAL     |              LEARNT              |          |\n");
        reportf("| Conflicts | Clauses Literals |   Limit Clauses Literals  Lit/Cl | Progress |\n");
        reportf("==============================================================================\n");
        // Hack:
        double nof_learnts = 0.3;
        int learnt_literals = 0; for (int i = 0; i < learnts.size(); i++) if (!learnts[i].null()) learnt_literals += learnts[i].size();
        reportf("| %9d | %7d %8d | %7d %7d %8d %7.1f | %6.3f %% |\n", (int)stats.conflicts, nClauses(), nLiterals(), (int)(nof_learnts*nClauses()), nLearnts(), learnt_literals, learnt_literals / (double)nLearnts(), progress_estimate*100);
    }

    if (occur_mode == occ_Off || opt_asym_branch) setupWatches();
    simplifyDB(true);
    if (!ok) return false;
    setupWatches();

    SearchParams    params(0.95, 0.999, opt_no_random ? 0 : 0.02);
    double  nof_conflicts = 100;
    double  nof_learnts   = 0.4; //0.3;
    lbool   status        = l_Undef;

    for (int i = 0; i < assumps.size(); i++){
        assert(!var_elimed[var(assumps[i])]);
        if (!propagate().null() || !assume(assumps[i]) || !propagate().null()){
            propQ.clear();
            cancelUntil(0);
            return false; }
    }
    root_level = decisionLevel();

    while (status == l_Undef){
        //if (verbosity >= 1) reportf("RESTART -- conflicts=%d   clauses=%d   learnts=%d/%d   progress=%.4f %%\n", (int)nof_conflicts, constrs.size(), learnts.size(), (int)(nof_learnts*nClauses()), progress_estimate*100);
        if (verbosity >= 1){
            int learnt_literals = 0; for (int i = 0; i < learnts.size(); i++) if (!learnts[i].null()) learnt_literals += learnts[i].size();
            reportf("| %9d | %7d %8d | %7d %7d %8d %7.1f | %6.3f %% |\n", (int)stats.conflicts, nClauses(), nLiterals(), (int)(nof_learnts*nClauses()), nLearnts(), learnt_literals, learnt_literals / (double)nLearnts(), progress_estimate*100);
        }
        status = search((int)nof_conflicts, (int)(nof_learnts*nClauses()), params);
        nof_conflicts *= 1.5;
        nof_learnts   *= 1.1;
        //**/if (learnts.size() > 1000) compressDB();
    }

    if (verbosity >= 1) reportf("==============================================================================\n");

    cancelUntil(0);
    return (status == l_True);
}
