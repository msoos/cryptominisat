#ifndef __CL_ABSTRACTION__H__
#define __CL_ABSTRACTION__H__

#define CL_ABST_TYPE uint32_t
#define CLAUSE_ABST_SIZE 32
#include "constants.h"

inline CL_ABST_TYPE abst_var(const uint32_t v)
{
    return 1UL << (v % CLAUSE_ABST_SIZE);
}

template <class T>
CL_ABST_TYPE calcAbstraction(const T& ps)
{
    CL_ABST_TYPE abstraction = 0;
    if (ps.size() > 200) {
        return ~((CL_ABST_TYPE)(0ULL));
    }

    for (auto l: ps)
        abstraction |= abst_var(l.var());

    return abstraction;
}

#endif //__CL_ABSTRACTION__H__
