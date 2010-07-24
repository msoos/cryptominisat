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

#ifndef PROPAGATEDFROM_H
#define PROPAGATEDFROM_H

#include "SolverTypes.h"
#include "Clause.h"
#ifdef _MSC_VER
#include <msvc/stdint.h>
#else
#include <stdint.h>
#endif //_MSC_VER

//#define DEBUG_PROPAGATEFROM

class PropagatedFrom
{
    private:
        union {Clause* clause; uint32_t otherLit;};
        uint32_t otherLit2;

    public:
        PropagatedFrom() :
            clause(NULL)
            , otherLit2(0)
        {}

        PropagatedFrom(Clause* c) :
            clause(c)
            , otherLit2(0)
        {
            #ifdef DEBUG_PROPAGATEFROM
            assert(c != NULL);
            #endif
        }

        PropagatedFrom(const Lit& lit) :
            otherLit(lit.toInt())
            , otherLit2(1)
        {
        }

        PropagatedFrom(const Lit& lit1, const Lit& lit2) :
            otherLit(lit1.toInt())
            , otherLit2(2 + (lit2.toInt() << 2))
        {
        }

        const bool isClause() const
        {
            return ((otherLit2&3) == 0);
        }

        const bool isBinary() const
        {
            return ((otherLit2&3) == 1);
        }

        const bool isTriClause() const
        {
            return ((otherLit2&3) == 2);
        }

        const Lit getOtherLit() const
        {
            #ifdef DEBUG_PROPAGATEFROM
            assert(isBinary() || isTriClause());
            #endif
            return Lit::toLit(otherLit);
        }

        const Lit getOtherLit2() const
        {
            #ifdef DEBUG_PROPAGATEFROM
            assert(isTriClause());
            #endif
            return Lit::toLit(otherLit2 >> 2);
        }

        const Clause* getClause() const
        {
            #ifdef DEBUG_PROPAGATEFROM
            assert(isClause());
            #endif
            return clause;
        }

        Clause* getClause()
        {
            #ifdef DEBUG_PROPAGATEFROM
            assert(isClause());
            #endif
            return clause;
        }

        const bool isNULL() const
        {
            if (!isClause()) return false;
            return clause == NULL;
        }

        const uint32_t size() const
        {
            if (isBinary()) return 2;
            if (isTriClause()) return 3;

            #ifdef DEBUG_PROPAGATEFROM
            assert(!isNULL());
            #endif

            return getClause()->size();
        }

        const Lit operator[](uint32_t i) const
        {
            if (isBinary()) {
                #ifdef DEBUG_PROPAGATEFROM
                assert(i == 1);
                #endif
                return getOtherLit();
            }

            if (isTriClause()) {
                #ifdef DEBUG_PROPAGATEFROM
                assert(i <= 2);
                #endif
                if (i == 1) return getOtherLit();
                if (i == 2) return getOtherLit2();
            }

            #ifdef DEBUG_PROPAGATEFROM
            assert(!isNULL());
            #endif
            return (*getClause())[i];
        }
};

#endif //PROPAGATEDFROM_H
