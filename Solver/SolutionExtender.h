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
#include "SolverTypes.h"

#ifdef VERBOSE_DEBUG
#define VERBOSE_DEBUG_RECONSTRUCT
#endif

//#define VERBOSE_DEBUG_RECONSTRUCT

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

            const vector<Lit>& getLits() const
            {
                return lits;
            }

        private:
            vector<Lit> lits;
            const bool isXor;
            const bool rhs;
    };
    public:
        SolutionExtender(Solver& solver);
        void extend();
        const bool addClause(const vector<Lit>& lits);
        void addBlockedClause(const BlockedClause& cl);
        void addXorClause(const vector<Lit>& lits, const bool xorEqualFalse);

        void enqueue(const Lit lit)
        {
            assigns[lit.var()] = boolToLBool(!lit.sign());
            trail.push_back(lit);
            #ifdef VERBOSE_DEBUG_RECONSTRUCT
            std::cout << "c Enqueueing lit " << lit << " during solution reconstruction" << std::endl;
            #endif
            solver.varData[lit.var()].level = std::numeric_limits< uint32_t >::max();
        }

        const lbool value(const Lit lit) const
        {
            return assigns[lit.var()] ^ lit.sign();
        }

        const lbool value(const Var var) const
        {
            return assigns[var];
        }

    private:
        const bool propagateCl(MyClause& cl);
        const bool propagate();
        const bool satisfiedNorm(const vector<Lit>& lits) const;
        const bool satisfiedXor(const vector<Lit>& lits, const bool rhs) const;
        const Lit pickBranchLit();

        const uint32_t nVars()
        {
            return assigns.size();
        }

        vector<vector<MyClause*> > occur;
        vector<MyClause*> clauses;

        Solver& solver;
        uint32_t qhead;
        vector<Lit> trail;
        vector<lbool> assigns;
};

#endif //__SOLUTIONEXTENDER_H__
