/***********************************************************************************
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

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
**************************************************************************************************/

#include "FailedVarSearcher.h"

#include <iomanip>
#include <utility>
#include <set>
using std::make_pair;
using std::set;

#include "Solver.h"
#include "ClauseCleaner.h"
#include "time_mem.h"
#include "VarReplacer.h"
#include "ClauseCleaner.h"
#include "StateSaver.h"

#ifdef _MSC_VER
#define __builtin_prefetch(a,b,c)
#endif //_MSC_VER

//#define VERBOSE_DEUBUG

FailedVarSearcher::FailedVarSearcher(Solver& _solver):
    solver(_solver)
    , tmpPs(2)
    , finishedLastTimeVar(true)
    , lastTimeWentUntilVar(0)
    , finishedLastTimeBin(true)
    , lastTimeWentUntilBin(0)
    , numPropsMultiplier(1.0)
    , lastTimeFoundTruths(0)
    , numCalls(0)
{
}

void FailedVarSearcher::addFromSolver(const vec< XorClause* >& cs)
{
    xorClauseSizes.clear();
    xorClauseSizes.growTo(cs.size());
    occur.resize(solver.nVars());
    for (Var var = 0; var < solver.nVars(); var++) {
        occur[var].clear();
    }
    
    uint32_t i = 0;
    for (XorClause * const*it = cs.getData(), * const*end = it + cs.size(); it !=  end; it++, i++) {
        if (it+1 != end)
            __builtin_prefetch(*(it+1), 0, 0);
        
        const XorClause& cl = **it;
        xorClauseSizes[i] = cl.size();
        for (const Lit *l = cl.getData(), *end2 = l + cl.size(); l != end2; l++) {
            occur[l->var()].push_back(i);
        }
    }
}

inline void FailedVarSearcher::removeVarFromXors(const Var var)
{
    vector<uint32_t>& occ = occur[var];
    if (occ.empty()) return;
    
    for (uint32_t *it = &occ[0], *end = it + occ.size(); it != end; it++) {
        xorClauseSizes[*it]--;
        if (!xorClauseTouched[*it]) {
            xorClauseTouched.setBit(*it);
            investigateXor.push(*it);
        }
    }
}

inline void FailedVarSearcher::addVarFromXors(const Var var)
{
    vector<uint32_t>& occ = occur[var];
    if (occ.empty()) return;
    
    for (uint32_t *it = &occ[0], *end = it + occ.size(); it != end; it++) {
        xorClauseSizes[*it]++;
    }
}

const TwoLongXor FailedVarSearcher::getTwoLongXor(const XorClause& c)
{
    TwoLongXor tmp;
    uint32_t num = 0;
    tmp.inverted = c.xor_clause_inverted();
    
    for(const Lit *l = c.getData(), *end = l + c.size(); l != end; l++) {
        if (solver.assigns[l->var()] == l_Undef) {
            assert(num < 2);
            tmp.var[num] = l->var();
            num++;
        } else {
            tmp.inverted ^= (solver.assigns[l->var()] == l_True);
        }
    }
    
    #ifdef VERBOSE_DEUBUG
    if (num != 2) {
        std::cout << "Num:" << num << std::endl;
        c.plainPrint();
    }
    #endif
    
    std::sort(&tmp.var[0], &tmp.var[0]+2);
    assert(num == 2);
    return tmp;
}

