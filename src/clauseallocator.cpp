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

#include <algorithm>
#include <string.h>
#include <limits>
#include <cassert>
 #include <stdlib.h>
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

void* ClauseAllocator::allocEnough(
    uint32_t clauseSize
) {
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
            std::cerr
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
            std::cerr
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
    est_sz = std::max(est_sz, (size_t)4); //we don't allow anything less than 4
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

    //Data for new struct
    vector<uint32_t> newOrigClauseSizes;
    vector<ClOffset> newOffsets;
    uint64_t newSize = 0;

    //Pointers that will be moved along
    BASE_DATA_TYPE* newDataStart = (BASE_DATA_TYPE*)malloc(currentlyUsedSize*sizeof(BASE_DATA_TYPE));
    BASE_DATA_TYPE* new_ptr = newDataStart;
    BASE_DATA_TYPE* old_ptr = dataStart;

    assert(sizeof(BASE_DATA_TYPE) % sizeof(Lit) == 0);
    for (const size_t sz: origClauseSizes) {
        Clause* clause = (Clause*)old_ptr;
        //Already freed, so skip entirely
        if (clause->freed()) {
            #ifdef VALGRIND_MAKE_MEM_DEFINED
            VALGRIND_MAKE_MEM_DEFINED(((char*)clause)+sizeof(Clause), clause->size()*sizeof(Lit));
            #endif
            old_ptr += sz;
        } else {
            //Move to new position
            size_t bytesNeeded = sizeof(Clause) + clause->size()*sizeof(Lit);
            size_t sizeNeeded = bytesNeeded/sizeof(BASE_DATA_TYPE) + (bool)(bytesNeeded % sizeof(BASE_DATA_TYPE));
            assert(sizeNeeded <= sz && "New clause size must not be bigger than orig clause size");
            memcpy(new_ptr, old_ptr, sizeNeeded*sizeof(BASE_DATA_TYPE));

            Clause* c_old = (Clause*)old_ptr;
            (*c_old)[0] = Lit::toLit(new_ptr-newDataStart);

            //Record position
            newOffsets.push_back(newSize);

            //Record sizes
            newOrigClauseSizes.push_back(sizeNeeded);
            newSize += sizeNeeded;
            assert(currentlyUsedSize >= newSize);

            //Move pointers along
            new_ptr += sizeNeeded;
            old_ptr += sz;
        }
    }

    //Update offsets & pointers(?) now, when everything is in memory still
    updateAllOffsetsAndPointers(solver, newDataStart, newOffsets);

    free(dataStart);
    dataStart = newDataStart;

    const double time_used = cpuTime() - myTime;
    if (solver->conf.verbosity >= 2) {
        cout << "c [mem] Consolidated memory ";
        cout << " cls"; print_value_kilo_mega(newOrigClauseSizes.size());
        cout << " old size"; print_value_kilo_mega(size);
        cout << " new size"; print_value_kilo_mega(newSize);
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

    //Update sizes
    size = newSize;
    maxSize = currentlyUsedSize;
    currentlyUsedSize = newSize;
    newOrigClauseSizes.swap(origClauseSizes);
}

void ClauseAllocator::check_all_cls_accessible(
    Solver* solver
    , const vector<ClOffset>& offsets
) {
    //Make sure all non-freed clauses were accessible from solver
    size_t origNumClauses = solver->longIrredCls.size();
    for(auto& lredcls: solver-> longRedCls) {
        origNumClauses += lredcls.size();
    }

    if (origNumClauses != offsets.size()) {
        std::cerr
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
}

void ClauseAllocator::updateAllOffsetsAndPointers(
    Solver* solver
    , BASE_DATA_TYPE* newDataStart
    , const vector<ClOffset>& offsets
) {
    check_all_cls_accessible(solver, offsets);

    for (size_t i = 0; i < solver->nVars(); i++) {
        VarData& vdata = solver->varData[i];
        if (vdata.reason.isClause()) {
            if (vdata.removed == Removed::none
                && solver->decisionLevel() >= vdata.level
                && solver->decisionLevel() != 0
                && solver->value(i) != l_Undef
            ) {
                Clause* old = ptr(vdata.reason.get_offset());
                uint32_t newoffset = (*old)[0].toInt();
                vdata.reason = PropBy(newoffset);
            } else {
                vdata.reason = PropBy();
            }
        }
    }

    #ifdef USE_GAUSS
    for(size_t i = 0; i < solver->gauss_matrixes.size(); i++) {
        Gaussian* g = solver->gauss_matrixes[i];
        g->assert_clauses_toclear_is_empty();
    }
    #endif

    for(auto& ws: solver->watches) {
        for(Watched& w: ws) {
            if (w.isClause()) {
                Clause* old = ptr(w.get_offset());
                uint32_t newoffset = (*old)[0].toInt();
                Lit blocked = w.getBlockedLit();
                w = Watched(newoffset, blocked);
            }
        }
    }

    //Clear clauses
    solver->longIrredCls.clear();
    for(auto& lredcls: solver->longRedCls) {
        lredcls.clear();
    }
    #ifdef USE_GAUSS
    for (Gaussian* gauss : solver->gauss_matrixes) {
        for(GaussClauseToClear& gcl: gauss->clauses_toclear) {
            ClOffset& off = gcl.offs;
            Clause* old = ptr(off);
            uint32_t newoffset = (*old)[0].toInt();
            off = newoffset;
        }
    }
    #endif //USE_GAUSS

    //Add back to the solver the correct red & irred clauses
    for(auto offset: offsets) {
        Clause* cl = (Clause*)(newDataStart + offset);
        assert(!cl->freed());

        //Put it in the right bucket
        if (!cl->gauss_temp_cl()) {
            if (cl->red()) {
                if (cl->stats.glue <= solver->conf.glue_must_keep_clause_if_below_or_eq) {
                    solver->longRedCls[0].push_back(offset);
                } else {
                    solver->longRedCls[1].push_back(offset);
                }
            } else {
                solver->longIrredCls.push_back(offset);
            }
        }
    }
}

size_t ClauseAllocator::mem_used() const
{
    uint64_t mem = 0;
    mem += maxSize*sizeof(BASE_DATA_TYPE);
    mem += origClauseSizes.capacity()*sizeof(uint32_t);

    return mem;
}
