/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.0 of the License, or (at your option) any later version.
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

#include "time_mem.h"
#include "assert.h"
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <set>
#include <algorithm>
#include <fstream>
#include <set>
#include <iostream>
#include <limits>


#include "simplifier.h"
#include "clause.h"
#include "solver.h"
#include "clausecleaner.h"
#include "constants.h"
#include "solutionextender.h"
#include "gatefinder.h"
#include "varreplacer.h"
#include "varupdatehelper.h"
#include "completedetachreattacher.h"
#include "subsumestrengthen.h"

#ifdef USE_M4RI
#include "xorfinder.h"
#endif

//#define VERBOSE_DEBUG
#ifdef VERBOSE_DEBUG
#define BIT_MORE_VERBOSITY
#define VERBOSE_ORGATE_REPLACE
#define VERBOSE_ASYMTE
#define VERBOSE_GATE_REMOVAL
#define VERBOSE_XORGATE_MIX
#define VERBOSE_DEBUG_XOR_FINDER
#define VERBOSE_DEBUG_VARELIM
#endif

using namespace CMSat;
using std::cout;
using std::endl;

//#define VERBOSE_DEBUG_VARELIM
//#define VERBOSE_DEBUG_XOR_FINDER
//#define BIT_MORE_VERBOSITY
//#define TOUCH_LESS
//#define VERBOSE_ORGATE_REPLACE
//#define VERBOSE_DEBUG_ASYMTE
//#define VERBOSE_GATE_REMOVAL
//#define VERBOSE_XORGATE_MIX

Simplifier::Simplifier(Solver* _solver):
    solver(_solver)
    , varElimOrder(VarOrderLt(varElimComplexity))
    , xorFinder(NULL)
    , gateFinder(NULL)
    , anythingHasBeenBlocked(false)
    , blockedMapBuilt(false)
    , seen(solver->seen)
    , seen2(solver->seen2)
    , toClear(solver->toClear)
{
    #ifdef USE_M4RI
    if (solver->conf.doFindXors) {
        xorFinder = new XorFinder(this, solver);
    }
    #endif
    subsumeStrengthen = new SubsumeStrengthen(this, solver);

    if (solver->conf.doGateFind) {
        gateFinder = new GateFinder(this, solver);
    }
}

Simplifier::~Simplifier()
{
    #ifdef USE_M4RI
    delete xorFinder;
    #endif
    delete subsumeStrengthen;

    delete gateFinder;
}

/**
@brief New var has been added to the solver

@note: MUST be called if a new var has been added to the solver

Adds occurrence list places, increments seen, etc.
*/
void Simplifier::newVar()
{
    if (solver->conf.doGateFind) {
        gateFinder->newVar();
    }

    //variable status
    var_elimed .push_back(false);
}

void Simplifier::updateVars(
    const vector<uint32_t>& outerToInter
    , const vector<uint32_t>& interToOuter
) {
    updateArray(var_elimed, interToOuter);
}

void Simplifier::print_blocked_clauses_reverse() const
{
    for(vector<BlockedClause>::const_reverse_iterator
        it = blockedClauses.rbegin(), end = blockedClauses.rend()
        ; it != end
        ; it++
    ) {
        //Display the internal variable numbers
        vector<Lit> tmp(it->lits);
        updateLitsMap(tmp, solver->outerToInterMain);

        cout
        << "blocked clause " << tmp
        << " blocked on var " << getUpdatedVar(it->blockedOn.var(), solver->outerToInterMain)+1
        << endl;
    }
}

void Simplifier::extendModel(SolutionExtender* extender)
{
    //Either a variable is not eliminated, or its value is false
    for(size_t i = 0; i < var_elimed.size(); i++) {
        assert(var_elimed[i] == false || solver->value(i) == l_Undef);
    }

    cleanBlockedClauses();
    #ifdef VERBOSE_DEBUG_RECONSTRUCT
    cout << "Number of blocked clauses:" << blockedClauses.size() << endl;
    print_blocked_clauses_reverse();
    #endif

    //go through in reverse order
    for (vector<BlockedClause>::const_reverse_iterator
        it = blockedClauses.rbegin(), end = blockedClauses.rend()
        ; it != end
        ; it++
    ) {
        vector<Lit> lits(it->lits);
        updateLitsMap(lits, solver->outerToInterMain);
        Lit blockedOn = getUpdatedLit(it->blockedOn, solver->outerToInterMain);
        bool ret = extender->addClause(lits, blockedOn);
        assert(ret);
    }
}

/**
@brief Removes&free-s a clause from everywhere
*/
void Simplifier::unlinkClause(const ClOffset offset, bool doDrup)
{
    Clause& cl = *solver->clAllocator->getPointer(offset);
    #ifdef DRUP
    if (solver->drup.enabled() && doDrup) {
       solver->drup
       << "d " << cl
       << " 0\n";
    }
    #endif

    //Remove from occur
    for (uint32_t i = 0; i < cl.size(); i++) {
        *toDecrease -= 2*solver->watches[cl[i].toInt()].size();

        removeWCl(solver->watches[cl[i].toInt()], offset);

        if (!cl.red())
            touched.touch(cl[i]);
    }

    if (cl.red()) {
        solver->litStats.redLits -= cl.size();
    } else {
        solver->litStats.irredLits -= cl.size();
    }

    //Free and set to NULL
    solver->clAllocator->clauseFree(&cl);
}

lbool Simplifier::cleanClause(ClOffset offset)
{
    assert(solver->ok);

    bool satisfied = false;
    Clause& cl = *solver->clAllocator->getPointer(offset);
    #ifdef VERBOSE_DEBUG
    cout << "Clause to clean: " << cl << endl;
    for(size_t i = 0; i < cl.size(); i++) {
        cout << cl[i] << " : "  << solver->value(cl[i]) << " , ";
    }
    cout << endl;
    #endif
    #ifdef DRUP
    vector<Lit> origCl(cl.size());
    std::copy(cl.begin(), cl.end(), origCl.begin());
    #endif

    Lit* i = cl.begin();
    Lit* j = cl.begin();
    const Lit* end = cl.end();
    *toDecrease -= cl.size();
    for(; i != end; i++) {
        if (solver->value(*i) == l_Undef) {
            *j++ = *i;
            continue;
        }

        if (solver->value(*i) == l_True)
            satisfied = true;

        if (solver->value(*i) == l_True
            || solver->value(*i) == l_False
        ) {
            removeWCl(solver->watches[i->toInt()], offset);
        }
    }
    cl.shrink(i-j);

    if (satisfied) {
        #ifdef VERBOSE_DEBUG
        cout << "Clause cleaning -- satisfied, removing" << endl;
        #endif
        #ifdef DRUP
        (solver->drup)
        << "d " << origCl
        << " 0\n";
        #endif

        unlinkClause(offset, false);
        return l_True;
    }

    //Update lits stat
    if (cl.red())
        solver->litStats.redLits -= i-j;
    else
        solver->litStats.irredLits -= i-j;

    #ifdef DRUP
    if (solver->conf.verbosity >= 6) {
        cout << "-> Clause became after cleaning:" << cl << endl;
    }
    if (i-j > 0) {
        solver->drup
        << cl
        << " 0\n"

        //Delete old one
        << "d " << origCl
        << " 0\n";
    }
    #endif

    switch(cl.size()) {
        case 0:
            unlinkClause(offset, false);
            solver->ok = false;
            return l_False;

        case 1:
            solver->enqueue(cl[0]);
            #ifdef STATS_NEEDED
            solver->propStats.propsUnit++;
            #endif
            unlinkClause(offset, false);
            return l_True;

        case 2:
            solver->attachBinClause(cl[0], cl[1], cl.red());
            unlinkClause(offset, false);
            return l_True;

        case 3:
            solver->attachTriClause(cl[0], cl[1], cl[2], cl.red());
            unlinkClause(offset, false);
            return l_True;

        default:
            cl.setStrenghtened();
            return l_Undef;
    }
}

/**
@brief Adds clauses from the solver to the occur
*/
bool Simplifier::addFromSolver(
    vector<ClOffset>& toAdd
    , bool alsoOccur
    , bool irred
    , uint64_t& numLitsAdded
) {
    //solver->printWatchMemUsed();

    //Estimate memory usage it would imply and if over
    //over + irred -> exit
    //over + red -> don't link
    if (alsoOccur) {
        uint64_t memUsage = 0;
        for (vector<ClOffset>::iterator
            it = toAdd.begin(), end = toAdd.end()
            ; it !=  end
            ; it++
        ) {
            Clause* cl = solver->clAllocator->getPointer(*it);
            //*2 because of the overhead of allocation
            memUsage += cl->size()*sizeof(Watched)*2;
        }

        //Estimate malloc overhead
        memUsage += solver->numActiveVars()*2*40;

        if (solver->conf.verbosity >= 2) {
            cout
            << "c [simp] mem usage for occur of "
            << (irred ?  "irred" : "red  ")
            << " " << std::setw(6) << memUsage/(1024ULL*1024ULL) << " MB"
            << endl;
        }

        if (irred
            && memUsage/(1024ULL*1024ULL) > solver->conf.maxOccurIrredMB
        ) {
            if (solver->conf.verbosity >= 2) {
                cout
                << "c [simp] Not linking in irred due to excessive expected memory usage"
                << endl;
            }

            return false;
        }

        if (!irred
            && memUsage/(1024ULL*1024ULL) > solver->conf.maxOccurRedMB
        ) {
            alsoOccur = false;
            if (solver->conf.verbosity >= 2) {
                cout
                << "c [simp] Not linking in red due to excessive expected memory usage"
                << endl;
            }
        }
    }

    if (!irred && alsoOccur) {
        std::sort(toAdd.begin(), toAdd.end(), MySorter(solver->clAllocator));
    }

    uint64_t linkedInLits = 0;
    size_t numNotLinkedIn = 0;
    size_t numLinkedIn = 0;
    for (vector<ClOffset>::iterator
        it = toAdd.begin(), end = toAdd.end()
        ; it !=  end
        ; it++
    ) {
        Clause* cl = solver->clAllocator->getPointer(*it);

        //Sanity check that the value given as irred is correct
        assert(
            (irred && !cl->red())
            || (!irred && cl->red())
        );

        if (alsoOccur
            //If irreduntant or (small enough AND link in limit not reached)
            && (irred
                || (cl->size() < solver->conf.maxRedLinkInSize
                    && linkedInLits < (solver->conf.maxOccurRedLitLinkedM*1000ULL*1000ULL))
            )
        ) {
            linkInClause(*cl);
            numLinkedIn++;
            linkedInLits += cl->size();
        } else {
            assert(cl->red());
            cl->setOccurLinked(false);
            numNotLinkedIn++;
        }

        clauses.push_back(*it);
    }
    toAdd.clear();
    numLitsAdded += linkedInLits;

    if (solver->conf.verbosity >= 2
        && !irred
    ) {
        //Pretty-printing
        double val;
        if (numLinkedIn + numNotLinkedIn == 0) {
            val = 0;
        } else {
            val = (double)numNotLinkedIn/(double)(numLinkedIn + numNotLinkedIn)*100.0;
        }

        cout
        << "c [simp] Not linked in red "
        << numNotLinkedIn << "/" << (numLinkedIn + numNotLinkedIn)
        << " ("
        << std::setprecision(2) << std::fixed
        << val
        << " %)"
        << endl;
    }

    return true;
}

