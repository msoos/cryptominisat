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

#ifndef CLAUSEALLOCATOR_H
#define CLAUSEALLOCATOR_H


#include "constants.h"
#include "cloffset.h"
#include <stdlib.h>
#include "vec.h"
#include <map>
#include <vector>

#include "watched.h"

#define BASE_DATA_TYPE uint64_t

namespace CMSat {

class Clause;
class Solver;
class PropEngine;

using std::map;
using std::vector;

/**
@brief Allocates memory for (xor) clauses

This class allocates memory in large chunks, then distributes it to clauses when
needed. When instructed, it consolidates the unused space (i.e. clauses free()-ed).
Essentially, it is a stack-like allocator for clauses. It is useful to have
this, because this way, we can address clauses according to their number,
which is 32-bit, instead of their address, which might be 64-bit
*/
class ClauseAllocator {
    public:
        ClauseAllocator();
        ~ClauseAllocator();

        template<class T> Clause* Clause_new(
            const T& ps
            , uint32_t conflictNum
            , bool recostruct = false
        );
        Clause* Clause_new(Clause& c);

        ClOffset getOffset(const Clause* ptr) const;

        /**
        @brief Returns the pointer of a clause given its offset

        Takes the "dataStart" of the correct stack, and adds the offset,
        returning the thus created pointer. Used a LOT in propagation, thus this
        is very important to be fast (therefore, it is an inlined method)
        */
        inline Clause* getPointer(const uint32_t offset) const
        {
            return (Clause*)(dataStart + offset);
        }

        void clauseFree(Clause* c); ///Frees memory and associated clause number
        void clauseFree(ClOffset offset);

        void consolidate(
            Solver* solver
            , const bool force = false
        );

        size_t memUsed() const;

    private:
        void updateAllOffsetsAndPointers(
            Solver* solver
            , const vector<ClOffset>& offsets
        );

        BASE_DATA_TYPE* dataStart; ///<Stacks start at these positions
        size_t size; ///<The number of BASE_DATA_TYPE datapieces currently used in each stack
        /**
        @brief Clauses in the stack had this size when they were allocated
        This my NOT be their current size: the clauses may be shrinked during
        the running of the solver. Therefore, it is imperative that their orignal
        size is saved. This way, we can later move clauses around.
        */
        vector<uint32_t> origClauseSizes;
        size_t maxSize; ///<The number of BASE_DATA_TYPE datapieces allocated
        /**
        @brief The estimated used size of the stack
        This is incremented by clauseSize each time a clause is allocated, and
        decremetented by clauseSize each time a clause is deallocated. The
        problem is, that clauses can shrink, and thus this value will be an
        overestimation almost all the time
        */
        size_t currentlyUsedSize;

        void* allocEnough(const uint32_t size, const bool reconstruct);
};

} //end namespace

#endif //CLAUSEALLOCATOR_H
