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
    void subsume_implicit();

private:
    Solver* solver;
    int64_t timeAvailable;

    Lit lastLit2;
    Lit lastLit3;
    Watched* lastBin;
    bool lastRed;
    vector<Lit> tmplits;

    uint64_t remBins;
    uint64_t remTris;
    uint64_t stampTriRem;
    uint64_t cacheTriRem;

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

    void clear()
    {
        lastLit2 = lit_Undef;
        lastLit3 = lit_Undef;
        lastBin = NULL;
        lastRed = false;

        remBins = 0;
        remTris = 0;
        stampTriRem = 0;
        cacheTriRem = 0;
    }

    void print(const double total_time, const size_t numWatchesLooked, int64_t timeAvailable) const
    {
        cout
        << "c [implicit] sub"
        << " bin: " << remBins
        << " tri: " << remTris
        << " (stamp: " << stampTriRem << ", cache: " << cacheTriRem << ")"

        << " T: " << std::fixed << std::setprecision(2)
        << total_time
        << " T-out: " << (timeAvailable < 0 ? "Y" : "N")
        << " w-visit: " << numWatchesLooked
        << endl;
    }

    //ImplSubsumeData impl_subs_dat;
    void try_subsume_tri(
        const Lit lit
        , Watched*& i
        , Watched*& j
        , const bool doStamp
    );
    void try_subsume_bin(
       const Lit lit
        , Watched*& i
        , Watched*& j
    );
};

} //end namespace

#endif //SUBSUMEIMPLICIT_H