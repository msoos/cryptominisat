/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.0 of the License, or (at your option) any later version.
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
class CompFinder;

/**
@brief Disconnected components are treated here

Uses CompFinder to find disconnected components and treats them using
subsolvers. The solutions (if SAT) are aggregated, and at then end, the
solution is extended with the sub-solutions, and the removed clauses are
added back to the problem.
*/
class CompHandler
{
    public:
        CompHandler(Solver* solver);
        ~CompHandler();

        struct RemovedClauses {
            vector<Lit> lits;
            vector<uint32_t> sizes;
        };

        bool handle();
        const vector<lbool>& getSavedState();
        void newVar();
        void addSavedState(vector<lbool>& solution);
        void readdRemovedClauses();
        const RemovedClauses& getRemovedClauses() const;

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
        bool assumpsInsideComponent(const vector<Var>& vars);

        void configureNewSolver(
            Solver* newSolver
            , const size_t numVars
        ) const;

        void moveVariablesBetweenSolvers(
            Solver* newSolver
            , vector<Var>& vars
            , const uint32_t comp
        );

        //For moving clauses
        void moveClausesImplicit(
            Solver* newSolver
            , const uint32_t comp
            , const vector<Var>& vars
        );
        void moveClausesLong(
            vector<ClOffset>& cs
            , Solver* newSolver
            , const uint32_t comp
        );

        Solver* solver;
        CompFinder* compFinder;

        ///The solutions that have been found by the comps
        vector<lbool> savedState;

        ///List of variables whose decision-ness has been removed (set to FALSE)
        vector<Var> decisionVarRemoved;

        //Re-numbering
        void createRenumbering(const vector<Var>& vars);
        vector<Var> useless; //temporary
        vector<Var> interToOuter;
        vector<Var> outerToInter;

        Lit updateLit(const Lit lit) const
        {
            return Lit(updateVar(lit.var()), lit.sign());
        }

        Var updateVar(const Var var) const
        {
            return outerToInter[var];
        }

        //Saving clauses
        template<class T>
        void saveClause(const T& lits);
        RemovedClauses removedClauses;

        //Clauses that have been moved to other comps
        //vector<ClOffset> clausesRemoved;
        //vector<pair<Lit, Lit> > binClausesRemoved;
};

/**
@brief Returns the saved state of a variable
*/
inline const vector<lbool>& CompHandler::getSavedState()
{
    return savedState;
}

inline const CompHandler::RemovedClauses& CompHandler::getRemovedClauses() const
{
    return removedClauses;
}

} //end of namespace

#endif //PARTHANDLER_H
