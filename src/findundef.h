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

#ifndef FINDUNDEF_H
#define FINDUNDEF_H

#include <vector>
#include "constants.h"

namespace CMSat
{

class Solver;
using std::vector;

class FindUndef {
    public:
        FindUndef(Solver* _solver);
        const uint32_t unRoll();

    private:
        Solver* solver;

        bool updateTables();
        void fill_potentials();
        void unset_potentials();

        //If set to TRUE, then that clause already has only 1 lit that is true,
        //so it can be skipped during updateFixNeed()
        vector<char> dontLookAtClause;

        vector<uint32_t> satisfies;
        vector<char> can_be_unset;
        uint32_t can_be_unsetSum = 0;
        bool all_sat = true;
        template<class C>
        bool look_at_one_clause(const C& c);

};

}

#endif //
