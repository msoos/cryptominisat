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

#ifndef CLAUSEVIVIFIER_H
#define CLAUSEVIVIFIER_H

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

class ClauseVivifier {
    public:
        ClauseVivifier(Solver* solver);
        bool vivify(bool alsoStrengthen);
        bool strengthenImplicit();

        struct Stats
        {
            Stats() :
                //Asymm
                timeNorm(0)
                , timeOut(0)
                , zeroDepthAssigns(0)
                , numClShorten(0)
                , numLitsRem(0)
                , checkedClauses(0)
                , potentialClauses(0)
                , numCalled(0)
            {}

            void clear()
            {
                Stats tmp;
                *this = tmp;
            }

            Stats& operator+=(const Stats& other)
            {
                timeNorm += other.timeNorm;
                timeOut += other.timeOut;
                zeroDepthAssigns += other.zeroDepthAssigns;
                numClShorten += other.numClShorten;
                numLitsRem += other.numLitsRem;
                checkedClauses += other.checkedClauses;
                potentialClauses += other.potentialClauses;
                numCalled += other.numCalled;

                //Cache-based
                irredCacheBased += other.irredCacheBased;
                redCacheBased += other.redCacheBased;

                return *this;
            }

            void printShort() const
            {
                //Irred cache-based asymm
                irredCacheBased.printShort("irred");

                //Irred cache-based asymm
                redCacheBased.printShort("red");

                //Norm Asymm
                cout
                << "c [vivif] asymm (tri+long)"
                << " useful: "<< numClShorten
                << "/" << checkedClauses << "/" << potentialClauses
                << " lits-rem:" << numLitsRem
                << " 0-depth-assigns:" << zeroDepthAssigns
                << " T: " << timeNorm << " s"
                << " time-out: " << (timeOut ? "Y" : "N")
                << endl;
            }

            void print(const size_t nVars) const
            {
                //Asymm
                cout << "c -------- ASYMM STATS --------" << endl;
                printStatsLine("c time"
                    , timeNorm
                    , timeNorm/(double)numCalled
                    , "per call"
                );

                printStatsLine("c timed out"
                    , timeOut
                    , (double)timeOut/(double)numCalled*100.0
                    , "% of calls"
                );

                printStatsLine("c asymm/checked/potential"
                    , numClShorten
                    , checkedClauses
                    , potentialClauses
                );

                printStatsLine("c lits-rem",
                    numLitsRem
                );
                printStatsLine("c 0-depth-assigns",
                    zeroDepthAssigns
                    , (double)zeroDepthAssigns/(double)nVars*100.0
                    , "% of vars"
                );

                cout << "c --> cache-based on irred cls" << endl;
                irredCacheBased.print();

                cout << "c --> cache-based on red cls" << endl;
                redCacheBased.print();
                cout << "c -------- ASYMM STATS END --------" << endl;
            }

            //Asymm
            double timeNorm;
            uint64_t timeOut;
            uint64_t zeroDepthAssigns;
            uint64_t numClShorten;
            uint64_t numLitsRem;
            uint64_t checkedClauses;
            uint64_t potentialClauses;
            uint64_t numCalled;

            struct CacheBased
            {
                double cpu_time;
                uint64_t numLitsRem;
                uint64_t numClSubsumed;
                uint64_t triedCls;
                uint64_t shrinked;
                uint64_t totalCls;
                uint64_t totalLits;
                uint64_t ranOutOfTime;
                uint64_t numCalled;

                CacheBased() :
                    cpu_time(0)
                    , numLitsRem(0)
                    , numClSubsumed(0)
                    , triedCls(0)
                    , shrinked(0)
                    , totalCls(0)
                    , totalLits(0)
                    , ranOutOfTime(0)
                    , numCalled(0)
                {}

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
                    << " time-out " << (ranOutOfTime ? "Y" : "N")
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
                        , (double)numLitsRem/(double)totalLits*100.0
                        , "% of lits tried"
                    );

                    printStatsLine("c called "
                        , numCalled
                        , (double)ranOutOfTime/(double)numCalled*100.0
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
                , const double myTime
                , const int64_t timeAvailable
            ) const;
        };
        StrImplicitData str_impl_data;
        // end

        ClOffset testVivify(
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
        void vivify_clause_with_lit(
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
        bool strenghten_clause_with_cache(const Lit lit);
        void try_subsuming_by_stamping(const bool red);
        void remove_lits_through_stamping_red();
        void remove_lits_through_stamping_irred();
        Stats::CacheBased tmpStats;
        bool needToFinish;
        bool vivify_clause(
            ClOffset& offset
            , bool red
            , const bool alsoStrengthen
        );
        void randomise_order_of_clauses(vector<ClOffset>& clauses);
        uint64_t calc_time_available(
            const bool alsoStrengthen
            , const bool red
        ) const;

        //Actual algorithms used
        bool asymmClausesLongIrred();
        bool vivifyClausesTriIrred();
        bool vivifyClausesCache(
            vector<ClOffset>& clauses
            , bool red
            , bool alsoStrengthen
        );
        int64_t timeAvailable;

        //Working set
        Solver* solver;

        //For vivify
        vector<Lit> lits;
        vector<Lit> lits2;
        vector<Lit> uselessLits;
        uint64_t extraTime;
        vector<uint16_t>& seen;
        vector<uint16_t>& seen_subs;

        //Global status
        Stats runStats;
        Stats globalStats;
        size_t numCalls;

};

inline const ClauseVivifier::Stats& ClauseVivifier::getStats() const
{
    return globalStats;
}

} //end namespace

#endif //CLAUSEVIVIFIER_H
