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

#ifndef _SEARCHHIST_H_
#define _SEARCHHIST_H_

#include <cstdint>
#include "avgcalc.h"
#include "boundedqueue.h"
#include <iostream>
using std::cout;
using std::endl;

namespace CMSat {

//History
struct SearchHist {
    //About the search
    AvgCalc<uint32_t>   branchDepthHist;     ///< Avg branch depth in current restart
    AvgCalc<uint32_t>   branchDepthDeltaHist;

    AvgCalc<uint32_t>   decisionLevelHistLT;
    AvgCalc<uint32_t>   backtrackLevelHistLT;
    AvgCalc<uint32_t>   trailDepthHistLT;
    AvgCalc<uint32_t>   vsidsVarsAvgLT; //vsids_vars.avg()

    bqueue<uint32_t>    trailDepthHistLonger; ///<total depth, incl. props, decisions and assumps
    AvgCalc<uint32_t>   trailDepthDeltaHist;

    //About the confl generated
    bqueue<uint32_t>    glueHist;   ///< Set of last decision levels in (glue of) conflict clauses
    AvgCalc<uint32_t>   glueHistLT;

    AvgCalc<uint32_t>   conflSizeHist;       ///< Conflict size history
    AvgCalc<uint32_t>   conflSizeHistLT;

    AvgCalc<uint32_t>   numResolutionsHist;  ///< Number of resolutions during conflict analysis
    AvgCalc<uint32_t>   numResolutionsHistLT;

    #ifdef STATS_NEEDED
    bqueue<uint32_t>    trailDepthHist;
    AvgCalc<bool>       conflictAfterConflict;
    #endif

    size_t mem_used() const
    {
        uint64_t used = sizeof(SearchHist);
        used += sizeof(AvgCalc<uint32_t>)*16;
        used += sizeof(AvgCalc<bool>)*4;
        used += sizeof(AvgCalc<size_t>)*2;
        used += sizeof(AvgCalc<double, double>)*2;
        used += glueHist.usedMem();

        return used;
    }

    void clear()
    {
        //About the search
        branchDepthHist.clear();
        branchDepthDeltaHist.clear();
        trailDepthDeltaHist.clear();

        //conflict generated
        glueHist.clear();
        conflSizeHist.clear();
        numResolutionsHist.clear();

        #ifdef STATS_NEEDED
        trailDepthHist.clear();
        conflictAfterConflict.clear();
        #endif
    }

    void reset_glue_hist_size(size_t shortTermHistorySize)
    {
        glueHist.clearAndResize(shortTermHistorySize);
        #ifdef STATS_NEEDED
        trailDepthHist.clearAndResize(shortTermHistorySize);
        #endif
    }

    void setSize(const size_t shortTermHistorySize, const size_t blocking_trail_hist_size)
    {
        glueHist.clearAndResize(shortTermHistorySize);
        trailDepthHistLonger.clearAndResize(blocking_trail_hist_size);
        #ifdef STATS_NEEDED
        trailDepthHist.clearAndResize(shortTermHistorySize);
        #endif
    }

    void print() const
    {
        cout
        << " glue"
        << " "
        #ifdef STATS_NEEDED
        << std::right << glueHist.getLongtTerm().avgPrint(1, 5)
        #endif
        << "/" << std::left << glueHistLT.avgPrint(1, 5)

        << " confllen"
        << " " << std::right << conflSizeHist.avgPrint(1, 5)
        << "/" << std::left << conflSizeHistLT.avgPrint(1, 5)

        << " branchd"
        << " " << std::right << branchDepthHist.avgPrint(1, 5)
        << " branchdd"

        << " " << std::right << branchDepthDeltaHist.avgPrint(1, 4)

        #ifdef STATS_NEEDED
        << " traild"
        << " " << std::right << trailDepthHist.getLongtTerm().avgPrint(0, 7)
        #endif

        << " traildd"
        << " " << std::right << trailDepthDeltaHist.avgPrint(0, 5)
        ;

        cout << std::right;
    }
};

} //end namespace

#endif //_SEARCHHIST_H_
