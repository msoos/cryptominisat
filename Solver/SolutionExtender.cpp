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
    assigns.resize(solver.nVars(), l_Undef);
    for (uint32_t i = 0; i < assigns.size(); i++) {
        assigns[i] = solver.assigns[i];
    }
    trail.clear();
    qhead = 0;

    assert(solver.subsumer->checkElimedUnassigned());
    assert(solver.xorSubsumer->checkElimedUnassigned());

    for (Clause **it = solver.clauses.getData(), **end = solver.clauses.getDataEnd(); it != end; it++) {
        Clause& cl = **it;
        assert(!cl.learnt());
        vector<Lit> tmp;
        for (uint32_t i = 0; i < cl.size(); i++) tmp.push_back(cl[i]);
        const bool OK = addClause(tmp);
        assert(OK);
    }

    uint32_t wsLit = 0;
    for (const vec2<Watched> *it = solver.watches.getData(), *end = solver.watches.getDataEnd(); it != end; it++, wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        const vec2<Watched>& ws = *it;
        for (vec2<Watched>::const_iterator it2 = ws.getData(), end2 = ws.getDataEnd(); it2 != end2; it2++) {
            if (it2->isNonLearntBinary()) {
                vector<Lit> tmp;
                tmp.push_back(lit);
                tmp.push_back(it2->getOtherLit());
                const bool OK = addClause(tmp);
                assert(OK);
            }
        }
    }

    #ifdef VERBOSE_DEBUG_RECONSTRUCT
    std::cout << "c " << std::endl;
    std::cout << "c Adding saved state" << std::endl;
    #endif
    solver.partHandler->addSavedState(this);

    #ifdef VERBOSE_DEBUG_RECONSTRUCT
    std::cout << "c " << std::endl;
    std::cout << "c Adding xor-elimed vars' clauses" << std::endl;
    #endif
    solver.xorSubsumer->extendModel(this);
    const bool OK = propagate();
    if (!OK) {
        std::cout << "Error! While picking lit and propagating before blocked clause adding" << std::endl;
        exit(-1);
    }

    #ifdef VERBOSE_DEBUG_RECONSTRUCT
    std::cout << "c " << std::endl;
    std::cout << "c Adding equivalent literals" << std::endl;
    #endif
    solver.varReplacer->extendModel(this);

    #ifdef VERBOSE_DEBUG_RECONSTRUCT
    std::cout << "c " << std::endl;
    std::cout << "c Picking braches and propagating" << std::endl;
    #endif
    while(pickBranchLit() != lit_Undef) {
        const bool OK = propagate();
        if (!OK) {
            std::cout << "Error! While picking lit and propagating after solution reconstruction" << std::endl;
            exit(-1);
        }
    }

    #ifdef VERBOSE_DEBUG_RECONSTRUCT
    std::cout << "c " << std::endl;
    std::cout << "c Adding blocked clauses" << std::endl;
    #endif
    solver.subsumer->extendModel(this);

    solver.checkSolution();

    //Copy model:
    solver.model.clear();
    solver.model.growTo(solver.nVars());
    for (Var var = 0; var != solver.nVars(); var++) solver.model[var] = value(var);

    //free clauses
    for (vector<MyClause*>::iterator it = clauses.begin(), end = clauses.end(); it != end; it++) {
        delete *it;
    }
}

const bool SolutionExtender::satisfiedNorm(const vector<Lit>& lits) const
{
    for (vector<Lit>::const_iterator it = lits.begin(), end = lits.end(); it != end; it++)
        if (value(*it) == l_True) return true;
    return false;
}

const bool SolutionExtender::satisfiedXor(const vector<Lit>& lits, const bool rhs) const
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

