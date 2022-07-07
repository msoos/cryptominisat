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

#ifndef __PROPENGINE_H__
#define __PROPENGINE_H__

// #define VERBOSE_DEBUG
#include <cstdio>
#include <string.h>
#include <stack>
#include <set>
#include <cmath>

#include "constants.h"
#include "propby.h"
#include "vmtf.h"

#include "avgcalc.h"
#include "propby.h"
#include "heap.h"
#include "alg.h"
#include "clause.h"
#include "boundedqueue.h"
#include "cnf.h"
#include "watchalgos.h"
#include "gqueuedata.h"

namespace CMSat {

using std::set;
class Solver;
class SQLStats;
class DataSync;

//#define VERBOSE_DEBUG_FULLPROP
//#define VERBOSE_DEBUG

#ifdef VERBOSE_DEBUG
#define VERBOSE_DEBUG_FULLPROP
#define ENQUEUE_DEBUG
#define DEBUG_ENQUEUE_LEVEL0
#endif

class Solver;
class ClauseAllocator;
class Gaussian;
class EGaussian;

enum PropResult {
    PROP_FAIL = 0
    , PROP_NOTHING = 1
    , PROP_SOMETHING = 2
    , PROP_TODO = 3
};

struct Trail {

    Trail () {
    }

    Trail (Lit _lit, uint32_t _lev) :
        lit(_lit)
        , lev(_lev)
    {}

    Lit lit;
    uint32_t lev;
};


struct RandHeap
{
    vector<unsigned char> in_heap;
    vector<uint32_t> vars;

    bool inHeap(uint32_t x) const {
        if (in_heap.size() <= x) {
            return false;
        }
        return in_heap[x];
    }

    void clear() {
        in_heap.clear();
        vars.clear();
    }

    void insert(uint32_t x) {
        assert(!inHeap(x));
        if (in_heap.size() <= x) {
            uint32_t n = x - in_heap.size() + 1;
            in_heap.insert(in_heap.end(), n, false);
        }
        in_heap[x] = true;
        vars.push_back(x);
    }

    size_t size() const {
        return vars.size();
    }

    void print_heap() const {
        for(const auto& x: vars) {
            cout << x << ", ";
        }
        cout << endl;
    }

    uint32_t mem_used() const {
        uint32_t ret = 0;
        ret += in_heap.capacity() * sizeof(unsigned char);
        //ret += vars.capacity() * sizeof(uint32_t);
        return ret;
    }

    void build(const vector<uint32_t>& vs) {
        in_heap.clear();
        uint32_t max = 0;
        for(const auto x: vs) {
            max = std::max(x, max);
        }
        in_heap.resize(max+1, false);
        vars.clear();
        std::copy(
            vs.begin(),
            vs.end(),
            std::inserter(vars, vars.end()));
        for(const auto& x: vars) {
            in_heap[x] = true;
        }
    }

    bool heap_property() const
    {
        for(const auto& x: vars) {
            if (!in_heap[x]) {
                return false;
            }
        }

        return true;
    }

    uint32_t get_random_element(MTRand& mtrand)
    {
        if (vars.empty()) {
            return var_Undef;
        }

        uint32_t which = mtrand.randInt(vars.size()-1);
        uint32_t picked = vars[which];
        std::swap(vars[which], vars[vars.size()-1]);
        vars.pop_back();
        assert(inHeap(picked));
        in_heap[picked] = false;

        return picked;
    }
};


/**
@brief The propagating and conflict generation class

Handles watchlists, conflict analysis, propagation, variable settings, etc.
*/
class PropEngine: public CNF
{
public:

    // Constructor/Destructor:
    //
    PropEngine(
        const SolverConf* _conf
        , Solver* solver
        , std::atomic<bool>* _must_interrupt_inter
    );
    virtual ~PropEngine();

    // Read state:
    //
    uint32_t nAssigns   () const;         ///<The current number of assigned literals.

    //Get state
    uint32_t    decisionLevel() const;      ///<Returns current decision level
    size_t      getTrailSize() const; //number of variables set at decision level 0
    size_t trail_size() const {
        return trail.size();
    }
    Lit trail_at(size_t at) const {
        return trail[at].lit;
    }

