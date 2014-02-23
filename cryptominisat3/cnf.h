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
#include "clauseallocator.h"
#include "varupdatehelper.h"

namespace CMSat {
using namespace CMSat;

class ClauseAllocator;

class CNF
{
public:
    void saveVarMem();
    void updateVars(
        const vector<Var>& outerToInter
        , const vector<Var>& interToOuter
    );
    size_t get_renumber_mem() const;

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

    CNF(const SolverConf& _conf) :
        conf(_conf)
        , minNumVars(0)
    {
        drup = new Drup();
    }

    ~CNF()
    {
        delete drup;
    }

    ClauseAllocator clAllocator;
    SolverConf conf;
    //If FALSE, state of CNF is UNSAT
    bool ok = true;
    watch_array watches;  ///< 'watches[lit]' is a list of constraints watching 'lit'
    vector<lbool> assigns;
    vector<VarData> varData;
    #ifdef STATS_NEEDED
    vector<VarData> varDataLT;
    #endif
    Stamp stamp;
    ImplCache implCache;
    uint32_t minNumVars;
    vector<ClOffset> longIrredCls;          ///< List of problem clauses that are larger than 2
    vector<ClOffset> longRedCls;          ///< List of redundant clauses.
    BinTriStats binTri;
    LitStats litStats;
    Drup* drup;

    //Temporaries
    vector<uint16_t> seen;
    vector<uint16_t> seen2;
    vector<Lit>      toClear;

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
    size_t cl_size(const Watched& ws) const;
    string watched_to_string(Lit otherLit, const Watched& ws) const;
    string watches_to_string(const Lit lit, watch_subarray_const ws) const;

    size_t print_mem_used_longclauses(size_t totalMem) const;
    template<class Function>
    void for_each_lit(
        const OccurClause& cl
        ,  Function func
        , int64_t* limit
    ) const;
    template<class Function>
    void for_each_lit_except_watched(
        const OccurClause& cl
        , Function func
        , int64_t* limit
    ) const;
    void remove_tri_but_lit1(
        const Lit lit1
        , const Lit lit2
        , const Lit lit3
        , const bool red
        , int64_t& timeAvailable
    );
    Var map_inter_to_outer(const Var inter) const
    {
        return interToOuterMain[inter];
    }
    Lit map_inter_to_outer(const Lit lit) const
    {
        return Lit(interToOuterMain[lit.var()], lit.sign());
    }
    Var map_outer_to_inter(const Var outer) const
    {
        return outerToInterMain[outer];
    }
    Lit map_outer_to_inter(const Lit outer) const
    {
        return Lit(outerToInterMain[outer.var()], outer.sign());
    }
    void map_inter_to_outer(vector<Lit>& lits) const
    {
        updateLitsMap(lits, interToOuterMain);
    }

    uint32_t nVarsOutside() const
    {
        assert(outer_to_with_bva_map.size() == nVarsOuter() - num_bva_vars);
        return nVarsOuter() - num_bva_vars;
    }

    template<class T>
    Lit map_to_with_bva(const T lit) const
    {
        return Lit(outer_to_with_bva_map.at(lit.var()), lit.sign());
    }

protected:
    vector<Var> build_outer_to_without_bva_map() const;
    virtual void newVar(bool bva, Var orig_outer);
    void test_reflectivity_of_renumbering() const;
    vector<lbool> back_number_solution(const vector<lbool>& solution) const
    {
        vector<lbool> back_numbered = solution;
        updateArrayRev(back_numbered, interToOuterMain);
        return back_numbered;
    }

    uint32_t nVars() const
    {
        return minNumVars;
    }

    uint32_t nVarsOuter() const
    {
        return assigns.size();
    }

    vector<lbool> map_back_to_without_bva(const vector<lbool>& val) const;

private:
    void enlarge_minimal_datastructs();
    void enlarge_nonminimial_datastructs();
    void swapVars(const Var which);

    vector<Var> outerToInterMain;
    vector<Var> interToOuterMain;
    size_t num_bva_vars = 0;
    vector<Var> outer_to_with_bva_map;
};

template<class Function>
void CNF::for_each_lit(
    const OccurClause& cl
    ,  Function func
    , int64_t* limit
) const {
    switch(cl.ws.getType()) {
        case CMSat::watch_binary_t:
            *limit -= 2;
            func(cl.lit);
            func(cl.ws.lit2());
            break;

        case CMSat::watch_tertiary_t:
            *limit -= 3;
            func(cl.lit);
            func(cl.ws.lit2());
            func(cl.ws.lit3());
            break;

        case CMSat::watch_clause_t: {
            const Clause& clause = *clAllocator.getPointer(cl.ws.getOffset());
            *limit -= clause.size();
            for(const Lit lit: clause) {
                func(lit);
            }
            break;
        }
    }
}

template<class Function>
void CNF::for_each_lit_except_watched(
    const OccurClause& cl
    , Function func
    , int64_t* limit
) const {
    switch(cl.ws.getType()) {
        case CMSat::watch_binary_t:
            *limit -= 1;
            func(cl.ws.lit2());
            break;

        case CMSat::watch_tertiary_t:
            *limit -= 2;
            func(cl.ws.lit2());
            func(cl.ws.lit3());
            break;

        case CMSat::watch_clause_t: {
            const Clause& clause = *clAllocator.getPointer(cl.ws.getOffset());
            *limit -= clause.size();
            for(const Lit lit: clause) {
                if (lit != cl.lit) {
                    func(lit);
                }
            }
            break;
        }
    }
}

struct ClauseSizeSorter
{
    ClauseSizeSorter(const ClauseAllocator& _clAllocator) :
        clAllocator(_clAllocator)
    {}
    bool operator () (const ClOffset x, const ClOffset y);
    const ClauseAllocator& clAllocator;
};

}

#endif //__CNF_H__
