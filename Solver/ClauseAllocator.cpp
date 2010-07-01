/***********************************************************************
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
************************************************************************/

#include "ClauseAllocator.h"

#include <string.h>
#include <limits>
#include "assert.h"
#include "SolverTypes.h"
#include "Clause.h"
#include "Solver.h"

ClauseAllocator::ClauseAllocator() :
    clausePoolBin(sizeof(Clause) + 2*sizeof(Lit))
{}

template<class T>
Clause* ClauseAllocator::Clause_new(const T& ps, const unsigned int group, const bool learnt)
{
    void* mem = allocEnough(ps.size());
    Clause* real= new (mem) Clause(ps, group, learnt);

    return real;
}
template Clause* ClauseAllocator::Clause_new(const vec<Lit>& ps, const unsigned int group, const bool learnt);
template Clause* ClauseAllocator::Clause_new(const Clause& ps, const unsigned int group, const bool learnt);
template Clause* ClauseAllocator::Clause_new(const XorClause& ps, const unsigned int group, const bool learnt);

template<class T>
XorClause* ClauseAllocator::XorClause_new(const T& ps, const bool inverted, const unsigned int group)
{
    void* mem = allocEnough(ps.size());
    XorClause* real= new (mem) XorClause(ps, inverted, group);

    return real;
}
template XorClause* ClauseAllocator::XorClause_new(const vec<Lit>& ps, const bool inverted, const unsigned int group);
template XorClause* ClauseAllocator::XorClause_new(const XorClause& ps, const bool inverted, const unsigned int group);

Clause* ClauseAllocator::Clause_new(Clause& c)
{
    void* mem = allocEnough(c.size());
    //Clause* real= new (mem) Clause(ps, group, learnt);
    memcpy(mem, &c, sizeof(Clause)+sizeof(Lit)*c.size());
    Clause& c2 = *(Clause*)mem;
    
    return &c2;
}

void* ClauseAllocator::allocEnough(const uint32_t size)
{
    assert(sizes.size() == dataStarts.size());
    assert(maxSizes.size() == dataStarts.size());
    assert(origClauseSizes.size() == dataStarts.size());

    assert(sizeof(Clause)%sizeof(uint32_t) == 0);
    assert(sizeof(Lit)%sizeof(uint32_t) == 0);

    if (size == 2) {
        return clausePoolBin.malloc();
    }
    
    uint32_t needed = sizeof(Clause)+sizeof(Lit)*size;
    bool found = false;
    uint32_t which = std::numeric_limits<uint32_t>::max();
    for (uint32_t i = 0; i < sizes.size(); i++) {
        if (sizes[i] + needed < maxSizes[i]) {
            found = true;
            which = i;
            break;
        }
    }

    if (!found) {
        std::cout << "c New list in ClauseAllocator" << std::endl;
        uint32_t nextSize;
        if (maxSizes.size() != 0)
            nextSize = maxSizes[maxSizes.size()-1]*3;
        else
            nextSize = 300000 * (sizeof(Clause) + 4*sizeof(Lit));
        assert(needed <  nextSize);
        
        uint32_t *dataStart = (uint32_t*)malloc(nextSize);
        assert(dataStart != NULL);
        dataStarts.push(dataStart);
        sizes.push(0);
        maxSizes.push(nextSize/sizeof(uint32_t));
        origClauseSizes.push();
        which = dataStarts.size()-1;
    }
    /*std::cout
    << "selected list = " << which
    << " size = " << sizes[which]
    << " maxsize = " << maxSizes[which]
    << " diff = " << maxSizes[which] - sizes[which] << std::endl;*/

    assert(which != std::numeric_limits<uint32_t>::max());
    Clause* pointer = (Clause*)(dataStarts[which] + sizes[which]);
    sizes[which] += needed/sizeof(uint32_t);
    origClauseSizes[which].push(size);

    return pointer;
}

ClauseOffset ClauseAllocator::getOffset(const Clause* ptr)
{
    uint32_t which = std::numeric_limits<uint32_t>::max();
    for (uint32_t i = 0; i < sizes.size(); i++) {
        if ((uint32_t*)ptr >= dataStarts[i] && (uint32_t*)ptr < dataStarts[i] + maxSizes[i])
            which = i;
    }
    assert(which != std::numeric_limits<uint32_t>::max());
    uint32_t outerOffset = which;
    uint32_t interOffset = ((uint32_t*)ptr-dataStarts[which]);
    return (outerOffset | (interOffset<<8));
}

void ClauseAllocator::clauseFree(Clause* c)
{
    if (c->wasBin()) {
        clausePoolBin.free(c);
    } else {
        c->setFreed();
    }
}

void ClauseAllocator::consolidate(Solver* solver)
{
    dasfdasf
}

