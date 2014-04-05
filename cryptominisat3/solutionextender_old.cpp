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

#include "solutionextender.h"
#include "varreplacer.h"
#include "simplifier.h"
#include "solver.h"
#include "completedetachreattacher.h"
using namespace CMSat;
using std::cout;
using std::endl;

SolutionExtender::SolutionExtender(
    Solver* _solver
    , const vector<lbool>& _solution
) :
    solver(_solver)
    , qhead (0)
    , assigns(_solution)
{
    solver->model.resize(nVarsOuter(), l_Undef);
    for (Var var = 0; var < nVarsOuter(); var++) {
        solver->model[var] = value(var);
    }
    release_assert(solver->verifyModel());
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

    assert(clausesToFree.empty());

    //First detach all long clauses
    CompleteDetachReatacher detachReattach(solver);
    detachReattach.detachNonBinsNonTris();

    //Make watches large enough to fit occur of all
    solver->watches.resize(nVarsOuter()*2);

    //Sanity check
    if (solver->simplifier) {
        solver->simplifier->checkElimedUnassignedAndStats();
    }

    //Adding binary clauses representing equivalent literals
    if (solver->conf.verbosity >= 3) {
        cout << "c Adding equivalent literals" << endl;
    }
    solver->varReplacer->extendModel(this);

    if (solver->conf.verbosity >= 3) {
        cout << "c Picking braches and propagating" << endl;
    }

    //Pick branches as long as we can
    for (Var var = 0; var < nVarsOuter(); var++) {
        if (value(var) == l_Undef
            //Don't pick replaced variables
            && solver->varData[var].removed != Removed::replaced
        ) {
            Lit toEnqueue = Lit(var, false);
            #ifdef VERBOSE_DEBUG_RECONSTRUCT
            cout << "c Picking lit for reconstruction: " << toEnqueue << endl;
            #endif
            enqueue(toEnqueue);

            const bool OK = propagate();
            if (!OK) {
                cout
                << "Error while picking lit " << toEnqueue
                << " and propagating after solution reconstruction"
                << endl;
                assert(false);

                std::exit(-1);
            }
        }
    }

    if (solver->conf.verbosity >= 3) {
        cout << "c Adding blocked clauses" << endl;
    }
    if (solver->simplifier) {
        solver->simplifier->extendModel(this);
    }

    //Copy&check model
    solver->model.resize(nVarsOuter(), l_Undef);
    for (Var var = 0; var < nVarsOuter(); var++) {
        solver->model[var] = value(var);
    }

    release_assert(solver->verifyModel());

    //free clauses
    for (vector<ClOffset>::iterator
        it = clausesToFree.begin(), end = clausesToFree.end()
        ; it != end
        ; it++
    ) {
        solver->clAllocator.clauseFree(*it);
    }
    clausesToFree.clear();

    //Reset watch size to smaller one
    solver->watches.resize(solver->nVars()*2);

    //Remove occur, go back to 0, and
    detachReattach.detachNonBinsNonTris();
    solver->cancelUntil(0);
    detachReattach.reattachLongs();
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
    tmpLits = givenLits;

    //Remove lits set at 0-level or return TRUE if any is set to TRUE at 0-level
    vector<Lit>::iterator i = tmpLits.begin();
    vector<Lit>::iterator j = i;
    for (vector<Lit>::iterator end = tmpLits.end(); i != end; i++) {
        if (value(*i) == l_True && solver->varData[i->var()].level == 0) {
            return true;
        }

        if (value(*i) == l_False && solver->varData[i->var()].level == 0) {
            continue;
        }

        *j++ = *i;
    }
    tmpLits.resize(tmpLits.size()-(i-j));

    #ifdef VERBOSE_DEBUG_RECONSTRUCT
    cout << "c Adding extend clause: " << tmpLits << " blocked on: " << blockedOn << endl;
    #endif

    //Empty clause, oops!
    if (tmpLits.empty())
        return false;

    //Create new clause, and add it
    Clause* cl = solver->clAllocator.Clause_new(
        tmpLits //the literals
        , 0 //the time it was created -- useless, ignoring
        , true //yes, this is extender, so don't care if it's <=3 in size
    );
    ClOffset offset = solver->clAllocator.getOffset(cl);
    clausesToFree.push_back(offset);
    for (vector<Lit>::const_iterator
        it = tmpLits.begin(), end = tmpLits.end()
        ; it != end
        ; it++
    ) {
        //Special used of blocked Lit -- for blocking, but not in the same
        //sense as the original
        solver->watches[it->toInt()].push(Watched(offset, blockedOn));
    }

    propagateCl(cl, blockedOn);
    if (!propagate()) {
        assert(false);
        return false;
    }

    return true;
}

inline bool SolutionExtender::propBinaryClause(
    watch_subarray_const::const_iterator i
    , const Lit p
) {
    const lbool val = value(i->lit2());
    if (val == l_Undef) {
        #ifdef VERBOSE_DEBUG_RECONSTRUCT
        cout
        << "c Due to cl "
        << ~p << ", " << i->lit2()
        << " propagate enqueueing "
        << i->lit2() << endl;
        #endif
        enqueue(i->lit2());
    } else if (val == l_False){
        return false;
    }

    return true;
}

