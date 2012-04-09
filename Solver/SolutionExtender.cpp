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

#include "SolutionExtender.h"
#include "VarReplacer.h"
#include "Subsumer.h"
#include "ThreadControl.h"
using std::cout;
using std::endl;

SolutionExtender::SolutionExtender(ThreadControl* _control, const vector<lbool>& _assigns) :
    control(_control)
    , occur(_control->nVars()*2)
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

    if (control->conf.verbosity >= 3) {
        cout << "c Extending solution" << endl;
    }

    /*if (greedyUnbound) {
        double time = cpuTime();
        FindUndef finder(*this);
        const uint32_t unbounded = finder.unRoll();
        if (conf.verbosity >= 1)
            cout << "c Greedy unbounding     : " << (cpuTime()-time) << ", unbounded: " << unbounded << " vars" << endl;
    }*/

    //Sanity check
    control->subsumer->checkElimedUnassignedAndStats();

    for (vector<Clause*>::iterator it = control->clauses.begin(), end = control->clauses.end(); it != end; it++) {
        Clause& cl = **it;
        assert(!cl.learnt());
        vector<Lit> tmp;
        for (uint32_t i = 0; i < cl.size(); i++) tmp.push_back(cl[i]);
        const bool OK = addClause(tmp);
        assert(OK);
    }

    uint32_t wsLit = 0;
    for (vector<vec<Watched> >::const_iterator it = control->watches.begin(), end = control->watches.end(); it != end; it++, wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        const vec<Watched>& ws = *it;
        for (vec<Watched>::const_iterator it2 = ws.begin(), end2 = ws.end(); it2 != end2; it2++) {
            if (it2->isNonLearntBinary()) {
                vector<Lit> tmp;
                tmp.push_back(lit);
                tmp.push_back(it2->getOtherLit());
                const bool OK = addClause(tmp);
                assert(OK);
            }
        }
    }

    if (control->conf.verbosity >= 3) {
        cout << "c Adding equivalent literals" << endl;
    }
    control->varReplacer->extendModel(this);

    if (control->conf.verbosity >= 3) {
        cout << "c Picking braches and propagating" << endl;
    }
    while(pickBranchLit() != lit_Undef) {
        const bool OK = propagate();
        if (!OK) {
            cout << "Error! While picking lit and propagating after solution reconstruction" << endl;
            exit(-1);
        }
    }

    if (control->conf.verbosity >= 3) {
        cout << "c Adding blocked clauses" << endl;
    }
    control->subsumer->extendModel(this);

    //Copy&check model
    control->model.resize(nVars(), l_Undef);
    for (Var var = 0; var != nVars(); var++) {
        control->model[var] = value(var);
    }

    release_assert(control->verifyModel());

    //free clauses
    for (vector<MyClause*>::iterator it = clauses.begin(), end = clauses.end(); it != end; it++) {
        delete *it;
    }
}

bool SolutionExtender::satisfiedNorm(const vector<Lit>& lits) const
{
    for (vector<Lit>::const_iterator it = lits.begin(), end = lits.end(); it != end; it++)
        if (value(*it) == l_True) return true;
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

void SolutionExtender::addBlockedClause(const BlockedClause& cl)
{
    assert(qhead == trail.size());
    if (control->conf.verbosity >= 3) {
        cout << "c Adding blocked clause: " << cl << endl;
    }

    vector<Lit> lits = cl.lits;
    Lit blockedOn = cl.blockedOn;

    addClause(lits);
    if (satisfiedNorm(lits)) return;

    uint32_t numUndef = 0;
    for (uint32_t i = 0; i < lits.size(); i++) {
        if (value(lits[i]) == l_Undef) numUndef++;
    }
    if (numUndef != 0) return;

    //Nothing is UNDEF and it's not satisfied!
    assert(value(blockedOn) == l_False);
    #ifdef VERBOSE_DEBUG_RECONSTRUCT
    cout << "c recursively flipping to " << blockedOn << endl;
    #endif
    assert(control->varData[blockedOn.var()].level != 0); // we cannot flip forced vars!!
    enqueue(blockedOn);
    //flip forward equiv
    if (control->varReplacer->getReplaceTable()[blockedOn.var()].var() != blockedOn.var()) {
        blockedOn = control->varReplacer->getReplaceTable()[blockedOn.var()] ^ blockedOn.sign();
        enqueue(Lit(blockedOn.var(), value(blockedOn.var()) == l_True));
    }
    //flip backward equiv
    map<Var, vector<Var> >::const_iterator revTable = control->varReplacer->getReverseTable().find(blockedOn.var());
    if (revTable != control->varReplacer->getReverseTable().end()) {
        const vector<Var>& toGoThrough = revTable->second;
        for (uint32_t i = 0; i < toGoThrough.size(); i++) {
            enqueue(Lit(toGoThrough[i], value(toGoThrough[i]) == l_True));
        }
    }
    #ifdef VERBOSE_DEBUG_RECONSTRUCT
    cout << "c recursive flip(s) done." << endl;
    #endif

    //Propagate&check, see what happens
    bool OK = propagate();
    if (!OK) {
        cout << "Error! Propagation leads to failure after flipping of value" << endl;
        exit(-1);
    }
}

bool SolutionExtender::addClause(const std::vector< Lit >& givenLits)
{
    vector<Lit> lits = givenLits;
    vector<Lit>::iterator i = lits.begin();
    vector<Lit>::iterator j = i;
    for (vector<Lit>::iterator end = lits.end(); i != end; i++) {
        if (value(*i) == l_True && control->varData[i->var()].level == 0) {
            return true;
        }

        if (value(*i) == l_False && control->varData[i->var()].level == 0) {
            continue;
        }

        *j++ = *i;
    }
    lits.resize(lits.size()-(i-j));

    #ifdef VERBOSE_DEBUG_RECONSTRUCT
    cout << "c Adding extend clause: " << lits << endl;
    #endif

    if (lits.size() == 0) return false;

    MyClause* cl = new MyClause(lits);
    clauses.push_back(cl);
    for (vector<Lit>::const_iterator it = lits.begin(), end = lits.end(); it != end; it++)
    {
        occur[it->toInt()].push_back(cl);
    }

    const bool OK = propagateCl(*cl);
    if (!OK || !propagate()) return false;
    return true;
}

bool SolutionExtender::propagate()
{
    bool ret = true;
    while(qhead < trail.size()) {
        const Lit p = trail[qhead++];
        const vector<MyClause*>& occ = occur[(~p).toInt()];
        for(vector<MyClause*>::const_iterator it = occ.begin(), end = occ.end(); it != end; it++) {
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
        if (numUndef > 1) break;
        lastUndef = *it;
    }
    if (numUndef == 1) {
        #ifdef VERBOSE_DEBUG_RECONSTRUCT
        cout << "c Due to cl " << cl.getLits() << " propagate enqueueing " << lastUndef << endl;
        #endif
        enqueue(lastUndef);
    }
    if (numUndef >= 1) return true;

    assert(numUndef == 0);
    return false;
}

Lit SolutionExtender::pickBranchLit()
{
    for (Var var = 0; var < nVars(); var++) {
        if (value(var) == l_Undef) {
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
    control->varData[lit.var()].level = std::numeric_limits< uint32_t >::max();
}






