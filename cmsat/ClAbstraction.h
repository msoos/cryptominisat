#ifndef __CL_ABSTRACTION__H__
#define __CL_ABSTRACTION__H__

#define CL_ABST_TYPE uint32_t
#define CLAUSE_ABST_SIZE 32

template <class T> CL_ABST_TYPE calcAbstraction(const T& ps)
{
    CL_ABST_TYPE abstraction = 0;
    for (uint16_t i = 0; i != ps.size(); i++)
        abstraction |= 1UL << (ps[i].var() % CLAUSE_ABST_SIZE);
    return abstraction;
}

#endif //__CL_ABSTRACTION__H__