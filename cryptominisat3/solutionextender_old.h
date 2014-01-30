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

class SolutionExtender
{
    public:
        SolutionExtender(Solver* _solver, const vector<lbool>& _assigns);
        void extend();
        bool addClause(const vector<Lit>& lits, const Lit blockedOn = lit_Undef);
        void enqueue(const Lit lit);

        lbool value(const Lit lit) const
        {
            return assigns[lit.var()] ^ lit.sign();
        }

        lbool value(const Var var) const
        {
            return assigns[var];
        }

    private:
        void replaceSet(Lit toSet);
        void replaceBackwardSet(const Lit toSet);
        bool propagateCl(const Clause* cl, const Lit blockedOn);
        bool propagate();
        bool propBinaryClause(
            watch_subarray_const::const_iterator i
            , const Lit p
        );
        bool propTriClause(
            watch_subarray_const::const_iterator i
            , const Lit p
        );
        bool satisfiedNorm(const vector<Lit>& lits) const;
        bool satisfiedXor(const vector<Lit>& lits, const bool rhs) const;
        Lit pickBranchLit();

        uint32_t nVarsOuter() const
        {
            return assigns.size();
        }

        //To reduce mem alloc overhead
        vector<Lit> tmpLits;


        Solver* solver;
        vector<ClOffset> clausesToFree;
        uint32_t qhead;
        vector<Lit> trail;
        vector<lbool> assigns;
};

} //end namespace

#endif //__SOLUTIONEXTENDER_H__
