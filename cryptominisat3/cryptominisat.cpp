#include "cryptominisat.h"
#include "solver.h"
#include "drup.h"

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
