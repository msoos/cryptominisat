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

#ifndef CLAUSECLEANER_H
#define CLAUSECLEANER_H

#include "constants.h"
#include "simplifier.h"

namespace CMSat {

/**
@brief Cleans clauses from false literals & removes satisfied clauses
*/
class ClauseCleaner
{
    public:
        ClauseCleaner(Solver* solver);

        void cleanClauses(vector<ClOffset>& cs);


        void clean_implicit_clauses();
        void removeAndCleanAll();
        bool satisfied(const Clause& c) const;

    private:
        //Implicit cleaning
        struct ImplicitData
        {
            uint64_t remNonLBin = 0;
            uint64_t remLBin = 0;
            uint64_t remNonLTri = 0;
            uint64_t remLTri = 0;

            //We can only attach these in delayed mode, otherwise we would
            //need to manipulate the watchlist we are going through
            vector<BinaryClause> toAttach;

            void update_solver_stats(Solver* solver);
        };
        ImplicitData impl_data;
        void clean_implicit_watchlist(
            watch_subarray& watch_list
            , const Lit lit
        );
        void clean_binary_implicit(
           Watched& ws
            , watch_subarray::iterator& j
            , const Lit lit
        );
        void clean_tertiary_implicit(
           Watched& ws
            , watch_subarray::iterator& j
            , const Lit lit
        );

        bool satisfied(const Watched& watched, Lit lit);
        bool cleanClause(ClOffset c);

        Solver* solver;
};

} //end namespace

#endif //CLAUSECLEANER_H
