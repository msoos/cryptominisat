/*****************************************************************************
SatELite -- (C) Niklas Een, Niklas Sorensson, 2004
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

Original code by SatELite authors are under an MIT licence.
Modifications for CryptoMiniSat are under GPLv3.
******************************************************************************/

#ifndef SIMPLIFIER_H
#define SIMPLIFIER_H

#include "Solver.h"
#include "Queue.h"
#include "CSet.h"
#include "BitArray.h"
#include <map>
#include <vector>
#include <list>
#include <set>
#include <queue>
#include <set>
#include <iomanip>

using std::vector;
using std::list;
using std::map;
using std::priority_queue;

class ClauseCleaner;

class OrGate {
    public:
        Lit eqLit;
        vector<Lit> lits;
        //uint32_t num;
};

inline std::ostream& operator<<(std::ostream& os, const OrGate& gate)
{
    os << " gate ";
    //os << " no. " << std::setw(4) << gate.num;
    os << " lits: ";
    for (uint32_t i = 0; i < gate.lits.size(); i++) {
        os << gate.lits[i] << " ";
    }
    os << " eqLit: " << gate.eqLit;
    return os;
}

class TouchList
{
    public:
        void resize(uint32_t size)
        {
            touched.resize(size, 0);
        }

        void addOne(Var var)
        {
            assert(touched.size() == var);
            touched.push_back(1);
            touchedList.push_back(var);
        }

        void touch(Lit lit, const bool learnt)
        {
            if (!learnt) touch(lit.var());
        }

        void touch(Var var)
        {
            if (!touched[var]) {
                touchedList.push_back(var);
                touched[var]= 1;
            }
        }

        void clear()
        {
            touchedList.clear();
            std::fill(touched.begin(), touched.end(), 0);
        }

        const uint32_t size() const
        {
            return touchedList.size();
        }

        vector<Var>::const_iterator begin() const
        {
            return touchedList.begin();
        }

        vector<Var>::const_iterator end() const
        {
            return touchedList.end();
        }

    private:
        vector<Var> touchedList;
        vector<char> touched;
};

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
    const bool simplifyBySubsumption();
    void newVar();

    //UnElimination
    void extendModel(Solver& solver2);
    const bool unEliminate(const Var var);

    //Get-functions
    const vec<char>& getVarElimed() const;
    const uint32_t getNumElimed() const;
    const bool checkElimedUnassigned() const;
    const double getTotalTime() const;
    const map<Var, vector<vector<Lit> > >& getElimedOutVar() const;
    const map<Var, vector<std::pair<Lit, Lit> > >& getElimedOutVarBin() const;
    const uint32_t getNumERVars() const;
    void incNumERVars(const uint32_t num);
    void setVarNonEliminable(const Var var);
    const bool getFinishedAddingVars() const;
    void setFinishedAddingVars(const bool val);
    const vector<OrGate*>& getGateOcc(const Lit lit) const;

