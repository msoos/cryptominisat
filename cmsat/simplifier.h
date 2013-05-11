/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos and collaborators. All rights reserved.
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


#include <map>
#include <vector>
#include <list>
#include <set>
#include <queue>
#include <set>
#include <iomanip>
#include <fstream>

#include "solver.h"
#include "clause.h"
#include "queue.h"
#include "bitarray.h"
#include "solvertypes.h"
#include "heap.h"
#include "touchlist.h"

namespace CMSat {

using std::vector;
using std::list;
using std::map;
using std::pair;
using std::priority_queue;

class ClauseCleaner;
class SolutionExtender;
class Solver;
class GateFinder;
class XorFinderAbst;

/**
@brief Handles subsumption, self-subsuming resolution, variable elimination, and related algorithms
*/
class Simplifier
{
public:

    //Construct-destruct
    Simplifier(Solver* solver);
    ~Simplifier();

    //Called from main
    bool simplify();
    void subsumeLearnts();
    void newVar();
    void print_elimed_vars() const;
    void updateVars(
        const vector<uint32_t>& outerToInter
        , const vector<uint32_t>& interToOuter
    );
    bool unEliminate(const Var var);
    uint64_t memUsed() const;
    uint64_t memUsedXor() const;

    //UnElimination
    void print_blocked_clauses_reverse() const;
    void extendModel(SolutionExtender* extender);

    //Get-functions
    struct Stats
    {
        Stats() :
            numCalls(0)
            //Time
            , linkInTime(0)
            , blockTime(0)
            , asymmTime(0)
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
            , blocked(0)
            , blockedSumLits(0)
            , asymmSubs(0)
            , subsumedBySub(0)
            , subsumedByStr(0)
            , subsumedByVE(0)
            , litsRemStrengthen(0)

            //Elimination
            , numVarsElimed(0)
            , varElimTimeOut(0)
            , clauses_elimed_long(0)
            , clauses_elimed_tri(0)
            , clauses_elimed_bin(0)
            , clauses_elimed_sumsize(0)
            , longLearntClRemThroughElim(0)
            , triLearntClRemThroughElim(0)
            , binLearntClRemThroughElim(0)
            , numLearntBinVarRemAdded(0)
            , testedToElimVars(0)
            , triedToElimVars(0)
            , usedAgressiveCheckToELim(0)
            , newClauses(0)

            , zeroDepthAssings(0)
        {
        }

        double totalTime() const
        {
            return linkInTime + blockTime + asymmTime
                + subsumeTime + strengthenTime
                + varElimTime + finalCleanupTime;
        }

        void clear()
        {
            Stats stats;
            *this = stats;
        }

        Stats& operator+=(const Stats& other)
        {
            numCalls += other.numCalls;

            //Time
            linkInTime += other.linkInTime;
            blockTime += other.blockTime;
            asymmTime += other.asymmTime;
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
            blocked += other.blocked;
            blockedSumLits += other.blockedSumLits;
            asymmSubs += other.asymmSubs;
            subsumedBySub += other.subsumedBySub;
            subsumedByStr += other.subsumedByStr;
            subsumedByVE  += other.subsumedByVE;
            litsRemStrengthen += other.litsRemStrengthen;

            //Elim
            numVarsElimed += other.numVarsElimed;
            varElimTimeOut += other.varElimTimeOut;
            clauses_elimed_long += other.clauses_elimed_long;
            clauses_elimed_tri += other.clauses_elimed_tri;
            clauses_elimed_bin += other.clauses_elimed_bin;
            clauses_elimed_sumsize += other.clauses_elimed_sumsize;
            longLearntClRemThroughElim += other.longLearntClRemThroughElim;
            triLearntClRemThroughElim += other.triLearntClRemThroughElim;
            binLearntClRemThroughElim += other.binLearntClRemThroughElim;
            numLearntBinVarRemAdded += other.numLearntBinVarRemAdded;
            testedToElimVars += other.testedToElimVars;
            triedToElimVars += other.triedToElimVars;
            usedAgressiveCheckToELim += other.usedAgressiveCheckToELim;
            newClauses += other.newClauses;

            zeroDepthAssings += other.zeroDepthAssings;

            return *this;
        }

        void printShortSubStr() const
        {
            //STRENGTH + SUBSUME
            cout << "c [subs] long"
            << " subBySub: " << subsumedBySub
            << " subByStr: " << subsumedByStr
            << " lits-rem-str: " << litsRemStrengthen
            << " T: " << std::fixed << std::setprecision(2)
            << (subsumeTime+strengthenTime+linkInTime+finalCleanupTime)
            << "(" << linkInTime+finalCleanupTime << " is overhead)"
            << " s"
            << endl;
        }

