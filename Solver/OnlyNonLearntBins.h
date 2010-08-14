/***********************************************************************
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
************************************************************************/

#ifndef ONLYNONLEARNTBINS_H
#define ONLYNONLEARNTBINS_H

#include "Solver.h"
#include <set>
using std::set;

class OnlyNonLearntBins
{
    public:
        OnlyNonLearntBins(Solver& solver);

        //propagation
        const bool propagate();
        const bool propagateBinExcept(const Lit& exceptLit);
        const bool propagateBinOneLevel();

        //Management of clauses
        const bool fill();
        void removeBin(Lit lit1, Lit lit2);
        void attachBin(const Clause& c);
        const uint32_t removeBins();

        //helper
        inline const uint32_t getWatchSize(const Lit lit) const;
        inline const vec<vec<WatchedBin> >& getBinWatches() const;

    private:
        vec<vec<WatchedBin> > binwatches;
        set<uint64_t> toRemove;
        
        Solver& solver;
};

inline const uint32_t OnlyNonLearntBins::getWatchSize(const Lit lit) const
{
    return binwatches[lit.toInt()].size();
}

inline const vec<vec<WatchedBin> >& OnlyNonLearntBins::getBinWatches() const
{
    return binwatches;
}

#endif //ONLYNONLEARNTBINS_H
