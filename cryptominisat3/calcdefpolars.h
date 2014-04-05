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

#ifndef CALCDEFAULTPOLARITIES__H
#define CALCDEFAULTPOLARITIES__H

#include "vec.h"
#include "clause.h"
#include "watchalgos.h"

namespace CMSat {
using namespace CMSat;

class Solver;

class CalcDefPolars
{
    public:
        CalcDefPolars(Solver* solver);
        const vector<unsigned char> calculate();

    private:
        void tally_clause_votes(const vector<ClOffset>& cs);
        void tally_implicit_votes(const watch_array& watched);
        void add_vote(const Lit lit, const double value);

        vector<double> votes;

        Solver* solver;
};

} //end namespace

#endif //CALCDEFAULTPOLARITIES__H
