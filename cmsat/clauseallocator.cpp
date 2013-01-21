/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2011, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
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
Clause* ClauseAllocator::Clause_new(const T& ps, const uint32_t conflictNum)
{
    assert(ps.size() > 3);
    void* mem = allocEnough(ps.size());
    Clause* real= new (mem) Clause(ps, conflictNum);

    return real;
}

template Clause* ClauseAllocator::Clause_new(const vector<Lit>& ps, uint32_t conflictNum);

/**
@brief Allocates space for a new clause & copies a give clause to it
*/
Clause* ClauseAllocator::Clause_new(Clause& c)
{
    assert(c.size() > 3);
    void* mem = allocEnough(c.size());
    memcpy(mem, &c, sizeof(Clause)+sizeof(Lit)*c.size());

    return (Clause*)mem;
}

void* ClauseAllocator::allocEnough(uint32_t clauseSize)
{
    assert(clauseSize > 3 && "Clause size cannot be 3 or less, those are stored implicitly");

    //Try to quickly find a place at the end of a dataStart
    uint32_t needed
        = (sizeof(Clause) + sizeof(Lit)*clauseSize) /sizeof(BASE_DATA_TYPE);

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
        dataStart = (BASE_DATA_TYPE*)realloc(dataStart, newMaxSize*sizeof(BASE_DATA_TYPE));
        if (!dataStart) {
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

struct sortByClauseNumLookedAtDescending
{
    bool operator () (const Clause* x, const Clause* y)
    {
        if (x->stats.numLookedAt > y->stats.numLookedAt) return 1;
        if (x->stats.numLookedAt < y->stats.numLookedAt) return 0;

        //Second tie: size. If size is smaller, go first
        return x->size() < y->size();
    }
};

/**
@brief Given the pointer of the clause it finds a 32-bit offset for it

Calculates the stack frame and the position of the pointer in the stack, and
rerturns a 32-bit value that is a concatenation of these two
*/
ClOffset ClauseAllocator::getOffset(const Clause* ptr) const
{
    return ((uint32_t*)ptr - dataStart);
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
    currentlyUsedSize -= (sizeof(Clause) + cl->size()*sizeof(Lit))/sizeof(BASE_DATA_TYPE);
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
    #ifdef DEBUG_PROPAGATEFROM
    checkGoodPropBy(solver);
    #endif

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

    //Calculate the new size needed
    uint64_t newMaxSize = (double)currentlyUsedSize*1.2 + MIN_LIST_SIZE;
    newMaxSize = std::min<uint64_t>(newMaxSize, MAXSIZE);
    BASE_DATA_TYPE* newDataStart = (BASE_DATA_TYPE*)malloc(newMaxSize*sizeof(BASE_DATA_TYPE));
    if (!newDataStart) {
        cout
        << "ERROR! While consolidating memory, malloc failed"
        << endl;

        exit(-1);
    }

    //Fill up 'clauses' with all the clauses stored and not freed
    clauses.clear();
    BASE_DATA_TYPE* currentLoc = dataStart;
    for (size_t i2 = 0; i2 < origClauseSizes.size(); i2++) {
        Clause* oldPointer = (Clause*)(currentLoc);
        if (!oldPointer->getFreed()) {
            clauses.push_back(oldPointer);
        } else {
            (*((NewPointerAndOffset*)(oldPointer))).newOffset = std::numeric_limits<uint32_t>::max();
        }
        currentLoc += origClauseSizes[i2];
    }

    //Sort clauses according to usage data
    std::sort(clauses.begin(), clauses.end(), sortByClauseNumLookedAtDescending());

    BASE_DATA_TYPE* newDataStartsAt = newDataStart;
    uint64_t newSize = 0;
    vector<uint32_t> newOrigClauseSizes;
    for (vector<Clause*>::iterator
        it = clauses.begin(), end = clauses.end()
        ; it != end
        ; it++
    ) {
        Clause* clause = *it;

        uint32_t sizeNeeded = (sizeof(Clause) + clause->size()*sizeof(Lit))/sizeof(BASE_DATA_TYPE);

        //Next line is needed, because in case of isRemoved()
        //, the size of the clause could become 0, thus having less
        // than enough space to carry the NewPointerAndOffset info
        sizeNeeded = std::max<uint32_t>(sizeNeeded, sizeof(NewPointerAndOffset)/sizeof(BASE_DATA_TYPE));
        memcpy(newDataStartsAt, (uint32_t*)clause, sizeNeeded*sizeof(BASE_DATA_TYPE));

        //Save new position of clause in the old place
        NewPointerAndOffset& ptr = *((NewPointerAndOffset*)clause);
        ptr.newOffset = newSize;
        ptr.newPointer = (Clause*)newDataStartsAt;

        newSize += sizeNeeded;
        newOrigClauseSizes.push_back(sizeNeeded);
        newDataStartsAt += sizeNeeded;
    }

    if (solver->conf.verbosity >= 3) {
        cout << "c consolidated memory. "
        << " Num cls:" << clauses.size()
        << " new size:" << newSize
        << " new maxSize:" << maxSize
        << endl;
    }

    //Update offsets & pointers(?) now, when everything is in memory still
    updateOffsets(solver->longIrredCls);
    updateOffsets(solver->longRedCls);
    updateAllOffsetsAndPointers(solver);

    //Free old piece
    free(dataStart);

    //Swap the new for the old
    dataStart = newDataStart;
    maxSize = newMaxSize;
    size = newSize;
    currentlyUsedSize = newSize;
    newOrigClauseSizes.swap(origClauseSizes);

    /*if (solver->conf.verbosity >= 3) {
        cout
        << "c Consolidated memory."
        << " old sum max size: "
        << ((double)oldSumMaxSize/(1000.0*1000.0)) << "M"
        << " old used size: "
        << ((double)oldSumSize/(1000.0*1000.0)) << "M"
        << " (" << oldNumPieces << " piece(s) )"
        << endl;

        cout
        << "c Consolidated memory."
        << " new sum max size: "
        << ((double)newSumMaxSize/(1000.0*1000.0)) << "M"
        << " new used size: "
        << ((double)newSumSize/(1000.0*1000.0)) << "M"
        << " (" << oldNumPieces << " piece(s) )"

        << " Time: " << cpuTime() - myTime
        << endl;
    }*/
}

void ClauseAllocator::checkGoodPropBy(const Solver* solver)
{
    //Go through each variable's varData and check if 'propBy' is correct
    Var var = 0;
    for (vector<VarData>::const_iterator
        it = solver->varData.begin(), end = solver->varData.end()
        ; it != end
        ; it++, var++
    ) {
        //If level is larger than current level, it's something that remained from old days
        //If level is 0, it's assigned at decision level 0, and can be ignored
        //If value is UNDEF then it's something that remained form old days
        //Remember: stuff remains from 'old days' because this is lazily updated
        if ((uint32_t)it->level > solver->decisionLevel()
            || it->level == 0
            || solver->value(var) == l_Undef
        ) {
            continue;
        }

        //If it's none of the above, then it's supposed to be actually correct
        //So check it
        if (it->reason.isClause()) {
            assert(!getPointer(it->reason.getClause())->getFreed());
            assert(!getPointer(it->reason.getClause())->getRemoved());
        }
    }
}


void ClauseAllocator::updateAllOffsetsAndPointers(PropEngine*  propEngine)
{
    updateOffsets(propEngine->watches);

    Var var = 0;
    for (vector<VarData>::iterator
        it = propEngine->varData.begin(), end = propEngine->varData.end()
        ; it != end
        ; it++, var++
    ) {
        if ((uint32_t)it->level > propEngine->decisionLevel()
            || it->level == 0
            || propEngine->value(var) == l_Undef
        ) {
            it->reason = PropBy();
            continue;
        }

        if (it->reason.isClause() && !it->reason.isNULL()) {

            //Has not been marked as invalid
            assert(
                ((NewPointerAndOffset*)(getPointer(it->reason.getClause())))
                ->newOffset !=
                std::numeric_limits<uint32_t>::max()
            );

            //Update reason
            it->reason = PropBy(
                ((NewPointerAndOffset*)(getPointer(it->reason.getClause())))
                ->newOffset
            );

        }
    }
}

/**
@brief A dumb helper function to update offsets
*/
void ClauseAllocator::updateOffsets(vector<vec<Watched> >& watches)
{
    for (uint32_t i = 0;  i < watches.size(); i++) {
        vec<Watched>& list = watches[i];
        for (vec<Watched>::iterator it = list.begin(), end = list.end(); it != end; it++) {
            if (it->isClause())
                it->setNormOffset(((NewPointerAndOffset*)(getPointer(it->getOffset())))->newOffset);
        }
    }
}

/**
@brief A dumb helper function to update offsets
*/
void ClauseAllocator::updateOffsets(vector<ClOffset>& clauses)
{
    for (uint32_t i = 0;  i < clauses.size(); i++) {
        clauses[i] = ((NewPointerAndOffset*)(getPointer(clauses[i])))->newOffset;
    }
}

/**
@brief A dumb helper function to update pointers
*/
template<class T>
void ClauseAllocator::updatePointers(vector<T*>& toUpdate)
{
    for (T **it = toUpdate.begin(), **end = toUpdate.end(); it != end; it++) {
        if (*it != NULL) {
            *it = (T*)(((NewPointerAndOffset*)(*it))->newPointer);
        }
    }
}

/**
@brief A dumb helper function to update pointers
*/
void ClauseAllocator::updatePointers(vector<pair<Clause*, uint32_t> >& toUpdate)
{
    for (vector<pair<Clause*, uint32_t> >::iterator it = toUpdate.begin(), end = toUpdate.end(); it != end; it++) {
        it->first = (((NewPointerAndOffset*)(it->first))->newPointer);
    }
}

uint64_t ClauseAllocator::getMemUsed() const
{
    return maxSize*sizeof(BASE_DATA_TYPE);
}
