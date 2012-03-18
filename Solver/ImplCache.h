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

#ifndef TRANSCACHE_H
#define TRANSCACHE_H

#include "constants.h"
#include <vector>
#include <limits>
#include <algorithm>
#include "SolverTypes.h"
#include <vector>

class ThreadControl;

class LitReachData {
    public:
        LitReachData() :
            lit(lit_Undef)
            , numInCache(0)
        {}
        Lit lit;
        uint32_t numInCache;
};

class LitExtra {
    public:
        LitExtra() {};

        LitExtra(const Lit l, const bool onlyNLBin)
        {
            x = onlyNLBin + (l.toInt() << 1);
        }

        const Lit getLit() const
        {
            return Lit::toLit(x>>1);
        }

        bool getOnlyNLBin() const
        {
            return x&1;
        }

        void setOnlyNLBin()
        {
            x  |= 0x1;
        }

        bool operator<(const LitExtra other) const
        {
            if (getOnlyNLBin() && !other.getOnlyNLBin()) return false;
            if (!getOnlyNLBin() && other.getOnlyNLBin()) return true;
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
        {};

        void merge(
            vector<LitExtra>& otherLits
            , const Lit extraLit
            , const bool learnt
            , const Lit leaveOut
            , vector<uint16_t>& seen
        );

        void updateVars(const std::vector< uint32_t >& outerToInter);

        std::vector<LitExtra> lits;
        //uint64_t conflictLastUpdated;
};

namespace std
{
    template <>
    inline void swap (TransCache& m1, TransCache& m2)
    {
         m1.lits.swap(m2.lits);
    }
}

inline std::ostream& operator<<(std::ostream& os, const TransCache& tc)
{
    for (size_t i = 0; i < tc.lits.size(); i++) {
        os << tc.lits[i].getLit()
        << "(" << (tc.lits[i].getOnlyNLBin() ? "NL" : "L") << ") ";
    }
    return os;
}

class ImplCache  {
    public:
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

        void addNew()
        {
            implCache.push_back(TransCache());
            implCache.push_back(TransCache());
        }

        size_t size() const
        {
            return implCache.size();
        }

        void updateVars(
            vector<uint16_t>& seen
            , const std::vector< uint32_t >& outerToInter
            , const std::vector< uint32_t >& interToOuter2
        );

        void clean(ThreadControl* control);
        bool tryBoth(ThreadControl* control);
        void handleNewData(
            vector<uint16_t>& val
            , Var var
            , Lit lit
            , uint32_t& bProp
            , uint32_t& bXProp
        );

        vector<std::pair<vector<Lit>, bool> > delayedClausesToAddXor;
        vector<Lit> delayedClausesToAddNorm;
        bool addDelayedClauses(ThreadControl* control);
};

#endif //TRANSCACHE_H
