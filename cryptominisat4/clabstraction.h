#ifndef __CL_ABSTRACTION__H__
#define __CL_ABSTRACTION__H__

typedef uint32_t cl_abst_type;
static const int cl_abst_modulo = 29;

inline cl_abst_type abst_var(const uint32_t v)
{
    return 1UL << (v % cl_abst_modulo);
}

template <class T>
cl_abst_type calcAbstraction(const T& ps)
{
    cl_abst_type abstraction = 0;
    if (ps.size() > 100) {
        return ~((cl_abst_type)(0ULL));
    }

    for (auto l: ps)
        abstraction |= abst_var(l.var());

    return abstraction;
}

#endif //__CL_ABSTRACTION__H__
