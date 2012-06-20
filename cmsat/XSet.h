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

#ifndef XSET_H
#define XSET_H

#include "cmsat/Vec.h"
#include <limits>
#include "cmsat/constants.h"

namespace CMSat {

class XorClause;

template <class T>
uint32_t calcXorAbstraction(const T& ps) {
    uint32_t abstraction = 0;
    for (uint32_t i = 0; i != ps.size(); i++)
        abstraction |= 1 << (ps[i].var() & 31);
    return abstraction;
}

//#pragma pack(push)
//#pragma pack(1)
class XorClauseSimp
{
    public:
        XorClauseSimp(XorClause* c, const uint32_t _index) :
        clause(c)
        , index(_index)
        {}

        XorClause* clause;
        uint32_t index;
};
//#pragma pack(pop)

class XSet {
    vec<uint32_t>       where;  // Map clause ID to position in 'which'.
    vec<XorClauseSimp> which;  // List of clauses (for fast iteration). May contain 'Clause_NULL'.
    vec<uint32_t>       free;   // List of positions holding 'Clause_NULL'.

    public:
        //XorClauseSimp& operator [] (uint32_t index) { return which[index]; }
        void reserve(uint32_t size) { where.reserve(size);}
        uint32_t size(void) const { return which.size(); }
        uint32_t nElems(void) const { return which.size() - free.size(); }

        bool add(const XorClauseSimp& c) {
            assert(c.clause != NULL);
            where.growTo(c.index+1, std::numeric_limits<uint32_t>::max());
            if (where[c.index] != std::numeric_limits<uint32_t>::max()) {
                return true;
            }
            if (free.size() > 0){
                where[c.index] = free.last();
                which[free.last()] = c;
                free.pop();
            }else{
                where[c.index] = which.size();
                which.push(c);
            }
            return false;
        }

        bool exclude(const XorClauseSimp& c) {
            assert(c.clause != NULL);
            if (c.index >= where.size() || where[c.index] == std::numeric_limits<uint32_t>::max()) {
                //not inside
                return false;
            }
            free.push(where[c.index]);
            which[where[c.index]].clause = NULL;
            where[c.index] = std::numeric_limits<uint32_t>::max();
            return true;
        }

        void clear(void) {
            for (uint32_t i = 0; i < which.size(); i++)  {
                if (which[i].clause != NULL) {
                    where[which[i].index] = std::numeric_limits<uint32_t>::max();
                }
            }
            which.clear();
            free.clear();
        }

        class iterator
        {
            public:
                iterator(XorClauseSimp* _it) :
                it(_it)
                {}

                void operator++()
                {
                    it++;
                }

                bool operator!=(const iterator& iter) const
                {
                    return (it != iter.it);;
                }

                XorClauseSimp& operator*() {
                    return *it;
                }

                XorClauseSimp*& operator->() {
                    return it;
                }
            private:
                XorClauseSimp* it;
        };

        iterator begin()
        {
            return iterator(which.getData());
        }

        iterator end()
        {
            return iterator(which.getData() + which.size());
        }
};

#endif //XSET_H

}
