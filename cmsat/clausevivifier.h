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
        void subsumeImplicit();
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

            vector<Lit> toEnqueue;

            void clear()
            {
                remLitFromBin = 0;
                remLitFromTri = 0;
                remLitFromTriByBin = 0;
                remLitFromTriByTri = 0;
                stampRem = 0;
                toEnqueue.clear();
            }
        };
        StrImplicitData str_impl_data;
        // end

        ClOffset testVivify(
            ClOffset offset
            , const bool red
            , const uint32_t queueByBy
        );
        void strengthen_bin_with_bin(const Lit lit, Watched*& i, Watched*& j, const Watched* end);

        //Actual algorithms used
        bool asymmClausesLongIrred();
        bool vivifyClausesTriIrred();
        bool vivifyClausesCache(
            vector<ClOffset>& clauses
            , bool red
            , bool alsoStrengthen
        );
        int64_t timeAvailable;


        //Subsumtion of bin with bin
        struct WatchSorter {
            bool operator()(const Watched& first, const Watched& second)
            {
                //Anything but clause!
                if (first.isClause())
                    return false;
                if (second.isClause())
                    return true;
                //Now nothing is clause

                if (first.lit2() < second.lit2()) return true;
                if (first.lit2() > second.lit2()) return false;
                if (first.isBinary() && second.isTri()) return true;
                if (first.isTri() && second.isBinary()) return false;
                //At this point either both are BIN or both are TRI


                //Both are BIN
                if (first.isBinary()) {
                    assert(second.isBinary());
                    if (first.red() == second.red()) return false;
                    if (!first.red()) return true;
                    return false;
                }

                //Both are Tri
                assert(first.isTri() && second.isTri());
                if (first.lit3() < second.lit3()) return true;
                if (first.lit3() > second.lit3()) return false;
                if (first.red() == second.red()) return false;
                if (!first.red()) return true;
                return false;
            }
        };
        void removeTri(Lit lit1, Lit lit2, Lit lit3, bool red);

        //Working set
        Solver* solver;

        //For vivify
        vector<Lit> lits;
        vector<Lit> uselessLits;
        uint64_t extraTime;

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
