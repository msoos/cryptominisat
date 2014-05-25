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

#include "comphandler.h"
#include "compfinder.h"
#include "varreplacer.h"
#include "solver.h"
#include "varupdatehelper.h"
#include "watchalgos.h"
#include "clauseallocator.h"
#include "clausecleaner.h"
#include <iostream>
#include <assert.h>
#include <iomanip>
#include "cryptominisat.h"

using namespace CMSat;
using std::make_pair;
using std::cout;
using std::endl;

//#define VERBOSE_DEBUG

CompHandler::CompHandler(Solver* _solver) :
    solver(_solver)
    , compFinder(NULL)
{
}

CompHandler::~CompHandler()
{
    if (compFinder != NULL) {
        delete compFinder;
    }
}

void CompHandler::new_var(const Var orig_outer)
{
    if (orig_outer == std::numeric_limits<Var>::max()) {
        savedState.push_back(l_Undef);
    }
}

void CompHandler::new_vars(size_t n)
{
    savedState.resize(savedState.size()+n, l_Undef);
}

void CompHandler::saveVarMem()
{
}

void CompHandler::createRenumbering(const vector<Var>& vars)
{
    smallsolver_to_bigsolver.resize(vars.size());
    bigsolver_to_smallsolver.resize(solver->nVars());

    for(size_t i = 0, size = vars.size()
        ; i < size
        ; i++
    ) {
        bigsolver_to_smallsolver[vars[i]] = i;
        smallsolver_to_bigsolver[i] = vars[i];
    }
}

bool CompHandler::assumpsInsideComponent(const vector<Var>& vars)
{
    for(Var var: vars) {
        if (solver->var_inside_assumptions(var)) {
            return true;
        }
    }

    return false;
}

bool CompHandler::handle()
{
    assert(solver->okay());
    double myTime = cpuTime();
    compFinder = new CompFinder(solver);
    if (!compFinder->findComps()) {
        return false;
    }
    if (compFinder->getTimedOut()) {
        return solver->okay();
    }

    const uint32_t num_comps = compFinder->getReverseTable().size();

    //If there is only one big comp, we can't do anything
    if (num_comps <= 1) {
        if (solver->conf.verbosity >= 3) {
            cout
            << "c [comp] Only one component, not handling it separately"
            << endl;
        }
        return true;
    }

    map<uint32_t, vector<Var> > reverseTable = compFinder->getReverseTable();
    assert(num_comps == compFinder->getReverseTable().size());

    //Get the sizes now
    vector<pair<uint32_t, uint32_t> > sizes;
    for (map<uint32_t, vector<Var> >::iterator
        it = reverseTable.begin()
        ; it != reverseTable.end()
        ; it++
    ) {
        sizes.push_back(make_pair(
            it->first //Comp number
            , (uint32_t)it->second.size() //Size of the table
        ));
    }

    //Sort according to smallest size first
    std::sort(sizes.begin(), sizes.end(), sort_pred());
    assert(sizes.size() > 1);

    size_t num_comps_solved = 0;
    size_t vars_solved = 0;
    for (uint32_t it = 0; it < sizes.size()-1; it++) {
        const uint32_t comp = sizes[it].first;
        vector<Var>& vars = reverseTable[comp];
        const bool cont = solve_component(it, comp, vars, num_comps);
        if (!cont) {
            break;
        }
        num_comps_solved++;
        vars_solved += vars.size();
    }

    if (!solver->okay())
        return false;

    //Coming back to the original instance now
    if (solver->conf.verbosity  >= 1) {
        cout
        << "c [comp] Coming back to original instance, solved "
        << num_comps_solved << " component(s), "
        << vars_solved << " vars"
        << " T: "
        << std::setprecision(2) << std::fixed
        << cpuTime() - myTime
        << endl;
    }

    //Filter out the variables that have been made non-decision
    check_local_vardata_sanity();

    delete compFinder;
    compFinder = NULL;
    return true;
}

