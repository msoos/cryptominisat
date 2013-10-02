#ifndef __CNF_H__
#define __CNF_H__

#include "propby.h"
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
        uint64_t irredBins = 0;
        uint64_t redBins = 0;
        uint64_t irredTris = 0;
        uint64_t redTris = 0;
        uint64_t numNewBinsSinceSCC = 0;
    };

    struct LitStats
    {
        uint64_t irredLits = 0;
        uint64_t redLits = 0;
    };

    CNF(ClauseAllocator* _clAllocator, const SolverConf& _conf) :
        clAllocator(_clAllocator)
        , conf(_conf)
        , minNumVars(0)
    {}

    ClauseAllocator* clAllocator;
    SolverConf conf;
    //If FALSE, state of CNF is UNSAT
    bool ok = true;
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
    LitStats litStats;

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

    size_t print_mem_used_longclauses(size_t totalMem) const;
};

}

#endif //__CNF_H__
