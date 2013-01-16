/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2011, Mate Soos and collaborators. All rights reserved.
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

#ifndef CLAUSEVIVIFIER_H
#define CLAUSEVIVIFIER_H

#include <vector>
#include "clause.h"
#include "constants.h"
#include "solvertypes.h"
#include "cloffset.h"
using std::vector;

class Solver;
class Clause;

class ClauseVivifier {
    public:
        ClauseVivifier(Solver* solver);
        bool vivify(bool alsoStrengthen);
        bool subsumeAndStrengthenImplicit();

        struct Stats
        {
            Stats() :
                //Asymm
                timeNorm(0)
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
                zeroDepthAssigns += other.zeroDepthAssigns;
                numClShorten += other.numClShorten;
                numLitsRem += other.numLitsRem;
                checkedClauses += other.checkedClauses;
                potentialClauses += other.potentialClauses;
                numCalled += other.numCalled;

                //Cache-based learnt
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
                << "c [vivif] asymm"
                << " cl-useful: "<< numClShorten
                << "/" << checkedClauses << "/" << potentialClauses

                << " lits-rem:" << numLitsRem
                << " 0-depth-assigns:" << zeroDepthAssigns
                << " T: " << timeNorm << " s"
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
            uint64_t zeroDepthAssigns;
            uint64_t numClShorten;
            uint64_t numLitsRem;
            uint64_t checkedClauses;
            uint64_t potentialClauses;
            uint64_t numCalled;

            //Cache learnt
            struct CacheBased
            {
                double cpu_time;
                uint64_t numLitsRem;
                uint64_t numClSubsumed;
                uint64_t tried;
                uint64_t shrinked;
                uint64_t totalCls;
                uint64_t ranOutOfTime;
                uint64_t numCalled;

                CacheBased() :
                    cpu_time(0)
                    , numLitsRem(0)
                    , numClSubsumed(0)
                    , tried(0)
                    , shrinked(0)
                    , totalCls(0)
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
                    << " cl tried " << std::setw(8) << tried
                    << " cl-sh " << std::setw(5) << shrinked
                    << " cl-rem " << std::setw(4) << numClSubsumed
                    << " lit-rem " << std::setw(6) << numLitsRem
                    << " time-out " << (ranOutOfTime ? "Y" : "N")
                    << " time: " << cpu_time
                    << endl;
                }

                void print() const
                {
                    printStatsLine("c time"
                        , cpu_time
                    );

                    printStatsLine("c shrinked/tried/total"
                        , shrinked
                        , tried
                        , totalCls
                    );

                    printStatsLine("c subsumed/tried/total"
                        , numClSubsumed
                        , tried
                        , totalCls
                    );

                    printStatsLine("c lits-rem"
                        , numLitsRem
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
                    tried += other.tried;
                    shrinked += other.shrinked;
                    totalCls += other.totalCls;
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

        ClOffset testVivify(
            ClOffset offset
            , Clause* cl
            , const bool learnt
            , const uint32_t queueByBy
        );

        //Actual algorithms used
        bool vivifyClausesLongIrred();
        bool vivifyClausesTriIrred();
        bool vivifyClausesCache(
            vector<ClOffset>& clauses
            , bool learnt
            , bool alsoStrengthen
        );
        std::pair<size_t, size_t> stampBasedLitRem(
            vector<Lit>& lits
            , const StampType stampType
        );
        vector<Lit> stampNorm;
        vector<Lit> stampInv;


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

                if (first.lit1() < second.lit1()) return true;
                if (first.lit1() > second.lit1()) return false;
                if (first.isBinary() && second.isTri()) return true;
                if (first.isTri() && second.isBinary()) return false;
                //At this point either both are BIN or both are TRI


                //Both are BIN
                if (first.isBinary()) {
                    assert(second.isBinary());
                    if (first.learnt() == second.learnt()) return false;
                    if (!first.learnt()) return true;
                    return false;
                }

                //Both are Tri
                assert(first.isTri() && second.isTri());
                if (first.lit2() < second.lit2()) return true;
                if (first.lit2() > second.lit2()) return false;
                if (first.learnt() == second.learnt()) return false;
                if (!first.learnt()) return true;
                return false;
            }
        };
        void removeTri(Lit lit1, Lit lit2, Lit lit3, bool learnt);

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

#endif //CLAUSEVIVIFIER_H
