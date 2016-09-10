/******************************************
Copyright (c) 2016, Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***********************************************/

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
        uint64_t numResolutions() const
        {
            return antec_data.num();
        }

        Data& operator+=(const Data& other)
        {
            num += other.num;
            lits += other.lits;
            age += other.age;

            glue += other.glue;
            numConfl += other.numConfl;
            used_for_uip_creation += other.used_for_uip_creation;
            antec_data += other.antec_data;

            #ifdef STATS_NEEDED
            numProp += other.numProp;
            numLookedAt += other.numLookedAt;
            #endif

            return *this;
        }

        uint64_t num = 0;
        uint64_t lits = 0;
        uint64_t age = 0;

        uint64_t glue = 0;
        uint64_t numConfl = 0;
        uint64_t used_for_uip_creation = 0;
        AtecedentData<uint64_t> antec_data;
        //NOTE: cannot have activity, it overflows

        #ifdef STATS_NEEDED
        uint64_t numProp = 0;
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
            numConfl += cl->stats.conflicts_made;
            numLookedAt += cl->stats.clause_looked_at;
            numProp += cl->stats.propagations_made;
            age += sumConfl - cl->stats.introduced_at_conflict;
            used_for_uip_creation += cl->stats.used_for_uip_creation;
            antec_data += cl->stats.antec_data;
            #endif
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
