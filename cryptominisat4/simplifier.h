/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.0 of the License.
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
    ) :
        blockedOn(_blockedOn)
        , toRemove(false)
        , lits(_lits)
        , dummy(false)
    {
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
    void new_var(const Var orig_outer);
    void new_vars(const size_t n);
    void saveVarMem();
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
            , varElimTime(0)
            , finalCleanupTime(0)

            //Startup stats
            , origNumFreeVars(0)
            , origNumMaxElimVars(0)
            , origNumIrredLongClauses(0)
            , origNumRedLongClauses(0)

            //Each algo
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
            , usedAggressiveCheckToELim(0)
            , newClauses(0)

            , zeroDepthAssings(0)
        {
        }

        double totalTime() const
        {
            return linkInTime + blockTime
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
            varElimTime += other.varElimTime;
            finalCleanupTime += other.finalCleanupTime;

            //Startup stats
            origNumFreeVars += other.origNumFreeVars;
            origNumMaxElimVars += other.origNumMaxElimVars;
            origNumIrredLongClauses += other.origNumIrredLongClauses;
            origNumRedLongClauses += other.origNumRedLongClauses;

            //Each algo
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
            usedAggressiveCheckToELim += other.usedAggressiveCheckToELim;
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
                << " T-out: " << (varElimTimeOut ? "Y" : "N")
                << endl;

                cout
                << "c [v-elim]"
                << " cl-new: " << newClauses
                << " tried: " << triedToElimVars
                << " tested: " << testedToElimVars
                << " ("
                << stats_line_percent(usedAggressiveCheckToELim, testedToElimVars)
                << " % aggressive)"
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
                , stats_line_percent(varElimTime, totalTime())
                , "% var-elim"
            );

            printStatsLine("c timeouted"
                , stats_line_percent(varElimTimeOut, numCalls)
                , "% called"
            );

            printStatsLine("c called"
                ,  numCalls
                , (double)totalTime()/(double)numCalls
                , "s per call"
            );

            printStatsLine("c v-elimed"
                , numVarsElimed
                , stats_line_percent(numVarsElimed, nVars)
                , "% vars"
            );

            cout << "c"
            << " v-elimed: " << numVarsElimed
            << " / " << origNumMaxElimVars
            << " / " << origNumFreeVars
            << endl;

            printStatsLine("c 0-depth assigns"
                , zeroDepthAssings
                , stats_line_percent(zeroDepthAssings, nVars)
                , "% vars"
            );

            printStatsLine("c cl-new"
                , newClauses
            );

            printStatsLine("c tried to elim"
                , triedToElimVars
                , stats_line_percent(usedAggressiveCheckToELim, triedToElimVars)
                , "% aggressively"
            );

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
        double varElimTime;
        double finalCleanupTime;

        //Startup stats
        uint64_t origNumFreeVars;
        uint64_t origNumMaxElimVars;
        uint64_t origNumIrredLongClauses;
        uint64_t origNumRedLongClauses;

        //Each algorithm
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
        uint64_t usedAggressiveCheckToELim;
        uint64_t newClauses;

        //General stat
        uint64_t zeroDepthAssings;
    };

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

    void check_delete_gatefinder();
    bool fill_occur();
    bool fill_occur_and_print_stats();
    void finishUp(size_t origTrailSize);
    vector<ClOffset> clauses;
    bool subsumeWithBinaries();

    //Persistent data
    Solver*  solver;              ///<The solver this simplifier is connected to
    vector<uint16_t>& seen;
    vector<uint16_t>& seen2;
    vector<Lit>& toClear;

    //Temporaries
    vector<Lit>     dummy;       ///<Used by merge()
    vector<Lit>     gate_lits_of_elim_cls;

    //Limits
    uint64_t clause_lits_added;
    int64_t  strengthening_time_limit;              ///<Max. number self-subsuming resolution tries to do this run
