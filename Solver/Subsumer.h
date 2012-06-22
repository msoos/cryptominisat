/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2011, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/

#ifndef SIMPLIFIER_H
#define SIMPLIFIER_H

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
#include <fstream>
#include "Heap.h"

using std::vector;
using std::list;
using std::map;
using std::priority_queue;

class ClauseCleaner;
class SolutionExtender;
class Solver;
class GateFinder;
class XorFinder;

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

        uint32_t size() const
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
    Subsumer(Solver* solver);
    ~Subsumer();

    //Called from main
    bool simplifyBySubsumption();
    void newVar();
    void updateVars(
        const vector<uint32_t>& outerToInter
        , const vector<uint32_t>& interToOuter
    );

    //UnElimination
    void extendModel(SolutionExtender* extender) const;
    bool unEliminate(const Var var, Solver* tcontrol);

    //Get-functions
    struct Stats
    {
        Stats() :
            //Time
            linkInTime(0)
            , blockTime(0)
            , asymmTime(0)
            , subsBinWithBinTime(0)
            , subsumeTime(0)
            , strengthenTime(0)
            , varElimTime(0)
            , finalCleanupTime(0)

            //Startup stats
            , origNumFreeVars(0)
            , origNumMaxElimVars(0)
            , origNumIrredLongClauses(0)
            , origNumRedLongClauses(0)

            //Each algo
            , subsBinWithBin(0)
            , blocked(0)
            , blockedSumLits(0)
            , asymmSubs(0)
            , clauses_subsumed(0)
            , litsRemStrengthen(0)

            //Elimination
            , numVarsElimed(0)
            , clauses_elimed_long(0)
            , clauses_elimed_bin(0)
            , clauses_elimed_sumsize(0)
            , longLearntClRemThroughElim(0)
            , binLearntClRemThroughElim(0)
            , numLearntBinVarRemAdded(0)
            , triedToElimVars(0)
            , usedAgressiveCheckToELim(0)
            , newClauses(0)
            , newClauseNotAdded(0)
            , newClauseNotAddedFancy(0)

            , zeroDepthAssings(0)
        {
        }

        double totalTime() const
        {
            return linkInTime + blockTime + asymmTime
                + subsBinWithBinTime + subsumeTime + strengthenTime
                + varElimTime + finalCleanupTime;
        }

        void clear()
        {
            Stats stats;
            *this = stats;
        }

        Stats& operator+=(const Stats& other)
        {
            //Time
            linkInTime += other.linkInTime;
            blockTime += other.blockTime;
            asymmTime += other.asymmTime;
            subsBinWithBinTime += other.subsBinWithBinTime;
            subsumeTime += other.subsumeTime;
            strengthenTime += other.strengthenTime;
            varElimTime += other.varElimTime;
            finalCleanupTime += other.finalCleanupTime;

            //Startup stats
            origNumFreeVars += other.origNumFreeVars;
            origNumMaxElimVars += other.origNumMaxElimVars;
            origNumIrredLongClauses += other.origNumIrredLongClauses;
            origNumRedLongClauses += other.origNumRedLongClauses;

            //Each algo
            subsBinWithBin += other.subsBinWithBin;
            blocked += other.blocked;
            blockedSumLits += other.blockedSumLits;
            asymmSubs += other.asymmSubs;
            clauses_subsumed += other.clauses_subsumed;
            litsRemStrengthen += other.litsRemStrengthen;

            //Elim
            numVarsElimed += other.numVarsElimed;
            clauses_elimed_long += other.clauses_elimed_bin;
            clauses_elimed_bin += other.clauses_elimed_long;
            clauses_elimed_sumsize += other.clauses_elimed_sumsize;
            longLearntClRemThroughElim += other.longLearntClRemThroughElim;
            binLearntClRemThroughElim += other.binLearntClRemThroughElim;
            numLearntBinVarRemAdded += other.numLearntBinVarRemAdded;
            triedToElimVars += other.triedToElimVars;
            usedAgressiveCheckToELim += other.usedAgressiveCheckToELim;
            newClauses += other.newClauses;
            newClauseNotAdded += other.newClauseNotAdded;
            newClauseNotAddedFancy += other.newClauseNotAddedFancy;

            zeroDepthAssings += other.zeroDepthAssings;

            return *this;
        }

        void printShort() const
        {
            //Subs bin-w-bin
            cout
            << "c bin-w-bin subsume "
            << "rem " << subsBinWithBin
            << " time: " << std::fixed << std::setprecision(2)
            << subsBinWithBinTime
            << endl;

            //STRENGTH + SUBSUME
            cout << "c"
            << " lits-rem: " << litsRemStrengthen
            << " cl-subs: " << clauses_subsumed
            << " time: " << std::fixed << std::setprecision(2)
            << (subsumeTime+strengthenTime) << " s"
            << endl;

            //ELIM
            cout
            << "c [v-elim]"
            << " elimed: " << numVarsElimed
            << " / " << origNumMaxElimVars
            << " / " << origNumFreeVars
            //<< " cl-elim: " << (clauses_elimed_long+clauses_elimed_bin)
            << " T: " << std::fixed << std::setprecision(2)
            << varElimTime << " s"
            << endl;

            cout
            << "c [v-elim]"
            << " cl-new: " << newClauses
            << " ( "
            << (double)newClauseNotAdded/(double)newClauses*100.0
            << " % not add, "
            << " "
            << (double)newClauseNotAddedFancy/(double)newClauses*100.0
            << "% complic)"
            << " tried: " << triedToElimVars
            << " ("
            << (double)usedAgressiveCheckToELim/(double)triedToElimVars*100.0
            << " % agressive)"
            << endl;

            cout
            << "c [v-elim]"
            << " learnt-bin rem: " << binLearntClRemThroughElim
            << " learnt-long rem: " << longLearntClRemThroughElim
            << " v-fix: " << std::setw(4) << zeroDepthAssings
            << endl;
        }

        void print(const size_t nVars) const
        {
            cout << "c -------- SatELite STATS ----------" << endl;
            printStatsLine("c time"
                , totalTime());

            printStatsLine("c v-elimed"
                , numVarsElimed
                , (double)numVarsElimed/(double)nVars*100.0
                , "% vars");

            cout << "c"
            << " v-elimed: " << numVarsElimed
            << " / " << origNumMaxElimVars
            << " / " << origNumFreeVars
            << endl;

            printStatsLine("c 0-detph assigns"
                , zeroDepthAssings
                , (double)zeroDepthAssings/(double)nVars*100.0
                , "% vars"
            );

            printStatsLine("c lit-rem-str"
                , litsRemStrengthen
            );

            printStatsLine("c cl-new"
                , newClauses
                , (double)newClauseNotAdded/(double)newClauses*100.0
                , "% not added"
            );

            printStatsLine("c cl-new"
                , newClauses
                , (double)newClauseNotAddedFancy/(double)newClauses*100.0
                , "% not added due to asymm algo"
            );

            printStatsLine("c tried to elim"
                , triedToElimVars
                , (double)usedAgressiveCheckToELim/(double)triedToElimVars*100.0
                , "% agressively"
            );

            printStatsLine("c cl-subs"
                , clauses_subsumed
                , (double)clauses_subsumed/(double)(origNumIrredLongClauses+origNumRedLongClauses)
                , "% clauses"
            );

            printStatsLine("c subs bin-w-bin"
                , subsBinWithBin
            );

            printStatsLine("c blocked"
                , blocked
                , (double)blocked/(double)origNumIrredLongClauses
                , "% of irred clauses"
            );

            printStatsLine("c asymmSub"
                , asymmSubs);

            printStatsLine("c elim-bin-lt-cl"
                , binLearntClRemThroughElim);

            printStatsLine("c elim-long-lt-cl"
                , longLearntClRemThroughElim);

            printStatsLine("c lt-bin added due to v-elim"
                , numLearntBinVarRemAdded);

            printStatsLine("c cl-elim-bin"
                , clauses_elimed_bin);

            printStatsLine("c cl-elim-long"
                , clauses_elimed_long);

            printStatsLine("c cl-elim-avg-s",
                ((double)clauses_elimed_sumsize
                /(double)(clauses_elimed_bin + clauses_elimed_long))
            );
            cout << "c -------- SatELite STATS END ----------" << endl;
        }

        //Time stats
        double linkInTime;
        double blockTime;
        double asymmTime;
        double subsBinWithBinTime;
        double subsumeTime;
        double strengthenTime;
        double varElimTime;
        double finalCleanupTime;

        //Startup stats
        uint64_t origNumFreeVars;
        uint64_t origNumMaxElimVars;
        uint64_t origNumIrredLongClauses;
        uint64_t origNumRedLongClauses;

        //Each algorithm
        uint64_t subsBinWithBin;
        uint64_t blocked;
        uint64_t blockedSumLits;
        uint64_t asymmSubs;
        uint64_t clauses_subsumed;     ///<Number of clauses subsumed in this run
        uint64_t litsRemStrengthen;

        //Stats for var-elim
        uint64_t numVarsElimed;
        uint64_t clauses_elimed_long;
        uint64_t clauses_elimed_bin;
        uint64_t clauses_elimed_sumsize;
        uint64_t longLearntClRemThroughElim;
        uint64_t binLearntClRemThroughElim;
        uint64_t numLearntBinVarRemAdded;
        uint64_t triedToElimVars;
        uint64_t usedAgressiveCheckToELim;
        uint64_t newClauses;
        uint64_t newClauseNotAdded;
        uint64_t newClauseNotAddedFancy;

        //General stat
        uint64_t zeroDepthAssings;
    };

    const vector<char>& getVarElimed() const;
    uint32_t getNumERVars() const;
    const vector<BlockedClause>& getBlockedClauses() const;
    const GateFinder* getGateFinder() const;
    const Stats& getStats() const;
    const XorFinder* getXorFinder() const;
    void checkElimedUnassignedAndStats() const;

