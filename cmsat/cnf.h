#include "solverconf.h"
#include "clauseallocator.h"
#include "stamp.h"
#include "solvertypes.h"

namespace CMSat {
using namespace CMSat;

struct VarData
{
    struct Stats
    {
        Stats() :
            posPolarSet(0)
            , negPolarSet(0)
            #ifdef STATS_NEEDED
            , posDecided(0)
            , negDecided(0)
            , flippedPolarity(0)
            #endif
        {}

        void addData(VarData::Stats& other)
        {
            posPolarSet += other.posPolarSet;
            negPolarSet += other.negPolarSet;
            #ifdef STATS_NEEDED
            posDecided += other.posDecided;
            negDecided += other.negDecided;
            flippedPolarity += other.flippedPolarity;

            trailLevelHist.addData(other.trailLevelHist);
            decLevelHist.addData(other.decLevelHist);
            #endif
        }

        void reset()
        {
            Stats tmp;
            *this = tmp;
        }

        ///Number of times positive/negative polarity has been set
        uint32_t posPolarSet;
        uint32_t negPolarSet;

        #ifdef STATS_NEEDED
        ///Decided on
        uint32_t posDecided;
        uint32_t negDecided;

        ///Number of times polarity has been flipped
        uint32_t flippedPolarity;

        ///The history of levels it was assigned
        AvgCalc<uint32_t> trailLevelHist;

        ///The history of levels it was assigned
        AvgCalc<uint32_t> decLevelHist;
        #endif
    };

    VarData() :
        level(std::numeric_limits< uint32_t >::max())
        , reason(PropBy())
        , removed(Removed::none)
        , polarity(false)
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

    #ifdef STATS_NEEDED
    Stats stats;
    #endif
};

struct CNF
{
    struct BinTriStats
    {
        BinTriStats() :
            irredLits(0)
            , redLits(0)
            , irredBins(0)
            , redBins(0)
            , irredTris(0)
            , redTris(0)
            , numNewBinsSinceSCC(0)
        {};

        uint64_t irredLits;  ///< Number of literals in irred clauses
        uint64_t redLits;  ///< Number of literals in redundant clauses
        uint64_t irredBins;
        uint64_t redBins;
        uint64_t irredTris;
        uint64_t redTris;
        uint64_t numNewBinsSinceSCC;
    };

    CNF(ClauseAllocator* _clAllocator) :
        clAllocator(_clAllocator)
        , ok(true)
        , minNumVars(0)
    {}

    ClauseAllocator* clAllocator;
    //If FALSE, state of CNF is UNSAT
    bool ok;
    vector<vec<Watched> > watches;  ///< 'watches[lit]' is a list of constraints watching 'lit'
    vector<lbool> assigns;
    vector<VarData> varData;
    #ifdef STATS_NEEDED
    vector<VarData::Stats> varDataLT;
    #endif
    Stamp stamp;
    uint32_t minNumVars;
    vector<char> decisionVar;
    vector<ClOffset> longIrredCls;          ///< List of problem clauses that are larger than 2
    vector<ClOffset> longRedCls;          ///< List of redundant clauses.
    BinTriStats binTri;

    uint32_t nVars() const
    {
        return minNumVars;
    }

    uint32_t nVarsReal() const
    {
        return assigns.size();
    }

    bool okay() const
    {
        return ok;
    }

    lbool value (const Var x) const
    {
        return assigns[x];
    }

    lbool value (const Lit p) const
    {
        return assigns[p.var()] ^ p.sign();
    }

};

}
