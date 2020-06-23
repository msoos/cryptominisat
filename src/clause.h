/******************************************
Copyright (c) 2016, Mate Soos

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


#ifndef CLAUSE_H
#define CLAUSE_H

#include <cstdio>
#include <vector>
#include <sys/types.h>
#include <string.h>
#include <limits>

#include "solverconf.h"
#include "solvertypes.h"
#include "constants.h"
#include "watched.h"
#include "alg.h"
#include "clabstraction.h"
#include "avgcalc.h"
#include "constants.h"
#include "searchhist.h"

namespace CMSat {

class ClauseAllocator;

template <class T>
struct AtecedentData
{
    void clear()
    {
        *this = AtecedentData<T>();
    }

    uint64_t num() const
    {
        return binRed + binIrred + longIrred + longRed;
    }

    template<class T2>
    AtecedentData& operator+=(const AtecedentData<T2>& other)
    {
        binRed += other.binRed;
        binIrred += other.binIrred;
        longIrred += other.longIrred;
        longRed += other.longRed;

        glue_long_reds += other.glue_long_reds;
        size_longs += other.size_longs;
        age_long_reds += other.age_long_reds;

        return *this;
    }

    template<class T2>
    AtecedentData& operator-=(const AtecedentData<T2>& other)
    {
        binRed -= other.binRed;
        binIrred -= other.binIrred;
        longIrred -= other.longIrred;
        longRed -= other.longRed;

        glue_long_reds -= other.glue_long_reds;
        size_longs -= other.size_longs;
        age_long_reds -= other.age_long_reds;

        return *this;
    }

    uint32_t sum_size() const
    {
        uint32_t sum = 0;
        sum += binIrred*2;
        sum += binRed*2;
        sum += size_longs.get_sum();

        return sum;
    }

    T binRed = 0;
    T binIrred = 0;
    T longIrred = 0;
    T longRed = 0;
    AvgCalc<uint32_t> glue_long_reds;
    AvgCalc<uint32_t> size_longs;
    AvgCalc<uint32_t> age_long_reds;
};

struct ClauseStats
{
    ClauseStats()
    {
        glue = 1000;
        marked_clause = false;
        ttl = 0;
        which_red_array = 2;
        locked_for_data_gen = 0;
        is_decision = false;
        //TODO it's weird, it has been tested to be better with "1"
        activity = 1;
        last_touched = 0;
    }

    //Stored data
    uint32_t glue:20;  //currently in code limited to 100'000
    uint32_t is_decision:1;
    uint32_t marked_clause:1;
    uint32_t ttl:2;
    uint32_t which_red_array:3;
    uint32_t locked_for_data_gen:1;
    union {
        float   activity;
        uint32_t hash_val; //used in BreakID to remove equivalent clauses
    };
    uint32_t last_touched;
    #ifdef FINAL_PREDICTOR
#ifdef EXTENDED_FEATURES
    uint32_t    rdb1_last_touched;
#endif
    float       glue_hist_long;
    float       glue_hist_queue;
#ifdef EXTENDED_FEATURES
    float       glue_hist;
#endif
    float       confl_size_hist_lt;
    uint32_t    glue_before_minim;
//     uint32_t    num_overlap_literals;
    float       antec_overlap_hist;
    uint32_t    num_total_lits_antecedents;
//     uint32_t    rdb1_last_touched_diff;
    uint32_t    num_antecedents;
    float       branch_depth_hist_queue;
    float       num_resolutions_hist_lt;
//     uint32_t    trail_depth_hist_longer;
#ifdef EXTENDED_FEATURES
    float       rdb1_act_ranking_rel = 0;
#endif
//     uint8_t     rdb1_act_ranking_top_10 = 0;
    float pred_short_use;
    float pred_long_use;
    float pred_forever_use;
    #endif

    #if defined(STATS_NEEDED) || defined (FINAL_PREDICTOR)
    uint16_t dump_no = 0;
    uint32_t orig_glue;
    uint32_t introduced_at_conflict = 0; ///<At what conflict number the clause  was introduced

    //for average and sum stats
    uint32_t sum_uip1_used = 0; ///N.o. times claue was used during 1st UIP generation for ALL TIME

    //below resets
    uint32_t used_for_uip_creation = 0; ///N.o. times claue was used during 1st UIP generation in this RDB
//     uint32_t rdb1_used_for_uip_creation = 0; ///N.o. times claue was used during 1st UIP generation in previous RDB
    uint32_t propagations_made = 0; ///<Number of times caused propagation
    uint32_t rdb1_propagations_made = 0; ///<Number of times caused propagation, last round
    #endif

    #ifdef STATS_NEEDED
    int32_t ID = 0;
    uint32_t sum_propagations_made = 0; ///<Number of times caused propagation

    AtecedentData<uint16_t> antec_data;
    uint32_t conflicts_made = 0; ///<Number of times caused conflict
    uint32_t clause_looked_at = 0; ///<Number of times the clause has been deferenced during propagation
    #endif

    #if defined(STATS_NEEDED) || defined (FINAL_PREDICTOR)
    void reset_rdb_stats()
    {
        ttl = 0;
        used_for_uip_creation = 0;
        propagations_made = 0;
        #if defined(STATS_NEEDED)
        clause_looked_at = 0;
        conflicts_made = 0;
        antec_data.clear();
        #endif
    }
    #endif

    static ClauseStats combineStats(const ClauseStats& first, const ClauseStats& second)
    {
        //Create to-be-returned data
        ClauseStats ret = first;

        //Combine stats
        ret.glue = std::min(first.glue, second.glue);
        ret.activity = std::max(first.activity, second.activity);

        #if defined(STATS_NEEDED) || defined (FINAL_PREDICTOR)
        if (first.introduced_at_conflict == 0) {
            ret.introduced_at_conflict = second.introduced_at_conflict;
        } else if (second.introduced_at_conflict == 0) {
            ret.introduced_at_conflict = first.introduced_at_conflict;
        } else {
            ret.introduced_at_conflict = std::min(first.introduced_at_conflict, second.introduced_at_conflict);
        }
        ret.used_for_uip_creation = first.used_for_uip_creation + second.used_for_uip_creation;
        ret.propagations_made = first.propagations_made + second.propagations_made;
        #endif

        #ifdef STATS_NEEDED
        ret.ID = 0; //don't track combined clauses
        ret.conflicts_made = first.conflicts_made + second.conflicts_made;
        ret.clause_looked_at = first.clause_looked_at + second.clause_looked_at;
        #endif

        ret.which_red_array = std::min(first.which_red_array, second.which_red_array);

        return ret;
    }
};

inline std::ostream& operator<<(std::ostream& os, const ClauseStats& stats)
{

    os << "glue " << stats.glue << " ";
    #if defined(STATS_NEEDED) || defined (FINAL_PREDICTOR)
    os << "conflIntro " << stats.introduced_at_conflict<< " ";
    os << "used_for_uip_creation " << stats.used_for_uip_creation << " ";
    os << "numProp " << stats.propagations_made<< " ";
    #endif
    #ifdef STATS_NEEDED
    os << "numConfl " << stats.conflicts_made<< " ";
    os << "numLook " << stats.clause_looked_at<< " ";
    #endif

    return os;
}

/**
@brief Holds a clause. Does not allocate space for literals

Literals are allocated by an external allocator that allocates enough space
for the class that it can hold the literals as well. I.e. it malloc()-s
    sizeof(Clause)+LENGHT*sizeof(Lit)
to hold the clause.
*/
class Clause
{
public:
    uint16_t isRed:1; ///<Is the clause a redundant clause?
    uint16_t isRemoved:1; ///<Is this clause queued for removal?
    uint16_t isFreed:1; ///<Has this clause been marked as freed by the ClauseAllocator ?
    uint16_t is_distilled:1;
    uint16_t is_ternary_resolved:1;
    uint16_t is_ternary_resolvent:1;
    uint16_t occurLinked:1;
    uint16_t must_recalc_abst:1;
    uint16_t _used_in_xor:1;
    uint16_t _used_in_xor_full:1;
    uint16_t _xor_is_detached:1;
    uint16_t _gauss_temp_cl:1; ///Used ONLY by Gaussian elimination to incicate where a proagation is coming from
    uint16_t reloced:1;


