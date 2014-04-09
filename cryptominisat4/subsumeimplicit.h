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
        void printShort() const;
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
    Stats getStats() const;

private:
    Solver* solver;
    int64_t timeAvailable;

    Lit lastLit2;
    Lit lastLit3;
    Watched* lastBin;
    bool lastRed;
    vector<Lit> tmplits;
    Stats runStats;
    Stats globalStats;

    struct WatchSorter {
        bool operator()(const Watched& a, const Watched& b)
        {
            //Anything but clause!
            if (a.isClause()) {
                //A is definitely not better than B
                return false;
            }
            if (b.isClause()) {
                //B is clause, A is NOT a clause. So A is better than B.
                return true;
            }
            //Now nothing is clause

            if (a.lit2() != b.lit2()) {
                return a.lit2() < b.lit2();
            }
            if (a.isBinary() && b.isTri()) return true;
            if (a.isTri() && b.isBinary()) return false;
            //At this point either both are BIN or both are TRI

            //Both are BIN
            if (a.isBinary()) {
                assert(b.isBinary());
                if (a.red() != b.red()) {
                    return !a.red();
                }
                return false;
            }

            //Both are Tri
            assert(a.isTri() && b.isTri());
            if (a.lit3() != b.lit3()) {
                return a.lit3() < b.lit3();
            }
            if (a.red() != b.red()) {
                return !a.red();
            }
            return false;
        }
    };

    void clear()
    {
        lastLit2 = lit_Undef;
        lastLit3 = lit_Undef;
        lastBin = NULL;
        lastRed = false;
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