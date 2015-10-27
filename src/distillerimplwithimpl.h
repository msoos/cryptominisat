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

#ifndef __DISTILLER_IMPL_WITH_IMP__
#define __DISTILLER_IMPL_WITH_IMP__

#include <vector>
#include "clause.h"
#include "constants.h"
#include "solvertypes.h"
#include "cloffset.h"
#include "watcharray.h"
using std::vector;

namespace CMSat {

class Solver;
class Clause;

class DistillerImplWithImpl {
public:
    DistillerImplWithImpl(Solver* _solver) :
        solver(_solver)
    {}

    bool distill_implicit_with_implicit();
private:
    Solver* solver;
    void distill_implicit_with_implicit_lit(const Lit lit);

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

    //Vars for strengthen implicit
    struct StrImplicitData
    {
        uint64_t remLitFromBin = 0;
        uint64_t remLitFromTri = 0;
        uint64_t remLitFromTriByBin = 0;
        uint64_t remLitFromTriByTri = 0;
        uint64_t stampRem = 0;

        uint64_t numWatchesLooked = 0;

        //For delayed enqueue and binary adding
        //Used for strengthening
        vector<Lit> toEnqueue;
        vector<BinaryClause> binsToAdd;

        void clear()
        {
            StrImplicitData tmp;
            *this = tmp;
        }

        void print(
            const size_t trail_diff
            , const double time_used
            , const int64_t timeAvailable
            , const int64_t orig_time
            , Solver* solver
        ) const;
    };
    StrImplicitData str_impl_data;
    int64_t timeAvailable;
    vector<Lit> lits;
};

}

#endif // __DISTILLER_IMPL_WITH_IMP__