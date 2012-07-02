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

#include "constants.h"
#include "Vec.h"
#include "Watched.h"

class Solver;
class Clause;

/**
@brief Helper class to completely detaches all(or only non-native) clauses

Used in classes that (may) do a lot of clause-changning, in which case
detaching&reattaching of clauses would be neccessary to do
individually, which is \b very slow

A main use-case is the following:
-# detach all clauses
-# play around with all clauses as desired. Cannot call solver.propagate() here
-# attach again
*/
class CompleteDetachReatacher
{
    public:
        CompleteDetachReatacher(Solver* solver);
        bool reattachNonBins();
        void detachNonBinsNonTris(const bool removeTri);

    private:
        void cleanAndAttachClauses(vector<Clause*>& cs);
        bool cleanClause(Clause*& cl);

        class ClausesStay {
            public:
                ClausesStay() :
                    learntBins(0)
                    , nonLearntBins(0)
                    , tris(0)
                {}

                ClausesStay& operator+=(const ClausesStay& other) {
                    learntBins += other.learntBins;
                    nonLearntBins += other.nonLearntBins;
                    tris += other.tris;
                    return *this;
                }

                uint32_t learntBins;
                uint32_t nonLearntBins;
                uint32_t tris;
        };
        ClausesStay clearWatchNotBinNotTri(vec<Watched>& ws, const bool removeTri = false);

        Solver* solver;
};