private:

    const bool subsumeWithBinaries();

    friend class ClauseCleaner;
    friend class ClauseAllocator;

    //Main
    /**
    @brief Clauses to be treated are moved here ClauseSimp::index refers to the index of the clause here
    */
    vec<Clause*>           clauses;
    vec<AbstData>          clauseData;
    TouchList              touchedVars;  ///<A list of the true elements in 'touched'.
    CSet                   cl_touched;       ///<Clauses strengthened/added
    vec<vec<ClauseSimp> >  occur;            ///<occur[index(lit)]' is a list of constraints containing 'lit'.
    CSet s0, s1;
    vec<char>              cannot_eliminate;///<Variables that cannot be eliminated due to, e.g. XOR-clauses
    vec<char>              seen_tmp;        ///<Used in various places to help perform algorithms
    vec<char>              seen_tmp2;       ///<Used in various places to help perform algorithms

    //Global stats
    Solver& solver;                        ///<The solver this simplifier is connected to
    vec<char> var_elimed;                  ///<Contains TRUE if var has been eliminated
    double totalTime;                      ///<Total time spent in this class
    uint32_t numElimed;                    ///<Total number of variables eliminated
    map<Var, vector<vector<Lit> > > elimedOutVar; ///<Contains the clauses to use to uneliminate a variable
    map<Var, vector<std::pair<Lit, Lit> > > elimedOutVarBin; ///<Contains the clauses to use to uneliminate a variable

    //Limits
    uint64_t addedClauseLits;
    uint32_t numVarsElimed;               ///<Number of variables elimed in this run
    int64_t numMaxSubsume1;              ///<Max. number self-subsuming resolution tries to do this run
    int64_t numMaxSubsume0;              ///<Max. number backward-subsumption tries to do this run
    int64_t numMaxElim;                  ///<Max. number of variable elimination tries to do this run
    int32_t numMaxElimVars;
    int64_t numMaxBlockToVisit;           ///<Max. number variable-blocking clauses to visit to do this run
    uint32_t numMaxBlockVars;             ///<Max. number variable-blocking tries to do this run

    //Start-up
    const uint64_t addFromSolver(vec<Clause*>& cs);
    void fillCannotEliminate();
    void clearAll();
    void setLimits();
    const bool subsume0AndSubsume1();
    vec<char> ol_seenPos;
    vec<char> ol_seenNeg;

    //Finish-up
    void freeMemory();
    void addBackToSolver();
    void removeWrong(vec<Clause*>& cs);
    void removeWrongBinsAndAllTris();
    void removeAssignedVarsFromEliminated();

    //Used by cleaner
    void unlinkClause(ClauseSimp cc, Clause& cl, const Var elim = var_Undef);
    ClauseSimp linkInClause(Clause& cl);
    const bool handleUpdatedClause(ClauseSimp& c, Clause& cl);

    //Findsubsumed
    template<class T>
    void findSubsumed(const uint32_t index,  const T& ps, const uint32_t abst, vec<ClauseSimp>& out_subsumed);
    template<class T>
    void findSubsumed1(const uint32_t index, const T& ps, uint32_t abs, vec<ClauseSimp>& out_subsumed, vec<Lit>& out_lits);
    template<class T>
    void fillSubs(const T& ps, const uint32_t index, uint32_t abs, vec<ClauseSimp>& out_subsumed, vec<Lit>& out_lits, const Lit lit);
    template<class T2>
    bool subset(const uint32_t aSize, const T2& B);
    template<class T1, class T2>
    const Lit subset1(const T1& A, const T2& B);
    bool subsetAbst(uint32_t A, uint32_t B);

    //binary clause-subsumption
    struct BinSorter {
        const bool operator()(const Watched& first, const Watched& second)
        {
            assert(first.isBinary() || first.isTriClause());
            assert(second.isBinary() || second.isTriClause());

            if (first.isTriClause() && second.isTriClause()) return false;
            if (first.isBinary() && second.isTriClause()) return true;
            if (second.isBinary() && first.isTriClause()) return false;

            assert(first.isBinary() && second.isBinary());
            if (first.getOtherLit().toInt() < second.getOtherLit().toInt()) return true;
            if (first.getOtherLit().toInt() > second.getOtherLit().toInt()) return false;
            if (first.getLearnt() == second.getLearnt()) return false;
            if (!first.getLearnt()) return true;
            return false;
        };
    };
    void subsumeBinsWithBins();

    //subsume0
    struct Sub0Ret {
        bool subsumedNonLearnt;
        uint32_t glue;
    };
    /**
    @brief Sort clauses according to size
    */
    struct sortBySize
    {
        const bool operator () (const Clause* x, const Clause* y)
        {
            return (x->size() < y->size());
        }
    };
    void subsume0(ClauseSimp c, Clause& ps);
    template<class T>
    Sub0Ret subsume0(const uint32_t index, const T& ps, uint32_t abs);
    void subsume0Touched();
    void makeNonLearntBin(const Lit lit1, const Lit lit2, const bool learnt);

    //subsume1
    class NewBinaryClause
    {
        public:
            NewBinaryClause(const Lit _lit1, const Lit _lit2, const bool _learnt) :
                lit1(_lit1), lit2(_lit2), learnt(_learnt)
            {};

            const Lit lit1;
            const Lit lit2;
            const bool learnt;
    };
    list<NewBinaryClause> clBinTouched; ///<Binary clauses strengthened/added
    const bool handleClBinTouched();

    void subsume1(ClauseSimp c, Clause& ps);
    const bool subsume1(vec<Lit>& ps, const bool wasLearnt);
    void strenghten(ClauseSimp& c, Clause& cl, const Lit toRemoveLit);
    const bool cleanClause(ClauseSimp& c, Clause& ps);

    //Variable elimination
    /**
    @brief Struct used to compare variable elimination difficulties

    Used to order variables according to their difficulty of elimination. Used by
    the std::sort() function. in \function orderVarsForElim()
    */
    struct myComp {
        bool operator () (const std::pair<int, Var>& x, const std::pair<int, Var>& y) {
            return x.first < y.first;
        }
    };
    class ClAndBin {
        public:
            ClAndBin(ClauseSimp& cl) :
                clsimp(cl)
                , lit1(lit_Undef)
                , lit2(lit_Undef)
                , isBin(false)
            {}

            ClAndBin(const Lit _lit1, const Lit _lit2) :
                clsimp(std::numeric_limits< uint32_t >::max())
                , lit1(_lit1)
                , lit2(_lit2)
                , isBin(true)
            {}

            ClauseSimp clsimp;
            Lit lit1;
            Lit lit2;
            bool isBin;
    };
    uint32_t numLearntBinVarRemAdded;
    void orderVarsForElim(vec<Var>& order);
    const uint32_t numNonLearntBins(const Lit lit) const;
    bool maybeEliminate(Var x);
    void addLearntBinaries(const Var var);
    void removeClauses(vec<ClAndBin>& posAll, vec<ClAndBin>& negAll, const Var var);
    void removeClausesHelper(vec<ClAndBin>& todo, const Var var, std::pair<uint32_t, uint32_t>& removed);
    bool merge(const ClAndBin& ps, const ClAndBin& qs, const Lit without_p, const Lit without_q, vec<Lit>& out_clause);
    const bool eliminateVars();
    void fillClAndBin(vec<ClAndBin>& all, vec<ClauseSimp>& cs, const Lit lit);

    void removeBinsAndTris(const Var var);
    const uint32_t removeBinAndTrisHelper(const Lit lit, vec<Watched>& ws);

    void makeAllBinsNonLearnt();

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
    template<class T>
    const bool allTautology(const T& ps, const Lit lit);
    uint32_t numblockedClauseRemoved;
    const bool tryOneSetting(const Lit lit);
    priority_queue<VarOcc, vector<VarOcc>, MyComp> touchedBlockedVars;
    vec<char> touchedBlockedVarsBool;
    void touchBlockedVar(const Var x);
    void blockedClauseElimAll(const Lit lit);

    //Gate extraction
    struct OrGateSorter {
        const bool operator() (const OrGate& gate1, const OrGate& gate2) {
            return (gate1.lits.size() > gate2.lits.size());
        }
    };
    struct OrGateSorter2 {
        const bool operator() (const OrGate& gate1, const OrGate& gate2) {
            if (gate1.lits.size() > gate2.lits.size()) return true;
            if (gate1.lits.size() < gate2.lits.size()) return false;

            assert(gate1.lits.size() == gate2.lits.size());
            for (uint32_t i = 0; i < gate1.lits.size(); i++) {
                if (gate1.lits[i] < gate2.lits[i]) return true;
                if (gate1.lits[i] > gate2.lits[i]) return false;
            }

            return false;
        }
    };
    const bool findOrGatesAndTreat();
    void findOrGates(const bool learntGatesToo);
    void findOrGate(const Lit eqLit, const ClauseSimp& c, const bool learntGatesToo);
    const bool shortenWithOrGate(const OrGate& gate);
    int64_t gateLitsRemoved;
    uint64_t totalOrGateSize;
    vector<OrGate> orGates;
    vector<vector<OrGate*> > gateOcc;
    uint32_t numOrGateReplaced;
    const bool findEqOrGates();
    const bool carryOutER();
    void createNewVars();
    const bool doAllOptimisationWithGates();
    uint32_t andGateNumFound;
    uint32_t andGateTotalSize;
    vec<char> dontElim;
    uint32_t numERVars;
    bool finishedAddingVars;

    const bool treatAndGates();
    const bool treatAndGate(const OrGate& gate, const bool reallyRemove, uint32_t& foundPotential, uint64_t& numOp);
    const bool treatAndGateClause(const ClauseSimp& other, const OrGate& gate, const Clause& cl);
    const bool findAndGateOtherCl(const vector<ClauseSimp>& sizeSortedOcc, const Lit lit, const uint32_t abst2, ClauseSimp& other);
    vector<vector<ClauseSimp> > sizeSortedOcc;

    //validity checking
    const bool verifyIntegrity();

    uint32_t clauses_subsumed; ///<Number of clauses subsumed in this run
    uint32_t literals_removed; ///<Number of literals removed from clauses through self-subsuming resolution in this run
    uint32_t numCalls;         ///<Number of times simplifyBySubsumption() has been called
    uint32_t clauseID;         ///<We need to have clauseIDs since clauses don't natively have them. The ClauseID is stored by ClauseSimp, which also stores a pointer to the clause
};

