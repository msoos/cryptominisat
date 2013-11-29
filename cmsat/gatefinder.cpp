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
#include "clauseallocator.h"

using namespace CMSat;
using std::cout;
using std::endl;

GateFinder::GateFinder(Simplifier *_simplifier, Solver *_solver) :
    numDotPrinted(0)
    , simplifier(_simplifier)
    , solver(_solver)
    , seen(_solver->seen)
    , seen2(_solver->seen2)
{
    sizeSortedOcc.resize(solver->conf.maxGateBasedClReduceSize+1);
}

bool GateFinder::doAll()
{
    runStats.clear();
    orGates.clear();
    clearIndexes();

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

size_t GateFinder::num_long_irred_cls(const Lit lit) const
{
    size_t num = 0;
    for(const Watched& ws: solver->watches[lit.toInt()]) {
        if (!ws.isClause())
            continue;

        const Clause& cl = *solver->clAllocator.getPointer(ws.getOffset());
        if (cl.red())
            continue;

        if (cl.size() > solver->conf.maxGateBasedClReduceSize)
            continue;

        num++;
    }

    return num;
}

void GateFinder::createNewVars()
{
    vector<NewGateData> newGates;
    numMaxCreateNewVars = 300LL*1000LL*1000LL;
    simplifier->limit_to_decrease = &numMaxCreateNewVars;

    vector<Lit> potential_lits;
    for(size_t i = 0; i < 2*solver->nVars(); i++) {
        const Lit lit(i/2, i %2);
        //size_t num = num_long_irred_cls(lit);
        size_t num = solver->watches[lit.toInt()].size();
        if (num > 3) {
            //Have to invert it, since this is an AND gate, so ~a = ~b AND ~c
            potential_lits.push_back(~lit);
        }
    }
    if (potential_lits.size() < 2)
        return;

    cout << "Lits size: " << potential_lits.size() << endl;

    size_t tries = 0;
    while(true) {
        if (*simplifier->limit_to_decrease < 0)
            break;

        //Pick sign randomly
        Lit lit1 = potential_lits[solver->mtrand.randInt(potential_lits.size()-1)];
        Lit lit2 = potential_lits[solver->mtrand.randInt(potential_lits.size()-1)];
        if (solver->value(lit1) != l_Undef
            || solver->varData[lit1.var()].removed != Removed::none
        ) {
            continue;
        }

        if (solver->value(lit2) != l_Undef
            || solver->varData[lit2.var()].removed != Removed::none
        ) {
            continue;
        }

        if (lit1.var() == lit2.var())
            continue;

        tries++;
        if (tries % 100000 == 0) {
            cout << "trying " << tries << endl;
        }

        //Make sure they are in the right order
        if (lit1 > lit2)
            std::swap(lit1, lit2);

        //See how many clauses this binary gate would allow us to remove
        uint32_t reduction = 0;
        OrGate gate(Lit(0,false), lit1, lit2, false);
        remove_clauses_using_and_gate(gate, false, true, reduction);
        remove_clauses_using_and_gate_tri(gate, false, true, reduction);

        //If we find the above to be adequate, then this should be a new gate
        if (reduction > 3) {
            cout << "Potential for reduction: "
            << reduction
            << " lit1: " << lit1 << " lit2: " << lit2
            << endl;
            //newGates.push_back(NewGateData(lit1, lit2, subs.size(), potential));
        }
    }

    /*if (solver->conf.verbosity >= 1) {
        cout
        << "c Added vars :" << addedVars
        << " tried: " << tries
        << " time: " << (cpuTime() - myTime) << endl;
    }
    runStats.numERVars += addedVars;
    runStats.erTime += cpuTime() - myTime;*/

    //return addedVars;
}

void GateFinder::findOrGates()
{
    assert(solver->ok);

    double myTime = cpuTime();
    numMaxGateFinder = 100LL*1000LL*1000LL;
    simplifier->limit_to_decrease = &numMaxGateFinder;

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
    if (*simplifier->limit_to_decrease < 0) {
        runStats.find_gate_timeout++;
    }
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
        double myTime = cpuTime();
        numMaxShortenWithGates = 100LL*1000LL*1000LL;
        simplifier->limit_to_decrease = &numMaxShortenWithGates;
        runStats.numLongCls = simplifier->runStats.origNumIrredLongClauses +
            simplifier->runStats.origNumRedLongClauses;
        runStats.numLongClsLits = solver->litStats.irredLits + solver->litStats.redLits;

        //Go through each gate, see if we can do something with it
        for (const OrGate& gate: orGates) {
            if (*simplifier->limit_to_decrease < 0) {
                break;
            }

            if (!shortenWithOrGate(gate))
                break;
        }
        runStats.orBasedTime += cpuTime() - myTime;
        runStats.or_based_timeout += (*simplifier->limit_to_decrease < 0);

        if (!solver->ok)
            return false;
    }

    //AND gate treatment
    if (solver->conf.doRemClWithAndGates) {
        numMaxClRemWithGates = 100LL*1000LL*1000LL;
        simplifier->limit_to_decrease = &numMaxClRemWithGates;
        double myTime = cpuTime();

        //Do clause removal
        uint32_t foundPotential;

        //Go through each gate, see if we can do something with it
        for (const OrGate& gate: orGates) {
            if (*simplifier->limit_to_decrease < 0) {
                break;
            }

            if (!remove_clauses_using_and_gate(gate, true, false, foundPotential))
                break;

            if (!remove_clauses_using_and_gate_tri(gate, true, false, foundPotential))
                break;
        }
        runStats.andBasedTime += cpuTime() - myTime;
        runStats.and_based_timeout += (*simplifier->limit_to_decrease < 0);

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
                watch_subarray_const ws = solver->watches[(~lit).toInt()];

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
            ClOffset offset = solver->clAllocator.getOffset(&cl);
            for (const Lit *l = cl.begin(), *end2 = cl.end(); l != end2; l++)
                findOrGate(~*l, offset, learntGatesToo, wasRed);
        }
}*/

