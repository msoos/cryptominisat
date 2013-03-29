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

namespace CMSat {

#ifdef VERBOSE_DEBUG
#define VERBOSE_DEBUG_RECONSTRUCT
#endif

class Solver;

class SolutionExtender
{
    class MyClause
    {
        public:
            MyClause(const vector<Lit>& _lits, const Lit _blockedOn = lit_Undef) :
                lits(_lits)
                , blockedOn(_blockedOn)
            {
            }

            MyClause(const Clause& cl, const Lit _blockedOn = lit_Undef) :
                blockedOn(_blockedOn)
            {
                for (const Lit *l = cl.begin(), *end = cl.end(); l != end; l++) {
                    lits.push_back(*l);
                }
            }

            MyClause(const Lit lit1, const Lit lit2, const Lit _blockedOn = lit_Undef) :
                blockedOn(_blockedOn)
            {
                lits.push_back(lit1);
                lits.push_back(lit2);
            }

            size_t size() const
            {
                return lits.size();
            }

            vector<Lit>::const_iterator begin() const
            {
                return lits.begin();
            }

            vector<Lit>::const_iterator end() const
            {
                return lits.end();
            }

            const vector<Lit>& getLits() const
            {
                return lits;
            }

            vector<Lit> lits;
            Lit blockedOn;
    };
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
        bool propagateCl(MyClause& cl);
        bool propagate();
        bool propBinaryClause(
            const vec<Watched>::const_iterator i
            , const Lit p
        );
        bool propTriClause(
            const vec<Watched>::const_iterator i
            , const Lit p
        );
        bool satisfiedNorm(const vector<Lit>& lits) const;
        bool satisfiedXor(const vector<Lit>& lits, const bool rhs) const;
        Lit pickBranchLit();

        uint32_t nVars()
        {
            return assigns.size();
        }


        Solver* solver;
        vector<vector<MyClause*> > occur;
        vector<MyClause*> clauses;
        uint32_t qhead;
        vector<Lit> trail;
        vector<lbool> assigns;
};

} //end namespace

#endif //__SOLUTIONEXTENDER_H__