/*const bool SolutionExtender::replace(vector<Lit>& lits, Lit& blockedOn)
{
    const vector<Lit>& table = solver.varReplacer->getReplaceTable();
    for(vector<Lit>::iterator l = lits.begin(), end = lits.end(); l != end; l++) {
        if (table[l->var()].var() == l->var()) continue;
        if (blockedOn == *l) {
            blockedOn = table[l->var()] ^ l->sign();
        }
        *l = table[l->var()] ^ l->sign();
    }

    std::sort(lits.begin(), lits.end());
    vector<Lit>::iterator i = lits.begin();
    vector<Lit>::iterator j = i;
    Lit last = lit_Undef;
    for(vector<Lit>::const_iterator end = lits.end(); i != end; i++) {
        if (*i == last) continue;
        if (*i == ~last) return true;
        *j++ = *i;
        last = *i;
    }
    lits.resize(lits.size()-(i-j));

    return false;
}*/

void SolutionExtender::addBlockedClause(const BlockedClause& cl)
{
    assert(qhead == trail.size());
    #ifdef VERBOSE_DEBUG_RECONSTRUCT
    std::cout << "c Adding... blocked on lit " << cl.blockedOn << " clause: " << cl.lits << std::endl;
    #endif

    vector<Lit> lits = cl.lits;
    Lit blockedOn = cl.blockedOn;
    /*bool satisfied = replace(lits, blockedOn);
    if (satisfied) {
        #ifdef VERBOSE_DEBUG_RECONSTRUCT
        std::cout << "c blocked is satisfied through replace" << std::endl;
        #endif
        return;
    }
    #ifdef VERBOSE_DEBUG_RECONSTRUCT
    std::cout << "c After replace() : lit=" << blockedOn << " clause=" << lits << std::endl;
    #endif*/

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
    std::cout << "c recursively flipping to " << blockedOn << std::endl;
    #endif
    assert(solver.varData[blockedOn.var()].level != 0); // we cannot flip forced vars!!
    enqueue(blockedOn);
    //flip forward equiv
    if (solver.varReplacer->getReplaceTable()[blockedOn.var()].var() != blockedOn.var()) {
        blockedOn = solver.varReplacer->getReplaceTable()[blockedOn.var()] ^ blockedOn.sign();
        enqueue(Lit(blockedOn.var(), value(blockedOn.var()) == l_True));
    }
    //flip backward equiv
    map<Var, vector<Var> >::const_iterator revTable = solver.varReplacer->getReverseTable().find(blockedOn.var());
    if (revTable != solver.varReplacer->getReverseTable().end()) {
        const vector<Var>& toGoThrough = revTable->second;
        for (uint32_t i = 0; i < toGoThrough.size(); i++) {
            enqueue(Lit(toGoThrough[i], value(toGoThrough[i]) == l_True));
        }
    }
    #ifdef VERBOSE_DEBUG_RECONSTRUCT
    std::cout << "c recursive flip(s) done." << std::endl;
    #endif

    //Propagate&check, see what happens
    bool OK = propagate();
    if (!OK) {
        std::cout << "Error! Propagation leads to failure after flipping of value" << std::endl;
        exit(-1);
    }
}

const bool SolutionExtender::addClause(const std::vector< Lit >& givenLits)
{
    vector<Lit> lits = givenLits;
    vector<Lit>::iterator i = lits.begin();
    vector<Lit>::iterator j = i;
    for (vector<Lit>::iterator end = lits.end(); i != end; i++) {
        if (value(*i) == l_True && solver.varData[i->var()].level == 0) {
            return true;
        }

        if (value(*i) == l_False && solver.varData[i->var()].level == 0) {
            continue;
        }

        *j++ = *i;
    }
    lits.resize(lits.size()-(i-j));

    #ifdef VERBOSE_DEBUG_RECONSTRUCT
    std::cout << "c Adding extend clause: " << lits << std::endl;
    #endif

    assert(lits.size() != 0);
    MyClause* cl = new MyClause(lits, false);
    clauses.push_back(cl);
    for (vector<Lit>::const_iterator it = lits.begin(), end = lits.end(); it != end; it++)
    {
        occur[it->toInt()].push_back(cl);
    }

    const bool OK = propagateCl(*cl);
    if (!OK || !propagate()) return false;
    return true;
}

