/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos and collaborators. All rights reserved.
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

#include "gatefinder.h"
#include "time_mem.h"
#include "solver.h"
#include "simplifier.h"

using namespace CMSat;
using std::cout;
using std::endl;

GateFinder::GateFinder(Simplifier *_subsumer, Solver *_solver) :
    numDotPrinted(0)
    , subsumer(_subsumer)
    , solver(_solver)
    , seen(_subsumer->seen)
    , seen2(_subsumer->seen2)
{}

bool GateFinder::doAll()
{
    runStats.clear();
    findOrGates();
    if (!doAllOptimisationWithGates())
        goto end;

    if (solver->conf.doPrintGateDot)
        printDot();

    //Do ER with randomly piced variables as gates
    /*if (solver->conf.doER) {
        const uint32_t addedVars = createNewVars();
        //Play with the newly found gates
        if (addedVars > 0 && !doAllOptimisationWithGates())
            goto end;
    }*/

    //TODO enable below
    /*if (solver->conf.doFindXors && solver->conf.doMixXorAndGates) {
        if (!mixXorAndGates())
            goto end;
    }*/


end:
    //Stats
    if (solver->conf.verbosity >= 1) {
        if (solver->conf.verbosity >= 3) {
            runStats.print(solver->nVars());
            printGateStats();
        } else {
            runStats.printShort();
        }
    }
    globalStats += runStats;

    return solver->ok;
}

/*uint32_t GateFinder::createNewVars()
{
    double myTime = cpuTime();
    vector<NewGateData> newGates;
    vector<Lit> tmp;
    vector<ClOffset> subs;
    uint64_t numOp = 0;
    subsumer->toDecrease = &numMaxCreateNewVars;

    const size_t size = solver->getNumFreeVars()-1;

    size_t tries = 0;
    for (; tries < std::min<size_t>(100000U, size*size/2); tries++) {
        if (*subsumer->toDecrease < 50LL*1000LL*1000LL)
            break;

        //Take some variables randomly
        const Var var1 = solver->mtrand.randInt(size);
        const Var var2 = solver->mtrand.randInt(size);

        //Check that var1 & var2 are sane choices (not equivalent, not removed, etc.)
        if (var1 == var2)
            continue;

        if (solver->value(var1) != l_Undef
            || !solver->decisionVar[var1]
            || solver->varData[var1].removed != Removed::none
            ) continue;

        if (solver->value(var2) != l_Undef
            || !solver->decisionVar[var2]
            || solver->varData[var2].removed != Removed::none
            ) continue;

        //Pick sign randomly
        Lit lit1 = Lit(var1, solver->mtrand.randInt(1));
        Lit lit2 = Lit(var2, solver->mtrand.randInt(1));

        //Make sure they are in the right order
        if (lit1 > lit2)
            std::swap(lit1, lit2);

        //See how many clauses this binary gate would shorten
        tmp.clear();
        tmp.push_back(lit1);
        tmp.push_back(lit2);
        subs.clear();
        subsumer->findSubsumed0(std::numeric_limits< uint32_t >::max(), tmp, calcAbstraction(tmp), subs);

        //See how many clauses this binary gate would allow us to remove
        uint32_t potential = 0;
        if (numOp < 100*1000*1000) {
            vector<Lit> lits;
            lits.push_back(lit1);
            lits.push_back(lit2);
            OrGate gate(lits, Lit(0,false), false);
            treatAndGate(gate, false, potential, numOp);
        }

        //If we find the above to be adequate, then this should be a new gate
        if (potential > 5 || subs.size() > 100
            || (potential > 1 && subs.size() > 50)) {
            newGates.push_back(NewGateData(lit1, lit2, subs.size(), potential));
        }
    }

    //Rank the potentially new gates
    std::sort(newGates.begin(), newGates.end());
    newGates.erase(std::unique(newGates.begin(), newGates.end() ), newGates.end() );

    //Add the new gates
    uint64_t addedVars = 0;
    for (uint32_t i = 0; i < newGates.size(); i++) {
        const NewGateData& n = newGates[i];
        if ((i > 50 && n.numLitRem < 1000 && n.numClRem < 25)
            || i > ((double)solver->getNumFreeVars()*0.01)
            || i > 100) break;

        const Var newVar = solver->newVar();
        dontElim[newVar] = true;
        const Lit newLit = Lit(newVar, false);
        vector<Lit> lits;
        lits.push_back(n.lit1);
        lits.push_back(n.lit2);
        OrGate gate(lits, newLit, false);
        orGates.push_back(gate);
        gateOccEq[gate.eqLit.toInt()].push_back(orGates.size()-1);
        for (uint32_t i = 0; i < gate.lits.size(); i++) {
            gateOcc[gate.lits[i].toInt()].push_back(orGates.size()-1);
        }

        tmp.clear();
        tmp.push_back(newLit);
        tmp.push_back(~n.lit1);
        Clause* cl = solver->addClauseInt(tmp);
        assert(cl == NULL);
        assert(solver->ok);

        tmp.clear();
        tmp.push_back(newLit);
        tmp.push_back(~n.lit2);
        cl = solver->addClauseInt(tmp);
        assert(cl == NULL);
        assert(solver->ok);

        tmp.clear();
        tmp.push_back(~newLit);
        tmp.push_back(n.lit1);
        tmp.push_back(n.lit2);
        cl = solver->addClauseInt(tmp, false, ClauseStats(), false);
        assert(cl != NULL);
        assert(solver->ok);
        cl->stats.conflictNumIntroduced = solver->sumStats.conflStats.numConflicts;
        subsumer->linkInClause(*cl);
        cl->defOfOrGate = true;

        addedVars++;
    }

    if (solver->conf.verbosity >= 1) {
        cout
        << "c Added vars :" << addedVars
        << " tried: " << tries
        << " time: " << (cpuTime() - myTime) << endl;
    }
    runStats.numERVars += addedVars;
    runStats.erTime += cpuTime() - myTime;

    return addedVars;
}*/

