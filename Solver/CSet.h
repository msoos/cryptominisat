/**************************************************************************************************
From: Solver.C -- (C) Niklas Een, Niklas Sorensson, 2004
**************************************************************************************************/

#ifndef CSET_H
#define CSET_H

#include "Vec.h"
#include <limits>
#include "constants.h"

#include "Clause.h"
using std::vector;

/**
@brief A class to hold a clause and a related index

This class is used in Subsumer. Basically, the index could be added to the
Clause class, but it would take space, and that would slow down the solving.

NOTE: On 64-bit systems, this datastructure needs 128 bits :O
*/
class ClauseIndex
{
public:
    ClauseIndex() :
        index(std::numeric_limits< uint32_t >::max())
        {};
    ClauseIndex(const uint32_t _index) :
        index(_index)
    {}

    uint32_t index; ///<The index of the clause in Subsumer::clauses

    bool operator<(const ClauseIndex& other) const
    {
        return index<other.index;
    }

    bool operator!=(const ClauseIndex& other) const
    {
        return index != other.index;
    }
};

#define CL_ABST_TYPE uint64_t
#define CLAUSE_ABST_SIZE 64

template <class T> CL_ABST_TYPE calcAbstraction(const T& ps) {
    CL_ABST_TYPE abstraction = 0;
    for (uint16_t i = 0; i != ps.size(); i++)
        abstraction |= 1UL << (ps[i].var() % CLAUSE_ABST_SIZE);
    return abstraction;
}

struct AbstData
{
    AbstData(Clause& c, const bool _defOfOrGate) :
        abst(calcAbstraction(c))
        , size(c.size())
        , defOfOrGate(_defOfOrGate)
    {}

    //Data about clause
    CL_ABST_TYPE abst;
    uint16_t     size;
    bool         defOfOrGate;
};

/**
 * occur[index(lit)]' is a list of constraints containing 'lit'.
 */
class Occur
{
public:
    typedef vector<ClauseIndex>::iterator iterator;
    typedef vector<ClauseIndex>::const_iterator const_iterator;

    /*
    //Normal
    iterator begin()
    {
        return occ.begin();
    }

    iterator end()
    {
        return occ.end();
    }*/


    //Constant iterator
    const_iterator begin() const
    {
        return occ.begin();
    }

    const_iterator end() const
    {
        return occ.end();
    }

    size_t size() const
    {
        return occ.size();
    }

    size_t empty() const
    {
        return occ.size();
    }

    //Sanity check
    bool isSorted(const vector<AbstData>& clauseData)
    {
        //If empty or contains only one element, it's sorted
        if (occ.size() < 2)
            return true;

        //Next clause's size cannot be smaller than the previous' size
        vector<ClauseIndex>::const_iterator it = occ.begin();
        vector<ClauseIndex>::const_iterator it2 = it;
        it2++;
        for(vector<ClauseIndex>::const_iterator end = occ.end(); it2 != end; it++, it2++) {
            assert(it->index != std::numeric_limits<uint32_t>::max());
            assert(it2->index != std::numeric_limits<uint32_t>::max());

            //Sorted in descending order. If occ[0] is smaller than occ[1], it's an error
            if (clauseData[it->index].size < clauseData[it2->index].size)
                return false;
        }

        return true;
    }

    void add(ClauseIndex c, const vector<AbstData>& clauseData, size_t size)
    {
        //Make room
        occ.push_back(ClauseIndex());

        size_t i = 0;
        ClauseIndex last;
        bool lastSet = false;
        for(; i < occ.size(); i++) {
            if (lastSet) {
                ClauseIndex backupLast = occ[i];
                occ[i] = last;
                last = backupLast;
                continue;
            }

            //It exists, and is larger, then skip over
            if (occ[i].index != std::numeric_limits<uint32_t>::max()
                && clauseData[occ[i].index].size > size)
                continue;

            //Last is not set, and it's time to swap
            lastSet = true;
            last = occ[i];
            occ[i] = c;
        }
        assert(lastSet);
    }

    void freeMem()
    {
        vector<ClauseIndex> tmp;
        occ.swap(tmp);
    }

    void clear()
    {
        occ.clear();
    }

    void remove(const ClauseIndex c)
    {
        removeW(occ, c);
    }

    void update(const ClauseIndex c, vector<AbstData>& clauseData)
    {
        //First remove
        remove(c);

        //Then add
        add(c, clauseData, clauseData[c.index].size);
    }

private:
    vector<ClauseIndex>  occ;
};

/**
@brief Used to quicky add, remove and iterate through a clause set

Used in Subsumer to put into a set all clauses that need to be treated
*/
class CSet {
    vector<uint32_t>       where;  ///<Map clause ID to position in 'which'.
    vector<ClauseIndex>     which;  ///< List of clauses (for fast iteration). May contain 'Clause_NULL'.
    vector<uint32_t>       free;   ///<List of positions holding 'Clause_NULL'.