void GateFinder::findOrGates(const bool redGatesToo)
{
    //Go through each TRI clause
    for(size_t i = 0; i < solver->watches.size(); i++) {
        if (*simplifier->limit_to_decrease < 0)
            break;

        const Lit lit = Lit::toLit(i);
        for(const Watched ws: solver->watches[i]) {
            *simplifier->limit_to_decrease -= 1;

            //Ran out of time?
            if (*simplifier->limit_to_decrease < 0) {
                break;
            }

            if (!ws.isTri())
                continue;

            //if no learnt gates are allowed and this is learnt, skip
            if (ws.red() && !redGatesToo)
                continue;

            const bool wasRed = ws.red();

            //Check how many literals have zero cache&binary clause
            //If too many have 0, it cannot possibly be an OR gate
            bool OK = true;
            for (const Lit l: ws) {
                //TODO stamping
                const vector<LitExtra>& cache = solver->implCache[(~l).toInt()].lits;
                watch_subarray_const ws = solver->watches[(~l).toInt()];

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
    for (const Lit otherLit: boost::array<Lit, 2>{{lit1, lit2}}) {
        //This is the other lineral in the binary clause
        //We are looking for a binary clause '~otherlit V ~eqLit'
        bool OK = false;

        //TODO stamping

        /*
        //Try to find corresponding binary clause in cache
        const vector<LitExtra>& cache = solver->implCache[(~otherLit).toInt()].lits;
        *simplifier->limit_to_decrease -= cache.size();
        for (LitExtra cacheLit: cache) {
            if ((redGatesToo || cacheLit.getOnlyIrredBin())
                 && cacheLit.getLit() == eqLit
            ) {
                wasRed |= !cacheLit.getOnlyIrredBin();
                OK = true;
                break;
            }
        }*/

        //Try to find corresponding binary clause in watchlist
        watch_subarray_const ws = solver->watches[(~otherLit).toInt()];
        *simplifier->limit_to_decrease -= ws.size();
        for (const Watched w: ws) {
            //Only binary clauses are of importance
            if (!w.isBinary())
                continue;

            if ((redGatesToo || !w.red())
                 && w.lit2() == eqLit
            ) {
                wasRed |= w.red();
                OK = true;
                break;
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
        Clause& cl = *solver->clAllocator.getPointer(offset);

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
        ClOffset offset2 = solver->clAllocator.getOffset(cl2);
        simplifier->clauses.push_back(offset2);

        #ifdef VERBOSE_ORGATE_REPLACE
        cout << "new  Clause : " << cl << endl;
        cout << "-----------" << endl;
        #endif
    }

    return true;
}

void GateFinder::set_seen2_and_abstraction(
    const Clause& cl
    , CL_ABST_TYPE& abstraction
) {
    *simplifier->limit_to_decrease -= cl.size();
    for (const Lit lit: cl) {
        if (!seen2[lit.toInt()]) {
            seen2[lit.toInt()] = true;
            seen2Set.push_back(lit.toInt());
        }
        abstraction |= 1UL << (lit.var() % CLAUSE_ABST_SIZE);
    }
}

CL_ABST_TYPE GateFinder::calc_sorted_occ_and_set_seen2(
    const OrGate& gate
    , uint16_t& maxSize
    , uint16_t& minSize
    , const bool only_irred
) {
    assert(seen2Set.empty());
    CL_ABST_TYPE abstraction = 0;
    for (vector<ClOffset>& certain_size_occ: sizeSortedOcc)
        certain_size_occ.clear();

    watch_subarray_const csOther = solver->watches[(~(gate.lit2)).toInt()];
    *simplifier->limit_to_decrease -= csOther.size();
    for (const Watched ws: csOther) {
        if (!ws.isClause())
            continue;

        const ClOffset offset = ws.getOffset();
        const Clause& cl = *solver->clAllocator.getPointer(offset);
        if (cl.red() && only_irred)
            continue;

        //We might be contracting 2 irred clauses based on a learnt gate
        //would lead to UNSAT->SAT
        if (!cl.red() && gate.red)
            continue;

        //Clause too long, skip
        if (cl.size() > solver->conf.maxGateBasedClReduceSize)
            continue;

        maxSize = std::max(maxSize, cl.size());
        minSize = std::min(minSize, cl.size());
        sizeSortedOcc[cl.size()].push_back(offset);
        set_seen2_and_abstraction(cl, abstraction);
    }

    return abstraction;
}

void GateFinder::set_seen2_tri(
    const OrGate& gate
    , const bool only_irred
) {
    assert(seen2Set.empty());
    watch_subarray_const csOther = solver->watches[(~(gate.lit2)).toInt()];
    *simplifier->limit_to_decrease -= csOther.size();
    for (const Watched ws: csOther) {
        if (!ws.isTri())
            continue;

        if (ws.red() && only_irred)
            continue;

        //We might be contracting 2 irred clauses based on a learnt gate
        //would lead to UNSAT->SAT
        if (!ws.red() && gate.red)
            continue;

        const Lit lits[2] = {ws.lit2(), ws.lit3()};
        for (size_t i = 0; i < 2; i++) {
            const Lit lit = lits[i];
            if (!seen2[lit.toInt()]) {
                seen2[lit.toInt()] = 1;
                seen2Set.push_back(lit.toInt());
            }
        }
    }
}

CL_ABST_TYPE GateFinder::calc_abst_and_set_seen(
    const Clause& cl
    , const OrGate& gate
) {
    CL_ABST_TYPE abst = 0;
    for (const Lit lit: cl) {
        //lit1 doesn't count into abstraction
        if (lit == ~(gate.lit1))
            continue;

        seen[lit.toInt()] = 1;
        abst |= 1UL << (lit.var() % CLAUSE_ABST_SIZE);
    }
    abst |= 1UL << ((~(gate.lit2)).var() % CLAUSE_ABST_SIZE);

    return abst;
}

bool GateFinder::check_seen_and_gate_against_cl(
    const Clause& this_cl
    , const OrGate& gate
) {
    *(simplifier->limit_to_decrease) -= this_cl.size();
    for (const Lit lit: this_cl) {

        //We know this is inside, skip
        if (lit == ~(gate.lit1))
            continue;

        //If some weird variable is inside, skip
        if (   lit.var() == gate.lit2.var()
            || lit.var() == gate.eqLit.var()
            //A lit is inside this clause isn't inside the others
            || !seen2[lit.toInt()]
        ) {
            return false;
        }
    }
    return true;
}

bool GateFinder::check_seen_and_gate_against_lit(
    const Lit lit
    , const OrGate& gate
) {
    //If some weird variable is inside, skip
    if (   lit.var() == gate.lit2.var()
        || lit.var() == gate.eqLit.var()
        //A lit is inside this clause isn't inside the others
        || !seen2[lit.toInt()]
    ) {
        return false;
    }

    return true;
}

ClOffset GateFinder::find_pair_for_and_gate_reduction(
    const Watched& ws
    , const size_t minSize
    , const size_t maxSize
    , const CL_ABST_TYPE general_abst
    , const OrGate& gate
    , const bool only_irred
) {
    //Only long clauses
    if (!ws.isClause())
        return CL_OFFSET_MAX;

    const ClOffset this_cl_offs = ws.getOffset();
    Clause& this_cl = *solver->clAllocator.getPointer(this_cl_offs);
    if ((ws.getAbst() | general_abst) != general_abst
        || (this_cl.red() && only_irred)
        || (!this_cl.red() && gate.red)
        || this_cl.size() > solver->conf.maxGateBasedClReduceSize
        || this_cl.size() > maxSize //Size must be smaller or equal to maxSize
        || this_cl.size() < minSize //Size must be larger or equal than minsize
        || sizeSortedOcc[this_cl.size()].empty()) //this bracket for sizeSortedOcc must be non-empty
    {
        //cout << "Not even possible, this clause cannot match any other" << endl;
        return CL_OFFSET_MAX;
    }

    if (!check_seen_and_gate_against_cl(this_cl, gate))
        return CL_OFFSET_MAX;


    const CL_ABST_TYPE this_cl_abst = calc_abst_and_set_seen(this_cl, gate);
    const ClOffset other_cl_offs = findAndGateOtherCl(
        sizeSortedOcc[this_cl.size()] //in this occur list that contains clauses of specific size
        , ~(gate.lit2) //this is the LIT that is meant to be in the clause
        , this_cl_abst //clause MUST match this abst
        , gate.red
        , only_irred
    );

    //Clear 'seen' from bits set
    *(simplifier->limit_to_decrease) -= this_cl.size();
    for (const Lit lit: this_cl) {
        seen[lit.toInt()] = 0;
    }

    return other_cl_offs;
}

bool GateFinder::find_pair_for_and_gate_reduction_tri(
    const Watched& ws
    , const OrGate& gate
    , const bool only_irred
    , Watched& found_pair
) {
    //Only long clauses
    if (!ws.isTri())
        return false;

    if (ws.red() && only_irred) {
        //cout << "Not even possible, this clause cannot match any other" << endl;
        return false;
    }

    //Check that we are not removing irred info based on learnt gate
    if (!ws.red() && gate.red)
        return false;

    if (!check_seen_and_gate_against_lit(ws.lit2(), gate)
        || !check_seen_and_gate_against_lit(ws.lit3(), gate))
    {
        return false;
    }

    seen[ws.lit2().toInt()] = 1;
    seen[ws.lit3().toInt()] = 1;
    const bool ret = findAndGateOtherCl_tri(
        solver->watches[(~(gate.lit2)).toInt()]
        , gate.red
        , only_irred
        , found_pair
    );

    seen[ws.lit2().toInt()] = 0;
    seen[ws.lit3().toInt()] = 0;

    return ret;
}

bool GateFinder::remove_clauses_using_and_gate(
    const OrGate& gate
    , const bool really_remove
    , const bool only_irred
    , uint32_t& reduction
) {
    assert(clToUnlink.empty());
    if (solver->watches[(~(gate.lit1)).toInt()].empty()
        || solver->watches[(~(gate.lit2)).toInt()].empty()
    ) {
        return solver->okay();
    }

    uint16_t maxSize = 0;
    uint16_t minSize = std::numeric_limits<uint16_t>::max();
    CL_ABST_TYPE general_abst = calc_sorted_occ_and_set_seen2(gate, maxSize, minSize, only_irred);
    general_abst |= 1UL << (gate.lit1.var() % CLAUSE_ABST_SIZE);
    if (maxSize == 0)
        return solver->okay();

    watch_subarray cs = solver->watches[(~(gate.lit1)).toInt()];
    *simplifier->limit_to_decrease -= cs.size();
    for (const Watched ws: cs) {
        if (*simplifier->limit_to_decrease < 0)
            break;

        const ClOffset other_cl_offs = find_pair_for_and_gate_reduction(
            ws, minSize, maxSize, general_abst, gate, only_irred
        );

        if (really_remove
           && other_cl_offs != CL_OFFSET_MAX
        ) {
            const ClOffset this_cl_offs = ws.getOffset();
            assert(other_cl_offs != this_cl_offs);
            clToUnlink.insert(other_cl_offs);
            clToUnlink.insert(this_cl_offs);
            treatAndGateClause(other_cl_offs, gate, this_cl_offs);
        }
        reduction += (other_cl_offs != CL_OFFSET_MAX);

        if (!solver->ok)
            return false;
    }

    //Clear from seen2 bits that have been set
    *(simplifier->limit_to_decrease) -= seen2Set.size();
    for(const size_t at: seen2Set) {
        seen2[at] = 0;
    }
    seen2Set.clear();

    //Now that all is computed, remove those that need removal
    for(const ClOffset offset: clToUnlink) {
        simplifier->unlinkClause(offset);
    }
    clToUnlink.clear();

    return solver->okay();
}

bool GateFinder::remove_clauses_using_and_gate_tri(
    const OrGate& gate
    , const bool really_remove
    , const bool only_irred
    , uint32_t& reduction
) {
    if (solver->watches[(~(gate.lit1)).toInt()].empty()
        || solver->watches[(~(gate.lit2)).toInt()].empty()
    ) {
        return solver->okay();
    }
    tri_to_unlink.clear();

    set_seen2_tri(gate, only_irred);
    watch_subarray_const cs = solver->watches[(~(gate.lit1)).toInt()];
    *simplifier->limit_to_decrease -= cs.size();
    for (const Watched ws: cs) {
        if (*simplifier->limit_to_decrease < 0)
            break;

        Watched other_ws;
        const bool found_pair = find_pair_for_and_gate_reduction_tri(
            ws, gate, only_irred, other_ws
        );

        if (really_remove && found_pair) {
            runStats.andGateUseful++;
            runStats.clauseSizeRem += 3;

            tri_to_unlink.insert(TriToUnlink(ws.lit2(), ws.lit3(), ws.red()));
            solver->detachTriClause(~(gate.lit2), other_ws.lit2(), other_ws.lit3(), other_ws.red());
            vector<Lit> lits = {~(gate.eqLit), ws.lit2(), ws.lit3()};
            solver->addClauseInt(
                lits
                , ws.red() && other_ws.red()
                , ClauseStats()
                , false //don't attach/propagate
            );
            if (!solver->ok)
                return false;
        }
        reduction += found_pair;
    }

    //Clear from seen2 bits that have been set
    *(simplifier->limit_to_decrease) -= seen2Set.size();
    for(const size_t at: seen2Set) {
        seen2[at] = false;
    }
    seen2Set.clear();

    for(const TriToUnlink tri: tri_to_unlink) {
        solver->detachTriClause(~(gate.lit1), tri.lit2, tri.lit3, tri.red);
    }
    tri_to_unlink.clear();

    return solver->okay();
}

void GateFinder::treatAndGateClause(
    const ClOffset other_cl_offset
    , const OrGate& gate
    , const ClOffset this_cl_offset
) {
    #ifdef VERBOSE_ORGATE_REPLACE
    cout << "AND gate-based cl rem" << endl;
    cout << "clause 1: " << cl << endl;
    //cout << "clause 2: " << *clauses[other_cl_offset.index] << endl;
    cout << "gate : " << gate << endl;
    #endif

    //Update stats
    runStats.andGateUseful++;
    const Clause& this_cl = *solver->clAllocator.getPointer(this_cl_offset);
    runStats.clauseSizeRem += this_cl.size();

    //Put into 'lits' the literals of the clause
    vector<Lit> lits;
    *simplifier->limit_to_decrease -= this_cl.size()*2;
    for (const Lit lit: this_cl) {
        if (lit != ~(gate.lit1)) {
            lits.push_back(lit);
            assert(lit.var() != gate.eqLit.var());
            assert(lit.var() != gate.lit1.var());
            assert(lit.var() != gate.lit2.var());
        }
    }
    lits.push_back(~(gate.eqLit));

    //Calculate learnt & glue
    const Clause& other_cl = *solver->clAllocator.getPointer(other_cl_offset);
    const bool red = other_cl.red() && this_cl.red();
    ClauseStats stats = ClauseStats::combineStats(this_cl.stats, other_cl.stats);

    #ifdef VERBOSE_ORGATE_REPLACE
    cout << "new clause:" << lits << endl;
    cout << "-----------" << endl;
    #endif

    //Create and link in new clause
    Clause* clNew = solver->addClauseInt(lits, red, stats, false);
    if (clNew != NULL) {
        simplifier->linkInClause(*clNew);
        ClOffset offsetNew = solver->clAllocator.getOffset(clNew);
        simplifier->clauses.push_back(offsetNew);
    }
}

ClOffset GateFinder::findAndGateOtherCl(
    const vector<ClOffset>& sizeSortedOcc
    , const Lit otherLit
    , const CL_ABST_TYPE abst
    , const bool gate_is_red
    , const bool only_irred
) {
    *(simplifier->limit_to_decrease) -= sizeSortedOcc.size();
    for (const ClOffset offset: sizeSortedOcc) {
        const Clause& cl = *solver->clAllocator.getPointer(offset);
        if (cl.red() && only_irred)
            continue;

        if (!cl.red() && gate_is_red)
            continue;

        //abstraction must match
        if (cl.abst != abst)
            continue;

        *(simplifier->limit_to_decrease) -= cl.size()/2+5;
        for (const Lit lit: cl) {
            //we skip the other lit in the gate
            if (lit == otherLit)
                continue;

            //Seen is perfectly correct, everything must match
            if (!seen[lit.toInt()])
                goto next;

        }
        return offset;
        next:;
    }

    return CL_OFFSET_MAX;
}

bool GateFinder::findAndGateOtherCl_tri(
    watch_subarray_const ws_list
    , const bool gate_is_red
    , const bool only_irred
    , Watched& ret
) {
    *(simplifier->limit_to_decrease) -= ws_list.size();
    for (const Watched& ws: ws_list) {
        if (!ws.isTri())
            continue;

        if (ws.red() && only_irred)
            continue;

        if (!ws.red() && gate_is_red)
            continue;

        if (seen[ws.lit2().toInt()]
            && seen[ws.lit3().toInt()]
        ) {
            ret = ws;
            return true;
        }
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
    gateOcc.push_back(vector<uint32_t>());
    gateOcc.push_back(vector<uint32_t>());
    gateOccEq.push_back(vector<uint32_t>());
    gateOccEq.push_back(vector<uint32_t>());
}

GateFinder::Stats& GateFinder::Stats::operator+=(const Stats& other)
{
    findGateTime += other.findGateTime;
    find_gate_timeout += other.find_gate_timeout;
    orBasedTime += other.orBasedTime;
    or_based_timeout += other.or_based_timeout;
    varReplaceTime += other.varReplaceTime;
    andBasedTime += other.andBasedTime;
    and_based_timeout += other.and_based_timeout;
    erTime += other.erTime;

    //OR-gate
    orGateUseful += other.orGateUseful;
    numLongCls += other.numLongCls;
    numLongClsLits += other.numLongClsLits;
    litsRem += other.litsRem;
    varReplaced += other.varReplaced;

    //And-gate
    andGateUseful += other.andGateUseful;
    clauseSizeRem += other.clauseSizeRem;

    //ER
    numERVars += other.numERVars;

    //Gates
    learntGatesSize += other.learntGatesSize;
    numRed += other.numRed;
    irredGatesSize += other.irredGatesSize;
    numIrred += other.numIrred;

    return *this;
}

void GateFinder::Stats::print(const size_t nVars) const
{
    cout << "c -------- GATE FINDING ----------" << endl;
    printStatsLine("c time"
        , totalTime()
    );

    printStatsLine("c find gate time"
        , findGateTime
        , findGateTime/totalTime()*100.0
        , "% time"
    );

    printStatsLine("c gate-based cl-sh time"
        , orBasedTime
        , orBasedTime/totalTime()*100.0
        , "% time"
    );

    printStatsLine("c gate-based cl-rem time"
        , andBasedTime
        , andBasedTime/totalTime()*100.0
        , "% time"
    );

    printStatsLine("c gate-based varrep time"
        , varReplaceTime
        , varReplaceTime/totalTime()*100.0
        , "% time"
    );

    printStatsLine("c gatefinder cl-short"
        , orGateUseful
        , (double)orGateUseful/(double)numLongCls
        , "% long cls"
    );

    printStatsLine("c gatefinder lits-rem"
        , litsRem
        , (double)litsRem/(double)numLongClsLits
        , "% long cls lits"
    );

    printStatsLine("c gatefinder cl-rem"
        , andGateUseful
        , (double)andGateUseful/(double)numLongCls
        , "% long cls"
    );

    printStatsLine("c gatefinder cl-rem's lits"
        , clauseSizeRem
        , (double)clauseSizeRem/(double)numLongClsLits
        , "% long cls lits"
    );

    printStatsLine("c gatefinder var-rep"
        , varReplaced
        , (double)varReplaced/(double)nVars
        , "% vars"
    );

    cout << "c -------- GATE FINDING END ----------" << endl;
}

void GateFinder::Stats::printShort() const
{
    //Gate find
    cout << "c [gate] found"
    << " irred:" << numIrred
    << " avg-s: " << std::fixed << std::setprecision(1)
    << ((double)irredGatesSize/(double)numIrred)
    << " red: " << numRed
    /*<< " avg-s: " << std::fixed << std::setprecision(1)
    << ((double)learntGatesSize/(double)numRed)*/
    << " T: " << std::fixed << std::setprecision(2)
    << findGateTime
    << " T-out: " << (find_gate_timeout ? "Y" : "N")
    << endl;

    //gate-based shorten
    cout << "c [gate] shorten"
    << " cl: " << std::setw(5) << orGateUseful
    << " l-rem: " << std::setw(6) << litsRem
    << " T: " << std::fixed << std::setw(7) << std::setprecision(2)
    << orBasedTime
    << " T-out: " << (or_based_timeout ? "Y" : "N")
    << endl;

    //gate-based cl-rem
    cout << "c [gate] rem"
    << " cl: " << andGateUseful
    << " avg s: " << ((double)clauseSizeRem/(double)andGateUseful)
    << " T: " << std::fixed << std::setprecision(2)
    << andBasedTime
    << " T-out: " << (and_based_timeout ? "Y" : "N")
    << endl;

    //var-replace
    cout << "c [gate] eqlit"
    << " v-rep: " << std::setw(3) << varReplaced
    << " T: " << std::fixed << std::setprecision(2)
    << varReplaceTime
    << endl;
}
