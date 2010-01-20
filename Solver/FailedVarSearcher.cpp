#include "FailedVarSearcher.h"

#include <iomanip>

#include "Solver.h"
#include "ClauseCleaner.h"
#include "time_mem.h"

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
            
            solver.newDecisionLevel();
            solver.uncheckedEnqueue(Lit(var, false));
            failed = (solver.propagate() != NULL);
            solver.cancelUntil(0);
            if (failed) {
                num++;
                solver.uncheckedEnqueue(Lit(var, true));
                solver.ok = (solver.propagate() == NULL);
                if (!solver.ok) return l_False;
                continue;
            }
            
            solver.newDecisionLevel();
            solver.uncheckedEnqueue(Lit(var, true));
            failed = (solver.propagate() != NULL);
            solver.cancelUntil(0);
            if (failed) {
                //std::cout << "Var " << i << " fails l_False" << std::endl;
                num++;
                solver.uncheckedEnqueue(Lit(var, false));
                solver.ok = (solver.propagate() == NULL);
                if (!solver.ok) return l_False;
                continue;
            }
            
            solver.polarity[var] = oldPolarity;
        }
    }
    
    std::cout << "c |  Number of failed vars: " << std::setw(5) << num << " time: " << std::setw(5) << std::setprecision(2) << cpuTime() - time << " s"<< std::setw(34) << "|" << std::endl;
    
    if (num != 0) {
        solver.order_heap.filter(Solver::VarFilter(solver));
        solver.clauseCleaner->removeAndCleanAll();
    }
    
    return l_Undef;
}