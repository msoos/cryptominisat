/****************************************************************************************[Solver.C]
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include "Solver.h"
#include "Sort.h"
#include <cmath>
#include <string.h>
#include <algorithm>
#include <limits.h>
#include <vector>
#include "clause.h"

//=================================================================================================
// Constructor/Destructor:


Solver::Solver() :
        // Parameters: (formerly in 'SearchParams')
        var_decay(1 / 0.95), clause_decay(1 / 0.999), random_var_freq(0.02)
        , restart_first(100), restart_inc(1.5), learntsize_factor((double)1/(double)3), learntsize_inc(1.1)

        // More parameters:
        //
        , expensive_ccmin  (true)
        , polarity_mode    (polarity_user)
        , verbosity        (0)
        , restrictedPickBranch(0)
        , useRealUnknowns(false)

        // Statistics: (formerly in 'SolverStats')
        //
        , starts(0), decisions(0), rnd_decisions(0), propagations(0), conflicts(0)
        , clauses_literals(0), learnts_literals(0), max_literals(0), tot_literals(0)

        , ok               (true)
        , cla_inc          (1)
        , var_inc          (1)
        , qhead            (0)
        , simpDB_assigns   (-1)
        , simpDB_props     (0)
        , order_heap       (VarOrderLt(activity))
        , progress_estimate(0)
        , remove_satisfied (true)
        , mtrand((unsigned long int)0)
        , logger(verbosity)
        , dynamic_behaviour_analysis(false) //do not document the proof as default
        , learnt_clause_group(0)
{
}


Solver::~Solver()
{
    for (int i = 0; i < learnts.size(); i++) free(learnts[i]);
    for (int i = 0; i < clauses.size(); i++) free(clauses[i]);
    for (int i = 0; i < xorclauses.size(); i++) free(xorclauses[i]);
}

//=================================================================================================
// Minor methods:


// Creates a new SAT variable in the solver. If 'decision_var' is cleared, variable will not be
// used as a decision variable (NOTE! This has effects on the meaning of a SATISFIABLE result).
Var Solver::newVar(bool sign, bool dvar)
{
    int v = nVars();
    watches   .push();          // (list for positive literal)
    watches   .push();          // (list for negative literal)
    xorwatches.push();          // (list for variables in xors)
    reason    .push(NULL);
    assigns   .push(l_Undef);
    level     .push(-1);
    activity  .push(0);
    seen      .push(0);
    polarity  .push((char)sign);

    polarity    .push((char)sign);
    decision_var.push((char)dvar);

    insertVarOrder(v);
    logger.new_var(v);

    return v;
}

bool Solver::addXorClause(vec<Lit>& ps, bool xor_clause_inverted, const uint group, const char* group_name)
{
    assert(decisionLevel() == 0);

    if (dynamic_behaviour_analysis) logger.set_group_name(group, group_name);

    if (!ok)
        return false;

    // Check if clause is satisfied and remove false/duplicate literals:
    sort(ps);
    Lit p;
    int i, j;
    for (i = j = 0, p = lit_Undef; i < ps.size(); i++) {
        while (ps[i].var() >= nVars()) newVar();
        xor_clause_inverted ^= ps[i].sign();
        ps[i] ^= ps[i].sign();

        if (ps[i] == p) {
            //added, but easily removed
            j--;
            p = lit_Undef;
            if (!assigns[ps[i].var()].isUndef())
                xor_clause_inverted ^= assigns[ps[i].var()].getBool();
        } else if (value(ps[i]) == l_Undef) //just add
            ps[j++] = p = ps[i];
        else xor_clause_inverted ^= (value(ps[i]) == l_True); //modify xor_clause_inverted instead of adding
    }
    ps.shrink(i - j);

    if (ps.size() == 0) {
        if (xor_clause_inverted)
            return true;

        if (dynamic_behaviour_analysis) logger.empty_clause(group);
        return ok = false;
    } else if (ps.size() == 1) {
        assert(value(ps[0]) == l_Undef);
        uncheckedEnqueue( (xor_clause_inverted) ? ~ps[0] : ps[0]);
        if (dynamic_behaviour_analysis)
            logger.propagation((xor_clause_inverted) ? ~ps[0] : ps[0], Logger::addclause_type, group);
        return ok = (propagate() == NULL);
    } else {
        learnt_clause_group = std::max(group+1, learnt_clause_group);

        XorClause* c = XorClause::XorClause_new(ps, xor_clause_inverted, group);

        xorclauses.push(c);
        attachClause(*c);
    }

    return true;
}

bool Solver::addClause(vec<Lit>& ps, const uint group, const char* group_name)
{
    assert(decisionLevel() == 0);

    if (dynamic_behaviour_analysis) logger.set_group_name(group, group_name);

    if (!ok)
        return false;

    // Check if clause is satisfied and remove false/duplicate literals:
    sort(ps);
    Lit p;
    int i, j;
    for (i = j = 0, p = lit_Undef; i < ps.size(); i++) {
        while (ps[i].var() >= nVars()) newVar();

        if (value(ps[i]) == l_True || ps[i] == ~p)
            return true;
        else if (value(ps[i]) != l_False && ps[i] != p)
            ps[j++] = p = ps[i];
    }
    ps.shrink(i - j);

    if (ps.size() == 0) {
        if (dynamic_behaviour_analysis) logger.empty_clause(group);
        return ok = false;
    } else if (ps.size() == 1) {
        assert(value(ps[0]) == l_Undef);
        uncheckedEnqueue(ps[0]);
        if (dynamic_behaviour_analysis)
            logger.propagation(ps[0], Logger::addclause_type, group);
        return ok = (propagate() == NULL);
    } else {
        learnt_clause_group = std::max(group+1, learnt_clause_group);

        Clause* c = Clause_new(ps, group, false);

        clauses.push(c);
        attachClause(*c);
    }

    return true;
}

void Solver::attachClause(XorClause& c)
{
    assert(c.size() > 1);

    xorwatches[c[0].var()].push(&c);
    xorwatches[c[1].var()].push(&c);

    if (c.learnt()) learnts_literals += c.size();
    else            clauses_literals += c.size();
}

void Solver::attachClause(Clause& c)
{
    assert(c.size() > 1);

    watches[(~c[0]).toInt()].push(&c);
    watches[(~c[1]).toInt()].push(&c);

    if (c.learnt()) learnts_literals += c.size();
    else            clauses_literals += c.size();
}


void Solver::detachClause(const XorClause& c)
{
    assert(c.size() > 1);
    assert(find(xorwatches[c[0].var()], &c));
    assert(find(xorwatches[c[1].var()], &c));
    remove(xorwatches[c[0].var()], &c);
    remove(xorwatches[c[1].var()], &c);

    if (c.learnt()) learnts_literals -= c.size();
    else            clauses_literals -= c.size();
}

void Solver::detachClause(const Clause& c)
{
    assert(c.size() > 1);
    assert(find(watches[(~c[0]).toInt()], &c));
    assert(find(watches[(~c[1]).toInt()], &c));
    remove(watches[(~c[0]).toInt()], &c);
    remove(watches[(~c[1]).toInt()], &c);
    if (c.learnt()) learnts_literals -= c.size();
    else            clauses_literals -= c.size();
}

template<class T>
void Solver::removeClause(T& c)
{
    detachClause(c);
    free(&c);
}


bool Solver::satisfied(const Clause& c) const
{
    for (uint i = 0; i < c.size(); i++)
        if (value(c[i]) == l_True)
            return true;
    return false;
}

bool Solver::satisfied(const XorClause& c) const
{
    bool final = c.xor_clause_inverted();
    for (uint k = 0; k < c.size(); k++ ) {
        const lbool& val = assigns[c[k].var()];
        if (val.isUndef()) return false;
        final ^= val.getBool();
    }
    return final;
}


// Revert to the state at given level (keeping all assignment at 'level' but not beyond).
//
void Solver::cancelUntil(int level)
{
#ifdef VERBOSE_DEBUG
    cout << "Canceling until level " << level;
    if (level > 0) cout << " sublevel: " << trail_lim[level];
    cout << endl;
#endif
    if (decisionLevel() > level) {
        for (int c = trail.size()-1; c >= trail_lim[level]; c--) {
            Var     x  = trail[c].var();
#ifdef VERBOSE_DEBUG
            cout << "Canceling var " << x+1 << " sublevel:" << c << endl;
#endif
            assigns[x] = l_Undef;
            insertVarOrder(x);
        }
        qhead = trail_lim[level];
        trail.shrink(trail.size() - trail_lim[level]);
        trail_lim.shrink(trail_lim.size() - level);
    }

#ifdef VERBOSE_DEBUG
    cout << "Canceling finished. (now at level: " << decisionLevel() << " sublevel:" << trail.size()-1 << ")" << endl;
#endif
}

//Permutates the clauses in the solver. Very useful to calcuate the average time it takes the solver to solve the prolbem
void Solver::permutateClauses()
{
    for (int i = 0; i < clauses.size(); i++) {
        int j = mtrand.randInt(i);
        Clause* tmp = clauses[i];
        clauses[i] = clauses[j];
        clauses[j] = tmp;
    }

    for (int i = 0; i < xorclauses.size(); i++) {
        int j = mtrand.randInt(i);
        XorClause* tmp = xorclauses[i];
        xorclauses[i] = xorclauses[j];
        xorclauses[j] = tmp;
    }
}

void Solver::setRealUnknown(const uint var)
{
    if (realUnknowns.size() < var+1)
        realUnknowns.resize(var+1, false);
    realUnknowns[var] = true;
}

void Solver::printLit(const Lit l) const
{
    printf("%s%d:%c", l.sign() ? "-" : "", l.var()+1, value(l) == l_True ? '1' : (value(l) == l_False ? '0' : 'X'));
}


void Solver::printClause(const Clause& c) const
{
    printf("(group: %d) ", c.group);
    for (uint i = 0; i < c.size();) {
        printLit(c[i]);
        i++;
        if (i < c.size()) printf(" ");
    }
}

void Solver::printClause(const XorClause& c) const
{
    printf("(group: %d) ", c.group);
    if (c.xor_clause_inverted()) printf(" /inverted/ ");
    for (uint i = 0; i < c.size();) {
        printLit(c[i].unsign());
        i++;
        if (i < c.size()) printf(" + ");
    }
}

//=================================================================================================
// Major methods:


Lit Solver::pickBranchLit(int polarity_mode)
{
#ifdef VERBOSE_DEBUG
    cout << "decision level:" << decisionLevel() << " ";
#endif
    Var next = var_Undef;

    // Random decision:
    if (mtrand.randDblExc() < random_var_freq && !order_heap.empty()) {
        if (restrictedPickBranch == 0) next = order_heap[mtrand.randInt(order_heap.size()-1)];
        else next = order_heap[mtrand.randInt(std::min((uint32_t)order_heap.size()-1, restrictedPickBranch))];

        if (assigns[next] == l_Undef && decision_var[next])
            rnd_decisions++;
    }

    // Activity based decision:
    //bool dont_do_bad_decision = false;
    //if (restrictedPickBranch != 0) dont_do_bad_decision = (mtrand.randInt(100) != 0);
    while (next == var_Undef || assigns[next] != l_Undef || !decision_var[next])
        if (order_heap.empty()) {
            next = var_Undef;
            break;
        } else {
            next = order_heap.removeMin();
        }

    bool sign = false;
    switch (polarity_mode) {
    case polarity_true:
        sign = false;
        break;
    case polarity_false:
        sign = true;
        break;
    case polarity_user:
        if (next != var_Undef)
            sign = polarity[next];
        break;
    case polarity_rnd:
        sign = mtrand.randInt(1);
        break;
    default:
        assert(false);
    }

    assert(next == var_Undef || value(next) == l_Undef);

    if (next == var_Undef) {
#ifdef VERBOSE_DEBUG
        cout << "SAT!" << endl;
#endif
        return lit_Undef;
    } else {
        Lit lit(next,sign);
#ifdef VERBOSE_DEBUG
        cout << "decided on: " << lit.var()+1 << " to set:" << !lit.sign() << endl;
#endif
        return lit;
    }
}


/*_________________________________________________________________________________________________
|
|  analyze : (confl : Clause*) (out_learnt : vec<Lit>&) (out_btlevel : int&)  ->  [void]
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
void Solver::analyze(Clause* confl, vec<Lit>& out_learnt, int& out_btlevel)
{
    int pathC = 0;
    Lit p     = lit_Undef;

    // Generate conflict clause:
    //
    out_learnt.push();      // (leave room for the asserting literal)
    int index   = trail.size() - 1;
    out_btlevel = 0;

    do {
        assert(confl != NULL);          // (otherwise should be UIP)
        Clause& c = *confl;

        if (c.learnt())
            claBumpActivity(c);

        for (uint j = (p == lit_Undef) ? 0 : 1; j < c.size(); j++) {
            const Lit& q = c[j];
            const uint my_var = q.var();

            if (!seen[my_var] && level[my_var] > 0) {
                if (!useRealUnknowns || (my_var < realUnknowns.size() && realUnknowns[my_var]))
                    varBumpActivity(my_var);
                seen[my_var] = 1;
                if (level[my_var] >= decisionLevel())
                    pathC++;
                else {
                    out_learnt.push(q);
                    if (level[my_var] > out_btlevel)
                        out_btlevel = level[my_var];
                }
            }
        }

        // Select next clause to look at:
        while (!seen[trail[index--].var()]);
        p     = trail[index+1];
        confl = reason[p.var()];
        seen[p.var()] = 0;
        pathC--;

    } while (pathC > 0);
    out_learnt[0] = ~p;

    // Simplify conflict clause:
    //
    int i, j;
    if (expensive_ccmin) {
        uint32_t abstract_level = 0;
        for (i = 1; i < out_learnt.size(); i++)
            abstract_level |= abstractLevel(out_learnt[i].var()); // (maintain an abstraction of levels involved in conflict)

        out_learnt.copyTo(analyze_toclear);
        for (i = j = 1; i < out_learnt.size(); i++)
            if (reason[out_learnt[i].var()] == NULL || !litRedundant(out_learnt[i], abstract_level))
                out_learnt[j++] = out_learnt[i];
    } else {
        out_learnt.copyTo(analyze_toclear);
        for (i = j = 1; i < out_learnt.size(); i++) {
            const Clause& c = *reason[out_learnt[i].var()];
            for (uint k = 1; k < c.size(); k++)
                if (!seen[c[k].var()] && level[c[k].var()] > 0) {
                    out_learnt[j++] = out_learnt[i];
                    break;
                }
        }
    }
    max_literals += out_learnt.size();
    out_learnt.shrink(i - j);
    tot_literals += out_learnt.size();

    // Find correct backtrack level:
    //
    if (out_learnt.size() == 1)
        out_btlevel = 0;
    else {
        int max_i = 1;
        for (int i = 2; i < out_learnt.size(); i++)
            if (level[out_learnt[i].var()] > level[out_learnt[max_i].var()])
                max_i = i;
        Lit p             = out_learnt[max_i];
        out_learnt[max_i] = out_learnt[1];
        out_learnt[1]     = p;
        out_btlevel       = level[p.var()];
    }


    for (int j = 0; j < analyze_toclear.size(); j++) seen[analyze_toclear[j].var()] = 0;    // ('seen[]' is now cleared)
}


// Check if 'p' can be removed. 'abstract_levels' is used to abort early if the algorithm is
// visiting literals at levels that cannot be removed later.
bool Solver::litRedundant(Lit p, uint32_t abstract_levels)
{
    analyze_stack.clear();
    analyze_stack.push(p);
    int top = analyze_toclear.size();
    while (analyze_stack.size() > 0) {
        assert(reason[analyze_stack.last().var()] != NULL);
        const Clause& c = *reason[analyze_stack.last().var()];
        analyze_stack.pop();

        for (uint i = 1; i < c.size(); i++) {
            Lit p  = c[i];
            if (!seen[p.var()] && level[p.var()] > 0) {
                if (reason[p.var()] != NULL && (abstractLevel(p.var()) & abstract_levels) != 0) {
                    seen[p.var()] = 1;
                    analyze_stack.push(p);
                    analyze_toclear.push(p);
                } else {
                    for (int j = top; j < analyze_toclear.size(); j++)
                        seen[analyze_toclear[j].var()] = 0;
                    analyze_toclear.shrink(analyze_toclear.size() - top);
                    return false;
                }
            }
        }
    }

    return true;
}


/*_________________________________________________________________________________________________
|
|  analyzeFinal : (p : Lit)  ->  [void]
|
|  Description:
|    Specialized analysis procedure to express the final conflict in terms of assumptions.
|    Calculates the (possibly empty) set of assumptions that led to the assignment of 'p', and
|    stores the result in 'out_conflict'.
|________________________________________________________________________________________________@*/
void Solver::analyzeFinal(Lit p, vec<Lit>& out_conflict)
{
    out_conflict.clear();
    out_conflict.push(p);

    if (decisionLevel() == 0)
        return;

    seen[p.var()] = 1;

    for (int i = trail.size()-1; i >= trail_lim[0]; i--) {
        Var x = trail[i].var();
        if (seen[x]) {
            if (reason[x] == NULL) {
                assert(level[x] > 0);
                out_conflict.push(~trail[i]);
            } else {
                const Clause& c = *reason[x];
                for (uint j = 1; j < c.size(); j++)
                    if (level[c[j].var()] > 0)
                        seen[c[j].var()] = 1;
            }
            seen[x] = 0;
        }
    }

    seen[p.var()] = 0;
}