/**
@brief Adds clauses from here, back to the solver
*/
void Simplifier::addBackToSolver()
{
    for (vector<ClOffset>::const_iterator
        it = clauses.begin(), end = clauses.end()
        ; it != end
        ; it++
    ) {
        Clause* cl = solver->clAllocator->getPointer(*it);

        //Clause has been removed
        if (cl->getFreed())
            continue;

        //All clauses are larger than 2-long
        assert(cl->size() > 3);

        //Check variable elimination sanity
        bool notLinkedNeedFree = false;
        for (Clause::const_iterator
            it2 = cl->begin(), end2 = cl->end()
            ; it2 != end2
            ; it2++
        ) {
            //The clause was too long, and wasn't linked in
            //but has been var-elimed, so remove it
            if (!cl->getOccurLinked()
                && solver->varData[it2->var()].removed == Removed::elimed
            ) {
                notLinkedNeedFree = true;
            }

            if (cl->getOccurLinked()
                && solver->varData[it2->var()].removed != Removed::none
                && solver->varData[it2->var()].removed != Removed::queued_replacer
            ) {
                cout
                << "ERROR! Clause " << *cl
                << " red: " << cl->red()
                << " contains lit " << *it2
                << " which has removed status"
                << removed_type_to_string(solver->varData[it2->var()].removed)
                << endl;

                assert(false);
                exit(-1);
            }
        }

        //The clause wasn't linked in but needs removal now
        if (notLinkedNeedFree) {
            if (cl->red()) {
                solver->litStats.redLits -= cl->size();
            } else {
                solver->litStats.irredLits -= cl->size();
            }

            //Free
            solver->clAllocator->clauseFree(cl);
            continue;
        }

        if (completeCleanClause(*cl)) {
            solver->attachClause(*cl);
            if (cl->red()) {
                solver->longRedCls.push_back(*it);
            } else {
                solver->longIrredCls.push_back(*it);
            }
        } else {
            solver->clAllocator->clauseFree(cl);
        }
    }
}

bool Simplifier::completeCleanClause(Clause& cl)
{
    assert(cl.size() > 3);

    //Remove all lits from stats
    //we will re-attach the clause either way
    if (cl.red()) {
        solver->litStats.redLits -= cl.size();
    } else {
        solver->litStats.irredLits -= cl.size();
    }

    #ifdef DRUP
    vector<Lit> origCl(cl.size());
    std::copy(cl.begin(), cl.end(), origCl.begin());
    #endif

    Lit *i = cl.begin();
    Lit *j = i;
    for (Lit *end = cl.end(); i != end; i++) {
        if (solver->value(*i) == l_True) {
            #ifdef DRUP
            (solver->drup)
            << "d " << origCl
            << " 0\n";
            #endif
            return false;
        }

        if (solver->value(*i) == l_Undef) {
            *j++ = *i;
        }
    }
    cl.shrink(i-j);

    #ifdef DRUP
    if (i - j > 0) {
        solver->drup
        << cl
        << " 0\n"

        //Delete old one
        << "d " << origCl
        << " 0\n";
    }
    #endif

    switch (cl.size()) {
        case 0:
            solver->ok = false;
            return false;

        case 1:
            solver->enqueue(cl[0]);
            #ifdef STATS_NEEDED
            solver->propStats.propsUnit++;
            #endif
            return false;

        case 2:
            solver->attachBinClause(cl[0], cl[1], cl.red());
            return false;

        case 3:
            solver->attachTriClause(cl[0], cl[1], cl[2], cl.red());
            return false;

        default:
            return true;
    }

    return true;
}

void Simplifier::removeAllLongsFromWatches()
{
    for (vector<vec<Watched> >::iterator
        it = solver->watches.begin(), end = solver->watches.end()
        ; it != end
        ; it++
    ) {
        vec<Watched>& ws = *it;

        vec<Watched>::iterator i = ws.begin();
        vec<Watched>::iterator j = i;
        for (vec<Watched>::iterator end2 = ws.end(); i != end2; i++) {
            if (i->isClause()) {
                continue;
            } else {
                assert(i->isBinary() || i->isTri());
                *j++ = *i;
            }
        }
        ws.shrink(i - j);
    }
}

bool Simplifier::eliminateVars()
{
    //Set-up
    double myTime = cpuTime();
    size_t vars_elimed = 0;
    size_t wenThrough = 0;
    toDecrease = &numMaxElim;

    //Order the variables
    orderVarsForElimInit();

    if (solver->conf.verbosity >= 5) {
        cout
        << "c #order size:"
        << varElimOrder.size()
        << endl;
    }

    //Go through the ordered list of variables to eliminate
    while(!varElimOrder.empty()
        && *toDecrease > 0
        && numMaxElimVars > 0
    ) {
        assert(toDecrease == &numMaxElim);
        Var var = varElimOrder.removeMin();

        //Stats
        *toDecrease -= 20;
        wenThrough++;

        //Print status
        if (solver->conf.verbosity >= 5
            && wenThrough % 200 == 0
        ) {
            cout << "toDecrease: " << *toDecrease << endl;
        }

        //Can this variable be eliminated at all?
        if (solver->value(var) != l_Undef
            || solver->varData[var].removed != Removed::none
            //|| !gateFinder->canElim(var)
        ) {
            continue;
        }

        //Try to eliminate
        if (maybeEliminate(var)) {
            vars_elimed++;
            numMaxElimVars--;
        }

        //During elimination, we reached UNSAT, finish
        if (!solver->ok)
            goto end;
    }

end:
    if (solver->conf.verbosity >= 2) {
        cout << "c  #try to eliminate: " << wenThrough << endl
        << "c  #var-elim: " << vars_elimed << endl
        << "c  #time-out: " << ((*toDecrease <= 0) ? "Y" : "N") << endl
        << "c  #time: " << (cpuTime() - myTime) << endl;
    }
    assert(toDecrease == &numMaxElim);

    runStats.varElimTimeOut += (*toDecrease <= 0);
    runStats.varElimTime += cpuTime() - myTime;

    return solver->ok;
}

bool Simplifier::propagate()
{
    assert(solver->ok);

    while (solver->qhead < solver->trail.size()) {
        Lit p = solver->trail[solver->qhead];
        solver->qhead++;
        vec<Watched>& ws = solver->watches[(~p).toInt()];

        //Go through each occur
        for (vec<Watched>::const_iterator
            it = ws.begin(), end = ws.end()
            ; it != end
            ; it++
        ) {
            if (it->isClause()) {
                const Clause& cl = *solver->clAllocator->getPointer(it->getOffset());

                //Cannot be already removed in occur
                assert(!cl.getFreed());

                //Find what's up with this clause
                Lit lastUndef = lit_Undef;
                uint32_t numUndef = 0;
                bool satisfied = false;
                for (uint32_t i = 0; i < cl.size(); i++) {
                    const lbool val = solver->value(cl[i]);
                    if (val == l_True) {
                        satisfied = true;
                        break;
                    }
                    if (val == l_Undef) {
                        numUndef++;
                        if (numUndef > 1) break;
                        lastUndef = cl[i];
                    }
                }

                //Satisfied
                if (satisfied)
                    continue;

                //UNSAT
                if (numUndef == 0) {
                    solver->ok = false;
                    return false;
                }

                //Propagation
                if (numUndef == 1) {
                    solver->enqueue(lastUndef);

                    //Update stats
                    #ifdef STATS_NEEDED
                    if (cl.size() == 3)
                        if (cl.red())
                            solver->propStats.propsTriRed++;
                        else
                            solver->propStats.propsTriIrred++;
                    else {
                        if (cl.red())
                            solver->propStats.propsLongRed++;
                        else
                            solver->propStats.propsLongIrred++;
                    }
                    #endif
                }
            }

            if (it->isBinary()) {
                const lbool val = solver->value(it->lit2());

                //UNSAT
                if (val == l_False) {
                    solver->ok = false;
                    return false;
                }

                //Propagation
                if (val == l_Undef) {
                    solver->enqueue(it->lit2());
                    #ifdef STATS_NEEDED
                    if (it->red())
                        solver->propStats.propsBinRed++;
                    else
                        solver->propStats.propsBinIrred++;
                    #endif
                }
            }
        }
    }

    return true;
}

void Simplifier::subsumeReds()
{
    double myTime = cpuTime();

    //Test & debug
    solver->testAllClauseAttach();
    solver->checkNoWrongAttach();
    assert(solver->varReplacer->getNewToReplaceVars() == 0
            && "Cannot work in an environment when elimnated vars could be replaced by other vars");

    //If too many clauses, don't do it
    if (solver->getNumLongClauses() > 10000000UL
        || solver->litStats.irredLits > 50000000UL
    )  return;

    //Setup
    addedClauseLits = 0;
    runStats.clear();
    clauses.clear();
    toDecrease = &numMaxSubsume1;
    size_t origTrailSize = solver->trail.size();

    //Remove all long clauses from watches
    removeAllLongsFromWatches();

    //Add red to occur
    runStats.origNumRedLongClauses = solver->longRedCls.size();
    addFromSolver(
        solver->longRedCls
        , true //try to add to occur
        , false //irreduntant?
        , addedClauseLits
    );
    solver->longRedCls.clear();
    runStats.origNumFreeVars = solver->getNumFreeVars();
    setLimits();

    //Print link-in and startup time
    double linkInTime = cpuTime() - myTime;
    runStats.linkInTime += linkInTime;

    //Carry out subsume0
    subsumeStrengthen->performSubsumption();

    //Add irred to occur, but only temporarily
    runStats.origNumIrredLongClauses = solver->longIrredCls.size();
    addFromSolver(solver->longIrredCls
        , false //try to add to occur
        , true //irreduntant?
        , addedClauseLits
    );
    solver->longIrredCls.clear();

    //Add back clauses to solver etc
    finishUp(origTrailSize);

    if (solver->conf.verbosity >= 1) {
        subsumeStrengthen->getRunStats().printShort();
    }
}

void Simplifier::checkAllLinkedIn()
{
    for(vector<ClOffset>::const_iterator
        it = clauses.begin(), end = clauses.end()
        ; it != end
        ; it++
    ) {
        Clause& cl = *solver->clAllocator->getPointer(*it);

        assert(cl.red() || cl.getOccurLinked());
        if (cl.freed() || cl.red())
            continue;

        for(size_t i = 0; i < cl.size(); i++) {
            Lit lit = cl[i];
            bool found = findWCl(solver->watches[lit.toInt()], *it);
            assert(found);
        }
    }
}


