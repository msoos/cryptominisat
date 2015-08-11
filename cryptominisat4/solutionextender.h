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

#ifndef __SOLUTIONEXTENDER_H__
#define __SOLUTIONEXTENDER_H__

#include "solvertypes.h"
#include "clause.h"
#include "watcharray.h"

namespace CMSat {

#ifdef VERBOSE_DEBUG
#define VERBOSE_DEBUG_RECONSTRUCT
#endif

class Solver;
class OccSimplifier;

class SolutionExtender
{
    public:
        SolutionExtender(Solver* _solver, OccSimplifier* simplifier);
        void extend();
        void addClause(const vector<Lit>& lits, const Lit blockedOn);
        void dummyBlocked(const Lit blockedOn);

    private:
        Solver* solver;
        OccSimplifier* simplifier;

        bool satisfied(const vector<Lit>& lits) const;
        bool contains_lit(
            const vector<Lit>& lits
            , const Lit tocontain
        ) const;
};

} //end namespace

#endif //__SOLUTIONEXTENDER_H__
