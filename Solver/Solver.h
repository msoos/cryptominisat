/****************************************************************************************[Solver.h]
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

#ifndef SOLVER_H
#define SOLVER_H

#include <cstdio>
#include <string.h>
#include <stdio.h>
#ifdef _MSC_VER
#include <msvc/stdint.h>
#else
#include <stdint.h>
#endif //_MSC_VER

#include "Vec.h"
#include "Heap.h"
#include "Alg.h"
#include "MersenneTwister.h"
#include "SolverTypes.h"
#include "Clause.h"
#include "VarReplacer.h"
#include "GaussianConfig.h"
#include "Logger.h"
#include "constants.h"
#include "BoundedQueue.h"
#include "RestartTypeChooser.h"

class Gaussian;
class MatrixFinder;
class Conglomerate;
class VarReplacer;
class XorFinder;
class FindUndef;
class ClauseCleaner;

#ifdef VERBOSE_DEBUG
using std::cout;
using std::endl;
#endif

//=================================================================================================
// Solver -- the main class:


class Solver
{
public:

    // Constructor/Destructor:
    //
    Solver();
    ~Solver();

    // Problem specification:
    //
    Var     newVar    (bool dvar = true); // Add a new variable with parameters specifying variable mode.
    void    varUsed(Var var);
    bool    addClause (vec<Lit>& ps, const uint group, char* group_name);  // Add a clause to the solver. NOTE! 'ps' may be shrunk by this method!
    bool    addXorClause (vec<Lit>& ps, bool xor_clause_inverted, const uint group, char* group_name, const bool internal = false);  // Add a xor-clause to the solver. NOTE! 'ps' may be shrunk by this method!

    // Solving:
    //
    lbool    solve       (const vec<Lit>& assumps); // Search for a model that respects a given set of assumptions.
    lbool    solve       ();                        // Search without assumptions.
    bool    okay         () const;                  // FALSE means solver is in a conflicting state

    // Variable mode:
    //
    void    setPolarity    (Var v, bool b); // Declare which polarity the decision heuristic should use for a variable. Requires mode 'polarity_user'.
    void    setDecisionVar (Var v, bool b); // Declare if a variable should be eligible for selection in the decision heuristic.
    void    setSeed (const uint32_t seed);  // Sets the seed to be the given number
    void    setMaxRestarts(const uint num); //sets the maximum number of restarts to given value
    void    set_gaussian_decision_until(const uint to);
    template<class T>
    void    removeWatchedCl(vec<T> &ws, const Clause *c);
    template<class T>
    bool    findWatchedCl(vec<T>& ws, const Clause *c);
    template<class T>
    void    removeWatchedBinCl(vec<T> &ws, const Clause *c);
    template<class T>
    bool    findWatchedBinCl(vec<T>& ws, const Clause *c);

    // Read state:
    //
    lbool   value      (const Var& x) const;       // The current value of a variable.
    lbool   value      (const Lit& p) const;       // The current value of a literal.
    lbool   modelValue (const Lit& p) const;       // The value of a literal in the last model. The last call to solve must have been satisfiable.
    uint32_t     nAssigns   ()      const;       // The current number of assigned literals.
    uint32_t     nClauses   ()      const;       // The current number of original clauses.
    uint32_t     nLearnts   ()      const;       // The current number of learnt clauses.
    uint32_t     nVars      ()      const;       // The current number of variables.

    // Extra results: (read-only member variable)
    //
    vec<lbool> model;             // If problem is satisfiable, this vector contains the model (if any).
    vec<Lit>   conflict;          // If problem is unsatisfiable (possibly under assumptions),
    // this vector represent the final conflict clause expressed in the assumptions.

    // Mode of operation:
    //
    double    var_decay;          // Inverse of the variable activity decay factor.                                            (default 1 / 0.95)
    double    random_var_freq;    // The frequency with which the decision heuristic tries to choose a random variable.        (default 0.02)
    int       restart_first;      // The initial restart limit.                                                                (default 100)
    double    restart_inc;        // The factor with which the restart limit is multiplied in each restart.                    (default 1.5)
    double    learntsize_factor;  // The intitial limit for learnt clauses is a factor of the original clauses.                (default 1 / 3)
    double    learntsize_inc;     // The limit for learnt clauses is multiplied with this factor each restart.                 (default 1.1)
    bool      expensive_ccmin;    // Controls conflict clause minimization.                                                    (default TRUE)
    int       polarity_mode;      // Controls which polarity the decision heuristic chooses. See enum below for allowed modes. (default polarity_false)
    int       verbosity;          // Verbosity level. 0=silent, 1=some progress report                                         (default 0)
    Var       restrictedPickBranch; // Pick variables to branch on preferentally from the highest [0, restrictedPickBranch]. If set to 0, preferentiality is turned off (i.e. picked randomly between [0, all])
    bool      xorFinder;            // Automatically find xor-clauses and convert them
    bool      performReplace;       // Should var-replacing be performed?
    friend class FindUndef;
    bool      greedyUnbound;        //If set, then variables will be greedily unbounded (set to l_Undef)
    RestartType fixRestartType;     // If set, the solver will always choose the given restart strategy
    

    enum { polarity_true = 0, polarity_false = 1, polarity_rnd = 3, polarity_auto = 4};

    // Statistics: (read-only member variable)
    //
    uint64_t starts, decisions, rnd_decisions, propagations, conflicts;
    uint64_t clauses_literals, learnts_literals, max_literals, tot_literals;
    uint64_t nbDL2, nbBin, lastNbBin, becameBinary, lastSearchForBinaryXor, nbReduceDB;

    //Logging
    void needStats();              // Prepares the solver to output statistics
    void needProofGraph();         // Prepares the solver to output proof graphs during solving
    void setVariableName(Var var, char* name); // Sets the name of the variable 'var' to 'name'. Useful for statistics and proof logs (i.e. used by 'logger')
    const vec<Clause*>& get_sorted_learnts(); //return the set of learned clauses, sorted according to the logic used in MiniSat to distinguish between 'good' and 'bad' clauses
    const vec<Clause*>& get_learnts() const; //Get all learnt clauses that are >1 long
    const vector<Lit> get_unitary_learnts() const; //return the set of unitary learnt clauses
    const uint get_unitary_learnts_num() const; //return the number of unitary learnt clauses
    void dumpSortedLearnts(const char* file, const uint32_t maxSize); // Dumps all learnt clauses (including unitary ones) into the file
    void needLibraryCNFFile(const char* fileName); //creates file in current directory with the filename indicated, and puts all calls from the library into the file.
    void printNondecisonVariables() const;

protected:
    vector<Gaussian*> gauss_matrixes;
    GaussianConfig gaussconfig;
    void print_gauss_sum_stats() const;
    void clearGaussMatrixes();
    friend class Gaussian;
    
    
    // Helper structures:
    //
    struct VarOrderLt {
        const vec<double>&  activity;
        bool operator () (Var x, Var y) const {
            return activity[x] > activity[y];
        }
        VarOrderLt(const vec<double>&  act) : activity(act) { }
    };

    friend class VarFilter;
    struct VarFilter {
        const Solver& s;
        VarFilter(const Solver& _s) : s(_s) {}
        bool operator()(Var v) const {
            return s.assigns[v].isUndef() && s.decision_var[v];
        }
    };

    // Solver state:
    //
    bool                ok;               // If FALSE, the constraints are already unsatisfiable. No part of the solver state may be used!
    vec<Clause*>        clauses;          // List of problem clauses.
    vec<Clause*>        binaryClauses;    // Binary clauses are regularly moved here
    vec<XorClause*>     xorclauses;       // List of problem xor-clauses. Will be freed
    vec<Clause*>        learnts;          // List of learnt clauses.
    vector<bool>        givenUnitaries;   // Unitary clauses given (i.e. not learnt)
    vec<XorClause*>     freeLater;        // List of xorclauses to free at the end (due to matrixes, they cannot be freed immediately)
    vec<double>         activity;         // A heuristic measurement of the activity of a variable.
    double              var_inc;          // Amount to bump next variable with.
    vec<vec<Watched> >  watches;          // 'watches[lit]' is a list of constraints watching 'lit' (will go there if literal becomes true).
    vec<vec<XorClausePtr> > xorwatches;   // 'xorwatches[var]' is a list of constraints watching var in XOR clauses.
    vec<vec<WatchedBin> >  binwatches;
    vec<lbool>          assigns;          // The current assignments
    vector<bool>        polarity;         // The preferred polarity of each variable.
    vector<bool>        decision_var;     // Declares if a variable is eligible for selection in the decision heuristic.
    vec<Lit>            trail;            // Assignment stack; stores all assigments made in the order they were made.
    vec<uint32_t>       trail_lim;        // Separator indices for different decision levels in 'trail'.
    vec<ClausePtr>      reason;           // 'reason[var]' is the clause that implied the variables current value, or 'NULL' if none.
    vec<int32_t>        level;            // 'level[var]' contains the level at which the assignment was made.
    vec<Var>            permDiff;         // LS: permDiff[var] contains the current conflict number... Used to count the number of different decision level variables in learnt clause
    #ifdef UPDATEVARACTIVITY
    vec<Lit>            lastDecisionLevel;
    #endif
    uint64_t            curRestart;
    uint32_t            nbclausesbeforereduce;
    uint32_t            qhead;            // Head of queue (as index into the trail -- no more explicit propagation queue in MiniSat).
    uint32_t            simpDB_assigns;   // Number of top-level assignments since last execution of 'simplify()'.
    int64_t             simpDB_props;     // Remaining number of propagations that must be made before next execution of 'simplify()'.
    vec<Lit>            assumptions;      // Current set of assumptions provided to solve by the user.
    Heap<VarOrderLt>    order_heap;       // A priority queue of variables ordered with respect to the variable activity.
    double              progress_estimate;// Set by 'search()'.
    bool                remove_satisfied; // Indicates whether possibly inefficient linear scan for satisfied clauses should be performed in 'simplify'.
    bqueue<uint> nbDecisionLevelHistory; // Set of last decision level in conflict clauses
    float               totalSumOfDecisionLevel;
    MTRand              mtrand;           // random number generaton
    RestartType         restartType;      // Used internally to determine which restart strategy to choose
    friend class        Logger;
    #ifdef STATS_NEEDED
    Logger logger;                        // dynamic logging, statistics
    bool                dynamic_behaviour_analysis; // Is logger running?
    #endif
    uint                maxRestarts;      // More than this number of restarts will not be performed

    // Temporaries (to reduce allocation overhead). Each variable is prefixed by the method in which it is
    // used, exept 'seen' wich is used in several places.
    //
    vector<bool>        seen;
    vec<Lit>            analyze_stack;
    vec<Lit>            analyze_toclear;
    vec<Lit>            add_tmp;
    unsigned long int   MYFLAG;

    //Logging
    uint                learnt_clause_group; //the group number of learnt clauses. Incremented at each added learnt clause
    FILE                *libraryCNFFile; //The file that all calls from the library are logged

    // Main internal methods:
    //
    lbool    simplify    ();                                                           // Removes already satisfied clauses.
    //int      nbPropagated     (int level);
    void     insertVarOrder   (Var x);                                                 // Insert a variable in the decision order priority queue.
    Lit      pickBranchLit    ();                                                      // Return the next decision variable.
    void     newDecisionLevel ();                                                      // Begins a new decision level.
    void     uncheckedEnqueue (Lit p, ClausePtr from = (Clause*)NULL);                 // Enqueue a literal. Assumes value of literal is undefined.
    bool     enqueue          (Lit p, Clause* from = NULL);                            // Test if fact 'p' contradicts current state, enqueue otherwise.
    Clause*  propagate        (const bool xor_as_well = true);                         // Perform unit propagation. Returns possibly conflicting clause.
    Clause*  propagate_xors   (const Lit& p);
    void     cancelUntil      (int level);                                             // Backtrack until a certain level.
    void     analyze          (Clause* confl, vec<Lit>& out_learnt, int& out_btlevel, int &nblevels); // (bt = backtrack)
    void     analyzeFinal     (Lit p, vec<Lit>& out_conflict);                         // COULD THIS BE IMPLEMENTED BY THE ORDINARIY "analyze" BY SOME REASONABLE GENERALIZATION?
    bool     litRedundant     (Lit p, uint32_t abstract_levels);                       // (helper method for 'analyze()')
    lbool    search           (int nof_conflicts, int nof_conflicts_fullrestart);      // Search for a given number of conflicts.
    void     reduceDB         ();                                                      // Reduce the set of learnt clauses.
    llbool   handle_conflict  (vec<Lit>& learnt_clause, Clause* confl, int& conflictC);// Handles the conflict clause
    llbool   new_decision     (const int& nof_conflicts, const int& nof_conflicts_fullrestart, int& conflictC);  // Handles the case when all propagations have been made, and now a decision must be made

    // Maintaining Variable/Clause activity:
    //
    void     varDecayActivity ();                      // Decay all variables with the specified factor. Implemented by increasing the 'bump' value instead.
    void     varBumpActivity  (Var v);                 // Increase a variable with the current 'bump' value.
    void     claDecayActivity ();                      // Decay all clauses with the specified factor. Implemented by increasing the 'bump' value instead.

    // Operations on clauses:
    //
    void     attachClause     (XorClause& c);
    void     attachClause     (Clause& c);             // Attach a clause to watcher lists.
    void     detachClause     (const XorClause& c);
    void     detachClause     (const Clause& c);       // Detach a clause to watcher lists.
    void     detachModifiedClause(const Lit lit1, const Lit lit2, const uint size, const Clause* address);
    void     detachModifiedClause(const Var var1, const Var var2, const uint origSize, const XorClause* address);
    void     removeClause(Clause& c);                  // Detach and free a clause.
    void     removeClause(XorClause& c);               // Detach and free a clause.
    bool     locked           (const Clause& c) const; // Returns TRUE if a clause is a reason for some implication in the current state.
    void     reverse_binary_clause(Clause& c) const;   // Binary clauses --- the first Lit has to be true

    // Misc:
    //
    uint32_t decisionLevel    ()      const; // Gives the current decisionlevel.
    uint32_t abstractLevel    (const Var& x) const; // Used to represent an abstraction of sets of decision levels.
    
    //Xor-finding related stuff
    friend class XorFinder;
    friend class Conglomerate;
    friend class MatrixFinder;
    friend class VarReplacer;
    friend class ClauseCleaner;
    friend class RestartTypeChooser;
    Conglomerate* conglomerate;
    VarReplacer* varReplacer;
    ClauseCleaner* clauseCleaner;
    void chooseRestartType(const lbool& status, RestartTypeChooser& restartTypeChooser, const uint& lastFullRestart);
    void setDefaultRestartType();
    void checkFullRestart(int& nof_conflicts, int& nof_conflicts_fullrestart, uint& lastFullRestart);
    void performStepsBeforeSolve();

    // Debug & etc:
    void     printLit         (const Lit l) const;
    void     verifyModel      ();
    bool     verifyClauses    (const vec<Clause*>& cs) const;
    bool     verifyXorClauses (const vec<XorClause*>& cs) const;
    void     checkLiteralCount();
    void     printStatHeader  () const;
    void     printRestartStat () const;
    void     printEndSearchStat() const;
    double   progressEstimate () const; // DELETE THIS ?? IT'S NOT VERY USEFUL ...
    uint     numGivenUnitaries() const; //Return the no. of given unitary clauses
    
    // Polarity chooser
    vector<bool> defaultPolarities; //The default polarity to set the var polarity when doing a full restart
    void         calculateDefaultPolarities(); //Calculates the default polarity for each var, and fills defaultPolarities[] with it
    bool         defaultPolarity(); //if polarity_mode is not polarity_auto, this returns the default polarity of the variable
    void         setDefaultPolarities(); //sets the polarity[] to that indicated by defaultPolarities[]
};


//=================================================================================================
// Implementation of inline methods:


inline void Solver::insertVarOrder(Var x)
{
    if (!order_heap.inHeap(x) && decision_var[x]) order_heap.insert(x);
}

inline void Solver::varDecayActivity()
{
    var_inc *= var_decay;
}
inline void Solver::varBumpActivity(Var v)
{
    if ( (activity[v] += var_inc) > 1e100 ) {
        // Rescale:
        for (uint32_t i = 0; i != nVars(); i++)
            activity[i] *= 1e-100;
        var_inc *= 1e-100;
    }

    // Update order_heap with respect to new activity:
    if (order_heap.inHeap(v))
        order_heap.decrease(v);
}

inline bool     Solver::enqueue         (Lit p, Clause* from)
{
    return value(p) != l_Undef ? value(p) != l_False : (uncheckedEnqueue(p, from), true);
}
inline bool     Solver::locked          (const Clause& c) const
{
    return reason[c[0].var()] == &c && value(c[0]) == l_True;
}
inline void     Solver::newDecisionLevel()
{
    trail_lim.push(trail.size());
    #ifdef VERBOSE_DEBUG
    cout << "New decision level: " << trail_lim.size() << endl;
    #endif
}
/*inline int     Solver::nbPropagated(int level) {
    if (level == decisionLevel())
        return trail.size() - trail_lim[level-1] - 1;
    return trail_lim[level] - trail_lim[level-1] - 1;
}*/
inline uint32_t      Solver::decisionLevel ()      const
{
    return trail_lim.size();
}
inline void Solver::varUsed(Var var)
{
    decision_var[var] = true;
    insertVarOrder(var);
}
inline uint32_t Solver::abstractLevel (const Var& x) const
{
    return 1 << (level[x] & 31);
}
inline lbool    Solver::value         (const Var& x) const
{
    return assigns[x];
}
inline lbool    Solver::value         (const Lit& p) const
{
    return assigns[p.var()] ^ p.sign();
}
inline lbool    Solver::modelValue    (const Lit& p) const
{
    return model[p.var()] ^ p.sign();
}
inline uint32_t      Solver::nAssigns      ()      const
{
    return trail.size();
}
inline uint32_t      Solver::nClauses      ()      const
{
    return clauses.size() + xorclauses.size();
}
inline uint32_t      Solver::nLearnts      ()      const
{
    return learnts.size();
}
inline uint32_t      Solver::nVars         ()      const
{
    return assigns.size();
}
inline void     Solver::setPolarity   (Var v, bool b)
{
    polarity    [v] = (char)b;
}
inline void     Solver::setDecisionVar(Var v, bool b)
{
    decision_var[v] = b;
    if (b) {
        insertVarOrder(v);
    }
}
inline lbool     Solver::solve         ()
{
    vec<Lit> tmp;
    return solve(tmp);
}
inline bool     Solver::okay          ()      const
{
    return ok;
}
inline void     Solver::setSeed (const uint32_t seed)
{
    mtrand.seed(seed);    // Set seed of the variable-selection and clause-permutation(if applicable)
}
#ifdef STATS_NEEDED
inline void     Solver::needStats()
{
    dynamic_behaviour_analysis = true;    // Sets the solver and the logger up to generate statistics
    logger.statistics_on = true;
}
inline void     Solver::needProofGraph()
{
    dynamic_behaviour_analysis = true;    // Sets the solver and the logger up to generate proof graphs during solving
    logger.proof_graph_on = true;
}
inline void     Solver::setVariableName(Var var, char* name)
{
    while (var >= nVars()) newVar();
    if (dynamic_behaviour_analysis)
        logger.set_variable_name(var, name);
} // Sets the varible 'var'-s name to 'name' in the logger
#else
inline void     Solver::setVariableName(Var var, char* name)
{}
#endif
inline uint Solver::numGivenUnitaries() const
{
    uint num = 0;
    for (uint i = 0; i != givenUnitaries.size(); i++)
        num += (uint)givenUnitaries[i];
    return num;
}

