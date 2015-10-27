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

#ifndef __DISTILLERWITHBIN_H__
#define __DISTILLERWITHBIN_H__

#include <vector>
#include "clause.h"
#include "constants.h"
#include "solvertypes.h"
#include "cloffset.h"
#include "watcharray.h"

namespace CMSat {

using std::vector;

class Solver;
class Clause;

class DistillWithImplicit {
    public:
        DistillWithImplicit(Solver* solver);
        bool distill_long_with_implicit(bool alsoStrengthen);

        struct Stats
        {
            void clear()
            {
                Stats tmp;
                *this = tmp;
            }

            Stats& operator+=(const Stats& other);
            void print_short(const Solver* solver) const;
            void print() const;

            struct CacheBased
            {
                double cpu_time = 0.0;
                uint64_t numLitsRem = 0;
                uint64_t numClSubsumed = 0;
                uint64_t triedCls = 0;
                uint64_t shrinked = 0;
                uint64_t totalCls = 0;
                uint64_t totalLits = 0;
                uint64_t ranOutOfTime = 0;
                uint64_t numCalled = 0;

                void clear()
                {
                    CacheBased tmp;
                    *this = tmp;
                }

                void print_short(const string type, const Solver* solver) const;
                void print() const;

                CacheBased& operator+=(const CacheBased& other)
                {
                    cpu_time += other.cpu_time;
                    numLitsRem += other.numLitsRem;
                    numClSubsumed += other.numClSubsumed;
                    triedCls += other.triedCls;
                    shrinked += other.shrinked;
                    totalCls += other.totalCls;
                    totalLits += other.totalLits;
                    ranOutOfTime += other.ranOutOfTime;
                    numCalled += other.numCalled;

                    return  *this;
                }
            };

            CacheBased irredCacheBased;
            CacheBased redCacheBased;
        };

        const Stats& get_stats() const;

    private:

        bool remove_or_shrink_clause(Clause& cl, ClOffset& offset);
        void strsub_with_cache_and_watch(
            bool alsoStrengthen
            , Clause& cl
        );
        void dump_stats_for_shorten_all_cl_with_cache_stamp(
            bool red
            , bool alsoStrengthen
            , double myTime
            , double orig_time_available
        );

        //Cache-based data
        struct CacheBasedData
        {
            size_t remLitTimeStampTotal = 0;
            size_t remLitTimeStampTotalInv = 0;
            size_t subsumedStamp = 0;
            size_t remLitCache = 0;
            size_t remLitBinTri = 0;
            size_t subBinTri = 0;
            size_t subCache = 0;
            void clear();
            size_t get_cl_subsumed() const;
            size_t get_lits_rem() const;
            void print() const;
        };
        CacheBasedData cache_based_data;
        bool isSubsumed;
        size_t thisRemLitCache;
        size_t thisRemLitBinTri;
        void str_and_sub_using_watch(
            Clause& cl
            , const Lit lit
            , const bool alsoStrengthen
        );
        void strengthen_clause_with_watch(
            const Lit lit
            , const Watched* wit
        );
        bool subsume_clause_with_watch(
           const Lit lit
            , Watched* wit
            , const Clause& cl
        );
        bool str_and_sub_clause_with_cache(const Lit lit, const bool alsoStrengthen);
        void try_subsuming_by_stamping(const bool red);
        void remove_lits_through_stamping_red();
        void remove_lits_through_stamping_irred();
        Stats::CacheBased tmpStats;
        //bool needToFinish;
        bool sub_str_cl_with_cache_watch_stamp(
            ClOffset& offset
            , bool red
            , const bool alsoStrengthen
        );
        void randomise_order_of_clauses(vector<ClOffset>& clauses);
        uint64_t calc_time_available(
            const bool alsoStrengthen
            , const bool red
        ) const;

        bool shorten_all_cl_with_cache_watch_stamp(
            vector<ClOffset>& clauses
            , bool red
            , bool alsoStrengthen
        );
        int64_t timeAvailable;

        //Working set
        Solver* solver;
        vector<Lit> lits;
        vector<Lit> lits2;
        vector<uint16_t>& seen;
        vector<uint16_t>& seen_subs;

        //Global status
        Stats runStats;
        Stats globalStats;
        size_t numCalls = 0;

};

inline const DistillWithImplicit::Stats& DistillWithImplicit::get_stats() const
{
    return globalStats;
}

} //end namespace

#endif //__DISTILLERWITHBIN_H__
