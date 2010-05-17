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

#ifdef _MSC_VER
#define __builtin_prefetch(a,b,c)
#endif //_MSC_VER

//#define VERBOSE_DEUBUG

FailedVarSearcher::FailedVarSearcher(Solver& _solver):
    solver(_solver)
    , finishedLastTimeVar(true)
    , lastTimeWentUntilVar(0)
    , finishedLastTimeBin(true)
    , lastTimeWentUntilBin(0)
    , numPropsMultiplier(1.0)
    , lastTimeFoundTruths(0)
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
    
    //Saving Solver state
    Heap<Solver::VarOrderLt> backup_order_heap(solver.order_heap);
    vector<bool> backup_polarities = solver.polarity;
    vec<uint32_t> backup_activity(solver.activity.size());
    std::copy(solver.activity.getData(), solver.activity.getDataEnd(), backup_activity.getData());
    uint32_t backup_var_inc = solver.var_inc;
    uint32_t origHeapSize = solver.order_heap.size();
    if (solver.readdOldLearnts && !readdRemovedLearnts()) goto end;
    
    //General Stats
    numFailed = 0;
    goodBothSame = 0;
    origProps = solver.propagations;
    
    //If failed var searching is going good, do successively more and more of it
    if (lastTimeFoundTruths > 500 || (double)lastTimeFoundTruths > (double)solver.order_heap.size() * 0.03) std::max(numPropsMultiplier*1.7, 5.0);
    else numPropsMultiplier = 1.0;
    numProps = (uint64_t) ((double)numProps * numPropsMultiplier);
    
    //For BothSame
    propagated.resize(solver.nVars(), 0);
    propValue.resize(solver.nVars(), 0);
    bothSame.clear();
    
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
    
    uint32_t fromBin;
    uint32_t fromVar;
    if (finishedLastTimeVar || lastTimeWentUntilVar >= solver.nVars())
        fromVar = 0;
    else
        fromVar = lastTimeWentUntilVar;
    finishedLastTimeVar = true;
    lastTimeWentUntilVar = solver.nVars();
    for (Var var = fromVar; var < solver.nVars(); var++) {
        if (solver.assigns[var] == l_Undef && solver.order_heap.inHeap(var)) {
            if (solver.propagations - origProps >= numProps)  {
                finishedLastTimeVar = false;
                lastTimeWentUntilVar = var;
                break;
            }
            if (!tryBoth(Lit(var, false), Lit(var, true)))
                goto end;
        }
    }
    
    if (solver.verbosity >= 1) printResults(myTime);
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
    }
    
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
    if (solver.readdOldLearnts && solver.ok) removeOldLearnts();

    //Print results
    if (solver.verbosity >= 1) printResults(myTime);
    
    solver.order_heap.filter(Solver::VarFilter(solver));
    
    if (solver.ok && (numFailed || goodBothSame)) {
        double time = cpuTime();
        if ((int)origHeapSize - (int)solver.order_heap.size() >  (int)origHeapSize/15 && solver.nClauses() + solver.learnts.size() > 500000) {
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
        } else {
            solver.clauseCleaner->removeAndCleanAll();
        }
        if (solver.verbosity >= 1 && numFailed + goodBothSame > 100) {
            std::cout << "c |  Cleaning up after failed var search: " << std::setw(8) << std::fixed << std::setprecision(2) << cpuTime() - time << " s "
            <<  std::setw(33) << " | " << std::endl;
        }
    }
    
    lastTimeFoundTruths = solver.trail.size() - origTrailSize;
    
    //Restore Solver state
    solver.var_inc = backup_var_inc;
    std::copy(backup_activity.getData(), backup_activity.getDataEnd(), solver.activity.getData());
    solver.order_heap = backup_order_heap;
    solver.polarity = backup_polarities;
    solver.order_heap.filter(Solver::VarFilter(solver));
    
    solver.testAllClauseAttach();
    return solver.ok;
}

void FailedVarSearcher::printResults(const double myTime) const
{
    std::cout << "c |  Failv: "<< std::setw(5) << numFailed <<
    " Bprop v: " << std::setw(6) << goodBothSame <<
    " bXBeca: " << std::setw(5) << newBinXor <<
    " bXProp: " << std::setw(5) << bothInvert <<
    " Prop: " << std::setw(5) << std::setprecision(2) << (solver.propagations - origProps)/1000  << "t"
    " Time: " << std::setw(5) << std::fixed << std::setprecision(2) << cpuTime() - myTime <<
    std::setw(5) << " |" << std::endl;
}

void FailedVarSearcher::removeOldLearnts()
{
    for (Clause **it = solver.removedLearnts.getData(), **end = solver.removedLearnts.getDataEnd(); it != end; it++) {
        solver.detachClause(**it);
    }
}

const bool FailedVarSearcher::readdRemovedLearnts()
{
    Clause **it1, **it2;
    it1 = it2 = solver.removedLearnts.getData();
    for (Clause **end = solver.removedLearnts.getDataEnd(); it1 != end; it1++) {
        Clause* c = solver.addClauseInt(**it1, (**it1).getGroup());
        free(*it1);
        if (c != NULL) {
            *it2 = c;
            it2++;
        }
        if (!solver.ok) {
            it1++;
            for (; it1 != end; it1++) free(*it1);
        }
    }
    solver.removedLearnts.shrink(it1-it2);
    //std::cout << "Readded old learnts. New facts:" << (int)origHeapSize - (int)solver.order_heap.size() << std::endl;

    return solver.ok;
}

const bool FailedVarSearcher::tryBoth(const Lit lit1, const Lit lit2)
{
    vec<Lit> tmpPs(2);
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
    
    solver.newDecisionLevel();
    solver.uncheckedEnqueue(lit1);
    failed = (solver.propagate(false) != NULL);
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
            if (solver.assigns[x].getBool())
                propValue.setBit(x);
            else
                propValue.clearBit(x);
            
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
    
    solver.newDecisionLevel();
    solver.uncheckedEnqueue(lit2);
    failed = (solver.propagate(false) != NULL);
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
                if (propValue[x] == solver.assigns[x].getBool()) {
                    //they both imply the same
                    bothSame.push_back(make_pair(x, !propValue[x]));
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
    
    for(uint32_t i = 0; i != bothSame.size(); i++) {
        solver.uncheckedEnqueue(Lit(bothSame[i].first, bothSame[i].second));
    }
    goodBothSame += bothSame.size();
    bothSame.clear();
    solver.ok = (solver.propagate(false) == NULL);
    if (!solver.ok) return false;
    
    return true;
}

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
            free(*i);
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
