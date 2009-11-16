#ifndef __FIND_UNDEF__
#define __FIND_UNDEF__

#include <vector>
using std::vector;

#include "Solver.h"

class FindUndef {
    public:
        FindUndef(Solver& S);
        const uint unRoll();
        
    private:
        Solver& S;
        
        void updateFixNeed();
        
        vector<bool> fixNeed; //If set to TRUE, then that variable is needed for sure
        vector<bool> dontLookAtClause; //If set to TRUE, then that clause already has only 1 lit that is true, so it can be skipped during updateFixNeed()
        
};

#endif