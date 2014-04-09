/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.0 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/

#include "clauseallocator.h"

#include <string.h>
#include <limits>
#include "assert.h"
#include "solvertypes.h"
#include "clause.h"
#include "solver.h"
#include "searcher.h"
#include "time_mem.h"
#include "simplifier.h"
#include "completedetachreattacher.h"
#ifdef USE_VALGRIND
#include "valgrind/valgrind.h"
#include "valgrind/memcheck.h"
#endif

using namespace CMSat;

using std::pair;
using std::cout;
using std::endl;


//For mild debug info:
//#define DEBUG_CLAUSEALLOCATOR

//For listing each and every clause location:
//#define DEBUG_CLAUSEALLOCATOR2

#define MIN_LIST_SIZE (50000 * (sizeof(Clause) + 4*sizeof(Lit))/sizeof(uint32_t))
#define ALLOC_GROW_MULT 2
//We shift stuff around in Watched, so not all of 32 bits are useable.
#define EFFECTIVELY_USEABLE_BITS 30
#define MAXSIZE ((1 << (EFFECTIVELY_USEABLE_BITS))-1)

ClauseAllocator::ClauseAllocator() :
    dataStart(NULL)
    , size(0)
    , maxSize(0)
    , currentlyUsedSize(0)
{
    assert(MIN_LIST_SIZE < MAXSIZE);
}

/**
@brief Frees all stacks
*/
ClauseAllocator::~ClauseAllocator()
{
    free(dataStart);
}

/**
@brief Allocates space&initializes a clause
*/
template<class T>
Clause* ClauseAllocator::Clause_new(
    const T& ps
    , const uint32_t conflictNum
    , const bool reconstruct
)
{
    assert(reconstruct || ps.size() > 3);
    void* mem = allocEnough(ps.size(), reconstruct);
    Clause* real= new (mem) Clause(ps, conflictNum);

    return real;
}

template Clause* ClauseAllocator::Clause_new(
    const vector<Lit>& ps
    , uint32_t conflictNum
    , bool reconstruct
);

/**
@brief Allocates space for a new clause & copies a give clause to it
*/
Clause* ClauseAllocator::Clause_new(Clause& c)
{
    assert(c.size() > 3);
    void* mem = allocEnough(c.size(), false);
    memcpy(mem, &c, sizeof(Clause)+sizeof(Lit)*c.size());

    return (Clause*)mem;
}

void* ClauseAllocator::allocEnough(
    uint32_t clauseSize
    , bool reconstruct //Are we reconstructing a solution?
) {
    assert(reconstruct
        || (clauseSize > 3
            && "Clause size cannot be 3 or less, those are stored implicitly"
        )
    );

    //Try to quickly find a place at the end of a dataStart
    uint32_t neededbytes = (sizeof(Clause) + sizeof(Lit)*clauseSize);
    uint32_t needed
        = neededbytes/sizeof(BASE_DATA_TYPE) + (bool)(neededbytes % sizeof(BASE_DATA_TYPE));

    if (size + needed > maxSize) {
        //Grow by default, but don't go under or over the limits
        size_t newMaxSize = maxSize * ALLOC_GROW_MULT;
        newMaxSize = std::min<size_t>(newMaxSize, MAXSIZE);
        newMaxSize = std::max<size_t>(newMaxSize, MIN_LIST_SIZE);

        //Oops, not enough space anyway
        if (newMaxSize < size + needed) {
            cout
            << "ERROR: memory manager can't handle the load"
            << " size: " << size
            << " needed: " << needed
            << " newMaxSize: " << newMaxSize
            << endl;

            throw std::bad_alloc();
        }

        //Reallocate data
        dataStart = (BASE_DATA_TYPE*)realloc(
            dataStart
            , newMaxSize*sizeof(BASE_DATA_TYPE)
        );

        //Realloc failed?
        if (dataStart == NULL) {
            cout
            << "ERROR: while reallocating clause space"
            << endl;

            throw std::bad_alloc();
        }

        //Update maxSize to reflect the update
        maxSize = newMaxSize;
    }

    //Add clause to the set
    Clause* pointer = (Clause*)(dataStart + size);
    size += needed;
    currentlyUsedSize += needed;
    origClauseSizes.push_back(needed);

    return pointer;
}

/**
@brief Given the pointer of the clause it finds a 32-bit offset for it

Calculates the stack frame and the position of the pointer in the stack, and
rerturns a 32-bit value that is a concatenation of these two
*/
ClOffset ClauseAllocator::getOffset(const Clause* ptr) const
{
    return ((BASE_DATA_TYPE*)ptr - dataStart);
}

/**
@brief Frees a clause

If clause was binary, it frees it in quite a normal way. If it isn't, then it
needs to set the data in the Clause that it has been freed, and updates the
stack it belongs to such that the stack can now that its effectively used size
is smaller

NOTE: The size of claues can change. Therefore, currentlyUsedSizes can in fact
be incorrect, since it was incremented by the ORIGINAL size of the clause, but
when the clause is "freed", it is decremented by the POTENTIALLY SMALLER size
of the clause. Therefore, the "currentlyUsedSizes" is an overestimation!!
*/
void ClauseAllocator::clauseFree(Clause* cl)
{
    assert(!cl->getFreed());

    cl->setFreed();
    size_t bytes_freed = (sizeof(Clause) + cl->size()*sizeof(Lit));
    size_t elems_freed = bytes_freed/sizeof(BASE_DATA_TYPE) + (bool)(bytes_freed % sizeof(BASE_DATA_TYPE));
    currentlyUsedSize -= elems_freed;

    #ifdef VALGRIND_MAKE_MEM_UNDEFINED
    VALGRIND_MAKE_MEM_UNDEFINED(((char*)cl)+sizeof(Clause), cl->size()*sizeof(Lit));
    #endif
}