void GateFinder::findOrGates()
{
    assert(solver->ok);

    double myTime = cpuTime();
    clearIndexes();
    numMaxGateFinder = 100LL*1000LL*1000LL;
    subsumer->toDecrease = &numMaxGateFinder;

    findOrGates(true);

    for(const auto orgate: orGates) {
        if (orgate.red) {
            runStats.learntGatesSize += orgate.lits.size();
            runStats.numRed++;
        } else  {
            runStats.irredGatesSize += orgate.lits.size();
            runStats.numNonRed++;
        }
    }
    runStats.findGateTime += cpuTime() - myTime;
}

void GateFinder::printGateStats() const
{
    uint32_t gateOccNum = 0;
    for (vector<vector<uint32_t> >::const_iterator
        it = gateOcc.begin(), end = gateOcc.end()
        ; it != end
        ; it++
    ) {
        gateOccNum += it->size();
    }

    uint32_t gateOccEqNum = 0;
    for (vector<vector<uint32_t> >::const_iterator
        it = gateOccEq.begin(), end = gateOccEq.end()
        ; it != end
        ; it++
    ) {
        gateOccEqNum += it->size();
    }

    uint32_t gateNum = 0;
    for (vector<OrGate>::const_iterator
        it = orGates.begin(), end = orGates.end()
        ; it != end
        ; it++
    ) {
        gateNum += !it->removed;
    }

    cout << "c gateOcc num: " << gateOccNum
    << " gateOccEq num: " << gateOccEqNum
    << " gates size: " << gateNum << endl;
}

void GateFinder::clearIndexes()
{
    //Clear gate definitions -- this will let us do more, because essentially
    //the other gates are not fully forgotten, so they don't bother us at all
    for (vector<ClOffset>::iterator
        it = solver->longIrredCls.begin(), end = solver->longIrredCls.end()
        ; it != end
        ; it++
    ) {
        Clause* cl = solver->clAllocator->getPointer(*it);
        cl->defOfOrGate = false;
    }

    //Clear gate statistics
    orGates.clear();
    for (size_t i = 0; i < gateOcc.size(); i++)
        gateOcc[i].clear();
    for (size_t i = 0; i < gateOccEq.size(); i++)
        gateOccEq[i].clear();
}

