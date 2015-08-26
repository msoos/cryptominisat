/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.0 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/

#ifndef __CNF_H__
#define __CNF_H__

#include "constants.h"
#include "vardata.h"
#include "propby.h"
#include "solverconf.h"
#include "stamp.h"
#include "solvertypes.h"
#include "implcache.h"
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
    void save_on_var_memory();
    void updateVars(
        const vector<Var>& outerToInter
        , const vector<Var>& interToOuter
    );
    size_t mem_used_renumberer() const;
    size_t mem_used() const;

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

    CNF(const SolverConf *_conf, bool* _needToInterrupt) :
        minNumVars(0)
    {
        if (_conf != NULL) {
            conf = *_conf;
        }
        drup = new Drup();
        assert(_needToInterrupt != NULL);
        needToInterrupt = _needToInterrupt;
    }

    virtual ~CNF()
    {
        delete drup;
    }

    ClauseAllocator cl_alloc;
    SolverConf conf;
    //If FALSE, state of CNF is UNSAT
    bool ok = true;
    watch_array watches;  ///< 'watches[lit]' is a list of constraints watching 'lit'
    vector<VarData> varData;
    #ifdef STATS_NEEDED
    vector<VarData> varDataLT;
    #endif
    Stamp stamp;
    ImplCache implCache;
    uint32_t minNumVars;
    vector<ClOffset> longIrredCls;          ///< List of problem clauses that are larger than 2
    int64_t num_red_cls_reducedb = 0;
    bool red_long_cls_is_reducedb(const Clause& cl) const;
    int64_t count_num_red_cls_reducedb() const;

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

    bool must_interrupt_asap() const
    {
        return *needToInterrupt;
    }

    void set_must_interrupt_asap()
    {
        *needToInterrupt = true;
    }

    void unset_must_interrupt_asap()
    {
        *needToInterrupt = false;
    }

    bool* get_must_interrupt_asap_ptr()
    {
        return needToInterrupt;
    }

    bool clause_locked(const Clause& c, const ClOffset offset) const;
    void unmark_all_irred_clauses();
    void unmark_all_red_clauses();

    bool redundant(const Watched& ws) const;
    bool redundant_or_removed(const Watched& ws) const;
    size_t cl_size(const Watched& ws) const;
    string watched_to_string(Lit otherLit, const Watched& ws) const;
    string watches_to_string(const Lit lit, watch_subarray_const ws) const;

    uint64_t print_mem_used_longclauses(size_t totalMem) const;
    uint64_t mem_used_longclauses() const;
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
    void renumber_outer_to_inter_lits(vector<Lit>& ps) const;

    uint32_t nVarsOutside() const
    {
        #ifdef DEBUG_SLOW
        assert(outer_to_with_bva_map.size() == nVarsOuter() - num_bva_vars);
        #endif
        return nVarsOuter() - num_bva_vars;
    }

    template<class T>
    Lit map_to_with_bva(const T lit) const
    {
        return Lit(outer_to_with_bva_map.at(lit.var()), lit.sign());
    }

    size_t nVars() const
    {
        return minNumVars;
    }

    size_t nVarsOuter() const
    {
        return assigns.size();
    }

    size_t get_num_bva_vars() const
    {
        return num_bva_vars;
    }

    vector<Var> build_outer_to_without_bva_map() const;
    void clean_occur_from_removed_clauses();
    void clean_occur_from_removed_clauses_only_smudged();
    void clean_occur_from_idx_types_only_smudged();
    void clear_one_occur_from_removed_clauses(watch_subarray w);
    bool no_marked_clauses() const;
    void check_no_removed_or_freed_cl_in_watch() const;

protected:
    virtual void new_var(const bool bva, const Var orig_outer);
    virtual void new_vars(const size_t n);
    void test_reflectivity_of_renumbering() const;
    vector<lbool> back_number_solution_from_inter_to_outer(const vector<lbool>& solution) const
    {
        vector<lbool> back_numbered = solution;
        updateArrayRev(back_numbered, interToOuterMain);
        return back_numbered;
    }

    vector<lbool> map_back_to_without_bva(const vector<lbool>& val) const;
    vector<lbool> assigns;

private:
    bool *needToInterrupt; ///<Interrupt cleanly ASAP if true
    void enlarge_minimal_datastructs(size_t n = 1);
    void enlarge_nonminimial_datastructs(size_t n = 1);
    void swapVars(const Var which, const int off_by = 0);

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
            const Clause& clause = *cl_alloc.ptr(cl.ws.get_offset());
            *limit -= clause.size();
            for(const Lit lit: clause) {
                func(lit);
            }
            break;
        }

        case watch_idx_t :
            assert(false);
            break;
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
            const Clause& clause = *cl_alloc.ptr(cl.ws.get_offset());
            *limit -= clause.size();
            for(const Lit lit: clause) {
                if (lit != cl.lit) {
                    func(lit);
                }
            }
            break;
        }

        case CMSat::watch_idx_t:
            assert(false);
            break;
    }
}