const bool FailedVarSearcher::search(uint64_t numProps)
{
    assert(solver.decisionLevel() == 0);
    solver.testAllClauseAttach();
    double myTime = cpuTime();
    uint32_t origHeapSize = solver.order_heap.size();
    StateSaver savedState(solver);
    Heap<Solver::VarOrderLt> order_heap_copy(solver.order_heap); //for hyperbin
    uint64_t origBinClauses = solver.binaryClauses.size();
    
    if (solver.readdOldLearnts && !readdRemovedLearnts()) goto end;
    
    //General Stats
    numFailed = 0;
    goodBothSame = 0;
    numCalls++;
    
    //If failed var searching is going good, do successively more and more of it
    if (lastTimeFoundTruths > 500 || (double)lastTimeFoundTruths > (double)solver.order_heap.size() * 0.03) std::max(numPropsMultiplier*1.7, 5.0);
    else numPropsMultiplier = 1.0;
    numProps = (uint64_t) ((double)numProps * numPropsMultiplier *3);
    
    //For BothSame
    propagated.resize(solver.nVars(), 0);
    propValue.resize(solver.nVars(), 0);
    
    //For calculating how many variables have really been set
    origTrailSize = solver.trail.size();
    
    //For 2-long xor (rule 6 of  Equivalent literal propagation in the DLL procedure by Chu-Min Li)
    toReplaceBefore = solver.varReplacer->getNewToReplaceVars();
    lastTrailSize = solver.trail.size();
    binXorFind = true;
    twoLongXors.clear();
    if (solver.xorclauses.size() < 5 ||
        solver.xorclauses.size() > 30000 ||
        solver.order_heap.size() > 30000 ||
        solver.nClauses() > 100000)
        binXorFind = false;
    if (binXorFind) {
        solver.clauseCleaner->cleanClauses(solver.xorclauses, ClauseCleaner::xorclauses);
        addFromSolver(solver.xorclauses);
    }
    xorClauseTouched.resize(solver.xorclauses.size(), 0);
    newBinXor = 0;
    
    //For 2-long xor through Le Berre paper
    bothInvert = 0;

    //For HyperBin
    unPropagatedBin.resize(solver.nVars(), 0);
    myimplies.resize(solver.nVars(), 0);
    hyperbinProps = 0;
    if (solver.addExtraBins && !orderLits()) return false;
    maxHyperBinProps = numProps/8;
    
    //uint32_t fromBin;
    uint32_t fromVar;
    if (finishedLastTimeVar || lastTimeWentUntilVar >= solver.nVars())
        fromVar = 0;
    else
        fromVar = lastTimeWentUntilVar;
    finishedLastTimeVar = true;
    lastTimeWentUntilVar = solver.nVars();
    origProps = solver.propagations;
    for (Var var = fromVar; var < solver.nVars(); var++) {
        if (solver.assigns[var] != l_Undef || !solver.decision_var[var])
            continue;
        if (solver.propagations - origProps >= numProps)  {
            finishedLastTimeVar = false;
            lastTimeWentUntilVar = var;
            break;
        }
        if (!tryBoth(Lit(var, false), Lit(var, true)))
            goto end;
    }

    numProps = (double)numProps * 1.2;
    hyperbinProps = 0;
    while (!order_heap_copy.empty()) {
        Var var = order_heap_copy.removeMin();
        if (solver.assigns[var] != l_Undef || !solver.decision_var[var])
            continue;
        if (solver.propagations - origProps >= numProps)  {
            finishedLastTimeVar = false;
            lastTimeWentUntilVar = var;
            break;
        }
        if (!tryBoth(Lit(var, false), Lit(var, true)))
            goto end;
    }
    
    /*if (solver.verbosity >= 1) printResults(myTime);
    if (finishedLastTimeBin || lastTimeWentUntilBin >= solver.binaryClauses.size())
        fromBin = 0;
    else
        fromBin = lastTimeWentUntilBin;
    finishedLastTimeBin = true;
    lastTimeWentUntilBin = solver.nVars();
    for (uint32_t binCl = 0; binCl < solver.binaryClauses.size(); binCl++) {
        if ((double)(solver.propagations - origProps) >= 1.1*(double)numProps)  {
            finishedLastTimeBin = false;
            lastTimeWentUntilBin = binCl;
            break;
        }
        
        Clause& cl = *solver.binaryClauses[binCl];
        if (solver.value(cl[0]) == l_Undef && solver.value(cl[1]) == l_Undef) {
            if (!tryBoth(cl[0], cl[1]))
                goto end;
        }
    }*/
    
    /*for (Clause **it = solver.clauses.getData(), **end = solver.clauses.getDataEnd(); it != end; it++) {
        Clause& c = **it;
        for (uint i = 0; i < c.size(); i++) {
            if (solver.value(c[i]) != l_Undef) goto next;
        }
        if (!tryAll(c.getData(), c.getDataEnd()))
            goto end;
        
        next:;
    }
    
    for (Clause **it = solver.learnts.getData(), **end = solver.learnts.getDataEnd(); it != end; it++) {
        Clause& c = **it;
        for (uint i = 0; i < c.size(); i++) {
            if (solver.value(c[i]) != l_Undef) goto next2;
        }
        if (!tryAll(c.getData(), c.getDataEnd()))
            goto end;
        
        next2:;
    }*/

end:
    bool removedOldLearnts = false;
    binClauseAdded = solver.binaryClauses.size() - origBinClauses;
    //Print results
    if (solver.verbosity >= 1) printResults(myTime);
    
    solver.order_heap.filter(Solver::VarFilter(solver));
    
    if (solver.ok && (numFailed || goodBothSame)) {
        double time = cpuTime();
        if ((int)origHeapSize - (int)solver.order_heap.size() >  (int)origHeapSize/15 && solver.nClauses() + solver.learnts.size() > 500000) {
            completelyDetachAndReattach();
            removedOldLearnts = true;
        } else {
            solver.clauseCleaner->removeAndCleanAll();
        }
        if (solver.verbosity >= 1 && numFailed + goodBothSame > 100) {
            std::cout << "c |  Cleaning up after failed var search: " << std::setw(8) << std::fixed << std::setprecision(2) << cpuTime() - time << " s "
            <<  std::setw(33) << " | " << std::endl;
        }
    }

    if (solver.ok && solver.readdOldLearnts && !removedOldLearnts) {
        if (solver.removedLearnts.size() < 100000) {
            removeOldLearnts();
        } else {
            completelyDetachAndReattach();
        }
    }
    
    lastTimeFoundTruths = solver.trail.size() - origTrailSize;

    savedState.restore();
    
    solver.testAllClauseAttach();
    return solver.ok;
}

