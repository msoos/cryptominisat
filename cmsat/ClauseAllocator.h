/***************************************************************************************[Solver.cc]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2009, Niklas Sorensson
Copyright (c) 2009-2012, Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef CLAUSEALLOCATOR_H
#define CLAUSEALLOCATOR_H

#include "cmsat/constants.h"
#include <stdlib.h>
#include "cmsat/Vec.h"
#include <map>
#include <vector>

#include "cmsat/ClauseOffset.h"
#include "cmsat/Watched.h"

#define NUM_BITS_OUTER_OFFSET 4
#define BASE_DATA_TYPE char

namespace CMSat {

using std::map;
using std::vector;

class Clause;
class XorClause;
class Solver;

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

        template<class T>
        Clause* Clause_new(const T& ps, const bool learnt = false);
        template<class T>
        XorClause* XorClause_new(const T& ps, const bool xorEqualFalse);
        Clause* Clause_new(Clause& c);

        ClauseOffset getOffset(const Clause* ptr) const;

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

        void clauseFree(Clause* c);

        void consolidate(Solver* solver, const bool force = false) throw (std::bad_alloc);

        uint32_t getNewClauseNum();

    private:
        uint32_t getOuterOffset(const Clause* c) const;
        uint32_t getInterOffset(const Clause* c, const uint32_t outerOffset) const;
        ClauseOffset combineOuterInterOffsets(const uint32_t outerOffset, const uint32_t interOffset) const;

        void updateAllOffsetsAndPointers(Solver* solver);
        template<class T>
        void updatePointers(vec<T*>& toUpdate);
        void updatePointers(vector<Clause*>& toUpdate);
        void updatePointers(vector<XorClause*>& toUpdate);
        void updatePointers(vector<std::pair<Clause*, uint32_t> >& toUpdate);
        void updateOffsets(vec<vec<Watched> >& watches);
        void checkGoodPropBy(const Solver* solver);

        void releaseClauseNum(const uint32_t num);

        vec<BASE_DATA_TYPE*> dataStarts; ///<Stacks start at these positions
        vec<size_t> sizes; ///<The number of 32-bit datapieces currently used in each stack
        /**
        @brief Clauses in the stack had this size when they were allocated
        This my NOT be their current size: the clauses may be shrinked during
        the running of the solver. Therefore, it is imperative that their orignal
        size is saved. This way, we can later move clauses around.
        */
        vec<vec<uint32_t> > origClauseSizes;
        vec<size_t> maxSizes; ///<The number of 32-bit datapieces allocated in each stack
        /**
        @brief The estimated used size of the stack
        This is incremented by clauseSize each time a clause is allocated, and
        decremetented by clauseSize each time a clause is deallocated. The
        problem is, that clauses can shrink, and thus this value will be an
        overestimation almost all the time
        */
        vec<size_t> currentlyUsedSizes;

        void* allocEnough(const uint32_t size) throw (std::bad_alloc);

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
};

}

#endif //CLAUSEALLOCATOR_H
