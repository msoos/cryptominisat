#ifndef FAILEDVARSEARCHER_H
#define FAILEDVARSEARCHER_H

#include "SolverTypes.h"
class Solver;

class FailedVarSearcher {
    public:
        FailedVarSearcher(Solver& _solver);
    
        const lbool search(const double maxTime);
        
    private:
        Solver& solver;
        bool finishedLastTime;
        uint32_t lastTimeWentUntil;
};


#endif //FAILEDVARSEARCHER_H