bool GateFinder::doAllOptimisationWithGates()
{
    assert(solver->ok);

    //OR gate treatment
    if (solver->conf.doShortenWithOrGates) {
        //Setup
        double myTime = cpuTime();
        numMaxShortenWithGates = 100LL*1000LL*1000LL;
        subsumer->toDecrease = &numMaxShortenWithGates;
        runStats.numLongCls = subsumer->runStats.origNumIrredLongClauses +
            subsumer->runStats.origNumRedLongClauses;
        runStats.numLongClsLits = solver->binTri.irredLits + solver->binTri.redLits;

        //Go through each gate, see if we can do something with it
        for (const auto& gate: orGates) {
            //Gate removed, skip
            if (gate.removed)
                continue;

            //Time is up!
            if (*subsumer->toDecrease < 0) {
                if (solver->conf.verbosity >= 2) {
                    cout
                    << "c No more time left for shortening with gates"
                    << endl;
                }
                break;
            }

            if (!shortenWithOrGate(gate))
                break;
        }

        //Handle results
        runStats.orBasedTime += cpuTime() - myTime;

        if (!solver->ok)
            return false;
    }

    //AND gate treatment
    if (solver->conf.doRemClWithAndGates) {
        //Setup
        numMaxClRemWithGates = 100LL*1000LL*1000LL;
        subsumer->toDecrease = &numMaxClRemWithGates;
        double myTime = cpuTime();

        //Do clause removal
        uint32_t foundPotential;
        uint64_t numOp = 0;

        //Go through each gate, see if we can do something with it
        for (const auto& gate: orGates) {
            //Gate has been removed, or is too large
            if (gate.removed || gate.lits.size() >2)
                continue;

            //Time's up?
            if (*subsumer->toDecrease < 0) {
                if (solver->conf.verbosity >= 2) {
                    cout
                    << "c No more time left for cl-removal with gates" << endl;
                }
                break;
            }

            if (!treatAndGate(gate, true, foundPotential, numOp))
                break;
        }


        //Handle results
        runStats.andBasedTime += cpuTime() - myTime;

        if (!solver->ok)
            return false;
    }

    //EQ gate treatment
    if (solver->conf.doFindEqLitsWithGates) {
        //Setup
        double myTime = cpuTime();

        //Do equivalence checking
        runStats.varReplaced += findEqOrGates();

        //Handle results
        runStats.varReplaceTime += cpuTime() - myTime;

        if (!solver->ok)
            return false;
    }

    return solver->ok;
}

size_t GateFinder::findEqOrGates()
{
    assert(solver->ok);
    size_t foundRep = 0;
    vector<OrGate> gates = orGates;
    std::sort(gates.begin(), gates.end(), OrGateSorter2());

    vector<Lit> tmp(2);
    for (uint32_t i = 1; i < gates.size(); i++) {
        const OrGate& gate1 = gates[i-1];
        const OrGate& gate2 = gates[i];
        if (gate1.removed || gate2.removed) continue;

        if (gate1.lits == gate2.lits
            && gate1.eqLit.var() != gate2.eqLit.var()
       ) {
            foundRep++;
            tmp[0] = gate1.eqLit.unsign();
            tmp[1] = gate2.eqLit.unsign();
            const bool RHS = gate1.eqLit.sign() ^ gate2.eqLit.sign();
            if (!solver->addXorClauseInt(tmp, RHS, false))
                return foundRep;
            tmp.resize(2);
        }
    }

    return foundRep;
}

