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

#ifndef PROPBY_H
#define PROPBY_H

#include "SolverTypes.h"
#include "Clause.h"
#include "constants.h"

//#define DEBUG_PROPAGATEFROM

#include "ClauseOffset.h"

enum PropByType {null_clause_t = 0, clause_t = 1, binary_t = 2, tertiary_t = 3};

class PropBy
{
    private:
        uint64_t type:2;
        //0: clause, NULL
        //1: clause, non-null
        //2: binary
        //3: tertiary
        uint64_t data1:30;
        uint64_t data2:32;

    public:
        PropBy() :
            type(null_clause_t)
            , data1(0)
            , data2(0)
        {}

        PropBy(ClauseOffset offset, const bool watchNum) :
            type(clause_t)
            , data1(watchNum)
            , data2(offset)
        {
        }

        PropBy(const Lit lit) :
            type(binary_t)
            , data1(lit.toInt())
        {
        }

        PropBy(const Lit lit1, const Lit lit2) :
            type(tertiary_t)
            , data1(lit1.toInt())
            , data2(lit2.toInt())
        {
        }

        bool isClause() const
        {
            return ((type&2) == 0);
        }

        PropByType getType() const
        {
            return (PropByType)type;
        }

        Lit getOtherLit() const
        {
            #ifdef DEBUG_PROPAGATEFROM
            assert(type == tertiary_t || type == binary_t);
            #endif
            return Lit::toLit(data1);
        }

        Lit getOtherLit2() const
        {
            #ifdef DEBUG_PROPAGATEFROM
            assert(type == tertiary_t);
            #endif
            return Lit::toLit(data2);
        }

        ClauseOffset getClause() const
        {
            #ifdef DEBUG_PROPAGATEFROM
            assert(isClause());
            #endif
            return data2;
        }

        bool isNULL() const
        {
            if (!isClause()) return false;
            return type == null_clause_t;
        }

        bool getWatchNum() const
        {
            #ifdef DEBUG_PROPAGATEFROM
            assert(isClause());
            #endif
            return data1;
        }

        bool operator==(const PropBy other) const
        {
            return (type == other.type
                    && data1 == other.data1
                    && data2 == other.data2);
        }

        bool operator!=(const PropBy other) const
        {
            return !(*this == other);
        }
};

inline std::ostream& operator<<(std::ostream& os, const PropBy& pb)
{
    switch (pb.getType()) {
        case binary_t :
            os << " binary, other lit= " << pb.getOtherLit();
            break;

        case tertiary_t :
            os << " tri, other 2 lits= " << pb.getOtherLit() << " , "<< pb.getOtherLit2();
            break;

        case clause_t :
            os << " clause, num= " << pb.getClause();
            break;

        case null_clause_t :
            os << " NULL";
            break;

        default:
            assert(false);
            break;
    }
    return os;
}

/*class PropByFull
{
    private:
        uint16_t type;
        uint16_t isize;
        Clause* clause;
        Lit lits[3];
        ClauseData data;

    public:
        PropByFull(PropBy orig
                    , Lit otherLit
                    , const ClauseAllocator& alloc
                    , const vector<ClauseData>& clauseData
                    , const vector<lbool>& assigns
        ) :
            type(10)
            , isize(0)
            , clause(NULL)
        {
            if (orig.isBinary() || orig.isTri()) {
                lits[0] = otherLit;
                lits[1] = orig.getOtherLit();
                if (orig.isTri()) {
                    lits[2] = orig.getOtherLit2();
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
                data = clauseData[clause->getNum()];
                if (orig.getWatchNum()) std::swap(data[0], data[1]);
                isize = clause->size();
                type = 0;
            }
        }

        PropByFull() :
            type(0)
            , clause(NULL)
        {}

        PropByFull(const PropByFull& other) :
            type(other.type)
            , isize(other.isize)
            , clause(other.clause)
            , data(other.data)
        {
            memcpy(lits, other.lits, sizeof(Lit)*3);
        }

        PropByFull& operator=(const PropByFull& other)
        {
            type = other.type,
            isize = other.isize;
            clause = other.clause;
            data = other.data;
            //delete xorLits;
            memcpy(lits, other.lits, sizeof(Lit)*3);
            return *this;
        }

        const uint32_t size() const
        {
            return isize;
        }

        const bool isNULL() const
        {
            return type == 0 && clause == NULL;
        }

        const bool isClause() const
        {
            return type == 0;
        }

        const bool isBinary() const
        {
            return type == 1;
        }

        const bool isTri() const
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

        const Lit operator[](const uint32_t i) const
        {
            switch (type) {
                case 0:
                    assert(clause != NULL);

                    if (i <= 1) return (*clause)[data[i]];
                    if (i == data[0]) return (*clause)[(data[1] == 0 ? 1 : 0)];
                    if (i == data[1]) return (*clause)[(data[0] == 1 ? 0 : 1)];
                    return (*clause)[i];

                default :
                    return lits[i];
            }
        }
};

inline std::ostream& operator<<(std::ostream& os, const PropByFull& propByFull)
{

    if (propByFull.isBinary()) {
        os << propByFull[0] << " " << propByFull[1];
    } else if (propByFull.isTri()) {
        os <<propByFull[0] << " " << propByFull[1] << " " << propByFull[2];
    } else if (propByFull.isClause()) {
        if (propByFull.isNULL()) os << "null clause";
        else os << *propByFull.getClause();
    }
    return os;
}*/

#endif //PROPBY_H
