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

/**
@brief Handles propagation, addition&removal of non-learnt binary clauses

It takes a snapshot of Solver's non-learnt binary clauses, builds its own
watchlists, and does everything itself. It is used in UselessBinRemover. We
need this class, because we don't store in Solver's watchlists if binary
clauses are learnt or not.

\todo We could do without this class if we had extra data in normal
watchlists saying whether the clause is learnt or not. That would bring its
own set of problems, but maybe it would be worth it. Who knows.
*/
class OnlyNonLearntBins
{
    public:
        OnlyNonLearntBins(Solver& solver);

        /**
        @brief For storing a binary clause
        */
        class WatchedBin {
        public:
            static void removeWBinAll(vec<WatchedBin> &ws, const Lit impliedLit);
            WatchedBin(Lit _impliedLit) : impliedLit(_impliedLit) {};
            Lit impliedLit;
        };

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
        vec<vec<WatchedBin> > binwatches; ///<Internal wathclists for non-learnt binary clauses
        set<uint64_t> toRemove; ///<Clauses that have been marked to be removed (two 32-bit lits: 64 bit to store)

        Solver& solver;
};

inline const uint32_t OnlyNonLearntBins::getWatchSize(const Lit lit) const
{
    return binwatches[lit.toInt()].size();
}

inline const vec<vec<OnlyNonLearntBins::WatchedBin> >& OnlyNonLearntBins::getBinWatches() const
{
    return binwatches;
}

inline void OnlyNonLearntBins::WatchedBin::removeWBinAll(vec<OnlyNonLearntBins::WatchedBin> &ws, const Lit impliedLit)
{
    WatchedBin *i = ws.getData();
    WatchedBin *j = i;
    for (WatchedBin* end = ws.getDataEnd(); i != end; i++) {
        if (i->impliedLit != impliedLit)
            *j++ = *i;
    }
    ws.shrink(i-j);
}

#endif //ONLYNONLEARNTBINS_H
