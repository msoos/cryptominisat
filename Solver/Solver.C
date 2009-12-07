/****************************************************************************************[Solver.C]
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
CryptoMiniSat -- Copyright (c) 2009 Mate Soos
glucose -- Gilles Audemard, Laurent Simon (2008)

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

#include "Clause.h"
#include "time_mem.h"

#include "VarReplacer.h"
#include "FindUndef.h"
#include "Gaussian.h"
#include "MatrixFinder.h"
#include "Conglomerate.h"
#include "XorFinder.h"
#include "ClauseCleaner.h"

//#define DEBUG_LIB

#ifdef DEBUG_LIB
#include <sstream>
FILE* myoutputfile;
static uint numcalled = 0;
#endif //DEBUG_LIB

//=================================================================================================
// Constructor/Destructor:


Solver::Solver() :
        // Parameters: (formerly in 'SearchParams')
        var_decay(1 / 0.95), random_var_freq(0.02)
        , restart_first(100), restart_inc(1.5), learntsize_factor((double)1/(double)3), learntsize_inc(1)

        // More parameters:
        //
        , expensive_ccmin  (true)
        , polarity_mode    (polarity_user)
        , verbosity        (0)
        , restrictedPickBranch(0)
        , useRealUnknowns  (false)
        , xorFinder        (true)
        , performReplace   (true)
        , greedyUnbound    (false)

        // Statistics: (formerly in 'SolverStats')
        //
        , nbDL2(0), nbBin(0), nbReduceDB(0)
        , starts(0), decisions(0), rnd_decisions(0), propagations(0), conflicts(0)
        , clauses_literals(0), learnts_literals(0), max_literals(0), tot_literals(0)
        

        , ok               (true)
        , var_inc          (1)
        
        , curRestart       (1)
        , conf4Stats       (0)
        , nbclausesbeforereduce (NBCLAUSESBEFOREREDUCE)
        
        , qhead            (0)
        , simpDB_assigns   (-1)
        , simpDB_props     (0)
        , order_heap       (VarOrderLt(activity))
        , progress_estimate(0)
        , remove_satisfied (true)
        , mtrand((unsigned long int)0)
        , logger(verbosity)
        , dynamic_behaviour_analysis(false) //do not document the proof as default
        , maxRestarts(UINT_MAX)
        , MYFLAG           (0)
        , learnt_clause_group(0)
{
    toReplace = new VarReplacer(this);
    conglomerate = new Conglomerate(this);
    clauseCleaner = new ClauseCleaner(*this);
    logger.setSolver(this);
    
    #ifdef DEBUG_LIB
    std::stringstream ss;
    ss << "inputfile" << numcalled << ".cnf";
    myoutputfile = fopen(ss.str().c_str(), "w");
    #endif
}


Solver::~Solver()
{
    for (int i = 0; i < learnts.size(); i++) free(learnts[i]);
    for (int i = 0; i < clauses.size(); i++) free(clauses[i]);
    for (int i = 0; i < xorclauses.size(); i++) free(xorclauses[i]);
    for (uint i = 0; i < gauss_matrixes.size(); i++) delete gauss_matrixes[i];
    for (uint i = 0; i < freeLater.size(); i++) free(freeLater[i]);
    delete toReplace;
    delete conglomerate;
    delete clauseCleaner;
    
    #ifdef DEBUG_LIB
    fclose(myoutputfile);
    #endif //DEBUG_LIB
}

//=================================================================================================
// Minor methods:


// Creates a new SAT variable in the solver. If 'decision_var' is cleared, variable will not be
// used as a decision variable (NOTE! This has effects on the meaning of a SATISFIABLE result).
Var Solver::newVar(bool sign, bool dvar)
{
    Var v = nVars();
    watches   .push();          // (list for positive literal)
    watches   .push();          // (list for negative literal)
    binwatches.push();          // (list for positive literal)
    binwatches.push();          // (list for negative literal)
    xorwatches.push();          // (list for variables in xors)
    reason    .push(NULL);
    assigns   .push(l_Undef);
    level     .push(-1);
    activity  .push(0);
    seen      .push(0);
    permDiff  .push(0);
    polarity  .push_back((char)sign);

    decision_var.push_back(dvar);
    toReplace->newVar();
    conglomerate->newVar();

    insertVarOrder(v);
    if (dynamic_behaviour_analysis)
        logger.new_var(v);
    
    #ifdef DEBUG_LIB
    fprintf(myoutputfile, "c Solver::newVar() called\n");
    #endif //DEBUG_LIB

    return v;
}

bool Solver::addXorClause(vec<Lit>& ps, bool xor_clause_inverted, const uint group, char* group_name, const bool internal)
{
    assert(decisionLevel() == 0);
    #ifdef DEBUG_LIB
    if (!internal) {
        fprintf(myoutputfile, "x");
        for (uint i = 0; i < ps.size(); i++) {
            fprintf(myoutputfile, "%s%d ", ps[i].sign() ? "-" : "", ps[i].var()+1);
        }
        fprintf(myoutputfile, "0\n");
    }
    #endif //DEBUG_LIB

    if (dynamic_behaviour_analysis) logger.set_group_name(group, group_name);

    if (!ok)
        return false;

    // Check if clause is satisfied and remove false/duplicate literals:
    if (toReplace->getNumReplacedLits()) {
        for (int i = 0; i != ps.size(); i++) {
            ps[i] = toReplace->getReplaceTable()[ps[i].var()] ^ ps[i].sign();
        }
    }
    
    sort(ps);
    Lit p;
    int i, j;
    for (i = j = 0, p = lit_Undef; i < ps.size(); i++) {
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

    switch(ps.size()) {
    case 0: {
        if (xor_clause_inverted)
            return true;

        if (dynamic_behaviour_analysis) logger.empty_clause(group);
        return ok = false;
    }
    case 1: {
        assert(value(ps[0]) == l_Undef);
        uncheckedEnqueue(ps[0] ^ xor_clause_inverted);
        if (dynamic_behaviour_analysis)
            logger.propagation((xor_clause_inverted) ? ~ps[0] : ps[0], Logger::add_clause_type, group);
        return ok = (propagate() == NULL);
    }
    case 2: {
        #ifdef VERBOSE_DEBUG
        cout << "--> xor is 2-long, replacing var " << ps[0].var()+1 << " with " << (!xor_clause_inverted ? "-" : "") << ps[1].var()+1 << endl;
        #endif
        
        toReplace->replace(ps, xor_clause_inverted, group);
        break;
    }
    default: {
        learnt_clause_group = std::max(group+1, learnt_clause_group);
        XorClause* c = XorClause_new(ps, xor_clause_inverted, group);
        
        xorclauses.push(c);
        attachClause(*c);
        if (!internal)
            toReplace->newClause();
        break;
    }
    }

    return true;
}

bool Solver::addClause(vec<Lit>& ps, const uint group, char* group_name)
{
    assert(decisionLevel() == 0);
    
    #ifdef DEBUG_LIB
    for (int i = 0; i < ps.size(); i++) {
        fprintf(myoutputfile, "%s%d ", ps[i].sign() ? "-" : "", ps[i].var()+1);
    }
    fprintf(myoutputfile, "0\n");
    #endif //DEBUG_LIB

    if (dynamic_behaviour_analysis)
        logger.set_group_name(group, group_name);

    if (!ok)
        return false;

    // Check if clause is satisfied and remove false/duplicate literals:
    if (toReplace->getNumReplacedLits()) {
        for (int i = 0; i != ps.size(); i++) {
            ps[i] = toReplace->getReplaceTable()[ps[i].var()] ^ ps[i].sign();
        }
    }
    
    sort(ps);
    Lit p;
    int i, j;
    for (i = j = 0, p = lit_Undef; i < ps.size(); i++) {
        if (value(ps[i]) == l_True || ps[i] == ~p)
            return true;
        else if (value(ps[i]) != l_False && ps[i] != p)
            ps[j++] = p = ps[i];
    }
    ps.shrink(i - j);

    if (ps.size() == 0) {
        if (dynamic_behaviour_analysis)
            logger.empty_clause(group);
        return ok = false;
    } else if (ps.size() == 1) {
        assert(value(ps[0]) == l_Undef);
        uncheckedEnqueue(ps[0]);
        if (dynamic_behaviour_analysis)
            logger.propagation(ps[0], Logger::add_clause_type, group);
        return ok = (propagate() == NULL);
    } else {
        learnt_clause_group = std::max(group+1, learnt_clause_group);
        Clause* c = Clause_new(ps, group);

        clauses.push(c);
        attachClause(*c);
        toReplace->newClause();
    }

    return true;
}

void Solver::attachClause(XorClause& c)
{
    assert(c.size() > 2);

    xorwatches[c[0].var()].push(&c);
    xorwatches[c[1].var()].push(&c);

    if (c.learnt()) learnts_literals += c.size();
    else            clauses_literals += c.size();
}

void Solver::attachClause(Clause& c)
{
    assert(c.size() > 1);
    int index0 = (~c[0]).toInt();
    int index1 = (~c[1]).toInt();
    
    if (c.size() == 2) {
        binwatches[index0].push(WatchedBin(&c, c[1]));
        binwatches[index1].push(WatchedBin(&c, c[0]));
    } else {
        watches[index0].push(&c);
        watches[index1].push(&c);
    }

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
    if (c.size() == 2) {
        assert(findWatchedBinCl(binwatches[(~c[0]).toInt()], &c));
        assert(findWatchedBinCl(binwatches[(~c[1]).toInt()], &c));
        
        removeWatchedBinCl(binwatches[(~c[0]).toInt()], &c);
        removeWatchedBinCl(binwatches[(~c[1]).toInt()], &c);
    } else {
        assert(findWatchedCl(watches[(~c[0]).toInt()], &c));
        assert(findWatchedCl(watches[(~c[1]).toInt()], &c));
        
        removeWatchedCl(watches[(~c[0]).toInt()], &c);
        removeWatchedCl(watches[(~c[1]).toInt()], &c);
    }
    
    if (c.learnt()) learnts_literals -= c.size();
    else            clauses_literals -= c.size();
}

void Solver::detachModifiedClause(const Lit lit1, const Lit lit2, const uint origSize, const Clause* address)
{
    assert(origSize > 1);
    
    if (origSize == 2) {
        assert(findWatchedBinCl(binwatches[(~lit1).toInt()], address));
        assert(findWatchedBinCl(binwatches[(~lit2).toInt()], address));
        removeWatchedBinCl(binwatches[(~lit1).toInt()], address);
        removeWatchedBinCl(binwatches[(~lit2).toInt()], address);
    } else {
        assert(find(watches[(~lit1).toInt()], address));
        assert(find(watches[(~lit2).toInt()], address));
        remove(watches[(~lit1).toInt()], address);
        remove(watches[(~lit2).toInt()], address);
    }
    if (address->learnt()) learnts_literals -= origSize;
    else            clauses_literals -= origSize;
}

void Solver::detachModifiedClause(const Var var1, const Var var2, const uint origSize, const XorClause* address)
{
    assert(origSize > 2);
    
    assert(find(xorwatches[var1], address));
    assert(find(xorwatches[var2], address));
    remove(xorwatches[var1], address);
    remove(xorwatches[var2], address);
    if (address->learnt()) learnts_literals -= origSize;
    else            clauses_literals -= origSize;
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
        
        for (Gaussian **gauss = &gauss_matrixes[0], **end= gauss + gauss_matrixes.size(); gauss != end; gauss++)
            (*gauss)->canceling(trail_lim[level]);
        
        for (int c = trail.size()-1; c >= trail_lim[level]; c--) {
            Var     x  = trail[c].var();
            #ifdef VERBOSE_DEBUG
            cout << "Canceling var " << x+1 << " sublevel: " << c << endl;
            #endif
            assigns[x] = l_Undef;
            insertVarOrder(x);
        }
        qhead = trail_lim[level];
        trail.shrink(trail.size() - trail_lim[level]);
        trail_lim.shrink(trail_lim.size() - level);
    }

    #ifdef VERBOSE_DEBUG
    cout << "Canceling finished. (now at level: " << decisionLevel() << " sublevel: " << trail.size()-1 << ")" << endl;
    #endif
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

void Solver::set_gaussian_decision_until(const uint to)
{
    gaussconfig.decision_until = to;
}

//=================================================================================================
// Major methods:


Lit Solver::pickBranchLit(int polarity_mode)
{
    #ifdef VERBOSE_DEBUG
    cout << "decision level: " << decisionLevel() << " ";
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
        assert(decision_var[lit.var()]);
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
void Solver::analyze(Clause* confl, vec<Lit>& out_learnt, int& out_btlevel, int &nbLevels/*, int &merged*/)
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
        if (p != lit_Undef)
            reverse_binary_clause(c);

        for (uint j = (p == lit_Undef) ? 0 : 1; j < c.size(); j++) {
            const Lit& q = c[j];
            const uint my_var = q.var();

            if (!seen[my_var] && level[my_var] > 0) {
                if (!useRealUnknowns || (my_var < realUnknowns.size() && realUnknowns[my_var]))
                    varBumpActivity(my_var);
                seen[my_var] = 1;
                if (level[my_var] >= decisionLevel()) {
                    pathC++;
                    #ifdef UPDATEVARACTIVITY
                    if ( reason[q.var()] != NULL  && reason[q.var()]->learnt() )
                        lastDecisionLevel.push(q);
                    #endif
                } else {
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
            Clause& c = *reason[out_learnt[i].var()];
            reverse_binary_clause(c);
            
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

    nbLevels = 0;
    MYFLAG++;
    for(int i = 0; i < out_learnt.size(); i++) {
        int lev = level[out_learnt[i].var()];
        if (permDiff[lev] != MYFLAG) {
            permDiff[lev] = MYFLAG;
            nbLevels++;
            //merged += nbPropagated(lev);
        }
    }
    
    #ifdef UPDATEVARACTIVITY
    if (lastDecisionLevel.size() > 0) {
        for(int i = 0; i<lastDecisionLevel.size(); i++) {
            if (reason[lastDecisionLevel[i].var()]->activity() < nbLevels)
                varBumpActivity(lastDecisionLevel[i].var());
        }
        lastDecisionLevel.clear();
    }
    #endif

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
        Clause& c = *reason[analyze_stack.last().var()];
        reverse_binary_clause(c);
        
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
    cout << "uncheckedEnqueue var " << p.var()+1 << " to " << !p.sign() << " level: " << decisionLevel() << " sublevel: " << trail.size() << endl;
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
    
    #ifdef VERBOSE_DEBUG
    cout << "Propagation started" << endl;
    #endif

    while (qhead < trail.size()) {
        Lit            p   = trail[qhead++];     // 'p' is enqueued fact to propagate.
        vec<Clause*>&  ws  = watches[p.toInt()];
        Clause         **i, **j, **end;
        num_props++;
        
        //First propagate binary clauses
        vec<WatchedBin> & wbin = binwatches[p.toInt()];
        for(WatchedBin *k = wbin.getData(), *end = k + wbin.size(); k != end; k++) {
            Lit imp = k->impliedLit;
            lbool val = value(imp);
            if (val == l_False)
                return k->clause;
            if (val == l_Undef) {
                uncheckedEnqueue(imp, k->clause);
                if (dynamic_behaviour_analysis)
                    logger.propagation(imp, Logger::simple_propagation_type, k->clause->group);
            }
        }
        
        //Next, propagate normal clauses
        
        #ifdef VERBOSE_DEBUG
        cout << "Propagating lit " << (p.sign() ? '-' : ' ') << p.var()+1 << endl;
        #endif

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
                for (uint k = 2; k != c.size(); k++)
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
                    #ifdef DYNAMICNBLEVEL
                    if (c.learnt() && c.activity() > 2) { // GA
                        MYFLAG++;
                        int nbLevels =0;
                        for(Lit *i = c.getData(), *end = i+c.size(); i != end; i++) {
                            int l = level[i->var()];
                            if (permDiff[l] != MYFLAG) {
                                permDiff[l] = MYFLAG;
                                nbLevels++;
                            }
                            
                        }
                        if(nbLevels+1 < c.activity())
                            c.setActivity(nbLevels);
                    }
                    #endif
                }
            }
FoundWatch:
            ;
        }
        ws.shrink(i - j);

        //Finally, propagate XOR-clauses
        if (xor_as_well && !confl) confl = propagate_xors(p);
    }
    propagations += num_props;
    simpDB_props -= num_props;
    
    #ifdef VERBOSE_DEBUG
    cout << "Propagation ended." << endl;
    #endif

    return confl;
}

