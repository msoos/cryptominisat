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

#ifndef PROPBY_H
#define PROPBY_H

#include "cmsat/SolverTypes.h"
#include "cmsat/Clause.h"
#include "cmsat/constants.h"
#include <stdio.h>

//#define DEBUG_PROPAGATEFROM

#include "cmsat/ClauseOffset.h"
#include "cmsat/ClauseAllocator.h"

namespace CMSat {

class PropBy
{
    private:
        uint64_t propType:2;
        //0: clause, NULL
        //1: clause, non-null
        //2: binary
        //3: tertiary
        uint64_t data1:30;
        uint64_t data2:32;

    public:
        PropBy() :
            propType(0)
            , data1(0)
            , data2(0)
        {}

        PropBy(ClauseOffset offset) :
            propType(1)
            , data2(offset)
        {
        }

        PropBy(const Lit lit) :
            propType(2)
            , data1(lit.toInt())
        {
        }

        PropBy(const Lit lit1, const Lit lit2) :
            propType(3)
            , data1(lit1.toInt())
            , data2(lit2.toInt())
        {
        }

        bool isClause() const
        {
            return ((propType&2) == 0);
        }

        bool isBinary() const
        {
            return (propType == 2);
        }

        bool isTri() const
        {
            return (propType == 3);
        }

        Lit getOtherLit() const
        {
            #ifdef DEBUG_PROPAGATEFROM
            assert(isBinary() || isTri());
            #endif
            return Lit::toLit(data1);
        }

        Lit getOtherLit2() const
        {
            #ifdef DEBUG_PROPAGATEFROM
            assert(isTri());
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

        ClauseOffset getClause()
        {
            #ifdef DEBUG_PROPAGATEFROM
            assert(isClause());
            #endif
            return data2;
        }

        bool isNULL() const
        {
            if (!isClause()) return false;
            return propType == 0;
        }
};

inline std::ostream& operator<<(std::ostream& os, const PropBy& pb)
{
    if (pb.isBinary()) {
        os << " binary, other lit= " << pb.getOtherLit();
    } else if (pb.isClause()) {
        os << " clause, num= " << pb.getClause();
    } else if (pb.isNULL()) {
        os << " NULL";
    } else if (pb.isTri()) {
        os << " tri, other 2 lits= " << pb.getOtherLit() << " , "<< pb.getOtherLit2();
    }
    return os;
}

class PropByFull
{
    private:
        uint32_t type;
        Clause* clause;
        Lit lits[3];

    public:
        PropByFull(PropBy orig, Lit otherLit, ClauseAllocator& alloc) :
            type(10)
            , clause(NULL)
        {
            if (orig.isBinary() || orig.isTri()) {
                lits[0] = otherLit;
                lits[1] = orig.getOtherLit();
                if (orig.isTri()) {
                    lits[2] = orig.getOtherLit2();
                    type = 2;
                } else {
                    type = 1;
                }
            }
            if (orig.isClause()) {
                type = 0;
                if (orig.isNULL()) {
                    clause = NULL;
                } else {
                    clause = alloc.getPointer(orig.getClause());
                }
            }
        }

        PropByFull() :
            type(10)
        {}

        PropByFull(const PropByFull& other) :
            type(other.type)
            , clause(other.clause)
        {
            memcpy(lits, other.lits, sizeof(Lit)*3);
        }

        uint32_t size() const
        {
            switch (type) {
                case 0 : return clause->size();
                case 1 : return 2;
                case 2 : return 3;
                default:
                    assert(false);
                    return 0;
            }
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
                case 0: {
                    assert(clause != NULL);
                    return (*clause)[i];
                }
                default : {
                    return lits[i];
                }
            }
        }
};

inline std::ostream& operator<<(std::ostream& cout, const PropByFull& propByFull)
{

    if (propByFull.isBinary()) {
        cout << "binary: " << " ? , " << propByFull[1];
    } else if (propByFull.isTri()) {
        cout << "tri: " << " ? , " <<propByFull[1] << " , " << propByFull[2];
    } else if (propByFull.isClause()) {
        if (propByFull.isNULL()) cout << "null clause";
        else cout << "clause:" << *propByFull.getClause();
    }
    return cout;
}

}

#endif //PROPBY_H