void FailedVarSearcher::completelyDetachAndReattach()
{
    solver.clauses_literals = 0;
    solver.learnts_literals = 0;
    for (uint32_t i = 0; i < solver.nVars(); i++) {
        solver.binwatches[i*2].clear();
        solver.binwatches[i*2+1].clear();
        solver.watches[i*2].clear();
        solver.watches[i*2+1].clear();
        solver.xorwatches[i].clear();
    }
    solver.varReplacer->reattachInternalClauses();
    cleanAndAttachClauses(solver.binaryClauses);
    cleanAndAttachClauses(solver.clauses);
    cleanAndAttachClauses(solver.learnts);
    cleanAndAttachClauses(solver.xorclauses);
}

void FailedVarSearcher::printResults(const double myTime) const
{
    std::cout << "c |  Flit: "<< std::setw(5) << numFailed <<
    " Blit: " << std::setw(6) << goodBothSame <<
    " bXBeca: " << std::setw(4) << newBinXor <<
    " bXProp: " << std::setw(4) << bothInvert <<
    " Bins:" << std::setw(7) << binClauseAdded <<
    " P: " << std::setw(4) << std::fixed << std::setprecision(1) << (double)(solver.propagations - origProps)/1000000.0  << "M"
    " T: " << std::setw(5) << std::fixed << std::setprecision(2) << cpuTime() - myTime <<
    std::setw(5) << " |" << std::endl;
}

const bool FailedVarSearcher::orderLits()
{
    uint64_t oldProps = solver.propagations;
    double myTime = cpuTime();
    uint32_t numChecked = 0;
    litDegrees.clear();
    litDegrees.resize(solver.nVars()*2, 0);
    BitArray alreadyTested;
    alreadyTested.resize(solver.nVars()*2, 0);
    uint32_t i;
    
    for (i = 0; i < 3*solver.order_heap.size(); i++) {
        if (solver.propagations - oldProps > 1500000) break;
        Var var = solver.order_heap[solver.mtrand.randInt(solver.order_heap.size()-1)];
        if (solver.assigns[var] != l_Undef || !solver.decision_var[var]) continue;

        Lit randLit(var, solver.mtrand.randInt(1));
        if (alreadyTested[randLit.toInt()]) continue;
        alreadyTested.setBit(randLit.toInt());

        numChecked++;
        solver.newDecisionLevel();
        solver.uncheckedEnqueueLight(randLit);
        failed = (solver.propagateBin() != NULL);
        if (failed) {
            solver.cancelUntil(0);
            solver.uncheckedEnqueue(~randLit);
            solver.ok = (solver.propagate() == NULL);
            if (!solver.ok) return false;
            continue;
        }
        assert(solver.decisionLevel() > 0);
        for (int c = solver.trail.size()-1; c > (int)solver.trail_lim[0]; c--) {
            Lit x = solver.trail[c];
            litDegrees[x.toInt()]++;
        }
        solver.cancelUntil(0);
    }
    std::cout << "c binary Degree finding time: " << cpuTime() - myTime << " s  num checked: " << numChecked << " i: " << i << " props: " << (solver.propagations - oldProps) << std::endl;
    solver.propagations = oldProps;

    return true;
}

void FailedVarSearcher::removeOldLearnts()
{
    for (Clause **it = solver.removedLearnts.getData(), **end = solver.removedLearnts.getDataEnd(); it != end; it++) {
        solver.detachClause(**it);
    }
}

struct reduceDB_ltOldLearnt
{
    bool operator () (const Clause* x, const Clause* y) {
        return x->size() > y->size();
    }
};