    template<bool inprocess>
    bool propagate_occur(int64_t* limit_to_decrease);
    void reverse_prop(const Lit l);
    void reverse_one_bnn(uint32_t idx, BNNPropType t);
    PropStats propStats;
    template<bool inprocess>
    void enqueue(const Lit p, const uint32_t level,
                 const PropBy from = PropBy(), const bool do_unit_frat = true);
    template<bool inprocess> void enqueue(const Lit p);
    void enqueue_light(const Lit p);
    void new_decision_level();

    /////////////////////
    // Branching
    /////////////////////
    vector<double> var_act_vsids;
    double var_decay = 0.95;
    struct VarOrderLt { ///Order variables according to their activities
        const vector<double>&  activities;
        bool operator () (const uint32_t x, const uint32_t y) const
        {
            return activities[x] > activities[y];
        }

        explicit VarOrderLt(const vector<double>& _activities) :
            activities(_activities)
        {}
    };
    ///activity-ordered heap of decision variables.
    Heap<VarOrderLt> order_heap_vsids; ///NOT VALID WHILE SIMPLIFYING
    RandHeap order_heap_rand;
    Queue vmtf_queue;
    uint64_t stats_bumped = 0;
    vector<uint64_t> vmtf_btab; ///< Indexed by variable number. enqueue time stamps for queue
    void vmtf_update_queue_unassigned (const uint32_t var);
    void vmtf_init_enqueue (const uint32_t var);
    void vmtf_dequeue (const uint32_t var);
    void vmtf_bump_queue (const uint32_t var);
    void vmtf_check_unassigned();
    uint32_t vmtf_pick_var();
    vector<Link> vmtf_links; ///< Indexed by variable number. table of vmtf_links for decision queue.
    double max_vsids_act = 0.0;

    //Clause activities
    double max_cl_act = 0.0;
    vector<int32_t> chain; ///< For resolution chains

    enum class gauss_ret {g_cont, g_nothing, g_false};
    vector<EGaussian*> gmatrices;
    vector<GaussQData> gqueuedata;

protected:
    friend class DataSync;
    int64_t simpDB_props = 0;
    void new_var(
        const bool bva,
        const uint32_t orig_outer,
        const bool insert_varorder = true) override;
    void new_vars(const size_t n) override;
    void save_on_var_memory();
    template<class T> uint32_t calc_glue(const T& ps);

    // Solver state:
    //
    vector<Trail>  trail; ///< Assignment stack; stores all assignments made in the order they were made.
    vector<uint32_t>    trail_lim;        ///< Separator indices for different decision levels in 'trail'.
    uint32_t            qhead;            ///< Head of queue (as index into the trail)
    Lit                 failBinLit;       ///< Used to store which watches[lit] we were looking through when conflict occured

    friend class EGaussian;

    /////////////////
    // Operations on clauses:
    /////////////////
    vector<Lit>* get_bnn_reason(BNN* bnn, Lit lit);
    void get_bnn_confl_reason(BNN* bnn, vector<Lit>* ret);
    void get_bnn_prop_reason(BNN* bnn, Lit lit, vector<Lit>* ret);
    lbool bnn_prop(
        const uint32_t bnn_idx, uint32_t level,
        Lit l, BNNPropType prop_t);
    void attachClause(
        const Clause& c
        , const bool checkAttach = true
    );

    void detach_bin_clause(
        Lit lit1
        , Lit lit2
        , bool red
        , const uint64_t ID
        , bool allow_empty_watch = false
        , bool allow_change_order = false
    ) {
        if (!allow_change_order) {
            if (!(allow_empty_watch && watches[lit1].empty())) {
                removeWBin(watches, lit1, lit2, red, ID);
            }
            if (!(allow_empty_watch && watches[lit2].empty())) {
                removeWBin(watches, lit2, lit1, red, ID);
            }
        } else {
            if (!(allow_empty_watch && watches[lit1].empty())) {
                removeWBin_change_order(watches, lit1, lit2, red, ID);
            }
            if (!(allow_empty_watch && watches[lit2].empty())) {
                removeWBin_change_order(watches, lit2, lit1, red, ID);
            }
        }
    }
    void attach_bin_clause(
        const Lit lit1
        , const Lit lit2
        , const bool red
        , const uint64_t ID
        , [[maybe_unused]] const bool checkUnassignedFirst = true
    );
    void detach_modified_clause(
        const Lit lit1
        , const Lit lit2
        , const Clause* address
    );

