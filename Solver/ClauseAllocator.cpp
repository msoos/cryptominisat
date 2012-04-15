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

#include "ClauseAllocator.h"

#include <string.h>
#include <limits>
#include "assert.h"
#include "SolverTypes.h"
#include "Clause.h"
#include "ThreadControl.h"
#include "CommandControl.h"
#include "time_mem.h"
#include "Subsumer.h"
#define BASE_DATA_TYPE char

using std::pair;
using std::cout;
using std::endl;


//For mild debug info:
//#define DEBUG_CLAUSEALLOCATOR

//For listing each and every clause location:
//#define DEBUG_CLAUSEALLOCATOR2

#define MIN_LIST_SIZE (300000 * (sizeof(Clause) + 4*sizeof(Lit)))
//#define MIN_LIST_SIZE (100 * (sizeof(Clause) + 4*sizeof(Lit)))
#define ALLOC_GROW_MULT 4
//We shift stuff around in Watched, so not all of 32 bits are useable.
#define EFFECTIVELY_USEABLE_BITS 30
#define MAXSIZE ((1 << (EFFECTIVELY_USEABLE_BITS-NUM_BITS_OUTER_OFFSET))-1)

ClauseAllocator::ClauseAllocator()
{
    assert(MIN_LIST_SIZE < MAXSIZE);

    //TODO, this is a HACK. dataStarts is queried by getPointer() which is used
    //during the run of threads, while another thread is calling Clause_new()
    //which may cause terrible problems
    dataStarts.reserve(20);
}

/**
@brief Frees all stacks
*/
ClauseAllocator::~ClauseAllocator()
{
    for (uint32_t i = 0; i < dataStarts.size(); i++) {
        free(dataStarts[i]);
    }
}

/**
@brief Allocates space&initializes a clause
*/
template<class T>
Clause* ClauseAllocator::Clause_new(const T& ps, const uint32_t conflictNum)
{
    assert(ps.size() > 2);
    void* mem = allocEnough(ps.size());
    Clause* real= new (mem) Clause(ps, conflictNum);

    return real;
}

template Clause* ClauseAllocator::Clause_new(const vector<Lit>& ps, uint32_t conflictNum);
template Clause* ClauseAllocator::Clause_new(const Clause& ps, uint32_t conflictNum);

/**
@brief Allocates space for a new clause & copies a give clause to it
*/
Clause* ClauseAllocator::Clause_new(Clause& c)
{
    assert(c.size() > 2);
    void* mem = allocEnough(c.size());
    memcpy(mem, &c, sizeof(Clause)+sizeof(Lit)*c.size());

    return (Clause*)mem;
}

/**
@brief Allocates enough space for a new clause

It tries to add the clause to the end of any already created stacks
if that is impossible, it creates a new stack, and adds the clause there
*/
void* ClauseAllocator::allocEnough(const uint32_t size)
{
    //Sanity checks
    assert(sizes.size() == dataStarts.size());
    assert(maxSizes.size() == dataStarts.size());
    assert(origClauseSizes.size() == dataStarts.size());
    if (dataStarts.size() == (1<<NUM_BITS_OUTER_OFFSET)) {
        std::cerr << "Memory manager cannot handle the load. Sorry. Exiting." << endl;
        exit(-1);
    }
    assert(size > 2 && "Clause size cannot be 2 or less, those are stored natively");

    //Try to quickly find a place at the end of a dataStart
    uint32_t needed = (sizeof(Clause)+sizeof(Lit)*size)/sizeof(BASE_DATA_TYPE);
    bool found = false;
    uint32_t which = std::numeric_limits<uint32_t>::max();
    for (uint32_t i = 0; i < sizes.size(); i++) {
        if (sizes[i] + needed < maxSizes[i]) {
            found = true;
            which = i;
            break;
        }
    }

    //OOps, no luck, try the hard way, allocating space, etc.
    if (!found) {
        //Checking whether we are out of memory, because the offset that we can
        //store is too little
        if (dataStarts.size() == (1<<NUM_BITS_OUTER_OFFSET))
            throw std::bad_alloc();

        uint32_t nextSize; //number of BYTES to allocate
        if (!maxSizes.empty()) {
            nextSize = std::min<uint32_t>(maxSizes[maxSizes.size()-1]*ALLOC_GROW_MULT, MAXSIZE);
            nextSize = std::max<uint32_t>(nextSize, (uint32_t)MIN_LIST_SIZE*2);
        } else {
            nextSize = (uint32_t)MIN_LIST_SIZE;
        }
        assert(needed <  nextSize);
        assert(nextSize <= MAXSIZE);

        #ifdef DEBUG_CLAUSEALLOCATOR
        cout << "c New list in ClauseAllocator. Size: " << nextSize
        << " (maxSize: " << MAXSIZE
        << ")" << endl;
        #endif //DEBUG_CLAUSEALLOCATOR

        char *dataStart;
        dataStart = (char *)malloc(nextSize);

        dataStarts.push_back(dataStart);
        sizes.push_back(0);
        maxSizes.push_back(nextSize);
        origClauseSizes.push_back(vector<uint32_t>());
        currentlyUsedSizes.push_back(0);
        which = dataStarts.size()-1;
    }
    #ifdef DEBUG_CLAUSEALLOCATOR2
    cout
    << "selected list = " << which
    << " size = " << sizes[which]
    << " maxsize = " << maxSizes[which]
    << " diff = " << maxSizes[which] - sizes[which] << endl;
    #endif //DEBUG_CLAUSEALLOCATOR

    assert(which != std::numeric_limits<uint32_t>::max());
    Clause* pointer = (Clause*)(dataStarts[which] + sizes[which]);
    sizes[which] += needed;
    currentlyUsedSizes[which] += needed;
    origClauseSizes[which].push_back(needed);

    return pointer;
}

