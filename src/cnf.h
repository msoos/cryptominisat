/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***********************************************/

#ifndef __CNF_H__
#define __CNF_H__

#include <atomic>
#include <limits>

#include "constants.h"
#include "vardata.h"
#include "propby.h"
#include "solverconf.h"
#include "solvertypes.h"
#include "watcharray.h"
#include "frat.h"
#include "clauseallocator.h"
#include "varupdatehelper.h"
#include "simplefile.h"
#include "gausswatched.h"
#include "xor.h"
#ifdef USE_TBUDDY
#include <pseudoboolean.h>
#endif

namespace CMSat {

class ClauseAllocator;

struct AssumptionPair {
    AssumptionPair()
    {}

    AssumptionPair(const Lit _outer, const Lit _outside):
        lit_outer(_outer)
        , lit_orig_outside(_outside)
    {
    }

    Lit lit_outer;
    Lit lit_orig_outside; //not outer, but outside(!)

    bool operator==(const AssumptionPair& other) const {
        return other.lit_outer == lit_outer &&
        other.lit_orig_outside == lit_orig_outside;
    }

    bool operator<(const AssumptionPair& other) const
    {
        //Yes, we need reverse in terms of inverseness
        return ~lit_outer < ~other.lit_outer;
    }
};

struct BinTriStats
{
    uint64_t irredBins = 0;
    uint64_t redBins = 0;
};

struct LitStats
{
    uint64_t irredLits = 0;
    uint64_t redLits = 0;
};

class CNF
{
public:
    FastBackwData fast_backw;
    void save_on_var_memory();
    void updateWatch(watch_subarray ws, const vector<uint32_t>& outerToInter);
    void updateVars(
        const vector<uint32_t>& outerToInter
        , const vector<uint32_t>& interToOuter
        , const vector<uint32_t>& interToOuter2
    );
    size_t mem_used_renumberer() const;
    size_t mem_used() const;

    CNF(const SolverConf *_conf, std::atomic<bool>* _must_interrupt_inter)
    {
        if (_conf != NULL) {
            conf = *_conf;
        }
        frat = new Drat;
        assert(_must_interrupt_inter != NULL);
        must_interrupt_inter = _must_interrupt_inter;
        longRedCls.resize(3);
        longRedClsSizes.resize(3, 0);
    }

    virtual ~CNF()
    {
        delete frat;
    }

    ClauseAllocator cl_alloc;
    SolverConf conf;

    bool ok = true; //If FALSE, state of CNF is UNSAT

    watch_array watches;
    vec<vec<GaussWatched>> gwatches;
    uint32_t num_sls_called = 0;
    vector<VarData> varData;
    branch branch_strategy = branch::vsids;
    string branch_strategy_str = "VSIDS";
    string branch_strategy_str_short = "vs";
    PolarityMode polarity_mode = PolarityMode::polarmode_automatic; //current polarity mode
    uint32_t longest_trail_ever_stable = 0;
    uint32_t longest_trail_ever_best = 0;
    uint32_t longest_trail_ever_inv = 0;
    vector<uint32_t> depth; //for ancestors in intree probing
    uint32_t minNumVars = 0;

    uint64_t sumConflicts = 0;
    uint64_t sumDecisions = 0;
    uint64_t sumAntecedents = 0;
    uint64_t sumPropagations = 0;
    uint64_t sumConflictClauseLits = 0;
    uint64_t sumAntecedentsLits = 0;
    uint64_t sumDecisionBasedCl = 0;
    uint64_t sumClLBD = 0;
    uint64_t sumClSize = 0;

    uint32_t latest_satzilla_feature_calc = 0;
    uint64_t last_satzilla_feature_calc_confl = 0;
    uint32_t latest_vardist_feature_calc = 0;
    uint64_t last_vardist_feature_calc_confl = 0;

    unsigned  cur_max_temp_red_lev2_cls = conf.max_temp_lev2_learnt_clauses;

