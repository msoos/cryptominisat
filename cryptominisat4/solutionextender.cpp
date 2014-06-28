#include "solutionextender.h"
#include "solver.h"
#include "varreplacer.h"
#include "simplifier.h"

//#define VERBOSE_DEBUG_SOLUTIONEXTENDER

using namespace CMSat;

SolutionExtender::SolutionExtender(Solver* _solver, Simplifier* _simplifier) :
    solver(_solver)
    , simplifier(_simplifier)
{
}

void SolutionExtender::extend()
{
    if (solver->varReplacer)
        solver->varReplacer->extendModel();

    if (simplifier)
        simplifier->extendModel(this);
}

bool SolutionExtender::satisfied(const vector< Lit >& lits) const
{
    for(const Lit lit: lits) {
        if (solver->modelValue(lit) == l_True)
            return true;
    }

    return false;
}

bool SolutionExtender::contains_lit(
    const vector<Lit>& lits
    , const Lit tocontain
) const {
    for(const Lit lit: lits) {
        if (lit == tocontain)
            return true;
    }

    return false;
}

void SolutionExtender::dummyBlocked(const Lit blockedOn)
{
    #ifdef VERBOSE_DEBUG_SOLUTIONEXTENDER
    cout
    << "dummy blocked lit "
    << solver->map_inter_to_outer(blockedOn)
    << endl;
    #endif

    const Var blockedOn_inter = solver->map_outer_to_inter(blockedOn.var());
    assert(solver->varData[blockedOn_inter].removed == Removed::elimed);

    //Oher blocked clauses set its value already
    if (solver->modelValue(blockedOn) != l_Undef)
        return;

    assert(solver->modelValue(blockedOn) == l_Undef);
    solver->model[blockedOn.var()] = l_True;
    solver->varReplacer->extendModel(blockedOn.var());

    #ifdef VERBOSE_DEBUG_SOLUTIONEXTENDER
    cout << "dummy now: " << solver->modelValue(blockedOn) << endl;
    #endif
}

void SolutionExtender::addClause(const vector<Lit>& lits, const Lit blockedOn)
{
    const Var blocked_on_inter = solver->map_outer_to_inter(blockedOn.var());
    assert(solver->varData[blocked_on_inter].removed == Removed::elimed);
    assert(contains_lit(lits, blockedOn));
    if (satisfied(lits))
        return;

    #ifdef VERBOSE_DEBUG_SOLUTIONEXTENDER
    for(Lit lit: lits) {
        Lit lit_inter = solver->map_outer_to_inter(lit);
        cout
        << lit << ": " << solver->modelValue(lit)
        << "(elim: " << removed_type_to_string(solver->varData[lit_inter.var()].removed) << ")"
        << ", ";
    }
    cout << "blocked on: " <<  blockedOn << endl;
    #endif

    assert(solver->modelValue(blockedOn) == l_Undef);
    solver->model[blockedOn.var()] = blockedOn.sign() ? l_False : l_True;
    assert(satisfied(lits));

    solver->varReplacer->extendModel(blockedOn.var());
}

