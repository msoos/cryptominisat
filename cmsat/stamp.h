#include <vector>
#include <algorithm>
#include "solvertypes.h"
#include "clause.h"
#include "constants.h"

#ifndef __STAMP_H__
#define __STAMP_H__

namespace CMSat {

using std::vector;
class VarReplacer;

enum StampType {
    STAMP_IRRED = 0
    , STAMP_RED = 1
};

struct Timestamp
{
    Timestamp()
    {
        start[STAMP_IRRED] = 0;
        start[STAMP_RED] = 0;

        end[STAMP_IRRED] = 0;
        end[STAMP_RED] = 0;

        dominator[STAMP_IRRED] = lit_Undef;
        dominator[STAMP_RED] = lit_Undef;

        numDom[STAMP_IRRED] = 0;
        numDom[STAMP_RED] = 0;
    }

    Timestamp& operator=(const Timestamp& other)
    {
        memmove(this, &other, sizeof(Timestamp));

        return *this;
    }

    uint64_t start[2];
    uint64_t end[2];

    Lit dominator[2];
    uint32_t numDom[2];
};

class Stamp
{
public:
    void remove_from_stamps(const Var var);
    bool stampBasedClRem(const vector<Lit>& lits) const;
    std::pair<size_t, size_t> stampBasedLitRem(
        vector<Lit>& lits
        , StampType stampType
    ) const;
    void updateDominators(const VarReplacer* replacer);
    void clearStamps();
    void newNumVars(uint32_t newNumVars)
    {
        tstamp.resize(newNumVars*2);
        tstamp.shrink_to_fit();
    }

    vector<Timestamp>   tstamp;
    void newVar()
    {
        tstamp.push_back(Timestamp());
        tstamp.push_back(Timestamp());
    }
    uint64_t getMemUsed() const
    {
        return tstamp.capacity()*sizeof(Timestamp);
    }

    void freeMem()
    {
        vector<Timestamp> tmp;
        tstamp.swap(tmp);
    }

private:
    struct StampSorter
    {
        StampSorter(
            const vector<Timestamp>& _timestamp
            , const StampType _stampType
            , const bool _rev
        ) :
            timestamp(_timestamp)
            , stampType(_stampType)
            , rev(_rev)
        {}

        const vector<Timestamp>& timestamp;
        const StampType stampType;
        const bool rev;

        bool operator()(const Lit lit1, const Lit lit2) const
        {
            if (!rev) {
                return timestamp[lit1.toInt()].start[stampType]
                        < timestamp[lit2.toInt()].start[stampType];
            } else {
                return timestamp[lit1.toInt()].start[stampType]
                        > timestamp[lit2.toInt()].start[stampType];
            }
        }
    };

    struct StampSorterInv
    {
        StampSorterInv(
            const vector<Timestamp>& _timestamp
            , const StampType _stampType
            , const bool _rev
        ) :
            timestamp(_timestamp)
            , stampType(_stampType)
            , rev(_rev)
        {}

        const vector<Timestamp>& timestamp;
        const StampType stampType;
        const bool rev;

        bool operator()(const Lit lit1, const Lit lit2) const
        {
            if (!rev) {
                return timestamp[(~lit1).toInt()].start[stampType]
                    < timestamp[(~lit2).toInt()].start[stampType];
            } else {
                return timestamp[(~lit1).toInt()].start[stampType]
                    > timestamp[(~lit2).toInt()].start[stampType];
            }
        }
    };

    mutable vector<Lit> stampNorm;
    mutable vector<Lit> stampInv;
};

} //end namespace

#endif //__STAMP_H__