    Lit* getData()
    {
        return (Lit*)((char*)this + sizeof(Clause));
    }

    const Lit* getData() const
    {
        return (Lit*)((char*)this + sizeof(Clause));
    }

public:
    cl_abst_type abst;
    ClauseStats stats;
    uint32_t mySize;

    template<class V>
    Clause(const V& ps, const uint32_t _introduced_at_conflict
        #ifdef STATS_NEEDED
        , const int64_t _ID
        #endif
        )
    {
        //assert(ps.size() > 2);

        stats.last_touched = _introduced_at_conflict;
        #if defined(FINAL_PREDICTOR) || defined(STATS_NEEDED)
        stats.introduced_at_conflict = _introduced_at_conflict;
        #endif

        #ifdef STATS_NEEDED
        stats.ID = _ID;
        assert(_ID >= 0);
        #endif
        stats.glue = std::min<uint32_t>(stats.glue, ps.size());
        isFreed = false;
        mySize = ps.size();
        isRed = false;
        isRemoved = false;
        is_distilled = false;
        is_ternary_resolved = false;
        is_ternary_resolvent = false;
        must_recalc_abst = true;
        _used_in_xor = false;
        _used_in_xor_full = false;
        _xor_is_detached = false;
        reloced = false;

        for (uint32_t i = 0; i < ps.size(); i++) {
            getData()[i] = ps[i];
        }
    }