void GateFinder::findOrGates(const bool redGatesToo)
{
    //Goi through each clause
    for (vector<ClOffset>::iterator
        it = solver->longIrredCls.begin(), end = solver->longIrredCls.end()
        ; it != end
        ; it++
    ) {
        //Ran out of time
        if (*subsumer->toDecrease < 0) {
            if (solver->conf.verbosity >= 1) {
                cout << "c Finishing gate-finding: ran out of time" << endl;
            }
            break;
        }

        const Clause& cl = *solver->clAllocator->getPointer(*it);

        //Clause removed
        if (cl.freed())
            continue;

        //If clause is larger than the cap on gate size, skip. Only for speed reasons.
        if (cl.size() > solver->conf.maxGateSize)
            continue;

        //if no learnt gates are allowed and this is learnt, skip
        if (!learntGatesToo && cl.red())
            continue;

        const bool wasRed = cl.red();

        //Check how many literals have zero cache&binary clause
        //If too many, it cannot possibly be an OR gate
        uint8_t numSizeZero = 0;
        for (const Lit *l = cl.begin(), *end2 = cl.end(); l != end2; l++) {
            Lit lit = *l;
            //TODO stamping
            //const vector<LitExtra>& cache = solver->implCache[(~lit).toInt()].lits;
            const vec<Watched>& ws = solver->watches[(~lit).toInt()];

            if (
                //cache.size() == 0 &&
                ws.size() == 0) {
                numSizeZero++;
                if (numSizeZero > 1)
                    break;
            }
        }
        if (numSizeZero > 1)
            continue;

        //Try to find a gate with eqlit (~*l)
        ClOffset offset = solver->clAllocator->getOffset(&cl);
        for (const Lit *l = cl.begin(), *end2 = cl.end(); l != end2; l++)
            findOrGate(~*l, offset, learntGatesToo, wasRed);
    }
}

void GateFinder::findOrGate(
    const Lit eqLit
    , const ClOffset offset
    , const bool redGatesToo
    , bool wasRed
) {
    Clause& cl = *solver->clAllocator->getPointer(offset);
    bool isEqual = true;
    for (const Lit *l2 = cl.begin(), *end3 = cl.end(); l2 != end3; l2++) {
        //We are NOT looking for the literal that is on the RHS
        if (*l2 == ~eqLit)
            continue;

        //This is the other lineral in the binary clause
        //We are looking for a binary clause '~otherlit V eqLit'
        const Lit otherLit = *l2;
        bool OK = false;

        //Try to find corresponding binary clause in cache
        //TODO stamping
        /*const vector<LitExtra>& cache = solver->implCache[(~otherLit).toInt()].lits;
        *subsumer->toDecrease -= cache.size();
        for (vector<LitExtra>::const_iterator
            cacheLit = cache.begin(), endCache = cache.end()
            ; cacheLit != endCache && !OK
            ; cacheLit++
        ) {
            if ((learntGatesToo || cacheLit->getOnlyIrredBin())
                 && cacheLit->getLit() == eqLit
            ) {
                wasRed |= !cacheLit->getOnlyIrredBin();
                OK = true;
            }
        }*/

        //Try to find corresponding binary clause in watchlist
        const vec<Watched>& ws = solver->watches[(~otherLit).toInt()];
        *subsumer->toDecrease -= ws.size();
        for (vec<Watched>::const_iterator
            wsIt = ws.begin(), endWS = ws.end()
            ; wsIt != endWS && !OK
            ; wsIt++
        ) {
            //Only binary clauses are of importance
            if (!wsIt->isBinary())
                continue;

            if ((learntGatesToo || !wsIt->red())
                 && wsIt->lit2() == eqLit
            ) {
                wasRed |= wsIt->red();
                OK = true;
            }
        }

        //We have to find the binary clause. If not, this is not a gate
        if (!OK) {
            isEqual = false;
            break;
        }
    }

    //Does not make a gate, return
    if (!isEqual)
        return;

    //Create gate
    vector<Lit> lits;
    for (const Lit *l2 = cl.begin(), *end3 = cl.end(); l2 != end3; l2++) {

        //Don't include RHS
        if (*l2 == ~eqLit)
            continue;

        lits.push_back(*l2);
    }
    OrGate gate(lits, eqLit, wasRed);

    //Find if there are any gates that are the same
    const vector<uint32_t>& similar = gateOccEq[gate.eqLit.toInt()];
    for (vector<uint32_t>::const_iterator it = similar.begin(), end = similar.end(); it != end; it++) {
        //The same gate? Then froget about this
        if (orGates[*it] == gate)
            return;
    }

    //Add gate
    *subsumer->toDecrease -= gate.lits.size()*2;
    orGates.push_back(gate);
    cl.defOfOrGate = true;
    gateOccEq[gate.eqLit.toInt()].push_back(orGates.size()-1);
    if (!wasRed) {
        for (uint32_t i = 0; i < gate.lits.size(); i++) {
            Lit lit = gate.lits[i];
            gateOcc[lit.toInt()].push_back(orGates.size()-1);
        }
    }

    #ifdef VERBOSE_ORGATE_REPLACE
    cout << "Found gate : " << gate << endl;
    #endif
}

