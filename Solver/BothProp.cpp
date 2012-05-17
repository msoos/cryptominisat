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

#include "Solver.h"
#include "BothProp.h"
#include "assert.h"
#include <cstring>

BothProp::BothProp(Solver* _solver) :
    solver(_solver)
    , tmpPs(2)
{
}

struct TwoSignVar{
    size_t minOfPolarities;
    size_t var;

    //Sort them according to largest firest
    bool operator<(const TwoSignVar& other) const
    {
        return minOfPolarities > other.minOfPolarities;
    }
};

bool BothProp::tryBothProp()
{
    //Set limits, etc.
    extraTime = 0;
    tried = 0;
    size_t numProps = 50L*1000L*1000L;
    bothSameAdded = 0;
    numFailed = 0;
    binXorAdded = 0;
    double myTime = cpuTime();
    size_t origZeroDepthAssigns = solver->trail.size();

    //For BothSame
    propagatedBitSet.clear();
    propagated.resize(solver->nVars(), 0);
    propValue.resize(solver->nVars(), 0);

    //uint32_t fromBin;
    size_t var;
    size_t varStart = 0;
    size_t whichCandidate = 0;
    const size_t origBogoProps = solver->propStats.bogoProps;
    vector<TwoSignVar> candidates(solver->candidateForBothProp.size());
    for(size_t i = 0; i < solver->candidateForBothProp.size(); i++) {
        Lit lit = Lit(i, false);
        candidates[i].var = lit.var();
        size_t posPolar =
            std::max<size_t>(solver->watches[lit.toInt()].size(), solver->candidateForBothProp[i].posLit);
        posPolar = std::max<size_t>(posPolar, solver->implCache[(~lit).toInt()].lits.size());

        size_t negPolar =
            std::max<size_t>(solver->watches[(~lit).toInt()].size(), solver->candidateForBothProp[i].negLit);
        negPolar = std::max<size_t>(negPolar, solver->implCache[lit.toInt()].lits.size());

        candidates[i].minOfPolarities = std::min(posPolar, negPolar);
        //cout << "can size: " << candidates[i].minOfPolarities << endl;
    }
    std::sort(candidates.begin(), candidates.end());
    std::fill(solver->candidateForBothProp.begin(), solver->candidateForBothProp.end(), Solver::TwoSignAppearances());

    //Do while not done
    while (solver->propStats.bogoProps + extraTime < origBogoProps + numProps) {
        bool candidateOK = false;
        //Prefer candidates, but if none is found, use iterative approach
        if (whichCandidate < candidates.size()) {
            var = candidates[whichCandidate].var;
            whichCandidate++;
            if (candidates[whichCandidate].minOfPolarities > 100) {
                candidateOK = true;
                //cout << "Candidate OK size: " << candidates[whichCandidate].minOfPolarities << endl;
            }
        }
        //candidateOK = false;

        if (!candidateOK) {
            //Var thisVvar = solver->mtrand.randInt(solver->nVars()-1);
            var = varStart;
            var %= solver->nVars();
            varStart++;
        }
        extraTime += 20;

        if (solver->value(var) != l_Undef || !solver->decision_var[var]) {
            if (candidateOK) {
                //cout << "Candidate already failed" << endl;
            }
            continue;
        }

        tried++;
        if (!tryBoth(Lit(var, false)))
            goto end;
    }

end:
    if (solver->conf.verbosity >= 1) {
        cout
        << "c bprop"
        << " 0-depth assigns: " << (solver->trail.size() - origZeroDepthAssigns)
        << " tried:" << tried
        << " failed: " << numFailed
        << " bothSame: " << bothSameAdded
        << " binXor: " << binXorAdded
        << " T: " << std::fixed << std::setprecision(2)
        << (cpuTime() - myTime)
        << endl;
    }
    return solver->ok;
}

bool BothProp::tryBoth(Lit litToSet)
{
    bool failed;
    propagated.removeThese(propagatedBitSet);
    propagatedBitSet.clear();
    bothSame.clear();
    binXorToAdd.clear();

    //First one
    //cout << "watch size: " << solver->watches[(litToSet).toInt()].size() << endl;
    solver->newDecisionLevel();
    solver->enqueue(litToSet);
    failed = (!solver->propagate().isNULL());
    if (failed) {
        solver->cancelUntil(0);
        numFailed++;
        solver->enqueue(~litToSet);
        solver->ok = solver->propagate().isNULL();
        return solver->ok;
    }

    assert(solver->decisionLevel() > 0);
    for (int64_t c = solver->trail.size()-1; c >= (int64_t)solver->trail_lim[0]; c--) {
        const Var x = solver->trail[c].var();
        extraTime += 5;

        //Visited this var, needs clear later on
        propagatedBitSet.push_back(x);

        //Set prop has been done
        propagated.setBit(x);

        //Set propValue
        if (solver->assigns[x].getBool())
            propValue.setBit(x);
        else
            propValue.clearBit(x);
    }
    //cout <<"Size was: " << solver->trail.size() - (int64_t)solver->trail_lim[0] << endl;
    solver->cancelUntil(0);

    //cout << "watch size: " << solver->watches[(~litToSet).toInt()].size() << endl;
    solver->newDecisionLevel();
    solver->enqueue(~litToSet);
    failed = (!solver->propagate().isNULL());
    if (failed) {
        solver->cancelUntil(0);
        numFailed++;
        solver->enqueue(litToSet);
        solver->ok = (solver->propagate().isNULL());
        return solver->ok;
    }

    assert(solver->decisionLevel() > 0);
    for (int64_t c = solver->trail.size()-1; c >= (int64_t)solver->trail_lim[0]; c--) {
        const Var x  = solver->trail[c].var();
        extraTime += 5;
        if (propagated[x]) {
            if (propValue[x] == solver->value(x).getBool()) {
                //they both imply the same
                bothSame.push_back(Lit(x, !propValue[x]));
            } else if (c != (int)solver->trail_lim[0]) {
                bool isEqualTrue;
                assert(litToSet.sign() == false);
                tmpPs[0] = Lit(litToSet.var(), false);
                tmpPs[1] = Lit(x, false);
                isEqualTrue = !propValue[x];
                binXorToAdd.push_back(BinXorToAdd(tmpPs[0], tmpPs[1], isEqualTrue));
            }
        }

        //Set propValue
        if (solver->assigns[x].getBool())
            propValue.setBit(x);
        else
            propValue.clearBit(x);
    }
    //cout <<"Size was: " << solver->trail.size() - (int64_t)solver->trail_lim[0] << endl;
    solver->cancelUntil(0);

    for(size_t i = 0; i < bothSame.size(); i++) {
        extraTime += 3;
        solver->enqueue(bothSame[i]);
    }
    bothSameAdded += bothSame.size();
    //cout << "bothSameAdded: " << bothSame.size() << endl;
    //cout << "---------------" << endl;
    solver->ok = (solver->propagate().isNULL());
    //Check if propagate lead to UNSAT
    if (!solver->ok)
        return false;

    for (size_t i = 0; i < binXorToAdd.size(); i++) {
        extraTime += 20;
        tmpPs[0] = binXorToAdd[i].lit1;
        tmpPs[1] = binXorToAdd[i].lit2;
        solver->addXorClauseInt(tmpPs, binXorToAdd[i].isEqualTrue);

        if (!solver->ok)
            return false;
    }
    binXorAdded += binXorToAdd.size();

    return true;
}