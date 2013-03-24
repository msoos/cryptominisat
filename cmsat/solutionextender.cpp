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

#include "solutionextender.h"
#include "varreplacer.h"
#include "simplifier.h"
#include "solver.h"
using namespace CMSat;
using std::cout;
using std::endl;

SolutionExtender::SolutionExtender(Solver* _solver, const vector<lbool>& _assigns) :
    solver(_solver)
    , occur(_solver->nVars()*2)
    , qhead (0)
    , assigns(_assigns)
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

    if (solver->conf.verbosity >= 3) {
        cout << "c Extending solution" << endl;
    }

    //Sanity check
    solver->simplifier->checkElimedUnassignedAndStats();

    //Temporary
    vector<Lit> tmp;

    size_t wsLit = 0;
    for (vector<vec<Watched> >::const_iterator
        it = solver->watches.begin(), end = solver->watches.end()
        ; it != end
        ; it++, wsLit++
    ) {
        Lit lit = Lit::toLit(wsLit);
        const vec<Watched>& ws = *it;
        for (vec<Watched>::const_iterator
            it2 = ws.begin(), end2 = ws.end()
            ; it2 != end2
            ; it2++
        ) {
            //Binary clauses
            if (it2->isBinary()
                //Only irred
                && !it2->learnt()
                //Only add each bin once
                && lit < it2->lit1()
            ) {
                tmp.clear();
                tmp.push_back(lit);
                tmp.push_back(it2->lit1());
                const bool OK = addClause(tmp);
                assert(OK);
            }

            //Tertiary clauses
            if (it2->isTri()
                //Only irred
                && !it2->learnt()
                //Each tri only once
                && lit < it2->lit1()
                && it2->lit1() < it2->lit2()
            ) {
                tmp.clear();
                tmp.push_back(lit);
                tmp.push_back(it2->lit1());
                tmp.push_back(it2->lit2());
                const bool OK = addClause(tmp);
                assert(OK);
            }
        }
    }

    if (solver->conf.verbosity >= 3) {
        cout << "c Adding equivalent literals" << endl;
    }
    solver->varReplacer->extendModel(this);

    for (vector<ClOffset>::iterator
        it = solver->longIrredCls.begin(), end = solver->longIrredCls.end()
        ; it != end
        ; it++
    ) {
        Clause& cl = *solver->clAllocator->getPointer(*it);
        assert(!cl.learnt());

        //Add clause to our local system
        tmp.clear();
        for (uint32_t i = 0; i < cl.size(); i++)
            tmp.push_back(cl[i]);
        const bool OK = addClause(tmp);
        assert(OK);
    }

    if (solver->conf.verbosity >= 3) {
        cout << "c Adding blocked clauses" << endl;
    }
    solver->simplifier->extendModel(this);

    while(pickBranchLit() != lit_Undef) {
        const bool OK = propagate();
        if (!OK) {
            cout << "Error! While picking lit and propagating after solution reconstruction" << endl;
            exit(-1);
        }
    }

    if (solver->conf.verbosity >= 3) {
        cout << "c Picking braches and propagating" << endl;
    }

    //Copy&check model
    solver->model.resize(nVars(), l_Undef);
    for (Var var = 0; var != nVars(); var++) {
        solver->model[var] = value(var);
    }

    release_assert(solver->verifyModel());

    //free clauses
    for (vector<MyClause*>::iterator it = clauses.begin(), end = clauses.end(); it != end; it++) {
        delete *it;
    }
}

bool SolutionExtender::satisfiedNorm(const vector<Lit>& lits) const
{
    for (vector<Lit>::const_iterator
        it = lits.begin(), end = lits.end()
        ; it != end
        ; it++
    ) {
        if (value(*it) == l_True)
            return true;
    }

    return false;
}

bool SolutionExtender::satisfiedXor(const vector<Lit>& lits, const bool rhs) const
{
    bool val = false;
    uint32_t undef = 0;
    for (vector<Lit>::const_iterator it = lits.begin(), end = lits.end(); it != end; it++) {
        assert(it->unsign() == *it);
        if (value(it->var()) == l_True) val ^= true;
        if (value(it->var()) == l_Undef) undef++;
    }
    return (undef > 0 || val == rhs);
}