bool GateFinder::shortenWithOrGate(const OrGate& gate)
{
    assert(solver->ok);

    //Find clauses that potentially could be shortened
    vector<ClOffset> subs;
    subsumer->findSubsumed0(
        std::numeric_limits< uint32_t >::max()
        , gate.lits
        , calcAbstraction(gate.lits)
        , subs
    );

    for (size_t i = 0; i < subs.size(); i++) {
        ClOffset offset = subs[i];
        Clause& cl = *solver->clAllocator->getPointer(offset);

        //Don't shorten definitions of OR gates
        // -- we could be manipulating the definition of the gate itself
        //Don't shorten irred clauses with learnt gates
        // -- potential loss if e.g. learnt clause is removed later
        if (cl.defOfOrGate
            || (!cl.red() && gate.learnt))
            continue;

        #ifdef VERBOSE_ORGATE_REPLACE
        cout << "OR gate-based cl-shortening" << endl;
        cout << "Gate used: " << gate << endl;
        cout << "orig Clause: " << *clauses[c.index]<< endl;
        #endif

        runStats.orGateUseful++;

        //Go through clause, check if RHS (eqLit) is inside the clause
        //If it is, we have two possibilities:
        //1) a = b V c , clause: a V b V c V d
        //2) a = b V c , clause: -a V b V c V d --> clause can be safely removed
        bool removedClause = false;
        bool eqLitInside = false;
        for (Lit *l = cl.begin(), *end = cl.end(); l != end; l++) {
            if (gate.eqLit.var() == l->var()) {
                if (gate.eqLit == *l) {
                    eqLitInside = true;
                    break;
                } else {
                    assert(gate.eqLit == ~*l);
                    subsumer->unlinkClause(offset);
                    removedClause = true;
                    break;
                }
            }
        }

        //This clause got removed. Moving on to next clause
        if (removedClause)
            continue;

        //Set up future clause's lits
        vector<Lit> lits;
        for (uint32_t i = 0; i < cl.size(); i++) {
            const Lit lit = cl[i];
            bool inGate = false;
            for (vector<Lit>::const_iterator
                it = gate.lits.begin(), end = gate.lits.end()
                ; it != end
                ; it++
            ) {
                if (*it == lit) {
                    inGate = true;
                    runStats.litsRem++;
                    break;
                }
            }

            if (!inGate)
                lits.push_back(lit);
        }
        if (!eqLitInside) {
            lits.push_back(gate.eqLit);
            runStats.litsRem--;
        }

        //Future clause's stat
        const bool red = cl.red();
        ClauseStats stats = cl.stats;

        //Free the old clause and allocate new one
        subsumer->unlinkClause(offset);
        Clause* cl2 = solver->addClauseInt(lits, learnt, stats, false);
        if (!solver->ok)
            return false;

        //If this clause is NULL, then just ignore
        if (cl2 == NULL)
            continue;

        subsumer->linkInClause(*cl2);
        ClOffset offset2 = solver->clAllocator->getOffset(cl2);
        if (learnt)
            solver->longRedCls.push_back(offset2);
        else
            solver->longIrredCls.push_back(offset2);


        #ifdef VERBOSE_ORGATE_REPLACE
        cout << "new  Clause : " << cl << endl;
        cout << "-----------" << endl;
        #endif
    }

    return true;
}

