#ifndef TRANSCACHE_H
#define TRANSCACHE_H

#include <vector>
#include <limits>

#ifdef _MSC_VER
#include <msvc/stdint.h>
#else
#include <stdint.h>
#endif //_MSC_VER

#include "SolverTypes.h"

class TransCache {
    public:
        TransCache() :
            conflictLastUpdated(std::numeric_limits<uint64_t>::max())
        {};

        std::vector<Lit> lits;
        uint64_t conflictLastUpdated;
};

#endif //TRANSCACHE_H