    public:
        //ClauseSimp& operator [] (uint32_t index) { return which[index]; }
        void reserve(uint32_t size) { where.reserve(size);}
        //uint32_t size(void) const { return which.size(); }
        ///@brief Number of elements in the set
        uint32_t nElems(void) const { return which.size() - free.size(); }

        /**
        @brief Add a clause to the set
        */
        bool add(const ClauseIndex& c) {
            assert(c.index != std::numeric_limits< uint32_t >::max());
            if (where.size() < c.index+1)
                where.resize(c.index+1, std::numeric_limits<uint32_t>::max());

            if (where[c.index] != std::numeric_limits<uint32_t>::max()) {
                return false;
            }
            if (free.size() > 0){
                where[c.index] = free.back();
                which[free.back()] = c;
                free.pop_back();;
            }else{
                where[c.index] = which.size();
                which.push_back(c);
            }
            return true;
        }

        bool alreadyIn(const ClauseIndex& c) const {
            assert(c.index != std::numeric_limits< uint32_t >::max());
            if (where.size() < c.index+1) return false;
            if (where[c.index] != std::numeric_limits<uint32_t>::max())
                return true;
            return false;
        }

        /**
        @brief Remove clause from set

        Handles it correctly if the clause was not in the set anyway
        */
        bool exclude(const ClauseIndex& c) {
            assert(c.index != std::numeric_limits< uint32_t >::max());
            if (c.index >= where.size() || where[c.index] == std::numeric_limits<uint32_t>::max()) {
                //not inside
                return false;
            }
            free.push_back(where[c.index]);
            which[where[c.index]].index =std::numeric_limits< uint32_t >::max();
            where[c.index] = std::numeric_limits<uint32_t>::max();
            return true;
        }

        /**
        @brief Fully clear the set
        */
        void clear(void) {
            for (uint32_t i = 0; i < which.size(); i++)  {
                if (which[i].index != std::numeric_limits< uint32_t >::max()) {
                    where[which[i].index] = std::numeric_limits<uint32_t>::max();
                }
            }
            which.clear();
            free.clear();
        }

        /**
        @brief A normal iterator to iterate through the set

        No other way exists of iterating correctly.
        */
        class iterator
        {
            public:
                iterator(vector<ClauseIndex>::iterator _it) :
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

                ClauseIndex& operator*() {
                    return *it;
                }

                vector<ClauseIndex>::iterator& operator->() {
                    return it;
                }
            private:
                vector<ClauseIndex>::iterator it;
        };

        /**
        @brief A constant iterator to iterate through the set

        No other way exists of iterating correctly.
        */
        class const_iterator
        {
            public:
                const_iterator(vector<ClauseIndex>::const_iterator _it) :
                it(_it)
                {}

                void operator++()
                {
                    it++;
                }

                bool operator!=(const const_iterator& iter) const
                {
                    return (it != iter.it);;
                }

                const ClauseIndex& operator*() {
                    return *it;
                }

                vector<ClauseIndex>::const_iterator& operator->() {
                    return it;
                }
            private:
                vector<ClauseIndex>::const_iterator it;
        };

        ///@brief Get starting iterator
        iterator begin()
        {
            return iterator(which.begin());
        }

        ///@brief Get ending iterator
        iterator end()
        {
            return iterator(which.begin() + which.size());
        }

        ///@brief Get starting iterator (constant version)
        const_iterator begin() const
        {
            return const_iterator(which.begin());
        }

        ///@brief Get ending iterator (constant version)
        const_iterator end() const
        {
            return const_iterator(which.begin() + which.size());
        }
};

class ClAndBin {
    public:
        ClAndBin();
        ClAndBin(ClauseIndex cl, const bool _learnt) :
            clsimp(cl)
            , lit1(lit_Undef)
            , lit2(lit_Undef)
            , isBin(false)
            , learnt(_learnt)
        {}

        ClAndBin(const Lit _lit1, const Lit _lit2, const bool _learnt) :
            clsimp(std::numeric_limits< uint32_t >::max())
            , lit1(_lit1)
            , lit2(_lit2)
            , isBin(true)
            , learnt(_learnt)
        {}

        ClauseIndex clsimp;
        Lit lit1;
        Lit lit2;
        bool isBin;
        bool learnt;

        std::string print(const vector<Clause*>&clauses) const
        {
            std::stringstream ss;
            if (isBin) {
                ss << lit1 << " , " << lit2;
            } else {
                if (clauses[clsimp.index])
                    ss << *clauses[clsimp.index];
                else
                    ss << "NULL";
            }
            return ss.str();
        }

        bool operator==(const ClAndBin& other) const
        {
            if (isBin) {
                if (clsimp != other.clsimp
                    || learnt != other.learnt)
                    return false;
            } else {
                if (lit1 != other.lit1
                    || lit2 != other.lit2)
                    return false;
            }

            return true;
        }
};

#endif //CSET_H

