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

#ifndef __STAMP_H__
#define __STAMP_H__

#include <vector>
#include <algorithm>
#include "solvertypes.h"
#include "clause.h"
#include "constants.h"

namespace CMSat {

using std::vector;
class VarReplacer;

enum StampType {
    STAMP_IRRED = 0
    , STAMP_RED = 1
};

struct Timestamp
{
    Timestamp()
    {
        start[STAMP_IRRED] = 0;
        start[STAMP_RED] = 0;

        end[STAMP_IRRED] = 0;
        end[STAMP_RED] = 0;
    }

    uint64_t start[2];
    uint64_t end[2];
};

class Stamp
{
public:
    bool stampBasedClRem(const vector<Lit>& lits) const;
    std::pair<size_t, size_t> stampBasedLitRem(
        vector<Lit>& lits
        , StampType stampType
    ) const;
    void updateVars(
        const vector<Var>& outerToInter
        , const vector<Var>& interToOuter2
        , vector<uint16_t>& seen
    );
    void clearStamps();
    void save_on_var_memory(const uint32_t newNumVars);

    vector<Timestamp>   tstamp;
    void new_var()
    {
        tstamp.push_back(Timestamp());
        tstamp.push_back(Timestamp());
    }
    void new_vars(const size_t n)
    {
        tstamp.resize(tstamp.size() + 2*n, Timestamp());
    }
    size_t mem_used() const
    {
        return tstamp.capacity()*sizeof(Timestamp);
    }

    void freeMem()
    {
        vector<Timestamp> tmp;
        tstamp.swap(tmp);
    }

private:
    struct StampSorter
    {
        StampSorter(
            const vector<Timestamp>& _timestamp
            , const StampType _stampType
            , const bool _rev
        ) :
            timestamp(_timestamp)
            , stampType(_stampType)
            , rev(_rev)
        {}

        const vector<Timestamp>& timestamp;
        const StampType stampType;
        const bool rev;

        bool operator()(const Lit lit1, const Lit lit2) const
        {
            if (!rev) {
                return timestamp[lit1.toInt()].start[stampType]
                        < timestamp[lit2.toInt()].start[stampType];
            } else {
                return timestamp[lit1.toInt()].start[stampType]
                        > timestamp[lit2.toInt()].start[stampType];
            }
        }
    };

    struct StampSorterInv
    {
        StampSorterInv(
            const vector<Timestamp>& _timestamp
            , const StampType _stampType
            , const bool _rev
        ) :
            timestamp(_timestamp)
            , stampType(_stampType)
            , rev(_rev)
        {}

        const vector<Timestamp>& timestamp;
        const StampType stampType;
        const bool rev;

        bool operator()(const Lit lit1, const Lit lit2) const
        {
            if (!rev) {
                return timestamp[(~lit1).toInt()].start[stampType]
                    < timestamp[(~lit2).toInt()].start[stampType];
            } else {
                return timestamp[(~lit1).toInt()].start[stampType]
                    > timestamp[(~lit2).toInt()].start[stampType];
            }
        }
    };

    mutable vector<Lit> stampNorm;
    mutable vector<Lit> stampInv;
};

} //end namespace

#endif //__STAMP_H__