void Solver::uncheckedEnqueue(Lit p, Clause* from)
{
#ifdef VERBOSE_DEBUG
    cout << "uncheckedEnqueue var " << p.var()+1 << " to " << !p.sign() << " level: " << decisionLevel() << " sublevel:" << trail.size() << endl;
#endif
    assert(value(p) == l_Undef);
    const Var v = p.var();
    assigns [v] = boolToLBool(!p.sign());//lbool(!sign(p));  // <<== abstract but not uttermost effecient
    level   [v] = decisionLevel();
    reason  [v] = from;
    polarity[p.var()] = p.sign();
    trail.push(p);
}


/*_________________________________________________________________________________________________
|
|  propagate : [void]  ->  [Clause*]
|
|  Description:
|    Propagates all enqueued facts. If a conflict arises, the conflicting clause is returned,
|    otherwise NULL.
|
|    Post-conditions:
|      * the propagation queue is empty, even if there was a conflict.
|________________________________________________________________________________________________@*/
Clause* Solver::propagate(const bool xor_as_well)
{
    Clause* confl = NULL;
    int     num_props = 0;

    while (qhead < trail.size()) {
        Lit            p   = trail[qhead++];     // 'p' is enqueued fact to propagate.
        vec<Clause*>&  ws  = watches[p.toInt()];
        Clause         **i, **j, **end;
        num_props++;


        for (i = j = ws.getData(), end = i + ws.size();  i != end;) {
            Clause& c = **i++;

            // Make sure the false literal is data[1]:
            const Lit false_lit(~p);
            if (c[0] == false_lit)
                c[0] = c[1], c[1] = false_lit;

            assert(c[1] == false_lit);

            // If 0th watch is true, then clause is already satisfied.
            const Lit& first = c[0];
            if (value(first) == l_True) {
                *j++ = &c;
            } else {
                // Look for new watch:
                for (uint k = 2; k < c.size(); k++)
                    if (value(c[k]) != l_False) {
                        c[1] = c[k];
                        c[k] = false_lit;
                        watches[(~c[1]).toInt()].push(&c);
                        goto FoundWatch;
                    }

                // Did not find watch -- clause is unit under assignment:
                *j++ = &c;
                if (value(first) == l_False) {
                    confl = &c;
                    qhead = trail.size();
                    // Copy the remaining watches:
                    while (i < end)
                        *j++ = *i++;
                } else {
                    uncheckedEnqueue(first, &c);
                    if (dynamic_behaviour_analysis)
                        logger.propagation(first,Logger::simple_propagation_type,c.group);
                }
            }
FoundWatch:
            ;
        }
        ws.shrink(i - j);

        if (xor_as_well && !confl) confl = propagate_xors(p);
    }
    propagations += num_props;
    simpDB_props -= num_props;

    return confl;
}

