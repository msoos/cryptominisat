#ifndef CSET_H
#define CSET_H

#include "Vec.h"

class Clause;

template <class T>
uint32_t calcAbstraction(const T& ps) {
    uint32_t abstraction = 0;
    for (uint32_t i = 0; i != ps.size(); i++)
        abstraction |= 1 << (ps[i].toInt() & 31);
    return abstraction;
}

#pragma pack(push)
#pragma pack(1)
class ClauseSimp
{
    public:
        ClauseSimp(Clause* c, const uint32_t _index) :
        clause(c)
        , index(_index)
        {
            abst = calcAbstraction(*c);
        }
        
        Clause* clause;
        uint32_t abst;
        uint32_t index;
};
#pragma pack(pop)

class CSet {
    vec<uint>       where;  // Map clause ID to position in 'which'.
    vec<ClauseSimp> which;  // List of clauses (for fast iteration). May contain 'Clause_NULL'.
    vec<uint>       free;   // List of positions holding 'Clause_NULL'.
    
    public:
        //ClauseSimp& operator [] (uint32_t index) { return which[index]; }
        int size(void) const { return which.size(); }
        int nElems(void) const { return which.size() - free.size(); }
        
        bool add(ClauseSimp& c) {
            assert(c.clause != NULL);
            where.growTo(c.index+1, -1);
            if (where[c.index] != -1) {
                //already in, only update
                which[where[c.index]].abst = c.abst;
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
        
        bool exclude(ClauseSimp& c) {
            assert(c.clause != NULL);
            if (c.index >= where.size() || where[c.index] == -1) {
                //not inside
                return false;
            }
            free.push(where[c.index]);
            which[where[c.index]].clause = NULL;
            where[c.index] = -1;
            return true;
        }
        
        void clear(void) {
            for (int i = 0; i < which.size(); i++)  {
                if (which[i].clause != NULL) {
                    where[which[i].index] = -1;
                }
            }
            which.clear();
            free.clear();
        }
        
        void update(ClauseSimp& c) {
            if (c.index >= where.size() || where[c.index] == -1)
                return;
            which[where[c.index]].abst = c.abst;
        }
        
        class iterator
        {
            public:
                iterator(ClauseSimp* _it) :
                it(_it)
                {}
                
                void operator++()
                {
                    it++;
                }
                
                const bool operator!=(const iterator& iter) const
                {
                    return (it != iter.it);;
                }
                
                ClauseSimp& operator*() {
                    return *it;
                }
                
                ClauseSimp*& operator->() {
                    return it;
                }
            private:
                ClauseSimp* it;
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

#endif //CSET_H