const bool FailedVarSearcher::readdRemovedLearnts()
{
    uint32_t toRemove = (solver.removedLearnts.size() > MAX_OLD_LEARNTS) ? (solver.removedLearnts.size() - MAX_OLD_LEARNTS/4) : 0;
    if (toRemove > 0)
        std::sort(solver.removedLearnts.getData(), solver.removedLearnts.getDataEnd(), reduceDB_ltOldLearnt());

    Clause **it1, **it2;
    it1 = it2 = solver.removedLearnts.getData();
    for (Clause **end = solver.removedLearnts.getDataEnd(); it1 != end; it1++) {
        if (toRemove > 0) {
            clauseFree(*it1);
            toRemove--;
            continue;
        }
        
        Clause* c = solver.addClauseInt(**it1, (**it1).getGroup());
        clauseFree(*it1);
        if (c != NULL) {
            *it2 = c;
            it2++;
        }
        if (!solver.ok) {
            it1++;
            for (; it1 != end; it1++) clauseFree(*it1);
        }
    }
    solver.removedLearnts.shrink(it1-it2);
    //std::cout << "Readded old learnts. New facts:" << (int)origHeapSize - (int)solver.order_heap.size() << std::endl;

    return solver.ok;
}

#define MAX_REMOVE_BIN_FULL_PROPS 20000000
#define EXTRATIME_DIVIDER 3

template<bool startUp>
const bool FailedVarSearcher::removeUslessBinFull()
{
    if (!solver.performReplace) return true;
    while (solver.performReplace && solver.varReplacer->getClauses().size() > 0) {
        if (!solver.varReplacer->performReplace(true)) return false;
        solver.clauseCleaner->removeAndCleanAll(true);
    }
    assert(solver.varReplacer->getClauses().size() == 0);
    solver.testAllClauseAttach();
    if (startUp) {
        solver.clauseCleaner->moveBinClausesToBinClauses();
    }

    double myTime = cpuTime();
    toDeleteSet.clear();
    toDeleteSet.growTo(solver.nVars()*2, 0);
    uint32_t origHeapSize = solver.order_heap.size();
    uint64_t origProps = solver.propagations;
    bool fixed = false;
    uint32_t extraTime = solver.binaryClauses.size() / EXTRATIME_DIVIDER;

    for (uint32_t i = 0; i != solver.order_heap.size(); i++) {
        Var var = solver.order_heap[i];
        if (solver.propagations - origProps + extraTime > MAX_REMOVE_BIN_FULL_PROPS) break;
        if (solver.assigns[var] != l_Undef || !solver.decision_var[var]) continue;

        Lit lit(var, true);
        if (!removeUselessBinaries<startUp>(lit)) {
            fixed = true;
            solver.cancelUntil(0);
            solver.uncheckedEnqueue(~lit);
            solver.ok = (solver.propagate() == NULL);
            if (!solver.ok) return false;
            continue;
        }

        /*lit = ~lit;
        if (!removeUselessBinaries<startUp>(lit)) {
            fixed = true;
            solver.cancelUntil(0);
            solver.uncheckedEnqueue(~lit);
            solver.ok = (solver.propagate() == NULL);
            if (!solver.ok) return false;
            continue;
        }*/
    }

    Clause **i, **j;
    i = j = solver.binaryClauses.getData();
    uint32_t num = 0;
    for (Clause **end = solver.binaryClauses.getDataEnd(); i != end; i++, num++) {
        if (!(*i)->removed()) {
            *j++ = *i;
        } else {
            clauseFree(*i);
        }
    }
    uint32_t removedUselessBin = i - j;
    solver.binaryClauses.shrink(i - j);
    
    if (fixed) solver.order_heap.filter(Solver::VarFilter(solver));

    if (solver.verbosity >= 1) {
        std::cout
        << "c Removed useless bin:" << std::setw(8) << removedUselessBin
        << " fixed: " << std::setw(4) << (origHeapSize - solver.order_heap.size())
        << " props: " << std::fixed << std::setprecision(2) << std::setw(4) << (double)(solver.propagations - origProps)/1000000.0 << "M"
        << " time: " << std::fixed << std::setprecision(2) << std::setw(5) << cpuTime() - myTime << std::endl;
    }

    return true;
}