inline const uint Solver::get_unitary_learnts_num() const
{
    if (decisionLevel() > 0)
        return trail_lim[0]-numGivenUnitaries();
    else
        return trail.size()-numGivenUnitaries();
}
template <class T>
inline void Solver::removeWatchedCl(vec<T> &ws, const Clause *c) {
    uint32_t j = 0;
    for (; j < ws.size() && ws[j].clause != c; j++);
    assert(j < ws.size());
    for (; j < ws.size()-1; j++) ws[j] = ws[j+1];
    ws.pop();
}
template <class T>
inline void Solver::removeWatchedBinCl(vec<T> &ws, const Clause *c) {
    uint32_t j = 0;
    for (; j < ws.size() && ws[j].clause != c; j++);
    assert(j < ws.size());
    for (; j < ws.size()-1; j++) ws[j] = ws[j+1];
    ws.pop();
}
template<class T>
inline bool Solver::findWatchedCl(vec<T>& ws, const Clause *c)
{
    uint32_t j = 0;
    for (; j < ws.size() && ws[j].clause != c; j++);
    return j < ws.size();
}
template<class T>
inline bool Solver::findWatchedBinCl(vec<T>& ws, const Clause *c)
{
    uint32_t j = 0;
    for (; j < ws.size() && ws[j].clause != c; j++);
    return j < ws.size();
}
inline void Solver::reverse_binary_clause(Clause& c) const {
    if (c.size() == 2 && value(c[0]) == l_False) {
        assert(value(c[1]) == l_True);
        Lit tmp = c[0];
        c[0] =  c[1], c[1] = tmp;
    }
}
inline void Solver::removeClause(Clause& c)
{
    detachClause(c);
    clauseFree(&c);
}
inline void Solver::removeClause(XorClause& c)
{
    detachClause(c);
    freeLater.push(&c);
    c.mark(1);
}


//=================================================================================================
// Debug + etc:

static inline void logLit(FILE* f, Lit l)
{
    fprintf(f, "%sx%d", l.sign() ? "~" : "", l.var()+1);
}

static inline void logLits(FILE* f, const vec<Lit>& ls)
{
    fprintf(f, "[ ");
    if (ls.size() > 0) {
        logLit(f, ls[0]);
        for (uint32_t i = 1; i < ls.size(); i++) {
            fprintf(f, ", ");
            logLit(f, ls[i]);
        }
    }
    fprintf(f, "] ");
}

static inline const char* showBool(bool b)
{
    return b ? "true" : "false";
}


// Just like 'assert()' but expression will be evaluated in the release version as well.
static inline void check(bool expr)
{
    assert(expr);
}

//=================================================================================================
#endif //SOLVER_H
