#ifndef __STAMP_H__
#define __STAMP_H__

#include <vector>
#include <algorithm>
#include "solvertypes.h"
#include "clause.h"
#include "constants.h"

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
    void updateVars(
        const vector<Var>& outerToInter
        , const vector<Var>& interToOuter2
        , vector<uint16_t>& seen
    );
    void updateDominators(const VarReplacer* replacer);
    void clearStamps();
    void saveVarMem(const uint32_t newNumVars);

    vector<Timestamp>   tstamp;
    void new_var()
    {
        tstamp.push_back(Timestamp());
        tstamp.push_back(Timestamp());
    }
    void new_vars(const size_t n)
    {
        tstamp.resize(tstamp.size() + 2*n, Timestamp());
    }
    size_t memUsed() const
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