/**
@brief Given the pointer of the clause it finds a 32-bit offset for it

Calculates the stack frame and the position of the pointer in the stack, and
rerturns a 32-bit value that is a concatenation of these two
*/
ClauseOffset ClauseAllocator::getOffset(const Clause* ptr) const
{
    uint32_t outerOffset = getOuterOffset(ptr);
    uint32_t interOffset = getInterOffset(ptr, outerOffset);
    return combineOuterInterOffsets(outerOffset, interOffset);
}

/**
@brief Combines the stack number and the internal offset into one 32-bit number
*/
inline ClauseOffset ClauseAllocator::combineOuterInterOffsets(const uint32_t outerOffset, const uint32_t interOffset) const
{
    return (outerOffset | (interOffset << NUM_BITS_OUTER_OFFSET));
}

/**
@brief Given a pointer, finds which stack it's in
*/
inline uint32_t ClauseAllocator::getOuterOffset(const Clause* ptr) const
{
    uint32_t which = std::numeric_limits<uint32_t>::max();
    for (uint32_t i = 0; i < sizes.size(); i++) {
        if ((char*)ptr >= dataStarts[i] && (char*)ptr < dataStarts[i] + maxSizes[i]) {
            which = i;
            break;
        }
    }
    assert(which != std::numeric_limits<uint32_t>::max());

    return which;
}