bool Simplifier::simplify()
{
    assert(solver->okay());

    //Test & debug
    solver->testAllClauseAttach();
    solver->checkNoWrongAttach();
    assert(solver->varReplacer->getNewToReplaceVars() == 0
            && "Cannot work in an environment when elimnated vars could be replaced by other vars");

    //Clean the clauses before playing with them
    solver->clauseCleaner->removeAndCleanAll();

    //If too many clauses, don't do it
    if (solver->getNumLongClauses() > 10ULL*1000ULL*1000ULL
        || solver->litStats.irredLits > 50ULL*1000ULL*1000ULL
    ) {
        return solver->okay();
    }

    //Setup
    double myTime = cpuTime();
    addedClauseLits = 0;
    runStats.clear();
    runStats.numCalls++;
    clauses.clear();
    toDecrease = &numMaxSubsume1;
    size_t origTrailSize = solver->trail.size();

    //Remove all long clauses from watches
    removeAllLongsFromWatches();

    //Try to add irreducible to occur
    runStats.origNumIrredLongClauses = solver->longIrredCls.size();
    bool ret = addFromSolver(solver->longIrredCls
        , true //try to add to occur list
        , true //it is irred
        , addedClauseLits
    );

    //Memory limit reached, irreduntant clauses cannot
    //be added to occur --> exit
    if (!ret) {
        CompleteDetachReatacher detRet(solver);;
        detRet.reattachLongs(true);
        return solver->okay();
    }

    //Do blocked clause elimination
    setLimits();
    if (solver->conf.doBlockClauses) {
        toDecrease = &numMaxBlocked;
        blockClauses();
        blockImplicit(false, true);
    }

    //Add redundant to occur
    runStats.origNumRedLongClauses = solver->longRedCls.size();
    addFromSolver(solver->longRedCls
        , true //try to add to occur list
        , false //irreduntant?
        , addedClauseLits
    );
    runStats.origNumFreeVars = solver->getNumFreeVars();
    setLimits();
    sanityCheckElimedVars();

    //Print memory usage after occur link-in
    if (solver->conf.verbosity >= 2) {
        solver->printWatchMemUsed(memUsed());
    }

    //Print link-in and startup time
    double linkInTime = cpuTime() - myTime;
    runStats.linkInTime += linkInTime;
    #ifdef DRUP
    const size_t origBlockedSize = blockedClauses.size();
    #endif

    //Gate-finding
    if (solver->conf.doCache && solver->conf.doGateFind) {
        if (!gateFinder->doAll())
            goto end;
    }

    //Subsume, strengthen, and var-elim until time-out/limit-reached or fixedpoint
    origTrailSize = solver->trail.size();

    //Do subsumption & var-elim in loop
    assert(solver->ok);

//   subsumeStrengthen->subsumeWithTris();

    //Carry out subsume0
    subsumeStrengthen->performSubsumption();

    //Carry out strengthening
    if (!subsumeStrengthen->performStrengthening())
        goto end;

    //XOR-finding
    #ifdef USE_M4RI
    if (solver->conf.doFindXors
        && xorFinder != NULL
        && !xorFinder->findXors()
    ) {
        goto end;
    }
    #endif

    if (!propImplicits()) {
        goto end;
    }

    /*if (solver->conf.doAsymmTE)
        asymmTE();*/

    //If no var elimination is needed, this IS fixedpoint
    if (solver->conf.doVarElim &&!eliminateVars()) {
        goto end;
    }

    assert(solver->ok);

end:

    #ifdef DRUP
    if (solver->conf.verbosity >= 6) {
        cout << "c Deleting blocked clauses for DRUP" << endl;
    }
    //Remove clauses that have been blocked recently
    if (solver->drup.enabled()) {
        for(size_t i = origBlockedSize; i < blockedClauses.size(); i++) {

            //If doing stamping or caching, we cannot delete binary redundant
            //clauses, because they are stored in the stamp/cache and so
            //will be used -- and DRUP will complain when used
            if (blockedClauses[i].lits.size() <= 2
                && (solver->conf.doCache
                    || solver->conf.doStamp)
            ) {
                continue;
            }

            solver->drup
            << "d ";

            for(vector<Lit>::const_iterator
                it = blockedClauses[i].lits.begin(), end = blockedClauses[i].lits.end()
                ; it != end
                ; it++
            ) {
                solver->drup << *it << " ";
            }
            solver->drup << "0\n";
        }
    }
    #endif

    finishUp(origTrailSize);

    //Print stats
    if (solver->conf.verbosity >= 1) {
        if (solver->conf.verbosity >= 3)
            runStats.print(solver->nVars());
        else
            runStats.printShort(solver->conf.doVarElim);
    }

    return solver->ok;
}

bool Simplifier::unEliminate(Var var)
{
    assert(solver->decisionLevel() == 0);
    assert(solver->okay());

    //Check that it was really eliminated
    assert(var_elimed[var]);
    assert(solver->varData[var].removed == Removed::elimed);
    assert(!solver->decisionVar[var]);
    assert(solver->value(var) == l_Undef);

    if (!blockedMapBuilt) {
        cleanBlockedClauses();
        buildBlockedMap();
    }

    //Uneliminate it in theory
    var_elimed[var] = false;
    globalStats.numVarsElimed--;
    solver->varData[var].removed = Removed::none;
    solver->setDecisionVar(var);
    if (solver->conf.doStamp) {
        solver->stamp.remove_from_stamps(var);
    }

    //Find if variable is really needed to be eliminated
    var = getUpdatedVar(var, solver->interToOuterMain);
    map<Var, vector<size_t> >::iterator it = blk_var_to_cl.find(var);
    if (it == blk_var_to_cl.end())
        return solver->okay();

    //Eliminate it in practice
    //NOTE: Need to eliminate in theory first to avoid infinite loops
    for(size_t i = 0; i < it->second.size(); i++) {
        size_t at = it->second[i];

        //Mark for removal from blocked list
        blockedClauses[at].toRemove = true;
        assert(blockedClauses[at].blockedOn.var() == var);

        //Re-insert into Solver
        const vector<Lit>& cl = blockedClauses[at].lits;
        #ifdef VERBOSE_DEBUG_RECONSTRUCT
        cout << "Uneliminating " << cl << " on var " << var+1 << endl;
        #endif
        solver->addClause(cl);
        if (!solver->okay())
            return false;
    }

    return solver->okay();
}

void Simplifier::buildBlockedMap()
{
    blk_var_to_cl.clear();
    for(size_t i = 0; i < blockedClauses.size(); i++) {
        const BlockedClause& blocked = blockedClauses[i];
        map<Var, vector<size_t> >::iterator it
            = blk_var_to_cl.find(blocked.blockedOn.var());

        if (it == blk_var_to_cl.end()) {
            vector<size_t> tmp;
            tmp.push_back(i);
            blk_var_to_cl[blocked.blockedOn.var()] = tmp;
        } else {
            it->second.push_back(i);
        }
    }
    blockedMapBuilt = true;
}

void Simplifier::finishUp(
    size_t origTrailSize
) {
    bool somethingSet = (solver->trail.size() - origTrailSize) > 0;

    runStats.zeroDepthAssings = solver->trail.size() - origTrailSize;
    double myTime = cpuTime();

    //if variable got assigned in the meantime, uneliminate/unblock corresponding clauses
    if (somethingSet) {
        cleanBlockedClauses();
    }


    //Add back clauses to solver
    removeAllLongsFromWatches();
    addBackToSolver();

    //Propagate
    if (solver->ok) {
        propImplicits();
    }

    //We can now propagate from solver
    //since the clauses are now back normally
    if (solver->ok) {
        solver->ok = solver->propagate().isNULL();
    }

    //Sanity checks
    if (solver->ok && somethingSet) {
        solver->testAllClauseAttach();
        solver->checkNoWrongAttach();
        solver->checkStats();
        solver->checkImplicitPropagated();
    }

    //Update global stats
    runStats.finalCleanupTime += cpuTime() - myTime;
    globalStats += runStats;
    subsumeStrengthen->finishedRun();

    if (solver->ok) {
        checkElimedUnassignedAndStats();
    }
}

bool Simplifier::propImplicits()
{
    size_t numRemovedHalfIrred = 0;
    size_t numRemovedHalfRed = 0;

    //Delayed enqueue for correct binary clause removal
    vector<Lit> toEnqueue;

    size_t wsLit = 0;
    for (vector<vec<Watched> >::iterator
        it = solver->watches.begin(), end = solver->watches.end()
        ; it != end
        ; it++, wsLit++
    ) {
        const Lit lit = Lit::toLit(wsLit);
        vec<Watched>& ws = *it;

        size_t i, j;
        for(i = 0, j = 0
            ; i < ws.size()
            ; i++
        ) {
            if (!ws[i].isBinary()) {
                ws[j++] = ws[i];
                continue;
            }

            assert(ws[i].isBinary());

            const Lit lit2 = ws[i].lit2();

            //Satisfied, remove
            if (solver->value(lit) == l_True
                || solver->value(lit2) == l_True)
            {
                if (ws[i].red())
                    numRemovedHalfRed++;
                else
                    numRemovedHalfIrred++;

                continue;
            }

            //UNSAT
            if (solver->value(lit) == l_False
                && solver->value(lit2) == l_False)
            {
                solver->ok = false;
                ws[j++] = ws[i];
                continue;
            }

            //Propagate lit1
            if (solver->value(lit) == l_Undef
                && solver->value(lit2) == l_False)
            {
                toEnqueue.push_back(lit);

                //Remove binary clause
                if (ws[i].red())
                    numRemovedHalfRed++;
                else
                    numRemovedHalfIrred++;

                continue;
            }

            //Propagate lit2
            if (solver->value(lit) == l_False
                && solver->value(lit2) == l_Undef)
            {
                toEnqueue.push_back(lit2);

                //Remove binary clause
                if (ws[i].red())
                    numRemovedHalfRed++;
                else
                    numRemovedHalfIrred++;

                continue;
            }

            if (solver->value(lit) == l_Undef
                && solver->value(lit2) == l_Undef)
            {
                ws[j++] = ws[i];
                continue;
            }

            assert(false);
        }
        ws.shrink(i-j);
    }

    //Enqueue in delayed mode
    //Otherwise the
    for(vector<Lit>::const_iterator
        it = toEnqueue.begin(), end = toEnqueue.end()
        ; it != end
        ; it++
    ) {
        lbool val = solver->value(*it);
        if (val == l_Undef)
            solver->enqueue(*it);
        else if (val == l_False)
            solver->ok = false;
    }

    assert(numRemovedHalfRed % 2 == 0);
    assert(numRemovedHalfIrred % 2 == 0);
    solver->binTri.redBins -= numRemovedHalfRed/2;
    solver->binTri.irredBins -= numRemovedHalfIrred/2;

    return solver->ok;
}

void Simplifier::sanityCheckElimedVars()
{
    //First, sanity-check the long clauses
    for (vector<ClOffset>::const_iterator
        it =  clauses.begin(), end = clauses.end()
        ; it != end
        ; it++
    ) {
        Clause* cl = solver->clAllocator->getPointer(*it);

        //Already removed
        if (cl->getFreed())
            continue;

        for (uint32_t i = 0; i < cl->size(); i++) {
            if (var_elimed[(*cl)[i].var()]) {
                cout
                << "Error: elimed var -- Lit " << (*cl)[i] << " in clause"
                << endl
                << "wrongly left in clause: " << *cl
                << endl;

                exit(-1);
            }
        }
    }

    //Then, sanity-check the binary clauses
    size_t wsLit = 0;
    for (vector<vec<Watched> >::const_iterator
        it = solver->watches.begin(), end = solver->watches.end()
        ; it != end
        ; it++, wsLit++
    ) {
        Lit lit = Lit::toLit(wsLit);
        const vec<Watched>& ws = *it;
        for (vec<Watched>::const_iterator it2 = ws.begin(), end2 = ws.end(); it2 != end2; it2++) {
            if (it2->isBinary()) {
                if (var_elimed[lit.var()] || var_elimed[it2->lit2().var()]) {
                    cout
                    << "Error: A var is elimed in a binary clause: "
                    << lit << " , " << it2->lit2()
                    << endl;

                    exit(-1);
                }
            }
        }
    }
}