bool CompHandler::solve_component(
    const uint32_t comp_at
    , const uint32_t comp
    , const vector<Var>& vars_orig
    , const size_t num_comps
) {
    for(const Var var: vars_orig) {
        assert(solver->value(var) == l_Undef);
    }

    if (vars_orig.size() > 100ULL*1000ULL) {
        //There too many variables -- don't create a sub-solver
        //I'm afraid that we will memory-out

        return true;
    }

    //Components with assumptions should not be removed
    if (assumpsInsideComponent(vars_orig))
        return true;

    vector<Var> vars(vars_orig);

    //Sort and renumber
    std::sort(vars.begin(), vars.end());
    /*for(Var var: vars) {
        cout << "var in component: " << solver->map_inter_to_outer(var) + 1 << endl;
    }*/
    createRenumbering(vars);

    //Print what we are going to do
    if (solver->conf.verbosity >= 1 && num_comps < 20) {
        cout
        << "c [comp] Solving component " << comp_at
        << " num vars: " << vars.size()
        << " ======================================="
        << endl;
    }

    //Set up new solver
    SolverConf conf = configureNewSolver(vars.size());
    SATSolver newSolver(conf, solver->get_must_interrupt_asap_ptr());
    moveVariablesBetweenSolvers(&newSolver, vars, comp);

    //Move clauses over
    moveClausesImplicit(&newSolver, comp, vars);
    moveClausesLong(solver->longIrredCls, &newSolver, comp);
    moveClausesLong(solver->longRedCls, &newSolver, comp);

    const lbool status = newSolver.solve();
    //Out of time
    if (status == l_Undef) {
        if (solver->conf.verbosity >= 2) {
            cout
            << "c subcomponent returned l_Undef -- timeout or interrupt."
            << endl;
        }
        readdRemovedClauses();
        return false;
    }

    if (status == l_False) {
        solver->ok = false;
        if (solver->conf.verbosity >= 2) {
            cout
            << "c [comp] The component is UNSAT -> problem is UNSAT"
            << endl;
        }
        return false;
    }

    check_solution_is_unassigned_in_main_solver(&newSolver, vars);
    save_solution_to_savedstate(&newSolver, vars, comp);
    move_decision_level_zero_vars_here(&newSolver);

    if (solver->conf.verbosity >= 1 && num_comps < 20) {
        cout
        << "c [comp] component " << comp_at
        << " ======================================="
        << endl;
    }
    return true;
}

void CompHandler::check_local_vardata_sanity()
{
    //Checking that all variables that are not in the remaining comp have
    //correct 'removed' flags, and none have been assigned

    for (Var var = 0; var < solver->nVars(); var++) {
        const Var outerVar = solver->map_inter_to_outer(var);
        if (savedState[outerVar] != l_Undef) {
            assert(solver->varData[var].is_decision == false);
            assert(solver->varData[var].removed == Removed::decomposed);
            assert(solver->value(var) == l_Undef || solver->varData[var].level == 0);
        }
    }
}

void CompHandler::check_solution_is_unassigned_in_main_solver(
    const SATSolver* newSolver
    , const vector<Var>& vars
) {
    for (size_t i = 0; i < vars.size(); i++) {
        Var var = vars[i];
        if (newSolver->get_model()[upd_bigsolver_to_smallsolver(var)] != l_Undef) {
            assert(solver->value(var) == l_Undef);
        }
    }
}

void CompHandler::save_solution_to_savedstate(
    const SATSolver* newSolver
    , const vector<Var>& vars
    , const uint32_t comp
) {
    assert(savedState.size() == solver->nVarsOuter());
    for (size_t i = 0; i < vars.size(); i++) {
        Var var = vars[i];
        Var outerVar = solver->map_inter_to_outer(var);
        if (newSolver->get_model()[upd_bigsolver_to_smallsolver(var)] != l_Undef) {
            assert(savedState[outerVar] == l_Undef);
            assert(compFinder->getVarComp(var) == comp);

            savedState[outerVar] = newSolver->get_model()[upd_bigsolver_to_smallsolver(var)];
        }
    }
}

void CompHandler::move_decision_level_zero_vars_here(
    const SATSolver* newSolver
) {
    const vector<Lit> zero_assigned = newSolver->get_zero_assigned_lits();
    for (Lit lit: zero_assigned) {
        assert(lit.var() < newSolver->nVars());
        assert(lit.var() < smallsolver_to_bigsolver.size());
        lit = Lit(smallsolver_to_bigsolver[lit.var()], lit.sign());
        assert(solver->value(lit) == l_Undef);
        solver->varData[lit.var()].removed = Removed::none;
        const Var outer = solver->map_inter_to_outer(lit.var());
        savedState[outer] = l_Undef;
        solver->enqueue(lit);

        //These vars are not meant to be in the orig solver
        //so they cannot cause UNSAT
        solver->ok = (solver->propagate().isNULL());
        assert(solver->ok);
    }
}


