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

#include "solver.h"
#include "cloffset.h"
#include <vector>
using std::vector;

namespace CMSat {

class Simplifier;
class GateFinder;

class SubsumeStrengthen
{
public:
    SubsumeStrengthen(Simplifier* simplifier, Solver* solver);
    size_t memUsed() const;
    void backward_subsumption_with_all_clauses();
    bool performStrengthening();
    uint32_t subsume_and_unlink_and_markirred(ClOffset offset);
    //bool subsumeWithTris();

    struct Sub0Ret {
        bool subsumedIrred = 0;
        ClauseStats stats;
        uint32_t numSubsumed = 0;
    };

    struct Sub1Ret {
        Sub1Ret& operator+=(const Sub1Ret& other)
        {
            sub += other.sub;
            str += other.str;

            return *this;
        }

        size_t sub = 0;
        size_t str = 0;
    };

    template<class T>
    Sub0Ret subsume_and_unlink(
        const ClOffset offset
        , const T& ps
        , const CL_ABST_TYPE abs
        , const bool removeImplicit = false
    );

    struct Stats
    {
        Stats& operator+=(const Stats& other)
        {
            subsumedBySub += other.subsumedBySub;
            subsumedByStr += other.subsumedByStr;
            litsRemStrengthen += other.litsRemStrengthen;

            subsumeTime += other.subsumeTime;
            strengthenTime += other.strengthenTime;

            return *this;
        }

        void printShort() const
        {
            //STRENGTH + SUBSUME
            cout << "c [subs] long"
            << " subBySub: " << subsumedBySub
            << " subByStr: " << subsumedByStr
            << " lits-rem-str: " << litsRemStrengthen
            << " T: " << std::fixed << std::setprecision(2)
            << (subsumeTime+strengthenTime)
            << " s"
            << endl;
        }

        void print() const
        {
            cout << "c -------- SubsumeStrengthen STATS ----------" << endl;
            printStatsLine("c cl-subs"
                , subsumedBySub + subsumedByStr
                , " Clauses"
            );
            printStatsLine("c cl-str rem lit"
                , litsRemStrengthen
                , " Lits"
            );
            printStatsLine("c cl-sub T"
                , subsumeTime
                , " s"
            );
            printStatsLine("c cl-str T"
                , strengthenTime
                , " s"
            );
            cout << "c -------- SubsumeStrengthen STATS END ----------" << endl;
        }

        uint64_t subsumedBySub = 0;
        uint64_t subsumedByStr = 0;
        uint64_t litsRemStrengthen = 0;

        double subsumeTime = 0.0;
        double strengthenTime = 0.0;
    };

    void finishedRun();
    const Stats& getStats() const;
    const Stats& getRunStats() const;

private:
    Stats globalstats;
    Stats runStats;

    Simplifier* simplifier;
    Solver* solver;

    void remove_literal(ClOffset c, const Lit toRemoveLit);
    friend class GateFinder;

    template<class T>
    void findSubsumed0(
        const ClOffset offset
        , const T& ps
        , const CL_ABST_TYPE abs
        , vector<ClOffset>& out_subsumed
        , const bool removeImplicit = false
    );
    template<class T>
    size_t find_smallest_watchlist_for_clause(const T& ps) const;

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

    template<class T1, class T2>
    Lit subset1(const T1& A, const T2& B);
    bool subsetAbst(const CL_ABST_TYPE A, const CL_ABST_TYPE B);
    Sub1Ret strengthen_subsume_and_unlink_and_markirred(ClOffset offset);

    vector<ClOffset> subs;
    vector<Lit> subsLits;
};

inline const SubsumeStrengthen::Stats& SubsumeStrengthen::getRunStats() const
{
    return runStats;
}

inline const SubsumeStrengthen::Stats& SubsumeStrengthen::getStats() const
{
    return globalstats;
}

} //end namespace