/*const bool Simplifier::mixXorAndGates()
{
    assert(solver->ok);
    uint32_t fixed = 0;
    uint32_t ored = 0;
    double myTime = cpuTime();
    uint32_t oldTrailSize = solver->trail.size();
    vector<Lit> lits;
    vector<Lit> tmp;

    uint32_t index = 0;
    for (vector<Xor>::iterator it = xors.begin(), end = xors.end(); it != end; it++, index++) {
        const Xor& thisXor = *it;
        if (thisXor.vars.size() != 3) continue;

        for (uint32_t i = 0; i < thisXor.vars.size(); i++) {
            seen[thisXor.vars[i]] = true;
        }

//         for (uint32_t i = 0; i < thisXor.vars.size(); i++) {
//             Var var = thisXor.vars[i];
//             const vector<uint32_t>& occ1 = gateOccEq[Lit(var, true).toInt()];
//             for (vector<uint32_t>::const_iterator it = occ1.begin(), end = occ1.end(); it != end; it++) {
//                 const OrGate& orGate = orGates[*it];
//                 uint32_t OK = 0;
//                 for (uint32_t i2 = 0; i2 < orGate.lits.size(); i2++) {
//                     if (orGate.lits[i2].sign() &&
//                         seen[orGate.lits[i2].var()]) OK++;
//                 }
//                 if (OK>1) {
//                     cout << "XOR to look at:" << thisXor << endl;
//                     cout << "gate to look at : " << orGate << endl;
//                     cout << "---------------" << endl;
//                 }
//             }
//         }

        for (uint32_t i = 0; i < thisXor.vars.size(); i++) {
            Var var = thisXor.vars[i];
            Lit eqLit = Lit(var, true);
            const vector<uint32_t>& occ = gateOccEq[eqLit.toInt()];
            for (vector<uint32_t>::const_iterator it = occ.begin(), end = occ.end(); it != end; it++) {
                const OrGate& orGate = orGates[*it];
                assert(orGate.eqLit == eqLit);
                uint32_t OK = 0;
                lits.clear();
                bool sign = false;
                for (uint32_t i2 = 0; i2 < orGate.lits.size(); i2++) {
                    if (seen[orGate.lits[i2].var()]) {
                        OK++;
                        lits.push_back(orGate.lits[i2]  ^ true);
                        sign ^= !orGate.lits[i2].sign();
                    }
                }
                if (OK == 2) {
                    #ifdef VERBOSE_XORGATE_MIX
                    cout << "XOR to look at:" << thisXor << endl;
                    cout << "gate to look at : " << orGate << endl;
                    #endif

                    if (!thisXor.rhs^sign) {
                        fixed++;
                        tmp.clear();
                        tmp.push_back(~lits[0]);
                        #ifdef VERBOSE_XORGATE_MIX
                        cout << "setting: " << tmp[0] << endl;
                        #endif
                        solver->addClauseInt(tmp);
                        if (!solver->ok) goto end;

                        tmp.clear();
                        tmp.push_back(~lits[1]);
                        #ifdef VERBOSE_XORGATE_MIX
                        cout << "setting: " << tmp[0] << endl;
                        #endif
                        solver->addClauseInt(tmp);
                        if (!solver->ok) goto end;
                    } else {
                        ored++;
                        tmp.clear();
                        tmp.push_back(lits[0]);
                        tmp.push_back(lits[1]);
                        #ifdef VERBOSE_XORGATE_MIX
                        cout << "orIng: " << tmp << endl;
                        #endif
                        Clause* c = solver->addClauseInt(tmp, true);
                        assert(c == NULL);
                        if (!solver->ok) goto end;
                    }

                    #ifdef VERBOSE_XORGATE_MIX
                    cout << "---------------" << endl;
                    #endif
                }
            }
        }

        end:
        for (uint32_t i = 0; i < thisXor.vars.size(); i++) {
            seen[thisXor.vars[i]] = false;
        }
        if (!solver->ok) break;
    }

    if (solver->conf.verbosity >= 1) {
        cout << "c OrXorMix"
        << " Or: " << std::setw(6) << ored
        << " Fix: " << std::setw(6) << fixed
        << " Fixed: " << std::setw(4) << (solver->trail.size() - oldTrailSize)
        << " T: " << std::setprecision(2) << std::setw(5) << (cpuTime() - myTime) << " s"
        << endl;
    }

    return solver->ok;
}*/

void Simplifier::blockImplicit(
    const bool bins
    , const bool tris
) {
    blockedMapBuilt = false;

    const double myTime = cpuTime();
    size_t tried = 0;
    size_t blockedBin = 0;
    size_t blockedTri = 0;
    toDecrease = &numMaxBlockedImpl;

    //Randomize start in the watchlist
    size_t upI;
    upI = solver->mtrand.randInt(solver->watches.size()-1);
    size_t numDone = 0;
    for (
        ; numDone < solver->watches.size() && *toDecrease > 0
        ; upI = (upI +1) % solver->watches.size(), numDone++
    ) {
        //Stats
        *toDecrease -= 2;
        if (solver->conf.verbosity >= 5
            && tried % 10000 == 0
        ) {
            cout << "toDecrease: " << *toDecrease << endl;
        }

        //Set-up
        const Lit lit = Lit::toLit(upI);
        vec<Watched>& ws = solver->watches[upI];

        size_t i, j;
        for(i = 0, j = 0
            ; i < ws.size()
            ; i++
        ) {
            //Blocking of clauses is handled elsewhere
            if (ws[i].isClause()
                || ws[i].red()
                //If binary, and we don't want to remove binaries, continue
                || (!bins && ws[i].isBinary())
                //If tri, and we don't want to remove tertiaries, continue
                || (!tris && ws[i].isTri())
                //Don't go through the same binary/tri twice
                || (ws[i].isBinary()
                    && lit >= ws[i].lit2())
                || (ws[i].isTri()
                    && (lit >= ws[i].lit2() || ws[i].lit2() >= ws[i].lit3()))
            ) {
                ws[j++] = ws[i];
                continue;
            }

            tried++;
            const Lit lit2 = ws[i].lit2();
            const Lit lit3 = ws[i].isTri() ? ws[i].lit3() : lit_Undef;

            *toDecrease -= 2;
            seen[lit.toInt()] = 1;
            seen[lit2.toInt()] = 1;
            if (lit3 != lit_Undef)
                seen[lit3.toInt()] = 1;

            //Try all two/three
            bool taut = false;
            Lit tautOn = lit_Undef;
            for(Lit l: {lit, lit2, lit3}) {
                if (l != lit_Undef) {
                    taut = checkBlocked(l);
                    if (taut) {
                        tautOn = l;
                        break;
                    }
                }
            }

            if (taut) {
                dummy.clear();
                dummy.push_back(lit);
                dummy.push_back(lit2);

                if (lit3 == lit_Undef) {
                    blockedBin++;
                    *toDecrease -= solver->watches[lit2.toInt()].size();
                    removeWBin(solver->watches, lit2, lit, false);
                    assert(!ws[i].red());
                    solver->binTri.irredBins--;
                } else {
                    blockedTri++;
                    dummy.push_back(lit3);
                    *toDecrease -= solver->watches[lit2.toInt()].size();
                    *toDecrease -= solver->watches[lit3.toInt()].size();
                    removeWTri(solver->watches, lit2, lit, lit3, false);
                    removeWTri(solver->watches, lit3, lit, lit2, false);
                    assert(!ws[i].red());
                    solver->binTri.irredTris--;
                }

                blockedClauses.push_back(BlockedClause(tautOn, dummy, solver->interToOuterMain));
                anythingHasBeenBlocked = true;
            } else {
                //Not blocked, so just go through
                ws[j++] = ws[i];
            }

            seen[lit.toInt()] = 0;
            seen[lit2.toInt()] = 0;
            if (lit3 != lit_Undef)
                seen[lit3.toInt()] = 0;
        }
        ws.shrink(i-j);
    }

    if (solver->conf.verbosity >= 1) {
        cout
        << "c [block] implicit"
        << " tried: " << tried
        << " bin: " << blockedBin
        << " tri: " << blockedTri
        << " t-out: " << (numDone == solver->watches.size() ? "N" : "Y")
        << " T : " << std::fixed << std::setprecision(2) << (cpuTime() - myTime)
        << endl;
    }
    runStats.blocked += blockedBin + blockedTri;
    runStats.blockedSumLits += blockedBin*2 + blockedTri*3;
    runStats.blockTime += cpuTime() - myTime;

    //If any binary has been blocked, clear the stamps
    if (blockedBin) {
        solver->stamp.clearStamps();

        if (solver->conf.verbosity >= 2) {
            cout
            << "c [stamping] cleared stamps because of blocked binaries"
            << endl;
        }

        if (solver->conf.doCache) {
            solver->implCache.makeAllRed();
        }
    }
}

void Simplifier::blockClauses()
{
    blockedMapBuilt = false;

    const double myTime = cpuTime();
    size_t blocked = 0;
    size_t blockedLits = 0;
    size_t wenThrough = 0;
    size_t tried = 0;
    toDecrease = &numMaxBlocked;
    while(*toDecrease > 0
        && wenThrough < 2*clauses.size()
    ) {
        wenThrough++;
        *toDecrease -= 2;

        //Print status
        if (solver->conf.verbosity >= 5
            && wenThrough % 10000 == 0
        ) {
            cout << "toDecrease: " << *toDecrease << endl;
        }

        size_t num = solver->mtrand.randInt(clauses.size()-1);
        ClOffset offset = clauses[num];
        Clause& cl = *solver->clAllocator->getPointer(offset);

        //Already removed or redundant
        if (cl.getFreed() || cl.red())
            continue;

        //Cannot be redundant
        assert(!cl.red());

        tried++;

        //Fill up temps
        bool toRemove = false;
        *toDecrease -= cl.size();
        for (const Lit *l = cl.begin(), *end = cl.end(); l != end; l++) {
            seen[l->toInt()] = 1;
        }

        //Blocked clause elimination
        for (const Lit* l = cl.begin(), *end = cl.end(); l != end; l++) {
            if (solver->varData[l->var()].removed != Removed::none)
                continue;

            if (checkBlocked(*l)) {
                vector<Lit> remCl(cl.size());
                std::copy(cl.begin(), cl.end(), remCl.begin());
                blockedClauses.push_back(BlockedClause(*l, remCl, solver->interToOuterMain));
                anythingHasBeenBlocked = true;

                blocked++;
                blockedLits += cl.size();
                toRemove = true;
                break;
            }
        }

        //Clear seen
        for (Clause::const_iterator
            l = cl.begin(), end = cl.end()
            ; l != end
            ; l++
        ) {
            seen[l->toInt()] = 0;
        }

        if (toRemove) {
            //cout << "Blocking " << cl << endl;
            unlinkClause(offset, false);
        } else {
            //cout << "Not blocking " << cl << endl;
        }
    }

    if (solver->conf.verbosity >= 1) {
        cout
        << "c [block] long"
        << " through: " << wenThrough
        << " tried: " << tried
        << " blocked: " << blocked
        << " T : " << std::fixed << std::setprecision(2) << std::setw(6) << (cpuTime() - myTime)
        << endl;
    }
    runStats.blocked += blocked;
    runStats.blockedSumLits += blockedLits;
    runStats.blockTime += cpuTime() - myTime;
}

