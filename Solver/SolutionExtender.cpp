/******************************************************************************
CryptoMiniSat -- Copyright (c) 2011 Mate Soos

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
******************************************************************************/

#include "SolutionExtender.h"
#include "PartHandler.h"
#include "VarReplacer.h"
#include "Subsumer.h"
#include "XorSubsumer.h"


SolutionExtender::SolutionExtender(Solver& _solver) :
    solver(_solver)
{
}

/**
@brief Extends a SAT solution to the full solution

variable elimination, variable replacement, sub-part solving, etc. all need to
be handled correctly to arrive at a solution that is a solution to ALL of the
original problem, not just of what remained of it at the end inside this class
(i.e. we need to combine things from the helper classes)
*/
void SolutionExtender::extend()
{

    /*if (greedyUnbound) {
        double time = cpuTime();
        FindUndef finder(*this);
        const uint32_t unbounded = finder.unRoll();
        if (conf.verbosity >= 1)
            printf("c Greedy unbounding     :%5.2lf s, unbounded: %7d vars\n", cpuTime()-time, unbounded);
    }*/

    occur.clear();
    occur.resize(solver.nVars()*2);

    assert(solver.subsumer->checkElimedUnassigned());
    assert(solver.xorSubsumer->checkElimedUnassigned());

    for (Clause **it = solver.clauses.getData(), **end = solver.clauses.getDataEnd(); it != end; it++) {
        Clause& clOrig = **it;
        assert(!clOrig.learnt());
        MyClause* cl = new MyClause(clOrig);
        clauses.push_back(cl);
        for (vector<Lit>::const_iterator l = cl->begin(), end2 = cl->end(); l != end2; l++) {
            occur[l->toInt()].push_back(cl);
        }
    }


    uint32_t wsLit = 0;
    for (const vec2<Watched> *it = solver.watches.getData(), *end = solver.watches.getDataEnd(); it != end; it++, wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        const vec2<Watched>& ws = *it;
        for (vec2<Watched>::const_iterator it2 = ws.getData(), end2 = ws.getDataEnd(); it2 != end2; it2++) {
            if (it2->isNonLearntBinary()) {
                MyClause* cl = new MyClause(lit, it2->getOtherLit());
                clauses.push_back(cl);
                occur[lit.toInt()].push_back(cl);
                occur[it2->getOtherLit().toInt()].push_back(cl);
            }
        }
    }

    solver.partHandler->addSavedState();
    solver.varReplacer->extendModelPossible();

    solver.varReplacer->extendModelImpossible(this);
    solver.subsumer->extendModel(this);
    solver.xorSubsumer->extendModel(this);
    propagate();
    while(pickBranchLit() != lit_Undef) propagate();

    solver.checkSolution();

    //Copy model:
    solver.model.growTo(solver.nVars());
    for (Var var = 0; var != solver.nVars(); var++) solver.model[var] = solver.value(var);

    //free clauses
    for (vector<MyClause*>::iterator it = clauses.begin(), end = clauses.end(); it != end; it++) {
        delete *it;
    }
}


void SolutionExtender::addClause(const std::vector< Lit >& givenLits)
{
    bool satisfied = false;

    vector<Lit> lits = givenLits;
    vector<Lit>::iterator i = lits.begin();
    vector<Lit>::iterator j = i;
    for (vector<Lit>::iterator end = lits.end(); i != end; i++) {
        if (solver.value(*i) == l_True && solver.varData[i->var()].level == 0) {
            return;
        }

        if (solver.value(*i) == l_False && solver.varData[i->var()].level == 0) {
            continue;
        }

        *j++ = *i;
    }
    lits.resize(lits.size()-(i-j));
    if (satisfied) return;

    if (lits.size() == 1) {
        solver.uncheckedEnqueue(lits[0]);
        return;
    }

    assert(lits.size() != 0);
    MyClause* cl = new MyClause(lits, false);
    clauses.push_back(cl);
    for (vector<Lit>::const_iterator it = lits.begin(), end = lits.end(); it != end; it++)
    {
        occur[it->toInt()].push_back(cl);
    }
}

void SolutionExtender::addXorClause(const vector<Lit>& lits, const bool xorEqualFalse)
{
    assert(false);
}

void SolutionExtender::propagate()
{
    while(solver.qhead < solver.trail.size()) {
        const Lit p = solver.trail[solver.qhead++];
        const vector<MyClause*>& occ = occur[(~p).toInt()];
        for(vector<MyClause*>::const_iterator it = occ.begin(), end = occ.end(); it != end; it++) {
            propagateCl(**it);
        }
    }
}

void SolutionExtender::propagateCl(MyClause& cl)
{
    //Don't handle XOR yet
    assert(!cl.getXor());

    size_t numUndef = 0;
    Lit lastUndef = lit_Undef;
    for (vector<Lit>::const_iterator it = cl.begin(), end = cl.end(); it != end; it++)
    {
        if (solver.value(*it) == l_True) return;
        if (solver.value(*it) == l_False) continue;

        assert(solver.value(*it) == l_Undef);
        numUndef++;
        if (numUndef > 1) break;
        lastUndef = *it;
    }
    if (numUndef == 1) solver.uncheckedEnqueue(lastUndef);
}

const Lit SolutionExtender::pickBranchLit()
{
    for (Var var = 0; var < solver.nVars(); var++) {
        if (solver.value(var) == l_Undef) solver.uncheckedEnqueue(Lit(var, false));
    }
    return lit_Undef;
}