CL_ABST_TYPE GateFinder::calculateSortedOcc(
    const OrGate& gate
    , uint16_t& maxSize
    , vector<size_t>& seen2Set
    , uint64_t& numOp
) {
    CL_ABST_TYPE abstraction = 0;

    //Initialise sizeSortedOcc, which s a temporary to save memory frees&requests
    for (uint32_t i = 0; i < sizeSortedOcc.size(); i++)
        sizeSortedOcc[i].clear();

    const vec<Watched>& csOther = solver->watches[(~(gate.lits[1])).toInt()];
    //cout << "csother: " << csOther.size() << endl;
    *subsumer->toDecrease -= csOther.size()*3;
    for (vec<Watched>::const_iterator it = csOther.begin(), end = csOther.end(); it != end; it++) {

        //Check if it's a long clause
        if (!it->isClause())
            continue;

        ClOffset offset = it->getOffset();
        const Clause& cl = *solver->clAllocator->getPointer(offset);

        if (cl.defOfOrGate //We might be removing the definition. Info loss
            || (!cl.red() && gate.learnt)) //We might be contracting 2 irred clauses based on a learnt gate. Info loss
            continue;

        numOp += cl.size();

        //Make sure sizeSortedOcc is enough, and add this clause to it
        maxSize = std::max(maxSize, cl.size());
        if (sizeSortedOcc.size() < (uint32_t)maxSize+1)
            sizeSortedOcc.resize(maxSize+1);

        sizeSortedOcc[cl.size()].push_back(offset);

        //Set seen2 & abstraction, which are optimisations to speed up and-gate-based-contraction
        for (uint32_t i = 0; i < cl.size(); i++) {
            if (!seen2[cl[i].toInt()]) {
                seen2[cl[i].toInt()] = true;
                seen2Set.push_back(cl[i].toInt());
            }
            abstraction |= 1UL << (cl[i].var() % CLAUSE_ABST_SIZE);
        }
    }
    abstraction |= 1UL << (gate.lits[0].var() % CLAUSE_ABST_SIZE);

    return abstraction;
}

