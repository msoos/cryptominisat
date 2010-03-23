/***********************************************************************************
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************************/

#include "PartHandler.h"
#include "VarReplacer.h"
#include <iostream>
#include <assert.h>

//#define VERBOSE_DEBUG

PartHandler::PartHandler(Solver& s) :
    solver(s)
{
}


const bool PartHandler::handle()
{
    if (solver.performReplace == false)
        return true;
    
    PartFinder partFinder(solver);
    if (!partFinder.findParts())
        return false;
    
    uint32_t num_parts = partFinder.getReverseTable().size();
    if (num_parts == 1)
        return true;
    
    map<uint32_t, vector<Var> > reverseTable = partFinder.getReverseTable();
    assert(num_parts == partFinder.getReverseTable().size());
    
    vector<pair<uint32_t, uint32_t> > sizes;
    for (map<uint32_t, vector<Var> >::iterator it = reverseTable.begin(); it != reverseTable.end(); it++)
        sizes.push_back(std::make_pair(it->first, it->second.size()));
    
    std::sort(sizes.begin(), sizes.end(), sort_pred());
    assert(sizes.size() > 1);
    
    for (uint32_t it = 0; it < sizes.size()-1; it++) {
        uint32_t part = sizes[it].first;
        vector<Var> vars = reverseTable[part];
        std::cout << "c Solving part " << part << std::endl;
        
        Solver newSolver;
        newSolver.mtrand.seed(solver.mtrand.randInt());
        newSolver.random_var_freq = solver.random_var_freq;
        newSolver.var_decay = solver.var_decay;
        newSolver.verbosity = solver.verbosity;
        newSolver.restrictedPickBranch = solver.restrictedPickBranch;
        newSolver.greedyUnbound = solver.greedyUnbound;
        newSolver.findNormalXors = solver.findNormalXors;
        newSolver.findBinaryXors = solver.findBinaryXors;
        newSolver.regularlyFindBinaryXors = solver.regularlyFindBinaryXors;
        newSolver.conglomerateXors = solver.conglomerateXors;
        newSolver.schedSimplification = solver.schedSimplification;
        newSolver.performReplace = solver.performReplace;
        newSolver.failedVarSearch = solver.failedVarSearch;
        newSolver.gaussconfig.dontDisable = solver.gaussconfig.dontDisable;
        newSolver.heuleProcess = solver.heuleProcess;
        newSolver.doSubsumption = solver.doSubsumption;
        newSolver.doPartHandler = solver.doPartHandler;
        newSolver.fixRestartType = solver.fixRestartType;
        newSolver.var_inc = solver.var_inc;
        newSolver.polarity_mode = Solver::polarity_manual;
        std::sort(vars.begin(), vars.end());
        uint32_t i2 = 0;
        for (uint32_t var = 0; var < solver.nVars(); var++) {
            if (i2 < vars.size() && vars[i2] == var) {
                newSolver.newVar(true);
                newSolver.activity[var] = solver.activity[var];
                newSolver.defaultPolarities[var] = solver.polarity[var];
                newSolver.order_heap.update(var);
                assert(partFinder.getVarPart(var) == part);
                solver.setDecisionVar(var, false);
                i2++;
            } else {
                assert(partFinder.getVarPart(var) != part);
                newSolver.newVar(false);
            }
        }
        solver.order_heap.filter(Solver::VarFilter(solver));
        
        assert(solver.varReplacer->getClauses().size() == 0);
        moveClauses(solver.clauses, newSolver, part, partFinder);
        moveClauses(solver.binaryClauses, newSolver, part, partFinder);
        moveClauses(solver.xorclauses, newSolver, part, partFinder);
        moveLearntClauses(solver.binaryClauses, newSolver, part, partFinder);
        moveLearntClauses(solver.learnts, newSolver, part, partFinder);
        
        lbool status = newSolver.solve();
        if (status == l_False)
            return false;
        assert(status != l_Undef);
        
        for (uint32_t i = 0; i < newSolver.nVars(); i++) {
            if (newSolver.model[i] != l_Undef) {
                assert(savedState[i] == l_Undef);
                savedState[i] = newSolver.model[i];
            }
        }
        
        std::cout << "c Solved part" << std::endl;
    }
    std::cout << "c Coming back to original instance" << std::endl;
    
    return true;
}

void PartHandler::moveClauses(vec<Clause*>& cs, Solver& newSolver, const uint32_t part, PartFinder& partFinder)
{
    Clause **i, **j, **end;
    for (i = j = cs.getData(), j = i , end = i + cs.size(); i != end; i++) {
        if ((**i).learnt() || partFinder.getVarPart((**i)[0].var()) != part) {
            *j++ = *i;
            continue;
        }
        solver.detachClause(**i);
        newSolver.addClause(**i, (**i).getGroup());
        free(*i);
    }
    cs.shrink(i-j);
}

void PartHandler::moveClauses(vec<XorClause*>& cs, Solver& newSolver, const uint32_t part, PartFinder& partFinder)
{
    XorClause **i, **j, **end;
    for (i = j = cs.getData(), end = i + cs.size(); i != end; i++) {
        if (partFinder.getVarPart((**i)[0].var()) != part) {
            *j++ = *i;
            continue;
        }
        solver.detachClause(**i);
        for (uint32_t i2 = 0; i2 < (*i)->size(); i2++)
            (**i)[i2] = (**i)[i2].unsign();
        newSolver.addXorClause(**i, (**i).xor_clause_inverted(), (**i).getGroup());
        free(*i);
    }
    cs.shrink(i-j);
}

void PartHandler::moveLearntClauses(vec<Clause*>& cs, Solver& newSolver, const uint32_t part, PartFinder& partFinder)
{
    Clause **i, **j, **end;
    for (i = j = cs.getData(), end = i + cs.size() ; i != end; i++) {
        if (!(**i).learnt()) {
            *j++ = *i;
            continue;
        }
        
        Clause& c = **i;
        assert(c.size() > 0);
        uint32_t clause_part = partFinder.getVarPart(c[0].var());
        bool removed = false;
        for (const Lit* l = c.getData(), *end = l + c.size(); l != end; l++) {
            if (partFinder.getVarPart(l->var()) != clause_part) {
                #ifdef VERBOSE_DEBUG
                std::cout << "Learnt clause in both parts!" << std::endl;
                #endif
                
                removed = true;
                solver.removeClause(**i);
                break;
            }
        }
        if (removed) continue;
        if (clause_part == part) {
            #ifdef VERBOSE_DEBUG
            std::cout << "Learnt clause in this part!" << std::endl;
            #endif
            
            solver.detachClause(**i);
            newSolver.addLearntClause(c, c.getGroup(), c.activity());
            free(*i);
        } else {
            #ifdef VERBOSE_DEBUG
            std::cout << "Learnt clause in other part!" << std::endl;
            #endif
            
            *j++ = *i;
        }
    }
    cs.shrink(i-j);
}

void PartHandler::addSavedState()
{
    for (uint32_t i = 0; i < savedState.size(); i++) {
        if (savedState[i] != l_Undef) {
            assert(solver.assigns[i] == l_Undef);
            solver.assigns[i] = savedState[i];
        }
    }
}

