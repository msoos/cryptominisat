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

#ifndef __SEARCHSTATS_H__
#define __SEARCHSTATS_H__

#include <cstdint>

#include "solvertypes.h"
#include "clause.h"

namespace CMSat {

class SearchStats
{
public:
    void clear()
    {
        SearchStats tmp;
        *this = tmp;
    }

    SearchStats& operator+=(const SearchStats& other);
    SearchStats& operator-=(const SearchStats& other);
    SearchStats operator-(const SearchStats& other) const;
    void printCommon(uint64_t props) const;
    void print_short(uint64_t props) const;
    void print(uint64_t props) const;

    //Restart stats
    uint64_t blocked_restart = 0;
    uint64_t blocked_restart_same = 0;
    uint64_t numRestarts = 0;

    //Decisions
    uint64_t  decisions = 0;
    uint64_t  decisionsAssump = 0;
    uint64_t  decisionsRand = 0;
    uint64_t  decisionFlippedPolar = 0;

    //Clause shrinking
    uint64_t litsRedNonMin = 0;
    uint64_t litsRedFinal = 0;
    uint64_t recMinCl = 0;
    uint64_t recMinLitRem = 0;
    uint64_t furtherShrinkAttempt = 0;
    uint64_t binTriShrinkedClause = 0;
    uint64_t cacheShrinkedClause = 0;
    uint64_t furtherShrinkedSuccess = 0;
    uint64_t stampShrinkAttempt = 0;
    uint64_t stampShrinkCl = 0;
    uint64_t stampShrinkLit = 0;
    uint64_t moreMinimLitsStart = 0;
    uint64_t moreMinimLitsEnd = 0;
    uint64_t recMinimCost = 0;

    //Learnt clause stats
    uint64_t learntUnits = 0;
    uint64_t learntBins = 0;
    uint64_t learntTris = 0;
    uint64_t learntLongs = 0;
    uint64_t otfSubsumed = 0;
    uint64_t otfSubsumedImplicit = 0;
    uint64_t otfSubsumedLong = 0;
    uint64_t otfSubsumedRed = 0;
    uint64_t otfSubsumedLitsGained = 0;
    uint64_t guess_different = 0;
    uint64_t cache_hit = 0;
    uint64_t red_cl_in_which0 = 0;

    //Hyper-bin & transitive reduction
    uint64_t advancedPropCalled = 0;
    uint64_t hyperBinAdded = 0;
    uint64_t transReduRemIrred = 0;
    uint64_t transReduRemRed = 0;

    //SolveFeatures
    uint64_t num_xors_found_last = 0;
    uint64_t num_gates_found_last = 0;
    uint64_t clauseID_at_start_inclusive = 0;
    uint64_t clauseID_at_end_exclusive = 0;

    //Resolution Stats
    AtecedentData<uint64_t> resolvs;

    //Stat structs
    ConflStats conflStats;

    //Time
    double cpu_time = 0.0;
};

}

#endif //__SEARCHSTATS_H__
