/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
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

class SATSolver;
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
        void new_var(const Var orig_outer);
        void new_vars(const size_t n);
        void saveVarMem();
        void addSavedState(vector<lbool>& solution);
        void readdRemovedClauses();
        const RemovedClauses& getRemovedClauses() const;

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
        void move_decision_level_zero_vars_here(
            const SATSolver* newSolver
        );
        void save_solution_to_savedstate(
            const SATSolver* newSolver
            , const vector<Var>& vars
            , const uint32_t comp
        );
        void check_solution_is_unassigned_in_main_solver(
            const SATSolver* newSolver
            , const vector<Var>& vars
        );
        void check_local_vardata_sanity();
        bool solve_component(
            const uint32_t comp_at
            , const uint32_t comp
            , const vector<Var>& vars
            , const size_t num_comps
        );

        SolverConf configureNewSolver(
            const size_t numVars
        ) const;

        void moveVariablesBetweenSolvers(
            SATSolver* newSolver
            , const vector<Var>& vars
            , const uint32_t comp
        );

        //For moving clauses
        void moveClausesImplicit(
            SATSolver* newSolver
            , const uint32_t comp
            , const vector<Var>& vars
        );
        void moveClausesLong(
            vector<ClOffset>& cs
            , SATSolver* newSolver
            , const uint32_t comp
        );

        Solver* solver;
        CompFinder* compFinder;

        ///The solutions that have been found by the comps
        vector<lbool> savedState;

        //Re-numbering
        void createRenumbering(const vector<Var>& vars);
        vector<Var> useless; //temporary
        vector<Var> smallsolver_to_bigsolver;
        vector<Var> bigsolver_to_smallsolver;

        Lit upd_bigsolver_to_smallsolver(const Lit lit) const
        {
            return Lit(upd_bigsolver_to_smallsolver(lit.var()), lit.sign());
        }

        Var upd_bigsolver_to_smallsolver(const Var var) const
        {
            return bigsolver_to_smallsolver[var];
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