const bool FailedVarSearcher::tryBoth(const Lit lit1, const Lit lit2)
{
    if (binXorFind) {
        if (lastTrailSize < solver.trail.size()) {
            for (uint32_t i = lastTrailSize; i != solver.trail.size(); i++) {
                removeVarFromXors(solver.trail[i].var());
            }
        }
        lastTrailSize = solver.trail.size();
        xorClauseTouched.setZero();
        investigateXor.clear();
    }
    
    propagated.setZero();
    twoLongXors.clear();
    propagatedVars.clear();
    unPropagatedBin.setZero();
    bothSame.clear();
    
    solver.newDecisionLevel();
    solver.uncheckedEnqueueLight(lit1);
    failed = (solver.propagateLight() != NULL);
    if (failed) {
        solver.cancelUntil(0);
        numFailed++;
        solver.uncheckedEnqueue(~lit1);
        solver.ok = (solver.propagate(false) == NULL);
        if (!solver.ok) return false;
        return true;
    } else {
        assert(solver.decisionLevel() > 0);
        for (int c = solver.trail.size()-1; c >= (int)solver.trail_lim[0]; c--) {
            Var x = solver.trail[c].var();
            propagated.setBit(x);
            if (solver.addExtraBins) {
                unPropagatedBin.setBit(x);
                propagatedVars.push(x);
            }
            if (solver.assigns[x].getBool()) propValue.setBit(x);
            else propValue.clearBit(x);
            
            if (binXorFind) removeVarFromXors(x);
        }
        
        if (binXorFind) {
            for (uint32_t *it = investigateXor.getData(), *end = investigateXor.getDataEnd(); it != end; it++) {
                if (xorClauseSizes[*it] == 2)
                    twoLongXors.insert(getTwoLongXor(*solver.xorclauses[*it]));
            }
            for (int c = solver.trail.size()-1; c >= (int)solver.trail_lim[0]; c--) {
                addVarFromXors(solver.trail[c].var());
            }
            xorClauseTouched.setZero();
            investigateXor.clear();
        }
        
        solver.cancelUntil(0);
    }

    if (solver.addExtraBins && hyperbinProps < maxHyperBinProps) addBinClauses(lit1);
    propagatedVars.clear();
    unPropagatedBin.setZero();
    
    solver.newDecisionLevel();
    solver.uncheckedEnqueueLight(lit2);
    failed = (solver.propagateLight() != NULL);
    if (failed) {
        solver.cancelUntil(0);
        numFailed++;
        solver.uncheckedEnqueue(~lit2);
        solver.ok = (solver.propagate(false) == NULL);
        if (!solver.ok) return false;
        return true;
    } else {
        assert(solver.decisionLevel() > 0);
        for (int c = solver.trail.size()-1; c >= (int)solver.trail_lim[0]; c--) {
            Var     x  = solver.trail[c].var();
            if (propagated[x]) {
                if (solver.addExtraBins) {
                    unPropagatedBin.setBit(x);
                    propagatedVars.push(x);
                }
                if (propValue[x] == solver.assigns[x].getBool()) {
                    //they both imply the same
                    bothSame.push(Lit(x, !propValue[x]));
                } else if (c != (int)solver.trail_lim[0]) {
                    bool invert;
                    if (lit1.var() == lit2.var()) {
                        assert(lit1.sign() == false && lit2.sign() == true);
                        tmpPs[0] = Lit(lit1.var(), false);
                        tmpPs[1] = Lit(x, false);
                        invert = propValue[x];
                    } else {
                        tmpPs[0] = Lit(lit1.var(), false);
                        tmpPs[1] = Lit(lit2.var(), false);
                        invert = lit1.sign() ^ lit2.sign();
                    }
                    if (!solver.varReplacer->replace(tmpPs, invert, 0))
                        return false;
                    bothInvert += solver.varReplacer->getNewToReplaceVars() - toReplaceBefore;
                    toReplaceBefore = solver.varReplacer->getNewToReplaceVars();
                }
            }
            if (solver.assigns[x].getBool()) propValue.setBit(x);
            else propValue.clearBit(x);
            if (binXorFind) removeVarFromXors(x);
        }
        
        if (binXorFind) {
            if (twoLongXors.size() > 0) {
                for (uint32_t *it = investigateXor.getData(), *end = it + investigateXor.size(); it != end; it++) {
                    if (xorClauseSizes[*it] == 2) {
                        TwoLongXor tmp = getTwoLongXor(*solver.xorclauses[*it]);
                        if (twoLongXors.find(tmp) != twoLongXors.end()) {
                            tmpPs[0] = Lit(tmp.var[0], false);
                            tmpPs[1] = Lit(tmp.var[1], false);
                            if (!solver.varReplacer->replace(tmpPs, tmp.inverted, solver.xorclauses[*it]->getGroup()))
                                return false;
                            newBinXor += solver.varReplacer->getNewToReplaceVars() - toReplaceBefore;
                            toReplaceBefore = solver.varReplacer->getNewToReplaceVars();
                        }
                    }
                }
            }
            for (int c = solver.trail.size()-1; c >= (int)solver.trail_lim[0]; c--) {
                addVarFromXors(solver.trail[c].var());
            }
        }
        
        solver.cancelUntil(0);
    }

    if (solver.addExtraBins && hyperbinProps < maxHyperBinProps) addBinClauses(lit2);
    
    for(uint32_t i = 0; i != bothSame.size(); i++) {
        solver.uncheckedEnqueue(bothSame[i]);
    }
    goodBothSame += bothSame.size();
    solver.ok = (solver.propagate(false) == NULL);
    if (!solver.ok) return false;
    
    return true;
}

struct litOrder
{
    litOrder(const vector<uint32_t>& _litDegrees) :
    litDegrees(_litDegrees)
    {}
    