bool SolutionExtender::addClause(
    const vector< Lit >& givenLits
    , const Lit blockedOn
) {
    vector<Lit> lits = givenLits;

    //Remove lits set at 0-level or return TRUE if any is set to TRUE at 0-level
    vector<Lit>::iterator i = lits.begin();
    vector<Lit>::iterator j = i;
    for (vector<Lit>::iterator end = lits.end(); i != end; i++) {
        if (value(*i) == l_True && solver->varData[i->var()].level == 0) {
            return true;
        }

        if (value(*i) == l_False && solver->varData[i->var()].level == 0) {
            continue;
        }

        *j++ = *i;
    }
    lits.resize(lits.size()-(i-j));

    #ifdef VERBOSE_DEBUG_RECONSTRUCT
    cout << "c Adding extend clause: " << lits << " blocked on: " << blockedOn << endl;
    #endif

    //Empty clause, oops!
    if (lits.size() == 0)
        return false;

    //Create new clause, and add it
    MyClause* cl = new MyClause(lits, blockedOn);
    clauses.push_back(cl);
    for (vector<Lit>::const_iterator
        it = lits.begin(), end = lits.end()
        ; it != end
        ; it++
    ) {
        occur[it->toInt()].push_back(cl);
    }

    propagateCl(*cl);
    if (!propagate()) {
        assert(false);
        return false;
    }

    return true;
}

bool SolutionExtender::propagate()
{
    bool ret = true;
    while(qhead < trail.size()) {
        const Lit p = trail[qhead++];
        const vector<MyClause*>& occ = occur[(~p).toInt()];
        for(vector<MyClause*>::const_iterator
            it = occ.begin(), end = occ.end()
            ; it != end
            ; it++
        ) {
            const bool thisRet = propagateCl(**it);
            if (!thisRet) {
                cout << "Problem with clause: " << (*it)->getLits() << endl;
            }
            ret &= thisRet;
        }
    }

    return ret;
}

bool SolutionExtender::propagateCl(MyClause& cl)
{
    size_t numUndef = 0;
    Lit lastUndef = lit_Undef;
    for (vector<Lit>::const_iterator it = cl.begin(), end = cl.end(); it != end; it++)
    {
        if (value(*it) == l_True) return true;
        if (value(*it) == l_False) continue;

        assert(value(*it) == l_Undef);
        numUndef++;

        //Doesn't propagate anything
        if (numUndef > 1)
            break;

        lastUndef = *it;
    }

    //Must set this one value
    if (numUndef == 1) {
        #ifdef VERBOSE_DEBUG_RECONSTRUCT
        cout << "c Due to cl " << cl.getLits() << " propagate enqueueing " << lastUndef << endl;
        #endif
        enqueue(lastUndef);
    }

    if (numUndef >= 1)
        return true;

    //Must flip
    #ifdef VERBOSE_DEBUG_RECONSTRUCT
    cout
    << "Flipping lit " << cl.blockedOn
    << " due to clause " << cl.lits << endl;
    #endif
    assert(cl.blockedOn != lit_Undef);

    assert(solver->varData[cl.blockedOn.var()].level != 0
        && "We cannot flip 0-level vars"
    );
    enqueue(cl.blockedOn);
    return true;
}

Lit SolutionExtender::pickBranchLit()
{
    for (Var var = 0; var < nVars(); var++) {
        if (value(var) == l_Undef
            //Don't pick replaced variables
            && solver->varData[var].elimed != ELIMED_VARREPLACER
        ) {
            Lit toEnqueue = Lit(var, false);
            #ifdef VERBOSE_DEBUG_RECONSTRUCT
            cout << "c Picking lit for reconstruction: " << toEnqueue << endl;
            #endif
            enqueue(toEnqueue);
            return toEnqueue;
        }
    }
    return lit_Undef;
}

void SolutionExtender::enqueue(const Lit lit)
{
    assigns[lit.var()] = boolToLBool(!lit.sign());
    trail.push_back(lit);
    #ifdef VERBOSE_DEBUG_RECONSTRUCT
    cout << "c Enqueueing lit " << lit << " during solution reconstruction" << endl;
    #endif
    solver->varData[lit.var()].level = std::numeric_limits< uint32_t >::max();
}