Clause* Solver::propagate_xors(const Lit& p)
{
    #ifdef VERBOSE_DEBUG_XOR
    cout << "Xor-Propagating variable " <<  p.var()+1 << endl;
    #endif
    
    Clause* confl = NULL;

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
        for (int k = 0, size = c.size(); k != size; k++ ) {
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
            cout << "final: " << std::boolalpha << final << " - ";
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
    bool operator () (const Clause* x, const Clause* y) {
        const uint xsize = x->size();
        const uint ysize = y->size();
        
        // First criteria
        if (xsize > 2 && ysize == 2) return 1;
        if (ysize > 2 && xsize == 2) return 0;
        if (xsize == 2 && ysize == 2) return 0;
        
        // Second criteria
        if (x->activity() > y->activity()) return 1;
        if (x->activity() < y->activity()) return 0;
        
        //return x->oldActivity() < y->oldActivity();
        return xsize < ysize;
    }
};

void Solver::reduceDB()
{
    int     i, j;

    nbReduceDB++;
    sort(learnts, reduceDB_lt());
    for (i = j = 0; i < learnts.size() / RATIOREMOVECLAUSES; i++){
        if (learnts[i]->size() > 2 && !locked(*learnts[i]) && learnts[i]->activity() > 2)
            removeClause(*learnts[i]);
        else
            learnts[j++] = learnts[i];
    }
    for (; i < learnts.size(); i++) {
        learnts[j++] = learnts[i];
    }
    learnts.shrink(i - j);
}

const vec<Clause*>& Solver::get_learnts() const
{
    return learnts;
}

const vec<Clause*>& Solver::get_sorted_learnts()
{
    sort(learnts, reduceDB_lt());
    return learnts;
}

const vector<Lit> Solver::get_unitary_learnts() const
{
    vector<Lit> unitaries;
    if (decisionLevel() > 0) {
        for (uint i = 0; i < trail_lim[0]; i++)
            unitaries.push_back(trail[i]);
    }
    
    return unitaries;
}

void Solver::dump_sorted_learnts(const char* file)
{
    FILE* outfile = fopen(file, "w");
    if (!outfile) {
        printf("Error: Cannot open file '%s' to write learnt clauses!\n", file);
        exit(-1);
    }
    
    if (decisionLevel() > 0) {
        for (uint i = 0; i < trail_lim[0]; i++)
            printf("%s%d 0\n", trail[i].sign() ? "-" : "", trail[i].var());
    }
    
    sort(learnts, reduceDB_lt());
    for (int i = learnts.size()-1; i >= 0 ; i--) {
        learnts[i]->plain_print(outfile);
    }
    fclose(outfile);
}

void Solver::setMaxRestarts(const uint num)
{
    maxRestarts = num;
}

/*_________________________________________________________________________________________________
|
|  simplify : [void]  ->  [bool]
|
|  Description:
|    Simplify the clause database according to the current top-level assigment. Currently, the only
|    thing done here is the removal of satisfied clauses, but more things can be put here.
|________________________________________________________________________________________________@*/
lbool Solver::simplify()
{
    assert(decisionLevel() == 0);

    if (!ok || propagate() != NULL) {
        if (dynamic_behaviour_analysis) {
            logger.end(Logger::unsat_model_found);
        }
        ok = false;
        return l_False;
    }

    if (nAssigns() == simpDB_assigns || (simpDB_props > 0)) {
        return l_Undef;
    }

    // Remove satisfied clauses:
    clauseCleaner->removeSatisfied(learnts, ClauseCleaner::learnts);
    if (remove_satisfied) {       // Can be turned off.
        clauseCleaner->removeSatisfied(clauses, ClauseCleaner::clauses);
        clauseCleaner->removeSatisfied(xorclauses, ClauseCleaner::xorclauses);
    }

    // Remove fixed variables from the variable heap:
    order_heap.filter(VarFilter(*this));

    simpDB_assigns = nAssigns();
    simpDB_props   = clauses_literals + learnts_literals;   // (shouldn't depend on stats really, but it will do for now)

    //clauseCleaner->cleanClauses(clauses);
    //clauseCleaner->cleanClauses(xorclauses);
    //clauseCleaner->cleanClauses(learnts);

    return l_Undef;
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
lbool Solver::search(int nof_conflicts)
{
    assert(ok);
    int         conflictC = 0;
    vec<Lit>    learnt_clause;
    llbool      ret;

    starts++;
    for (Gaussian **gauss = &gauss_matrixes[0], **end= gauss + gauss_matrixes.size(); gauss != end; gauss++) {
        ret = (*gauss)->full_init();
        if (ret != l_Nothing) return ret;
    }

    for (;;) {
        Clause* confl = propagate();

        if (confl != NULL) {
            ret = handle_conflict(learnt_clause, confl, conflictC);
            if (ret != l_Nothing) return ret;
        } else {
            bool at_least_one_continue = false;
            for (Gaussian **gauss = &gauss_matrixes[0], **end= gauss + gauss_matrixes.size(); gauss != end; gauss++)  {
                ret = (*gauss)->find_truths(learnt_clause, conflictC);
                if (ret == l_Continue) at_least_one_continue = true;
                else if (ret != l_Nothing) return ret;
            }
            if (at_least_one_continue) continue;
            ret = new_decision(nof_conflicts, conflictC);
            if (ret != l_Nothing) return ret;
        }
    }
}

llbool Solver::new_decision(int& nof_conflicts, int& conflictC)
{
    if (nof_conflicts >= 0 && conflictC >= nof_conflicts) {
        // Reached bound on number of conflicts:
        progress_estimate = progressEstimate();
        cancelUntil(0);
        if (dynamic_behaviour_analysis) {
            logger.end(Logger::restarting);
        }
        return l_Undef;
    }

    // Simplify the set of problem clauses:
    if (decisionLevel() == 0 && simplify() == l_False) {
        if (dynamic_behaviour_analysis) {
            logger.end(Logger::unsat_model_found);
        }
        return l_False;
    }

    // Reduce the set of learnt clauses:
    if (conflicts >= curRestart * nbclausesbeforereduce) {
        curRestart ++;
        reduceDB();
        nbclausesbeforereduce += 500;
    }

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
            if (dynamic_behaviour_analysis) {
                logger.end(Logger::unsat_model_found);
            }
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
            if (dynamic_behaviour_analysis) {
                logger.end(Logger::model_found);
            }
            return l_True;
        }
    }

    // Increase decision level and enqueue 'next'
    assert(value(next) == l_Undef);
    newDecisionLevel();
    uncheckedEnqueue(next);
    if (dynamic_behaviour_analysis)
        logger.propagation(next, Logger::guess_type);

    return l_Nothing;
}

