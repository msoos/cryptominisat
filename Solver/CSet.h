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
@brief Used to quicky add, remove and iterate through a clause set

Used in Subsumer to put into a set all clauses that need to be treated
*/
class CSet {
    vector<uint32_t>       where;  ///<Map clause ID to position in 'which'.
    vector<ClauseOffset>   which;  ///< List of clauses (for fast iteration). May contain 'Clause_NULL'.
    vector<uint32_t>       free;   ///<List of positions holding 'Clause_NULL'.

    public:
        //ClauseSimp& operator [] (uint32_t index) { return which[index]; }
        void reserve(uint32_t size) {
            where.reserve(size);
            which.reserve(size);
        }
        //uint32_t size(void) const { return which.size(); }
        ///@brief Number of elements in the set
        uint32_t nElems(void) const { return which.size() - free.size(); }

        /**
        @brief Add a clause to the set
        */
        bool add(const ClauseOffset offs) {
            //Don't check for special value
            assert(offs != std::numeric_limits< uint32_t >::max());

            if (where.size() < offs+1)
                where.resize(offs+1, std::numeric_limits<uint32_t>::max());

            if (where[offs] != std::numeric_limits<uint32_t>::max()) {
                return false;
            }
            if (free.size() > 0){
                where[offs] = free.back();
                which[free.back()] = offs;
                free.pop_back();
            }else{
                where[offs] = which.size();
                which.push_back(offs);
            }
            return true;
        }

        bool alreadyIn(const ClauseOffset offs) const
        {
            //Don't check for special value
            assert(offs != std::numeric_limits< uint32_t >::max());

            if (where.size() < offs+1)
                return false;

            return where[offs] != std::numeric_limits<uint32_t>::max();
        }

        /**
        @brief Remove clause from set

        Handles it correctly if the clause was not in the set anyway
        */
        bool exclude(const ClauseOffset offs) {
            //Don't check for special value
            assert(offs != std::numeric_limits< uint32_t >::max());

            //not inside
            if (offs >= where.size()
                || where[offs] == std::numeric_limits<uint32_t>::max()
            ) {
                return false;
            }

            free.push_back(where[offs]);
            which[where[offs]] = std::numeric_limits< uint32_t >::max();
            where[offs] = std::numeric_limits<uint32_t>::max();

            return true;
        }

        /**
        @brief Fully clear the set
        */
        void clear(void) {
            where.clear();
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
                iterator(vector<ClauseOffset>::iterator _it) :
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

                ClauseOffset& operator*() {
                    return *it;
                }

                vector<ClauseOffset>::iterator& operator->() {
                    return it;
                }
            private:
                vector<ClauseOffset>::iterator it;
        };

        /**
        @brief A constant iterator to iterate through the set

        No other way exists of iterating correctly.
        */
        class const_iterator
        {
            public:
                const_iterator(vector<ClauseOffset>::const_iterator _it) :
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

                const ClauseOffset& operator*() {
                    return *it;
                }

                vector<ClauseOffset>::const_iterator& operator->() {
                    return it;
                }
            private:
                vector<ClauseOffset>::const_iterator it;
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

#endif //CSET_H

