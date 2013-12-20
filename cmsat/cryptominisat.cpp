#include "cryptominisat.h"
#include "solver.h"

using namespace CryptoMiniSat;

Solver::Solver()
{
    solver = new ::CMSat::Solver();
};

bool Solver::add_clause(const vector< Lit >& lits)
{
    return solver->addClauseOuter(lits);
}

lbool Solver::solve(vector< Lit >* assumptions)
{
    return toLbool(solver->solve_with_assumptions(assumptions).getchar());
}

const vector< lbool >& Solver::get_model() const
{
    return (vector<lbool>&)solver->model;
}
