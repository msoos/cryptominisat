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

class PartFinder {

    public:
        PartFinder(Solver* solver);
        bool findParts();
        bool getTimedOut() const;

        const map<uint32_t, vector<Var> >& getReverseTable() const; // part->var
        uint32_t getVarPart(const Var var) const;
        const vector<uint32_t>& getTable() const; //var -> part
        const vector<Var>& getPartVars(const uint32_t part);

    private:
        void addToPartImplicits();
        void addToPartClauses(const vector<ClOffset>& cs);
        template<class T>
        void addToPartClause(const T& cl);

        struct MySorter
        {
            bool operator () (
                const pair<uint32_t, uint32_t>& left
                , const pair<uint32_t, uint32_t>& right
            ) {
                return left.second < right.second;
            }
        };

        /*const uint32_t setParts();
        template<class T>
        void calcIn(const vec<T*>& cs, vector<uint32_t>& numClauseInPart, vector<uint32_t>& sumLitsInPart);
        void calcInBins(vector<uint32_t>& numClauseInPart, vector<uint32_t>& sumLitsInPart);*/

        //part -> vars
        map<uint32_t, vector<Var> > reverseTable;

        //var -> part
        vector<uint32_t> table;

        //The part counter
        uint32_t part_no;
        uint32_t used_part_no;

        //Temporary
        vector<Var> newSet;
        vector<uint32_t> tomerge;

        //Keep track of time
        uint64_t timeUsed;
        bool timedout;

        Solver* solver;
};

inline const map<uint32_t, vector<Var> >& PartFinder::getReverseTable() const
{
    assert(!timedout);
    return reverseTable;
}

inline const vector<Var>& PartFinder::getTable() const
{
    assert(!timedout);
    return table;
}

inline uint32_t PartFinder::getVarPart(const Var var) const
{
    assert(!timedout);
    return table[var];
}

inline const vector<Var>& PartFinder::getPartVars(const uint32_t part)
{
    assert(!timedout);
    return reverseTable[part];
}

inline bool PartFinder::getTimedOut() const
{
    return timedout;
}

} //End namespace

#endif //PARTFINDER_H