//     int64_t  numMaxTriSub;
    int64_t  subsumption_time_limit;              ///<Max. number backward-subsumption tries to do this run
    int64_t  norm_varelim_time_limit;
    int64_t  empty_varelim_time_limit;
    int64_t  varelim_num_limit;
    int64_t  aggressive_elim_time_limit;
    int64_t bounded_var_elim_time_limit;
    int64_t* limit_to_decrease;

    //Start-up
    bool addFromSolver(
        vector<ClOffset>& toAdd
        , bool alsoOccur
        , bool irred
    );
    struct LinkInData
    {
        LinkInData()
        {}

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
    void remove_by_drup_recently_blocked_clauses(size_t origBlockedSize);
    void addBackToSolver();
    bool check_varelim_when_adding_back_cl(const Clause* cl) const;
    void removeAllLongsFromWatches();
    bool completeCleanClause(Clause& ps);

    //Clause update
    lbool       cleanClause(ClOffset c);
    void        unlinkClause(ClOffset cc, bool drup = true, bool allow_empty_watch = false, bool only_set_is_removed = false);
    void        linkInClause(Clause& cl);
    bool        handleUpdatedClause(ClOffset c);

    struct watch_sort_smallest_first {
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
    void        add_clause_to_blck(Lit lit, const vector<Lit>& lits);
    void        set_var_as_eliminated(const Var var, const Lit lit);
    bool        can_eliminate_var(const Var var) const;


    TouchList   touched;
    vector<ClOffset> cl_to_free_later;
    bool        maybeEliminate(const Var x);
    void        free_clauses_to_free();
    void        clean_occur_from_removed_clauses();
    void        try_to_subsume_with_new_bin_or_tri(const vector<Lit>& lits);
    void        create_dummy_blocked_clause(const Lit lit);
    int         test_elim_and_fill_resolvents(Var var);
    void        mark_gate_in_poss_negs(Lit elim_lit, watch_subarray_const poss, watch_subarray_const negs);
    void        mark_gate_parts(
        Lit elim_lit
        , watch_subarray_const a
        , watch_subarray_const b
        , vector<char>& a_mark
        , vector<char>& b_mark
    );
    bool        find_gate(Lit elim_lit, watch_subarray_const a, watch_subarray_const b);
    bool        skip_resolution_thanks_to_gate(const size_t at_poss, const size_t at_negs) const;
    void        print_var_eliminate_stat(Lit lit) const;
    bool        add_varelim_resolvent(vector<Lit>& finalLits, const ClauseStats& stats);
    bool        check_if_new_2_long_subsumes_3_long_return_already_inside(const vector<Lit>& lits);
    void        update_varelim_complexity_heap(const Var var);
    void        print_var_elim_complexity_stats(const Var var) const;
    struct Resolvent {
        Resolvent(const vector<Lit>& _lits, const ClauseStats _stats) :
            lits(_lits)
            , stats(_stats)
        {}
        vector<Lit> lits;
        ClauseStats stats;
        bool operator<(const Resolvent& other) const
        {
            return lits.size() > other.lits.size();
        }
    };
    vector<Resolvent> resolvents;
    vector<char> poss_gate_parts;
    vector<char> negs_gate_parts;
    bool gate_found_elim;
    bool gate_found_elim_pos;

    struct HeuristicData
    {
        HeuristicData() :
            bin(0)
            , tri(0)
            , longer(0)
            , lit(0)
            , count(std::numeric_limits<uint32_t>::max()) //resolution count (if can be counted, otherwise MAX)
        {}

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
    HeuristicData calc_data_for_heuristic(const Lit lit);
    std::pair<int, int> strategyCalcVarElimScore(const Var var);
    uint64_t time_spent_on_calc_otf_update;
    uint64_t num_otf_update_until_now;

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
    bool aggressiveCheck(
        const Lit lit
        , const Lit noPosLit
        , bool& retval
    );
    bool        eliminateVars();
    void        eliminate_empty_resolvent_vars();
    bool        loopSubsumeVarelim();

    /////////////
    //Bounded Variable Addition
    bool bva_verbosity = 0;
    size_t bva_worked;
    size_t bva_simp_size;
    struct lit_pair{
        lit_pair(Lit a, Lit b = lit_Undef)
        {
            if (b == lit_Undef) {
                lit1 = a;
            } else {
                assert(false && "lits are supposed to be sorted in occur lists");
                assert(a != b);
                if (a > b) {
                    std::swap(a, b);
                }
                lit1 = a;
                lit2 = b;
            }
        }

        bool operator==(const lit_pair& other) const
        {
            return lit1 == other.lit1 && lit2 == other.lit2;
        }

        unsigned hash(const uint32_t N) const
        {
            unsigned long h;
            h = lit1.toInt();

            if (lit2 == lit_Undef)
                return h % N;

            h = h*31 + lit2.toInt();
            return h % N;
        }

        bool operator!=(const lit_pair& other) const
        {
            return !(*this == other);
        }

        Lit lit1;
        Lit lit2;
    };
    struct PotentialClause {
        PotentialClause(const lit_pair _lits, const OccurClause cl) :
            lits(_lits)
            , occur_cl(cl)
        {}

        bool operator<(const PotentialClause& other) const
        {
            if (lits == other.lits)
                return false;

            if (lits.lit1 != other.lits.lit1)
                return lits.lit1 < other.lits.lit1;

            return lits.lit2 < other.lits.lit2;
        }

        lit_pair lits;
        OccurClause occur_cl;
        string to_string(const Solver* solver) const;
    };
    struct m_cls_lits_and_red
    {
        //Used during removal to lower overhead
        m_cls_lits_and_red(const vector<Lit>& _lits, bool _red) :
            lits(_lits)
            , red(_red)
        {}
        vector<Lit> lits;
        bool red;
    };
    bool bounded_var_addition();
    size_t calc_watch_irred_size(const Lit lit) const;
    void calc_watch_irred_sizes();
    lit_pair most_occuring_lit_in_potential(size_t& num_occur);
    lit_pair lit_diff_watches(const OccurClause& a, const OccurClause& b);
    Lit least_occurring_except(const OccurClause& c);
    bool inside(const vector<Lit>& lits, const Lit notin) const;
    bool simplifies_system(const size_t num_occur) const;
    int simplification_size(
        const int m_lit_size
        , const int m_cls_size
    ) const;
    void fill_potential(const Lit lit);
    bool try_bva_on_lit(const Lit lit);
    bool bva_simplify_system();
    void update_touched_lits_in_bva();
    bool add_longer_clause(const Lit lit, const OccurClause& cl);
    void remove_duplicates_from_m_cls();
    void remove_matching_clause(
        const m_cls_lits_and_red& cl_lits
        , const lit_pair lit_replace
    );
    Clause* find_cl_for_bva(
        const vector<Lit>& torem
        , const bool red
    ) const;
    void fill_m_cls_lits_and_red();
    vector<Lit> bva_tmp_lits; //To reduce overhead
    vector<m_cls_lits_and_red> m_cls_lits; //used during removal to lower overhead
    vector<Lit> to_remove; //to reduce overhead
    vector<PotentialClause> potential;
    vector<lit_pair> m_lits;
    vector<lit_pair> m_lits_this_cl;
    vector<OccurClause> m_cls;
    vector<size_t> watch_irred_sizes;
    struct VarBVAOrder
    {
        VarBVAOrder(vector<size_t>& _watch_irred_sizes) :
            watch_irred_sizes(_watch_irred_sizes)
        {}

        bool operator()(const uint32_t lit1_uint, const uint32_t lit2_uint) const;
        vector<size_t>& watch_irred_sizes;
    };
    Heap<VarBVAOrder> var_bva_order;

    /////////////////////
    //Helpers
    friend class XorFinder;
    friend class GateFinder;
    XorFinderAbst *xorFinder;
    GateFinder *gateFinder;

    /////////////////////
    //Blocked clause elimination
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
