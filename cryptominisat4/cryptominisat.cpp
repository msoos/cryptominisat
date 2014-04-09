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

#include "cryptominisat.h"
#include "solver.h"
#include "drup.h"
#include <stdexcept>

using namespace CMSat;

SATSolver::SATSolver(const SolverConf conf)
{
    solver = (void*)(new ::CMSat::Solver(conf));
}

SATSolver::~SATSolver()
{
    delete ((CMSat::Solver*)solver);
}

bool SATSolver::add_clause(const vector< Lit >& lits)
{
    return ((CMSat::Solver*)solver)->add_clause_outer(lits);
}

bool SATSolver::add_xor_clause(const std::vector<unsigned>& vars, bool rhs)
{
    return ((CMSat::Solver*)solver)->add_xor_clause_outer(vars, rhs);
}

lbool SATSolver::solve(vector< Lit >* assumptions)
{
    return ((CMSat::Solver*)solver)->solve_with_assumptions(assumptions);
}

const vector< lbool >& SATSolver::get_model() const
{
    return (vector<lbool>&)((CMSat::Solver*)solver)->get_model();
}

const std::vector<Lit>& SATSolver::get_conflict() const
{
    return (vector<Lit>&)(((CMSat::Solver*)solver)->get_final_conflict());
}

uint32_t SATSolver::nVars() const
{
    return ((CMSat::Solver*)solver)->nVarsOutside();
}

void SATSolver::new_var()
{
    ((CMSat::Solver*)solver)->new_external_var();
}

void SATSolver::add_sql_tag(const std::string& tagname, const std::string& tag)
{
    ((CMSat::Solver*)solver)->add_sql_tag(tagname, tag);
}

SolverConf SATSolver::get_conf() const
{
    return ((CMSat::Solver*)solver)->getConf();
}

const char* SATSolver::get_version()
{
    return CMSat::Solver::getVersion();
}

void SATSolver::print_stats() const
{
    ((CMSat::Solver*)solver)->printStats();
}

void SATSolver::set_drup(std::ostream* os)
{
    CMSat::DrupFile* drup = new CMSat::DrupFile();
    drup->setFile(os);
    ((CMSat::Solver*)solver)->drup = drup;
}

void SATSolver::interrupt_asap()
{
    ((CMSat::Solver*)solver)->setNeedToInterrupt();
}

void SATSolver::open_file_and_dump_irred_clauses(std::string fname) const
{
    ((CMSat::Solver*)solver)->open_file_and_dump_irred_clauses(fname);
}

void SATSolver::open_file_and_dump_red_clauses(std::string fname) const
{
    ((CMSat::Solver*)solver)->open_file_and_dump_red_clauses(fname);
}

void SATSolver::add_in_partial_solving_stats()
{
    ((CMSat::Solver*)solver)->add_in_partial_solving_stats();
}

std::vector<Lit> SATSolver::get_zero_assigned_lits() const
{
    return ((CMSat::Solver*)solver)->get_zero_assigned_lits();
}


unsigned long SATSolver::get_sql_id() const
{
    return ((CMSat::Solver*)solver)->get_sql_id();
}
