/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
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

#include <stdlib.h>
#include <algorithm>
#include <string.h>
#include <limits>
#include <cassert>
#include "solvertypes.h"
#include "clause.h"
#include "solver.h"
#include "searcher.h"
#include "time_mem.h"
#include "sqlstats.h"
#ifdef USE_GAUSS
#include "gaussian.h"
#endif

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
#define ALLOC_GROW_MULT 1.5
//We shift stuff around in Watched, so not all of 32 bits are useable.
#define EFFECTIVELY_USEABLE_BITS 30
#define MAXSIZE ((1 << (EFFECTIVELY_USEABLE_BITS))-1)

ClauseAllocator::ClauseAllocator() :
    dataStart(NULL)
    , size(0)
    , capacity(0)
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

void* ClauseAllocator::allocEnough(
    uint32_t clauseSize
) {
    //Try to quickly find a place at the end of a dataStart
    uint32_t neededbytes = (sizeof(Clause) + sizeof(Lit)*clauseSize);
    uint32_t needed
        = neededbytes/sizeof(BASE_DATA_TYPE) + (bool)(neededbytes % sizeof(BASE_DATA_TYPE));

    if (size + needed > capacity) {
        //Grow by default, but don't go under or over the limits
        size_t newcapacity = capacity * ALLOC_GROW_MULT;
        newcapacity = std::min<size_t>(newcapacity, MAXSIZE);
        newcapacity = std::max<size_t>(newcapacity, MIN_LIST_SIZE);

        //Oops, not enough space anyway
        if (newcapacity < size + needed) {
            std::cerr
            << "ERROR: memory manager can't handle the load"
            << " size: " << size
            << " needed: " << needed
            << " newcapacity: " << newcapacity
            << endl;

            throw std::bad_alloc();
        }

        //Reallocate data
        dataStart = (BASE_DATA_TYPE*)realloc(
            dataStart
            , newcapacity*sizeof(BASE_DATA_TYPE)
        );

        //Realloc failed?
        if (dataStart == NULL) {
            std::cerr
            << "ERROR: while reallocating clause space"
            << endl;

            throw std::bad_alloc();
        }

        //Update capacity to reflect the update
        capacity = newcapacity;
    }

    //Add clause to the set
    Clause* pointer = (Clause*)(dataStart + size);
    size += needed;
    currentlyUsedSize += needed;

    return pointer;
}

/**
@brief Given the pointer of the clause it finds a 32-bit offset for it

Calculates the stack frame and the position of the pointer in the stack, and
rerturns a 32-bit value that is a concatenation of these two
*/
ClOffset ClauseAllocator::get_offset(const Clause* ptr) const
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
    assert(!cl->freed());

    cl->setFreed();
    size_t est_sz = cl->size();
    est_sz = std::max(est_sz, (size_t)3);
    size_t bytes_freed = (sizeof(Clause) + est_sz*sizeof(Lit));
    size_t elems_freed = bytes_freed/sizeof(BASE_DATA_TYPE) + (bool)(bytes_freed % sizeof(BASE_DATA_TYPE));
    currentlyUsedSize -= elems_freed;

    #ifdef VALGRIND_MAKE_MEM_UNDEFINED
    VALGRIND_MAKE_MEM_UNDEFINED(((char*)cl)+sizeof(Clause), cl->size()*sizeof(Lit));
    #endif
}

void ClauseAllocator::clauseFree(ClOffset offset)
{
    Clause* cl = ptr(offset);
    clauseFree(cl);
}

