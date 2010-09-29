/**************************************************************************************************
Originally From: Solver.C -- (C) Niklas Een, Niklas Sorensson, 2004
Substantially modified by: Mate Soos (2010)
**************************************************************************************************/

#ifndef SIMPLIFIER_H
#define SIMPLIFIER_H

#include "Solver.h"
#include "Queue.h"
#include "CSet.h"
#include "BitArray.h"
#include <map>
#include <vector>
#include <queue>
using std::vector;
using std::map;
using std::priority_queue;

class ClauseCleaner;
class OnlyNonLearntBins;

/**
@brief Handles subsumption, self-subsuming resolution, variable elimination, and related algorithms

There are two main functions in this class, simplifyBySubsumption() and subsumeWithBinaries().
The first one is the most important of the two: it performs everything, except manipuation
with non-existing binary clauses. The second does self-subsuming resolution with existing binary
clauses, and then does self-subsuming resolution and subsumption with binary clauses that don't exist
and never will exist: they are temporarily "created" (memorised), and used by subsume0BIN().
*/
class Subsumer
{
public:

    //Construct-destruct
    Subsumer(Solver& S2);

    //Called from main
    const bool simplifyBySubsumption(const bool alsoLearnt = false);
    const bool subsumeWithBinaries(OnlyNonLearntBins* onlyNonLearntBins);
    void newVar();

    //Used by cleaner
    void unlinkClause(ClauseSimp cc, const Var elim = var_Undef);
    ClauseSimp linkInClause(Clause& cl);

    //UnElimination
    void extendModel(Solver& solver2);
    const bool unEliminate(const Var var);

    //Get-functions
    const vec<char>& getVarElimed() const;
    const uint32_t getNumElimed() const;
    const bool checkElimedUnassigned() const;
    const double getTotalTime() const;
    const map<Var, vector<Clause*> >& getElimedOutVar() const;

private:

    friend class ClauseCleaner;
    friend class ClauseAllocator;

    //Main
    /**
    @brief Clauses to be treated are moved here ClauseSimp::index refers to the index of the clause here
    */
    vec<ClauseSimp>        clauses;
    vec<char>              touched;        ///<Is set to true when a variable is part of a removed clause. Also true initially (upon variable creation).
    vec<Var>               touched_list;   ///<A list of the true elements in 'touched'.
    CSet                   cl_touched;     ///<Clauses strengthened/added
    vec<vec<ClauseSimp> >  occur;          ///<occur[index(lit)]' is a list of constraints containing 'lit'.
    vec<CSet* >            iter_sets;      ///<Sets currently used in iterations.
    vec<char>              cannot_eliminate;///<Variables that cannot be eliminated due to, e.g. XOR-clauses
    vec<char>              seen_tmp;       ///<Used in various places to help perform algorithms

    //Global stats
    Solver& solver;                        ///<The solver this simplifier is connected to
    vec<char> var_elimed;                  ///<Contains TRUE if var has been eliminated
    double totalTime;                      ///<Total time spent in this class
    uint32_t numElimed;                    ///<Total number of variables eliminated
    map<Var, vector<Clause*> > elimedOutVar; ///<Contains the clauses to use to uneliminate a variable

    //Limits
    uint32_t numVarsElimed;               ///<Number of variables elimed in this run
    uint32_t numMaxSubsume1;              ///<Max. number self-subsuming resolution tries to do this run
    uint32_t numMaxSubsume0;              ///<Max. number backward-subsumption tries to do this run
    uint32_t numMaxElim;                  ///<Max. number of variable elimination tries to do this run
    int64_t numMaxBlockToVisit;           ///<Max. number variable-blocking clauses to visit to do this run
    uint32_t numMaxBlockVars;             ///<Max. number variable-blocking tries to do this run

    //Start-up
    void addFromSolver(vec<Clause*>& cs, bool alsoLearnt = false, const bool addBinAndAddToCL = true);
    void fillCannotEliminate();
    void clearAll();
    void setLimits(const bool alsoLearnt);
    void subsume0AndSubsume1();

    //Finish-up
    void freeMemory();
    void addBackToSolver();
    void removeWrong(vec<Clause*>& cs);
    void removeAssignedVarsFromEliminated();