bool GateFinder::treatAndGate(
    const OrGate& gate
    , const bool reallyRemove
    , uint32_t& foundPotential
    , uint64_t& numOp
) {
    assert(gate.lits.size() == 2);

    //If there are no clauses that contain the opposite of the literals on the LHS, there is nothing we can do
    if (solver->watches[(~(gate.lits[0])).toInt()].empty()
        || solver->watches[(~(gate.lits[1])).toInt()].empty())
        return true;

    //Set up sorted occurrance list of the other lit (lits[1]) in the gate
    uint16_t maxSize = 0; //Maximum clause size in this occur
    vector<size_t> seen2Set; //Bits that have been set in seen2, and later need to be cleared
    CL_ABST_TYPE abstraction = calculateSortedOcc(gate, maxSize, seen2Set, numOp);

    //Setup
    set<ClOffset> clToUnlink;
    ClOffset other;
    foundPotential = 0;

    //Now go through lits[0] and see if anything matches
    vec<Watched>& cs = solver->watches[(~(gate.lits[0])).toInt()];
    *subsumer->toDecrease -= cs.size()*3;
    for (vec<Watched>::const_iterator
        it2 = cs.begin(), end2 = cs.end()
        ; it2 != end2
        ; it2++
    ) {
        //Only look through clauses
        if (!it2->isClause())
            continue;

        ClOffset offset = it2->getOffset();
        Clause& cl = *solver->clAllocator->getPointer(offset);
        if ((it2->getAbst() | abstraction) != abstraction //Abstraction must be OK
            || cl.defOfOrGate //Don't remove definition by accident
            || cl.size() > maxSize //Size must be less than maxSize
            || sizeSortedOcc[cl.size()].empty()) //this bracket for sizeSortedOcc must be non-empty
        {
            continue;
        }

        //Check that we are not removing irred info based on learnt gate
        if (!cl.red() && gate.learnt)
            continue;

        //Check that ~lits[1] is not inside this clause, and that eqLit is not inside, either
        //Also check that all literals inside have at least been set by seen2 (otherwise, no chance of exact match)
        bool OK = true;
        numOp += cl.size();
        for (uint32_t i = 0; i < cl.size(); i++) {
            if (cl[i] == ~(gate.lits[0])) continue;
            if (   cl[i].var() == ~(gate.lits[1].var())
                || cl[i].var() == gate.eqLit.var()
                || !seen2[cl[i].toInt()]
            ) {
                OK = false;
                break;
            }
        }

        //Not possible, continue
        if (!OK)
            continue;

        //Calculate abstraction and set 'seen'
        CL_ABST_TYPE abst2 = 0;
        for (uint32_t i = 0; i < cl.size(); i++) {
            //Lit0 doesn't count into abstraction
            if (cl[i] == ~(gate.lits[0]))
                continue;

            seen[cl[i].toInt()] = true;
            abst2 |= 1UL << (cl[i].var() % CLAUSE_ABST_SIZE);
        }
        abst2 |= 1UL << ((~(gate.lits[1])).var() % CLAUSE_ABST_SIZE);

        //Find matching pair
        numOp += sizeSortedOcc[cl.size()].size()*5;
        const bool foundOther = findAndGateOtherCl(sizeSortedOcc[cl.size()], ~(gate.lits[1]), abst2, other);
        foundPotential += foundOther;
        if (reallyRemove && foundOther) {
            assert(other != offset);
            clToUnlink.insert(other);
            clToUnlink.insert(offset);
            //Add new clause that is shorter and represents both of the clauses above
            treatAndGateClause(other, gate, cl);
            if (!solver->ok)
                return false;
        }

        //Clear 'seen' from bits set
        for (uint32_t i = 0; i < cl.size(); i++) {
            seen[cl[i].toInt()] = false;
        }
    }

    //Clear from seen2 bits that have been set
    for(size_t at: seen2Set) {
        seen2[at] = false;
    }

    //Now that all is computed, remove those that need removal
    for(ClOffset offset: clToUnlink) {
        subsumer->unlinkClause(offset);
    }
    clToUnlink.clear();

    return true;
}

void GateFinder::treatAndGateClause(
    const ClOffset other
    , const OrGate& gate
    , const Clause& cl
) {
    #ifdef VERBOSE_ORGATE_REPLACE
    cout << "AND gate-based cl rem" << endl;
    cout << "clause 1: " << cl << endl;
    cout << "clause 2: " << *clauses[other.index] << endl;
    cout << "gate : " << gate << endl;
    #endif

    //Update stats
    runStats.andGateUseful++;
    runStats.clauseSizeRem += cl.size();

    //Put into 'lits' the literals of the clause
    vector<Lit> lits;
    lits.clear();
    for (uint32_t i = 0; i < cl.size(); i++) {
        if (cl[i] != ~(gate.lits[0]))
            lits.push_back(cl[i]);

        assert(cl[i].var() != gate.eqLit.var());
    }
    lits.push_back(~(gate.eqLit));

    //Calculate learnt & glue
    Clause& otherCl = *solver->clAllocator->getPointer(other);
    *subsumer->toDecrease -= otherCl.size()*2;
    bool red = otherCl.red() && cl.red();
    ClauseStats stats = ClauseStats::combineStats(cl.stats, otherCl.stats);

    #ifdef VERBOSE_ORGATE_REPLACE
    cout << "new clause:" << lits << endl;
    cout << "-----------" << endl;
    #endif

    //Create and link in new clause
    Clause* clNew = solver->addClauseInt(lits, learnt, stats, false);
    if (clNew != NULL) {
        subsumer->linkInClause(*clNew);
        ClOffset offsetNew = solver->clAllocator->getOffset(clNew);
        if (learnt)
            solver->longRedCls.push_back(offsetNew);
        else
            solver->longIrredCls.push_back(offsetNew);
    }
}