struct ClauseSizeSorter
{
    ClauseSizeSorter(const ClauseAllocator& _cl_alloc) :
        cl_alloc(_cl_alloc)
    {}
    bool operator () (const ClOffset x, const ClOffset y);
    const ClauseAllocator& cl_alloc;
};

inline void CNF::clean_occur_from_removed_clauses()
{
    for(watch_subarray w: watches) {
        clear_one_occur_from_removed_clauses(w);
    }
}

inline void CNF::clean_occur_from_removed_clauses_only_smudged()
{
    for(const Lit l: watches.get_smudged_list()) {
        clear_one_occur_from_removed_clauses(watches[l.toInt()]);
    }
    watches.clear_smudged();
}

inline bool CNF::no_marked_clauses() const
{
    for(ClOffset offset: longIrredCls) {
        Clause* cl = cl_alloc.ptr(offset);
        if (cl->stats.marked_clause) {
            return false;
        }
    }

    for(ClOffset offset: longRedCls) {
        Clause* cl = cl_alloc.ptr(offset);
        if (cl->stats.marked_clause) {
            return false;
        }
    }

    return true;
}

inline void CNF::clean_occur_from_idx_types_only_smudged()
{
    for(const Lit lit: watches.get_smudged_list()) {
        watch_subarray ws = watches[lit.toInt()];
        watch_subarray::iterator i = ws.begin();
        watch_subarray::iterator j = ws.begin();
        for(watch_subarray::const_iterator end = ws.end(); i < end; i++) {
            if (!i->isIdx()) {
                *j++ = *i;
            }
        }
        ws.shrink(i-j);
    }
    watches.clear_smudged();
}

inline bool CNF::clause_locked(const Clause& c, const ClOffset offset) const
{
    return value(c[0]) == l_True
        && varData[c[0].var()].reason.isClause()
        && varData[c[0].var()].reason.get_offset() == offset;
}

inline void CNF::clear_one_occur_from_removed_clauses(watch_subarray w)
{
    size_t i = 0;
    size_t j = 0;
    size_t end = w.size();
    for(; i < end; i++) {
        const Watched ws = w[i];
        if (!ws.isClause()) {
            w[j++] = w[i];
            continue;
        }

        Clause* cl = cl_alloc.ptr(ws.get_offset());
        if (!cl->getRemoved()) {
            w[j++] = w[i];
        }
    }
    w.shrink(i-j);
}

inline void CNF::unmark_all_irred_clauses()
{
    for(ClOffset offset: longIrredCls) {
        Clause* cl = cl_alloc.ptr(offset);
        cl->stats.marked_clause = false;
    }
}

inline void CNF::unmark_all_red_clauses()
{
    for(ClOffset offset: longRedCls) {
        Clause* cl = cl_alloc.ptr(offset);
        cl->stats.marked_clause = false;
    }
}

inline void CNF::renumber_outer_to_inter_lits(vector<Lit>& ps) const
{
    for (Lit& lit: ps) {
        const Lit origLit = lit;

        //Update variable numbering
        assert(lit.var() < nVarsOuter());
        lit = map_outer_to_inter(lit);

        if (conf.verbosity >= 52) {
            cout
            << "var-renumber updating lit "
            << origLit
            << " to lit "
            << lit
            << endl;
        }
    }
}

inline bool CNF::red_long_cls_is_reducedb(const Clause& cl) const
{
    assert(cl.red());
    return cl.stats.glue > conf.glue_must_keep_clause_if_below_or_eq && !cl.stats.locked && cl.stats.ttl == 0;
}

inline int64_t CNF::count_num_red_cls_reducedb() const
{
    int64_t num = 0;
    for(ClOffset offset: longRedCls) {
         Clause& cl = *cl_alloc.ptr(offset);
         if (red_long_cls_is_reducedb(cl)) {
             num++;
         }
    }
    return num;
}

inline void CNF::check_no_removed_or_freed_cl_in_watch() const
{
    for(const watch_subarray_const ws: watches) {
        for(const Watched& w: ws) {
            assert(!w.isIdx());
            if (w.isBinary() || w.isTri()) {
                continue;
            }
            assert(w.isClause());
            Clause& cl = *cl_alloc.ptr(w.get_offset());
            assert(!cl.getRemoved());
            assert(!cl.freed());
        }
    }
}

}

#endif //__CNF_H__