    //Note that this array can have the same internal variable more than
    //once, in case one has been replaced with the other. So if var 1 =  var 2
    //and var 1 was set to TRUE and var 2 to be FALSE, then we'll have var 1
    //insided this array twice, once it needs to be set to TRUE and once FALSE
    vector<AssumptionPair> assumptions;

    //frat
    Drat* frat;
    void add_frat(FILE* os);

    //Clauses
    vector<ClOffset> longIrredCls;

    //if the solver object only saw add_clause and new_var(s)
    bool fresh_solver = true;

    /**
    level 0 = never remove
    level 1 = check rarely
    level 2 = check often
    **/
    vector<vector<ClOffset> > longRedCls;
    vector<uint64_t> longRedClsSizes;
    #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
    vector<ClauseStatsExtra> red_stats_extra;
    #endif
    vector<ClOffset> detached_xor_repr_cls; //these are still in longIrredCls

    // NOTE: xorclauses and xorclauses_unused ENCODE INFORMATION THAT's NOWHERE ELSE
    //       that's right. We can discover an XOR clause then strengthen the
    //       representing clauses to a binary, and then remove the
    //       binary with e.g. irred-bin-removal, which then leads to nothing representing
    //       the XOR clause! Then FRAT fails. This ALSO means that when cleaning the XOR clauses
    //       we can actually encounter UNSAT. Since it encodes information that's nowhere else.
    vector<Xor> xorclauses; // working set. Should be empty most of the time.
    vector<Xor> xorclauses_orig; // used to re-generate the matrix
    vector<Xor> xorclauses_unused; // used to help knowing when can we detach. Only makes sense when matrixes are set up.

    vector<BNN*> bnns;
    vector<vector<Lit>> bnn_reasons;
    vector<Lit> bnn_confl_reason;
    vector<uint32_t> bnn_reasons_empty_slots;
    vector<uint32_t> removed_xorclauses_clash_vars;
    bool detached_xor_clauses = false;
    bool xor_clauses_updated = false;
    BinTriStats binTri;
    LitStats litStats;
    int32_t clauseID = 0;
    int64_t restartID = 1;
    SQLStats* sqlStats = NULL;

    //Temporaries
    vector<uint32_t> seen;
    vector<uint8_t> seen2;
    vector<uint64_t> permDiff;
    vector<Lit>      toClear;
    uint64_t MYFLAG = 1;

    uint32_t level(Lit l) const
    {
        return varData[l.var()].level;
    }

    bool okay() const
    {
        assert(!
        (!ok && frat->enabled() && unsat_cl_ID == 0 && unsat_cl_ID != -1) && "If in UNSAT state, and we have FRAT, we MUST already know the unsat_cl_ID or it must be -1, i.e. known by tbuddy");
        return ok;
    }

    lbool value (const uint32_t x) const
    {
        return assigns[x];
    }

    lbool value (const Lit p) const
    {
        return assigns[p.var()] ^ p.sign();
    }

    bool must_interrupt_asap() const
    {
        return must_interrupt_inter->load(std::memory_order_relaxed);
    }

    void set_must_interrupt_asap()
    {
        must_interrupt_inter->store(true, std::memory_order_relaxed);
    }

    void unset_must_interrupt_asap()
    {
        must_interrupt_inter->store(false, std::memory_order_relaxed);
    }

    std::atomic<bool>* get_must_interrupt_inter_asap_ptr()
    {
        return must_interrupt_inter;
    }

    const vector<BNN*>& get_bnns() const
    {
        return bnns;
    }

    bool check_bnn_sane(BNN& bnn);
    bool clause_locked(const Clause& c, const ClOffset offset) const;
    bool redundant(const Watched& ws) const;
    bool redundant_or_removed(const Watched& ws) const;
    size_t cl_size(const Watched& ws) const;
    string watched_to_string(Lit otherLit, const Watched& ws) const;
    string watches_to_string(const Lit lit, watch_subarray_const ws) const;
    bool satisfied(const ClOffset& off) const;

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
    uint32_t map_inter_to_outer(const uint32_t inter) const
    {
        return interToOuterMain[inter];
    }
    Lit map_inter_to_outer(const Lit lit) const
    {
        return Lit(interToOuterMain[lit.var()], lit.sign());
    }
    uint32_t map_outer_to_inter(const uint32_t outer) const
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