inline bool SolutionExtender::propTriClause(
    watch_subarray_const::const_iterator i
    , const Lit p
) {
    const Lit lit2 = i->lit2();
    lbool val2 = value(lit2);

    //literal is already satisfied, nothing to do
    if (val2 == l_True)
        return true;

    const Lit lit3 = i->lit3();
    lbool val3 = value(lit3);

    //literal is already satisfied, nothing to do
    if (val3 == l_True)
        return true;

    if (val2 == l_False && val3 == l_False) {
        return false;
    }
    if (val2 == l_Undef && val3 == l_False) {
        #ifdef VERBOSE_DEBUG_RECONSTRUCT
        cout
        << "c Due to cl "
        << ~p << ", "
        << i->lit2() << ", "
        << i->lit3()
        << " propagate enqueueing "
        << lit2 << endl;
        #endif
        enqueue(lit2);
        return true;
    }

    if (val3 == l_Undef && val2 == l_False) {
        #ifdef VERBOSE_DEBUG_RECONSTRUCT
        cout
        << "c Due to cl "
        << ~p << ", "
        << i->lit2() << ", "
        << i->lit3()
        << " propagate enqueueing "
        << lit3 << endl;
        #endif
        enqueue(lit3);
        return true;
    }

    return true;
}

bool SolutionExtender::propagate()
{
    bool ret = true;
    while(qhead < trail.size()) {
        const Lit p = trail[qhead++];
        watch_subarray_const ws = solver->watches[(~p).toInt()];
        for(watch_subarray::const_iterator
            it = ws.begin(), end = ws.end()
            ; it != end
            ; it++
        ) {
            if (it->isBinary() && !it->red()) {
                bool thisret = propBinaryClause(it, p);
                ret &= thisret;
                if (!thisret) {
                    cout
                    << "Problem with implicit binary clause: "
                    << ~p
                    << ", " << it->lit2()
                    << endl;
                }

                continue;
            }

            //Propagate tri clause
            if (it->isTri() && !it->red()) {
                bool thisret = propTriClause(it, p);
                ret &= thisret;
                if (!thisret) {
                    cout
                    << "Problem with implicit tertiary clause: "
                    << ~p
                    << ", " << it->lit2()
                    << ", " << it->lit3()
                    << endl;
                }

                continue;
            }

            if (it->isClause()) {
                ClOffset offset = it->getOffset();
                const Clause* cl = solver->clAllocator.getPointer(offset);
                const Lit blockedOn = it->getBlockedLit();
                const bool thisRet = propagateCl(cl, blockedOn);
                if (!thisRet) {
                    cout << "Problem with clause: " << (*it) << endl;
                }
                ret &= thisRet;
            }
        }
    }

    return ret;
}

bool SolutionExtender::propagateCl(
    const Clause* cl
    , const Lit blockedOn
) {
    size_t numUndef = 0;
    Lit lastUndef = lit_Undef;
    for (const Lit
        *it = cl->begin(), *end = cl->end()
        ; it != end
        ; it++
    ) {
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
        cout << "c Due to cl " << *cl << " propagate enqueueing " << lastUndef << endl;
        #endif
        enqueue(lastUndef);
    }

    if (numUndef >= 1)
        return true;

    //Must flip
    #ifdef VERBOSE_DEBUG_RECONSTRUCT
    cout
    << "Flipping lit " << blockedOn
    << " due to clause " << *cl << endl;
    #endif
    assert(blockedOn != lit_Undef);

    if (solver->varData[blockedOn.var()].level == 0) {
        cout
        << "!! Flip 0-level var:"
        << solver->map_inter_to_outer(blockedOn.var()) + 1
        << endl;
    }

    assert(
        (solver->varData[blockedOn.var()].level != 0
            //|| solver->varData[blockedOn.var()].removed == Removed::decomposed
        )
        && "We cannot flip 0-level vars"
    );
    enqueue(blockedOn);
    replaceSet(blockedOn);
    return true;
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

void SolutionExtender::replaceSet(Lit toSet)
{
    //set forward equivalent
    if (solver->varReplacer->isReplaced(toSet)) {
        toSet = solver->varReplacer->getLitReplacedWith(toSet);
        enqueue(toSet);
    }
    replaceBackwardSet(toSet);

    #ifdef VERBOSE_DEBUG_RECONSTRUCT
    cout << "c recursive set(s) done." << endl;
    #endif
}

void SolutionExtender::replaceBackwardSet(const Lit toSet)
{
    //set backward equiv
    map<Var, vector<Var> >::const_iterator revTable = solver->varReplacer->getReverseTable().find(toSet.var());
    if (revTable != solver->varReplacer->getReverseTable().end()) {
        const vector<Var>& toGoThrough = revTable->second;
        for (size_t i = 0; i < toGoThrough.size(); i++) {
            //Get sign of replacement
            const Lit lit = Lit(toGoThrough[i], false);
            Lit tmp = solver->varReplacer->getLitReplacedWith(lit);

            //Set var
            enqueue(lit ^ tmp.sign() ^ toSet.sign());
        }
    }
}
