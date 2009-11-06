#ifndef __MATRIXFINDER_H__
#define __MATRIXFINDER_H__

#include <sys/types.h>

class Solver;

class MatrixFinder {
    
    public:
        MatrixFinder(Solver* S);
        uint numMatrix;
    
    private:
        
        Solver* S;
};


#endif
