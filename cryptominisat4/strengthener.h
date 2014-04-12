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

#ifndef __STRENGTHENER_H__
#define __STRENGTHENER_H__

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

class Strengthener {
    public:
        Strengthener(Solver* solver);
        bool strengthen(bool alsoStrengthen);
        bool strengthenImplicit();

        struct Stats
        {
            void clear()
            {
                Stats tmp;
                *this = tmp;
            }

            Stats& operator+=(const Stats& other);
            void printShort() const;
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

                void printShort(const string type) const
                {
                    cout << "c [vivif] cache-based "
                    << std::setw(5) << type
                    << "-- "
                    << " cl tried " << std::setw(8) << triedCls
                    << " cl-sh " << std::setw(5) << shrinked
                    << " cl-rem " << std::setw(4) << numClSubsumed
                    << " lit-rem " << std::setw(6) << numLitsRem
                    << " T-out " << (ranOutOfTime ? "Y" : "N")
                    << " T: " << std::setprecision(2) << cpu_time
                    << endl;
                }

                void print() const
                {
                    printStatsLine("c time"
                        , cpu_time
                        , cpu_time/(double)numCalled
                        , "s/call"
                    );

                    printStatsLine("c shrinked/tried/total"
                        , shrinked
                        , triedCls
                        , totalCls
                    );

                    printStatsLine("c subsumed/tried/total"
                        , numClSubsumed
                        , triedCls
                        , totalCls
                    );

                    printStatsLine("c lits-rem"
                        , numLitsRem
                        , stats_line_percent(numLitsRem, totalLits)
                        , "% of lits tried"
                    );

                    printStatsLine("c called "
                        , numCalled
                        , stats_line_percent(ranOutOfTime, numCalled)
                        , "% ran out of time"
                    );
                }

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

        const Stats& getStats() const;

    private:

        //Vars for strengthen implicit
        struct StrImplicitData
        {
            uint64_t remLitFromBin;
            uint64_t remLitFromTri;
            uint64_t remLitFromTriByBin;
            uint64_t remLitFromTriByTri;
            uint64_t stampRem;

            //For delayed enqueue and binary adding
            //Used for strengthening
            vector<Lit> toEnqueue;
            vector<BinaryClause> binsToAdd;

            uint64_t numWatchesLooked;

            void clear()
            {
                remLitFromBin = 0;
                remLitFromTri = 0;
                remLitFromTriByBin = 0;
                remLitFromTriByTri = 0;
                stampRem = 0;
                numWatchesLooked = 0;
                toEnqueue.clear();
                binsToAdd.clear();
            }

            void print(
                const size_t trail_diff
                , const double time_used
                , const int64_t timeAvailable
                , const int64_t orig_time
                , Solver* solver
            ) const;
        };
        StrImplicitData str_impl_data;
        // end

        ClOffset try_vivify_clause_and_return_new(
            ClOffset offset
            , const bool red
            , const uint32_t queueByBy
        );
        void strengthen_bin_with_bin(
            const Lit lit
            , Watched*& i
            , Watched*& j
            , const Watched* end
        );
        void strengthen_tri_with_bin_tri_stamp(
           const Lit lit
            , Watched*& i
            , Watched*& j
        );
        void strengthen_implicit_lit(const Lit lit);

        //Cache-based data
        struct CacheBasedData
        {
            size_t remLitTimeStampTotal;
            size_t remLitTimeStampTotalInv;
            size_t subsumedStamp;
            size_t remLitCache;
            size_t remLitBinTri;
            size_t subBinTri;
            size_t subCache;

            void clear()
            {
                remLitTimeStampTotal = 0;
                remLitTimeStampTotalInv = 0;
                subsumedStamp = 0;
                remLitCache = 0;
                remLitBinTri = 0;
                subBinTri = 0;
                subCache = 0;
            }

            size_t get_cl_subsumed() const
            {
                return subBinTri + subsumedStamp + subCache;
            }

            size_t get_lits_rem() const
            {
                return remLitBinTri + remLitCache
                    + remLitTimeStampTotal + remLitTimeStampTotalInv;
            }

            void print() const
            {
                cout
                << "c [cl-str] stamp-based"
                << " lit-rem: " << remLitTimeStampTotal
                << " inv-lit-rem: " << remLitTimeStampTotalInv
                << " stamp-cl-rem: " << subsumedStamp
                << endl;

                cout
                << "c [cl-str] bintri-based"
                << " lit-rem: " << remLitBinTri
                << " cl-sub: " << subBinTri
                << endl;

                cout
                << "c [cl-str] cache-based"
                << " lit-rem: " << remLitCache
                << " cl-sub: " << subCache
                << endl;
            }
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
        bool str_and_sub_clause_with_cache(const Lit lit);
        void try_subsuming_by_stamping(const bool red);
        void remove_lits_through_stamping_red();
        void remove_lits_through_stamping_irred();
        Stats::CacheBased tmpStats;
        //bool needToFinish;
        bool shorten_clause_with_cache_watch_stamp(
            ClOffset& offset
            , bool red
            , const bool alsoStrengthen
        );
        void randomise_order_of_clauses(vector<ClOffset>& clauses);
        uint64_t calc_time_available(
            const bool alsoStrengthen
            , const bool red
        ) const;

        bool shorten_all_clauses_with_cache_watch_stamp(
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

inline const Strengthener::Stats& Strengthener::getStats() const
{
    return globalStats;
}

} //end namespace

#endif //CLAUSEVIVIFIER_H
