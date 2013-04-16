/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2011, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
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

#include "parthandler.h"
#include "partfinder.h"
#include "varreplacer.h"
#include "solver.h"
#include "varupdatehelper.h"
#include <iostream>
#include <assert.h>
#include <iomanip>

using namespace CMSat;
using std::make_pair;
using std::cout;
using std::endl;

//#define VERBOSE_DEBUG

PartHandler::PartHandler(Solver* _solver) :
    solver(_solver)
    , partFinder(NULL)
{
}

PartHandler::~PartHandler()
{
    if (partFinder != NULL) {
        delete partFinder;
    }
}

bool PartHandler::handle()
{
    partFinder = new PartFinder(solver);
    partFinder->findParts();
    const uint32_t num_parts = partFinder->getReverseTable().size();

    //If there is only one big part, we can't do anything
    if (num_parts <= 1) {
        if (solver->conf.verbosity >= 2) {
            cout
            << "c Only one part, not handling it separately"
            << endl;
        }
        return true;
    }

    map<uint32_t, vector<Var> > reverseTable = partFinder->getReverseTable();
    assert(num_parts == partFinder->getReverseTable().size());

    //Get the sizes now
    vector<pair<uint32_t, uint32_t> > sizes;
    for (map<uint32_t, vector<Var> >::iterator
        it = reverseTable.begin()
        ; it != reverseTable.end()
        ; it++
    ) {
        sizes.push_back(make_pair(
            it->first //Part number
            , (uint32_t)it->second.size() //Size of the table
        ));
    }

    //Sort according to smallest size first
    std::sort(sizes.begin(), sizes.end(), sort_pred());
    assert(sizes.size() > 1);

    for (uint32_t it = 0; it < sizes.size()-1; it++) {
        //What are we solving?
        const uint32_t part = sizes[it].first;
        vector<Var> vars = reverseTable[part];
        if (solver->conf.verbosity >= 1) {
            cout
            << "c Solving part " << it
            << " num vars: " << vars.size()
            << " ======================================="
            << endl;
        }

        SolverConf conf;
        Solver newSolver(conf);
        configureNewSolver(&newSolver);
        moveVariablesBetweenSolvers(&newSolver, vars, part);

        moveClausesImplicit(&newSolver, part);
        moveClausesLong(solver->longIrredCls, &newSolver, part);
        moveClausesLong(solver->longRedCls, &newSolver, part);
        //assert(checkClauseMovement(newSolver, part));

        lbool status = newSolver.solve();
        assert(status != l_Undef);
        if (status == l_False) {
            solver->ok = false;
            if (solver->conf.verbosity >= 2) {
                cout
                << "c One of the sub-problems was UNSAT -> problem is unsat."
                << endl;
            }
            return false;
        }

        //Check that the newly found solution is really unassigned in the
        //original solver
        for (size_t i = 0; i < vars.size(); i++) {
            Var var = vars[i];
            if (newSolver.model[var] != l_Undef) {
                assert(solver->assigns[var] == l_Undef);
            }
        }

        //Move decision level 0 vars over
        assert(newSolver.decisionLevel() == 0);
        assert(solver->decisionLevel() == 0);
         for (size_t i = 0; i < vars.size(); i++) {
            Var var = vars[i];
            lbool val = newSolver.value(var);
            if (val != l_Undef) {
                Lit lit(var, val == l_False);
                solver->enqueue(lit);

                //These vars are not meant to be in the orig solver
                //so they cannot cause UNSAT
                solver->ok = (solver->propagate().isNULL());
                assert(solver->ok);
            }
        }

        //Save the solution as savedState
        for (size_t i = 0; i < vars.size(); i++) {
            Var var = vars[i];
            if (newSolver.model[var] != l_Undef) {
                assert(savedState[var] == l_Undef);
                assert(partFinder->getVarPart(var) == part);

                savedState[var] = newSolver.model[var];
            }
        }

        if (solver->conf.verbosity  >= 1) {
            cout
            << "c Solved part " << it
            << " ======================================="
            << endl;
        }
    }

    //Coming back to the original instance now
    if (solver->conf.verbosity  >= 1) {
        cout
        << "c Coming back to original instance"
        << " ======================================="
        << endl;
    }

    //Filter out the variables that have been made non-decision
    solver->filterOrderHeap();

    //Checking that all variables that are not in the remaining part are all
    //non-decision vars, and none have been assigned
    for (Var var = 0; var < solver->nVars(); var++) {
        if (savedState[var] != l_Undef) {
            assert(solver->decisionVar[var] == false);
            assert(solver->assigns[var] == l_Undef || solver->varData[var].level == 0);
        }
    }

    //Checking that all remaining clauses contain only variables
    //that are in the remaining part
    //assert(checkClauseMovement(solver, sizes[sizes.size()-1].first));

    delete partFinder;
    partFinder = NULL;
    return true;
}

