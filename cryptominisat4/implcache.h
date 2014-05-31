/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
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

#ifndef TRANSCACHE_H
#define TRANSCACHE_H

#include <vector>
#include <limits>
#include <algorithm>
#include <vector>
#include "constants.h"
#include "solvertypes.h"

namespace CMSat {

class Solver;

class LitExtra {
public:
    LitExtra() {}

    LitExtra(const Lit l, const bool onlyNLBin)
    {
        x = ((uint32_t)onlyNLBin) | (l.toInt() << 1);
    }

    const Lit getLit() const
    {
        return Lit::toLit(x>>1);
    }

    bool getOnlyIrredBin() const
    {
        return x&1;
    }

    void setOnlyIrredBin()
    {
        x  |= 0x1;
    }

    bool operator<(const LitExtra other) const
    {
        if (getOnlyIrredBin() && !other.getOnlyIrredBin()) return false;
        if (!getOnlyIrredBin() && other.getOnlyIrredBin()) return true;
        return (getLit() < other.getLit());
    }

    bool operator==(const LitExtra other) const
    {
        return x == other.x;
    }

    bool operator!=(const LitExtra other) const
    {
        return x != other.x;
    }
private:
    uint32_t x;

};

class TransCache {
public:
    TransCache()
        //conflictLastUpdated(std::numeric_limits<uint64_t>::max())
    {}

    bool merge(
        const vector<LitExtra>& otherLits
        , const Lit extraLit
        , const bool red
        , const Var leaveOut
        , vector<uint16_t>& seen
    );
    bool merge(
        const vector<Lit>& otherLits //Lits to add
        , const Lit extraLit //Add this, too to the list of lits
        , const bool red //The step was a redundant-dependent step?
        , const Var leaveOut //Leave this literal out
        , vector<uint16_t>& seen
    );
    void makeAllRed();

    void updateVars(
        const std::vector< uint32_t >& outerToInter
        , const size_t newMaxVars
    );

    std::vector<LitExtra> lits;
    //uint64_t conflictLastUpdated;

private:
    bool mergeHelper(
        const Lit extraLit //Add this, too to the list of lits
        , const bool red //The step was a redundant-dependent step?
        , vector<uint16_t>& seen
    );
};

inline std::ostream& operator<<(std::ostream& os, const TransCache& tc)
{
    for (size_t i = 0; i < tc.lits.size(); i++) {
        os << tc.lits[i].getLit()
        << "(" << (tc.lits[i].getOnlyIrredBin() ? "NL" : "L") << ") ";
    }
    return os;
}

class ImplCache  {
public:
    void printStats(const Solver* solver) const;
    void printStatsSort(const Solver* solver) const;
    size_t memUsed() const;
    void makeAllRed();
    void saveVarMems(uint32_t newNumVars)
    {
        implCache.resize(newNumVars*2);
        implCache.shrink_to_fit();
    }

    std::vector<TransCache> implCache;

    std::vector<TransCache>::iterator begin()
    {
        return implCache.begin();
    }

    std::vector<TransCache>::iterator end()
    {
        return implCache.end();
    }

    std::vector<TransCache>::const_iterator begin() const
    {
        return implCache.begin();
    }

    std::vector<TransCache>::const_iterator end() const
    {
        return implCache.end();
    }

    const TransCache& operator[](const size_t at) const
    {
        return implCache[at];
    }

    TransCache& operator[](const size_t at)
    {
        return implCache[at];
    }

    void new_var()
    {
        implCache.push_back(TransCache());
        implCache.push_back(TransCache());
    }

    void new_vars(const size_t n)
    {
        implCache.resize(implCache.size()+2*n);
    }

    size_t size() const
    {
        return implCache.size();
    }

    void updateVars(
        vector<uint16_t>& seen
        , const std::vector< uint32_t >& outerToInter
        , const std::vector< uint32_t >& interToOuter2
        , const size_t newMaxVars
    );

    bool clean(Solver* solver, bool* setSomething = NULL);
    bool tryBoth(Solver* solver);

    struct TryBothStats
    {
        TryBothStats() :
            numCalls(0)
            , cpu_time(0)
            , zeroDepthAssigns(0)
            , varReplaced(0)
            , bProp(0)
            , bXProp(0)
        {}

        void clear()
        {
            TryBothStats tmp;
            *this = tmp;
        }

        TryBothStats& operator+=(const TryBothStats& other)
        {
            numCalls += other.numCalls;
            cpu_time += other.cpu_time;
            zeroDepthAssigns += other.zeroDepthAssigns;
            varReplaced += other.varReplaced;
            bProp += other.bProp;
            bXProp += other.bXProp;

            return *this;
        }

        void printShort() const
        {
            cout
            << "c [bcache] "
            //<< " set: " << bProp
            << " 0-depth ass: " << zeroDepthAssigns
            //<< " BXProp: " << bXProp
            << " BXprop: " << bXProp
            << " T: " << cpu_time
            << endl;
        }

        uint64_t numCalls;
        double cpu_time;
        uint64_t zeroDepthAssigns;
        uint64_t varReplaced;
        uint64_t bProp;
        uint64_t bXProp;
    };

    TryBothStats runStats;
    TryBothStats globalStats;
    const TryBothStats& getStats() const
    {
        return globalStats;
    }

    void free()
    {
        vector<TransCache> tmp;
        implCache.swap(tmp);
    }

    void clear()
    {
        for(vector<TransCache>::iterator
            it = implCache.begin(), end = implCache.end()
            ; it != end
            ; it++
        ) {
            it->lits.clear();
        }
    }

private:
    void tryVar(Solver* solver, Var var);

    void handleNewData(
        vector<uint16_t>& val
        , Var var
        , Lit lit
    );

    vector<std::pair<vector<Lit>, bool> > delayedClausesToAddXor;
    vector<Lit> delayedClausesToAddNorm;
    bool addDelayedClauses(Solver* solver);
};

} //end namespace

namespace std
{
    template <>
    inline void swap (CMSat::TransCache& m1, CMSat::TransCache& m2) noexcept (true)
    {
         m1.lits.swap(m2.lits);
    }
}

#endif //TRANSCACHE_H