private:

    bool subsumeWithBinaries();

    //Indexes
    vector<Clause*>    clauses;  ///<ClauseSimp::index refers to the index of the clause here
    vector<AbstData>   clauseData;
    vector<Occur>      occur;            ///<occur[index(lit)]' is a list of constraints containing 'lit'.

    //Touched, elimed, etc.
    TouchList    touchedVars; ///<A list of the true elements in 'touched'.
    CSet         strengthenWith;  ///<Clauses strengthened/added
    CSet         subsumeWith;
    vector<char> ol_seenNeg;
    vector<char> ol_seenPos;
    vector<char> alreadyAdded;

    //Persistent data
    Solver*  solver;              ///<The solver this simplifier is connected to
    vector<char>    var_elimed;           ///<Contains TRUE if var has been eliminated

    //Temporaries
    vector<char>    seen;        ///<Used in various places to help perform algorithms
    vector<char>    seen2;       ///<Used in various places to help perform algorithms
    vector<Lit>     dummy;       ///<Used by merge()
    vector<Lit>     dummy2;      ///<Used by merge()
    vector<Lit>     finalLits;   ///<Used by addClauseInt()

    //Limits
    int64_t  addedClauseLits;
    int64_t  numMaxSubsume1;              ///<Max. number self-subsuming resolution tries to do this run
    int64_t  numMaxSubsume0;              ///<Max. number backward-subsumption tries to do this run
    int64_t  numMaxElim;                  ///<Max. number of variable elimination tries to do this run
    int64_t  numMaxElimVars;
    int64_t  numMaxAsymm;
    int64_t  numMaxBlocked;
    int64_t  numMaxVarElimAgressiveCheck;
    int64_t* toDecrease;
    uint32_t clTouchedTodo;
    void     printLimits();

    //Propagation&handling of stuff
    bool propagate();

    //Start-up
    uint64_t addFromSolver(vector<Clause*>& cs);
    void clearAll();
    void setLimits();
    void performSubsume0();
    bool performSubsume1();

    //Finish-up
    void freeMemory();
    void addBackToSolver();
    void removeWrongBins();
    void removeAllTris();
    void removeAssignedVarsFromEliminated();

    //Clause update
    void        strengthen(ClauseIndex& c, const Lit toRemoveLit);
    lbool       cleanClause(ClauseIndex c, Clause& cl);
    void        unlinkClause(ClauseIndex cc);
    ClauseIndex linkInClause(Clause& cl);
    bool        handleUpdatedClause(ClauseIndex& c, Clause& cl);

    //Findsubsumed
    template<class T> void findSubsumed0(const uint32_t index, const T& ps, const CL_ABST_TYPE abs, vector<ClauseIndex>& out_subsumed);
    template<class T> void findSubsumed1(const uint32_t index, const T& ps, const CL_ABST_TYPE abs, vector<ClauseIndex>& out_subsumed, vector<Lit>& out_lits);
    template<class T> void fillSubs(const T& ps, const uint32_t index, CL_ABST_TYPE abs, vector<ClauseIndex>& out_subsumed, vector<Lit>& out_lits, const Lit lit);
    template<class T1, class T2> bool subset(const T1& A, const T2& B);
    bool subsetReverse(const Clause& B) const;
    template<class T1, class T2> Lit subset1(const T1& A, const T2& B);
    bool subsetAbst(const CL_ABST_TYPE A, const CL_ABST_TYPE B);

    //binary clause-subsumption
    struct BinSorter {
        bool operator()(const Watched& first, const Watched& second)
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

    /**
    @brief Sort clauses according to size
    */
    struct sortBySize
    {
        bool operator () (const Clause* x, const Clause* y)
        {
            return (x->size() < y->size());
        }
    };

    /////////////////////
    //subsume0
    struct Sub0Ret {
        Sub0Ret() :
            subsumedNonLearnt(false)
            , numSubsumed(0)
        {};

        bool subsumedNonLearnt;
        ClauseStats stats;
        uint32_t numSubsumed;
    };
    uint32_t subsume0(ClauseIndex c, Clause& ps);
    template<class T> Sub0Ret subsume0(const uint32_t index, const T& ps, const CL_ABST_TYPE abs);

    /////////////////////
    //subsume1
    void subsume1(ClauseIndex c, Clause& ps);

    /////////////////////
    //Variable elimination

    vector<std::pair<int, int> > varElimComplexity;
    ///Order variables according to their complexity of elimination
    struct VarOrderLt {
        const vector<std::pair<int, int> >&  varElimComplexity;
        bool operator () (const uint32_t x, const uint32_t y) const
        {
            //Of the FIRST, the smallest is best
            if (varElimComplexity[x].first != varElimComplexity[y].first)
                return varElimComplexity[x].first < varElimComplexity[y].first;

            //Of the SECOND, the largest is best
            return varElimComplexity[x].second > varElimComplexity[y].second;
        }

        VarOrderLt(
            const vector<std::pair<int,int> >& _varElimComplexity
        ) :
            varElimComplexity(_varElimComplexity)
        {}
    };
    void        orderVarsForElimInit();
    Heap<VarOrderLt> varElimOrder;
    uint32_t    numNonLearntBins(const Lit lit) const;
    void        removeAfterVarelim(const vector<ClAndBin>& myset);
    void        addLearntBinaries(const Var var);
    void        removeClauses(const Var var);
    void        removeClausesHelper(vector<ClAndBin>& todo, const Lit lit);
    vector<ClAndBin> posAll, negAll;
    bool        maybeEliminate(const Var x);
    int         testVarElim(Var var);
    std::pair<int, int>  heuristicCalcVarElimScore(const Lit lit);
    bool        merge(
        const ClAndBin& ps
        , const ClAndBin& qs
        , const Lit without_p
        , const Lit without_q
        , const bool useCache
        , const bool final
    );
    void varElimCheckUpdate(
        const vector<ClAndBin>& gothrough
        , vector<Var>& varElimToCheck
        , vector<char>& varElimToCheckHelper
    );
    bool        eliminateVars();
    bool        loopSubsumeVarelim();
    void        fillClAndBin(vector<ClAndBin>& all, const Occur& cs, const Lit lit);
    void        removeBinsAndTris(const Var var);
    uint32_t    removeBinAndTrisHelper(const Lit lit, vec<Watched>& ws);

    /////////////////////
    //XOR finding
    friend class XorFinder;
    XorFinder *xorFinder;

    /////////////////////
    //Blocked clause elimination
    void asymmTE();
    void blockClauses();
    bool allTautologySlim(const Lit lit);
    vector<BlockedClause> blockedClauses;

    /////////////////////
    //Gate extraction
    friend class GateFinder;
    GateFinder *gateFinder;

    //validity checking
    bool verifyIntegrity();
    void checkForElimedVars();

    ///Number of times simplifyBySubsumption() has been called
    size_t numCalls;

    ///Stats from this run
    Stats runStats;

    ///Stats globally
    Stats globalStats;
};