/**
@brief Sets up the sub-solver with a specific configuration
*/
void PartHandler::configureNewSolver(Solver* newSolver) const
{
    newSolver->conf = solver->conf;
    newSolver->mtrand.seed(solver->mtrand.randInt());

    //Don't recurse, please
    newSolver->conf.doPartHandler = false;
}

/**
@brief Moves the variables to the new solver

This implies making the right variables decision in the new solver,
and making it non-decision in the old solver.
*/
void PartHandler::moveVariablesBetweenSolvers(
    Solver* newSolver
    , vector<Var>& vars
    , const uint32_t part
) {
    std::sort(vars.begin(), vars.end());
    uint32_t i2 = 0;
    for (Var var = 0; var < solver->nVars(); var++) {
        //Inside this part?
        if (i2 < vars.size() && vars[i2] == var) {

            //Yes, inside, so make it decision
            #ifdef VERBOSE_DEBUG
            if (!solver->decisionVar[var]) {
                cout
                << "var " << var + 1
                << " is non-decision, but in part... strange."
                << endl;
            }
            #endif //VERBOSE_DEBUG
            newSolver->newVar(solver->decisionVar[var]);
            assert(partFinder->getVarPart(var) == part);
            if (solver->decisionVar[var]) {
                solver->unsetDecisionVar(var);
                decisionVarRemoved.push_back(var);
            }
            i2++;
        } else {
            //No, not inside, so make it non-decision
            assert(partFinder->getVarPart(var) != part);
            newSolver->newVar(false);
        }
    }

    //Update order_heap to remove variables set to non-decision
    solver->filterOrderHeap();
}

void PartHandler::moveClausesLong(
    vector<ClOffset>& cs
    , Solver* newSolver
    , const uint32_t part
) {
    vector<Lit> tmp;

    vector<ClOffset>::iterator i, j, end;
    for (i = j = cs.begin(), end = cs.end()
        ; i != end
        ; i++
    ) {
        Clause& cl = *solver->clAllocator->getPointer(*i);

        //Irred, different part
        if (!cl.learnt()) {
            if (partFinder->getVarPart(cl[0].var()) != part) {
                //different part, move along
                *j++ = *i;
                continue;
            }
        }

        if (cl.learnt()) {
            //Check which part(s) it belongs to
            bool thisPart = false;
            bool otherPart = false;
            for (Lit* l = cl.begin(), *end2 = cl.end(); l != end2; l++) {
                if (partFinder->getVarPart(l->var()) == part)
                    thisPart = true;

                if (partFinder->getVarPart(l->var()) != part)
                    otherPart = true;
            }

            //In both parts, remove it
            if (thisPart && otherPart) {
                solver->detachClause(cl);
                solver->clAllocator->clauseFree(&cl);
                continue;
            }

            //In one part, but not this one
            if (!thisPart) {
                //different part, move along
                *j++ = *i;
                continue;
            }
            assert(thisPart && !otherPart);
        }

        //Let's move it to the other solver!
        #ifdef VERBOSE_DEBUG
        cout << "clause in this part:" << cl << endl;;
        #endif

        tmp.resize(cl.size());
        std::copy(cl.begin(), cl.end(), tmp.begin());
        if (cl.learnt()) {
            cl.stats.conflictNumIntroduced = 0;
            newSolver->addLearntClause(tmp, cl.stats);
        } else {
            newSolver->addClause(tmp);
        }

        //Remove from here
        solver->detachClause(cl);
        solver->clAllocator->clauseFree(&cl);
    }
    cs.resize(cs.size() - (i-j));
}

