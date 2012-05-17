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
#include "constants.h"
#include "SolverTypes.h"
using std::vector;

class Solver;
class Clause;

class ClauseVivifier {
    public:
        ClauseVivifier(Solver* solver);
        bool vivify();

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
                    << " cl-sh " << std::setw(7) << shrinked
                    << " cl-rem " << std::setw(7) << numClSubsumed
                    << " lit-rem " << std::setw(7) << numLitsRem
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

        //Actual algorithms used
        bool vivifyClausesNormal();
        bool vivifyClausesCache(vector<Clause*>& clauses, bool learnt);
        void       makeNonLearntBin(const Lit lit1, const Lit lit2);

        ///Sort clauses according to size
        struct SortBySize
        {
            bool operator () (const Clause* x, const Clause* y);
        };

        //Working set
        Solver* solver;

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
