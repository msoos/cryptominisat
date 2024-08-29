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

#pragma once

#include <atomic>
#include <random>
#include <gmpxx.h>

#include "constants.h"
#include "solvertypesmini.h"
#include "vardata.h"
#include "propby.h"
#include "solverconf.h"
#include "solvertypes.h"
#include "watcharray.h"
#include "frat.h"
#include "idrup.h"
#include "clauseallocator.h"
#include "varupdatehelper.h"
#include "gausswatched.h"
#include "xor.h"

namespace CMSat {

class ClauseAllocator;

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
    void update_watch(watch_subarray ws, const vector<uint32_t>& outer_to_inter);
    void update_vars(
        const vector<uint32_t>& outer_to_inter
        , const vector<uint32_t>& inter_to_outer
        , const vector<uint32_t>& inter_to_outer2
    );
    size_t mem_used_renumberer() const;
    size_t mem_used() const;
    bool get_weighted() const { return weighted; }
    void set_weighted(const bool w) { weighted = w; }
    void set_multiplier_weight(const mpz_class mult) { multiplier_weight = mult; }
    mpz_class get_multiplier_weight() const { return multiplier_weight; }

    CNF(const SolverConf *_conf, std::atomic<bool>* _must_interrupt_inter)
    {
        if (_conf != nullptr) conf = *_conf;
        mtrand.seed(conf.origSeed);
        frat = new Frat;
        assert(_must_interrupt_inter != nullptr);
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
    std::mt19937_64 mtrand;

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
    vector<Lit> assumptions;

    //frat
    Frat* frat;
    void add_frat(FILE* os);
    void add_idrup(FILE* os);

    //Clauses
    vector<ClOffset> longIrredCls;

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

    // xorclauses -- current, attached XORs. Some XORs may be XOR'ed together
    //               however, they are never in a matrix.
    // xorclause_orig -- definitive XORs, not XOR'ed together, never attached.
    //
    // NOTE: XORs that are currently in matrixes are not in xorclauses.
    vector<Xor> xorclauses;
    void print_xors(const vector<Xor>& xors);

    // variables that have been removed due to them being ONLY in XORs
    // that have beeen XOR-ed together and hence the variable is no longer
    // part of the CNF
    bool xorclauses_updated = false;

    vector<BNN*> bnns;
    vector<vector<Lit>> bnn_reasons;
    vector<Lit> bnn_confl_reason;
    vector<uint32_t> bnn_reasons_empty_slots;
    BinTriStats binTri;
    LitStats litStats;
    int32_t clauseID = 0;
    int32_t clauseXID = 0;
    int64_t restartID = 1;
    SQLStats* sqlStats = nullptr;
    bool weighted = false;
    mpz_class multiplier_weight = 1;

    //Temporaries
    vector<uint32_t> seen;
    vector<uint8_t> seen2;
    vector<uint64_t> permDiff;
    vector<Lit>      toClear;
    uint64_t MYFLAG = 1;

    bool okay() const {
        assert(!(!ok && frat->enabled() && unsat_cl_ID == 0 && unsat_cl_ID != -1) &&
               "If in UNSAT state, and we have FRAT, we MUST already know the unsat_cl_ID");
        return ok;
    }
    auto level(Lit l) const { return varData[l.var()].level; }
    lbool value (const uint32_t x) const { return assigns[x]; }
    lbool value (const Lit p) const { return assigns[p.var()] ^ p.sign(); }
    bool must_interrupt_asap() const { return must_interrupt_inter->load(std::memory_order_relaxed); }
    void set_must_interrupt_asap() { must_interrupt_inter->store(true, std::memory_order_relaxed); }
    void unset_must_interrupt_asap() { must_interrupt_inter->store(false, std::memory_order_relaxed); }
    std::atomic<bool>* get_must_interrupt_inter_asap_ptr() { return must_interrupt_inter; }
    const vector<BNN*>& get_bnns() const { return bnns; }

    bool check_bnn_sane(BNN& bnn);
    bool clause_locked(const Clause& c, const ClOffset offset) const;
    bool redundant(const Watched& ws) const;
    bool redundant_or_removed(const Watched& ws) const;
    size_t cl_size(const Watched& ws) const;
    string watched_to_string(Lit other_lit, const Watched& ws) const;
    string watches_to_string(const Lit lit, watch_subarray_const ws) const;
    bool satisfied(const ClOffset& off) const;

    uint64_t print_mem_used_longclauses(size_t total_mem) const;
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
        return inter_to_outerMain[inter];
    }
    Lit map_inter_to_outer(const Lit lit) const
    {
        return Lit(inter_to_outerMain[lit.var()], lit.sign());
    }
    uint32_t map_outer_to_inter(const uint32_t outer) const
    {
        return outer_to_interMain[outer];
    }
    void map_outer_to_inter(vector<uint32_t>& outer) const
    {
        for(auto& v: outer) v = outer_to_interMain[v];
    }
    void map_inter_to_outer(vector<uint32_t>& inter) const
    {
        for(auto& v: inter) v = inter_to_outerMain[v];
    }
    Lit map_outer_to_inter(const Lit outer) const
    {
        return Lit(outer_to_interMain[outer.var()], outer.sign());
    }
    void map_inter_to_outer(vector<Lit>& lits) const
    {
        updateLitsMap(lits, inter_to_outerMain);
    }
    void renumber_outer_to_inter_lits(vector<Lit>& ps) const;



