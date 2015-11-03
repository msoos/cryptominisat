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

#ifndef _TOPLEVELGAUSS_H_
#define _TOPLEVELGAUSS_H_

#include "xor.h"
#include <vector>
#include <set>
using std::vector;
using std::set;

namespace CMSat {

class Solver;
class OccSimplifier;

class TopLevelGauss
{
public:
    TopLevelGauss(OccSimplifier* _occsimplifier, Solver* _solver);
    bool toplevelgauss(const vector<Xor>& _xors);

    struct Stats
    {
        void clear()
        {
            Stats tmp;
            *this = tmp;
        }

        double total_time() const
        {
            return extractTime + blockCutTime;
        }

        Stats& operator+=(const Stats& other);
        void print_short(const Solver* solver) const;
        void print(const size_t numCalls) const;

        //Time
        uint32_t numCalls = 0;
        double extractTime = 0.0;
        double blockCutTime = 0.0;

        //XOR stats
        uint64_t numVarsInBlocks = 0;
        uint64_t numBlocks = 0;

        //Usefulness stats
        uint64_t time_outs = 0;
        uint64_t newUnits = 0;
        uint64_t newBins = 0;

        size_t zeroDepthAssigns = 0;
    };

    Stats runStats;
    Stats globalStats;
    size_t mem_used() const;

private:
    Solver* solver;
    OccSimplifier* occsimplifier;

    bool extractInfo();
    void cutIntoBlocks(const vector<size_t>& xorsToUse);
    bool extractInfoFromBlock(const vector<uint32_t>& block, const size_t blockNum);
    vector<uint32_t> getXorsForBlock(const size_t blockNum);

    //Major calculated data and indexes to this data
    vector<vector<uint32_t> > blocks; ///<Blocks of vars that are in groups of XORs
    vector<uint32_t> varToBlock; ///<variable-> block index map

    //Temporaries for putting xors into matrix, and extracting info from matrix
    vector<uint32_t> outerToInterVarMap;
    vector<uint32_t> interToOUterVarMap;

    vector<Xor> xors;
};

}

#endif // _TOPLEVELGAUSS_H_