Clause* Solver::propagate_xors(const Lit& p)
{
    Clause* confl = NULL;
#ifdef VERBOSE_DEBUG_XOR
    cout << "Propagating variable " <<  p.var() << endl;
#endif

    vec<XorClause*>&  ws  = xorwatches[p.var()];
    XorClause         **i, **j, **end;
    for (i = j = ws.getData(), end = i + ws.size();  i != end;) {
        XorClause& c = **i++;

        // Make sure the false literal is data[1]:
        if (c[0].var() == p.var()) {
            Lit tmp(c[0]);
            c[0] = c[1];
            c[1] = tmp;
        }
        assert(c[1].var() == p.var());
#ifdef VERBOSE_DEBUG_XOR
        cout << "--> xor thing -- " << endl;
        printClause(c);
        cout << endl;
#endif
        bool final = c.xor_clause_inverted();
        for (int k = 0, size = c.size(); k < size; k++ ) {
            const lbool& val = assigns[c[k].var()];
            if (val.isUndef() && k >= 2) {
                Lit tmp(c[1]);
                c[1] = c[k];
                c[k] = tmp;
#ifdef VERBOSE_DEBUG_XOR
                cout << "new watch set" << endl << endl;
#endif
                xorwatches[c[1].var()].push(&c);
                goto FoundWatch;
            }

            c[k] = c[k].unsign() ^ val.getBool();
            final ^= val.getBool();
        }


        {
            // Did not find watch -- clause is unit under assignment:
            *j++ = &c;

#ifdef VERBOSE_DEBUG_XOR
            cout << "final: " << boolalpha << final << " - ";
#endif
            if (assigns[c[0].var()].isUndef()) {
                c[0] = c[0].unsign()^final;
#ifdef VERBOSE_DEBUG_XOR
                cout << "propagating ";
                printLit(c[0]);
                cout << endl;
                cout << "propagation clause -- ";
                printClause(*(Clause*)&c);
                cout << endl << endl;
#endif
                uncheckedEnqueue(c[0], (Clause*)&c);
                if (dynamic_behaviour_analysis)
                    logger.propagation(c[0], Logger::simple_propagation_type, c.group);
            } else if (!final) {
#ifdef VERBOSE_DEBUG_XOR
                printf("conflict clause -- ");
                printClause(*(Clause*)&c);
                cout << endl << endl;
#endif
                confl = (Clause*)&c;
                qhead = trail.size();
                // Copy the remaining watches:
                while (i < end)
                    *j++ = *i++;
            } else {
#ifdef VERBOSE_DEBUG_XOR
                printf("xor satisfied\n");
#endif
                Lit tmp(c[0]);
                c[0] = c[1];
                c[1] = tmp;
            }
        }
FoundWatch:
        ;
    }
    ws.shrink(i - j);

    return confl;
}


