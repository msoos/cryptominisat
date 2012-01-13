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

        void merge(std::vector<Lit>& otherLits, bool onlynonlearnt, std::vector<uint16_t>& seen)
        {
            for (size_t i = 0, size = otherLits.size(); i < size; i++) {
                seen[otherLits[i].toInt()] = true;
            }

            for (size_t i = 0, size = lits.size(); i < size; i++) {
                if (onlynonlearnt
                    && !lits[i].getOnlyNLBin()
                    && seen[lits[i].getLit().toInt()])
                        lits[i].setOnlyNLBin();

                seen[lits[i].getLit().toInt()] = 0;
            }

            for (size_t i = 0 ,size = otherLits.size(); i < size; i++) {
                Lit lit = otherLits[i];
                if (seen[lit.toInt()]) {
                    lits.push_back(LitExtra(lit, onlynonlearnt));
                    seen[lit.toInt()] = false;
                }
            }
        }

        std::vector<LitExtra> lits;
        //uint64_t conflictLastUpdated;
};

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

        void clean(ThreadControl* control);
        bool tryBoth(ThreadControl* control);
};

#endif //TRANSCACHE_H
