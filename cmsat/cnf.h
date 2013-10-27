#ifndef __CNF_H__
#define __CNF_H__

#include "propby.h"
#include "solverconf.h"
#include "stamp.h"
#include "solvertypes.h"
#include "implcache.h"
#include "vardata.h"
#include "watcharray.h"
#include "drup.h"

namespace CMSat {
using namespace CMSat;

class ClauseAllocator;

struct CNF
{
    void newVar()
    {
        seen      .push_back(0);
        seen      .push_back(0);
        seen2     .push_back(0);
        seen2     .push_back(0);

        outerToInterMain.push_back(outerToInterMain.size());
        interToOuterMain.push_back(interToOuterMain.size());
    }

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
    {
        drup = new Drup();
    }

    ClauseAllocator* clAllocator;
    SolverConf conf;
    //If FALSE, state of CNF is UNSAT
    bool ok = true;
    watch_array watches;  ///< 'watches[lit]' is a list of constraints watching 'lit'
    vector<lbool> assigns;
    vector<VarData> varData;
    #ifdef STATS_NEEDED
    vector<VarData::Stats> varDataLT;
    #endif
    Stamp stamp;
    ImplCache implCache;
    uint32_t minNumVars;
    vector<bool> decisionVar;
    vector<ClOffset> longIrredCls;          ///< List of problem clauses that are larger than 2
    vector<ClOffset> longRedCls;          ///< List of redundant clauses.
    BinTriStats binTri;
    LitStats litStats;
    Drup* drup;
    vector<Var> outerToInterMain;
    vector<Var> interToOuterMain;

    //Temporaries
    vector<uint16_t> seen;
    vector<uint16_t> seen2;
    vector<Lit>      toClear;

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

    bool redundant(const Watched& ws) const;

    size_t print_mem_used_longclauses(size_t totalMem) const;
};

struct ClauseSizeSorter
{
    ClauseSizeSorter(const ClauseAllocator* _clAllocator) :
        clAllocator(_clAllocator)
    {}
    bool operator () (const ClOffset x, const ClOffset y);
    const ClauseAllocator* clAllocator;
};

}

#endif //__CNF_H__
