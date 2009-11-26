#ifndef FINDUNDEF_H
#define FINDUNDEF_H

#include <vector>
using std::vector;

#include "Solver.h"

class FindUndef {
    public:
        FindUndef(Solver& S);
        const uint unRoll();
        
    private:
        Solver& S;
        
        bool updateTables();
        void fillPotential();
        void unboundIsPotentials();
        
        vector<bool> dontLookAtClause; //If set to TRUE, then that clause already has only 1 lit that is true, so it can be skipped during updateFixNeed()
        vector<uint32_t> satisfies;
        vector<bool> isPotential;
        uint32_t isPotentialSum;
        
};

#endif //