void Simplifier::asymmTE()
{
    assert(false && "asymmTE has a bug (unknown), cannot be used");
    //Random system would die here
    if (clauses.empty())
        return;

    if (solver->conf.doBlockClauses) {
        blockedMapBuilt = false;
    }

    const double myTime = cpuTime();
    uint32_t blocked = 0;
    size_t blockedLits = 0;
    uint32_t asymmSubsumed = 0;
    uint32_t removed = 0;
    size_t wenThrough = 0;

    vector<Lit> tmpCl;
    toDecrease = &numMaxAsymm;
    while(*toDecrease > 0
        && wenThrough < 2*clauses.size()
    ) {
        *toDecrease -= 2;
        wenThrough++;

        //Print status
        if (solver->conf.verbosity >= 5
            && wenThrough % 10000 == 0
        ) {
            cout << "toDecrease: " << *toDecrease << endl;
        }

        size_t num = solver->mtrand.randInt(clauses.size()-1);
        ClOffset offset = clauses[num];
        Clause& cl = *solver->clAllocator->getPointer(offset);

        //Already removed or redundant
        if (cl.getFreed() || cl.red())
            continue;


        *toDecrease -= cl.size()*2;

        //Fill tmpCl, seen
        tmpCl.clear();
        for (const Lit *l = cl.begin(), *end = cl.end(); l != end; l++) {
            seen[l->toInt()] = true;
            tmpCl.push_back(*l);
        }

        //add to tmpCl literals that could be added through reverse strengthening
        //ONLY irred
        //TODO stamping
        /*for (const Lit *l = cl.begin(), *end = cl.end(); l != end; l++) {
            const vector<LitExtra>& cache = solver->implCache[l->toInt()].lits;
            *toDecrease -= cache.size();
            for (vector<LitExtra>::const_iterator cacheLit = cache.begin(), endCache = cache.end(); cacheLit != endCache; cacheLit++) {
                if (cacheLit->getOnlyIrredBin()
                    && !seen[(~cacheLit->getLit()).toInt()]
                ) {
                    const Lit toAdd = ~(cacheLit->getLit());
                    tmpCl.push_back(toAdd);
                    seen[toAdd.toInt()] = true;
                }
            }
        }*/


        //subsumption with binary clauses
        bool toRemove = false;
        if (solver->conf.doExtBinSubs) {
            //for (vector<Lit>::const_iterator l = tmpCl.begin(), end = tmpCl.end(); l != end; l++) {
//            for (const Lit* l = cl.begin(), *end = cl.end(); l != end; l++) {

                //TODO stamping
                /*const vector<LitExtra>& cache = solver->implCache[l->toInt()].lits;
                *toDecrease -= cache.size();
                for (vector<LitExtra>::const_iterator cacheLit = cache.begin(), endCache = cache.end(); cacheLit != endCache; cacheLit++) {
                    if ((cacheLit->getOnlyIrredBin() || cl.red()) //subsume irred with irred
                        && seen[cacheLit->getLit().toInt()]
                    ) {
                        toRemove = true;
                        asymmSubsumed++;
                        #ifdef VERBOSE_DEBUG_ASYMTE
                        cout << "c AsymLitAdd removing: " << cl << endl;
                        #endif
                        goto next;
                    }
                }*/
//            }
        }

        if (cl.red())
            goto next;

        //Blocked clause elimination
        if (solver->conf.doBlockClauses && numMaxBlocked > 0) {
            toDecrease = &numMaxBlocked;
            for (const Lit* l = cl.begin(), *end = cl.end(); l != end; l++) {
                if (solver->varData[l->var()].removed != Removed::none)
                    continue;

                if (checkBlocked(*l)) {
                    vector<Lit> remCl(cl.size());
                    std::copy(cl.begin(), cl.end(), remCl.begin());
                    blockedClauses.push_back(BlockedClause(*l, remCl, solver->interToOuterMain));
                    anythingHasBeenBlocked = true;

                    blocked++;
                    blockedLits += cl.size();
                    toRemove = true;
                    toDecrease = &numMaxAsymm;
                    goto next;
                }
            }
        }

        /*
        //subsumption with irred larger clauses
        CL_ABST_TYPE abst;
        abst = calcAbstraction(tmpCl);
        *toDecrease -= tmpCl.size()*2;
        for (vector<Lit>::const_iterator it = tmpCl.begin(), end = tmpCl.end(); it != end; it++) {
            const Occur& occ = occur[it->toInt()];
            *toDecrease -= occ.size();
            for (Occur::const_iterator it2 = occ.begin(), end2 = occ.end(); it2 != end2; it2++) {
                if (it2->index != index
                    && subsetAbst(clauseData[it2->index].abst, abst)
                    && clauses[it2->index] != NULL
                    && !clauses[it2->index]->red()
                    && subsetReverse(*clauses[it2->index])
                )  {
                    #ifdef VERBOSE_DEBUG_ASYMTE
                    cout << "c AsymTE removing: " << cl << " -- subsumed by cl: " << *clauses[it2->index] << endl;
                    #endif
                    toRemove = true;
                    goto next;
                }
            }
        }*/

        next:
        if (toRemove) {
            unlinkClause(offset);
            removed++;
        }

        //Clear seen
        for (vector<Lit>::const_iterator l = tmpCl.begin(), end = tmpCl.end(); l != end; l++) {
            seen[l->toInt()] = false;
        }
    }

    if (solver->conf.verbosity >= 1) {
        cout << "c AsymmTElim"
        << " asymm subsumed: " << asymmSubsumed
        << " blocked: " << blocked
        << " T : " << std::fixed << std::setprecision(2) << std::setw(6) << (cpuTime() - myTime)
        << endl;
    }
    runStats.asymmSubs += asymmSubsumed;
    runStats.blocked += blocked;
    runStats.blockedSumLits += blockedLits;
    runStats.asymmTime += cpuTime() - myTime;
}

/**
@brief Calculate limits for backw-subsumption, var elim, etc.

It is important to have limits, otherwise the time taken to perfom these tasks
could be huge. Furthermore, it seems that there is a benefit in doing these
simplifications slowly, instead of trying to use them as much as possible
from the beginning.
*/
void Simplifier::setLimits()
{
    numMaxSubsume0    = 850LL*1000LL*1000LL;
    numMaxSubsume1    = 400LL*1000LL*1000LL;
//     numMaxTriSub      = 600LL*1000LL*1000LL;
    numMaxElim        = 800LL*1000LL*1000LL;
    numMaxAsymm       = 40LL *1000LL*1000LL;
    numMaxBlocked     = 40LL *1000LL*1000LL;
    numMaxBlockedImpl = 1800LL *1000LL*1000LL;
    numMaxVarElimAgressiveCheck  = 300LL *1000LL*1000LL;

    //numMaxElim = 0;
    //numMaxElim = std::numeric_limits<int64_t>::max();

    //If variable elimination isn't going so well
    if (globalStats.testedToElimVars > 0
        && (double)globalStats.numVarsElimed/(double)globalStats.testedToElimVars < 0.1
    ) {
        numMaxElim /= 2;
    }

    #ifdef BIT_MORE_VERBOSITY
    cout << "c addedClauseLits: " << addedClauseLits << endl;
    #endif
    if (addedClauseLits < 10ULL*1000ULL*1000ULL) {
        numMaxElim *= 2;
        numMaxSubsume0 *= 2;
        numMaxSubsume1 *= 2;
    }

    if (addedClauseLits < 3ULL*1000ULL*1000ULL) {
        numMaxElim *= 2;
        numMaxSubsume0 *= 2;
        numMaxSubsume1 *= 2;
    }

    numMaxElimVars = ((double)solver->getNumFreeVars() * solver->conf.varElimRatioPerIter);
    if (globalStats.numCalls > 0) {
        numMaxElimVars = (double)numMaxElimVars * (globalStats.numCalls+0.5);
    }
    runStats.origNumMaxElimVars = numMaxElimVars;

    if (!solver->conf.doSubsume1) {
        numMaxSubsume1 = 0;
    }

    //For debugging

    //numMaxSubsume0 = 0;
    //numMaxSubsume1 = 0;
    //numMaxElimVars = 0;
    //numMaxElim = 0;
    //numMaxSubsume0 = std::numeric_limits<int64_t>::max();
    //numMaxSubsume1 = std::numeric_limits<int64_t>::max();
    //numMaxElimVars = std::numeric_limits<int32_t>::max();
    //numMaxElim     = std::numeric_limits<int64_t>::max();
}

void Simplifier::cleanBlockedClauses()
{
    assert(solver->decisionLevel() == 0);
    vector<BlockedClause>::iterator i = blockedClauses.begin();
    vector<BlockedClause>::iterator j = blockedClauses.begin();
    size_t at = 0;

    for (vector<BlockedClause>::iterator
        end = blockedClauses.end()
        ; i != end
        ; i++, at++
    ) {
        const Var blockedOn = getUpdatedVar(i->blockedOn.var(), solver->outerToInterMain);
        if (var_elimed[blockedOn] == true
            && solver->value(blockedOn) != l_Undef
        ) {
            cout
            << "ERROR: lit " << *i << " elimed:"
            << var_elimed[blockedOn]
            << " value: " << solver->value(blockedOn)
            << endl;
            assert(false);
            exit(-1);
        }

        if (blockedClauses[at].toRemove) {
            blockedMapBuilt = false;
        } else {
            *j++ = *i;
        }
    }
    blockedClauses.resize(blockedClauses.size()-(i-j));
}

void Simplifier::removeClausesHelper(
    const vec<Watched>& todo
    , const Lit lit
) {
    blockedMapBuilt = false;
    vector<Lit> lits;

    for (uint32_t i = 0; i < todo.size(); i++) {
        const Watched& watch = todo[i];
        lits.clear();
        bool red = false;

        if (watch.isClause()) {
            ClOffset offset = watch.getOffset();
            Clause& cl = *solver->clAllocator->getPointer(offset);

            //Update stats
            if (cl.red()) {
                red = true;
                runStats.longRedClRemThroughElim++;
            } else {
                runStats.clauses_elimed_long++;
                runStats.clauses_elimed_sumsize += cl.size();

                lits.resize(cl.size());
                std::copy(cl.begin(), cl.end(), lits.begin());
                blockedClauses.push_back(BlockedClause(lit, lits, solver->interToOuterMain));
            }

            //Remove -- only DRUP the ones that are redundant
            //The irred will be removed thanks to 'blocked' system
            unlinkClause(offset, cl.red());
        }

        if (watch.isBinary()) {

            //Update stats
            if (!watch.red()) {
                runStats.clauses_elimed_bin++;
                runStats.clauses_elimed_sumsize += 2;
            } else {
                red = true;
                runStats.binRedClRemThroughElim++;
            }

            //Put clause into blocked status
            lits.resize(2);
            lits[0] = lit;
            lits[1] = watch.lit2();
            if (!watch.red()) {
                blockedClauses.push_back(BlockedClause(lit, lits, solver->interToOuterMain));

                //touch removed lits
                touched.touch(watch.lit2());
            } else {
                //If redundant, delayed blocked-based DRUP deletion will not work
                //so delete explicitly
                #ifdef DRUP
                if (!solver->conf.doStamp
                    && !solver->conf.doCache
                ) {
                   solver->drup
                   << "d "
                   << lits[0] << " "
                   << lits[1]
                   << " 0\n";
                }
                #endif
            }

            //Remove
            *toDecrease -= solver->watches[lits[0].toInt()].size();
            *toDecrease -= solver->watches[lits[1].toInt()].size();
            solver->detachBinClause(lits[0], lits[1], watch.red());
        }

        if (watch.isTri()) {

            //Update stats
            if (!watch.red()) {
                runStats.clauses_elimed_tri++;
                runStats.clauses_elimed_sumsize += 3;
            } else {
                red = true;
                runStats.triRedClRemThroughElim++;
            }

            //Put clause into blocked status
            lits.resize(3);
            lits[0] = lit;
            lits[1] = watch.lit2();
            lits[2] = watch.lit3();
            if (!watch.red()) {
                blockedClauses.push_back(BlockedClause(lit, lits, solver->interToOuterMain));

                //Touch removed lits
                touched.touch(watch.lit2());
                touched.touch(watch.lit3());
            } else {
                //If redundant, delayed blocked-based DRUP deletion will not work
                //so delete explicitly
                #ifdef DRUP
               solver->drup
               << "d "
               << lits[0] << " "
               << lits[1] << " "
               << lits[2]
               << " 0\n";
                #endif
            }

            //Remove
            *toDecrease -= solver->watches[lits[0].toInt()].size();
            *toDecrease -= solver->watches[lits[1].toInt()].size();
            *toDecrease -= solver->watches[lits[2].toInt()].size();
            solver->detachTriClause(lits[0], lits[1], lits[2], watch.red());
        }

        if (solver->conf.verbosity >= 3 && !lits.empty()) {
            cout
            << "Eliminated clause " << lits << " (red: " << red << ")"
            << " on var " << lit.var()+1
            << endl;
        }
    }
}

