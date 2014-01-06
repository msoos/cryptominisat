#include "cryptominisat.h"
#include "solver.h"
#include "drup.h"

using namespace CMSat;

MainSolver::MainSolver(const SolverConf conf)
{
    solver = (void*)(new ::CMSat::Solver(conf));
};

bool MainSolver::add_clause(const vector< Lit >& lits)
{
    return ((CMSat::Solver*)solver)->addClauseOuter(lits);
}

lbool MainSolver::solve(vector< Lit >* assumptions)
{
    return toLbool(((CMSat::Solver*)solver)->solve_with_assumptions(assumptions).getchar());
}

const vector< lbool >& MainSolver::get_model() const
{
    return (vector<lbool>&)((CMSat::Solver*)solver)->model;
}

const std::vector<Lit>& MainSolver::get_conflict() const
{
    return (vector<Lit>&)(((CMSat::Solver*)solver)->conflict);
}

uint32_t MainSolver::nVars() const
{
    return ((CMSat::Solver*)solver)->nVarsOutside();
}

void MainSolver::new_var()
{
    ((CMSat::Solver*)solver)->new_external_var();
}

void MainSolver::add_file(const std::string& filename)
{
    ((CMSat::Solver*)solver)->fileAdded(filename);
}

SolverConf MainSolver::get_conf() const
{
    return ((CMSat::Solver*)solver)->getConf();
}

std::string MainSolver::get_version() const
{
    return ((CMSat::Solver*)solver)->getVersion();
}

void MainSolver::print_stats() const
{
    ((CMSat::Solver*)solver)->printStats();
}

void MainSolver::set_drup(std::ostream* os)
{
    CMSat::DrupFile* drup = new CMSat::DrupFile();
    drup->setFile(os);
    ((CMSat::Solver*)solver)->drup = drup;
}

void MainSolver::interrupt_asap()
{
    ((CMSat::Solver*)solver)->setNeedToInterrupt();
}

void MainSolver::open_file_and_dump_irred_clauses(std::string fname) const
{
    ((CMSat::Solver*)solver)->open_file_and_dump_irred_clauses(fname);
}

void MainSolver::open_file_and_dump_red_clauses(std::string fname) const
{
    ((CMSat::Solver*)solver)->open_file_and_dump_red_clauses(fname);
}

void MainSolver::add_in_partial_solving_stats()
{
    ((CMSat::Solver*)solver)->add_in_partial_solving_stats();
}

std::vector<Lit> MainSolver::get_zero_assigned_lits() const
{
    return ((CMSat::Solver*)solver)->get_zero_assigned_lits();
}