/*_________________________________________________________________________________________________
|
|  reduceDB : ()  ->  [void]
|
|  Description:
|    Remove half of the learnt clauses, minus the clauses locked by the current assignment. Locked
|    clauses are clauses that are reason to some assignment. Binary clauses are never removed.
|________________________________________________________________________________________________@*/
struct reduceDB_lt {
    bool operator () (Clause* x, Clause* y) {
        return x->size() > 2 && (y->size() == 2 || x->activity() < y->activity());
    }
};
void Solver::reduceDB()
{
    int     i, j;
    double  extra_lim = cla_inc / learnts.size();    // Remove any clause below this activity

    sort(learnts, reduceDB_lt());
    for (i = j = 0; i < learnts.size() / 2; i++) {
        if (learnts[i]->size() > 2 && !locked(*learnts[i]))
            removeClause(*learnts[i]);
        else
            learnts[j++] = learnts[i];
    }
    for (; i < learnts.size(); i++) {
        if (learnts[i]->size() > 2 && !locked(*learnts[i]) && learnts[i]->activity() < extra_lim)
            removeClause(*learnts[i]);
        else
            learnts[j++] = learnts[i];
    }
    learnts.shrink(i - j);
}

const vec<Clause*>& Solver::get_sorted_learnts()
{
    sort(learnts, reduceDB_lt());
    return learnts;
}