uint32_t Simplifier::numIrredBins(const Lit lit) const
{
    uint32_t num = 0;
    const vec<Watched>& ws = solver->watches[lit.toInt()];
    for (vec<Watched>::const_iterator it = ws.begin(), end = ws.end(); it != end; it++) {
        if (it->isBinary() && !it->red()) num++;
    }

    return num;
}

int Simplifier::testVarElim(const Var var)
{
    assert(solver->ok);
    assert(!var_elimed[var]);
    assert(solver->varData[var].removed == Removed::none);
    //assert(solver->decisionVar[var]);
    assert(solver->value(var) == l_Undef);

    //Gather data
    HeuristicData pos = calcDataForHeuristic(Lit(var, false));
    HeuristicData neg = calcDataForHeuristic(Lit(var, true));

    //Heuristic calculation took too much time
    if (*toDecrease < 0) {
        return 1000;
    }

    //Check if we should do agressive check or not
    const bool agressive = (numMaxVarElimAgressiveCheck > 0);
    runStats.usedAgressiveCheckToELim += agressive;

    //set-up
    const Lit lit = Lit(var, false);
    vec<Watched>& poss = solver->watches[lit.toInt()];
    vec<Watched>& negs = solver->watches[(~lit).toInt()];
    std::sort(poss.begin(), poss.end(), WatchSorter());
    std::sort(negs.begin(), negs.end(), WatchSorter());
    resolvents.clear();

    //Pure literal, no resolvents
    //we look at "pos" and "neg" (and not poss&negs) because we don't care about redundant clauses
    if (pos.totalCls() == 0 || neg.totalCls() == 0) {
        return -100;
    }

    /*// Heuristic CUT OFF:
    if (posSize >= 15 && negSize >= 15)
        return -1000;*/

    // Count clauses/literals after elimination
    uint32_t before_clauses = pos.bin + pos.tri + pos.longer + neg.bin + neg.tri + neg.longer;
    uint32_t after_clauses = 0;
    uint32_t after_long = 0;
    uint32_t after_bin = 0;
    uint32_t after_tri = 0;
    uint32_t after_literals = 0;
    for (vec<Watched>::const_iterator
        it = poss.begin(), end = poss.end()
        ; it != end
        ; it++
    ) {
        //Decrement available time
        *toDecrease -= 3;

        //Ignore redundant
        if (((it->isBinary() || it->isTri()) && it->red())
            || (it->isClause() && solver->clAllocator->getPointer(it->getOffset())->red())
        ) {
            continue;
        }

        for (vec<Watched>::const_iterator
            it2 = negs.begin(), end2 = negs.end()
            ; it2 != end2
            ; it2++
        ) {
            //Decrement available time
            *toDecrease -= 3;

            //Ignore redundant
            if (
                ((it2->isBinary() || it2->isTri())
                    && it2->red())
                || (it2->isClause()
                    && solver->clAllocator->getPointer(it2->getOffset())->red())
            ) {
                continue;
            }

            //Resolve the two clauses
            bool ok = merge(*it, *it2, lit, agressive);

            //The resolvent is tautological
            if (!ok)
                continue;

            #ifdef VERBOSE_DEBUG_VARELIM
            cout << "Adding new clause due to varelim: " << dummy << endl;
            #endif

            //Update after-stats
            after_clauses++;
            after_literals += dummy.size();
            if (dummy.size() > 3)
                after_long++;
            if (dummy.size() == 3)
                after_tri++;
            if (dummy.size() == 2)
                after_bin++;

            //Early-abort or over time
            if (after_clauses > before_clauses
                //Over-time
                || *toDecrease < -10LL*1000LL
            )
                return 1000;

            //Calculate new clause stats
            ClauseStats stats;
            if ((it->isBinary() || it->isTri()) && it2->isClause())
                stats = solver->clAllocator->getPointer(it2->getOffset())->stats;
            else if ((it2->isBinary() || it2->isTri()) && it->isClause())
                stats = solver->clAllocator->getPointer(it->getOffset())->stats;
            else if (it->isClause() && it2->isClause())
                stats = ClauseStats::combineStats(
                    solver->clAllocator->getPointer(it->getOffset())->stats
                    , solver->clAllocator->getPointer(it2->getOffset())->stats
            );

            resolvents.push_back(std::make_pair(dummy, stats));
        }
    }

    //Smaller value returned, the better
    int cost = (int)after_long + (int)after_tri + (int)after_bin*(int)3
        - (int)pos.longer - (int)neg.longer
        - (int)pos.tri - (int)neg.tri
        - (int)pos.bin*3 - (int)neg.bin*(int)3;

    return cost;
}

void Simplifier::printOccur(const Lit lit) const
{
    for(size_t i = 0; i < solver->watches[lit.toInt()].size(); i++) {
        const Watched& w = solver->watches[lit.toInt()][i];
        if (w.isBinary()) {
            cout
            << "Bin   --> "
            << lit << ", "
            << w.lit2()
            << "(red: " << w.red()
            << ")"
            << endl;
        }

        if (w.isTri()) {
            cout
            << "Tri   --> "
            << lit << ", "
            << w.lit2() << " , " << w.lit3()
            << "(red: " << w.red()
            << ")"
            << endl;
        }

        if (w.isClause()) {
            cout
            << "Clause--> "
            << *solver->clAllocator->getPointer(w.getOffset())
            << "(red: " << solver->clAllocator->getPointer(w.getOffset())->red()
            << ")"
            << endl;
        }
    }
}

/**
@brief Tries to eliminate variable
*/
bool Simplifier::maybeEliminate(const Var var)
{
    assert(solver->ok);

    //Print complexity stat for this var
    if (solver->conf.verbosity >= 5) {
        cout << "trying complexity: "
        << varElimComplexity[var].first
        << ", " << varElimComplexity[var].second
        << endl;
    }

    //Cannot eliminate variables that are in the assumptions
    if (solver->assumptionsSet[var]) {
        return false;
    }

    //Test if we should remove, and fill posAll&negAll
    runStats.testedToElimVars++;
    if (testVarElim(var) == 1000) {
        return false;
    }

    runStats.triedToElimVars++;

    //The literal
    const Lit lit = Lit(var, false);

    //Eliminate:
    if (solver->conf.verbosity >= 5) {
        cout
        << "Eliminating var " << lit
        << " with occur sizes "
        << solver->watches[lit.toInt()].size() << " , "
        << solver->watches[(~lit).toInt()].size()
        << endl;

        cout << "POS: " << endl;
        printOccur(lit);
        cout << "NEG: " << endl;
        printOccur(~lit);
    }

    //Save original state
    const vec<Watched> poss = solver->watches[lit.toInt()];
    const vec<Watched> negs = solver->watches[(~lit).toInt()];

    //Remove clauses
    touched.clear();
    removeClausesHelper(poss, lit);
    removeClausesHelper(negs, ~lit);

    //Occur is cleared
    assert(solver->watches[lit.toInt()].empty());
    assert(solver->watches[(~lit).toInt()].empty());

    //Add resolvents calculated in testVarElim()
    for(vector<pair<vector<Lit>, ClauseStats> >::const_iterator
        it = resolvents.begin(), end = resolvents.end()
        ; it != end
        ; it++
    ) {
        runStats.newClauses++;

        finalLits = it->first;

        //Check if a new 2-long would subsume a 3-long
        if (finalLits.size() == 2) {
            for(vec<Watched>::const_iterator
                it2 = solver->watches[finalLits[0].toInt()].begin()
                , end2 = solver->watches[finalLits[0].toInt()].end()
                ; it2 != end2
                ; it2++
            ) {
                if (it2->isTri() && !it2->red()
                    && (it2->lit2() == finalLits[1]
                        || it2->lit3() == finalLits[1])
                ) {
                    if (solver->conf.verbosity >= 6) {
                        cout
                        << "Removing irred tri-clause due to addition of"
                        << " irred bin: "
                        << finalLits[0]
                        << ", " << it2->lit2()
                        << ", " << it2->lit3()
                        << endl;
                    }

                    touched.touch(it2->lit2());
                    touched.touch(it2->lit3());

                    runStats.subsumedByVE++;
                    solver->detachTriClause(
                        finalLits[0]
                        , it2->lit2()
                        , it2->lit3()
                        , it2->red()
                    );

                    //We have to break: we just modified the stuff we are
                    //going through...
                    break;
                }
            }
        }

        //Add clause and do subsumption
        Clause* newCl = solver->addClauseInt(
            it->first //Literals in new clause
            , false //Is the new clause redundant?
            , it->second //Statistics for this new clause (usage, etc.)
            , false //Should clause be attached?
            , &finalLits //Return final set of literals here
        );

        if (!solver->ok)
            goto end;

        if (newCl != NULL) {
            linkInClause(*newCl);
            ClOffset offset = solver->clAllocator->getOffset(newCl);
            clauses.push_back(offset);
            runStats.subsumedByVE += subsumeStrengthen->subsume0(offset);
        } else if (finalLits.size() == 3 || finalLits.size() == 2) {
            //Subsume long
            SubsumeStrengthen::Sub0Ret ret = subsumeStrengthen->subsume0AndUnlink(
                std::numeric_limits<uint32_t>::max() //Index of this implicit clause (non-existent)
                , finalLits //Literals in this binary clause
                , calcAbstraction(finalLits) //Abstraction of literals
                , true //subsume implicit ones
            );
            runStats.subsumedByVE += ret.numSubsumed;
            if (ret.numSubsumed > 0) {
                if (solver->conf.verbosity >= 5) {
                    cout << "Subsumed: " << ret.numSubsumed << endl;
                }
            }
        }

        //Touch every var of the new clause, so we re-estimate
        //elimination complexity for this var
        for(vector<Lit>::const_iterator
            it3 = finalLits.begin(), end3 = finalLits.end()
            ; it3 != end3
            ; it3++
        ) {
            touched.touch(*it3);
        }
    }

    //Update var elim complexity heap
    if (solver->conf.updateVarElimComplexityOTF) {
        for(vector<Var>::const_iterator
            it = touched.getTouchedList().begin()
            , end = touched.getTouchedList().end()
            ; it != end
            ; it++
        ) {
            //No point in updating the score of this var
            //it's eliminated already, or not to be eliminated at all
            if (*it == var
                || !varElimOrder.inHeap(*it)
                || solver->value(*it) != l_Undef
                || solver->varData[*it].removed != Removed::none
            ) {
                continue;
            }

            varElimComplexity[*it] = strategyCalcVarElimScore(*it);;
            varElimOrder.update(*it);
        }
    }

end:
    if (solver->conf.verbosity >= 5) {
        cout << "Elimination of var " << lit << " finished " << endl;
    }

    var_elimed[var] = true;
    solver->varData[var].removed = Removed::elimed;
    runStats.numVarsElimed++;
    solver->unsetDecisionVar(var);

    return solver->ok;
}

