/***********************************************************************************
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

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
**************************************************************************************************/

#ifndef PARTHANDLER_H
#define PARTHANDLER_H

#include "Solver.h"
#include "PartFinder.h"
#include "Vec.h"
#include "SolverTypes.h"

#include <map>
#include <vector>

namespace CMSat {

using std::map;
using std::vector;
using std::pair;

/**
@brief Disconnected components are treated here

Uses PartFinder to find disconnected components and treats them using
subsolvers. The solutions (if SAT) are aggregated, and at then end, the
solution is extended with the sub-solutions, and the removed clauses are
added back to the problem.
*/
class PartHandler
{
    public:
        PartHandler(Solver& solver);
        const bool handle();
        const vec<lbool>& getSavedState();
        void newVar();
        void addSavedState();
        void readdRemovedClauses();

        friend class ClauseAllocator;

    private:
        struct sort_pred {
            bool operator()(const std::pair<int,int> &left, const std::pair<int,int> &right) {
                return left.second < right.second;
            }
        };

        void configureNewSolver(Solver& newSolver) const;
        void moveVariablesBetweenSolvers(Solver& newSolver, vector<Var>& vars, const uint32_t part, const PartFinder& partFinder);

        //For moving clauses
        void moveBinClauses(Solver& newSolver, const uint32_t part, PartFinder& partFinder);
        void moveClauses(vec<XorClause*>& cs, Solver& newSolver, const uint32_t part, PartFinder& partFinder);
        void moveClauses(vec<Clause*>& cs, Solver& newSolver, const uint32_t part, PartFinder& partFinder);
        void moveLearntClauses(vec<Clause*>& cs, Solver& newSolver, const uint32_t part, PartFinder& partFinder);

        //Checking moved clauses
        const bool checkClauseMovement(const Solver& thisSolver, const uint32_t part, const PartFinder& partFinder) const;
        template<class T>
        const bool checkOnlyThisPart(const vec<T*>& cs, const uint32_t part, const PartFinder& partFinder) const;
        const bool checkOnlyThisPartBin(const Solver& thisSolver, const uint32_t part, const PartFinder& partFinder) const;

        Solver& solver; ///<The base solver
        /**
        @brief The SAT solutions that have been found by the parts

        When a part finishes with SAT, its soluton is saved here. In th end
        the solutions are aggregaed using addSavedState()
        */
        vec<lbool> savedState;
        vec<Var> decisionVarRemoved; ///<List of variables whose decision-ness has been removed (set to FALSE)

        //Clauses that have been moved to other parts
        vec<Clause*> clausesRemoved;
        vector<pair<Lit, Lit> > binClausesRemoved;
        vec<XorClause*> xorClausesRemoved;
};

/**
@brief Returns the saved state of a variable
*/
inline const vec<lbool>& PartHandler::getSavedState()
{
    return savedState;
}

/**
@brief Creates a space in savedState

So that the solution can eventually be saved here (if parts are used). By
default the value is l_Undef, i.e. no solution has been saved there.
*/
inline void PartHandler::newVar()
{
    savedState.push(l_Undef);
}

}

#endif //PARTHANDLER_H