template<class T>
void Solver::removeSatisfied(vec<T*>& cs)
{
    int i,j;
    for (i = j = 0; i < cs.size(); i++) {
        if (satisfied(*cs[i]))
            removeClause(*cs[i]);
        else
            cs[j++] = cs[i];
    }
    cs.shrink(i - j);
}

void Solver::cleanClauses(vec<Clause*>& cs)
{
    uint useful = 0;
    for (int s = 0; s < cs.size(); s++) {
        Clause& c = *cs[s];
        Lit *i, *j, *end;
        uint at = 0;
        for (i = j = c.getData(), end = i + c.size();  i != end; i++, at++) {
            if (value(*i) == l_Undef) {
                *j = *i;
                j++;
            } else assert(at > 1);
            assert(value(*i) != l_True);
        }
        c.shrink(i-j);
        if (i-j > 0) useful++;
    }
#ifdef VERBOSE_DEBUG
    cout << "cleanClauses(Clause) useful:" << useful << endl;
#endif
}

void Solver::cleanClauses(vec<XorClause*>& cs)
{
    uint useful = 0;
    for (int s = 0; s < cs.size(); s++) {
        XorClause& c = *cs[s];
        Lit *i, *j, *end;
        uint at = 0;
        for (i = j = c.getData(), end = i + c.size();  i != end; i++, at++) {
            const lbool& val = assigns[i->var()];
            if (val.isUndef()) {
                *j = *i;
                j++;
            } else /*assert(at>1),*/ c.invert(val.getBool());
        }
        c.shrink(i-j);
        if (i-j > 0) useful++;
    }
#ifdef VERBOSE_DEBUG
    cout << "cleanClauses(XorClause) useful:" << useful << endl;
#endif
}

