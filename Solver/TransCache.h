#ifndef TRANSCACHE_H
#define TRANSCACHE_H

#include <vector>
#include <limits>
#include <algorithm>

#ifdef _MSC_VER
#include <msvc/stdint.h>
#else
#include <stdint.h>
#endif //_MSC_VER

#include "SolverTypes.h"

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

        const bool getOnlyNLBin() const
        {
            return x&1;
        }

        void setOnlyNLBin()
        {
            x  |= 0x1;
        }

        const bool operator<(const LitExtra other) const
        {
            if (getOnlyNLBin() && !other.getOnlyNLBin()) return false;
            if (!getOnlyNLBin() && other.getOnlyNLBin()) return true;
            return (getLit() < other.getLit());
        }

        const bool operator==(const LitExtra other) const
        {
            return x == other.x;
        }

        const bool operator!=(const LitExtra other) const
        {
            return x != other.x;
        }
    private:
        uint32_t x;

};

class TransCache {
    public:
        TransCache() :
            conflictLastUpdated(std::numeric_limits<uint64_t>::max())
        {};

        void merge(std::vector<Lit>& otherLits, bool onlynonlearnt, vec<char>& seen)
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
        uint64_t conflictLastUpdated;
};

#endif //TRANSCACHE_H