/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
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
class Watched;

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
        void save_on_var_memory();
        void addSavedState(vector<lbool>& solution);
        void readdRemovedClauses();
        const RemovedClauses& getRemovedClauses() const;
        void dump_removed_clauses(std::ostream* outfile) const;
        size_t get_num_vars_removed() const;
        size_t mem_used() const;

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
        bool try_to_solve_component(
            const uint32_t comp_at
            , const uint32_t comp
            , const vector<Var>& vars
            , const size_t num_comps
        );
        bool solve_component(
            const uint32_t comp_at
            , const uint32_t comp
            , const vector<Var>& vars_orig
            , const size_t num_comps
        );
        vector<pair<uint32_t, uint32_t> > get_component_sizes() const;

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
        void move_binary_clause(
            SATSolver* newSolver
            , const uint32_t comp
            ,  Watched *i
            , const Lit lit
        );
        void move_tri_clause(
            SATSolver* newSolver
            , const uint32_t comp
            ,  Watched *i
            , const Lit lit
        );
        void remove_tri_except_for_lit1(
            const Lit lit
            , const Lit lit2
            , const Lit lit3
        );
        void remove_bin_except_for_lit1(const Lit lit, const Lit lit2);

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
        size_t num_vars_removed = 0;

        //Clauses that have been moved to other comps
        //vector<ClOffset> clausesRemoved;
        //vector<pair<Lit, Lit> > binClausesRemoved;

        uint32_t numRemovedHalfIrred = 0;
        uint32_t numRemovedHalfRed = 0;
        uint32_t numRemovedThirdIrred = 0;
        uint32_t numRemovedThirdRed = 0;
        vector<Lit> tmp_lits;
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

inline size_t CompHandler::get_num_vars_removed() const
{
    return num_vars_removed;
}

} //end of namespace

#endif //PARTHANDLER_H
