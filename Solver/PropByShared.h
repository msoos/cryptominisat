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

#endif //PROPBY_H