    size_t get_num_bva_vars() const { return num_bva_vars; }
    size_t nVars() const { return minNumVars; }
    size_t nVarsOuter() const { return assigns.size(); }
    vector<uint32_t> get_outside_var_incidence();
    vector<uint32_t> get_outside_lit_incidence();
    vector<uint32_t> get_outside_var_incidence_also_red();

    void clean_occur_from_removed_clauses();
    void clean_occur_from_removed_clauses_only_smudged();
    void clean_occur_from_idx_types_only_smudged();
    void clean_occur_from_idx(const Lit lit);
    void clear_one_occur_from_removed_clauses(watch_subarray w);
    bool no_marked_clauses() const;
    bool norm_clause_is_attached(const ClOffset offset) const;
    void find_all_attached() const;
    void find_all_attached(const vector<ClOffset>& cs) const;
    bool find_clause(const ClOffset offset) const;
    void check_no_idx_in_watchlist() const;
    void check_no_removed_or_freed_cl_in_watch() const;
    void check_all_xorclause_attached() const;
    void check_all_clause_attached() const;
    void check_all_clause_attached(const vector<ClOffset>& offsets) const;
    bool check_xor_attached(const Xor& x, const uint32_t i) const;
    void check_wrong_attach() const;
    int32_t clean_xor_vars_no_prop(vector<Lit>& ps, bool& rhs, int32_t XID);
    inline void clean_xor_vars_no_prop(Xor& x);
    void check_watchlist(watch_subarray_const ws) const;
    template<class T> bool satisfied(const T& cl) const;
    template<typename T> bool no_duplicate_lits(const T& lits) const;
    void check_no_duplicate_lits_anywhere() const;
    void check_no_zero_ID_bins() const;

#ifdef ARJUN_SERIALIZE
    template<class T> void unserialize(T& ar);
    template<class T> void serialize(T& ar) const;
#endif
    size_t get_num_long_cls() const;
    size_t get_num_long_irred_cls() const;
    size_t get_num_long_red_cls() const;
    void print_all_clauses() const;
    void print_watchlist_stats() const;
    bool zero_irred_cls(const Lit lit) const;
    template<class T> void clean_xor_no_prop(T& ps, bool& rhs);
    uint64_t count_lits(
        const vector<ClOffset>& clause_array
        , const bool red
        , const bool allow_freed
    ) const;

    /** if set to TRUE, a clause has been removed during add_clause_int
    that contained "lit, ~lit". So "lit" must be set to a value
    Contains OUTER variables */
    vector<bool> undef_must_set_vars;
    vector<int32_t> unit_cl_IDs;
    vector<int32_t> unit_cl_XIDs;
    int32_t unsat_cl_ID = 0;
    void add_chain();
    vector<int32_t> chain; ///< For resolution chains

protected:
    virtual void new_var(
        const bool bva,
        const uint32_t orig_outer,
        const bool insert_varorder = true);
    virtual void new_vars(const size_t n);
    void test_reflectivity_of_renumbering() const;
    vector<lbool> assigns;

