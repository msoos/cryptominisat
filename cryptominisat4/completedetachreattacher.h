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

#ifndef __COMPLETE_DETACH_REATTACHER__
#define __COMPLETE_DETACH_REATTACHER__

#include "constants.h"
#include "vec.h"
#include "watched.h"
#include "watcharray.h"

namespace CMSat {

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
        bool reattachLongs(bool removeStatsFrist = false);
        void detachNonBinsNonTris();

    private:
        void cleanAndAttachClauses(
            vector<ClOffset>& cs
            , bool removeStatsFrist
        );
        bool cleanClause(Clause* cl);

        class ClausesStay {
            public:
                ClausesStay() :
                    redBins(0)
                    , irredBins(0)
                    , redTris(0)
                    , irredTris(0)
                {}

                ClausesStay& operator+=(const ClausesStay& other) {
                    redBins += other.redBins;
                    irredBins += other.irredBins;
                    redTris += other.redTris;
                    irredTris += other.irredTris;
                    return *this;
                }

                uint64_t redBins;
                uint64_t irredBins;
                uint64_t redTris;
                uint64_t irredTris;
        };
        ClausesStay clearWatchNotBinNotTri(watch_subarray ws);

        Solver* solver;
};

} //end namespace

#endif //__COMPLETE_DETACH_REATTACHER__