/**
@brief Decides only using abstraction if clause A could subsume clause B

@note: It can give false positives. Never gives false negatives.

For A to subsume B, everything that is in A MUST be in B. So, if (A & ~B)
contains even one bit, it means that A contains something that B doesn't. So
A may be a subset of B only if (A & ~B) == 0
*/
inline bool Subsumer::subsetAbst(const CL_ABST_TYPE A, const CL_ABST_TYPE B)
{
    return ((A & ~B) == 0);
}

//A subsumes B (A <= B)
template<class T1, class T2>
bool Subsumer::subset(const T1& A, const T2& B)
{
    bool ret;
    uint16_t i = 0;
    uint16_t i2;
    Lit lastB = lit_Undef;
    for (i2 = 0; i2 != B.size(); i2++) {
        if (lastB != lit_Undef)
            assert(lastB < B[i2]);
        lastB = B[i2];
        //Literals are ordered
        if (A[i] < B[i2]) {
            ret = false;
            goto end;
        }
        else if (A[i] == B[i2]) {
            i++;

            //went through the whole of A now, so A subsumes B
            if (i == A.size()) {
                ret = true;
                goto end;
            }
        }
    }
    ret = false;

    end:
    *toDecrease -= i2*4 + i*4;
    return ret;
}

inline bool Subsumer::subsetReverse(const Clause& B) const
{
    for (uint32_t i = 0; i != B.size(); i++) {
        if (!seen[B[i].toInt()]) return false;
    }
    return true;
}