SolverConf CompHandler::configureNewSolver(
    const size_t numVars
) const {
    SolverConf conf(solver->conf);
    conf.origSeed = solver->mtrand.randInt();
    if (numVars < 60) {
        conf.regularly_simplify_problem = false;
        conf.doStamp = false;
        conf.doCache = false;
        conf.doProbe = false;
        conf.otfHyperbin = false;
        conf.verbosity = std::min(solver->conf.verbosity, 0);
    }
    conf.doSQL = false;

    //To small, don't clogger up the screen
    if (numVars < 20 && solver->conf.verbosity < 3) {
        conf.verbosity = 0;
    }

    //Don't recurse
    conf.doCompHandler = false;

    return conf;
}

/**
@brief Moves the variables to the new solver

This implies making the right variables decision in the new solver,
and making it non-decision in the old solver.
*/
void CompHandler::moveVariablesBetweenSolvers(
    SATSolver* newSolver
    , const vector<Var>& vars
    , const uint32_t comp
) {
    for(const Var var: vars) {
        //Misc check
        #ifdef VERBOSE_DEBUG
        if (!solver->varData[var].is_decision) {
            cout
            << "var " << var + 1
            << " is non-decision, but in comp... strange."
            << endl;
        }
        #endif //VERBOSE_DEBUG

        newSolver->new_var();
        assert(compFinder->getVarComp(var) == comp);
        assert(solver->value(var) == l_Undef);

        assert(solver->varData[var].removed == Removed::none);
        assert(solver->varData[var].is_decision);
        solver->unsetDecisionVar(var);
        solver->varData[var].removed = Removed::decomposed;
    }
}

void CompHandler::moveClausesLong(
    vector<ClOffset>& cs
    , SATSolver* newSolver
    , const uint32_t comp
) {
    vector<Lit> tmp;

    vector<ClOffset>::iterator i, j, end;
    for (i = j = cs.begin(), end = cs.end()
        ; i != end
        ; i++
    ) {
        Clause& cl = *solver->clAllocator.getPointer(*i);

        //Irred, different comp
        if (!cl.red()) {
            if (compFinder->getVarComp(cl[0].var()) != comp) {
                //different comp, move along
                *j++ = *i;
                continue;
            }
        }

        if (cl.red()) {
            //Check which comp(s) it belongs to
            bool thisComp = false;
            bool otherComp = false;
            for (Lit* l = cl.begin(), *end2 = cl.end(); l != end2; l++) {
                if (compFinder->getVarComp(l->var()) == comp)
                    thisComp = true;

                if (compFinder->getVarComp(l->var()) != comp)
                    otherComp = true;
            }

            //In both comps, remove it
            if (thisComp && otherComp) {
                solver->detachClause(cl);
                solver->clAllocator.clauseFree(&cl);
                continue;
            }

            //In one comp, but not this one
            if (!thisComp) {
                //different comp, move along
                *j++ = *i;
                continue;
            }
            assert(thisComp && !otherComp);
        }

        //Let's move it to the other solver!
        #ifdef VERBOSE_DEBUG
        cout << "clause in this comp:" << cl << endl;
        #endif

        //Create temporary space 'tmp' and copy to backup
        tmp.resize(cl.size());
        for (size_t i2 = 0; i2 < cl.size(); i2++) {
            tmp[i2] = upd_bigsolver_to_smallsolver(cl[i2]);
        }

        //Add 'tmp' to the new solver
        if (cl.red()) {
            cl.stats.introduced_at_conflict = 0;
            //newSolver->addRedClause(tmp, cl.stats);
        } else {
            saveClause(cl);
            newSolver->add_clause(tmp);
        }

        //Remove from here
        solver->detachClause(cl);
        solver->clAllocator.clauseFree(&cl);
    }
    cs.resize(cs.size() - (i-j));
}

