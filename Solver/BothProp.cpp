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
#include "ThreadControl.h"
#include "BothProp.h"
#include "assert.h"
#include <cstring>

BothProp::BothProp(ThreadControl* _control) :
    control(_control)
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
    size_t origZeroDepthAssigns = control->trail.size();

    //For BothSame
    propagatedBitSet.clear();
    propagated.resize(control->nVars(), 0);
    propValue.resize(control->nVars(), 0);

    //uint32_t fromBin;
    size_t var;
    size_t varStart = 0;
    size_t whichCandidate = 0;
    const size_t origBogoProps = control->propStats.bogoProps;
    vector<TwoSignVar> candidates(control->candidateForBothProp.size());
    for(size_t i = 0; i < control->candidateForBothProp.size(); i++) {
        Lit lit = Lit(i, false);
        candidates[i].var = lit.var();
        size_t posPolar =
            std::max<size_t>(control->watches[lit.toInt()].size(), control->candidateForBothProp[i].posLit);
        posPolar = std::max<size_t>(posPolar, control->implCache[(~lit).toInt()].lits.size());

        size_t negPolar =
            std::max<size_t>(control->watches[(~lit).toInt()].size(), control->candidateForBothProp[i].negLit);
        negPolar = std::max<size_t>(negPolar, control->implCache[lit.toInt()].lits.size());

        candidates[i].minOfPolarities = std::min(posPolar, negPolar);
        //cout << "can size: " << candidates[i].minOfPolarities << endl;
    }
    std::sort(candidates.begin(), candidates.end());
    std::fill(control->candidateForBothProp.begin(), control->candidateForBothProp.end(), ThreadControl::TwoSignAppearances());

    //Do while not done
    while (control->propStats.bogoProps + extraTime < origBogoProps + numProps) {
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
            //Var thisVvar = control->mtrand.randInt(control->nVars()-1);
            var = varStart;
            var %= control->nVars();
            varStart++;
        }
        extraTime += 20;

        if (control->value(var) != l_Undef || !control->decision_var[var]) {
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
    if (control->conf.verbosity >= 1) {
        cout
        << "c bprop"
        << " 0-depth assigns: " << (control->trail.size() - origZeroDepthAssigns)
        << " tried:" << tried
        << " failed: " << numFailed
        << " bothSame: " << bothSameAdded
        << " binXor: " << binXorAdded
        << " T: " << std::fixed << std::setprecision(2)
        << (cpuTime() - myTime)
        << endl;
    }
    return control->ok;
}

bool BothProp::tryBoth(Lit litToSet)
{
    bool failed;
    propagated.removeThese(propagatedBitSet);
    propagatedBitSet.clear();
    bothSame.clear();
    binXorToAdd.clear();

    //First one
    //cout << "watch size: " << control->watches[(litToSet).toInt()].size() << endl;
    control->newDecisionLevel();
    control->enqueue(litToSet);
    failed = (!control->propagate().isNULL());
    if (failed) {
        control->cancelUntil(0);
        numFailed++;
        control->enqueue(~litToSet);
        control->ok = control->propagate().isNULL();
        return control->ok;
    }

    assert(control->decisionLevel() > 0);
    for (int64_t c = control->trail.size()-1; c >= (int64_t)control->trail_lim[0]; c--) {
        const Var x = control->trail[c].var();
        extraTime += 5;

        //Visited this var, needs clear later on
        propagatedBitSet.push_back(x);

        //Set prop has been done
        propagated.setBit(x);

        //Set propValue
        if (control->assigns[x].getBool())
            propValue.setBit(x);
        else
            propValue.clearBit(x);
    }
    //cout <<"Size was: " << control->trail.size() - (int64_t)control->trail_lim[0] << endl;
    control->cancelUntil(0);

    //cout << "watch size: " << control->watches[(~litToSet).toInt()].size() << endl;
    control->newDecisionLevel();
    control->enqueue(~litToSet);
    failed = (!control->propagate().isNULL());
    if (failed) {
        control->cancelUntil(0);
        numFailed++;
        control->enqueue(litToSet);
        control->ok = (control->propagate().isNULL());
        return control->ok;
    }

    assert(control->decisionLevel() > 0);
    for (int64_t c = control->trail.size()-1; c >= (int64_t)control->trail_lim[0]; c--) {
        const Var x  = control->trail[c].var();
        extraTime += 5;
        if (propagated[x]) {
            if (propValue[x] == control->value(x).getBool()) {
                //they both imply the same
                bothSame.push_back(Lit(x, !propValue[x]));
            } else if (c != (int)control->trail_lim[0]) {
                bool isEqualTrue;
                assert(litToSet.sign() == false);
                tmpPs[0] = Lit(litToSet.var(), false);
                tmpPs[1] = Lit(x, false);
                isEqualTrue = !propValue[x];
                binXorToAdd.push_back(BinXorToAdd(tmpPs[0], tmpPs[1], isEqualTrue));
            }
        }

        //Set propValue
        if (control->assigns[x].getBool())
            propValue.setBit(x);
        else
            propValue.clearBit(x);
    }
    //cout <<"Size was: " << control->trail.size() - (int64_t)control->trail_lim[0] << endl;
    control->cancelUntil(0);

    for(size_t i = 0; i < bothSame.size(); i++) {
        extraTime += 3;
        control->enqueue(bothSame[i]);
    }
    bothSameAdded += bothSame.size();
    //cout << "bothSameAdded: " << bothSame.size() << endl;
    //cout << "---------------" << endl;
    control->ok = (control->propagate().isNULL());
    //Check if propagate lead to UNSAT
    if (!control->ok)
        return false;

    for (size_t i = 0; i < binXorToAdd.size(); i++) {
        extraTime += 20;
        tmpPs[0] = binXorToAdd[i].lit1;
        tmpPs[1] = binXorToAdd[i].lit2;
        control->addXorClauseInt(tmpPs, binXorToAdd[i].isEqualTrue);

        if (!control->ok)
            return false;
    }
    binXorAdded += binXorToAdd.size();

    return true;
}