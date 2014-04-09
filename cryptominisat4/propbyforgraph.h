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

#include "solvertypes.h"
#include "clause.h"
#include "propby.h"
#include "clauseallocator.h"

#ifndef __PROPBYFORGRAPH_H__
#define __PROPBYFORGRAPH_H__

namespace CMSat {

class PropByForGraph
{
    private:
        uint16_t type;
        uint32_t isize;
        Clause* clause;
        Lit lits[3];

    public:
        PropByForGraph(PropBy orig
                    , Lit otherLit
                    , const ClauseAllocator& alloc
        ) :
            type(10)
            , isize(0)
            , clause(NULL)
        {
            if (orig.getType() == binary_t || orig.getType() == tertiary_t) {
                lits[0] = otherLit;
                lits[1] = orig.lit2();
                if (orig.getType() == tertiary_t) {
                    lits[2] = orig.lit3();
                    type = 2;
                    isize = 3;
                } else {
                    type = 1;
                    isize = 2;
                }
            }
            if (orig.isClause()) {
                if (orig.isNULL()) {
                    type = 0;
                    isize = 0;
                    clause = NULL;
                    return;
                }
                clause = alloc.getPointer(orig.getClause());
                isize = clause->size();
                type = 0;
            }
        }

        PropByForGraph() :
            type(0)
            , clause(NULL)
        {}

        PropByForGraph(const PropByForGraph& other) :
            type(other.type)
            , isize(other.isize)
            , clause(other.clause)
        {
            memcpy(lits, other.lits, sizeof(Lit)*3);
        }

        PropByForGraph& operator=(const PropByForGraph& other)
        {
            type = other.type,
            isize = other.isize;
            clause = other.clause;
            //delete xorLits;
            memcpy(lits, other.lits, sizeof(Lit)*3);
            return *this;
        }

        uint32_t size() const
        {
            return isize;
        }

        bool isNULL() const
        {
            return type == 0 && clause == NULL;
        }

        bool isClause() const
        {
            return type == 0;
        }

        bool isBinary() const
        {
            return type == 1;
        }

        bool isTri() const
        {
            return type == 2;
        }

        const Clause* getClause() const
        {
            return clause;
        }

        Clause* getClause()
        {
            return clause;
        }

        Lit operator[](const uint32_t i) const
        {
            switch (type) {
                case 0:
                    assert(clause != NULL);
                    return (*clause)[i];

                default :
                    return lits[i];
            }
        }
};

inline std::ostream& operator<<(
    std::ostream& os
    , const PropByForGraph& propByFull
) {

    if (propByFull.isBinary()) {
        os << propByFull[0] << " " << propByFull[1];
    } else if (propByFull.isTri()) {
        os <<propByFull[0] << " " << propByFull[1] << " " << propByFull[2];
    } else if (propByFull.isClause()) {
        if (propByFull.isNULL()) os << "null clause";
        else os << *propByFull.getClause();
    }
    return os;
}

} //end namespace

#endif //__PROPBYFORGRAPH_H__