/*void Simplifier::addRedBinaries(const Var var)
{
    vector<Lit> tmp(2);
    Lit lit = Lit(var, false);
    const vec<Watched>& ws = solver->watches[(~lit).toInt()];
    const vec<Watched>& ws2 = solver->watches[lit.toInt()];

    for (vec<Watched>::const_iterator w1 = ws.begin(), end1 = ws.end(); w1 != end1; w1++) {
        if (!w1->isBinary()) continue;
        const bool numOneIsRed = w1->red();
        const Lit lit1 = w1->lit2();
        if (solver->value(lit1) != l_Undef || var_elimed[lit1.var()]) continue;

        for (vec<Watched>::const_iterator w2 = ws2.begin(), end2 = ws2.end(); w2 != end2; w2++) {
            if (!w2->isBinary()) continue;
            const bool numTwoIsRed = w2->red();
            if (!numOneIsRed && !numTwoIsRed) {
                //At least one must be redundant
                continue;
            }

            const Lit lit2 = w2->lit2();
            if (solver->value(lit2) != l_Undef || var_elimed[lit2.var()]) continue;

            tmp[0] = lit1;
            tmp[1] = lit2;
            Clause* tmpOK = solver->addClauseInt(tmp, true);
            runStats.numRedBinVarRemAdded++;
            release_assert(tmpOK == NULL);
            release_assert(solver->ok);
        }
    }
    assert(solver->value(lit) == l_Undef);
}*/

/**
@brief Resolves two clauses on a variable

Clause ps must contain without_p
Clause ps must contain without_q
And without_p = ~without_q

@return FALSE if clause is always satisfied ('out_clause' should not be used)
*/
bool Simplifier::merge(
    const Watched& ps
    , const Watched& qs
    , const Lit noPosLit
    , const bool aggressive
) {
    //If clause has already been freed, skip
    if (ps.isClause()
        && solver->clAllocator->getPointer(ps.getOffset())->freed()
    ) {
        return false;
    }
    if (qs.isClause()
        && solver->clAllocator->getPointer(qs.getOffset())->freed()
    ) {
        return false;
    }

    dummy.clear(); //The final clause
    toClear.clear(); //Used to clear 'seen'

    //Handle PS
    bool retval = true;
    if (ps.isBinary() || ps.isTri()) {
        *toDecrease -= 1;
        assert(ps.lit2() != noPosLit);

        seen[ps.lit2().toInt()] = 1;
        dummy.push_back(ps.lit2());
    }

    if (ps.isTri()) {
        assert(ps.lit2() < ps.lit3());

        seen[ps.lit3().toInt()] = 1;
        dummy.push_back(ps.lit3());
    }

    if (ps.isClause()) {
        Clause& cl = *solver->clAllocator->getPointer(ps.getOffset());
        //assert(!clauseData[ps.clsimp.index].defOfOrGate);
        *toDecrease -= cl.size();
        for (uint32_t i = 0; i < cl.size(); i++){
            //Skip noPosLit
            if (cl[i] == noPosLit)
                continue;

            seen[cl[i].toInt()] = 1;
            dummy.push_back(cl[i]);
        }
    }

    //Handle QS
    if (qs.isBinary() || qs.isTri()) {
        *toDecrease -= 2;
        assert(qs.lit2() != ~noPosLit);

        if (seen[(~qs.lit2()).toInt()]) {
            retval = false;
            toClear = dummy;
            goto end;
        }
        if (!seen[qs.lit2().toInt()]) {
            dummy.push_back(qs.lit2());
            seen[qs.lit2().toInt()] = 1;
        }
    }

    if (qs.isTri()) {
        assert(qs.lit2() < qs.lit3());

        if (seen[(~qs.lit3()).toInt()]) {
            retval = false;
            toClear = dummy;
            goto end;
        }
        if (!seen[qs.lit3().toInt()]) {
            dummy.push_back(qs.lit3());
            seen[qs.lit3().toInt()] = 1;
        }
    }

    if (qs.isClause()) {
        Clause& cl = *solver->clAllocator->getPointer(qs.getOffset());
        //assert(!clauseData[qs.clsimp.index].defOfOrGate);
        *toDecrease -= cl.size();
        for (uint32_t i = 0; i < cl.size(); i++){

            //Skip ~noPosLit
            if (cl[i] == ~noPosLit)
                continue;

            //Opposite is inside, nothing to add
            if (seen[(~cl[i]).toInt()]) {
                retval = false;
                toClear = dummy;
                goto end;
            }

            //Add the literal
            if (!seen[cl[i].toInt()]) {
                dummy.push_back(cl[i]);
                seen[cl[i].toInt()] = 1;
            }
        }
    }
    toClear = dummy;

    //We add to 'seen' what COULD be added to the clause
    //This is essentially the reverse of cache-based vivification
    if (aggressive
        && solver->conf.doAsymmTE
    ) {
        for (size_t i = 0
            ; i < dummy.size() && numMaxVarElimAgressiveCheck > 0
            ; i++
        ) {
            numMaxVarElimAgressiveCheck -= 3;
            const Lit lit = toClear[i];
            assert(lit.var() != noPosLit.var());

            //Use cache
            if (!ps.isBinary()
                && !qs.isBinary()
                && solver->conf.doCache
            ) {
                const vector<LitExtra>& cache = solver->implCache[lit.toInt()].lits;
                numMaxVarElimAgressiveCheck -= cache.size()/3;
                for(vector<LitExtra>::const_iterator
                    it = cache.begin(), end = cache.end()
                    ; it != end
                    ; it++
                ) {
                    //If redundant, that doesn't help
                    if (!it->getOnlyIrredBin())
                        continue;

                    const Lit otherLit = it->getLit();
                    if (otherLit.var() == noPosLit.var())
                        continue;

                    //If (a) was in original clause
                    //then (a V b) means -b can be put inside
                    if (!seen[(~otherLit).toInt()]) {
                        toClear.push_back(~otherLit);
                        seen[(~otherLit).toInt()] = 1;
                    }

                    //If (a V b) is irred in the clause, then done
                    if (seen[otherLit.toInt()]) {
                        retval = false;
                        goto end;
                    }
                }
            }

            /*
            //TODO
            //Use watchlists
            if (numMaxVarElimAgressiveCheck > 0) {
                if (agressiveCheck(lit, noPosLit, retval))
                    goto end;
            }*/
        }
    }

    if (aggressive
        && solver->conf.doAsymmTE
        && solver->conf.doStamp
    ) {
        //Use stamping
        //but only if none of the clauses were binary
        //Otherwise we cannot tell if the value in the cache is dependent
        //on the binary clause itself, so that would cause a circular de-
        //pendency

        if (!ps.isBinary() && !qs.isBinary()) {
            numMaxVarElimAgressiveCheck -= 20;
            if (solver->stamp.stampBasedClRem(toClear)) {
                goto end;
            }
        }
    }

    end:
    //Clear 'seen'
    *toDecrease -= toClear.size()/2 + 1;
    for (vector<Lit>::const_iterator
        it = toClear.begin(), end = toClear.end()
        ; it != end
        ; it++
    ) {
        seen[it->toInt()] = 0;
    }

    return retval;
}

bool Simplifier::agressiveCheck(
    const Lit lit
    , const Lit noPosLit
    , bool& retval
) {
    const vec<Watched>& ws = solver->watches[lit.toInt()];
    numMaxVarElimAgressiveCheck -= ws.size()/3 + 2;
    for(vec<Watched>::const_iterator it =
        ws.begin(), end = ws.end()
        ; it != end
        ; it++
    ) {
        //Can't do much with clauses, too expensive
        if (it->isClause())
            continue;

        //handle tri
        if (it->isTri() && !it->red()) {

            //See if any of the literals is in
            Lit otherLit = lit_Undef;
            unsigned inside = 0;
            if (seen[it->lit2().toInt()]) {
                otherLit = it->lit3();
                inside++;
            }

            if (seen[it->lit3().toInt()]) {
                otherLit = it->lit2();
                inside++;
            }

            //Could subsume
            if (inside == 2) {
                retval = false;
                return true;
            }

            //None is in, skip
            if (inside == 0)
                continue;

            if (otherLit.var() == noPosLit.var())
                continue;

            //Extend clause
            if (!seen[(~otherLit).toInt()]) {
                toClear.push_back(~otherLit);
                seen[(~otherLit).toInt()] = 1;
            }

            continue;
        }

        //Handle binary
        if (it->isBinary() && !it->red()) {
            const Lit otherLit = it->lit2();
            if (otherLit.var() == noPosLit.var())
                continue;

            //If (a V b) is irred, and in the clause, then we can remove
            if (seen[otherLit.toInt()]) {
                retval = false;
                return true;
            }

            //If (a) is in clause
            //then (a V b) means -b can be put inside
            if (!seen[(~otherLit).toInt()]) {
                toClear.push_back(~otherLit);
                seen[(~otherLit).toInt()] = 1;
            }
        }
    }

    return false;
}

Simplifier::HeuristicData Simplifier::calcDataForHeuristic(const Lit lit)
{
    HeuristicData ret;

    const vec<Watched>& ws = solver->watches[lit.toInt()];
    *toDecrease -= ws.size() + 100;
    for (vec<Watched>::const_iterator
        it = ws.begin(), end = ws.end()
        ; it != end
        ; it++
    ) {
        //Handle binary
        if (it->isBinary())
        {
            //Only count irred
            if (!it->red()) {
                ret.bin++;
                ret.lit += 2;
            }
            continue;
        }

        //Handle tertiary
        if (it->isTri()) {
            //Only count irred
            if (!it->red()) {
                ret.tri++;
                ret.lit += 3;
            }

            continue;
        }

        if (it->isClause()) {
            const Clause* cl = solver->clAllocator->getPointer(it->getOffset());

            //If in occur then it cannot be freed
            assert(!cl->freed());

            //Only irred is of relevance
            if (!cl->red()) {
                ret.longer++;
                ret.lit += cl->size();
            }

            continue;
        }

        //Only the two above types are there
        assert(false);
    }

    return ret;
}

