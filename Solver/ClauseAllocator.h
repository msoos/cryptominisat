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

#ifndef CLAUSEALLOCATOR_H
#define CLAUSEALLOCATOR_H

#ifdef _MSC_VER
#include <msvc/stdint.h>
#else
#include <stdint.h>
#endif //_MSC_VER

#include "Vec.h"
#include <boost/pool/pool.hpp>

class Clause;
class XorClause;
class Solver;

typedef uint32_t ClauseOffset;

class ClauseAllocator {
    public:
        ClauseAllocator();
        
        template<class T>
        Clause* Clause_new(const T& ps, const uint group, const bool learnt = false);
        template<class T>
        XorClause* XorClause_new(const T& ps, const bool inverted, const uint group);
        Clause* Clause_new(Clause& c);

        ClauseOffset getOffset(const Clause* ptr);

        inline Clause* getPointer(const uint32_t offset)
        {
            return (Clause*)(dataStarts[offset&255]+(offset>>8));
        }

        void clauseFree(Clause* c);

        void consolidate(Solver* solver);

    private:
        vec<uint32_t*> dataStarts;
        vec<size_t> sizes;
        vec<vec<uint32_t> > origClauseSizes;
        vec<size_t> maxSizes;
        vec<uint32_t> origSizes;

        boost::pool<> clausePoolBin;

        void* allocEnough(const uint32_t size);
};

#endif //CLAUSEALLOCATOR_H
