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

#ifndef SUBSUMEIMPLICIT_H
#define SUBSUMEIMPLICIT_H

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

class SubsumeImplicit
{
public:
    SubsumeImplicit(Solver* solver);
    void subsume_implicit(bool check_stats = true);

    struct Stats {
        void clear()
        {
            *this = Stats();
        }
        Stats operator+=(const Stats& other);
        void print_short(const Solver* solver) const;
        void print() const;

        double time_used = 0.0;
        uint64_t numCalled = 0;
        uint64_t time_out = 0;
        uint64_t remBins = 0;
        uint64_t remTris = 0;
        uint64_t stampTriRem = 0;
        uint64_t cacheTriRem = 0;
        uint64_t numWatchesLooked = 0;
    };
    Stats get_stats() const;

private:
    Solver* solver;
    int64_t timeAvailable;

    Lit lastLit2;
    Watched* lastBin;
    bool lastRed;
    vector<Lit> tmplits;
    Stats runStats;
    Stats globalStats;

    void clear()
    {
        lastLit2 = lit_Undef;
        lastBin = NULL;
        lastRed = false;
    }

    //ImplSubsumeData impl_subs_dat;
    void try_subsume_bin(
       const Lit lit
        , Watched* i
        , Watched*& j
    );
};

} //end namespace

#endif //SUBSUMEIMPLICIT_H
