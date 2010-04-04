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
using std::make_pair;

#include "Solver.h"
#include "ClauseCleaner.h"
#include "time_mem.h"
#include "BitArray.h"

FailedVarSearcher::FailedVarSearcher(Solver& _solver):
    solver(_solver)
    , finishedLastTime(true)
    , lastTimeWentUntil(0)
    , lastTimeFoundTruths(0)
    , numPropsMultiplier(1.0)
{
}

const bool FailedVarSearcher::search(uint64_t numProps)
{
    assert(solver.decisionLevel() == 0);
    
    //Saving Solver state
    Heap<Solver::VarOrderLt> backup_order_heap(solver.order_heap);
    vector<bool> backup_polarities = solver.polarity;
    vec<double> backup_activity;
    backup_activity.growTo(solver.activity.size());
    memcpy(backup_activity.getData(), solver.activity.getData(), solver.activity.size()*sizeof(double));
    double backup_var_inc = solver.var_inc;
    
    //General Stats
    double time = cpuTime();
    uint32_t numFailed = 0;
    uint32_t goodBothSame = 0;
    uint32_t from;
    if (finishedLastTime || lastTimeWentUntil >= solver.nVars())
        from = 0;
    else
        from = lastTimeWentUntil;
    uint64_t origProps = solver.propagations;
    
    //If failed var searching is going good, do successively more and more of it
    if (lastTimeFoundTruths > 500) numPropsMultiplier *= 1.7;
    else numPropsMultiplier = 1.0;
    numProps = (uint64_t) ((double)numProps * numPropsMultiplier);
    
    //For failure
    bool failed;
    
    //For BothSame
    BitArray propagated;
    propagated.resize(solver.nVars());
    BitArray propValue;
    propValue.resize(solver.nVars());
    vector<pair<Var, bool> > bothSame;
    
    
    finishedLastTime = true;
    lastTimeWentUntil = solver.nVars();
    for (Var var = from; var < solver.nVars(); var++) {
        if (solver.assigns[var] == l_Undef && solver.order_heap.inHeap(var)) {
            if ((int)solver.propagations - (int)origProps >= (int)numProps)  {
                finishedLastTime = false;
                lastTimeWentUntil = var;
                break;
            }
            
            propagated.setZero();
            
            solver.newDecisionLevel();
            solver.uncheckedEnqueue(Lit(var, false));
            failed = (solver.propagate(false) != NULL);
            if (failed) {
                solver.cancelUntil(0);
                numFailed++;
                solver.uncheckedEnqueue(Lit(var, true));
                solver.ok = (solver.propagate(false) == NULL);
                if (!solver.ok) goto end;
                continue;
            } else {
                assert(solver.decisionLevel() > 0);
                for (int c = solver.trail.size()-1; c >= (int)solver.trail_lim[0]; c--) {
                    Var     x  = solver.trail[c].var();
                    propagated.setBit(x);
                    if (solver.assigns[x].getBool())
                        propValue.setBit(x);
                    else
                        propValue.clearBit(x);
                }
                solver.cancelUntil(0);
            }
            
            solver.newDecisionLevel();
            solver.uncheckedEnqueue(Lit(var, true));
            failed = (solver.propagate(false) != NULL);
            if (failed) {
                solver.cancelUntil(0);
                //std::cout << "Var " << i << " fails l_False" << std::endl;
                numFailed++;
                solver.uncheckedEnqueue(Lit(var, false));
                solver.ok = (solver.propagate(false) == NULL);
                if (!solver.ok) goto end;
                continue;
            } else {
                assert(solver.decisionLevel() > 0);
                for (int c = solver.trail.size()-1; c >= (int)solver.trail_lim[0]; c--) {
                    Var     x  = solver.trail[c].var();
                    if (propagated[x] && propValue[x] == solver.assigns[x].getBool())
                        bothSame.push_back(make_pair(x, !propValue[x]));
                }
                solver.cancelUntil(0);
            }
            
            for(uint32_t i = 0; i != bothSame.size(); i++)
                solver.uncheckedEnqueue(Lit(bothSame[i].first, bothSame[i].second));
            goodBothSame += bothSame.size();
            bothSame.clear();
            solver.ok = (solver.propagate(false) == NULL);
            if (!solver.ok) goto end;
        }
    }

end:
    //Restoring Solver state
    if (solver.verbosity >= 1) {
        std::cout << "c |  No. failvars: "<< std::setw(5) << numFailed <<
        "     No. bothprop vars: " << std::setw(6) << goodBothSame <<
        " Props: " << std::setw(8) << std::setprecision(2) << (int)solver.propagations - (int)origProps  <<
        " Time: " << std::setw(6) << std::fixed << std::setprecision(2) << cpuTime() - time <<
        std::setw(8) << " |" << std::endl;
    }
    
    if (numFailed || goodBothSame) {
        double time = cpuTime();
        solver.clauseCleaner->removeAndCleanAll();
        if (solver.verbosity >= 1 && numFailed + goodBothSame > 100) {
            std::cout << "c |  Cleaning up after failed var search: " << std::setw(8) << std::fixed << std::setprecision(2) << cpuTime() - time << " s "
            <<  std::setw(33) << " | " << std::endl;
        }
    }
    
    lastTimeFoundTruths = goodBothSame + numFailed;
    
    solver.var_inc = backup_var_inc;
    memcpy(solver.activity.getData(), backup_activity.getData(), solver.activity.size()*sizeof(double));
    solver.order_heap = backup_order_heap;
    solver.polarity = backup_polarities;
    solver.order_heap.filter(Solver::VarFilter(solver));
    
    return solver.ok;
}