    //Iterations
    void registerIteration  (CSet& iter_set) { iter_sets.push(&iter_set); }
    void unregisterIteration(CSet& iter_set) { remove(iter_sets, &iter_set); }

    //Touching
    void touch(const Var x);
    void touch(const Lit p);

    //Findsubsumed
    template<class T>
    void findSubsumed(const T& ps, const uint32_t abst, vec<ClauseSimp>& out_subsumed);
    template<class T>
    void findSubsumed1(const T& ps, uint32_t abs, vec<ClauseSimp>& out_subsumed, vec<Lit>& out_lits);
    template<class T>
    void fillSubs(const T& ps, uint32_t abs, vec<ClauseSimp>& out_subsumed, vec<Lit>& out_lits, const Lit lit);
    template<class T2>
    bool subset(const uint32_t aSize, const T2& B);
    template<class T1, class T2>
    const Lit subset1(const T1& A, const T2& B);
    bool subsetAbst(uint32_t A, uint32_t B);

    //subsume0
    struct subsume0Happened {
        bool subsumedNonLearnt;
        uint32_t activity;
        float oldActivity;
    };
    void subsume0(Clause& ps);
    template<class T>
    subsume0Happened subsume0Orig(const T& ps, uint32_t abs);
    void subsume0Touched();

    //subsume1
    void subsume1(Clause& ps);
    void strenghten(ClauseSimp& c, const Lit toRemoveLit);
    const bool cleanClause(Clause& c);
    void handleSize1Clause(const Lit lit);

    //Variable elimination
    /**
    @brief Struct used to compare variable elimination difficulties

    Used to order variables according to their difficulty of elimination. Used by
    the std::sort() function. in \function orderVarsForElim()
    */
    struct myComp {
        bool operator () (const std::pair<int, Var>& x, const std::pair<int, Var>& y) {
            return x.first < y.first ||
                (!(y.first < x.first) && x.second < y.second);
        }
    };
    void orderVarsForElim(vec<Var>& order);
    bool maybeEliminate(Var x);
    void MigrateToPsNs(vec<ClauseSimp>& poss, vec<ClauseSimp>& negs, vec<ClauseSimp>& ps, vec<ClauseSimp>& ns, const Var x);
    bool merge(const Clause& ps, const Clause& qs, const Lit without_p, const Lit without_q, vec<Lit>& out_clause);

    //Subsume with Nonexistent Bins
    const bool subsWNonExistBinsFull(OnlyNonLearntBins* onlyNonLearntBins);
    const bool subsWNonExistBins(const Lit& lit, OnlyNonLearntBins* onlyNonLearntBins);
    void subsume0BIN(const Lit lit, const vec<char>& lits);
    bool subsNonExistentFinish;
    uint32_t doneNum;
    uint64_t extraTimeNonExist;
    vec<Lit> toVisit;      ///<Literals that we have visited from a given literal during subsumption w/ non-existent binaries (list)
    vec<char> toVisitAll;  ///<Literals that we have visited from a given literal during subsumption w/ non-existent binaries (contains '1' for literal.toInt() that we visited)

    //Blocked clause elimination
    class VarOcc {
        public:
            VarOcc(const Var& v, const uint32_t num) :
                var(v)
                , occurnum(num)
            {}
            Var var;
            uint32_t occurnum;
    };
    struct MyComp {
        const bool operator() (const VarOcc& l1, const VarOcc& l2) const {
            return l1.occurnum > l2.occurnum;
        }
    };
    void blockedClauseRemoval();
    const bool allTautology(const vec<Lit>& ps, const Lit lit);
    uint32_t numblockedClauseRemoved;
    const bool tryOneSetting(const Lit lit, const Lit negLit);
    priority_queue<VarOcc, vector<VarOcc>, MyComp> touchedBlockedVars;
    vec<bool> touchedBlockedVarsBool;
    void touchBlockedVar(const Var x);
    double blockTime;


    //validity checking
    const bool verifyIntegrity();

