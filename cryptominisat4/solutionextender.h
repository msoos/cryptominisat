/******************************************************************************
CryptoMiniSat -- Copyright (c) 2011 Mate Soos

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

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
class Simplifier;

class SolutionExtender
{
    public:
        SolutionExtender(Solver* _solver, Simplifier* simplifier);
        void extend();
        void addClause(const vector<Lit>& lits, const Lit blockedOn);
        void dummyBlocked(const Lit blockedOn);

    private:
        Solver* solver;
        Simplifier* simplifier;

        bool satisfied(const vector<Lit>& lits) const;
        bool contains_lit(
            const vector<Lit>& lits
            , const Lit tocontain
        ) const;
};

} //end namespace

#endif //__SOLUTIONEXTENDER_H__
