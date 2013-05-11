/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos and collaborators. All rights reserved.
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

#ifndef __COMPLETE_DETACH_REATTACHER__
#define __COMPLETE_DETACH_REATTACHER__

#include "constants.h"
#include "vec.h"
#include "watched.h"

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
                    learntBins(0)
                    , nonLearntBins(0)
                    , learntTris(0)
                    , nonLearntTris(0)
                {}

                ClausesStay& operator+=(const ClausesStay& other) {
                    learntBins += other.learntBins;
                    nonLearntBins += other.nonLearntBins;
                    learntTris += other.learntTris;
                    nonLearntTris += other.nonLearntTris;
                    return *this;
                }

                uint64_t learntBins;
                uint64_t nonLearntBins;
                uint64_t learntTris;
                uint64_t nonLearntTris;
        };
        ClausesStay clearWatchNotBinNotTri(vec<Watched>& ws);

        Solver* solver;
};

} //end namespace

#endif //__COMPLETE_DETACH_REATTACHER__
