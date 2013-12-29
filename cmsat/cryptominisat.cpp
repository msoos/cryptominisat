#include "cryptominisat.h"
#include "solver.h"
#include "drup.h"

using namespace CryptoMiniSat;

Solver::Solver(const SolverConf conf)
{
    solver = (void*)(new ::CMSat::Solver(conf));
};

bool Solver::add_clause(const vector< Lit >& lits)
{
    return ((CMSat::Solver*)solver)->addClauseOuter(lits);
}

lbool Solver::solve(vector< Lit >* assumptions)
{
    return toLbool(((CMSat::Solver*)solver)->solve_with_assumptions(assumptions).getchar());
}

const vector< lbool >& Solver::get_model() const
{
    return (vector<lbool>&)((CMSat::Solver*)solver)->model;
}

const std::vector<Lit>& Solver::get_conflict() const
{
    return (vector<Lit>&)(((CMSat::Solver*)solver)->conflict);
}

uint32_t Solver::nVars() const
{
    return ((CMSat::Solver*)solver)->nVarsOutside();
}

void Solver::new_var()
{
    ((CMSat::Solver*)solver)->new_external_var();
}

void Solver::add_file(const std::string& filename)
{
    ((CMSat::Solver*)solver)->fileAdded(filename);
}

SolverConf Solver::get_conf() const
{
    return ((CMSat::Solver*)solver)->getConf();
}

std::string Solver::get_version() const
{
    return ((CMSat::Solver*)solver)->getVersion();
}

void Solver::print_stats() const
{
    ((CMSat::Solver*)solver)->printStats();
}

void Solver::set_drup(std::ostream* os)
{
    CMSat::DrupFile* drup = new CMSat::DrupFile();
    drup->setFile(os);
    ((CMSat::Solver*)solver)->drup = drup;
}