/**
@brief Decides if A subsumes B, or if not, if A could strenghten B

@note: Assumes 'seen' is cleared (will leave it cleared)

Helper function findSubsumed1. Does two things in one go:
1) decides if clause A could subsume clause B
2) decides if clause A could be used to perform self-subsuming resoltuion on
clause B

@return lit_Error, if neither (1) or (2) is true. Returns lit_Undef (1) is true,
and returns the literal to remove if (2) is true
*/
template<class T1, class T2>
Lit Subsumer::subset1(const T1& A, const T2& B)
{
    Lit retLit = lit_Undef;

    uint16_t i = 0;
    uint16_t i2;
    for (i2 = 0; i2 != B.size(); i2++) {
        if (A[i] == ~B[i2] && retLit == lit_Undef) {
            retLit = B[i2];
            i++;
            if (i == A.size())
                goto end;

            continue;
        }

        //Literals are ordered
        if (A[i] < B[i2]) {
            retLit = lit_Error;
            goto end;
        }

        if (A[i] == B[i2]) {
            i++;

            if (i == A.size())
                goto end;
        }
    }
    retLit = lit_Error;

    end:
    *toDecrease -= i2*4 + i*4;
    return retLit;
}

inline const vector<BlockedClause>& Subsumer::getBlockedClauses() const
{
    return blockedClauses;
}