void PartHandler::moveClausesImplicit(
    Solver* newSolver
    , const uint32_t part
) {
    vector<Lit> lits;
    uint32_t numRemovedHalfNonLearnt = 0;
    uint32_t numRemovedHalfLearnt = 0;
    uint32_t numRemovedThirdNonLearnt = 0;
    uint32_t numRemovedThirdLearnt = 0;

    uint32_t wsLit = 0;
    for (vector<vec<Watched> >::iterator
        it = solver->watches.begin(), end = solver->watches.end()
        ; it != end
        ; it++, wsLit++
    ) {
        const Lit lit = Lit::toLit(wsLit);
        vec<Watched>& ws = *it;

        Watched *i = ws.begin();
        Watched *j = i;
        for (Watched *end2 = ws.end()
            ; i != end2
            ; i++
        ) {
            //At least one variable inside part
            if (i->isBinary()
                && (partFinder->getVarPart(lit.var()) == part
                    || partFinder->getVarPart(i->lit1().var()) == part
                )
            ) {
                const Lit lit2 = i->lit1();

                //Unless learnt, cannot be in 2 parts at once
                assert((partFinder->getVarPart(lit.var()) == part
                            && partFinder->getVarPart(lit2.var()) == part
                       ) || i->learnt()
                );

                //If it's learnt and the lits are in different parts, remove it.
                if (partFinder->getVarPart(lit.var()) != part
                    || partFinder->getVarPart(lit2.var()) != part
                ) {
                    assert(i->learnt());
                    numRemovedHalfLearnt++;
                    continue;
                }

                //don't add the same clause twice
                if (lit < lit2) {

                    //Add clause
                    lits.resize(2);
                    lits[0] = lit;
                    lits[1] = lit2;
                    assert(partFinder->getVarPart(lit.var()) == part);
                    assert(partFinder->getVarPart(lit2.var()) == part);
                    if (i->learnt()) {
                        newSolver->addLearntClause(lits);
                        numRemovedHalfLearnt++;
                    } else {
                        //binClausesRemoved.push_back(BinClause(lit, lit2));
                        newSolver->addClause(lits);
                        numRemovedHalfNonLearnt++;
                    }
                } else {

                    //Just remove, already added above
                    if (i->learnt()) {
                        numRemovedHalfLearnt++;
                    } else {
                        numRemovedHalfNonLearnt++;
                    }
                }

                //Yes, remove
                continue;
            }

            if (i->isTri()
                && (partFinder->getVarPart(lit.var()) == part
                    || partFinder->getVarPart(i->lit1().var()) == part
                    || partFinder->getVarPart(i->lit2().var()) == part
                )
            ) {
                const Lit lit2 = i->lit1();
                const Lit lit3 = i->lit2();

                //Unless learnt, cannot be in 2 parts at once
                assert((partFinder->getVarPart(lit.var()) == part
                            && partFinder->getVarPart(lit2.var()) == part
                            && partFinder->getVarPart(lit3.var()) == part
                       ) || i->learnt()
                );

                //If it's learnt and the lits are in different parts, remove it.
                if (partFinder->getVarPart(lit.var()) != part
                    || partFinder->getVarPart(lit2.var()) != part
                    || partFinder->getVarPart(lit3.var()) != part
                ) {
                    assert(i->learnt());
                    numRemovedThirdLearnt++;
                    continue;
                }

                //don't add the same clause twice
                if (lit < lit2
                    && lit2 < lit3
                ) {

                    //Add clause
                    lits.resize(3);
                    lits[0] = lit;
                    lits[1] = lit2;
                    lits[2] = lit3;
                    assert(partFinder->getVarPart(lit.var()) == part);
                    assert(partFinder->getVarPart(lit2.var()) == part);
                    assert(partFinder->getVarPart(lit3.var()) == part);
                    if (i->learnt()) {
                        newSolver->addLearntClause(lits);
                        numRemovedThirdLearnt++;
                    } else {
                        //triClausesRemoved.push_back(TriClause(lit, lit2, lit3));
                        newSolver->addClause(lits);
                        numRemovedThirdNonLearnt++;
                    }
                } else {

                    //Just remove, already added above
                    if (i->learnt()) {
                        numRemovedThirdLearnt++;
                    } else {
                        numRemovedThirdNonLearnt++;
                    }
                }

                //Yes, remove
                continue;
            }

            *j++ = *i;
        }
        ws.shrink_(i-j);
    }

    assert(numRemovedHalfNonLearnt % 2 == 0);
    solver->binTri.irredBins -= numRemovedHalfNonLearnt/2;
    solver->binTri.irredLits -= numRemovedHalfNonLearnt;

    assert(numRemovedThirdNonLearnt % 3 == 0);
    solver->binTri.irredTris -= numRemovedThirdNonLearnt/3;
    solver->binTri.irredLits -= numRemovedThirdNonLearnt;

    assert(numRemovedHalfLearnt % 2 == 0);
    solver->binTri.redBins -= numRemovedHalfLearnt/2;
    solver->binTri.redLits -= numRemovedHalfLearnt;

    assert(numRemovedThirdLearnt % 3 == 0);
    solver->binTri.redTris -= numRemovedThirdLearnt/3;
    solver->binTri.redLits -= numRemovedThirdLearnt;
}