void SolutionExtender::addXorClause(const vector<Lit>& givenLits, const bool xorEqualFalse)
{
    bool val = false;
    bool rhs = !xorEqualFalse;

    vector<Lit> lits = givenLits;
    vector<Lit>::iterator i = lits.begin();
    vector<Lit>::iterator j = i;
    for (vector<Lit>::iterator end = lits.end(); i != end; i++) {
        assert(i->unsign() == *i);

        if (value(i->var()) != l_Undef) {
            val ^= value(i->var()).getBool();
        }

        if (value(i->var()) != l_Undef && solver.varData[i->var()].level == 0) {
            rhs ^= value(i->var()).getBool();
            val ^= value(i->var()).getBool();
            continue;
        }

        *j++ = *i;
    }
    lits.resize(lits.size()-(i-j));

    if (lits.size() == 0) {
        if (val != rhs) {
            std::cout << "Error adding xor-clause while extending" << std::endl;
            exit(-1);
        }
        return;
    }

    assert(lits.size() != 0);
    MyClause* cl = new MyClause(lits, true, rhs);
    clauses.push_back(cl);
    for (vector<Lit>::const_iterator it = lits.begin(), end = lits.end(); it != end; it++)
    {
        occur[it->toInt()].push_back(cl);
        occur[(~*it).toInt()].push_back(cl);
    }

    const bool OK = propagateCl(*cl);
    if (!OK || !propagate()) {
        std::cout << "Error! Problems while adding elimed clause! Error!" << std::endl;
        exit(-1);
    }
}

const bool SolutionExtender::propagate()
{
    bool ret = true;
    while(qhead < trail.size()) {
        const Lit p = trail[qhead++];
        const vector<MyClause*>& occ = occur[(~p).toInt()];
        for(vector<MyClause*>::const_iterator it = occ.begin(), end = occ.end(); it != end; it++) {
            const bool thisRet = propagateCl(**it);
            if (!thisRet) {
                std::cout << "Problem with clause: " << (*it)->getLits() << std::endl;
            }
            ret &= thisRet;
        }
    }

    return ret;
}

const bool SolutionExtender::propagateCl(MyClause& cl)
{
    if (cl.getXor()) {
        size_t numUndef = 0;
        bool val = false;
        Var lastUndef = var_Undef;
        for (vector<Lit>::const_iterator it = cl.begin(), end = cl.end(); it != end; it++)
        {
            if (value(it->var()) == l_True) val ^= true;

            if (value(it->var()) == l_Undef) {
                numUndef++;
                if (numUndef > 1) break;
                lastUndef = it->var();
            }
        }
        if (numUndef == 1) {
            Lit toEnqueue = Lit(lastUndef, val == cl.getRhs());
            #ifdef VERBOSE_DEBUG_RECONSTRUCT
            std::cout << "c Due to xcl " << cl.getLits() << " = " << cl.getRhs() << " --> propagate enqueueing " << toEnqueue << std::endl;
            #endif
            enqueue(toEnqueue);
        }
        if (numUndef >= 1) return true;

        assert(numUndef == 0);
        return cl.getRhs() == val;
    } else {
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
            std::cout << "c Due to cl " << cl.getLits() << " propagate enqueueing " << lastUndef << std::endl;
            #endif
            enqueue(lastUndef);
        }
        if (numUndef >= 1) return true;

        assert(numUndef == 0);
        return false;
    }
}

const Lit SolutionExtender::pickBranchLit()
{
    for (Var var = 0; var < nVars(); var++) {
        if (value(var) == l_Undef) {
            Lit toEnqueue = Lit(var, false);
            #ifdef VERBOSE_DEBUG_RECONSTRUCT
            std::cout << "c Picking lit for reconstruction: " << toEnqueue << std::endl;
            #endif
            enqueue(toEnqueue);
            return toEnqueue;
        }
    }
    return lit_Undef;
}








