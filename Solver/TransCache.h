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

        void merge(std::vector<LitExtra>& otherLits)
        {
            std::sort(lits.begin(), lits.end());
            lits.erase(std::unique(lits.begin(), lits.end()), lits.end());
            std::sort(otherLits.begin(), otherLits.end());

            //swap to keep original place if possible
            std::vector<LitExtra> oldLits(lits);
            std::vector<LitExtra> newLits;
            newLits.swap(lits);
            newLits.clear();

            std::vector<LitExtra>::iterator it = oldLits.begin();
            std::vector<LitExtra>::iterator itOther = otherLits.begin();
            for (;itOther != otherLits.end() || it != oldLits.end();) {
                if (it == oldLits.end()) {
                    newLits.push_back(*itOther);
                    itOther++;
                    continue;
                }
                if (itOther == otherLits.end()) {
                    newLits.push_back(*it);
                    it++;
                    continue;
                }

                //now both of them contain something
                if (it->getLit() == itOther->getLit()) {
                    bool onlyNLBin = it->getOnlyNLBin() || it->getOnlyNLBin();
                    newLits.push_back(LitExtra(it->getLit(), onlyNLBin));
                    it++;
                    itOther++;
                    continue;
                }

                if (it->getLit() < itOther->getLit()) {
                    newLits.push_back(*it);
                    it++;
                    continue;
                }

                assert(it->getLit() > itOther->getLit());
                newLits.push_back(*itOther);
                it++;
            }
            lits.swap(newLits);
        }

        std::vector<LitExtra> lits;
        uint64_t conflictLastUpdated;
};

#endif //TRANSCACHE_H