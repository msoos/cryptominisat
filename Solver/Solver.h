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
#include <stack>

#ifdef _MSC_VER
#include <msvc/stdint.h>
#else
#include <stdint.h>
#endif //_MSC_VER

#ifdef STATS_NEEDED
#include "Logger.h"
#endif //STATS_NEEDED


#include "PropagatedFrom.h"
#include "Vec.h"
#include "Heap.h"
#include "Alg.h"
#include "MersenneTwister.h"
#include "SolverTypes.h"
#include "Clause.h"
#include "constants.h"
#include "BoundedQueue.h"
#include "GaussianConfig.h"
#include "ClauseAllocator.h"

#define release_assert(a) \
    do { \
        if (!(a)) {\
            fprintf(stderr, "*** ASSERTION FAILURE in %s() [%s:%d]: %s\n", \
            __FUNCTION__, __FILE__, __LINE__, #a); \
            abort(); \
        } \
    } while (0)

class Gaussian;
class MatrixFinder;
class Conglomerate;
class VarReplacer;
class XorFinder;
class FindUndef;
class ClauseCleaner;
class FailedVarSearcher;
class Subsumer;
class XorSubsumer;
class PartHandler;
class RestartTypeChooser;
class StateSaver;
class UselessBinRemover;

#ifdef VERBOSE_DEBUG
#define DEBUG_UNCHECKEDENQUEUE_LEVEL0
using std::cout;
using std::endl;
#endif

//=================================================================================================
// Solver -- the main class:

struct reduceDB_ltMiniSat
{
    bool operator () (const Clause* x, const Clause* y);
};

struct reduceDB_ltGlucose
{
    bool operator () (const Clause* x, const Clause* y);
};

/**
@brief The main solver class

This class creates and manages all the others. It is here that settings must
be set, and it is here that all data enter and leaves the system. The basic
use is to add normal and XOR clauses, and then solve(). The solver will then
solve the problem, and return with either a SAT solution with corresponding
variable settings, or report that the problem in UNSATisfiable.

The prolbem-solving can be interrupted with the "interrupt" varible, and can
also be pre-set to stop after a certain number of restarts. The data until the
interruption can be dumped by previously setting parameters like
dumpSortedLearnts
*/
class Solver
{
public:

    // Constructor/Destructor:
    //
    Solver();
    ~Solver();

    // Problem specification:
    //
    Var     newVar    (bool dvar = true);           // Add a new variable with parameters specifying variable mode.
    template<class T>
    bool    addClause (T& ps, const uint32_t group = 0, const char* group_name = NULL);  // Add a clause to the solver. NOTE! 'ps' may be shrunk by this method!
    template<class T>
    bool    addXorClause (T& ps, bool xorEqualFalse, const uint32_t group = 0, const char* group_name = NULL);  // Add a xor-clause to the solver. NOTE! 'ps' may be shrunk by this method!
    template<class T>
    bool addLearntClause(T& ps, const uint32_t glue, const float miniSatActivity, const uint32_t group);

    // Solving:
    //
    lbool    solve       (const vec<Lit>& assumps); ///<Search for a model that respects a given set of assumptions.
    lbool    solve       ();                        ///<Search without assumptions.
    void     handleSATSolution();                   ///<Extends model, if needed, and fills "model"
    void     handleUNSATSolution();                 ///<If conflict really was zero-length, sets OK to false
    bool     okay         () const;                 ///<FALSE means solver is in a conflicting state

    // Variable mode:
    //
    void    setDecisionVar (Var v, bool b);         ///<Declare if a variable should be eligible for selection in the decision heuristic.
    void    setSeed (const uint32_t seed);          ///<Sets the seed to be the given number
    void    setMaxRestarts(const uint32_t num);     ///<sets the maximum number of restarts to given value

    // Read state:
    //
    lbool   value      (const Var& x) const;       ///<The current value of a variable.
    lbool   value      (const Lit& p) const;       ///<The current value of a literal.
    lbool   modelValue (const Lit& p) const;       ///<The value of a literal in the last model. The last call to solve must have been satisfiable.
    uint32_t     nAssigns   ()      const;         ///<The current number of assigned literals.
    uint32_t     nClauses   ()      const;         ///<The current number of original clauses.
    uint32_t     nLiterals  ()      const;         ///<The current number of total literals.
    uint32_t     nLearnts   ()      const;         ///<The current number of learnt clauses.
    uint32_t     nVars      ()      const;         ///<The current number of variables.

    // Extra results: (read-only member variable)
    //
    vec<lbool> model;             ///<If problem is satisfiable, this vector contains the model (if any).
    vec<Lit>   conflict;          ///<If problem is unsatisfiable (possibly under assumptions), this vector represent the final conflict clause expressed in the assumptions.

    // Mode of operation:
    //
    double    random_var_freq;    ///<The frequency with which the decision heuristic tries to choose a random variable.        (default 0.02) NOTE: This is really strange. If the number of variables set is large, then the random chance is in fact _far_ lower than this value. This is because the algorithm tries to set one variable randomly, but if that variable is already set, then it _silently_ fails, and moves on (doing non-random flip)!
    double    clause_decay;       ///<Inverse of the clause activity decay factor. Only applies if using MiniSat-style clause activities  (default: 1 / 0.999)
    int       restart_first;      ///<The initial restart limit.                                                                (default 100)
    double    restart_inc;        ///<The factor with which the restart limit is multiplied in each restart.                    (default 1.5)
    double    learntsize_factor;  ///<The intitial limit for learnt clauses is a factor of the original clauses.                (default 1 / 3)
    double    learntsize_inc;     ///<The limit for learnt clauses is multiplied with this factor each restart.                 (default 1.1)
    bool      expensive_ccmin;    ///<Should clause minimisation by Sorensson&Biere be used?                                    (default TRUE)
    int       polarity_mode;      ///<Controls which polarity the decision heuristic chooses. Auto means Jeroslow-Wang          (default: polarity_auto)
    int       verbosity;          ///<Verbosity level. 0=silent, 1=some progress report, 2=lots of report, 3 = all report       (default 2)
    Var       restrictedPickBranch;///<Pick variables to branch on preferentally from the highest [0, restrictedPickBranch]. If set to 0, preferentiality is turned off (i.e. picked randomly between [0, all])
    bool      findNormalXors;     ///<Automatically find non-binary xor clauses and convert them to xor clauses
    bool      findBinaryXors;     ///<Automatically find binary xor clauses (i.e. variable equi- and antivalences)
    bool      regFindBinaryXors;  ///<Regularly find binary xor clauses (i.e. variable equi- and antivalences)
    bool      doReplace;          ///<Should var-replacing be performed? If set to FALSE, equi- and antivalent variables will not be replaced with one another. NOTE: This precludes using a lot of the algorithms!
    bool      conglomerateXors;   ///<Do variable elimination at the XOR-level (xor-ing 2 xor clauses thereby removing a variable)
    bool      heuleProcess;       ///<Perform local subsitutuion as per Heule's theis
    bool      schedSimplification;///<Should simplifyProblem() be scheduled regularly? (if set to FALSE, a lot of opmitisations are disabled)
    bool      doSubsumption;      ///<Should try to subsume & self-subsuming resolve & variable-eliminate & block-clause eliminate?
    bool      doXorSubsumption;   ///<Should try to subsume & local-subsitute xor clauses
    bool      doPartHandler;      ///<Should try to find disconnected components and solve them individually?
    bool      doHyperBinRes;      ///<Should try carry out hyper-binary resolution
    bool      doBlockedClause;    ///<Should try to remove blocked clauses
    bool      doVarElim;          ///<Perform variable elimination
    bool      doSubsume1;         ///<Perform self-subsuming resolution
    bool      doAsymmBranch;      ///<Perform asymmetric branching at the beginning of the solving
    bool      doAsymmBranchReg;   ///<Perform asymmetric branching regularly
    bool      doSortWatched;      ///<Sort watchlists according to size&type: binary, tertiary, normal (>3-long), xor clauses
    bool      doMinimLearntMore;  ///<Perform learnt-clause minimisation using watchists' binary and tertiary clauses? ("strong minimization" in PrecoSat)
    bool      doMinimLMoreRecur;  ///<Always perform recursive/transitive on-the-fly self self-subsuming resolution --> an enhancement of "strong minimization" of PrecoSat
    bool      failedVarSearch;    ///<Do Failed literal probing + doubly propagated literal detection + 2-long xor clause detection during failed literal probing + hyper-binary resoolution
    bool      addExtraBins;       ///<Should perform hyper-binary resolution during failed literal probing?
    bool      remUselessBins;     ///<Should try to remove useless binary clauses at the beginning of solving?
    bool      regRemUselessBins;  ///<Should try to remove useless binary clauses regularly?
    bool      subsWNonExistBins;  ///<Try to do subsumption and self-subsuming resolution with non-existent binary clauses (i.e. binary clauses that don't exist but COULD exists)
    bool      regSubsWNonExistBins;
    bool      needToInterrupt;    ///<Used internally mostly. If set to TRUE, we will interrupt cleanly ASAP. The important thing is "cleanly", since we need to wait until a point when all datastructures are in a sane state (i.e. not in the middle of some algorithm)
    bool      needToDumpLearnts;  ///<If set to TRUE, learnt clauses will be dumped to the file speified by "learntsFilename"
    bool      needToDumpOrig;     ///<If set to TRUE, a simplified version of the original clause-set will be dumped to the file speified by "origFilename". The solution to this file should perfectly satisfy the problem
    char*     learntsFilename;    ///<Dump sorted learnt clauses to this file. Only active if "needToDumpLearnts" is set to TRUE
    char*     origFilename;       ///<Dump simplified original problem CNF to this file. Only active if "needToDumpOrig" is set to TRUE
    uint32_t  maxDumpLearntsSize; ///<When dumping the learnt clauses, this is the maximum clause size that should be dumped
    bool      libraryUsage;       ///<Set to true if not used as a library. In fact, this is TRUE by default, and Main.cpp sets it to "FALSE". Disables some simplifications at the beginning of solving (mostly performStepsBeforeSolve() )
    bool      greedyUnbound;      ///<If set, then variables will be greedily unbounded (set to l_Undef). This is EXPERIMENTAL
    RestartType fixRestartType;   ///<If set, the solver will always choose the given restart strategy instead of automatically trying to guess a strategy. Note that even if set to dynamic_restart, there will be a few restarts made statically after each full restart.
    GaussianConfig gaussconfig;   ///<Configuration for the gaussian elimination can be set here

    enum { polarity_true = 0, polarity_false = 1, polarity_rnd = 3, polarity_auto = 4};

    // Statistics: (read-only member variable)
    //
    uint64_t starts; ///<Num restarts
    uint64_t dynStarts; ///<Num dynamic restarts
    uint64_t staticStarts; ///<Num static restarts: note that after full restart, we do a couple of static restarts always
    /**
    @brief Num full restarts

    Full restarts are restarts that are made always, no matter what, after
    a certan number of conflicts have passed. The problem will tried to be
    decomposed into multiple parts, and then there will be a couple of static
    restarts made. Finally, the problem will be determined to be MiniSat-type
    or Glucose-type.

    NOTE: I belive there is a point in having full restarts even if the
    glue-clause vs. MiniSat clause can be fully resolved
    */
    uint64_t fullStarts;    ///<Number of full restarts made
    uint64_t decisions;     ///<Number of decisions made
    uint64_t rnd_decisions; ///<Numer of random decisions made
    /**
    @brief An approximation of accumulated propagation difficulty

    It does not hold the number of propagations made. Rather, it hold a
    value that is approximate of the difficulty of the propagations made
    This makes sense, since it is not at all the same difficulty to proapgate
    a 2-long clause than to propagate a 20-long clause. In certain algorihtms,
    there is a need to know how difficult the propagation part was. This value
    can be used in these algorihms. However, the reported "statistic" will be
    bogus.
    */
    uint64_t propagations;
    uint64_t conflicts; ///<Num conflicts
    uint64_t clauses_literals, learnts_literals, max_literals, tot_literals;
    uint64_t nbGlue2; ///<Num learnt clauses that had a glue of 2 when created
    uint64_t nbBin; ///<Num learnt clauses that were binary when created
    uint64_t lastNbBin; ///<Last time we seached for binary xors, the number of clauses in binaryClauses was this much
    /**
    @brief When a clause becomes binary through shrinking, we increment this

    It is used to determine if we should try to look for binary xors among
    the binary clauses
    */
    uint64_t becameBinary;
    uint64_t lastSearchForBinaryXor; ///<Last time we looked for binary xors, this many bogoprops(=propagations) has been done
    uint64_t nbReduceDB; ///<Number of times learnt clause have been cleaned
    uint64_t improvedClauseNo; ///<Num clauses improved using on-the-fly subsumption
    uint64_t improvedClauseSize; ///<Num literals removed using on-the-fly subsumption
    uint64_t numShrinkedClause; ///<Num clauses improved using on-the-fly self-subsuming resolution
    uint64_t numShrinkedClauseLits; ///<Num literals removed by on-the-fly self-subsuming resolution
    uint64_t moreRecurMinLDo, moreRecurMinLDoLit, moreRecurMinLStop; ///<Stats about transitive on-the-fly self-subsuming resolution

    //Logging
    void needStats();              // Prepares the solver to output statistics
    void needProofGraph();         // Prepares the solver to output proof graphs during solving
    void setVariableName(const Var var, const char* name); // Sets the name of the variable 'var' to 'name'. Useful for statistics and proof logs (i.e. used by 'logger')
    const vec<Clause*>& get_sorted_learnts(); //return the set of learned clauses, sorted according to the logic used in MiniSat to distinguish between 'good' and 'bad' clauses
    const vec<Clause*>& get_learnts() const; //Get all learnt clauses that are >1 long
    const vector<Lit> get_unitary_learnts() const; //return the set of unitary learnt clauses
    const uint32_t get_unitary_learnts_num() const; //return the number of unitary learnt clauses
    void dumpSortedLearnts(const char* file, const uint32_t maxSize); // Dumps all learnt clauses (including unitary ones) into the file
    void needLibraryCNFFile(const char* fileName); //creates file in current directory with the filename indicated, and puts all calls from the library into the file.
    void dumpOrigClauses(const char* fileName, const bool alsoLearntBin = false) const;

    #ifdef USE_GAUSS
    const uint32_t get_sum_gauss_called() const;
    const uint32_t get_sum_gauss_confl() const;
    const uint32_t get_sum_gauss_prop() const;
    const uint32_t get_sum_gauss_unit_truths() const;
    #endif //USE_GAUSS

    //Printing statistics
    const uint32_t getNumElimSubsume() const;       ///<Get variable elimination stats from Subsumer
    const uint32_t getNumElimXorSubsume() const;    ///<Get variable elimination stats from XorSubsumer
    const uint32_t getNumXorTrees() const;          ///<Get the number of trees built from 2-long XOR-s. This is effectively the number of variables that replace other variables
    const uint32_t getNumXorTreesCrownSize() const; ///<Get the number of variables being replaced by other variables
    /**
    @brief Get total time spent in Subsumer.

    This includes: subsumption, self-subsuming resolution, variable elimination,
    blocked clause elimination, subsumption and self-subsuming resolution
    using non-existent binary clauses.
    */
    const double   getTotalTimeSubsumer() const;

    /**
    @brief Get total time spent in XorSubsumer.

    This included subsumption, variable elimination through XOR, and local
    substitution (see Heule's Thesis)
    */
    const double   getTotalTimeXorSubsumer() const;

protected:
    #ifdef USE_GAUSS
    void print_gauss_sum_stats();
    void clearGaussMatrixes();
    vector<Gaussian*> gauss_matrixes;

    //stats
    uint32_t sum_gauss_called;
    uint32_t sum_gauss_confl;
    uint32_t sum_gauss_prop;
    uint32_t sum_gauss_unit_truths;
    #endif //USE_GAUSS

    // Helper structures:
    //
    struct VarOrderLt {
        const vec<uint32_t>&  activity;
        bool operator () (Var x, Var y) const {
            return activity[x] > activity[y];
        }
        VarOrderLt(const vec<uint32_t>&  act) : activity(act) { }
    };

    struct VarFilter {
        const Solver& s;
        VarFilter(const Solver& _s) : s(_s) {}
        bool operator()(Var v) const {
            return s.assigns[v].isUndef() && s.decision_var[v];
        }
    };

    // Solver state:
    //
    bool                ok;               ///< If FALSE, the constraints are already unsatisfiable. No part of the solver state may be used!
    ClauseAllocator     clauseAllocator;  ///< Handles memory allocation for claues
    vec<Clause*>        clauses;          ///< List of problem clauses that are normally larger than 2. Sometimes, due to on-the-fly self-subsuming resoulution, clauses here become 2-long. They are never purposfully put here such that they are long
    vec<Clause*>        binaryClauses;    ///< Binary clauses are regularly moved here. When Clause::sorted is true, they are sorted here
    vec<XorClause*>     xorclauses;       ///< List of problem xor-clauses. Will be freed
    vec<Clause*>        learnts;          ///< List of learnt clauses.
    vec<XorClause*>     freeLater;        ///< xor clauses that need to be freed later (this is needed due to Gauss) \todo Get rid of this
    vec<uint32_t>       activity;         ///< A heuristic measurement of the activity of a variable.
    uint32_t            var_inc;          ///< Amount to bump next variable with.
    double              cla_inc;          ///< Amount to bump learnt clause oldActivity with
    vec<vec<Watched> >  watches;          ///< 'watches[lit]' is a list of constraints watching 'lit' (will go there if literal becomes true).
    vec<lbool>          assigns;          ///< The current assignments
    vector<bool>        decision_var;     ///< Declares if a variable is eligible for selection in the decision heuristic.
    vec<Lit>            trail;            ///< Assignment stack; stores all assigments made in the order they were made.
    vec<uint32_t>       trail_lim;        ///< Separator indices for different decision levels in 'trail'.
    vec<PropagatedFrom> reason;           ///< 'reason[var]' is the clause that implied the variables current value, or 'NULL' if none.
    vec<int32_t>        level;            ///< 'level[var]' contains the level at which the assignment was made.
    uint64_t            numCleanedLearnts;///< Number of times learnt clauses have been removed through simplify() up until now
    uint32_t            nbClBeforeRed;    ///< Number of learnt clauses before learnt-clause cleaning
    uint32_t            nbCompensateSubsumer; ///< Number of learnt clauses that subsumed normal clauses last time subs. was executed (used to delay learnt clause-cleaning)
    uint32_t            qhead;            ///< Head of queue (as index into the trail)
    Lit                 failBinLit;       ///< Used to store which watches[~lit] we were looking through when conflict occured
    vec<Lit>            assumptions;      ///< Current set of assumptions provided to solve by the user.
    Heap<VarOrderLt>    order_heap;       ///< A priority queue of variables ordered with respect to the variable activity. All variables here MUST be decision variables. If you changed the decision variables, you MUST filter this
    #ifdef RANDOM_LOOKAROUND_SEARCHSPACE
    bqueue<uint32_t>    avgBranchDepth;   ///< Avg branch depth. We collect this, and use it to do random look-around in the searchspace during simplifyProblem()
    #endif //RANDOM_LOOKAROUND_SEARCHSPACE
    MTRand              mtrand;           ///< random number generator
    uint32_t            maxRestarts;      ///< More than this number of restarts will not be performed. Instead, we will stop, and optinally dump the reduced problem and/or the learnt clauses

    /////////////////////////
    // For glue calculation & dynamic restarts
    /////////////////////////
    uint64_t MYFLAG; ///<For glue calculation
    template<class T>
    const uint32_t calcNBLevels(const T& ps);
    vec<uint64_t> permDiff;  ///<permDiff[var] is used to count the number of different decision level variables in learnt clause (filled with data from MYFLAG )
    #ifdef UPDATEVARACTIVITY
    vec<Var>            lastDecisionLevel;
    #endif
    bqueue<uint32_t>    glueHistory;  ///< Set of last decision levels in (glue of) conflict clauses. Used for dynamic restarting
    double              totalSumOfGlue; ///< The total decision levels in (glues of) conflict clauses. This is used to calculate the average glue overall. It is used to dynamically restart the solving
    uint64_t            compTotSumGlue;    ///< To compensate for conflicts that were made but which have not been accumulated into Solver::totalSumOfDecisionLevel, so when we calculate the average, we must take this into account (i.e,. substruct it from Solver::conflicts )

    // Temporaries (to reduce allocation overhead). Each variable is prefixed by the method in which it is
    // used, exept 'seen' wich is used in several places.
    //
    vector<bool>        seen; ///<Used in multiple places. Contains 2 * numVars() elements, all zeroed out
    vector<bool>        seen2; ///<Used in minimiseLeartFurther(), contains 2 * numVars() elements, all zeroed out
    vec<Lit>            analyze_stack;
    vec<Lit>            analyze_toclear;
    vec<Lit> allAddedToSeen2; ///<To reduce temoprary data creation overhead. Used in minimiseLeartFurther()
    std::stack<Lit> toRecursiveProp; ///<To reduce temoprary data creation overhead. Used in minimiseLeartFurther()

    ////////////
    //Logging
    ///////////
    #ifdef STATS_NEEDED
    Logger   logger;                     // dynamic logging, statistics
    bool     dynamic_behaviour_analysis; // Is logger running?
    #endif
    uint32_t learnt_clause_group;       //the group number of learnt clauses. Incremented at each added learnt clause
    FILE     *libraryCNFFile;           //The file that all calls from the library are logged

    /////////////////
    // Propagating
    ////////////////
    Lit      pickBranchLit    ();                                                      // Return the next decision variable.
    void     newDecisionLevel ();                                                      // Begins a new decision level.
    void     uncheckedEnqueue (const Lit p, const PropagatedFrom& from = PropagatedFrom()); // Enqueue a literal. Assumes value of literal is undefined.
    void     uncheckedEnqueueLight (const Lit p);
    PropagatedFrom propagateBin();
    PropagatedFrom propagate(const bool update = true); // Perform unit propagation. Returns possibly conflicting clause.
    void     propTriClause   (Watched* &i, Watched* &j, const Watched *end, const Lit& p, PropagatedFrom& confl);
    void     propBinaryClause(Watched* &i, Watched* &j, const Watched *end, const Lit& p, PropagatedFrom& confl);
    void     propNormalClause(Watched* &i, Watched* &j, const Watched *end, const Lit& p, PropagatedFrom& confl, const bool update);
    void     propXorClause   (Watched* &i, Watched* &j, const Watched *end, const Lit& p, PropagatedFrom& confl);
    void     sortWatched();

    ///////////////
    // Conflicting
    ///////////////
    void     cancelUntil      (int level);                                             // Backtrack until a certain level.
    Clause*  analyze          (PropagatedFrom confl, vec<Lit>& out_learnt, int& out_btlevel, uint32_t &nblevels, const bool update); // (bt = backtrack)
    void     analyzeFinal     (Lit p, vec<Lit>& out_conflict);                         // COULD THIS BE IMPLEMENTED BY THE ORDINARIY "analyze" BY SOME REASONABLE GENERALIZATION?
    bool     litRedundant     (Lit p, uint32_t abstract_levels);                       // (helper method for 'analyze()')
    void     insertVarOrder   (Var x);                                                 // Insert a variable in the decision order priority queue.
    void     minimiseLeartFurther(vec<Lit>& cl);

    /////////////////
    // Searching
    /////////////////
    lbool    search           (int nof_conflicts, int nof_conflicts_fullrestart, const bool update = true);      // Search for a given number of conflicts.
    llbool   handle_conflict  (vec<Lit>& learnt_clause, PropagatedFrom confl, int& conflictC, const bool update);// Handles the conflict clause
    llbool   new_decision     (const int& nof_conflicts, const int& nof_conflicts_fullrestart, int& conflictC);  // Handles the case when all propagations have been made, and now a decision must be made

    /////////////////
    // Maintaining Variable/Clause activity:
    /////////////////
    void     claBumpActivity (Clause& c);
    void     varDecayActivity ();                      // Decay all variables with the specified factor. Implemented by increasing the 'bump' value instead.
    void     varBumpActivity  (Var v);                 // Increase a variable with the current 'bump' value.
    void     claDecayActivity ();                      // Decay all clauses with the specified factor. Implemented by increasing the 'bump' value instead.

    /////////////////
    // Operations on clauses:
    /////////////////
    template <class T>
    Clause*    addClauseInt(T& ps, uint32_t group);
    template<class T>
    XorClause* addXorClauseInt(T& ps, bool xorEqualFalse, const uint32_t group);
    void       attachClause     (XorClause& c);
    void       attachClause     (Clause& c);             // Attach a clause to watcher lists.
    void       detachClause     (const XorClause& c);
    void       detachClause     (const Clause& c);       // Detach a clause to watcher lists.
    void       detachModifiedClause(const Lit lit1, const Lit lit2, const Lit lit3, const uint32_t origSize, const Clause* address);
    void       detachModifiedClause(const Var var1, const Var var2, const uint32_t origSize, const XorClause* address);
    template<class T>
    void       removeClause(T& c);                       // Detach and free a clause.
    bool       locked           (const Clause& c) const; // Returns TRUE if a clause is a reason for some implication in the current state.

    ///////////////////////////
    // Debug clause attachment
    ///////////////////////////
    void       testAllClauseAttach() const;
    void       findAllAttach() const;
    const bool findClause(XorClause* c) const;
    const bool findClause(Clause* c) const;

    // Misc:
    //
    uint32_t decisionLevel    ()      const; // Gives the current decisionlevel.
    uint32_t abstractLevel    (const Var& x) const; // Used to represent an abstraction of sets of decision levels.

    /////////////////////////
    //Classes that must be friends, since they accomplish things on our datastructures
    /////////////////////////
    friend class VarFilter;
    friend class Gaussian;
    friend class FindUndef;
    friend class Logger;
    friend class XorFinder;
    friend class Conglomerate;
    friend class MatrixFinder;
    friend class PartFinder;
    friend class VarReplacer;
    friend class ClauseCleaner;
    friend class RestartTypeChooser;
    friend class FailedVarSearcher;
    friend class Subsumer;
    friend class XorSubsumer;
    friend class PartHandler;
    friend class StateSaver;
    friend class UselessBinRemover;
    friend class OnlyNonLearntBins;
    friend class ClauseAllocator;
    friend class CompleteDetachReatacher;
    Conglomerate*       conglomerate;
    VarReplacer*        varReplacer;
    ClauseCleaner*      clauseCleaner;
    FailedVarSearcher*  failedVarSearcher;
    PartHandler*        partHandler;
    Subsumer*           subsumer;
    XorSubsumer*        xorSubsumer;
    RestartTypeChooser* restartTypeChooser;
    MatrixFinder*       matrixFinder;

    /////////////////////////
    // Restart type handling
    /////////////////////////
    const bool  chooseRestartType(const uint32_t& lastFullRestart);
    void        setDefaultRestartType();
    const bool  checkFullRestart(int& nof_conflicts, int& nof_conflicts_fullrestart, uint32_t& lastFullRestart);
    RestartType restartType;             ///<Used internally to determine which restart strategy is currently in use
    RestartType lastSelectedRestartType; ///<The last selected restart type. Used when we are just after a full restart, and need to know how to really act

    //////////////////////////
    // Problem simplification
    //////////////////////////
    void        performStepsBeforeSolve();
    const lbool simplifyProblem(const uint32_t numConfls);
    void        reduceDB();       // Reduce the set of learnt clauses.
    const bool  simplify();       // Removes satisfied clauses and finds binary xors
    bool        simplifying;      ///<We are currently doing burst search
    uint32_t    simpDB_assigns;   ///< Number of top-level assignments since last execution of 'simplify()'.
    int64_t     simpDB_props;     ///< Remaining number of propagations that must be made before next execution of 'simplify()'.

    /////////////////////////////
    // SAT solution verification
    /////////////////////////////
    void       checkSolution    ();
    const bool verifyModel      () const;
    const bool verifyClauses    (const vec<Clause*>& cs) const;
    const bool verifyXorClauses (const vec<XorClause*>& cs) const;

    // Debug & etc:
    void     printLit         (const Lit l) const;
    void     checkLiteralCount();
    void     printStatHeader  () const;
    void     printRestartStat (const char* type = "N");
    void     printEndSearchStat();
    void     interruptCleanly();
    void     addSymmBreakClauses();
    void     initialiseSolver();

    /////////////////////
    // Polarity chooser
    /////////////////////
    void calculateDefaultPolarities(); //Calculates the default polarity for each var, and fills defaultPolarities[] with it
    bool defaultPolarity(); //if polarity_mode is not polarity_auto, this returns the default polarity of the variable
    void tallyVotes(const vec<Clause*>& cs, vector<double>& votes) const;
    void tallyVotes(const vec<XorClause*>& cs, vector<double>& votes) const;
    void setPolarity(Var v, bool b); // Declare which polarity the decision heuristic should use for a variable. Requires mode 'polarity_user'.
    vector<bool> polarity;      // The preferred polarity of each variable.
    #ifdef USE_OLD_POLARITIES
    vector<bool> oldPolarity;   // The polarity before the last setting. Good for unsetting polairties that have been changed since the last conflict
    #endif //USE_OLD_POLARITIES
};


//=================================================================================================
// Implementation of inline methods:


inline void Solver::insertVarOrder(Var x)
{
    if (!order_heap.inHeap(x) && decision_var[x]) order_heap.insert(x);
}

inline void Solver::varDecayActivity()
{
    var_inc *= 11;
    var_inc /= 10;
}
inline void Solver::varBumpActivity(Var v)
{
    if ( (activity[v] += var_inc) > (0x1U) << 24 ) {
        //printf("RESCALE!!!!!!\n");
        //std::cout << "var_inc: " << var_inc << std::endl;
        // Rescale:
        for (Var var = 0; var != nVars(); var++) {
            activity[var] >>= 14;
        }
        var_inc >>= 14;
        //var_inc = 1;
        //std::cout << "var_inc: " << var_inc << std::endl;

        /*Heap<VarOrderLt> copy_order_heap2(order_heap);
        while(!copy_order_heap2.empty()) {
            Var v = copy_order_heap2.getmin();
            if (decision_var[v])
                std::cout << "var_" << v+1 << " act: " << activity[v] << std::endl;
        }*/
    }

    // Update order_heap with respect to new activity:
    if (order_heap.inHeap(v))
        order_heap.decrease(v);
}

inline void Solver::claBumpActivity (Clause& c)
{
    if ( (c.getMiniSatAct() += cla_inc) > 1e20 ) {
        // Rescale:
        for (uint32_t i = 0; i < learnts.size(); i++)
            learnts[i]->getMiniSatAct() *= 1e-17;
        cla_inc *= 1e-20;
    }
}

inline void Solver::claDecayActivity()
{
    //cla_inc *= clause_decay;
}

inline bool Solver::locked(const Clause& c) const
{
    if (c.size() <= 3) return true; //we don't know in this case :I
    PropagatedFrom from(reason[c[0].var()]);
    return from.isClause() && from.getClause() == &c && value(c[0]) == l_True;
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
    return clauses.size() + xorclauses.size()+binaryClauses.size();
}
inline uint32_t      Solver::nLiterals      ()      const
{
    return clauses_literals + learnts_literals;
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
inline void     Solver::setVariableName(const Var var, const char* name)
{
    while (var >= nVars()) newVar();
    if (dynamic_behaviour_analysis)
        logger.set_variable_name(var, name);
} // Sets the varible 'var'-s name to 'name' in the logger
#else
inline void     Solver::setVariableName(const Var var, const char* name)
{}
#endif

#ifdef USE_GAUSS
inline const uint32_t Solver::get_sum_gauss_unit_truths() const
{
    return sum_gauss_unit_truths;
}

inline const uint32_t Solver::get_sum_gauss_called() const
{
    return sum_gauss_called;
}

inline const uint32_t Solver::get_sum_gauss_confl() const
{
    return sum_gauss_confl;
}

inline const uint32_t Solver::get_sum_gauss_prop() const
{
    return sum_gauss_prop;
}
#endif

inline const uint32_t Solver::get_unitary_learnts_num() const
{
    if (decisionLevel() > 0)
        return trail_lim[0];
    else
        return trail.size();
}

//////////////////
// Xor Clause
//////////////////


/*inline void Solver::calculate_xor_clause(Clause& c2) const {
    if (c2.isXor() && ((XorClause*)&c2)->updateNeeded())  {
        XorClause& c = *((XorClause*)&c2);
        bool final = c.xorEqualFalse();
        for (int k = 0, size = c.size(); k != size; k++ ) {
            const lbool& val = assigns[c[k].var()];
            assert(val != l_Undef);

            c[k] = c[k].unsign() ^ val.getBool();
            final ^= val.getBool();
        }
        if (final)
            c[0] = c[0].unsign() ^ !assigns[c[0].var()].getBool();

        c.setUpdateNeeded(false);
    }
}*/

template<class T>
inline void Solver::removeClause(T& c)
{
    detachClause(c);
    clauseAllocator.clauseFree(&c);
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

#ifndef DEBUG_ATTACH
inline void Solver::testAllClauseAttach() const
{
    return;
}
inline void Solver::findAllAttach() const
{
    return;
}
#endif //DEBUG_ATTACH

inline void Solver::uncheckedEnqueueLight(const Lit p)
{
    assert(assigns[p.var()] == l_Undef);

    assigns [p.var()] = boolToLBool(!p.sign());//lbool(!sign(p));  // <<== abstract but not uttermost effecient
    trail.push(p);
}

//=================================================================================================
#endif //SOLVER_H
