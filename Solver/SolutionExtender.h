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

#include "Solver.h"

class SolutionExtender
{
    class MyClause
    {
        public:
            MyClause(const vector<Lit>& _lits, const bool _isXor, const bool _rhs = true) :
                lits(_lits)
                , isXor(_isXor)
                , rhs(_rhs)
            {
            }

            MyClause(const Clause& cl) :
                isXor(false)
                , rhs(true)
            {
                for (const Lit *l = cl.getData(), *end = cl.getDataEnd(); l != end; l++) {
                    lits.push_back(*l);
                }
            }

            MyClause(const Lit lit1, const Lit lit2) :
                isXor(false)
                , rhs(true)
            {
                lits.push_back(lit1);
                lits.push_back(lit2);
            }

            MyClause(const XorClause& cl) :
                isXor(true)
                , rhs(!cl.xorEqualFalse())
            {
                for (const Lit *l = cl.getData(), *end = cl.getDataEnd(); l != end; l++) {
                    assert(!l->sign());
                    lits.push_back(l->unsign());
                }
            }

            const bool getXor() const
            {
                return isXor;
            }

            const bool getRhs() const
            {
                return rhs;
            }

            const size_t size() const
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

        private:
            vector<Lit> lits;
            const bool isXor;
            const bool rhs;
    };
    public:
        SolutionExtender(Solver& solver);
        void extend();
        void addClause(const vector<Lit>& lits);
        void addXorClause(const vector<Lit>& lits, const bool xorEqualFalse);

    private:
        void propagateCl(MyClause& cl);
        void propagate();
        const Lit pickBranchLit();

        vector<vector<MyClause*> > occur;
        vector<MyClause*> clauses;

        Solver& solver;
};

#endif //__SOLUTIONEXTENDER_H__
