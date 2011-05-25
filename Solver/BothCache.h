#ifndef BOTHCACHE_H
#define BOTHCACHE_H

#include "Solver.h"

namespace CMSat {

class BothCache
{
    public:
        BothCache(Solver& solver);
        const bool tryBoth();

    private:
        Solver& solver;
};

}

#endif //BOTHCACHE_H
