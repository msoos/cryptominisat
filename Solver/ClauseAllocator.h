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

#ifndef CLAUSEALLOCATOR_H
#define CLAUSEALLOCATOR_H


#include "constants.h"
#include "ClauseOffset.h"
#include <stdlib.h>
#include "Vec.h"
#include <map>
#include <vector>

#include "Watched.h"

#define NUM_BITS_OUTER_OFFSET 4

class Clause;
class ThreadControl;

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

        template<class T> Clause* Clause_new(const T& ps);
        Clause* Clause_new(Clause& c);

        const ClauseOffset getOffset(const Clause* ptr) const;

        /**
        @brief Returns the pointer of a clause given its offset

        Takes the "dataStart" of the correct stack, and adds the offset,
        returning the thus created pointer. Used a LOT in propagation, thus this
        is very important to be fast (therefore, it is an inlined method)
        */
        inline Clause* getPointer(const uint32_t offset) const
        {
            return (Clause*)(dataStarts[offset&((1 << NUM_BITS_OUTER_OFFSET) - 1)]
                            +(offset >> NUM_BITS_OUTER_OFFSET));
        }

        void clauseFree(Clause* c); ///Frees memory and associated clause number
        void releaseClauseNum(Clause* cl); ///<used when long clause becomes 3-long

        void consolidate(ThreadControl* control, const bool force = false);

    private:
        uint32_t getOuterOffset(const Clause* c) const;
        uint32_t getInterOffset(const Clause* c, const uint32_t outerOffset) const;
        const ClauseOffset combineOuterInterOffsets(const uint32_t outerOffset, const uint32_t interOffset) const;

        void updateAllOffsetsAndPointers(ThreadControl* control);
        template<class T>
        void updatePointers(vector<T*>& toUpdate);
        void updatePointers(vector<Clause*>& toUpdate);
        void updatePointers(vector<std::pair<Clause*, uint32_t> >& toUpdate);
        void updateOffsets(vector<vec<Watched> >& watches);

        void checkGoodPropBy(const ThreadControl* control);

        vector<char*> dataStarts; ///<Stacks start at these positions
        vector<size_t> sizes; ///<The number of 32-bit datapieces currently used in each stack
        /**
        @brief Clauses in the stack had this size when they were allocated
        This my NOT be their current size: the clauses may be shrinked during
        the running of the solver. Therefore, it is imperative that their orignal
        size is saved. This way, we can later move clauses around.
        */
        vector<vector<uint32_t> > origClauseSizes;
        vector<size_t> maxSizes; ///<The number of 32-bit datapieces allocated in each stack
        /**
        @brief The estimated used size of the stack
        This is incremented by clauseSize each time a clause is allocated, and
        decremetented by clauseSize each time a clause is deallocated. The
        problem is, that clauses can shrink, and thus this value will be an
        overestimation almost all the time
        */
        vector<size_t> currentlyUsedSizes;

        void* allocEnough(const uint32_t size);

        /**
        @brief The clause's data is replaced by this to aid updating

        We need to update the pointer or offset that points to the clause
        The best way to do that is to simply fill the original place of the clause
        with the pointer/offset of the new location.
        */
        struct NewPointerAndOffset
        {
            uint32_t newOffset; ///<The new offset where the clause now resides
            Clause* newPointer; ///<The new place
        };

        vector<Clause*> otherClauses;
        vector<Clause*> threeLongClauses;
        Clause* getClause();
        void putClausesIntoDatastruct(std::vector<Clause*>& clauses);

        const uint32_t getNewClauseNum(const uint32_t size);
        void renumberClauses(vector<Clause*>& clauses, ThreadControl* solver);
        vector<uint32_t> freedNums;  //Free clause nums that can be used now
        uint32_t maxClauseNum;
};

#endif //CLAUSEALLOCATOR_H
