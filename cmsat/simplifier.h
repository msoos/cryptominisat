/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.0 of the License, or (at your option) any later version.
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

#include "clause.h"
#include "queue.h"
#include "bitarray.h"
#include "solvertypes.h"
#include "heap.h"
#include "touchlist.h"
#include "varupdatehelper.h"
#include "watched.h"
#include "watcharray.h"

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
class SubsumeStrengthen;

struct BlockedClause {
    BlockedClause()
    {}

    BlockedClause(Lit dummyLit) :
        blockedOn(dummyLit)
        , toRemove(false)
        , dummy(true)
    {}

    BlockedClause(
        const Lit _blockedOn
        , const vector<Lit>& _lits
        , const vector<uint32_t>& interToOuterMain
    ) :
        blockedOn( getUpdatedLit(_blockedOn, interToOuterMain))
        , toRemove(false)
        , lits(_lits)
        , dummy(false)
    {
        updateLitsMap(lits, interToOuterMain);
    }

    Lit blockedOn;
    bool toRemove;
    vector<Lit> lits;
    bool dummy;
};

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
    void subsumeReds();
    void newVar();
    void print_elimed_vars() const;
    void updateVars(
        const vector<uint32_t>& outerToInter
        , const vector<uint32_t>& interToOuter
    );
    bool unEliminate(const Var var);
    size_t memUsed() const;
    size_t memUsedXor() const;
    void printGateFinderStats() const;

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
            , varElimTime(0)
            , finalCleanupTime(0)

            //Startup stats
            , origNumFreeVars(0)
            , origNumMaxElimVars(0)
            , origNumIrredLongClauses(0)
            , origNumRedLongClauses(0)

            //Each algo
            , asymmSubs(0)
            , subsumedByVE(0)

            //Elimination
            , numVarsElimed(0)
            , varElimTimeOut(0)
            , clauses_elimed_long(0)
            , clauses_elimed_tri(0)
            , clauses_elimed_bin(0)
            , clauses_elimed_sumsize(0)
            , longRedClRemThroughElim(0)
            , triRedClRemThroughElim(0)
            , binRedClRemThroughElim(0)
            , numRedBinVarRemAdded(0)
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
            varElimTime += other.varElimTime;
            finalCleanupTime += other.finalCleanupTime;

            //Startup stats
            origNumFreeVars += other.origNumFreeVars;
            origNumMaxElimVars += other.origNumMaxElimVars;
            origNumIrredLongClauses += other.origNumIrredLongClauses;
            origNumRedLongClauses += other.origNumRedLongClauses;

            //Each algo
            asymmSubs += other.asymmSubs;
            subsumedByVE  += other.subsumedByVE;

            //Elim
            numVarsElimed += other.numVarsElimed;
            varElimTimeOut += other.varElimTimeOut;
            clauses_elimed_long += other.clauses_elimed_long;
            clauses_elimed_tri += other.clauses_elimed_tri;
            clauses_elimed_bin += other.clauses_elimed_bin;
            clauses_elimed_sumsize += other.clauses_elimed_sumsize;
            longRedClRemThroughElim += other.longRedClRemThroughElim;
            triRedClRemThroughElim += other.triRedClRemThroughElim;
            binRedClRemThroughElim += other.binRedClRemThroughElim;
            numRedBinVarRemAdded += other.numRedBinVarRemAdded;
            testedToElimVars += other.testedToElimVars;
            triedToElimVars += other.triedToElimVars;
            usedAgressiveCheckToELim += other.usedAgressiveCheckToELim;
            newClauses += other.newClauses;

            zeroDepthAssings += other.zeroDepthAssings;

            return *this;
        }

        void printShort(const bool print_var_elim = true) const
        {

            cout
            << "c [occur] " << linkInTime+finalCleanupTime << " is overhead"
            << endl;

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
                << " red-bin rem: " << binRedClRemThroughElim
                << " red-tri rem: " << triRedClRemThroughElim
                << " red-long rem: " << longRedClRemThroughElim
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
                , "% vars"
            );

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

            printStatsLine("c cl-new"
                , newClauses
            );

            printStatsLine("c tried to elim"
                , triedToElimVars
                , (double)usedAgressiveCheckToELim/(double)triedToElimVars*100.0
                , "% agressively"
            );

            printStatsLine("c asymmSub"
                , asymmSubs);

            printStatsLine("c elim-bin-lt-cl"
                , binRedClRemThroughElim);

            printStatsLine("c elim-tri-lt-cl"
                , triRedClRemThroughElim);

            printStatsLine("c elim-long-lt-cl"
                , longRedClRemThroughElim);

            printStatsLine("c lt-bin added due to v-elim"
                , numRedBinVarRemAdded);

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

            printStatsLine("c v-elim-sub"
                , subsumedByVE
            );

            cout << "c -------- Simplifier STATS END ----------" << endl;
        }

        uint64_t numCalls;

        //Time stats
        double linkInTime;
        double blockTime;
        double asymmTime;
        double varElimTime;
        double finalCleanupTime;

        //Startup stats
        uint64_t origNumFreeVars;
        uint64_t origNumMaxElimVars;
        uint64_t origNumIrredLongClauses;
        uint64_t origNumRedLongClauses;

        //Each algorithm
        uint64_t asymmSubs;
        uint64_t subsumedByVE;

        //Stats for var-elim
        int64_t numVarsElimed;
        uint64_t varElimTimeOut;
        uint64_t clauses_elimed_long;
        uint64_t clauses_elimed_tri;
        uint64_t clauses_elimed_bin;
        uint64_t clauses_elimed_sumsize;
        uint64_t longRedClRemThroughElim;
        uint64_t triRedClRemThroughElim;
        uint64_t binRedClRemThroughElim;
        uint64_t numRedBinVarRemAdded;
        uint64_t testedToElimVars;
        uint64_t triedToElimVars;
        uint64_t usedAgressiveCheckToELim;
        uint64_t newClauses;

        //General stat
        uint64_t zeroDepthAssings;
    };

    bool getVarElimed(const Var var) const;
    const vector<BlockedClause>& getBlockedClauses() const;
    //const GateFinder* getGateFinder() const;
    const Stats& getStats() const;
    const SubsumeStrengthen* getSubsumeStrengthen() const;
    void checkElimedUnassignedAndStats() const;
    void checkElimedUnassigned() const;
    bool getAnythingHasBeenBlocked() const;
    void freeXorMem();