    vector<uint32_t> outer_to_interMain;
    vector<uint32_t> inter_to_outerMain;

private:
    std::atomic<bool> *must_interrupt_inter; ///<Interrupt cleanly ASAP if true
    void enlarge_minimal_datastructs(size_t n = 1);
    void enlarge_nonminimial_datastructs(size_t n = 1);
    void swapVars(const uint32_t which, const int off_by = 0);
    size_t num_bva_vars = 0;
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
    return ((ws.isBin() && ws.red())
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
   return cl->red() || cl->get_removed();
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
    SLOW_DEBUG_DO( check_no_removed_or_freed_cl_in_watch());
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
        if (!cl->get_removed()) {
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
            assert(!cl.get_removed());
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

template<class T> void CNF::clean_xor_no_prop(T& ps, bool& rhs) {
    std::sort(ps.begin(), ps.end());
    Lit p;
    uint32_t i, j;
    for (i = j = 0, p = lit_Undef; i != ps.size(); i++) {
        assert(ps[i].sign() == false);

        if (ps[i].var() == p.var()) {
            //same twice in XOR, removing
            j--;
            p = lit_Undef;
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

inline bool CNF::satisfied(const ClOffset& off) const
{
    Clause* cl = cl_alloc.ptr(off);
    return satisfied(*cl);
}

inline size_t CNF::get_num_long_irred_cls() const { return longIrredCls.size(); }
inline size_t CNF::get_num_long_red_cls() const { return longRedCls.size(); }
inline size_t CNF::get_num_long_cls() const { return longIrredCls.size() + longRedCls.size(); }

inline void CNF::clean_xor_vars_no_prop(Xor& x) {
    frat_func_start_raw();
    if (x.trivial()) {
        assert(x.XID == 0);
        frat_func_end_raw();
        return;
    }
    *frat << deldelayx << x << fin;
    std::sort(x.vars.begin(), x.vars.end());
    uint32_t p;
    uint32_t i, j;
    chain = {x.XID};
    for (i = j = 0, p = var_Undef; i != x.vars.size(); i++) {
        if (x[i] == p) {
            //same twice in XOR, removing
            j--;
            p = var_Undef;
        } else if (value(x[i]) == l_Undef) {
            //Add and remember as last one to have been added
            x[j++] = p = x[i];
            assert(varData[p].removed != Removed::elimed);
        } else {
            //modify rhs instead of adding
            chain.push_back(unit_cl_XIDs[x[i]]);
            x.rhs ^= value(x[i]) == l_True;
            p = var_Undef;
        }
    }

    if (j < x.size()) {
        x.resize(j);
        if (j == 0 && x.rhs == false) {
            // in case it's trivial, we set XID 0
            *frat << findelay;
            x.XID = 0;
        } else {
            x.XID = ++clauseXID;
            if (frat->enabled()) { *frat << addx << x; add_chain(); *frat << fin << findelay;}
        }
    }
    frat->forget_delay();
    frat_func_end_raw();
}

inline int32_t CNF::clean_xor_vars_no_prop(vector<Lit>& ps, bool& rhs, int32_t XID) {
    frat_func_start_raw();
    if (!ps.empty()) ps[0] ^= !rhs;
    *frat << deldelayx << XID << ps << fin;
    if (!ps.empty()) ps[0] ^= !rhs;

    std::sort(ps.begin(), ps.end());
    Lit p;
    uint32_t i, j;
    chain = {XID};
    for (i = j = 0, p = lit_Undef; i != ps.size(); i++) {
        if (ps[i].var() == p.var()) {
            //same twice in XOR, removing
            assert(p.sign() == false); // we clean sign it below
            rhs ^= ps[i].sign() ^ p.sign();
            j--;
            p = lit_Undef;
        } else if (value(ps[i]) == l_Undef) {
            //Add and remember as last one to have been added
            rhs ^= ps[i].sign();
            ps[i] = ps[i].unsign();
            ps[j++] = p = ps[i];
            assert(varData[p.var()].removed != Removed::elimed);
        } else {
            //modify rhs instead of adding
            chain.push_back(unit_cl_XIDs[ps[i].var()]);
            rhs ^= value(ps[i]) == l_True;
            p = lit_Undef;
        }
    }

    if (j < ps.size()) {
        if (j == 0 && rhs == false) {
            ps.resize(0);
            // in case it's trivial, we delete & return 0
            *frat << findelay;
            frat_func_end_raw();
            return 0;
        }
        ps.resize(j);
        if (j > 0) ps[0] ^= !rhs;
        const auto XID2 = ++clauseXID;
        if (frat->enabled()) { *frat << addx << XID2 << ps; add_chain(); *frat << fin << findelay;}
        if (j > 0) ps[0] ^= !rhs;
        frat_func_end_raw();
        return XID2;
    }
    frat->forget_delay();
    frat_func_end_raw();
    return XID;
}

#ifdef ARJUN_SERIALIZE
template<class T> void CNF::unserialize(T& ar)
{
    ar >> outer_to_with_bva_map;
    ar >> num_bva_vars;
}

template<class T> void CNF::serialize(T& ar) const
{
    ar << outer_to_with_bva_map;
    ar << num_bva_vars;
}
#endif

}