        void printShort(const bool print_var_elim = true) const
        {
            printShortSubStr();

            //About elimination
            if (print_var_elim) {
                cout
                << "c [v-elim]"
                << " elimed: " << numVarsElimed
                << " / " << origNumMaxElimVars
                << " / " << origNumFreeVars
                //<< " cl-elim: " << (clauses_elimed_long+clauses_elimed_bin)
                << " T: " << std::fixed << std::setprecision(2)
                << varElimTime << " s"
                << " T-out: " << varElimTimeOut
                << endl;

                cout
                << "c [v-elim]"
                << " cl-new: " << newClauses
                << " tried: " << triedToElimVars
                << " tested: " << testedToElimVars
                << " ("
                << (double)usedAgressiveCheckToELim/(double)testedToElimVars*100.0
                << " % agressive)"
                << endl;

                cout
                << "c [v-elim]"
                << " subs: "  << subsumedByVE
                << " learnt-bin rem: " << binLearntClRemThroughElim
                << " learnt-tri rem: " << triLearntClRemThroughElim
                << " learnt-long rem: " << longLearntClRemThroughElim
                << " v-fix: " << std::setw(4) << zeroDepthAssings
                << endl;
            }

            cout
            << "c [simp] link-in T: " << linkInTime
            << " cleanup T: " << finalCleanupTime
            << endl;
        }

        void print(const size_t nVars) const
        {
            cout << "c -------- Simplifier STATS ----------" << endl;
            printStatsLine("c time"
                , totalTime()
                , varElimTime/totalTime()*100.0
                , "% var-elim"
            );

            printStatsLine("c timeouted"
                , (double)varElimTimeOut/(double)numCalls*100.0
                , "% called"
            );

            printStatsLine("c called"
                ,  numCalls
                , (double)totalTime()/(double)numCalls
                , "s per call"
            );

            printStatsLine("c v-elimed"
                , numVarsElimed
                , (double)numVarsElimed/(double)nVars*100.0
                , "% vars");

            cout << "c"
            << " v-elimed: " << numVarsElimed
            << " / " << origNumMaxElimVars
            << " / " << origNumFreeVars
            << endl;

            printStatsLine("c 0-depth assigns"
                , zeroDepthAssings
                , (double)zeroDepthAssings/(double)nVars*100.0
                , "% vars"
            );

            printStatsLine("c lit-rem-str"
                , litsRemStrengthen
            );

            printStatsLine("c cl-new"
                , newClauses
            );

            printStatsLine("c tried to elim"
                , triedToElimVars
                , (double)usedAgressiveCheckToELim/(double)triedToElimVars*100.0
                , "% agressively"
            );

            printStatsLine("c cl-subs"
                , subsumedBySub + subsumedByStr + subsumedByVE
                , (double)(subsumedBySub + subsumedByStr + subsumedByVE)
                /(double)(origNumIrredLongClauses+origNumRedLongClauses)
                , "% clauses"
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

            printStatsLine("c elim-tri-lt-cl"
                , triLearntClRemThroughElim);

            printStatsLine("c elim-long-lt-cl"
                , longLearntClRemThroughElim);

            printStatsLine("c lt-bin added due to v-elim"
                , numLearntBinVarRemAdded);

            printStatsLine("c cl-elim-bin"
                , clauses_elimed_bin);

            printStatsLine("c cl-elim-tri"
                , clauses_elimed_tri);

            printStatsLine("c cl-elim-long"
                , clauses_elimed_long);

            printStatsLine("c cl-elim-avg-s",
                ((double)clauses_elimed_sumsize
                /(double)(clauses_elimed_bin + clauses_elimed_tri + clauses_elimed_long))
            );
            cout << "c -------- Simplifier STATS END ----------" << endl;
        }

        uint64_t numCalls;

        //Time stats
        double linkInTime;
        double blockTime;
        double asymmTime;
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
        uint64_t blocked;
        uint64_t blockedSumLits;
        uint64_t asymmSubs;
        uint64_t subsumedBySub;
        uint64_t subsumedByStr;
        uint64_t subsumedByVE;
        uint64_t litsRemStrengthen;

        //Stats for var-elim
        int64_t numVarsElimed;
        uint64_t varElimTimeOut;
        uint64_t clauses_elimed_long;
        uint64_t clauses_elimed_tri;
        uint64_t clauses_elimed_bin;
        uint64_t clauses_elimed_sumsize;
        uint64_t longLearntClRemThroughElim;
        uint64_t triLearntClRemThroughElim;
        uint64_t binLearntClRemThroughElim;
        uint64_t numLearntBinVarRemAdded;
        uint64_t testedToElimVars;
        uint64_t triedToElimVars;
        uint64_t usedAgressiveCheckToELim;
        uint64_t newClauses;

        //General stat
        uint64_t zeroDepthAssings;
    };