    typedef Lit* iterator;
    typedef const Lit* const_iterator;

    uint32_t size() const
    {
        return mySize;
    }

    bool used_in_xor() const
    {
        return _used_in_xor;
    }

    void set_used_in_xor(const bool val)
    {
        _used_in_xor = val;
    }

    bool used_in_xor_full() const
    {
        return _used_in_xor_full;
    }

    void set_used_in_xor_full(const bool val)
    {
        _used_in_xor_full = val;
    }

    void shrink(const uint32_t i)
    {
        assert(i <= size());
        mySize -= i;
        if (i > 0)
            setStrenghtened();
    }

    void resize (const uint32_t i)
    {
        assert(i <= size());
        if (i == size()) return;
        mySize = i;
        setStrenghtened();
    }

    bool red() const
    {
        return isRed;
    }

    bool freed() const
    {
        return isFreed;
    }

    void reCalcAbstraction()
    {
        abst = calcAbstraction(*this);
        must_recalc_abst = false;
    }

    void setStrenghtened()
    {
        must_recalc_abst = true;
        //is_ternary_resolved = false; //probably not a good idea
        //is_distilled = false; //TODO?
    }

    void recalc_abst_if_needed()
    {
        if (must_recalc_abst) {
            reCalcAbstraction();
        }
    }

    Lit& operator [] (const uint32_t i)
    {
        return *(getData() + i);
    }

    const Lit& operator [] (const uint32_t i) const
    {
        return *(getData() + i);
    }

    void makeIrred()
    {
        assert(isRed);
        #if STATS_NEEDED
        stats.ID = 0;
        #endif
        isRed = false;
    }

    void makeRed(uint64_t
    #if defined(FINAL_PREDICTOR) || defined(STATS_NEEDED)
    confl_num
    #endif
    )
    {
        isRed = true;
        #if defined(FINAL_PREDICTOR) || defined(STATS_NEEDED)
        stats.introduced_at_conflict = confl_num;
        #endif
    }

    void strengthen(const Lit p)
    {
        remove(*this, p);
        setStrenghtened();
    }

    void add(const Lit p)
    {
        mySize++;
        getData()[mySize-1] = p;
        setStrenghtened();
    }

    const Lit* begin() const
    {
        #ifdef SLOW_DEBUG
        assert(!freed());
        assert(!getRemoved());
        #endif
        return getData();
    }

    Lit* begin()
    {
        #ifdef SLOW_DEBUG
        assert(!freed());
        assert(!getRemoved());
        #endif
        return getData();
    }

    const Lit* end() const
    {
        return getData()+size();
    }

    Lit* end()
    {
        return getData()+size();
    }

    void setRemoved()
    {
        isRemoved = true;
    }

    bool getRemoved() const
    {
        return isRemoved;
    }

    void unset_removed()
    {
        isRemoved = false;
    }

    void setFreed()
    {
        isFreed = true;
    }

    void combineStats(const ClauseStats& other)
    {
        stats = ClauseStats::combineStats(stats, other);
    }

    void set_distilled(bool distilled)
    {
        is_distilled = distilled;
    }

    bool getdistilled() const
    {
        return is_distilled;
    }

    bool getOccurLinked() const
    {
        return occurLinked;
    }

    void setOccurLinked(bool toset)
    {
        occurLinked = toset;
    }

    void print_extra_stats() const
    {
        cout
        << "Clause size " << std::setw(4) << size();
        if (red()) {
            cout << " glue : " << std::setw(4) << stats.glue;
        }
        #ifdef STATS_NEEDED
        cout
        << " Confls: " << std::setw(10) << stats.conflicts_made
        << " Props: " << std::setw(10) << stats.propagations_made
        << " Looked at: " << std::setw(10)<< stats.clause_looked_at
        << " UIP used: " << std::setw(10)<< stats.used_for_uip_creation;
        #endif
        cout << endl;
    }
};

inline std::ostream& operator<<(std::ostream& os, const Clause& cl)
{
    for (uint32_t i = 0; i < cl.size(); i++) {
        os << cl[i];

        if (i+1 != cl.size())
            os << " ";
    }

    return os;
}

struct BinaryXor
{
    uint32_t vars[2];
    bool rhs;

    BinaryXor(uint32_t var1, uint32_t var2, const bool _rhs) {
        if (var1 > var2) {
            std::swap(var1, var2);
        }
        vars[0] = var1;
        vars[1] = var2;
        rhs = _rhs;
    }

    bool operator<(const BinaryXor& other) const
    {
        if (vars[0] != other.vars[0]) {
            return vars[0] < other.vars[0];
        }

        if (vars[1] != other.vars[1]) {
            return vars[1] < other.vars[1];
        }

        if (rhs != other.rhs) {
            return (int)rhs < (int)other.rhs;
        }
        return false;
    }
};

} //end namespace

#endif //CLAUSE_H