inline bool GateFinder::findAndGateOtherCl(
    const vector<ClOffset>& sizeSortedOcc
    , const Lit lit
    , const CL_ABST_TYPE abst2
    , ClOffset& other
) {
    *subsumer->toDecrease -= sizeSortedOcc.size();
    for (vector<ClOffset>::const_iterator
        it = sizeSortedOcc.begin(), end = sizeSortedOcc.end()
        ; it != end
        ; it++
    ) {
        ClOffset offset = *it;
        Clause& cl = *solver->clAllocator->getPointer(offset);

        if (cl.abst != abst2 //abstraction must match
            || cl.defOfOrGate //Don't potentially remove clause that is the definition itself
        ) {
            continue;
        }

        for (uint32_t i = 0; i < cl.size(); i++) {
            //we skip the other lit in the gate
            if (cl[i] == lit)
                continue;

            //Seen is correct, so this one is not the one we are searching for
            if (!seen[cl[i].toInt()])
                goto next;

        }
        other = *it;
        return true;

        next:;
    }

    return false;
}

void GateFinder::printDot2()
{
    std::stringstream ss;
    ss << "Gates" << (numDotPrinted++) << ".dot";
    std::string filenename = ss.str();
    std::ofstream file(filenename.c_str(), std::ios::out);
    file << "digraph G {" << endl;
    uint32_t index = 0;
    vector<bool> gateUsed;
    gateUsed.resize(orGates.size(), false);
    index = 0;
    for (vector<OrGate>::const_iterator it = orGates.begin(), end = orGates.end(); it != end; it++, index++) {
        for (vector<Lit>::const_iterator it2 = it->lits.begin(), end2 = it->lits.end(); it2 != end2; it2++) {
            vector<uint32_t>& occ = gateOccEq[it2->toInt()];
            for (vector<uint32_t>::const_iterator it3 = occ.begin(), end3 = occ.end(); it3 != end3; it3++) {
                if (*it3 == index) continue;

                file << "Gate" << *it3;
                gateUsed[*it3] = true;
                file << " -> ";

                file << "Gate" << index;
                gateUsed[index] = true;

                file << "[arrowsize=\"0.4\"];" << endl;
            }

            /*vector<uint32_t>& occ2 = gateOccEq[(~*it2).toInt()];
            for (vector<uint32_t>::const_iterator it3 = occ2.begin(), end3 = occ2.end(); it3 != end3; it3++) {
                if (*it3 == index) continue;

                file << "Gate" << *it3;
                gateUsed[*it3] = true;
                file << " -> ";

                file << "Gate" << index;
                gateUsed[index] = true;

                file << "[style = \"dotted\", arrowsize=\"0.4\"];" << endl;
            }*/
        }
    }

    index = 0;
    for (vector<OrGate>::iterator it = orGates.begin(), end = orGates.end(); it != end; it++, index++) {

        if (gateUsed[index]) {
            file << "Gate" << index << " [ shape=\"point\"";
            file << ", size = 0.8";
            file << ", style=\"filled\"";
            if (it->learnt) file << ", color=\"darkseagreen4\"";
            else file << ", color=\"darkseagreen\"";
            file << "];" << endl;
        }
    }

    file  << "}" << endl;
    file.close();
    cout << "c Printed gate structure to file " << filenename << endl;
}

void GateFinder::printDot()
{
    printDot2();
}

void GateFinder::newVar()
{
    dontElim.push_back(0);
    gateOcc.push_back(vector<uint32_t>());
    gateOcc.push_back(vector<uint32_t>());
    gateOccEq.push_back(vector<uint32_t>());
    gateOccEq.push_back(vector<uint32_t>());
}