    const vector<bool>& getVarElimed() const;
    uint32_t getNumERVars() const;
    const vector<BlockedClause>& getBlockedClauses() const;
    const GateFinder* getGateFinder() const;
    const Stats& getStats() const;
    void checkElimedUnassignedAndStats() const;
    void checkElimedUnassigned() const;
    bool getAnythingHasBeenBlocked() const;
    void freeXorMem();

private:

    //debug
    void checkAllLinkedIn();

    void finishUp(size_t origTrailSize);
    vector<ClOffset> clauses;
    bool subsumeWithBinaries();

    //Persistent data
    Solver*  solver;              ///<The solver this simplifier is connected to
    vector<bool>    var_elimed;           ///<Contains TRUE if var has been eliminated

    //Temporaries
    vector<unsigned char>    seen;        ///<Used in various places to help perform algorithms
    vector<unsigned char>    seen2;       ///<Used in various places to help perform algorithms
    vector<Lit>     dummy;       ///<Used by merge()
    vector<Lit>     toClear;      ///<Used by merge()
    vector<Lit>     finalLits;   ///<Used by addClauseInt()
    vector<ClOffset> subs;
    vector<Lit> subsLits;

    //Limits
    uint64_t addedClauseLits;
    int64_t  numMaxSubsume1;              ///<Max. number self-subsuming resolution tries to do this run
//     int64_t  numMaxTriSub;
    int64_t  numMaxSubsume0;              ///<Max. number backward-subsumption tries to do this run
    int64_t  numMaxElim;                  ///<Max. number of variable elimination tries to do this run
    int64_t  numMaxElimVars;
    int64_t  numMaxAsymm;
    int64_t  numMaxBlocked;
    int64_t  numMaxBlockedImpl;
    int64_t  numMaxVarElimAgressiveCheck;
    int64_t* toDecrease;

    //Propagation&handling of stuff
    bool propagate();

    //Start-up
    bool addFromSolver(
        vector<ClOffset>& toAdd
        , bool alsoOccur
        , bool irred
        , uint64_t& numLitsAdded
    );
    void setLimits();
    void performSubsumption();
    bool performStrengthening();

    //Finish-up
    void addBackToSolver();
    bool propImplicits();
    void removeAllLongsFromWatches();
    bool completeCleanClause(Clause& ps);

    //Clause update
    void        strengthen(ClOffset c, const Lit toRemoveLit);
    lbool       cleanClause(ClOffset c);
    void        unlinkClause(ClOffset cc, bool drup = true);
    void        linkInClause(Clause& cl);
    bool        handleUpdatedClause(ClOffset c);

    //Findsubsumed
    template<class T>
    void findSubsumed0(
        const ClOffset offset
        , const T& ps
        , const CL_ABST_TYPE abs
        , vector<ClOffset>& out_subsumed
        , const bool removeImplicit = false
    );

    template<class T>
    void findStrengthened(
        const ClOffset offset
        , const T& ps
        , const CL_ABST_TYPE abs
        , vector<ClOffset>& out_subsumed
        , vector<Lit>& out_lits
    );

    template<class T>
    void fillSubs(
        const ClOffset offset
        , const T& ps
        , CL_ABST_TYPE abs
        , vector<ClOffset>& out_subsumed
        , vector<Lit>& out_lits
        , const Lit lit
    );

    template<class T1, class T2>
    bool subset(const T1& A, const T2& B);
    bool subsetReverse(const Clause& B) const;

    template<class T1, class T2>
    Lit subset1(const T1& A, const T2& B);
    bool subsetAbst(const CL_ABST_TYPE A, const CL_ABST_TYPE B);

    struct WatchSorter {
        bool operator()(const Watched& first, const Watched& second)
        {
            //Anything but clause!
            if (first.isClause())
                return false;
            if (second.isClause())
                return true;

            //BIN is better than TRI
            if (first.isBinary() && second.isTri()) return true;

            return false;
        }
    };

    /**
    @brief Sort clauses according to size
    */
    struct MySorter
    {
        MySorter(const ClauseAllocator* _clAllocator) :
            clAllocator(_clAllocator)
        {}