void CompHandler::moveClausesImplicit(
    SATSolver* newSolver
    , const uint32_t comp
    , const vector<Var>& vars
) {
    vector<Lit> lits;
    uint32_t numRemovedHalfIrred = 0;
    uint32_t numRemovedHalfRed = 0;
    uint32_t numRemovedThirdIrred = 0;
    uint32_t numRemovedThirdRed = 0;

    for(const Var var: vars) {
    for(unsigned sign = 0; sign < 2; sign++) {
        const Lit lit = Lit(var, sign);
        watch_subarray ws = solver->watches[lit.toInt()];

        //If empty, nothing to to, skip
        if (ws.empty()) {
            continue;
        }

        Watched *i = ws.begin();
        Watched *j = i;
        for (Watched *end2 = ws.end()
            ; i != end2
            ; i++
        ) {
            //At least one variable inside comp
            if (i->isBinary()
                && (compFinder->getVarComp(lit.var()) == comp
                    || compFinder->getVarComp(i->lit2().var()) == comp
                )
            ) {
                const Lit lit2 = i->lit2();

                //Unless redundant, cannot be in 2 comps at once
                assert((compFinder->getVarComp(lit.var()) == comp
                            && compFinder->getVarComp(lit2.var()) == comp
                       ) || i->red()
                );

                //If it's redundant and the lits are in different comps, remove it.
                if (compFinder->getVarComp(lit.var()) != comp
                    || compFinder->getVarComp(lit2.var()) != comp
                ) {
                    //Can only be redundant, otherwise it would be in the same
                    //component
                    assert(i->red());

                    //The way we go through this, it's definitely going to be
                    //lit2 that's in the other component
                    assert(compFinder->getVarComp(lit2.var()) != comp);

                    removeWBin(solver->watches, lit2, lit, true);

                    //Update stats
                    solver->binTri.redBins--;

                    //Not copy, that's the other Watched removed
                    continue;
                }

                //don't add the same clause twice
                if (lit < lit2) {

                    //Add clause
                    lits = {upd_bigsolver_to_smallsolver(lit), upd_bigsolver_to_smallsolver(lit2)};
                    assert(compFinder->getVarComp(lit.var()) == comp);
                    assert(compFinder->getVarComp(lit2.var()) == comp);

                    //Add new clause
                    if (i->red()) {
                        //newSolver->addRedClause(lits);
                        numRemovedHalfRed++;
                    } else {
                        //Save backup
                        saveClause(vector<Lit>{lit, lit2});

                        newSolver->add_clause(lits);
                        numRemovedHalfIrred++;
                    }
                } else {

                    //Just remove, already added above
                    if (i->red()) {
                        numRemovedHalfRed++;
                    } else {
                        numRemovedHalfIrred++;
                    }
                }

                //Yes, remove
                continue;
            }

            if (i->isTri()
                && (compFinder->getVarComp(lit.var()) == comp
                    || compFinder->getVarComp(i->lit2().var()) == comp
                    || compFinder->getVarComp(i->lit3().var()) == comp
                )
            ) {
                const Lit lit2 = i->lit2();
                const Lit lit3 = i->lit3();

                //Unless redundant, cannot be in 2 comps at once
                assert((compFinder->getVarComp(lit.var()) == comp
                            && compFinder->getVarComp(lit2.var()) == comp
                            && compFinder->getVarComp(lit3.var()) == comp
                       ) || i->red()
                );

                //If it's redundant and the lits are in different comps, remove it.
                if (compFinder->getVarComp(lit.var()) != comp
                    || compFinder->getVarComp(lit2.var()) != comp
                    || compFinder->getVarComp(lit3.var()) != comp
                ) {
                    assert(i->red());

                    //The way we go through this, it's definitely going to be
                    //either lit2 or lit3, not lit, that's in the other comp
                    assert(compFinder->getVarComp(lit2.var()) != comp
                        || compFinder->getVarComp(lit3.var()) != comp
                    );

                    //Update stats
                    solver->binTri.redTris--;

                    //We need it sorted, because that's how we know what order
                    //it is in the Watched()
                    lits = {lit, lit2, lit3};
                    std::sort(lits.begin(), lits.end());

                    //Remove only 2, the remaining gets removed by not copying it over
                    if (lits[0] != lit) {
                        removeWTri(solver->watches, lits[0], lits[1], lits[2], true);
                    }
                    if (lits[1] != lit) {
                        removeWTri(solver->watches, lits[1], lits[0], lits[2], true);
                    }
                    if (lits[2] != lit) {
                        removeWTri(solver->watches, lits[2], lits[0], lits[1], true);
                    }

                    //Not copying, that's the 3rd one
                    continue;
                }

                //don't add the same clause twice
                if (lit < lit2
                    && lit2 < lit3
                ) {

                    //Add clause
                    lits = {upd_bigsolver_to_smallsolver(lit), upd_bigsolver_to_smallsolver(lit2), upd_bigsolver_to_smallsolver(lit3)};
                    assert(compFinder->getVarComp(lit.var()) == comp);
                    assert(compFinder->getVarComp(lit2.var()) == comp);
                    assert(compFinder->getVarComp(lit3.var()) == comp);

                    //Add new clause
                    if (i->red()) {
                        //newSolver->addRedClause(lits);
                        numRemovedThirdRed++;
                    } else {
                        //Save backup
                        saveClause(vector<Lit>{lit, lit2, lit3});

                        newSolver->add_clause(lits);
                        numRemovedThirdIrred++;
                    }
                } else {

                    //Just remove, already added above
                    if (i->red()) {
                        numRemovedThirdRed++;
                    } else {
                        numRemovedThirdIrred++;
                    }
                }

                //Yes, remove
                continue;
            }

            *j++ = *i;
        }
        ws.shrink_(i-j);
    }}

    assert(numRemovedHalfIrred % 2 == 0);
    solver->binTri.irredBins -= numRemovedHalfIrred/2;

    assert(numRemovedThirdIrred % 3 == 0);
    solver->binTri.irredTris -= numRemovedThirdIrred/3;

    assert(numRemovedHalfRed % 2 == 0);
    solver->binTri.redBins -= numRemovedHalfRed/2;

    assert(numRemovedThirdRed % 3 == 0);
    solver->binTri.redTris -= numRemovedThirdRed/3;
}