    uint32_t clauses_subsumed; ///<Number of clauses subsumed in this run
    uint32_t literals_removed; ///<Number of literals removed from clauses through self-subsuming resolution in this run
    uint32_t numCalls;         ///<Number of times simplifyBySubsumption() has been called
    uint32_t clauseID;         ///<We need to have clauseIDs since clauses don't natively have them. The ClauseID is stored by ClauseSimp, which also stores a pointer to the clause
    bool subsWithBins;         ///<Are we currently subsuming with binaries only?
};

template <class T, class T2>
void maybeRemove(vec<T>& ws, const T2& elem)
{
    if (ws.size() > 0)
        removeW(ws, elem);
}

/**
@brief Put varible in touched_list

call it when the number of occurrences of this variable changed.

@param[in] x The varible that must be put into touched_list
*/
inline void Subsumer::touch(const Var x)
{
    if (!touched[x]) {
        touched[x] = 1;
        touched_list.push(x);
    }
}

/**
@brief Put varible in touchedBlockedVars

call it when the number of occurrences of this variable changed.
*/
inline void Subsumer::touchBlockedVar(const Var x)
{
    if (!touchedBlockedVarsBool[x]) {
        touchedBlockedVars.push(VarOcc(x, occur[Lit(x, false).toInt()].size()*occur[Lit(x, true).toInt()].size()));
        touchedBlockedVarsBool[x] = 1;
    }
}

/**
@brief Put variable of literal in touched_list

call it when the number of occurrences of this variable changed
*/
inline void Subsumer::touch(const Lit p)
{
    touch(p.var());
}

/**
@brief Decides only using abstraction if clause A could subsume clause B

@note: It can give false positives. Never gives false negatives.

For A to subsume B, everything that is in A MUST be in B. So, if (A & ~B)
contains even one bit, it means that A contains something that B doesn't. So
A may be a subset of B only if (A & ~B) == 0
*/
inline bool Subsumer::subsetAbst(const uint32_t A, const uint32_t B)
{
    return !(A & ~B);
}

//A subsumes B (A is <= B)
template<class T2>
bool Subsumer::subset(const uint32_t aSize, const T2& B)
{
    uint32_t num = 0;
    for (uint32_t i = 0; i != B.size(); i++) {
        num += seen_tmp[B[i].toInt()];
    }
    return num == aSize;
}


/**
@brief Decides if A subsumes B, or if not, if A could strenghten B

@note: Assumes 'seen' is cleared (will leave it cleared)

Helper function findSubsumed1. Does two things in one go:
1) decides if clause A could subsume clause B
1) decides if clause A could be used to perform self-subsuming resoltuion on
clause B

@return lit_Error, if neither (1) or (2) is true. Returns lit_Undef (1) is true,
and returns the literal to remove if (2) is true
*/
template<class T1, class T2>
const Lit Subsumer::subset1(const T1& A, const T2& B)
{
    Lit retLit = lit_Undef;

    for (uint32_t i = 0; i != B.size(); i++)
        seen_tmp[B[i].toInt()] = 1;
    for (uint32_t i = 0; i != A.size(); i++) {
        if (!seen_tmp[A[i].toInt()]) {
            if (retLit == lit_Undef && seen_tmp[(~A[i]).toInt()])
                retLit = ~A[i];
            else {
                retLit = lit_Error;
                goto end;
            }
        }
    }

    end:
    for (uint32_t i = 0; i != B.size(); i++)
        seen_tmp[B[i].toInt()] = 0;
    return retLit;
}

/**
@brief New var has been added to the solver

@note: MUST be called if a new var has been added to the solver

Adds occurrence list places, increments seen_tmp, etc.
*/
inline void Subsumer::newVar()
{
    occur       .push();
    occur       .push();
    seen_tmp    .push(0);       // (one for each polarity)
    seen_tmp    .push(0);
    touched     .push(1);
    var_elimed  .push(0);
    touchedBlockedVarsBool.push(0);
    cannot_eliminate.push(0);
}

inline const map<Var, vector<Clause*> >& Subsumer::getElimedOutVar() const
{
    return elimedOutVar;
}

inline const vec<char>& Subsumer::getVarElimed() const
{
    return var_elimed;
}

inline const uint32_t Subsumer::getNumElimed() const
{
    return numElimed;
}

inline const double Subsumer::getTotalTime() const
{
    return totalTime;
}

#endif //SIMPLIFIER_H