    bool operator () (const Lit& x, const Lit& y) {
        return litDegrees[x.toInt()] > litDegrees[y.toInt()];
    }
    
    const vector<uint32_t>& litDegrees;
};

void FailedVarSearcher::addBinClauses(const Lit& lit)
{
    uint64_t oldProps = solver.propagations;
    #ifdef VERBOSE_DEBUG
    std::cout << "Checking one BTC vs UP" << std::endl;
    #endif //VERBOSE_DEBUG
    vec<Lit> toVisit;
    
    solver.newDecisionLevel();
    solver.uncheckedEnqueueLight(lit);
    failed = (solver.propagateBin() != NULL);
    assert(!failed);

    assert(solver.decisionLevel() > 0);
    if (propagatedVars.size() - (solver.trail.size()-solver.trail_lim[0]) == 0) {
        solver.cancelUntil(0);
        goto end;
    }
    for (int c = solver.trail.size()-1; c >= (int)solver.trail_lim[0]; c--) {
        Lit x = solver.trail[c];
        unPropagatedBin.clearBit(x.var());
        toVisit.push(x);
    }
    solver.cancelUntil(0);

    std::sort(toVisit.getData(), toVisit.getDataEnd(), litOrder(litDegrees));
    /*************************
    //To check that the ordering is the right way
    // --> i.e. to avoid mistake present in Glucose's ordering
    for (uint32_t i = 0; i < toVisit.size(); i++) {
        std::cout << "i:" << std::setw(8) << i << " degree:" << litDegrees[toVisit[i].toInt()] << std::endl;
    }
    std::cout << std::endl;
    ***************************/

    //difference between UP and BTC is in unPropagatedBin
    for (Lit *l = toVisit.getData(), *end = toVisit.getDataEnd(); l != end; l++) {
        #ifdef VERBOSE_DEBUG
        std::cout << "Checking visit level " << end-l-1 << std::endl;
        uint32_t thisLevel = 0;
        #endif //VERBOSE_DEBUG
        fillImplies(*l);
        if (unPropagatedBin.nothingInCommon(myimplies)) goto next;
        for (const Var *var = propagatedVars.getData(), *end2 = propagatedVars.getDataEnd(); var != end2; var++) {
            if (unPropagatedBin[*var] && myimplies[*var]) {
                #ifdef VERBOSE_DEBUG
                thisLevel++;
                #endif //VERBOSE_DEBUG
                addBin(~*l, Lit(*var, !propValue[*var]));
                unPropagatedBin.removeThese(myImpliesSet);
                if (unPropagatedBin.isZero()) {
                    myimplies.removeThese(myImpliesSet);
                    myImpliesSet.clear();
                    goto end;
                }
            }
        }
        next:
        myimplies.removeThese(myImpliesSet);
        myImpliesSet.clear();
        #ifdef VERBOSE_DEBUG
        if (thisLevel > 0) {
            std::cout << "Added " << thisLevel << " level diff:" << end-l-1 << std::endl;
        }
        #endif //VERBOSE_DEBUG
    }
    assert(unPropagatedBin.isZero());

    end:
    hyperbinProps += solver.propagations - oldProps;
}

void FailedVarSearcher::fillImplies(const Lit& lit)
{
    solver.newDecisionLevel();
    solver.uncheckedEnqueue(lit);
    failed = (solver.propagateLight() != NULL);
    assert(!failed);
    
    assert(solver.decisionLevel() > 0);
    for (int c = solver.trail.size()-1; c >= (int)solver.trail_lim[0]; c--) {
        Lit x = solver.trail[c];
        myimplies.setBit(x.var());
        myImpliesSet.push(x.var());
    }
    solver.cancelUntil(0);
}

template<bool startUp>
const bool FailedVarSearcher::fillBinImpliesMinusLast(const Lit& origLit, const Lit& lit, vec<Lit>& wrong)
{
    solver.newDecisionLevel();
    solver.uncheckedEnqueueLight(lit);
    //if it's a cycle, it doesn't work, so don't propagate origLit
    failed = (solver.propagateBinExcept<startUp>(origLit) != NULL);
    if (failed) return false;

    assert(solver.decisionLevel() > 0);
    int c;
    extraTime += (solver.trail.size() - solver.trail_lim[0]) / EXTRATIME_DIVIDER;
    for (c = solver.trail.size()-1; c > (int)solver.trail_lim[0]; c--) {
        Lit x = solver.trail[c];
        if (toDeleteSet[x.toInt()]) {
            wrong.push(x);
            toDeleteSet[x.toInt()] = false;
        };
        solver.assigns[x.var()] = l_Undef;
    }
    solver.assigns[solver.trail[c].var()] = l_Undef;
    
    solver.qhead = solver.trail_lim[0];
    solver.trail.shrink_(solver.trail.size() - solver.trail_lim[0]);
    solver.trail_lim.clear();
    //solver.cancelUntil(0);

    return true;
}