bool Simplifier::checkEmptyResolvent(const Lit lit)
{
    size_t num_bits_set = checkEmptyResolventHelper(
        lit
        , ResolventCountAction::set
        , 0
    );

    size_t num_resolvents = std::numeric_limits<size_t>::max();

    //Can only count if the POS was small enough
    //otherwise 'seen' cannot properly store the data
    if (num_bits_set > sizeof(unsigned char)*8) {
        num_resolvents = checkEmptyResolventHelper(
            ~lit
            , ResolventCountAction::count
            , num_bits_set
        );
    }

    //Clear the 'seen' array
    checkEmptyResolventHelper(
        lit
        , ResolventCountAction::unset
        , 0
    );

    //Okay, this would be great
    return (num_resolvents == 0);
}


bool Simplifier::checkEmptyResolventHelper(
    const Lit lit
    , ResolventCountAction action
    , size_t otherSize
) {
    unsigned char at = 1;
    size_t count = 0;
    size_t numCls = 0;

    const vec<Watched>& ws = solver->watches[lit.toInt()];
    *toDecrease -= ws.size() + 100;
    for (vec<Watched>::const_iterator
        it = ws.begin(), end = ws.end()
        ; it != end
        ; it++
    ) {
        //Handle binary
        if (it->isBinary()){
            //Only count irred
            if (!it->red()) {
                numCls++;

                switch(action) {
                    case ResolventCountAction::set:
                        seen[it->lit2().toInt()] |= at;
                        at <<= 1;
                        break;

                    case ResolventCountAction::unset:
                        seen[it->lit2().toInt()] = 0;
                        break;

                    case ResolventCountAction::count:
                        count += otherSize - __builtin_popcount(seen[(~it->lit2()).toInt()]);
                        break;
                }
            }
            continue;
        }

        //Handle tertiary
        if (it->isTri()) {
            //Only count irred
            if (!it->red()) {
                numCls++;

                switch(action) {
                    case ResolventCountAction::set:
                        seen[it->lit2().toInt()] |= at;
                        seen[it->lit3().toInt()] |= at;
                        at <<= 1;
                        break;

                    case ResolventCountAction::unset:
                        seen[it->lit2().toInt()] = 0;
                        seen[it->lit3().toInt()] = 0;
                        break;

                    case ResolventCountAction::count:
                        unsigned tmp = seen[(~it->lit2()).toInt()] | seen[(~it->lit3()).toInt()];
                        count += otherSize - __builtin_popcount(tmp);
                        break;
                }
            }

            continue;
        }

        if (it->isClause()) {
            const Clause* cl = solver->clAllocator->getPointer(it->getOffset());

            //If in occur then it cannot be freed
            assert(!cl->freed());

            //Only irred is of relevance
            if (!cl->red()) {
                numCls++;
                unsigned tmp = 0;
                for(size_t i = 0; i < cl->size(); i++) {
                    const Lit lit = (*cl)[i];

                    switch (action) {
                        case ResolventCountAction::set:
                            seen[lit.toInt()] |= at;
                            break;

                        case ResolventCountAction::unset:
                            seen[lit.toInt()] = 0;
                            break;

                        case ResolventCountAction::count:
                            tmp |= seen[(~lit).toInt()];
                            break;
                    }
                }

                //move setit data along
                if (action == ResolventCountAction::set) {
                    at <<= 1;
                }

                //Count using tmp
                if (action == ResolventCountAction::count) {
                    count += otherSize - __builtin_popcount(tmp);
                }
            }

            continue;
        }

        //Only these types are possible
        assert(false);
    }


    switch(action) {
        case ResolventCountAction::count:
            return numCls;

        case ResolventCountAction::set:
            return 0;

        case ResolventCountAction::unset:
            return 0;
    }

    return count;
}



pair<int, int> Simplifier::heuristicCalcVarElimScore(const Var var)
{
    const Lit lit(var, false);
    const HeuristicData pos = calcDataForHeuristic(lit);
    const HeuristicData neg = calcDataForHeuristic(~lit);

    //Estimate cost
    int posTotalLonger = pos.longer + pos.tri;
    int negTotalLonger = neg.longer + neg.tri;
    int normCost;
    switch(solver->conf.varElimCostEstimateStrategy) {
        case 0:
            normCost =  posTotalLonger*negTotalLonger
                + pos.bin*negTotalLonger*2
                + neg.bin*posTotalLonger*2
                + pos.bin*neg.bin*3;
            break;

        case 1:
            normCost =  posTotalLonger*negTotalLonger
                + pos.bin*negTotalLonger*2
                + neg.bin*posTotalLonger*2
                + pos.bin*neg.bin*4;
            break;

        default:
            cout
            << "ERROR: Invalid var-elim cost estimation strategy"
            << endl;
            exit(-1);
            break;
    }


    /*if ((pos.longer + pos.tri + pos.bin) <= 2
        && (neg.longer + neg.tri + neg.bin) <= 2
    ) {
        normCost /= 2;
    }*/

    if ((pos.longer + pos.tri + pos.bin) == 0
        || (neg.longer + neg.tri + neg.bin) == 0
    ) {
        normCost = 0;
    }

    int litCost = pos.lit * neg.lit;

    return std::make_pair(normCost, litCost);
}

void Simplifier::orderVarsForElimInit()
{
    varElimOrder.clear();
    varElimComplexity.clear();
    varElimComplexity.resize(
        solver->nVars()
        , std::make_pair<int, int>(1000, 1000)
    );

    //Go through all vars
    for (
        size_t var = 0
        ; var < solver->nVars() && *toDecrease > 0
        ; var++
    ) {
        *toDecrease -= 50;

        //Can this variable be eliminated at all?
        if (solver->value(var) != l_Undef
            || solver->varData[var].removed != Removed::none
            //|| !gateFinder->canElim(var)
        ) {
            continue;
        }

        assert(!varElimOrder.inHeap(var));
        varElimComplexity[var] = strategyCalcVarElimScore(var);;
        varElimOrder.insert(var);
    }
    assert(varElimOrder.heapProperty());

    //Print sorted listed list
    #ifdef VERBOSE_DEBUG_VARELIM
    cout << "-----------" << endl;
    for(size_t i = 0; i < varElimOrder.size(); i++) {
        cout
        << "varElimOrder[" << i << "]: "
        << " var: " << varElimOrder[i]+1
        << " val: " << varElimComplexity[varElimOrder[i]].first
        << " , " << varElimComplexity[varElimOrder[i]].second
        << endl;
    }
    #endif
}

std::pair<int, int> Simplifier::strategyCalcVarElimScore(const Var var)
{
    std::pair<int, int> cost;
    if (solver->conf.varelimStrategy == 0) {
        cost = heuristicCalcVarElimScore(var);
    } else {
        int ret = testVarElim(var);

        cost.first = ret;
        cost.second = 0;
    }

    return cost;
}

inline bool Simplifier::checkBlocked(const Lit lit)
{
    //clauses which contain '~lit'
    const vec<Watched>& ws = solver->watches[(~lit).toInt()];
    for (vec<Watched>::const_iterator
        it = ws.begin(), end = ws.end()
        ; it != end
        ; it++
    ) {
        *toDecrease -= 2;

        //Handle binary
        if (it->isBinary() && !it->red()) {
            if (seen[(~it->lit2()).toInt()]) {
                assert(it->lit2() != ~lit);
                continue;
            }
            return false;
        }

        //Handle tertiary
        if (it->isTri() && !it->red()) {
            assert(it->lit2() < it->lit3());
            if (seen[(~it->lit2()).toInt()]
                || seen[(~it->lit3()).toInt()]
            ) {
                continue;
            }
            return false;
        }

        //Handle long clause
        if (it->isClause()) {
            const Clause& cl = *solver->clAllocator->getPointer(it->getOffset());

            //Only irred
            if (cl.red())
                continue;

            *toDecrease -= 10;
            for (const Lit *l = cl.begin(), *end2 = cl.end(); l != end2; l++) {
                *toDecrease -= 1;
                if (seen[(~(*l)).toInt()] && *l != ~lit) {
                    goto next;
                }
            }

            return false;
        }

        next:
        ;
    }

    return true;
}

void Simplifier::checkElimedUnassigned() const
{
    for (size_t i = 0; i < var_elimed.size(); i++) {
        if (var_elimed[i]) {
            assert(solver->assigns[i] == l_Undef);
        }
    }
}

void Simplifier::checkElimedUnassignedAndStats() const
{
    assert(solver->ok);
    int64_t checkNumElimed = 0;
    for (size_t i = 0; i < var_elimed.size(); i++) {
        if (var_elimed[i]) {
            checkNumElimed++;
            assert(solver->value(i) == l_Undef);
        }
    }
    if (globalStats.numVarsElimed != checkNumElimed) {
        cout
        << "ERROR: globalStats.numVarsElimed is "<<
        globalStats.numVarsElimed
        << " but checkNumElimed is: " << checkNumElimed
        << endl;

        assert(false);
    }
}

void Simplifier::print_elimed_vars() const
{
    for(size_t i = 0; i < var_elimed.size(); i++) {
        if (var_elimed[i]) {
            cout << "Elimed var: " << i+1 << endl;
        }
    }
}

size_t Simplifier::memUsed() const
{
    size_t b = 0;
    b += seen.capacity()*sizeof(char);
    b += seen2.capacity()*sizeof(char);
    b += dummy.capacity()*sizeof(char);
    b += toClear.capacity()*sizeof(Lit);
    b += finalLits.capacity()*sizeof(Lit);
    b += subsumeStrengthen->memUsed();

    b += var_elimed.capacity()*sizeof(char); //TODO wrong, because of template specialization of bit-array
    for(map<Var, vector<size_t> >::const_iterator
        it = blk_var_to_cl.begin(), end = blk_var_to_cl.end()
        ; it != end
        ; it++
    ) {
        b += it->second.capacity()*sizeof(size_t);
    }
    b += blockedClauses.capacity()*sizeof(BlockedClause);
    for(vector<BlockedClause>::const_iterator
        it = blockedClauses.begin(), end = blockedClauses.end()
        ; it != end
        ; it++
    ) {
        b += it->lits.capacity()*sizeof(Lit);
    }
    b += blk_var_to_cl.size()*(sizeof(Var)+sizeof(vector<size_t>)); //TODO under-counting
    b += varElimOrder.memUsed();
    b += varElimComplexity.capacity()*sizeof(int)*2;
    b += touched.memUsed();
    b += clauses.capacity()*sizeof(ClOffset);

    return b;
}

uint64_t Simplifier::memUsedXor() const
{
    #ifdef USE_M4RI
    if (xorFinder) {
        return xorFinder->memUsed();
    } else {
        return 0;
    }
    #else
    return 0;
    #endif
}

void Simplifier::freeXorMem()
{
    delete xorFinder;
    xorFinder = NULL;
}

void Simplifier::linkInClause(Clause& cl)
{
    assert(cl.size() > 3);
    ClOffset offset = solver->clAllocator->getOffset(&cl);
    std::sort(cl.begin(), cl.end());
    for (uint32_t i = 0; i < cl.size(); i++) {
        vec<Watched>& ws = solver->watches[cl[i].toInt()];
        *toDecrease -= ws.size();

        ws.push(Watched(offset, cl.abst));
    }
    assert(cl.abst == calcAbstraction(cl));
    cl.setOccurLinked(true);
}

/*const GateFinder* Simplifier::getGateFinder() const
{
    return gateFinder;
}*/