    Lit map_to_with_bva(const Lit lit) const
    {
        return Lit(outer_to_with_bva_map.at(lit.var()), lit.sign());
    }

    uint32_t map_to_with_bva(const uint32_t var) const
    {
        return outer_to_with_bva_map.at(var);
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
    vector<uint32_t> get_outside_var_incidence();
    vector<uint32_t> get_outside_lit_incidence();
    vector<uint32_t> get_outside_var_incidence_also_red();

    vector<uint32_t> build_outer_to_without_bva_map() const;
    vector<uint32_t> build_outer_to_without_bva_map_extended() const;
    void clean_occur_from_removed_clauses();
    void clean_occur_from_removed_clauses_only_smudged();
    void clean_occur_from_idx_types_only_smudged();
    void clean_occur_from_idx(const Lit lit);
    void clear_one_occur_from_removed_clauses(watch_subarray w);
    bool no_marked_clauses() const;
    void check_no_removed_or_freed_cl_in_watch() const;
    bool normClauseIsAttached(const ClOffset offset) const;
    void find_all_attach() const;
    void find_all_attach(const vector<ClOffset>& cs) const;
    bool find_clause(const ClOffset offset) const;
    void test_all_clause_attached() const;
    void test_all_clause_attached(const vector<ClOffset>& offsets) const;
    void check_wrong_attach() const;
    void check_watchlist(watch_subarray_const ws) const;
    template<class T>
    bool satisfied(const T& cl) const;
    template<typename T> bool no_duplicate_lits(const T& lits) const;
    void check_no_duplicate_lits_anywhere() const;
    void check_no_zero_ID_bins() const;
    #ifdef USE_TBUDDY
    void free_bdds(vector<Xor>& xors);
    #endif
    void print_all_clauses() const;
    void print_watchlist_stats() const;
    bool zero_irred_cls(const Lit lit) const;
    template<class T> void clean_xor_no_prop(T& ps, bool& rhs);
    template<class T> void clean_xor_vars_no_prop(T& ps, bool& rhs);
    uint64_t count_lits(
        const vector<ClOffset>& clause_array
        , const bool red
        , const bool allowFreed
    ) const;

    /** if set to TRUE, a clause has been removed during add_clause_int
    that contained "lit, ~lit". So "lit" must be set to a value
    Contains OUTER variables */
    vector<bool> undef_must_set_vars;
    vector<int32_t> unit_cl_IDs;
    int32_t unsat_cl_ID = 0;

protected:
    virtual void new_var(
        const bool bva,
        const uint32_t orig_outer,
        const bool insert_varorder = true);
    virtual void new_vars(const size_t n);
    void test_reflectivity_of_renumbering() const;

    template<class T>
    vector<T> map_back_vars_to_without_bva(const vector<T>& val) const;
    template<class T>
    vector<T> map_back_lits_to_without_bva(const vector<T>& val) const;
    vector<lbool> assigns;

    vector<uint32_t> outerToInterMain;
    vector<uint32_t> interToOuterMain;

private:
    std::atomic<bool> *must_interrupt_inter; ///<Interrupt cleanly ASAP if true
    void enlarge_minimal_datastructs(size_t n = 1);
    void enlarge_nonminimial_datastructs(size_t n = 1);
    void swapVars(const uint32_t which, const int off_by = 0);

    size_t num_bva_vars = 0;
    vector<uint32_t> outer_to_with_bva_map;
};

template<class Function>
void CNF::for_each_lit(
    const OccurClause& cl
    ,  Function func
    , int64_t* limit
) const {
    switch(cl.ws.getType()) {
        case WatchType::watch_binary_t:
            *limit -= 2;
            func(cl.lit);
            func(cl.ws.lit2());
            break;

        case WatchType::watch_clause_t: {
            const Clause& clause = *cl_alloc.ptr(cl.ws.get_offset());
            *limit -= (int64_t)clause.size();
            for(const Lit lit: clause) {
                func(lit);
            }
            break;
        }

        case WatchType::watch_bnn_t :
        case WatchType::watch_idx_t :
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
        case WatchType::watch_binary_t:
            *limit -= 1;
            func(cl.ws.lit2());
            break;

        case WatchType::watch_clause_t: {
            const Clause& clause = *cl_alloc.ptr(cl.ws.get_offset());
            *limit -= clause.size();
            for(const Lit lit: clause) {
                if (lit != cl.lit) {
                    func(lit);
                }
            }
            break;
        }

        case WatchType::watch_bnn_t: //no idea what to do with this, let's error
        case WatchType::watch_idx_t:
            assert(false);
            break;
    }
}

struct ClauseSizeSorter
{
    explicit ClauseSizeSorter(const ClauseAllocator& _cl_alloc) :
        cl_alloc(_cl_alloc)
    {}
    bool operator () (const ClOffset x, const ClOffset y);
    const ClauseAllocator& cl_alloc;
};

inline bool CNF::redundant(const Watched& ws) const
{
    return (   (ws.isBin() && ws.red())
            || (ws.isClause() && cl_alloc.ptr(ws.get_offset())->red())
    );
}

inline bool CNF::redundant_or_removed(const Watched& ws) const
{
    if (ws.isBin()) {
        return ws.red();
    }

   assert(ws.isClause());
   const Clause* cl = cl_alloc.ptr(ws.get_offset());
   return cl->red() || cl->getRemoved();
}

inline void CNF::clean_occur_from_removed_clauses()
{
    for(watch_subarray w: watches) {
        clear_one_occur_from_removed_clauses(w);
    }
}

inline void CNF::clean_occur_from_removed_clauses_only_smudged()
{
    for(const Lit l: watches.get_smudged_list()) {
        clear_one_occur_from_removed_clauses(watches[l]);
    }
    watches.clear_smudged();

    #ifdef SLOW_DEBUG
    for(const auto& ws: watches) {
        for(const auto& w: ws) {
            if (!w.isClause()) {
                continue;
            }
            Clause* cl = cl_alloc.ptr(w.get_offset());
            assert(!cl->isRemoved);
        }
    }
    #endif
}

inline void CNF::clean_occur_from_idx_types_only_smudged()
{
    for(const Lit lit: watches.get_smudged_list()) {
        clean_occur_from_idx(lit);
    }
    watches.clear_smudged();
}

inline void CNF::clean_occur_from_idx(const Lit lit)
{
    watch_subarray ws = watches[lit];
    Watched* i = ws.begin();
    Watched* j = ws.begin();
    for(const Watched* end = ws.end(); i < end; i++) {
        if (!i->isIdx()) {
            *j++ = *i;
        }
    }
    ws.shrink(i-j);
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
         if (ws.isBNN()) {
            BNN* bnn = bnns[ws.get_bnn()];
            if (!bnn->isRemoved) {
                w[j++] = w[i];
            }
            continue;
        }

        if (ws.isBin()) {
            w[j++] = w[i];
            continue;
        }

        assert(ws.isClause());
        Clause* cl = cl_alloc.ptr(ws.get_offset());
        if (!cl->getRemoved()) {
            w[j++] = w[i];
        }
    }
    w.shrink(i-j);
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

template<typename T>
inline vector<Lit> unsign_lits(const T& lits)
{
    vector<Lit> ret(lits.size());
    for(size_t i = 0; i < lits.size(); i++) {
        ret[i] = lits[i].unsign();
    }
    return ret;
}

inline void CNF::check_no_removed_or_freed_cl_in_watch() const
{
    for(watch_subarray_const ws: watches) {
        for(const Watched& w: ws) {
            assert(!w.isIdx());
            if (w.isBin()) {
                continue;
            }
            assert(w.isClause());
            Clause& cl = *cl_alloc.ptr(w.get_offset());
            assert(!cl.getRemoved());
            assert(!cl.freed());
        }
    }
}

template<class T>
bool CNF::satisfied(const T& cl) const {
    for(Lit lit: cl) {
        if (value(lit) == l_True) {
            return true;
        }
    }
    return false;
}


template<typename T>
bool CNF::no_duplicate_lits(const T& lits) const
{
    vector<Lit> x(lits.size());
    for(size_t i = 0; i < x.size(); i++) {
        x[i] = lits[i];
    }
    std::sort(x.begin(), x.end());
    for(size_t i = 1; i < x.size(); i++) {
        if (x[i-1] == x[i])
            return false;
    }
    return true;
}

inline void CNF::check_no_duplicate_lits_anywhere() const
{
    for(const ClOffset offs: longIrredCls) {
        Clause * cl = cl_alloc.ptr(offs);
        assert(no_duplicate_lits((*cl)));
    }
    for(const auto& l: longRedCls) {
        for(const ClOffset offs: l) {
            Clause * cl = cl_alloc.ptr(offs);
            assert(no_duplicate_lits((*cl)));
        }
    }
}

template<class T>
void CNF::clean_xor_no_prop(T& ps, bool& rhs)
{
    std::sort(ps.begin(), ps.end());
    Lit p;
    uint32_t i, j;
    for (i = j = 0, p = lit_Undef; i != ps.size(); i++) {
        assert(ps[i].sign() == false);

        if (ps[i].var() == p.var()) {
            //added, but easily removed
            j--;
            p = lit_Undef;

            //Flip rhs if neccessary
            if (value(ps[i]) != l_Undef) {
                rhs ^= value(ps[i]) == l_True;
            }

        } else if (value(ps[i]) == l_Undef) {
            //Add and remember as last one to have been added
            ps[j++] = p = ps[i];

            assert(varData[p.var()].removed != Removed::elimed);
        } else {
            //modify rhs instead of adding
            rhs ^= value(ps[i]) == l_True;
        }
    }
    ps.resize(ps.size() - (i - j));
}

template<class T>
void CNF::clean_xor_vars_no_prop(T& ps, bool& rhs)
{
    std::sort(ps.begin(), ps.end());
    uint32_t p;
    uint32_t i, j;
    for (i = j = 0, p = numeric_limits<uint32_t>::max(); i != ps.size(); i++) {
        if (ps[i] == p) {
            //added, but easily removed
            j--;
            p = numeric_limits<uint32_t>::max();

            //Flip rhs if neccessary
            if (value(ps[i]) != l_Undef) {
                rhs ^= value(ps[i]) == l_True;
            }

        } else if (value(ps[i]) == l_Undef) {
            //Add and remember as last one to have been added
            ps[j++] = p = ps[i];

            assert(varData[p].removed != Removed::elimed);
        } else {
            //modify rhs instead of adding
            rhs ^= value(ps[i]) == l_True;
        }
    }
    if ((i - j) > 0) {
        ps.resize(ps.size() - (i - j));
        //TODO tbdd ?
    }
}

template<class T>
vector<T> CNF::map_back_lits_to_without_bva(const vector<T>& val) const
{
    vector<T> ret;
    assert(val.size() == nVarsOuter()*2);
    ret.reserve(nVarsOutside()*2);
    for(size_t i = 0; i < nVarsOuter()*2; i++) {
        Lit l = Lit::toLit(i);
        if (!varData[map_outer_to_inter(l).var()].is_bva) {
            ret.push_back(val[i]);
        }
    }
    assert(ret.size() == nVarsOutside()*2);
    return ret;
}


template<class T>
vector<T> CNF::map_back_vars_to_without_bva(const vector<T>& val) const
{
    vector<T> ret;
    assert(val.size() == nVarsOuter());
    ret.reserve(nVarsOutside());
    for(size_t i = 0; i < nVarsOuter(); i++) {
        if (!varData[map_outer_to_inter(i)].is_bva) {
            ret.push_back(val[i]);
        }
    }
    assert(ret.size() == nVarsOutside());
    return ret;
}

inline bool CNF::satisfied(const ClOffset& off) const
{
    Clause* cl = cl_alloc.ptr(off);
    return satisfied(*cl);
}

}

#endif //__CNF_H__
