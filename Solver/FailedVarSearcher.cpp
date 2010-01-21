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
{
}

const lbool FailedVarSearcher::search(const double maxTime)
{
    assert(solver.decisionLevel() == 0);
    
    double time = cpuTime();
    uint32_t num = 0;
    bool failed;
    uint32_t from;
    if (finishedLastTime || lastTimeWentUntil >= solver.order_heap.size())
        from = 0;
    else {
        from = lastTimeWentUntil;
    }
    
    BitArray propagated;
    propagated.resize(solver.nVars());
    BitArray propValue;
    propValue.resize(solver.nVars());
    vector<pair<Var, bool> > bothSame;
    uint goodBothSame = 0;
    
    finishedLastTime = true;
    lastTimeWentUntil = solver.order_heap.size();
    for (uint32_t i = from; i < solver.order_heap.size(); i++) {
        Var var = solver.order_heap[i];
        if (solver.assigns[var] == l_Undef) {
            if (cpuTime() - time >= maxTime)  {
                finishedLastTime = false;
                lastTimeWentUntil = var;
                break;
            }
            
            bool oldPolarity = solver.polarity[var];
            propagated.setZero();
            
            solver.newDecisionLevel();
            solver.uncheckedEnqueue(Lit(var, false));
            failed = (solver.propagate() != NULL);
            if (failed) {
                solver.cancelUntil(0);
                num++;
                solver.uncheckedEnqueue(Lit(var, true));
                solver.ok = (solver.propagate() == NULL);
                if (!solver.ok) return l_False;
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
            failed = (solver.propagate() != NULL);
            if (failed) {
                solver.cancelUntil(0);
                //std::cout << "Var " << i << " fails l_False" << std::endl;
                num++;
                solver.uncheckedEnqueue(Lit(var, false));
                solver.ok = (solver.propagate() == NULL);
                if (!solver.ok) return l_False;
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
            
            for(uint i = 0; i != bothSame.size(); i++)
                solver.uncheckedEnqueue(Lit(bothSame[i].first, bothSame[i].second));
            goodBothSame += bothSame.size();
            bothSame.clear();
            solver.ok = (solver.propagate() == NULL);
            if (!solver.ok) return l_False;
            
            solver.polarity[var] = oldPolarity;
        }
    }
    
    std::cout << "c |  Number of both propagated vars: " << std::setw(5) << goodBothSame << std::endl;
    std::cout << "c |  Number of failed vars: " << std::setw(5) << num << " time: " << std::setw(5) << std::setprecision(2) << cpuTime() - time << " s"<< std::setw(34) << "|" << std::endl;
    
    if (num != 0) {
        solver.order_heap.filter(Solver::VarFilter(solver));
        solver.clauseCleaner->removeAndCleanAll();
    }
    
    return l_Undef;
}