/**
@brief Given a pointer and its stack number, returns its position inside the stack
*/
inline uint32_t ClauseAllocator::getInterOffset(const Clause* ptr, uint32_t outerOffset) const
{
    return ((char*)ptr - dataStarts[outerOffset]);
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
void ClauseAllocator::clauseFree(Clause* c)
{
    assert(!c->getFreed());

    c->setFreed();
    uint32_t outerOffset = getOuterOffset(c);
    //uint32_t interOffset = getInterOffset(c, outerOffset);
    currentlyUsedSizes[outerOffset] -= (sizeof(Clause) + c->size()*sizeof(Lit))/sizeof(BASE_DATA_TYPE);
    //above should be
    //origClauseSizes[outerOffset][interOffset]
    //but it cannot be :(
}

struct ClauseSorter
{
    bool operator()(const Clause* a, const Clause* b) {
        if (a->size() <= 3 && b->size() > 3) return true;
        if (a->size() > 3 && b->size() <= 3) return false;
        return ( a->size() < b->size());
    }
};

/**
@brief If needed, compacts stacks, removing unused clauses

Firstly, the algorithm determines if the number of useless slots is large or
small compared to the problem size. If it is small, it does nothing. If it is
large, then it allocates new stacks, copies the non-freed clauses to these new
stacks, updates all pointers and offsets, and frees the original stacks.
*/
void ClauseAllocator::consolidate(
    ThreadControl* control
    , const bool force
) {
    double myTime = cpuTime();
    #ifdef DEBUG_PROPAGATEFROM
    checkGoodPropBy(control);
    #endif

    uint32_t sum = 0;
    for (uint32_t i = 0; i < sizes.size(); i++) {
        sum += currentlyUsedSizes[i];
    }
    uint32_t sumAlloc = 0;
    for (uint32_t i = 0; i < sizes.size(); i++) {
        sumAlloc += sizes[i];
    }

    #ifdef DEBUG_CLAUSEALLOCATOR
    cout << "c ratio:" << (double)sum/(double)sumAlloc << endl;
    #endif //DEBUG_CLAUSEALLOCATOR

    //If re-allocation is not really neccessary, don't do it
    //Neccesities:
    //1) There is too much memory allocated. Re-allocation will save space
    //   Avoiding segfault (max is 16 outerOffsets, more than 10 is near)
    //2) There is too much empty, unused space (>30%)
    if (!force
        && ((double)sum/(double)sumAlloc > 0.7 && sizes.size() < 10)
       ) {
        if (control->conf.verbosity >= 3) {
            cout << "c Not consolidating memory." << endl;
        }
        return;
    }

    //Calculate the new size needed
    int64_t newMaxSizeNeed = (double)sum*1.2 + MIN_LIST_SIZE;
    #ifdef DEBUG_CLAUSEALLOCATOR
    cout << "c ------ Consolidating Memory ------------" << endl;
    cout << "c newMaxSizeNeed = " << newMaxSizeNeed << endl;
    #endif //DEBUG_CLAUSEALLOCATOR

    //Stats
    size_t oldSumMaxSize = 0;
    size_t oldSumSize = 0;
    size_t newSumMaxSize = 0;
    size_t newSumSize = 0;
    size_t oldNumPieces = sizes.size();
    size_t newNumPieces = 0;

    vector<uint32_t> newMaxSizes;
    for (uint32_t i = 0; i < (1 << NUM_BITS_OUTER_OFFSET); i++) {
        if (newMaxSizeNeed <= 0)
            break;

        uint32_t thisMaxSize = std::min(newMaxSizeNeed, (int64_t)MAXSIZE);
        if (i == 0) {
            thisMaxSize = std::max<uint32_t>(thisMaxSize, MIN_LIST_SIZE);
        } else {
            assert(i > 0);
            thisMaxSize = MAXSIZE;
        }
        newMaxSizeNeed -= thisMaxSize;
        assert(thisMaxSize <= MAXSIZE);
        newMaxSizes.push_back(thisMaxSize);
        newSumMaxSize += thisMaxSize;
        newNumPieces++;

        //because the clauses don't always fit
        //it might occur that there is enough place in total
        //but the very last clause would need to be fragmented
        //over multiple lists' ends :O
        //So this "magic" constant could take care of that....
        //or maybe not (if _very_ large clauses are used, always
        //bad chance, etc. :O )
        //NOTE: the + MIN_LIST_SIZE should take care of this above at
        // newMaxSizeNeed = sum + MIN_LIST_SIZE;
        #ifdef DEBUG_CLAUSEALLOCATOR
        cout << "c NEW MaxSizes:" << newMaxSizes[i] << endl;
        #endif //DEBUG_CLAUSEALLOCATOR
    }
    #ifdef DEBUG_CLAUSEALLOCATOR
    cout << "c ------------------" << endl;
    #endif //DEBUG_CLAUSEALLOCATOR

    if (newMaxSizeNeed > 0)
        throw std::bad_alloc();

    vector<uint32_t> newSizes;
    vector<vector<uint32_t> > newOrigClauseSizes;
    vector<char*> newDataStartsPointers;
    vector<char*> newDataStarts;
    for (uint32_t i = 0; i < newMaxSizes.size(); i++) {
        newSizes.push_back(0);
        newOrigClauseSizes.push_back(vector<uint32_t>());
        char* pointer;
        pointer = (char*)malloc(newMaxSizes[i]);

        newDataStartsPointers.push_back(pointer);
        newDataStarts.push_back(pointer);
    }

    vector<Clause*> clauses;
    for (uint32_t i = 0; i < dataStarts.size(); i++) {
        uint32_t currentLoc = 0;
        for (uint32_t i2 = 0; i2 < origClauseSizes[i].size(); i2++) {
            Clause* oldPointer = (Clause*)(dataStarts[i] + currentLoc);
            if (!oldPointer->getFreed()) {
                clauses.push_back(oldPointer);
            } else {
                (*((NewPointerAndOffset*)(oldPointer))).newOffset = std::numeric_limits<uint32_t>::max();
            }
            currentLoc += origClauseSizes[i][i2];
        }
    }

    putClausesIntoDatastruct(clauses);

    uint32_t outerPart = 0;
    //uint64_t skippedNum = 0;
    for (uint32_t i = 0; i < clauses.size(); i++) {
        Clause* clause = getClause();

        uint32_t sizeNeeded = (sizeof(Clause) + clause->size()*sizeof(Lit))/sizeof(BASE_DATA_TYPE);

        //Next line is needed, because in case of isRemoved()
        //, the size of the clause could become 0, thus having less
        // than enough space to carry the NewPointerAndOffset info
        sizeNeeded = std::max<uint32_t>(sizeNeeded, (uint32_t)sizeof(NewPointerAndOffset)/sizeof(BASE_DATA_TYPE));

        if (newSizes[outerPart] + sizeNeeded > newMaxSizes[outerPart]) {
            outerPart++;
            assert(outerPart < newMaxSizes.size());
        }
        memcpy(newDataStartsPointers[outerPart], (char*)clause, sizeNeeded);

        NewPointerAndOffset& ptr = *((NewPointerAndOffset*)clause);
        ptr.newOffset = combineOuterInterOffsets(outerPart, newSizes[outerPart]);
        ptr.newPointer = (Clause*)newDataStartsPointers[outerPart];

        newSizes[outerPart] += sizeNeeded;
        newSumSize += sizeNeeded;
        newOrigClauseSizes[outerPart].push_back(sizeNeeded);
        newDataStartsPointers[outerPart] += sizeNeeded;
    }

    updatePointers(control->clauses);
    updatePointers(control->learnts);
    updateAllOffsetsAndPointers(control);

    for (uint32_t i = 0; i < dataStarts.size(); i++) {
        free(dataStarts[i]);
        oldSumMaxSize += maxSizes[i];
        oldSumSize += sizes[i];
    }

    dataStarts.clear();
    maxSizes.clear();
    sizes.clear();
    origClauseSizes.clear();
    currentlyUsedSizes.clear();
    origClauseSizes.clear();

    for (uint32_t i = 0; i < newMaxSizes.size(); i++) {
        dataStarts.push_back(newDataStarts[i]);
        maxSizes.push_back(newMaxSizes[i]);
        sizes.push_back(newSizes[i]);
        currentlyUsedSizes.push_back(newSizes[i]);
    }
    newOrigClauseSizes.swap(origClauseSizes);

    if (control->conf.verbosity >= 3) {
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
    }
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

void ClauseAllocator::putClausesIntoDatastruct(std::vector<Clause*>& clauses)
{
    //Setup otherclauses and threelongclauses
    otherClauses.clear();
    threeLongClauses.clear();

    //Observe that 3-long clauses are never resolved during propagation
    //and conflict generation. So we put them at the beginning
    //and then they don't bother anyone.
    //
    //The rest of the clauses are then sorted according to how many times
    //they were visited

    for (uint32_t i = 0; i < clauses.size(); i++) {
        Clause* c = clauses[i];
        if (c->size() <= 3) {
            threeLongClauses.push_back(c);
            continue;
        }
        otherClauses.push_back(c);
    }

    std::sort(otherClauses.begin(), otherClauses.end(), sortByClauseNumLookedAtDescending());
}

Clause* ClauseAllocator::getClause()
{
    if (!threeLongClauses.empty()) {
        Clause* tmp = threeLongClauses.back();
        threeLongClauses.pop_back();
        return tmp;
    }

    assert(!otherClauses.empty());
    Clause* tmp = otherClauses.back();
    otherClauses.pop_back();
    return tmp;
}

void ClauseAllocator::checkGoodPropBy(const ThreadControl* control)
{
    //Go through each variable's varData and check if 'propBy' is correct
    Var var = 0;
    for (vector<VarData>::const_iterator
        it = control->varData.begin(), end = control->varData.end()
        ; it != end
        ; it++, var++
    ) {
        //If level is larger than current level, it's something that remained from old days
        //If level is 0, it's assigned at decision level 0, and can be ignored
        //If value is UNDEF then it's something that remained form old days
        //Remember: stuff remains from 'old days' because this is lazily updated
        if ((uint32_t)it->level > control->decisionLevel()
            || it->level == 0
            || control->value(var) == l_Undef
        ) {
            continue;
        }

        //If it's none of the above, then it's supposed to be actually correct
        //So check it
        if (it->reason.isClause() && !it->reason.isNULL()) {
            assert(!getPointer(it->reason.getClause())->getFreed());
            assert(!getPointer(it->reason.getClause())->getRemoved());
        }
    }
}


template<class T>
void ClauseAllocator::updateAllOffsetsAndPointers(T* solver)
{
    updateOffsets(solver->watches);

    Var var = 0;
    for (vector<VarData>::iterator it = solver->varData.begin(), end = solver->varData.end(); it != end; it++, var++) {
        if ((uint32_t)it->level > solver->decisionLevel()
            || it->level == 0
            || solver->value(var) == l_Undef
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
template void ClauseAllocator::updateAllOffsetsAndPointers(Solver* solver);
template void ClauseAllocator::updateAllOffsetsAndPointers(CommandControl* solver);

/**
@brief A dumb helper function to update offsets
*/
void ClauseAllocator::updateOffsets(vector<vec<Watched> >& watches)
{
    for (uint32_t i = 0;  i < watches.size(); i++) {
        vec<Watched>& list = watches[i];
        for (vec<Watched>::iterator it = list.begin(), end = list.end(); it != end; it++) {
            if (it->isClause())
                it->setNormOffset(((NewPointerAndOffset*)(getPointer(it->getNormOffset())))->newOffset);
        }
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
void ClauseAllocator::updatePointers(vector<Clause*>& toUpdate)
{
    for (vector<Clause*>::iterator it = toUpdate.begin(), end = toUpdate.end(); it != end; it++) {
        *it = (((NewPointerAndOffset*)(*it))->newPointer);
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
