#include "solverconf.h"
#include "clauseallocator.h"
#include "stamp.h"
#include "solvertypes.h"
#include "implcache.h"
#include "vardata.h"

namespace CMSat {
using namespace CMSat;

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
    ImplCache implCache;
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