void ClauseAllocator::clauseFree(ClOffset offset)
{
    Clause* cl = getPointer(offset);
    clauseFree(cl);
}

/**
@brief If needed, compacts stacks, removing unused clauses

Firstly, the algorithm determines if the number of useless slots is large or
small compared to the problem size. If it is small, it does nothing. If it is
large, then it allocates new stacks, copies the non-freed clauses to these new
stacks, updates all pointers and offsets, and frees the original stacks.
*/
void ClauseAllocator::consolidate(
    Solver* solver
    , const bool force
) {
    //double myTime = cpuTime();

    //If re-allocation is not really neccessary, don't do it
    //Neccesities:
    //1) There is too much memory allocated. Re-allocation will save space
    //   Avoiding segfault (max is 16 outerOffsets, more than 10 is near)
    //2) There is too much empty, unused space (>30%)
    if (!force
        && ((double)currentlyUsedSize/(double)size > 0.7)
    ) {
        if (solver->conf.verbosity >= 3) {
            cout << "c Not consolidating memory." << endl;
        }
        return;
    }

    //Data for new struct
    vector<uint32_t> newOrigClauseSizes;
    vector<ClOffset> newOffsets;
    uint64_t newSize = 0;

    //Pointers that will be moved along
    BASE_DATA_TYPE* newDataStart = dataStart;
    BASE_DATA_TYPE* tmpDataStart = dataStart;

    assert(sizeof(Clause) % sizeof(BASE_DATA_TYPE) == 0);
    assert(sizeof(BASE_DATA_TYPE) % sizeof(Lit) == 0);
    for (const size_t sz: origClauseSizes) {
        Clause* clause = (Clause*)tmpDataStart;
        //Already freed, so skip entirely
        if (clause->freed()) {
            #ifdef VALGRIND_MAKE_MEM_DEFINED
            VALGRIND_MAKE_MEM_DEFINED(((char*)clause)+sizeof(Clause), clause->size()*sizeof(Lit));
            #endif
            tmpDataStart += sz;
            continue;
        }

        //Move to new position
        size_t bytesNeeded = sizeof(Clause) + clause->size()*sizeof(Lit);
        size_t sizeNeeded = bytesNeeded/sizeof(BASE_DATA_TYPE) + (bool)(bytesNeeded % sizeof(BASE_DATA_TYPE));
        assert(sizeNeeded <= sz && "New clause size must not be bigger than orig clause size");
        memmove(newDataStart, tmpDataStart, sizeNeeded*sizeof(BASE_DATA_TYPE));

        //Record position
        newOffsets.push_back(newSize);

        //Record sizes
        newOrigClauseSizes.push_back(sizeNeeded);
        newSize += sizeNeeded;

        //Move pointers along
        newDataStart += sizeNeeded;
        tmpDataStart += sz;
    }

    if (solver->conf.verbosity >= 3) {
        cout << "c consolidated memory. "
        << " Num cls:" << newOrigClauseSizes.size()
        << " old size:" << size
        << " new size:" << newSize
        << endl;
    }

    //Update offsets & pointers(?) now, when everything is in memory still
    updateAllOffsetsAndPointers(solver, newOffsets);

    //Update sizes
    size = newSize;
    currentlyUsedSize = newSize;
    newOrigClauseSizes.swap(origClauseSizes);
}

void ClauseAllocator::updateAllOffsetsAndPointers(
    Solver* solver
    , const vector<ClOffset>& offsets
) {
    //Must be at toplevel, otherwise propBy reset will not work
    //and also, detachReattacher will fail
    assert(solver->decisionLevel() == 0);

    //We are at decision level 0, so we can reset all PropBy-s
    for (auto& vdata: solver->varData) {
        vdata.reason = PropBy();
    }

    //Detach long clauses
    CompleteDetachReatacher detachReattach(solver);
    detachReattach.detachNonBinsNonTris();

    //Make sure all non-freed clauses were accessible from solver
    const size_t origNumClauses =
        solver->longIrredCls.size() + solver->longRedCls.size();
    if (origNumClauses != offsets.size()) {
        cout
        << "ERROR: Not all non-freed clauses are accessible from Solver"
        << endl
        << " This usually means that a clause was not freed, i.e. a mem leak"
        << endl
        << " no. clauses accessible from solver: " << origNumClauses
        << endl
        << " no. clauses non-freed: " << offsets.size()
        << endl;

        assert(origNumClauses == offsets.size());
        std::exit(-1);
    }

    //Clear clauses
    solver->longIrredCls.clear();
    solver->longRedCls.clear();

    //Add back to the solver the correct red & irred clauses
    for(auto offset: offsets) {
        Clause* cl = getPointer(offset);
        assert(!cl->getFreed());

        //Put it in the right bucket
        if (cl->red()) {
            solver->longRedCls.push_back(offset);
        } else {
            solver->longIrredCls.push_back(offset);
        }
    }

    //Finally, reattach long clauses
    detachReattach.reattachLongs();
}

size_t ClauseAllocator::memUsed() const
{
    uint64_t mem = 0;
    mem += maxSize*sizeof(BASE_DATA_TYPE);
    mem += origClauseSizes.capacity()*sizeof(uint32_t);

    return mem;
}
