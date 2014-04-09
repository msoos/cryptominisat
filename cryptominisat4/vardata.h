#ifndef __VARDATA_H__
#define __VARDATA_H__

#include "propby.h"
#include "avgcalc.h"

namespace CMSat
{
using namespace CMSat;

struct VarData
{
    #ifdef STATS_NEEDED_EXTRA
    struct Stats
    {
        void addData(VarData::Stats& other)
        {
            posPolarSet += other.posPolarSet;
            negPolarSet += other.negPolarSet;
            posDecided += other.posDecided;
            negDecided += other.negDecided;
            flippedPolarity += other.flippedPolarity;

            trailLevelHist.addData(other.trailLevelHist);
            decLevelHist.addData(other.decLevelHist);
        }

        void reset()
        {
            Stats tmp;
            *this = tmp;
        }

        ///Number of times positive/negative polarity has been set
        uint32_t posPolarSet = 0;
        uint32_t negPolarSet = 0;

        ///Decided on
        uint32_t posDecided = 0;
        uint32_t negDecided = 0;

        ///Number of times polarity has been flipped
        uint32_t flippedPolarity = 0;

        ///The history of levels it was assigned
        AvgCalc<uint32_t> trailLevelHist;

        ///The history of levels it was assigned
        AvgCalc<uint32_t> decLevelHist;
    };
    Stats stats;
    #endif

    VarData() :
        level(std::numeric_limits< uint32_t >::max())
        , reason(PropBy())
        , removed(Removed::none)
        , polarity(false)
        , is_decision(true)
        , is_bva(false)
    {}

    ///contains the decision level at which the assignment was made.
    uint32_t level;

    //Used during hyper-bin and trans-reduction for speed
    uint32_t depth;

    //Reason this got propagated. NULL means decision/toplevel
    PropBy reason;

    ///Whether var has been eliminated (var-elim, different component, etc.)
    Removed removed;

    ///The preferred polarity of each variable.
    bool polarity;
    bool is_decision;
    bool is_bva;
};

}

#endif //__VARDATA_H__