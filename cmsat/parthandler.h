/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2011, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
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

#ifndef PARTHANDLER_H
#define PARTHANDLER_H

#include "solvertypes.h"
#include "cloffset.h"
#include <map>
#include <vector>

namespace CMSat {

using std::map;
using std::vector;
using std::pair;

class Solver;
class PartFinder;

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
        PartHandler(Solver* solver);
        ~PartHandler();

        bool handle();
        const vector<lbool>& getSavedState();
        void newVar();
        void addSavedState(vector<lbool>& model);
        void readdRemovedClauses();
        void updateVars(const vector<Var>& interToOuter);

        friend class ClauseAllocator;

    private:
        struct sort_pred {
            bool operator()(
                const std::pair<int,int> &left
                , const std::pair<int,int> &right
            ) {
                return left.second < right.second;
            }
        };

        void configureNewSolver(Solver* newSolver) const;
        void moveVariablesBetweenSolvers(
            Solver* newSolver
            , vector<Var>& vars
            , const uint32_t part
        );

        //For moving clauses
        void moveClausesImplicit(
            Solver* newSolver
            , const uint32_t part
        );
        void moveClausesLong(
            vector<ClOffset>& cs
            , Solver* newSolver
            , const uint32_t part
        );

        Solver* solver;
        PartFinder* partFinder;

        ///The solutions that have been found by the parts
        vector<lbool> savedState;

        ///List of variables whose decision-ness has been removed (set to FALSE)
        vector<Var> decisionVarRemoved;

        //Clauses that have been moved to other parts
        //vector<ClOffset> clausesRemoved;
        //vector<pair<Lit, Lit> > binClausesRemoved;
};

/**
@brief Returns the saved state of a variable
*/
inline const vector<lbool>& PartHandler::getSavedState()
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
    savedState.push_back(l_Undef);
}

} //end of namespace

#endif //PARTHANDLER_H