void PartHandler::addSavedState(vector<lbool>& model)
{
    //Add them to model
    for (size_t var = 0; var < savedState.size(); var++) {
        if (savedState[var] != l_Undef) {
            model[var] = savedState[var];
            solver->varData[var].polarity = (savedState[var] == l_True);
        }
    }

    //Re-set them to be decision vars
    for (size_t i = 0; i < decisionVarRemoved.size(); i++) {
        solver->setDecisionVar(decisionVarRemoved[i]);
    }
    decisionVarRemoved.clear();
}

void PartHandler::updateVars(const vector<Var>& interToOuter)
{
    updateArray(savedState, interToOuter);
}

/*void PartHandler::readdRemovedClauses()
{
    FILE* backup_libraryCNFfile = solver.libraryCNFFile;
    solver.libraryCNFFile = NULL;
    for (Clause **it = clausesRemoved.getData(), **end = clausesRemoved.getDataEnd(); it != end; it++) {
        solver.addClause(**it, (*it)->getGroup());
        assert(solver.ok);
    }
    clausesRemoved.clear();

    for (XorClause **it = xorClausesRemoved.getData(), **end = xorClausesRemoved.getDataEnd(); it != end; it++) {
        solver.addXorClause(**it, (**it).xorEqualFalse(), (*it)->getGroup());
        assert(solver.ok);
    }
    xorClausesRemoved.clear();

    for (vector<pair<Lit, Lit> >::const_iterator it = binClausesRemoved.begin(), end = binClausesRemoved.end(); it != end; it++) {
        vec<Lit> lits(2);
        lits[0] = it->first;
        lits[1] = it->second;
        solver.addClause(lits);
        assert(solver.ok);
    }
    binClausesRemoved.clear();

    solver.libraryCNFFile = backup_libraryCNFfile;
}

const bool PartHandler::checkClauseMovement(
    const Solver& thisSolver
    , const uint32_t part
) const {
    if (!checkOnlyThisPartLong(thisSolver.longIrredCls, part))
        return false;

    if (!checkOnlyThisPartLong(thisSolver.longRedCls, part))
        return false;

    if (!checkOnlyThisPartImplicit(thisSolver, part))
        return false;

    return true;
}


template<class T>
const bool PartHandler::checkOnlyThisPart(const vec<T*>& cs, const uint32_t part, const PartFinder& partFinder) const
{
    for(T * const*it = cs.getData(), * const*end = it + cs.size(); it != end; it++) {
        const T& c = **it;
        for(const Lit *l = c.getData(), *end2 = l + c.size(); l != end2; l++) {
            if (partFinder->getVarPart(l->var()) != part) return false;
        }
    }

    return true;
}

const bool PartHandler::checkOnlyThisPartBin(const Solver& thisSolver, const uint32_t part, const PartFinder& partFinder) const
{
    bool retval = true;
    uint32_t wsLit = 0;
    for (const vec<Watched> *it = thisSolver.watches.getData(), *end = thisSolver.watches.getDataEnd(); it != end; it++, wsLit++) {
        const Lit lit = ~Lit::toLit(wsLit);
        const vec<Watched>& ws = *it;
        for (const Watched *it2 = ws.getData(), *end2 = ws.getDataEnd(); it2 != end2; it2++) {
            if (it2->isBinary()) {
                if (partFinder->getVarPart(lit.var()) != part
                    || partFinder->getVarPart(it2->getOtherLit().var()) != part
                    ) {
                    cout << "bin incorrectly moved to this part:" << lit << " , " << it2->getOtherLit() << endl;
                    retval = false;
                }
            }
        }
    }

    return retval;
}*/