uint32_t ClauseAllocator::move_cl(
    uint32_t* newDataStart
    , uint32_t*& new_ptr
    , Clause* old
) const {
    size_t bytesNeeded = sizeof(Clause) + old->size()*sizeof(Lit);
    size_t sizeNeeded = bytesNeeded/sizeof(BASE_DATA_TYPE) + (bool)(bytesNeeded % sizeof(BASE_DATA_TYPE));
    memcpy(new_ptr, old, sizeNeeded*sizeof(BASE_DATA_TYPE));

    uint32_t new_offset = new_ptr-newDataStart;
    (*old)[0] = Lit::toLit(new_offset);
    old->reloced = true;

    new_ptr += sizeNeeded;
    return new_offset;
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
    //If re-allocation is not really neccessary, don't do it
    //Neccesities:
    //1) There is too much memory allocated. Re-allocation will save space
    //   Avoiding segfault (max is 16 outerOffsets, more than 10 is near)
    //2) There is too much empty, unused space (>30%)
    if (!force
        && (float_div(currentlyUsedSize, size) > 0.8 || currentlyUsedSize < (100ULL*1000ULL))
    ) {
        if (solver->conf.verbosity >= 3) {
            cout << "c Not consolidating memory." << endl;
        }
        return;
    }
    const double myTime = cpuTime();

    //Pointers that will be moved along
    BASE_DATA_TYPE * const newDataStart = (BASE_DATA_TYPE*)malloc(currentlyUsedSize*sizeof(BASE_DATA_TYPE));
    BASE_DATA_TYPE * new_ptr = newDataStart;

    assert(sizeof(BASE_DATA_TYPE) % sizeof(Lit) == 0);
    for(auto& ws: solver->watches) {
        for(Watched& w: ws) {
            if (w.isClause()) {
                Clause* old = ptr(w.get_offset());
                assert(!old->freed());
                Lit blocked = w.getBlockedLit();
                if (old->reloced) {
                    Lit newoffset = (*old)[0];
                    w = Watched(newoffset.toInt(), blocked);
                } else {
                    uint32_t new_offset = move_cl(newDataStart, new_ptr, old);
                    w = Watched(new_offset, blocked);
                }
            }
        }
    }

    #ifdef USE_GAUSS
    for (Gaussian* gauss : solver->gauss_matrixes) {
        for(GaussClauseToClear& gcl: gauss->clauses_toclear) {
            ClOffset& off = gcl.offs;
            Clause* old = ptr(off);
            if (old->reloced) {
                uint32_t new_offset = (*old)[0].toInt();
                off = new_offset;
            } else {
                uint32_t new_offset = move_cl(newDataStart, new_ptr, old);
                off = new_offset;
            }
            assert(!old->freed());
        }
    }
    #endif //USE_GAUSS

    update_offsets(solver->longIrredCls);
    for(auto& lredcls: solver->longRedCls) {
        update_offsets(lredcls);
    }

    //Fix up propBy
    for (size_t i = 0; i < solver->nVars(); i++) {
        VarData& vdata = solver->varData[i];
        if (vdata.reason.isClause()) {
            if (vdata.removed == Removed::none
                && solver->decisionLevel() >= vdata.level
                && vdata.level != 0
                && solver->value(i) != l_Undef
            ) {
                Clause* old = ptr(vdata.reason.get_offset());
                assert(!old->freed());
                uint32_t new_offset = (*old)[0].toInt();
                vdata.reason = PropBy(new_offset);
            } else {
                vdata.reason = PropBy();
            }
        }
    }

    //Update sizes
    const uint32_t old_size = size;
    size = new_ptr-newDataStart;
    capacity = currentlyUsedSize;
    currentlyUsedSize = size;
    free(dataStart);
    dataStart = newDataStart;

    const double time_used = cpuTime() - myTime;
    if (solver->conf.verbosity >= 2) {
        cout << "c [mem] Consolidated memory ";
        cout << " old size"; print_value_kilo_mega(old_size);
        cout << " new size"; print_value_kilo_mega(size);
        cout << solver->conf.print_times(time_used)
        << endl;
    }
    if (solver->sqlStats) {
        solver->sqlStats->time_passed_min(
            solver
            , "consolidate"
            , time_used
        );
    }
}

void ClauseAllocator::update_offsets(
    vector<ClOffset>& offsets
) {

    for(ClOffset& offs: offsets) {
        Clause* cl = ptr(offs);
        assert(cl->reloced);
        offs = (*cl)[0].toInt();
    }
}

size_t ClauseAllocator::mem_used() const
{
    uint64_t mem = 0;
    mem += capacity*sizeof(BASE_DATA_TYPE);

    return mem;
}
