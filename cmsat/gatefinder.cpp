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

#include "gatefinder.h"
#include "time_mem.h"
#include "solver.h"
#include "simplifier.h"
#include "subsumestrengthen.h"

using namespace CMSat;
using std::cout;
using std::endl;

GateFinder::GateFinder(Simplifier *_simplifier, Solver *_solver) :
    numDotPrinted(0)
    , simplifier(_simplifier)
    , solver(_solver)
    , seen(_simplifier->seen)
    , seen2(_simplifier->seen2)
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
    simplifier->toDecrease = &numMaxCreateNewVars;

    const size_t size = solver->getNumFreeVars()-1;

    size_t tries = 0;
    for (; tries < std::min<size_t>(100000U, size*size/2); tries++) {
        if (*simplifier->toDecrease < 50LL*1000LL*1000LL)
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
        simplifier->findSubsumed0(std::numeric_limits< uint32_t >::max(), tmp, calcAbstraction(tmp), subs);

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
        simplifier->linkInClause(*cl);
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
    simplifier->toDecrease = &numMaxGateFinder;

    findOrGates(false);

    for(const auto orgate: orGates) {
        if (orgate.red) {
            runStats.learntGatesSize += 2;
            runStats.numRed++;
        } else  {
            runStats.irredGatesSize += 2;
            runStats.numIrred++;
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

    cout << "c gateOcc num: " << gateOccNum
    << " gateOccEq num: " << gateOccEqNum
    << " gates size: " << orGates.size() << endl;
}

void GateFinder::clearIndexes()
{
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
        simplifier->toDecrease = &numMaxShortenWithGates;
        runStats.numLongCls = simplifier->runStats.origNumIrredLongClauses +
            simplifier->runStats.origNumRedLongClauses;
        runStats.numLongClsLits = solver->binTri.irredLits + solver->binTri.redLits;

        //Go through each gate, see if we can do something with it
        for (const OrGate& gate: orGates) {
            //Time is up!
            if (*simplifier->toDecrease < 0) {
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
        simplifier->toDecrease = &numMaxClRemWithGates;
        double myTime = cpuTime();

        //Do clause removal
        uint32_t foundPotential;

        //Go through each gate, see if we can do something with it
        size_t at = 0;
        for (const OrGate& gate: orGates) {
            //cout << "At AND-gate no. " << at << "/" << orGates.size() << endl;
            at++;

            //Time's up?
            if (*simplifier->toDecrease < 0) {
                if (solver->conf.verbosity >= 2) {
                    cout
                    << "c No more time left for cl-removal with gates" << endl;
                }
                break;
            }

            if (!tryAndGate(gate, true, foundPotential))
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
    std::sort(gates.begin(), gates.end());

    vector<Lit> tmp(2);
    for (uint32_t i = 1; i < gates.size(); i++) {
        const OrGate& gate1 = gates[i-1];
        const OrGate& gate2 = gates[i];

        if (gate1.lit1 == gate2.lit1
            && gate1.lit2 == gate2.lit2
            && gate1.eqLit.var() != gate2.eqLit.var()
       ) {
            foundRep++;
            tmp[0] = gate1.eqLit.unsign();
            tmp[1] = gate2.eqLit.unsign();
            const bool RHS = gate1.eqLit.sign() ^ gate2.eqLit.sign();
            if (!solver->addXorClauseInt(tmp, RHS, false))
                return foundRep;
        }
    }

    return foundRep;
}

/*void GateFinder::largeGate()
{
    //Ran out of time?
            if (*simplifier->toDecrease < 0) {
                if (solver->conf.verbosity >= 1) {
                    cout << "c Finishing gate-finding: ran out of time" << endl;
                }
                break;
            }

            //Clause removed
            if (cl.freed())
                continue;

            //If clause is larger than the cap on gate size, skip. Only for speed reasons.
            if (cl.size() > solver->conf.maxGateSize)
                continue;

            //if no learnt gates are allowed and this is learnt, skip
            if (!redGatesToo && cl.red())
                continue;

            const bool wasRed = cl.red();

            //Check how many literals have zero cache&binary clause
            //If too many have 0, it cannot possibly be an OR gate
            uint8_t numSizeZero = 0;
            for (const Lit *l = cl.begin(), *end2 = cl.end(); l != end2; l++) {
                Lit lit = *l;
                //TODO stamping
                const vector<LitExtra>& cache = solver->implCache[(~lit).toInt()].lits;
                const vec<Watched>& ws = solver->watches[(~lit).toInt()];

                if (
                    cache.size() == 0 &&
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
}*/

void GateFinder::findOrGates(const bool redGatesToo)
{
    //Go through each TRI clause
    for(size_t i = 0; i < solver->watches.size(); i++) {
        const Lit lit = Lit::toLit(i);
        for(const Watched ws: solver->watches[i]) {
            *simplifier->toDecrease -= 1;

            //Ran out of time?
            if (*simplifier->toDecrease < 0) {
                if (solver->conf.verbosity >= 1) {
                    cout << "c Finishing gate-finding: ran out of time" << endl;
                }
                break;
            }

            if (!ws.isTri())
                continue;

            //if no learnt gates are allowed and this is learnt, skip
            if (!redGatesToo && ws.red())
                continue;

            const bool wasRed = ws.red();

            //Check how many literals have zero cache&binary clause
            //If too many have 0, it cannot possibly be an OR gate
            bool OK = true;
            for (const Lit l: ws) {
                //TODO stamping
                const vector<LitExtra>& cache = solver->implCache[(~l).toInt()].lits;
                const vec<Watched>& ws = solver->watches[(~l).toInt()];

                if (cache.size() == 0
                    && ws.size() == 0
                ) {
                    OK = false;
                    break;
                }
            }

            //Too many zeroes
            if (!OK)
                continue;

            //Try to find a gate with eqlit (~*l)
            findOrGate(~lit, ws.lit2(), ws.lit3(), redGatesToo, wasRed);
        }
    }
}

void GateFinder::findOrGate(
    const Lit eqLit
    , const Lit lit1
    , const Lit lit2
    , const bool redGatesToo
    , bool wasRed
) {
    bool isEqual = true;
    for (Lit otherLit: boost::array<Lit, 2>{{lit1, lit2}}) {
        //This is the other lineral in the binary clause
        //We are looking for a binary clause '~otherlit V ~eqLit'
        bool OK = false;

        //TODO stamping

        //Try to find corresponding binary clause in cache
        const vector<LitExtra>& cache = solver->implCache[(~otherLit).toInt()].lits;
        *simplifier->toDecrease -= cache.size();
        for (LitExtra cacheLit: cache) {
            if ((redGatesToo || cacheLit.getOnlyIrredBin())
                 && cacheLit.getLit() == eqLit
            ) {
                wasRed |= !cacheLit.getOnlyIrredBin();
                OK = true;
                break;
            }
        }

        //Try to find corresponding binary clause in watchlist
        const vec<Watched>& ws = solver->watches[(~otherLit).toInt()];
        *simplifier->toDecrease -= ws.size();
        for (vec<Watched>::const_iterator
            wsIt = ws.begin(), endWS = ws.end()
            ; wsIt != endWS && !OK
            ; wsIt++
        ) {
            //Only binary clauses are of importance
            if (!wsIt->isBinary())
                continue;

            if ((redGatesToo || !wsIt->red())
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
    OrGate gate(eqLit, lit1, lit2, wasRed);

    //Find if there are any gates that are the same
    for (uint32_t at: gateOccEq[gate.eqLit.toInt()]) {
        //The same gate? Then froget about this
        if (orGates[at] == gate)
            return;
    }

    //Add gate
    const size_t at = orGates.size();
    orGates.push_back(gate);
    gateOccEq[gate.eqLit.toInt()].push_back(at);
    if (!wasRed) {
        for (Lit lit: boost::array<Lit, 2>{{gate.lit1, gate.lit2}}) {
            gateOcc[lit.toInt()].push_back(at);
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
    simplifier->subsumeStrengthen->findSubsumed0(
        std::numeric_limits< uint32_t >::max()
        , gate.getLits()
        , calcAbstraction(gate.getLits())
        , subs
    );

    for (size_t i = 0; i < subs.size(); i++) {
        ClOffset offset = subs[i];
        Clause& cl = *solver->clAllocator->getPointer(offset);

        // OLD STUFF
        //  //Don't shorten definitions of OR gates
        //  // -- we could be manipulating the definition of the gate itself

        //Don't shorten irred clauses with red gates
        // -- potential loss if e.g. red clause is removed later
        if ((!cl.red() && gate.red))
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
        //2) a = b V c , clause: -a V b V c V d
        //But we will simply ignore this. One of these clauses can be strengthened
        //the other subsumed. But let's ignore these, subsumption/strenghtening will take care of this
        bool eqLitInside = false;
        for (Lit lit: cl) {
            if (gate.eqLit.var() == lit.var()) {
                eqLitInside = true;
                break;
            }
        }
        if (eqLitInside)
            continue;

        //Set up future clause's lits
        vector<Lit> lits;
        for (const Lit lit: cl) {
            bool inGate = false;
            for (Lit lit2: gate.getLits()) {
                if (lit == lit2) {
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
        simplifier->unlinkClause(offset);
        Clause* cl2 = solver->addClauseInt(lits, red, stats, false);
        if (!solver->ok)
            return false;

        //If the clause is implicit, it's already linked in, ignore
        if (cl2 == NULL)
            continue;

        simplifier->linkInClause(*cl2);
        ClOffset offset2 = solver->clAllocator->getOffset(cl2);
        simplifier->clauses.push_back(offset2);

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
    , uint16_t& minSize
) {
    assert(seen2Set.empty());
    CL_ABST_TYPE abstraction = 0;

    //Initialise sizeSortedOcc, which s a temporary to save memory frees&requests
    for (uint32_t i = 0; i < sizeSortedOcc.size(); i++)
        sizeSortedOcc[i].clear();

    const vec<Watched>& csOther = solver->watches[(~(gate.lit2)).toInt()];
    //cout << "csother: " << csOther.size() << endl;
    *simplifier->toDecrease -= csOther.size()*3;
    for (const Watched ws: csOther) {
        //Check if it's a long clause
        if (!ws.isClause())
            continue;

        ClOffset offset = ws.getOffset();
        const Clause& cl = *solver->clAllocator->getPointer(offset);

        //We might be contracting 2 irred clauses based on a learnt gate
        //would lead to UNSAT->SAT
        if (!cl.red() && gate.red)
            continue;

        //Clause too long, skip
        if (cl.size() > solver->conf.maxGateBasedClReduceSize)
            continue;

        *simplifier->toDecrease -= cl.size();

        //Make sure sizeSortedOcc is enough, and add this clause to it
        maxSize = std::max(maxSize, cl.size());
        if (sizeSortedOcc.size() < (uint32_t)maxSize+1) {
            sizeSortedOcc.resize(maxSize+1);
        }
        sizeSortedOcc[cl.size()].push_back(offset);
        minSize = std::min(minSize, cl.size());

        //Set seen2 & abstraction, which is an optimisations to speed up
        //gate-based clause removal
        for (uint32_t i = 0; i < cl.size(); i++) {
            if (!seen2[cl[i].toInt()]) {
                seen2[cl[i].toInt()] = true;
                seen2Set.push_back(cl[i].toInt());
            }
            abstraction |= 1UL << (cl[i].var() % CLAUSE_ABST_SIZE);
        }
    }
    abstraction |= 1UL << (gate.lit1.var() % CLAUSE_ABST_SIZE);

    return abstraction;
}

bool GateFinder::tryAndGate(
    const OrGate& gate
    , const bool reallyRemove
    , uint32_t& foundPotential
) {
    assert(clToUnlink.empty());

    //If there are no clauses that contain the opposite of the literals
    //on the LHS, there is nothing we can do
    if (solver->watches[(~(gate.lit1)).toInt()].empty()
        || solver->watches[(~(gate.lit2)).toInt()].empty()
    ) {
        cout << "Nothing in one of the watchlists" << endl;
        return true;
    }

    //Set up sorted occurrance list of the other lit (lit2) in the gate
    uint16_t maxSize = 0; //Maximum clause size in this occur
    uint16_t minSize = std::numeric_limits<uint16_t>::max(); //Minimum clause size in this occur
    CL_ABST_TYPE abstraction = calculateSortedOcc(gate, maxSize, minSize);

    ClOffset other;
    foundPotential = 0;

    //Now go through lit1 and see if anything matches
    vec<Watched>& cs = solver->watches[(~(gate.lit1)).toInt()];
    *simplifier->toDecrease -= cs.size()*3;
    for (const Watched ws: cs) {
        //Only look through long clauses
        if (!ws.isClause()) {
            //cout << "Not long" << endl;
            continue;
        } else {
            //cout << "OK, long" << endl;
        }

        ClOffset offset = ws.getOffset();
        Clause& cl = *solver->clAllocator->getPointer(offset);
        if ((ws.getAbst() | abstraction) != abstraction //Abstraction must be OK
            || cl.size() > solver->conf.maxGateBasedClReduceSize
            || cl.size() > maxSize //Size must be smaller or equal to maxSize
            || cl.size() < minSize //Size must be larger or equal than minsize
            || sizeSortedOcc[cl.size()].empty()) //this bracket for sizeSortedOcc must be non-empty
        {
            //cout << "Not even possible, this clause cannot match any other" << endl;
            continue;
        }

        //Check that we are not removing irred info based on learnt gate
        if (!cl.red() && gate.red)
            continue;

        //Check that ~lits2 is not inside this clause, and that eqLit is not inside, either
        //Also check that all literals inside have at least been set by seen2 (otherwise, no chance of exact match)
        bool OK = true;
        *(simplifier->toDecrease) -= cl.size();
        for (const Lit lit: cl) {
            //We know this is inside, skip
            if (lit == ~(gate.lit1))
                continue;

            //If some weird variable is inside, skip
            if (   lit.var() == gate.lit2.var()
                || lit.var() == gate.eqLit.var()
                //A var is inside this clause isn't inside the others
                || !seen2[lit.toInt()]
            ) {
                OK = false;
                break;
            }
        }

        //Not possible, continue
        if (!OK) {
            //cout << "Has weird literal inside, skip" << endl;
            continue;
        }

        //Calculate abstraction and set 'seen'
        CL_ABST_TYPE abst2 = 0;
        for (const Lit lit: cl) {
            //lit1 doesn't count into abstraction
            if (lit == ~(gate.lit1))
                continue;

            seen[lit.toInt()] = true;
            abst2 |= 1UL << (lit.var() % CLAUSE_ABST_SIZE);
        }
        abst2 |= 1UL << ((~(gate.lit2)).var() % CLAUSE_ABST_SIZE);

        //Find matching pair
        const bool foundOther = findAndGateOtherCl(
            sizeSortedOcc[cl.size()] //in this occur list that contains clauses of specific size
            , ~(gate.lit2) //this is the LIT that is meant to be in the clause
            , abst2 //clause MUST match this abst
            , other
        );
        if (!foundOther) {
            //cout << "Wrong, looked through sizeSortedOcc, but no." << endl;
        }

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
        for (Lit lit: cl) {
            seen[lit.toInt()] = false;
        }
    }

    //Clear from seen2 bits that have been set
    for(size_t at: seen2Set) {
        seen2[at] = false;
    }
    seen2Set.clear();

    //Now that all is computed, remove those that need removal
    for(ClOffset offset: clToUnlink) {
        simplifier->unlinkClause(offset);
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
    const Clause& otherCl = *solver->clAllocator->getPointer(other);
    runStats.clauseSizeRem += otherCl.size();

    //Put into 'lits' the literals of the clause
    vector<Lit> lits;
    lits.clear();
    *simplifier->toDecrease -= cl.size()*2;
    for (const Lit lit: cl) {
        if (lit != ~(gate.lit1))
            lits.push_back(lit);

        assert(lit.var() != gate.eqLit.var());
    }
    lits.push_back(~(gate.eqLit));

    //Calculate learnt & glue
    bool red = otherCl.red() && cl.red();
    ClauseStats stats = ClauseStats::combineStats(cl.stats, otherCl.stats);

    #ifdef VERBOSE_ORGATE_REPLACE
    cout << "new clause:" << lits << endl;
    cout << "-----------" << endl;
    #endif

    //Create and link in new clause
    Clause* clNew = solver->addClauseInt(lits, red, stats, false);
    if (clNew != NULL) {
        simplifier->linkInClause(*clNew);
        ClOffset offsetNew = solver->clAllocator->getOffset(clNew);
        simplifier->clauses.push_back(offsetNew);
    }
}

inline bool GateFinder::findAndGateOtherCl(
    const vector<ClOffset>& sizeSortedOcc
    , const Lit otherLit
    , const CL_ABST_TYPE abst
    , ClOffset& other
) {
    *(simplifier->toDecrease) += sizeSortedOcc.size()*5;
    for (const ClOffset offset: sizeSortedOcc) {
        const Clause& cl = *solver->clAllocator->getPointer(offset);

        //abstraction must match
        if (cl.abst != abst)
            continue;

        for (const Lit lit: cl) {
            //we skip the other lit in the gate
            if (lit == otherLit)
                continue;

            //Seen is perfectly correct, everything must match
            if (!seen[lit.toInt()])
                goto next;

        }
        other = offset;
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
    vector<bool> gateUsed;
    gateUsed.resize(orGates.size(), false);
    size_t index = 0;
    for (const OrGate orGate: orGates) {
        index++;
        for (const Lit lit: orGate.getLits()) {
            for (uint32_t at: gateOccEq[lit.toInt()]) {
                //The same one, skip
                if (at == index)
                    continue;

                file << "Gate" << at;
                gateUsed[at] = true;
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
    for (const OrGate orGate: orGates) {
        index++;

        if (gateUsed[index]) {
            file << "Gate" << index << " [ shape=\"point\"";
            file << ", size = 0.8";
            file << ", style=\"filled\"";
            if (orGate.red)
                file << ", color=\"darkseagreen4\"";
            else
                file << ", color=\"darkseagreen\"";

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
