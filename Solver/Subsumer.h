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
    
private:
    
    friend class ClauseCleaner;
    friend class ClauseAllocator;
    
    //Main
    vec<ClauseSimp>        clauses;
    vec<char>              touched;        // Is set to true when a variable is part of a removed clause. Also true initially (upon variable creation).
    vec<Var>               touched_list;   // A list of the true elements in 'touched'.
    CSet                   cl_touched;     // Clauses strengthened.
    vec<vec<ClauseSimp> >  occur;          // 'occur[index(lit)]' is a list of constraints containing 'lit'.
    vec<CSet* >            iter_sets;      // Sets currently used for iterations.
    vec<char>              cannot_eliminate;//
    vec<char>              seen_tmp; // (used in various places)

    //Global stats
    Solver& solver;
    vec<char> var_elimed; //TRUE if var has been eliminated
    double totalTime;
    uint32_t numElimed;
    map<Var, vector<Clause*> > elimedOutVar;

    //Limits
    uint32_t numVarsElimed;
    uint32_t numMaxSubsume1;
    uint32_t numMaxSubsume0;
    uint32_t numMaxElim;
    int64_t numMaxBlockToVisit;
    uint32_t numMaxBlockVars;
    
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
    void subsume0(vec<Lit>& ps, uint32_t abs);
    template<class T>
    subsume0Happened subsume0Orig(const T& ps, uint32_t abs);
    void subsume0Touched();

    //subsume1
    void subsume1(Clause& ps);
    void strenghten(ClauseSimp c, const Lit toRemoveLit);
    void handleSize1Clause(const Lit lit);

    //Variable elimination
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
    vec<Lit> toVisit;
    vec<char> toVisitAll;
    
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
    
    uint32_t clauses_subsumed;
    uint32_t literals_removed;
    uint32_t numCalls;
    uint32_t clauseID;
    bool subsWithBins;
};

template <class T, class T2>
void maybeRemove(vec<T>& ws, const T2& elem)
{
    if (ws.size() > 0)
        removeW(ws, elem);
}

inline void Subsumer::touch(const Var x)
{
    if (!touched[x]) {
        touched[x] = 1;
        touched_list.push(x);
    }
}

inline void Subsumer::touchBlockedVar(const Var x)
{
    if (!touchedBlockedVarsBool[x]) {
        touchedBlockedVars.push(VarOcc(x, occur[Lit(x, false).toInt()].size()*occur[Lit(x, true).toInt()].size()));
        touchedBlockedVarsBool[x] = 1;
    }
}

inline void Subsumer::touch(const Lit p)
{
    touch(p.var());
}

inline bool Subsumer::subsetAbst(const uint32_t A, const uint32_t B)
{
    //A subsumes B? (everything that is in A MUST be in B)
    //if (A & ~B) contains even one bit, it means that A contains something
    //that B doesn't. So A may be a subset of B only if (A & ~B) == 0;
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

// Assumes 'seen' is cleared (will leave it cleared)
//Checks whether A subsumes(1) B
//Returns lit_Undef if it simply subsumes
//Returns lit_Error if it doesnt subsume1 or subsume0
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
