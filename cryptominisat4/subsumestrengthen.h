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

#ifndef __SUBSUMESTRENGTHEN_H__
#define __SUBSUMESTRENGTHEN_H__

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
        , const cl_abst_type abs
        , const bool removeImplicit = false
    );

    struct Stats
    {
        Stats& operator+=(const Stats& other);
        void printShort() const;
        void print() const;

        uint64_t subsumedBySub = 0;
        uint64_t subsumedByStr = 0;
        uint64_t litsRemStrengthen = 0;

        double subsumeTime = 0.0;
        double strengthenTime = 0.0;
    };

    void finishedRun();
    const Stats& getStats() const;
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

    Simplifier* simplifier;
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

inline const SubsumeStrengthen::Stats& SubsumeStrengthen::getStats() const
{
    return globalstats;
}

} //end namespace

#endif //__SUBSUMESTRENGTHEN_H__