inline const vector<char>& Subsumer::getVarElimed() const
{
    return var_elimed;
}

/**
@brief Finds clauses that are backward-subsumed by given clause

Only handles backward-subsumption. Uses occurrence lists

@param[in] ps The clause to backward-subsume with.
@param[in] abs Abstraction of the clause ps
@param[out] out_subsumed The set of clauses subsumed by this clause
*/
template<class T> void Subsumer::findSubsumed0(
    const uint32_t index //Will not match with index of the name value
    , const T& ps //Literals in clause
    , const CL_ABST_TYPE abs //Abstraction of literals in clause
    , vector<ClauseIndex>& out_subsumed //List of clause indexes subsumed
) {
    #ifdef VERBOSE_DEBUG
    cout << "findSubsumed: ";
    for (uint32_t i = 0; i < ps.size(); i++) {
        cout << ps[i] << " , ";
    }
    cout << endl;
    #endif

    //Which literal in the clause has the smallest occur list? -- that will be picked to go through
    uint32_t min_i = 0;
    for (uint32_t i = 1; i < ps.size(); i++){
        if (occur[ps[i].toInt()].size() < occur[ps[min_i].toInt()].size())
            min_i = i;
    }
    *toDecrease -= ps.size();

    //Go through the occur list of the literal that has the smallest occur list
    Occur& cs = occur[ps[min_i].toInt()];
    *toDecrease -= cs.size()*15 + 40;
    for (Occur::const_iterator it = cs.begin(), end = cs.end(); it != end; it++){
        //Check if this clause is subsumed by the clause given
        if (it->index != index
            && subsetAbst(abs, clauseData[it->index].abst)
            && ps.size() <= clauseData[it->index].size
        ) {
            *toDecrease -= 50;
            if (subset(ps, *clauses[it->index])) {
                out_subsumed.push_back(*it);
                #ifdef VERBOSE_DEBUG
                cout << "subsumed: " << *clauses[it->index] << endl;
                #endif
            }
        }
    }
}

inline const Subsumer::Stats& Subsumer::getStats() const
{
    return globalStats;
}

inline const XorFinder* Subsumer::getXorFinder() const
{
    return xorFinder;
}

#endif //SIMPLIFIER_H
