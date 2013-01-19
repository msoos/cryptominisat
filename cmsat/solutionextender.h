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

#ifdef VERBOSE_DEBUG
#define VERBOSE_DEBUG_RECONSTRUCT
#endif

//#define VERBOSE_DEBUG_RECONSTRUCT

class Solver;

class SolutionExtender
{
    class MyClause
    {
        public:
            MyClause(const vector<Lit>& _lits) :
                lits(_lits)
            {
            }

            MyClause(const Clause& cl)
            {
                for (const Lit *l = cl.begin(), *end = cl.end(); l != end; l++) {
                    lits.push_back(*l);
                }
            }

            MyClause(const Lit lit1, const Lit lit2)
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

        private:
            vector<Lit> lits;
    };
    public:
        SolutionExtender(Solver* _solver, const vector<lbool>& _assigns);
        void extend();
        bool addClause(const vector<Lit>& lits);
        void addBlockedClause(const BlockedClause& cl);
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
        bool propagateCl(MyClause& cl);
        bool propagate();
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

#endif //__SOLUTIONEXTENDER_H__