void FailedVarSearcher::addBin(const Lit& lit1, const Lit& lit2)
{
    #ifdef VERBOSE_DEBUG
    std::cout << "Adding extra bin: ";
    lit1.print(); std::cout << " "; lit2.printFull();
    #endif //VERBOSE_DEBUG

    tmpPs[0] = lit1;
    tmpPs[1] = lit2;
    solver.addLearntClause(tmpPs, 0, 0);
    tmpPs.growTo(2);
    assert(solver.ok);
}

template<bool startUp>
const bool FailedVarSearcher::removeUselessBinaries(const Lit& lit)
{
    //Nothing can be learnt at this point!
    //Otherwise, it might happen that the path to X has learnts,
    //but binary clause to X is not learnt.
    //So we remove X , then we might remove
    //the path (since it's learnt) -- removing a FACT!!
    //[note:removal can be through variable elimination
    //, and removeWrong() will happily remove it
    assert(!startUp || solver.learnts.size() == 0);

    solver.newDecisionLevel();
    solver.uncheckedEnqueueLight(lit);
    failed = (solver.propagateBinOneLevel<startUp>() != NULL);
    if (failed) return false;
    bool ret = true;

    oneHopAway.clear();
    assert(solver.decisionLevel() > 0);
    int c;
    if (solver.trail.size()-solver.trail_lim[0] == 0) {
        solver.cancelUntil(0);
        goto end;
    }
    extraTime += (solver.trail.size() - solver.trail_lim[0]) / EXTRATIME_DIVIDER;
    for (c = solver.trail.size()-1; c > (int)solver.trail_lim[0]; c--) {
        Lit x = solver.trail[c];
        toDeleteSet[x.toInt()] = true;
        oneHopAway.push(x);
        solver.assigns[x.var()] = l_Undef;
    }
    solver.assigns[solver.trail[c].var()] = l_Undef;
    
    solver.qhead = solver.trail_lim[0];
    solver.trail.shrink_(solver.trail.size() - solver.trail_lim[0]);
    solver.trail_lim.clear();
    //solver.cancelUntil(0);

    wrong.clear();
    for(uint32_t i = 0; i < oneHopAway.size(); i++) {
        //no need to visit it if it already queued for removal
        //basically, we check if it's in 'wrong'
        if (toDeleteSet[oneHopAway[i].toInt()]) {
            if (!fillBinImpliesMinusLast<startUp>(lit, oneHopAway[i], wrong)) {
                ret = false;
                goto end;
            }
        }
    }

    for (uint32_t i = 0; i < wrong.size(); i++) {
        removeBin(~lit, wrong[i]);
    }
    
    end:
    for(uint32_t i = 0; i < oneHopAway.size(); i++) {
        toDeleteSet[oneHopAway[i].toInt()] = false;
    }

    return ret;
}
template const bool FailedVarSearcher::removeUselessBinaries <true>(const Lit& lit);
template const bool FailedVarSearcher::removeUselessBinaries <false>(const Lit& lit);
template const bool FailedVarSearcher::fillBinImpliesMinusLast <true>(const Lit& origLit, const Lit& lit, vec<Lit>& wrong);
template const bool FailedVarSearcher::fillBinImpliesMinusLast <false>(const Lit& origLit, const Lit& lit, vec<Lit>& wrong);
template const bool FailedVarSearcher::removeUslessBinFull <true>();
template const bool FailedVarSearcher::removeUslessBinFull <false>();

void FailedVarSearcher::removeBin(const Lit& lit1, const Lit& lit2)
{
    /*******************
    Lit litFind1 = lit_Undef;
    Lit litFind2 = lit_Undef;
    
    if (solver.binwatches[(~lit1).toInt()].size() < solver.binwatches[(~lit2).toInt()].size()) {
        litFind1 = lit1;
        litFind2 = lit2;
    } else {
        litFind1 = lit2;
        litFind2 = lit1;
    }
    ********************/

    //Find AND remove from watches
    vec<WatchedBin>& bwin = solver.binwatches[(~lit1).toInt()];
    extraTime += bwin.size() / EXTRATIME_DIVIDER;
    Clause *cl = NULL;
    WatchedBin *i, *j;
    i = j = bwin.getData();
    for (const WatchedBin *end = bwin.getDataEnd(); i != end; i++) {
        if (i->impliedLit == lit2 && cl == NULL) {
            cl = i->clause;
        } else {
            *j++ = *i;
        }
    }
    bwin.shrink(1);
    assert(cl != NULL);

    bool found = false;
    vec<WatchedBin>& bwin2 = solver.binwatches[(~lit2).toInt()];
    extraTime += bwin2.size() / EXTRATIME_DIVIDER;
    i = j = bwin2.getData();
    for (const WatchedBin *end = bwin2.getDataEnd(); i != end; i++) {
        if (i->clause == cl) {
            found = true;
        } else {
            *j++ = *i;
        }
    }
    bwin2.shrink(1);
    assert(found);

    #ifdef VERBOSE_DEBUG
    std::cout << "Removing useless bin: ";
    cl->plainPrint();
    #endif //VERBOSE_DEBUG

    cl->setRemoved();
    solver.clauses_literals -= 2;
}