    // Debug & etc:
    void     print_all_clauses();
    void     printWatchList(const Lit lit) const;
    void     print_trail();

    //Var selection, activity, etc.
    void updateVars(
        const vector<uint32_t>& outerToInter
        , const vector<uint32_t>& interToOuter
    );

    size_t mem_used() const
    {
        size_t mem = 0;
        mem += CNF::mem_used();
        mem += trail.capacity()*sizeof(Lit);
        mem += trail_lim.capacity()*sizeof(uint32_t);
        mem += toClear.capacity()*sizeof(Lit);
        return mem;
    }

protected:
    template<bool inprocess, bool red_also = true, bool use_disable = false>
    PropBy propagate_any_order();
    template<bool bin_only=true> PropBy propagate_light();
    template<bool inprocess>
    PropResult prop_normal_helper(
        Clause& c
        , ClOffset offset
        , Watched*& j
        , const Lit p
    );
    template<bool inprocess>
    PropResult handle_normal_prop_fail(Clause& c, ClOffset offset, PropBy& confl);

private:
    Solver* solver;

    template<bool inprocess>
    bool prop_bin_cl_occur(const Watched& ws);
    template<bool inprocess>
    bool prop_long_cl_occur(const ClOffset offset);
    template<bool inprocess>
    bool prop_bin_cl(
        const Watched* i
        , const Lit p
        , PropBy& confl
        , uint32_t currLevel
    );
    template<bool inprocess, bool red_also, bool use_disable>
    bool prop_long_cl_any_order(
        Watched* i
        , Watched*& j
        , const Lit p
        , PropBy& confl
        , uint32_t currLevel
    );
    void sql_dump_vardata_picktime(uint32_t v, PropBy from);

