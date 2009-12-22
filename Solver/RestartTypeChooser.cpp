/***********************************************************************************
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************************/

#include "RestartTypeChooser.h"
#include "Solver.h"

//#define VERBOSE_DEBUG

RestartTypeChooser::RestartTypeChooser(const Solver* const _s) :
    S(_s)
    , topX(100)
    , limit(30)
{
}

const RestartType RestartTypeChooser::choose()
{
    firstVarsOld = firstVars;
    calcHeap();
    uint sameIn = 0;
    if (!firstVarsOld.empty()) {
        uint thisTopX = std::min(firstVarsOld.size(), (size_t)topX);
        for (uint i = 0; i != thisTopX; i++) {
            if (std::find(firstVars.begin(), firstVars.end(), firstVarsOld[i]) != firstVars.end())
                sameIn++;
        }
        #ifdef VERBOSE_DEBUG
        std::cout << "    Same vars in first&second first 100: " << sameIn << std::endl;
        #endif
        sameIns.push_back(sameIn);
    } else
        return static_restart;
    
    #ifdef VERBOSE_DEBUG
    std::cout << "Avg same vars in first&second first 100: " << avg() << std::endl;
    #endif
    
    if (avg() > (double)limit)
        return static_restart;
    else
        return dynamic_restart;
}

const double RestartTypeChooser::avg() const
{
    double sum = 0.0;
    for (uint i = 0; i != sameIns.size(); i++)
        sum += sameIns[i];
    return (sum/(double)sameIns.size());
}

void RestartTypeChooser::calcHeap()
{
    firstVars.resize(topX);
    #ifdef VERBOSE_DEBUG
    std::cout << "First vars:" << std::endl;
    #endif
    Heap<Solver::VarOrderLt> tmp(S->order_heap);
    uint thisTopX = std::min(S->order_heap.size(), topX);
    for (uint i = 0; i != thisTopX; i++) {
        #ifdef VERBOSE_DEBUG
        std::cout << tmp.removeMin()+1 << ", ";
        #endif
        firstVars[i] = tmp.removeMin();
    }
    #ifdef VERBOSE_DEBUG
    std::cout << std::endl;
    #endif
}