/*_________________________________________________________________________________________________
|
|  simplify : [void]  ->  [bool]
|
|  Description:
|    Simplify the clause database according to the current top-level assigment. Currently, the only
|    thing done here is the removal of satisfied clauses, but more things can be put here.
|________________________________________________________________________________________________@*/
bool Solver::simplify()
{
    assert(decisionLevel() == 0);

    if (!ok || propagate() != NULL) {
        if (dynamic_behaviour_analysis) logger.end(Logger::unsat_model_found);
        return ok = false;
    }

    if (nAssigns() == simpDB_assigns || (simpDB_props > 0)) {
        return true;
    }

    // Remove satisfied clauses:
    removeSatisfied(learnts);
    if (remove_satisfied) {       // Can be turned off.
        removeSatisfied(clauses);
        removeSatisfied(xorclauses);
    }

    // Remove fixed variables from the variable heap:
    order_heap.filter(VarFilter(*this));

    simpDB_assigns = nAssigns();
    simpDB_props   = clauses_literals + learnts_literals;   // (shouldn't depend on stats really, but it will do for now)

    //cleanClauses(clauses);
    cleanClauses(xorclauses);
    //cleanClauses(learnts);

    return true;
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
lbool Solver::search(int nof_conflicts, int nof_learnts)
{
    assert(ok);
    int         conflictC = 0;
    vec<Lit>    learnt_clause;
    llbool      ret;

    starts++;

    if (dynamic_behaviour_analysis) logger.begin();

    for (;;) {
        Clause* confl = propagate();

        if (confl != NULL) {
            ret = handle_conflict(learnt_clause, confl, conflictC);
            if (ret != l_Nothing) return ret;
        } else {
            ret = new_decision(nof_conflicts, nof_learnts, conflictC);
            if (ret != l_Nothing) return ret;
        }
    }
}

llbool Solver::new_decision(int& nof_conflicts, int& nof_learnts, int& conflictC)
{
    if (nof_conflicts >= 0 && conflictC >= nof_conflicts) {
        // Reached bound on number of conflicts:
        progress_estimate = progressEstimate();
        cancelUntil(0);
        if (dynamic_behaviour_analysis) logger.end(Logger::restarting);
        return l_Undef;
    }

    // Simplify the set of problem clauses:
    if (decisionLevel() == 0 && !simplify()) {
        if (dynamic_behaviour_analysis) logger.end(Logger::unsat_model_found);
        return l_False;
    }

    if (nof_learnts >= 0 && learnts.size()-nAssigns() >= nof_learnts)
        // Reduce the set of learnt clauses:
        reduceDB();

    Lit next = lit_Undef;
    while (decisionLevel() < assumptions.size()) {
        // Perform user provided assumption:
        Lit p = assumptions[decisionLevel()];
        if (value(p) == l_True) {
            // Dummy decision level:
            newDecisionLevel();
            if (dynamic_behaviour_analysis) logger.propagation(p, Logger::assumption_type);
        } else if (value(p) == l_False) {
            analyzeFinal(~p, conflict);
            if (dynamic_behaviour_analysis) logger.end(Logger::unsat_model_found);
            return l_False;
        } else {
            next = p;
            break;
        }
    }

    if (next == lit_Undef) {
        // New variable decision:
        decisions++;
        next = pickBranchLit(polarity_mode);

        if (next == lit_Undef) {
            // Model found:
            if (dynamic_behaviour_analysis) logger.end(Logger::model_found);
            return l_True;
        }
    }

    // Increase decision level and enqueue 'next'
    assert(value(next) == l_Undef);
    newDecisionLevel();
    uncheckedEnqueue(next);
    if (dynamic_behaviour_analysis) logger.propagation(next, Logger::guess_type);

    return l_Nothing;
}

llbool Solver::handle_conflict(vec<Lit>& learnt_clause, Clause* confl, int& conflictC)
{
    int backtrack_level;

    conflicts++;
    conflictC++;
    if (decisionLevel() == 0) {
        if (dynamic_behaviour_analysis) logger.end(Logger::unsat_model_found);
        return l_False;
    }
    learnt_clause.clear();
    analyze(confl, learnt_clause, backtrack_level);
    cancelUntil(backtrack_level);
    if (dynamic_behaviour_analysis) logger.conflict(Logger::simple_confl_type, backtrack_level, confl->group, learnt_clause);
#ifdef VERBOSE_DEBUG
    cout << "Learning:";
    for (uint i = 0; i < learnt_clause.size(); i++) printLit(learnt_clause[i]), cout << " ";
    cout << endl;
    cout << "reverting var " << learnt_clause[0].var()+1 << " to " << !learnt_clause[0].sign() << endl;
#endif
    assert(value(learnt_clause[0]) == l_Undef);
    if (learnt_clause.size() == 1) {
        uncheckedEnqueue(learnt_clause[0]);
        if (dynamic_behaviour_analysis)
            logger.propagation(learnt_clause[0], Logger::learnt_unit_clause_type);
        assert(backtrack_level == 0 && "Unit clause learnt, so must cancel until level 0, right?");
#ifdef VERBOSE_DEBUG
        cout << "Unit clause learnt." << endl;
#endif
    } else {
        Clause* c = Clause_new(learnt_clause, learnt_clause_group++, true);
        learnts.push(c);
        attachClause(*c);
        claBumpActivity(*c);
        if (dynamic_behaviour_analysis) logger.new_group(c->group);
        uncheckedEnqueue(learnt_clause[0], c);

        if (dynamic_behaviour_analysis)
            logger.propagation(learnt_clause[0], Logger::revert_guess_type, c->group);
    }

    varDecayActivity();
    claDecayActivity();

    return l_Nothing;
}


double Solver::progressEstimate() const
{
    double  progress = 0;
    double  F = 1.0 / nVars();

    for (int i = 0; i <= decisionLevel(); i++) {
        int beg = i == 0 ? 0 : trail_lim[i - 1];
        int end = i == decisionLevel() ? trail.size() : trail_lim[i];
        progress += pow(F, i) * (end - beg);
    }

    return progress / nVars();
}


bool Solver::solve(const vec<Lit>& assumps)
{
    model.clear();
    conflict.clear();

    if (!ok) return false;

    assumps.copyTo(assumptions);

    double  nof_conflicts = restart_first;
    double  nof_learnts   = nClauses() * learntsize_factor;
    lbool   status        = l_Undef;

    if (verbosity >= 1) {
        printf("============================[ Search Statistics ]==============================\n");
        printf("| Conflicts |          ORIGINAL         |          LEARNT          | Progress |\n");
        printf("|           |    Vars  Clauses Literals |    Limit  Clauses Lit/Cl |          |\n");
        printf("===============================================================================\n");
    }

    // Search:
    while (status == l_Undef) {
        if (verbosity >= 1) {
            printf("| %9d | %7d %8d %8d | %8d %8d %6.0f | %6.3f %% |", (int)conflicts, order_heap.size(), nClauses(), (int)clauses_literals, (int)nof_learnts, nLearnts(), (double)learnts_literals/nLearnts(), progress_estimate*100), fflush(stdout);
            printf("\n");
        }
        status = search((int)nof_conflicts, (int)nof_learnts);
        nof_conflicts *= restart_inc;
        nof_learnts   *= learntsize_inc;
    }

    if (verbosity >= 1) {
        printf("===============================================================================");
        printf("\n");
    }


    if (status == l_True) {
        // Extend & copy model:
        model.growTo(nVars());
        for (int i = 0; i < nVars(); i++) model[i] = value(i);
#ifndef NDEBUG
        verifyModel();
#endif
    } else {
        assert(status == l_False);
        if (conflict.size() == 0)
            ok = false;
    }

    cancelUntil(0);
    return status == l_True;
}

//=================================================================================================
// Debug methods:


void Solver::verifyModel()
{
    bool failed = false;
    for (int i = 0; i < clauses.size(); i++) {
        Clause& c = *clauses[i];
        for (uint j = 0; j < c.size(); j++)
            if (modelValue(c[j]) == l_True)
                goto next;

        printf("unsatisfied clause: ");
        printClause(*clauses[i]);
        printf("\n");
        failed = true;
next:
        ;
    }

    for (int i = 0; i < xorclauses.size(); i++) {
        XorClause& c = *xorclauses[i];
        bool final = c.xor_clause_inverted();
        for (uint j = 0; j < c.size(); j++)
            final ^= (modelValue(c[j].unsign()) == l_True);
        if (!final) {
            printf("unsatisfied clause: ");
            printClause(*xorclauses[i]);
            printf("\n");
            failed = true;
        }
    }

    assert(!failed);

    printf("Verified %d original clauses.\n", clauses.size() + xorclauses.size());
}


void Solver::checkLiteralCount()
{
    // Check that sizes are calculated correctly:
    int cnt = 0;
    for (int i = 0; i < clauses.size(); i++)
        cnt += clauses[i]->size();

    for (int i = 0; i < xorclauses.size(); i++)
        cnt += xorclauses[i]->size();

    if ((int)clauses_literals != cnt) {
        fprintf(stderr, "literal count: %d, real value = %d\n", (int)clauses_literals, cnt);
        assert((int)clauses_literals == cnt);
    }
}