template<class T>
inline void FailedVarSearcher::cleanAndAttachClauses(vec<T*>& cs)
{
    T **i = cs.getData();
    T **j = i;
    for (T **end = cs.getDataEnd(); i != end; i++) {
        if (cleanClause(**i)) {
            solver.attachClause(**i);
            *j++ = *i;
        } else {
            clauseFree(*i);
        }
    }
    cs.shrink(i-j);
}

inline const bool FailedVarSearcher::cleanClause(Clause& ps)
{
    uint32_t origSize = ps.size();
    
    Lit *i = ps.getData();
    Lit *j = i;
    for (Lit *end = ps.getDataEnd(); i != end; i++) {
        if (solver.value(*i) == l_True) return false;
        if (solver.value(*i) == l_Undef) {
            *j++ = *i;
        }
    }
    ps.shrink(i-j);
    assert(ps.size() > 1);
    
    if (ps.size() != origSize) ps.setStrenghtened();
    if (origSize != 2 && ps.size() == 2)
        solver.becameBinary++;
    
    return true;
}

inline const bool FailedVarSearcher::cleanClause(XorClause& ps)
{
    uint32_t origSize = ps.size();
    
    Lit *i = ps.getData(), *j = i;
    for (Lit *end = ps.getDataEnd(); i != end; i++) {
        if (solver.assigns[i->var()] == l_True) ps.invert(true);
        if (solver.assigns[i->var()] == l_Undef) {
            *j++ = *i;
        }
    }
    ps.shrink(i-j);
    
    if (ps.size() == 0) return false;
    assert(ps.size() > 1);
    
    if (ps.size() != origSize) ps.setStrenghtened();
    if (ps.size() == 2) {
        ps[0] = ps[0].unsign();
        ps[1] = ps[1].unsign();
        solver.varReplacer->replace(ps, ps.xor_clause_inverted(), ps.getGroup());
        return false;
    }
    
    return true;
}



/***************
UNTESTED CODE
*****************
const bool FailedVarSearcher::tryAll(const Lit* begin, const Lit* end)
{
    propagated.setZero();
    BitArray propagated2;
    propagated2.resize(solver.nVars(), 0);
    propValue.resize(solver.nVars(), 0);
    bool first = true;
    bool last = false;

    for (const Lit *it = begin; it != end; it++, first = false) {
        if (it+1 == end) last = true;

        if (!first && !last) propagated2.setZero();
        solver.newDecisionLevel();
        solver.uncheckedEnqueue(*it);
        failed = (solver.propagate(false) != NULL);
        if (failed) {
            solver.cancelUntil(0);
            numFailed++;
            solver.uncheckedEnqueue(~(*it));
            solver.ok = (solver.propagate(false) == NULL);
            if (!solver.ok) return false;
            return true;
        } else {
            assert(solver.decisionLevel() > 0);
            for (int c = solver.trail.size()-1; c >= (int)solver.trail_lim[0]; c--) {
                Var x = solver.trail[c].var();
                if (last) {
                    if (propagated[x] && propValue[x] == solver.assigns[x].getBool())
                        bothSame.push_back(make_pair(x, !propValue[x]));
                } else {
                    if (first) {
                        propagated.setBit(x);
                        if (solver.assigns[x].getBool())
                            propValue.setBit(x);
                        else
                            propValue.clearBit(x);
                    } else if (propValue[x] == solver.assigns[x].getBool()) {
                        propagated2.setBit(x);
                    }
                }
            }
            solver.cancelUntil(0);
        }
        if (!last && !first) {
            propagated &= propagated2;
            if (propagated.isZero()) return true;
        }
    }

    for(uint32_t i = 0; i != bothSame.size(); i++) {
        solver.uncheckedEnqueue(Lit(bothSame[i].first, bothSame[i].second));
    }
    goodBothSame += bothSame.size();
    bothSame.clear();
    solver.ok = (solver.propagate(false) == NULL);
    if (!solver.ok) return false;

    return true;
}
**************
Untested code end
**************/
