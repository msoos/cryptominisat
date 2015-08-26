/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
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

#ifndef PARTFINDER_H
#define PARTFINDER_H

#include <vector>
#include <map>
#include "constants.h"
#include "solvertypes.h"
#include "cloffset.h"

namespace CMSat {

class Solver;
class Clause;

using std::map;
using std::vector;
using std::pair;

class CompFinder {

    public:
        CompFinder(Solver* solver);
        bool find_components();
        bool getTimedOut() const;

        const map<uint32_t, vector<Var> >& getReverseTable() const; // comp->var
        uint32_t getVarComp(const Var var) const;
        const vector<uint32_t>& getTable() const; //var -> comp
        const vector<Var>& getCompVars(const uint32_t comp);

    private:
        void addToCompImplicits();
        void add_clauses_to_component(const vector<ClOffset>& cs);
        template<class T>
        void add_clause_to_component(const T& cl);
        template<class T>
        bool belong_to_same_component(const T& cl);
        template<class T>
        void fill_newset_and_tomerge(const T& cl);
        void merge_newset_into_single_component();

        void time_out_print(const double myTime) const;
        void print_found_components() const;
        bool reverse_table_is_correct() const;
        void print_and_add_to_sql_result(const double myTime) const;

        struct MySorter
        {
            bool operator () (
                const pair<uint32_t, uint32_t>& left
                , const pair<uint32_t, uint32_t>& right
            ) {
                return left.second < right.second;
            }
        };

        //comp -> vars
        map<uint32_t, vector<Var> > reverseTable;

        //var -> comp
        vector<uint32_t> table;

        //The comp counter
        uint32_t comp_no;
        uint32_t used_comp_no;

        //Temporary
        vector<Var> newSet;
        vector<uint32_t> tomerge;

        //Keep track of time
        long long bogoprops_remain;
        long long orig_bogoprops;
        bool timedout;

        vector<uint16_t>& seen;
        Solver* solver;
};

inline const map<uint32_t, vector<Var> >& CompFinder::getReverseTable() const
{
    assert(!timedout);
    return reverseTable;
}

inline const vector<Var>& CompFinder::getTable() const
{
    assert(!timedout);
    return table;
}

inline uint32_t CompFinder::getVarComp(const Var var) const
{
    assert(!timedout);
    return table[var];
}

inline const vector<Var>& CompFinder::getCompVars(const uint32_t comp)
{
    assert(!timedout);
    return reverseTable[comp];
}

inline bool CompFinder::getTimedOut() const
{
    return timedout;
}

} //End namespace

#endif //PARTFINDER_H
