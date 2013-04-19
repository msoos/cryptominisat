#ifndef __VARUPDATE_HELPER_H__
#define __VARUPDATE_HELPER_H__

#include "solvertypes.h"
#include <iostream>
#include <set>

namespace CMSat {

Var getUpdatedVar(Var toUpdate, const vector< uint32_t >& mapper);
Lit getUpdatedLit(Lit toUpdate, const vector< uint32_t >& mapper);

template<typename T>
void updateArray(T& toUpdate, const vector< uint32_t >& mapper)
{
    assert(toUpdate.size() >= mapper.size());
    T backup = toUpdate;
    for(size_t i = 0; i < mapper.size(); i++) {
        toUpdate[i] = backup[mapper[i]];
    }
}

template<typename T>
void updateArrayRev(T& toUpdate, const vector< uint32_t >& mapper)
{
    assert(toUpdate.size() >= mapper.size());
    T backup = toUpdate;
    for(size_t i = 0; i < mapper.size(); i++) {
        toUpdate[mapper[i]] = backup[i];
    }
}

template<typename T>
void updateArrayMapCopy(T& toUpdate, const vector< uint32_t >& mapper)
{
    assert(toUpdate.size() >= mapper.size());
    T backup = toUpdate;
    for(size_t i = 0; i < toUpdate.size(); i++) {
        if (backup[i] < mapper.size()) {
            toUpdate[i] = mapper[backup[i]];
        }
    }
}

template<typename T>
void updateLitsMap(T& toUpdate, const vector< uint32_t >& mapper)
{
    for(size_t i = 0; i < toUpdate.size(); i++) {
        if (toUpdate[i].var() < mapper.size()) {
            toUpdate[i] = getUpdatedLit(toUpdate[i], mapper);
        }
    }
}

inline Lit getUpdatedLit(Lit toUpdate, const vector< uint32_t >& mapper)
{
    return Lit(mapper[toUpdate.var()], toUpdate.sign());
}

inline Var getUpdatedVar(Var toUpdate, const vector< uint32_t >& mapper)
{
    return mapper[toUpdate];
}

template<typename T, typename T2>
inline void updateBySwap(T& toUpdate, T2& seen, const vector< uint32_t >& mapper)
{
    for(size_t i = 0; i < mapper.size(); i++) {
        //Already updated
        if (seen[i])
            continue;

        //Swap circularly until we reach full circle
        uint32_t var = i;
        const uint32_t origStart = var;
        while(true) {
            uint32_t swapwith = mapper[var];
            assert(seen[swapwith] == 0);
            //std::cout << "Swapping " << var << " with " << swapwith << std::endl;
            std::swap(toUpdate[var], toUpdate[swapwith]);
            seen[swapwith] = 1;
            var = swapwith;

            //Full circle
            if (mapper[var] == origStart) {
                seen[mapper[var]] = 1;
                break;
            }
        };
    }

    //clear seen
    for(size_t i = 0; i < toUpdate.size(); i++) {
        assert(seen[i] == 1);
        seen[i] = 0;
    }
}

} //end namespace

#endif //__VARUPDATE_HELPER_H__