private:

    friend class SubsumeStrengthen;
    SubsumeStrengthen* subsumeStrengthen;

    //debug
    bool subsetReverse(const Clause& B) const;
    void checkAllLinkedIn();

    void finishUp(size_t origTrailSize);
    vector<ClOffset> clauses;
    bool subsumeWithBinaries();

    //Persistent data
    Solver*  solver;              ///<The solver this simplifier is connected to
    vector<bool>    var_elimed;           ///<Contains TRUE if var has been eliminated
    vector<uint16_t>& seen;
    vector<uint16_t>& seen2;
    vector<Lit>& toClear;

    //Temporaries
    vector<Lit>     dummy;       ///<Used by merge()
    vector<Lit>     finalLits;   ///<Used by addClauseInt()

    //Limits
    uint64_t clause_lits_added_limit;
    int64_t  strengthening_time_limit;              ///<Max. number self-subsuming resolution tries to do this run
//     int64_t  numMaxTriSub;
    int64_t  subsumption_time_limit;              ///<Max. number backward-subsumption tries to do this run
    int64_t  varelim_time_limit;
    int64_t  varelim_num_limit;
    int64_t  asymm_time_limit;
    int64_t  aggressive_elim_time_limit;
    int64_t* limit_to_decrease;

    //Propagation&handling of stuff
    bool propagate();

    //Start-up
    bool addFromSolver(
        vector<ClOffset>& toAdd
        , bool alsoOccur
        , bool irred
    );
    struct LinkInData
    {
        LinkInData()
        {};

        LinkInData(uint64_t _cl_linked, uint64_t _cl_not_linked) :
            cl_linked(_cl_linked)
            , cl_not_linked(_cl_not_linked)
        {}

        uint64_t cl_linked = 0;
        uint64_t cl_not_linked = 0;
    };
    uint64_t calc_mem_usage_of_occur(const vector<ClOffset>& toAdd) const;
    void     print_mem_usage_of_occur(bool irred, uint64_t memUsage) const;
    void     print_linkin_data(const LinkInData link_in_data) const;
    bool     decide_occur_limit(bool irred, uint64_t memUsage);
    LinkInData link_in_clauses(
        const vector<ClOffset>& toAdd
        , bool irred
        , bool alsoOccur
    );
    void setLimits();

    //Finish-up
    void addBackToSolver();
    bool check_varelim_when_adding_back_cl(const Clause* cl) const;
    bool propImplicits();
    void removeAllLongsFromWatches();
    bool completeCleanClause(Clause& ps);

    //Clause update
    lbool       cleanClause(ClOffset c);
    void        unlinkClause(ClOffset cc, bool drup = true);
    void        linkInClause(Clause& cl);
    bool        handleUpdatedClause(ClOffset c);

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
    void        order_vars_for_elim();
    Heap<VarOrderLt> varElimOrder;
    uint32_t    numIrredBins(const Lit lit) const;
    //void        addRedBinaries(const Var var);
    size_t      rem_cls_from_watch_due_to_varelim(watch_subarray_const todo, const Lit lit);
    void        set_var_as_eliminated(const Var var, const Lit lit);
    bool        can_eliminate_var(const Var var) const;


    TouchList   touched;
    bool        maybeEliminate(const Var x);
    void        create_dummy_blocked_clause(const Lit lit);
    int         test_elim_and_fill_posall_negall(Var var);
    void        print_var_eliminate_stat(Lit lit) const;
    bool        add_varelim_resolvent(vector<Lit>& finalLits, const ClauseStats& stats);
    void        check_if_new_2_long_subsumes_3_long(const vector<Lit>& lits);
    void        update_varelim_complexity_heap(const Var var);
    void        print_var_elim_complexity_stats(const Var var) const;
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

        uint32_t totalCls() const
        {
            return bin + tri + longer;
        }

        uint32_t bin;
        uint32_t tri;
        uint32_t longer;
        uint32_t lit;
        uint32_t count;
    };
    HeuristicData calcDataForHeuristic(const Lit lit);
    std::pair<int, int> strategyCalcVarElimScore(const Var var);

    //For empty resolvents
    enum class ResolvCount{count, set, unset};
    bool checkEmptyResolvent(const Lit lit);
    int checkEmptyResolventHelper(
        const Lit lit
        , ResolvCount action
        , int otherSize
    );

    pair<int, int>  heuristicCalcVarElimScore(const Var var);
    bool resolve_clauses(
        const Watched ps
        , const Watched qs
        , const Lit noPosLit
        , const bool useCache
    );
    void add_pos_lits_to_dummy_and_seen(
        const Watched ps
        , const Lit posLit
    );
    bool add_neg_lits_to_dummy_and_seen(
        const Watched qs
        , const Lit posLit
    );
    bool reverse_vivification_of_dummy(
        const Watched ps
        , const Watched qs
        , const Lit posLit
    );
    bool subsume_dummy_through_stamping(
       const Watched ps
        , const Watched qs
    );
    bool agressiveCheck(
        const Lit lit
        , const Lit noPosLit
        , bool& retval
    );
    bool        eliminateVars();
    void        eliminate_empty_resolvent_vars();
    bool        loopSubsumeVarelim();

    /////////////////////
    //Helpers
    friend class XorFinder;
    friend class GateFinder;
    XorFinderAbst *xorFinder;
    GateFinder *gateFinder;

    /////////////////////
    //Blocked clause elimination
    void asymmTE();
    bool anythingHasBeenBlocked;
    vector<BlockedClause> blockedClauses;
    map<Var, vector<size_t> > blk_var_to_cl;
    bool blockedMapBuilt;
    void buildBlockedMap();
    void cleanBlockedClauses();

    //validity checking
    void sanityCheckElimedVars();
    void printOccur(const Lit lit) const;

    ///Stats from this run
    Stats runStats;

    ///Stats globally
    Stats globalStats;
};

inline const vector<BlockedClause>& Simplifier::getBlockedClauses() const
{
    return blockedClauses;
}

inline bool Simplifier::getVarElimed(const Var var) const
{
    return var_elimed[var];
}

inline const Simplifier::Stats& Simplifier::getStats() const
{
    return globalStats;
}

inline bool Simplifier::getAnythingHasBeenBlocked() const
{
    return anythingHasBeenBlocked;
}

inline std::ostream& operator<<(std::ostream& os, const BlockedClause& bl)
{
    os << bl.lits << " blocked on: " << bl.blockedOn;

    return os;
}

inline bool Simplifier::subsetReverse(const Clause& B) const
{
    for (uint32_t i = 0; i != B.size(); i++) {
        if (!seen[B[i].toInt()])
            return false;
    }
    return true;
}

inline const SubsumeStrengthen* Simplifier::getSubsumeStrengthen() const
{
    return subsumeStrengthen;
}

/*inline const XorFinder* Simplifier::getXorFinder() const
{
    return xorFinder;
}*/

} //end namespace

#endif //SIMPLIFIER_H
