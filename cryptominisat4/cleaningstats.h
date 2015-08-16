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

#ifndef __CLEANINGSTATS_H__
#define __CLEANINGSTATS_H__

#include <cstdint>
#include "clause.h"

namespace CMSat {

class Solver;

struct CleaningStats
{
    struct Data
    {
        uint64_t sumResolutions() const
        {
            return resol.sum();
        }

        Data& operator+=(const Data& other)
        {
            num += other.num;
            lits += other.lits;
            age += other.age;

            glue += other.glue;
            numConfl += other.numConfl;
            used_for_uip_creation += other.used_for_uip_creation;
            resol += other.resol;
            act += other.act;

            #ifdef STATS_NEEDED
            numProp += other.numProp;
            numLookedAt += other.numLookedAt;
            numLitVisited += other.numLitVisited;
            #endif

            return *this;
        }

        uint64_t num = 0;
        uint64_t lits = 0;
        uint64_t age = 0;

        uint64_t glue = 0;
        uint64_t numConfl = 0;
        uint64_t used_for_uip_creation = 0;
        ResolutionTypes<uint64_t> resol;
        double   act = 0.0;

        #ifdef STATS_NEEDED
        uint64_t numProp = 0;
        uint64_t numLitVisited = 0;
        uint64_t numLookedAt = 0;
        #endif

        void incorporate(const Clause* cl, size_t 
            #ifdef STATS_NEEDED
            sumConfl
            #endif
        ) {
            num ++;
            lits += cl->size();
            glue += cl->stats.glue;

            #ifdef STATS_NEEDED
            act += cl->stats.activity;
            numConfl += cl->stats.conflicts_made;
            numLitVisited += cl->stats.visited_literals;
            numLookedAt += cl->stats.clause_looked_at;
            numProp += cl->stats.propagations_made;
            age += sumConfl - cl->stats.introduced_at_conflict;
            used_for_uip_creation += cl->stats.used_for_uip_creation;
            #endif
            resol += cl->stats.resolutions;
        }


    };
    CleaningStats& operator+=(const CleaningStats& other);

    void print(const double total_cpu_time) const;
    void print_short(const Solver* solver) const;

    double cpu_time = 0;

    //Before remove
    uint64_t origNumClauses = 0;
    uint64_t origNumLits = 0;

    //Clause Cleaning
    Data removed;
    Data remain;
};

}

#endif //__CLEANINGSTATS_H__