        bool operator () (const ClOffset x, const ClOffset y)
        {
            Clause* cl1 = clAllocator->getPointer(x);
            Clause* cl2 = clAllocator->getPointer(y);
            return (cl1->size() < cl2->size());
        }

        const ClauseAllocator* clAllocator;
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
    uint32_t subsume0(ClOffset offset);
//     bool subsumeWithTris();

    template<class T>
    Sub0Ret subsume0AndUnlink(
        const ClOffset offset
        , const T& ps
        , const CL_ABST_TYPE abs
        , const bool removeImplicit = false
    );

    /////////////////////
    //subsume1
    struct Sub1Ret {
        Sub1Ret() :
            sub(0)
            , str(0)
        {};

        Sub1Ret& operator+=(const Sub1Ret& other)
        {
            sub += other.sub;
            str += other.str;

            return *this;
        }

        size_t sub;
        size_t str;
    };
    Sub1Ret subsume1(ClOffset offset);

    /////////////////////
    //Variable elimination

    vector<pair<int, int> > varElimComplexity;
    ///Order variables according to their complexity of elimination
    struct VarOrderLt {
        const vector<pair<int, int> >&  varElimComplexity;
        bool operator () (const size_t x, const size_t y) const
        {
            //Smallest cost first
            if (varElimComplexity[x].first != varElimComplexity[y].first)
                return varElimComplexity[x].first < varElimComplexity[y].first;

            //Smallest cost first
            return varElimComplexity[x].second < varElimComplexity[y].second;
        }

        VarOrderLt(
            const vector<pair<int,int> >& _varElimComplexity
        ) :
            varElimComplexity(_varElimComplexity)
        {}
    };
    void        orderVarsForElimInit();
    Heap<VarOrderLt> varElimOrder;
    uint32_t    numNonLearntBins(const Lit lit) const;
    //void        addLearntBinaries(const Var var);
    void        removeClausesHelper(const vec<Watched>& todo, const Lit lit);


    TouchList   touched;
    bool        maybeEliminate(const Var x);
    int         testVarElim(Var var);
    vector<pair<vector<Lit>, ClauseStats> > resolvents;

    struct HeuristicData
    {
        HeuristicData() :
            bin(0)
            , tri(0)
            , longer(0)
            , lit(0)
            , count(std::numeric_limits<uint32_t>::max()) //resolution count (if can be counted, otherwise MAX)
        {};

        size_t totalCls() const
        {
            return bin + tri + longer;
        }

        uint32_t bin;
        uint32_t tri;
        uint32_t longer;
        uint32_t lit;
        uint32_t count;
    };
    HeuristicData calcDataForHeuristic(
        const Lit lit
        , bool setit = false
        , bool countIt = false
        , unsigned otherSize = 0
        , bool unset = false
    );
    std::pair<int, int> strategyCalcVarElimScore(const Var var);

    pair<int, int>  heuristicCalcVarElimScore(const Var var);
    bool merge(
        const Watched& ps
        , const Watched& qs
        , const Lit noPosLit
        , const bool useCache
    );
    bool agressiveCheck(
        const Lit lit
        , const Lit noPosLit
        , bool& retval
    );
    bool        eliminateVars();
    bool        loopSubsumeVarelim();

    /////////////////////
    //XOR finding
    friend class XorFinder;
    XorFinderAbst *xorFinder;

    /////////////////////
    //Blocked clause elimination
    void asymmTE();
    bool anythingHasBeenBlocked;
    void blockClauses();
    void blockImplicit(bool bins = false, bool tris = true);
    bool allTautologySlim(const Lit lit);
    vector<BlockedClause> blockedClauses;
    map<Var, vector<size_t> > blk_var_to_cl;
    bool blockedMapBuilt;
    void buildBlockedMap();
    void cleanBlockedClauses();

    /////////////////////
    //Gate extraction
    friend class GateFinder;
    GateFinder *gateFinder;