template <class T, class T2>
void maybeRemove(vec<T>& ws, const T2& elem)
{
    if (ws.size() > 0)
        removeW(ws, elem);
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
    seen_tmp2   .push(0);       // (one for each polarity)
    seen_tmp2   .push(0);
    touchedVars .addOne(solver.nVars()-1);
    var_elimed  .push(0);
    touchedBlockedVarsBool.push(0);
    cannot_eliminate.push(0);
    ol_seenPos.push(1);
    ol_seenPos.push(1);
    ol_seenNeg.push(1);
    ol_seenNeg.push(1);
    dontElim.push(0);
    gateOcc.push_back(vector<OrGate*>());
    gateOcc.push_back(vector<OrGate*>());
}

inline const map<Var, vector<vector<Lit> > >& Subsumer::getElimedOutVar() const
{
    return elimedOutVar;
}

inline const map<Var, vector<std::pair<Lit, Lit> > >& Subsumer::getElimedOutVarBin() const
{
    return elimedOutVarBin;
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

inline const uint32_t Subsumer::getNumERVars() const
{
    return numERVars;
}

inline void Subsumer::incNumERVars(const uint32_t num)
{
    numERVars += num;
}

inline void Subsumer::setVarNonEliminable(const Var var)
{
    dontElim[var] = true;
}

inline const bool Subsumer::getFinishedAddingVars() const
{
    return finishedAddingVars;
}

inline void Subsumer::setFinishedAddingVars(const bool val)
{
    finishedAddingVars = val;
}

inline const vector<OrGate*>& Subsumer::getGateOcc(const Lit lit) const
{
    return gateOcc[lit.toInt()];
}

#endif //SIMPLIFIER_H
