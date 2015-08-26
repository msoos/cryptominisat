/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
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

#ifndef __SUBSUMESTRENGTHEN_H__
#define __SUBSUMESTRENGTHEN_H__

#include "cloffset.h"
#include "cryptominisat4/solvertypesmini.h"
#include "clabstraction.h"
#include "clause.h"
#include <vector>
using std::vector;

namespace CMSat {

class OccSimplifier;
class GateFinder;
class Solver;

class SubsumeStrengthen
{
public:
    SubsumeStrengthen(OccSimplifier* simplifier, Solver* solver);
    size_t mem_used() const;

    void backward_subsumption_long_with_long();
    bool backward_strengthen_long_with_long();

    //Called from simplifier at resolvent-adding of var-elim
    uint32_t subsume_and_unlink_and_markirred(const ClOffset offset);

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

    //Called from simplifier at resolvent-adding of var-elim
    template<class T>
    Sub0Ret subsume_and_unlink(
        const ClOffset offset
        , const T& ps
        , const cl_abst_type abs
        , const bool removeImplicit = false
    );

    struct Stats
    {
        Stats& operator+=(const Stats& other);
        void print_short(const Solver* solver) const;
        void print() const;

        uint64_t subsumedBySub = 0;
        uint64_t subsumedByStr = 0;
        uint64_t litsRemStrengthen = 0;

        double subsumeTime = 0.0;
        double strengthenTime = 0.0;
    };

    void finishedRun();
    const Stats& get_stats() const;
    const Stats& getRunStats() const;

    template<class T>
    void find_subsumed(
        const ClOffset offset
        , const T& ps
        , const cl_abst_type abs
        , vector<ClOffset>& out_subsumed
        , const bool removeImplicit = false
    );

private:
    Stats globalstats;
    Stats runStats;

    OccSimplifier* simplifier;
    Solver* solver;

    void remove_literal(ClOffset c, const Lit toRemoveLit);

    template<class T>
    size_t find_smallest_watchlist_for_clause(const T& ps) const;

    template<class T>
    void findStrengthened(
        const ClOffset offset
        , const T& ps
        , const cl_abst_type abs
        , vector<ClOffset>& out_subsumed
        , vector<Lit>& out_lits
    );

    template<class T>
    void fillSubs(
        const ClOffset offset
        , const T& ps
        , cl_abst_type abs
        , vector<ClOffset>& out_subsumed
        , vector<Lit>& out_lits
        , const Lit lit
    );

    template<class T1, class T2>
    bool subset(const T1& A, const T2& B);

    template<class T1, class T2>
    Lit subset1(const T1& A, const T2& B);
    bool subsetAbst(const cl_abst_type A, const cl_abst_type B);
    Sub1Ret strengthen_subsume_and_unlink_and_markirred(ClOffset offset);

    vector<ClOffset> subs;
    vector<Lit> subsLits;
};

inline const SubsumeStrengthen::Stats& SubsumeStrengthen::getRunStats() const
{
    return runStats;
}

inline const SubsumeStrengthen::Stats& SubsumeStrengthen::get_stats() const
{
    return globalstats;
}

} //end namespace

#endif //__SUBSUMESTRENGTHEN_H__