    PropBy gauss_jordan_elim(const Lit p, const uint32_t currLevel);
};

inline void PropEngine::new_decision_level()
{
    trail_lim.push_back(trail.size());
    #ifdef VERBOSE_DEBUG
    cout << "New decision level: " << trail_lim.size() << endl;
    #endif
}

inline uint32_t PropEngine::decisionLevel() const
{
    return trail_lim.size();
}

inline uint32_t PropEngine::nAssigns() const
{
    return trail.size();
}

inline size_t PropEngine::getTrailSize() const
{
    if (decisionLevel() == 0) {
        return trail.size();
    } else {
        return trail_lim[0];
    }
}

template<class T> inline
uint32_t PropEngine::calc_glue(const T& ps)
{
    MYFLAG++;
    uint32_t nblevels = 0;
    for (Lit lit: ps) {
        int l = varData[lit.var()].level;
        if (l != 0 && permDiff[l] != MYFLAG) {
            permDiff[l] = MYFLAG;
            nblevels++;
            if (nblevels >= 1000) {
                return nblevels;
            }
        }
    }
    return nblevels;
}

template<bool inprocess>
inline PropResult PropEngine::prop_normal_helper(
    Clause& c
    , ClOffset offset
    , Watched*& j
    , const Lit p
) {
    // Make sure the false literal is data[1]:
    if (c[0] == ~p) {
        std::swap(c[0], c[1]);
    }

    assert(c[1] == ~p);

    // If 0th watch is true, then clause is already satisfied.
    if (value(c[0]) == l_True) {
        *j = Watched(offset, c[0]);
        j++;
        return PROP_NOTHING;
    }

    // Look for new watch:
    for (Lit *k = c.begin() + 2, *end2 = c.end()
        ; k != end2
        ; k++
    ) {
        //Literal is either unset or satisfied, attach to other watchlist
        if (value(*k) != l_False) {
            c[1] = *k;
            *k = ~p;
            watches[c[1]].push(Watched(offset, c[0]));
            return PROP_NOTHING;
        }
    }

    return PROP_TODO;
}


template<bool inprocess>
inline PropResult PropEngine::handle_normal_prop_fail(
    Clause&
    #ifdef STATS_NEEDED
    c
    #endif
    , ClOffset offset
    , PropBy& confl
) {
    confl = PropBy(offset);
    #ifdef VERBOSE_DEBUG_FULLPROP
    Clause& c = *cl_alloc.ptr(offset);
    cout << "Conflict from cl: " << c << endl;
    #endif

    //Update stats
    #ifdef STATS_NEEDED
    if (!inprocess && c.red()) {
        red_stats_extra[c.stats.extra_pos].conflicts_made++;
    }
    #endif

    qhead = trail.size();
    return PROP_FAIL;
}

template<bool inprocess>
void PropEngine::enqueue(const Lit p)
{
    enqueue<inprocess>(p, decisionLevel(), PropBy());
}

template<bool inprocess>
void PropEngine::enqueue(const Lit p, const uint32_t level, const PropBy from, bool do_unit_frat)
{
    #ifdef VERBOSE_DEBUG
    if (level == 0) {
        cout << "enqueue var " << p.var()+1
        << " to val " << !p.sign()
        << " level: " << level
        << " decisonLevel(): " << decisionLevel()
        << " sublevel: " << trail.size()
        << " by: " << from << endl;
        cout << "trail at level 0: ";
        for(auto const& x: trail) {
            cout << "(lit: " << x.lit << " lev: " << x.lev << ")";
        }
        cout << endl;
    }
    #endif //DEBUG_ENQUEUE_LEVEL0

    #ifdef ENQUEUE_DEBUG
    assert(trail.size() <= nVarsOuter());
    #endif

    const uint32_t v = p.var();
    assert(value(v) == l_Undef);
    SLOW_DEBUG_DO(assert(varData[v].removed == Removed::none));
    if (level == 0 && frat->enabled())
    {   if (do_unit_frat) {
            const uint32_t ID = ++clauseID;
            chain.clear();
            if (from.getType() == PropByType::binary_t) {
                chain.push_back(from.getID());
                chain.push_back(unit_cl_IDs[from.lit2().var()]);
            } else if (from.getType() == PropByType::clause_t) {
                Clause* cl = cl_alloc.ptr(from.get_offset());
                chain.push_back(cl->stats.ID);
                for(auto const& l: *cl) if (l != p) chain.push_back(unit_cl_IDs[l.var()]);
            } else {
                // These are too difficult and not worth it
            }

            *frat << add << ID << p;
            if (!chain.empty()) {
                *frat << DratFlag::chain;
                for(auto const& id: chain) *frat << id;
            }
            *frat << fin;

            VERBOSE_PRINT("unit " << p << " ID: " << ID);
            assert(unit_cl_IDs[v] == 0);
            unit_cl_IDs[v] = ID;
        } else {
            assert(unit_cl_IDs[v] != 0);
        }
    }

    if (!watches[~p].empty()) watches.prefetch((~p).toInt());

    #if defined(STATS_NEEDED_BRANCH) || defined(FINAL_PREDICTOR_BRANCH)
    if (!inprocess) {
        varData[v].set++;
        if (from == PropBy()) {
            #ifdef STATS_NEEDED_BRANCH
            sql_dump_vardata_picktime(v, from);
            varData[v].num_decided++;
            varData[v].last_decided_on = sumConflicts;
            if (!p.sign()) varData[v].num_decided_pos++;
            #endif
        } else {
            sumPropagations++;
            #ifdef STATS_NEEDED_BRANCH
            bool flipped = (varData[v].polarity != !p.sign());
            if (flipped) {
                varData[v].last_flipped = sumConflicts;
            }
            varData[v].num_propagated++;
            varData[v].last_propagated = sumConflicts;
            if (!p.sign()) varData[v].num_propagated_pos++;
            #endif
        }
    }
    #endif

    const bool sign = p.sign();
    assigns[v] = boolToLBool(!sign);
    varData[v].reason = from;
    varData[v].level = level;
    varData[v].sublevel = trail.size();
    if (!inprocess) {
        #ifdef STATS_NEEDED
        if (sign) {
            propStats.varSetNeg++;
        } else {
            propStats.varSetPos++;
        }
        #endif
    }
    trail.push_back(Trail(p, level));

    if (inprocess) {
        propStats.bogoProps += 1;
    }
}

template<bool bin_only>
PropBy PropEngine::propagate_light()
{
    PropBy confl;
    VERBOSE_PRINT("propagate_light started");

    while (qhead < trail.size() && confl.isNULL()) {
        const Lit p = trail[qhead].lit;
        watch_subarray ws = watches[~p];

        Watched* i = ws.begin();
        Watched* j = i;
        Watched* end = ws.end();
        propStats.bogoProps += ws.size()/4 + 1;
        for (; i != end; i++) {
            if (bin_only && !confl.isNULL()) break;

            // propagate binary clause
            if (i->isBin()) {
                if (!bin_only) *j++ = *i;

                const lbool val = value(i->lit2());
                if (val == l_Undef) {
                    enqueue_light(i->lit2());
                } else if (val == l_False) {
                    confl = PropBy(~p, i->red(), i->get_ID());
                }
                continue;
            }

            if (!bin_only && i->isClause()) {
                if (value(i->getBlockedLit()) == l_True) {
                    *j++ = *i;
                    continue;
                }
                propStats.bogoProps += 4;
                const ClOffset offset = i->get_offset();
                Clause& c = *cl_alloc.ptr(offset);

                if (c[0] == ~p) std::swap(c[0], c[1]);
                assert(c[1] == ~p);

                // If 0th watch is true, then clause is already satisfied.
                if (value(c[0]) == l_True) {
                    *j = Watched(offset, c[0]);
                    j++;
                    continue;
                }

                // Look for new watch:
                bool cont = false;
                for (Lit *k = c.begin() + 2, *end2 = c.end()
                    ; k != end2
                    ; k++
                ) {
                    //Literal is either unset or satisfied, attach to other watchlist
                    if (value(*k) != l_False) {
                        c[1] = *k;
                        *k = ~p;
                        watches[c[1]].push(Watched(offset, c[0]));
                        cont = true;
                        break;
                    }
                }
                if (cont) continue;

                // Did not find watch -- clause is unit under assignment:
                *j++ = *i;
                if (value(c[0]) == l_False) confl = PropBy(offset);
                else enqueue_light(c[0]);
                continue;
            }

            if (!bin_only) {
                *j++=*i;
            }
        }

        if (!bin_only) {
            while (i != end) {
                *j++ = *i++;
            }
            ws.shrink_(end-j);
        }

        VERBOSE_PRINT("propagate_light went through watchlist of " << p);
        qhead++;
    }
    VERBOSE_PRINT("propagate_light ended.");
    return confl;
}

inline void PropEngine::enqueue_light(const Lit p)
{
    const uint32_t v = p.var();
    assert(value(v) == l_Undef);
    SLOW_DEBUG_DO(assert(varData[v].removed == Removed::none));
    if (!watches[~p].empty()) watches.prefetch((~p).toInt());

    const bool sign = p.sign();
    assigns[v] = boolToLBool(!sign);
    trail.push_back(Trail(p, 1));
    propStats.bogoProps += 1;
}

inline void PropEngine::attach_bin_clause(
    const Lit lit1
    , const Lit lit2
    , const bool red
    , const uint64_t ID
    , [[maybe_unused]] const bool checkUnassignedFirst
) {
    #ifdef DEBUG_ATTACH
    assert(lit1.var() != lit2.var());
    if (checkUnassignedFirst) {
        assert(value(lit1.var()) == l_Undef);
        assert(value(lit2) == l_Undef || value(lit2) == l_False);
    }

    assert(varData[lit1.var()].removed == Removed::none);
    assert(varData[lit2.var()].removed == Removed::none);
    #endif //DEBUG_ATTACH

    watches[lit1].push(Watched(lit2, red, ID));
    watches[lit2].push(Watched(lit1, red, ID));
}

} //end namespace

#endif //__PROPENGINE_H__
