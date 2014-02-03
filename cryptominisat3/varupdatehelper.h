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
    T backup = toUpdate;
    for(size_t i = 0; i < toUpdate.size(); i++) {
        toUpdate.at(i) = backup.at(mapper.at(i));
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
    //assert(toUpdate.size() == mapper.size());
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
    return Lit(getUpdatedVar(toUpdate.var(), mapper), toUpdate.sign());
}

inline Var getUpdatedVar(Var toUpdate, const vector< uint32_t >& mapper)
{
    return mapper.at(toUpdate);
}

template<typename T, typename T2>
inline void updateBySwap(T& toUpdate, T2& seen, const vector< uint32_t >& mapper)
{
    for(size_t i = 0; i < toUpdate.size(); i++) {
        if (seen.at(i)) {
            //Already updated, skip
            continue;
        }

        //Swap circularly until we reach full circle
        uint32_t var = i;
        const uint32_t origStart = var;
        while(true) {
            uint32_t swapwith = mapper.at(var);
            assert(seen.at(swapwith) == 0);
            //std::cout << "Swapping " << var << " with " << swapwith << std::endl;
            using std::swap;
            swap(toUpdate.at(var), toUpdate.at(swapwith));
            seen.at(swapwith) = 1;
            var = swapwith;

            //Full circle
            if (mapper.at(var) == origStart) {
                seen.at(mapper.at(var)) = 1;
                break;
            }
        };
    }

    //clear seen
    for(size_t i = 0; i < toUpdate.size(); i++) {
        assert(seen.at(i) == 1);
        seen.at(i) = 0;
    }
}

} //end namespace

#endif //__VARUPDATE_HELPER_H__