void CompHandler::addSavedState(vector<lbool>& solution)
{
    //Enqueue them. They may need to be extended, so enqueue is needed
    //manipulating "model" may not be good enough
    assert(savedState.size() == solver->nVarsOuter());
    assert(solution.size() == solver->nVarsOuter());
    for (size_t var = 0; var < savedState.size(); var++) {
        if (savedState[var] != l_Undef) {
            const Var interVar = solver->map_outer_to_inter(var);
            assert(solver->varData[interVar].removed == Removed::decomposed);
            assert(solver->varData[interVar].is_decision == false);

            const lbool val = savedState[var];
            assert(solution[var] == l_Undef);
            solution[var] = val;
            //cout << "Solution to var " << var + 1 << " has been added: " << val << endl;

            solver->varData[interVar].polarity = (val == l_True);
        }
    }
}

template<class T>
void CompHandler::saveClause(const T& lits)
{
    //Update variable number to 'outer' number. This means we will not have
    //to update the variables every time the internal variable numbering changes
    for (const Lit lit : lits ) {
        removedClauses.lits.push_back(
            solver->map_inter_to_outer(lit)
        );
    }
    removedClauses.sizes.push_back(lits.size());
}

void CompHandler::readdRemovedClauses()
{
    assert(solver->okay());
    double myTime = cpuTime();

    //Avoid recursion, clear 'removed' status
    for(size_t i = 0; i < solver->nVarsOuter(); i++) {
        VarData& dat = solver->varData[i];
        if (dat.removed == Removed::decomposed) {
            dat.removed = Removed::none;
            solver->setDecisionVar(i);
        }
    }

     //Clear saved state
    for(lbool& val: savedState) {
        val = l_Undef;
    }

    vector<Lit> tmp;
    size_t at = 0;
    for (uint32_t sz: removedClauses.sizes) {

        //addClause() needs *outer* literals, so just do that
        tmp.clear();
        for(size_t i = at; i < at + sz; i++) {
            tmp.push_back(removedClauses.lits[i]);
        }
        if (solver->conf.verbosity >= 6) {
            cout << "c [comp] Adding back component clause " << tmp << endl;
        }

        //Add the clause to the system
        solver->addClause(tmp);
        assert(solver->okay());

        //Move 'at' along
        at += sz;
    }

    //Explain what we just did
    if (solver->conf.verbosity >= 2) {
        cout
        << "c [comp] re-added components. Lits: "
        << removedClauses.lits.size()
        << " cls:" << removedClauses.sizes.size()
        << " T: " << std::fixed << std::setprecision(2) << cpuTime() - myTime
        << endl;
    }

    //Clear added data
    removedClauses.lits.clear();
    removedClauses.sizes.clear();
}