    //validity checking
    void checkForElimedVars();
    void printOccur(const Lit lit) const;

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
inline bool Simplifier::subsetAbst(const CL_ABST_TYPE A, const CL_ABST_TYPE B)
{
    return ((A & ~B) == 0);
}

//A subsumes B (A <= B)
template<class T1, class T2>
bool Simplifier::subset(const T1& A, const T2& B)
{
    #ifdef MORE_DEUBUG
    cout << "A:" << A << endl;
    for(size_t i = 1; i < A.size(); i++) {
        assert(A[i-1] < A[i]);
    }

    cout << "B:" << B << endl;
    for(size_t i = 1; i < B.size(); i++) {
        assert(B[i-1] < B[i]);
    }
    #endif

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

inline bool Simplifier::subsetReverse(const Clause& B) const
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
Lit Simplifier::subset1(const T1& A, const T2& B)
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

inline const vector<BlockedClause>& Simplifier::getBlockedClauses() const
{
    return blockedClauses;
}

inline const vector<bool>& Simplifier::getVarElimed() const
{
    return var_elimed;
}

inline const Simplifier::Stats& Simplifier::getStats() const
{
    return globalStats;
}

/**
@brief Finds clauses that are backward-subsumed by given clause

Only handles backward-subsumption. Uses occurrence lists

@param[in] ps The clause to backward-subsume with.
@param[in] abs Abstraction of the clause ps
@param[out] out_subsumed The set of clauses subsumed by this clause
*/
template<class T> void Simplifier::findSubsumed0(
    const ClOffset offset //Will not match with index of the name value
    , const T& ps //Literals in clause
    , const CL_ABST_TYPE abs //Abstraction of literals in clause
    , vector<ClOffset>& out_subsumed //List of clause indexes subsumed
    , bool removeImplicit
) {
    #ifdef VERBOSE_DEBUG
    cout << "findSubsumed0: ";
    for (uint32_t i = 0; i < ps.size(); i++) {
        cout << ps[i] << " , ";
    }
    cout << endl;
    #endif

    //Which literal in the clause has the smallest occur list? -- that will be picked to go through
    size_t min_i = 0;
    for (uint32_t i = 1; i < ps.size(); i++){
        if (solver->watches[ps[i].toInt()].size() < solver->watches[ps[min_i].toInt()].size())
            min_i = i;
    }
    *toDecrease -= ps.size();

    //Go through the occur list of the literal that has the smallest occur list
    vec<Watched>& occ = solver->watches[ps[min_i].toInt()];
    *toDecrease -= occ.size()*8 + 40;

    vec<Watched>::iterator it = occ.begin();
    vec<Watched>::iterator it2 = occ.begin();
    size_t numBinFound = 0;
    for (vec<Watched>::const_iterator
        end = occ.end()
        ; it != end
        ; it++
    ) {
        if (removeImplicit) {
            if (it->isBinary()
                && ps.size() == 2
                && ps[!min_i] == it->lit1()
                && !it->learnt()
            ) {
                /*cout
                << "ps " << ps << " could subsume this bin: "
                << ps[min_i] << ", " << it->lit1()
                << endl;*/
                numBinFound++;

                //We cannot remove ourselves
                if (numBinFound > 1) {
                    removeWBin(solver->watches, it->lit1(), ps[min_i], it->learnt());
                    solver->binTri.irredBins--;
                    solver->binTri.irredLits-=2;
                    continue;
                }
            }

            if (it->isTri()
                && ps.size() == 2
                && (ps[!min_i] == it->lit1() || ps[!min_i] == it->lit2())
            ) {
                /*cout
                << "ps " << ps << " could subsume this tri: "
                << ps[min_i] << ", " << it->lit1() << ", " << it->lit2()
                << endl;
                */
                Lit lits[3];
                lits[0] = ps[min_i];
                lits[1] = it->lit1();
                lits[2] = it->lit2();
                std::sort(lits + 0, lits + 3);
                removeTriAllButOne(solver->watches, ps[min_i], lits, it->learnt());
                if (it->learnt()) {
                    solver->binTri.redTris--;
                    solver->binTri.redLits-=3;
                } else {
                    solver->binTri.irredTris--;
                    solver->binTri.irredLits-=3;
                }
                continue;
            }
        }
        *it2++ = *it;

        if (!it->isClause()) {
            continue;
        }

        *toDecrease -= 15;

        if (it->getOffset() == offset
            || !subsetAbst(abs, it->getAbst())
        ) {
            continue;
        }

        ClOffset offset2 = it->getOffset();
        const Clause& cl2 = *solver->clAllocator->getPointer(offset2);

        if (ps.size() > cl2.size())
            continue;

        *toDecrease -= 50;
        if (subset(ps, cl2)) {
            out_subsumed.push_back(it->getOffset());
            #ifdef VERBOSE_DEBUG
            cout << "subsumed cl offset: " << it->getOffset() << endl;
            #endif
        }
    }
    occ.shrink(it-it2);
}

inline bool Simplifier::getAnythingHasBeenBlocked() const
{
    return anythingHasBeenBlocked;
}

/*inline const XorFinder* Simplifier::getXorFinder() const
{
    return xorFinder;
}*/

} //end namespace

#endif //SIMPLIFIER_H