llbool Solver::handle_conflict(vec<Lit>& learnt_clause, Clause* confl, int& conflictC)
{
    #ifdef VERBOSE_DEBUG
    cout << "Handling conflict: ";
    for (uint i = 0; i < learnt_clause.size(); i++)
        cout << learnt_clause[i].var()+1 << ",";
    cout << endl;
    #endif
    
    int backtrack_level;
    int nbLevels;

    conflicts++;
    conflictC++;
    if (decisionLevel() == 0) {
        if (dynamic_behaviour_analysis) {
            logger.end(Logger::unsat_model_found);
        }
        return l_False;
    }
    learnt_clause.clear();
    analyze(confl, learnt_clause, backtrack_level, nbLevels);
    conf4Stats++;
    
    if (dynamic_behaviour_analysis)
        logger.conflict(Logger::simple_confl_type, backtrack_level, confl->group, learnt_clause);
    cancelUntil(backtrack_level);
    
    #ifdef VERBOSE_DEBUG
    cout << "Learning:";
    for (uint i = 0; i < learnt_clause.size(); i++) printLit(learnt_clause[i]), cout << " ";
    cout << endl;
    cout << "reverting var " << learnt_clause[0].var()+1 << " to " << !learnt_clause[0].sign() << endl;
    #endif
    
    assert(value(learnt_clause[0]) == l_Undef);
    //Unitary learnt
    if (learnt_clause.size() == 1) {
        uncheckedEnqueue(learnt_clause[0]);
        if (dynamic_behaviour_analysis) {
            logger.set_group_name(learnt_clause_group, "unitary learnt clause");
            logger.propagation(learnt_clause[0], Logger::unit_clause_type, learnt_clause_group);
            learnt_clause_group++;
        }
        assert(backtrack_level == 0 && "Unit clause learnt, so must cancel until level 0, right?");
        
        #ifdef VERBOSE_DEBUG
        cout << "Unit clause learnt." << endl;
        #endif
    //Normal learnt
    } else {
        Clause* c = Clause_new(learnt_clause, learnt_clause_group++, true);
        learnts.push(c);
        c->setActivity(nbLevels); // LS
        if (nbLevels <= 2) nbDL2++;
        attachClause(*c);
        uncheckedEnqueue(learnt_clause[0], c);

        if (dynamic_behaviour_analysis) {
            logger.set_group_name(c->group, "learnt clause");
            logger.propagation(learnt_clause[0], Logger::simple_propagation_type, c->group);
        }
    }

    varDecayActivity();

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

void Solver::print_gauss_sum_stats() const
{
    if (gauss_matrixes.size() == 0) {
        printf("  no matrixes found |\n");
        return;
    }
    
    uint called = 0;
    uint useful_prop = 0;
    uint useful_confl = 0;
    uint disabled = 0;
    for (Gaussian *const*gauss = &gauss_matrixes[0], *const*end= gauss + gauss_matrixes.size(); gauss != end; gauss++) {
        disabled += (*gauss)->get_disabled();
        called += (*gauss)->get_called();
        useful_prop += (*gauss)->get_useful_prop();
        useful_confl += (*gauss)->get_useful_confl();
        //gauss->print_stats();
        //gauss->print_matrix_stats();
    }
    
    if (called == 0) {
        printf("      disabled      |\n");
    } else {
        printf(" %3.0lf%% |", (double)useful_prop/(double)called*100.0);
        printf(" %3.0lf%% |", (double)useful_confl/(double)called*100.0);
        printf(" %3.0lf%% |\n", 100.0-(double)disabled/(double)gauss_matrixes.size()*100.0);
    }
}

lbool Solver::solve(const vec<Lit>& assumps)
{
    #ifdef DEBUG_LIB
    fprintf(myoutputfile, "c Solver::solve() called\n");
    #endif
    
    model.clear();
    conflict.clear();

    if (!ok) return l_False;

    assumps.copyTo(assumptions);

    double  nof_conflicts = restart_first;
    lbool   status        = l_Undef;
    
    if (nClauses() * learntsize_factor < nbclausesbeforereduce) {
        if (nClauses() * learntsize_factor < nbclausesbeforereduce/2)
            nbclausesbeforereduce = nbclausesbeforereduce/4;
        else
            nbclausesbeforereduce = (nClauses() * learntsize_factor)/2;
    }
    
    conglomerate->addRemovedClauses();
    
    if (performReplace) {
        toReplace->performReplace();
        if (!ok) return l_False;
    }

    if (xorFinder) {
        double time;
        if (clauses.size() < 400000) {
            time = cpuTime();
            clauseCleaner->removeSatisfied(clauses, ClauseCleaner::clauses);
            clauseCleaner->cleanClauses(clauses, ClauseCleaner::clauses);
            uint sumLengths = 0;
            XorFinder xorFinder(this, clauses);
            uint foundXors = xorFinder.doNoPart(sumLengths, 2, 10);
            if (!ok) return l_False;
            
            if (verbosity >=1)
                printf("|  Finding XORs:        %5.2lf s (found: %7d, avg size: %3.1lf)               |\n", cpuTime()-time, foundXors, (double)sumLengths/(double)foundXors);
            
            if (performReplace) {
                toReplace->performReplace();
                if (!ok) return l_False;
            }
        }
        
        if (xorclauses.size() > 1) {
            uint orig_total = 0;
            uint orig_num_cls = xorclauses.size();
            for (uint i = 0; i < xorclauses.size(); i++) {
                orig_total += xorclauses[i]->size();
            }
            
            time = cpuTime();
            uint foundCong = conglomerate->conglomerateXors();
            if (verbosity >=1)
                printf("|  Conglomerating XORs:  %4.2lf s (removed %6d vars)                         |\n", cpuTime()-time, foundCong);
            if (!ok) return l_False;
            
            uint new_total = 0;
            uint new_num_cls = xorclauses.size();
            for (uint i = 0; i < xorclauses.size(); i++) {
                new_total += xorclauses[i]->size();
            }
            if (verbosity >=1) {
                printf("|  Sum xclauses before: %8d, after: %12d                         |\n", orig_num_cls, new_num_cls);
                printf("|  Sum xlits before: %11d, after: %12d                         |\n", orig_total, new_total);
            }
            
            if (performReplace) {
                toReplace->performReplace();
                if (!ok) return l_False;
            }
        }
    }
    
    if (gaussconfig.decision_until > 0 && xorclauses.size() > 1 && xorclauses.size() < 20000) {
        double time = cpuTime();
        MatrixFinder m(this);
        const uint numMatrixes = m.findMatrixes();
        if (verbosity >=1)
            printf("|  Finding matrixes :    %4.2lf s (found  %5d)                                |\n", cpuTime()-time, numMatrixes);
    }
    

    if (verbosity >= 1 && !(dynamic_behaviour_analysis && logger.statistics_on)) {
        printf("============================[ Search Statistics ]========================================\n");
        printf("| Conflicts |          ORIGINAL         |          LEARNT          |        GAUSS       |\n");
        printf("|           |    Vars  Clauses Literals |    Limit  Clauses Lit/Cl | Prop   Confl   On  |\n");
        printf("=========================================================================================\n");
    }
    
    if (dynamic_behaviour_analysis)
        logger.end(Logger::done_adding_clauses);

    // Search:
    while (status == l_Undef && starts < maxRestarts) {
        clauseCleaner->removeSatisfied(clauses, ClauseCleaner::clauses);
        clauseCleaner->removeSatisfied(xorclauses, ClauseCleaner::xorclauses);
        clauseCleaner->removeSatisfied(learnts, ClauseCleaner::learnts);
        
        clauseCleaner->cleanClauses(clauses, ClauseCleaner::clauses);
        clauseCleaner->cleanClauses(xorclauses, ClauseCleaner::xorclauses);
        clauseCleaner->cleanClauses(learnts, ClauseCleaner::learnts);
        
        if (verbosity >= 1 && !(dynamic_behaviour_analysis && logger.statistics_on))  {
            printf("| %9d | %7d %8d %8d | %8d %8d %6.0f |", (int)conflicts, order_heap.size(), nClauses(), (int)clauses_literals, (int)nbclausesbeforereduce*curRestart, nLearnts(), (double)learnts_literals/nLearnts());
            print_gauss_sum_stats();
        }
        for (Gaussian **gauss = &gauss_matrixes[0], **end= gauss + gauss_matrixes.size(); gauss != end; gauss++)
            (*gauss)->reset_stats();
        
        if (dynamic_behaviour_analysis)
            logger.begin();
        status = search((int)nof_conflicts);
        nof_conflicts *= restart_inc;
    }

    if (verbosity >= 1 && !(dynamic_behaviour_analysis && logger.statistics_on)) {
        printf("====================================================================");
        print_gauss_sum_stats();
    }
    
    for (uint i = 0; i < gauss_matrixes.size(); i++)
        delete gauss_matrixes[i];
    gauss_matrixes.clear();
    for (uint i = 0; i < freeLater.size(); i++)
        free(freeLater[i]);
    freeLater.clear();

    if (status == l_True) {
        conglomerate->doCalcAtFinish();
        toReplace->extendModel();
        // Extend & copy model:
        model.growTo(nVars());
        for (int i = 0; i < nVars(); i++) model[i] = value(i);
#ifndef NDEBUG
        verifyModel();
#endif
        if (greedyUnbound) {
            double time = cpuTime();
            FindUndef finder(*this);
            const uint unbounded = finder.unRoll();
            printf("Greedy unbounding     :%5.2lf s, unbounded: %7d vars\n", cpuTime()-time, unbounded);
        }
    } if (status == l_False) {
        if (conflict.size() == 0)
            ok = false;
    }
    
    #ifdef LS_STATS_NBBUMP
    for(int i=0;i<learnts.size();i++)
        printf("## %d %d %d\n", learnts[i]->size(),learnts[i]->activity(),
               (unsigned int)learnts[i]->nbBump());
    #endif

    cancelUntil(0);
    return status;
}

//=================================================================================================
// Debug methods:

bool Solver::verifyXorClauses(const vec<XorClause*>& cs) const
{
    #ifdef VERBOSE_DEBUG
    cout << "Checking xor-clauses whether they have been properly satisfied." << endl;;
    #endif
    
    bool failed = false;
    
    for (int i = 0; i < xorclauses.size(); i++) {
        XorClause& c = *xorclauses[i];
        bool final = c.xor_clause_inverted();
        
        #ifdef VERBOSE_DEBUG
        XorClause* c2 = XorClause_new(c, c.xor_clause_inverted(), c.group);
        std::sort(c2->getData(), c2->getData()+ c2->size());
        c2->plain_print();
        free(c2);
        #endif
        
        for (uint j = 0; j < c.size(); j++) {
            assert(modelValue(c[j].unsign()) != l_Undef);
            final ^= (modelValue(c[j].unsign()) == l_True);
        }
        if (!final) {
            printf("unsatisfied clause: ");
            printClause(*xorclauses[i]);
            printf("\n");
            failed = true;
        }
    }
    
    return failed;
}

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
    
    failed |= verifyXorClauses(xorclauses);
    failed |= verifyXorClauses(conglomerate->getCalcAtFinish());

    assert(!failed);

    if (verbosity >=1)
        printf("Verified %d clauses.\n", clauses.size() + xorclauses.size() + conglomerate->getCalcAtFinish().size());
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
