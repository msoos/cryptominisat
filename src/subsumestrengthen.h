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

#ifndef __SUBSUMESTRENGTHEN_H__
#define __SUBSUMESTRENGTHEN_H__

#include "cloffset.h"
#include "solvertypesmini.h"
#include "clabstraction.h"
#include "clause.h"
#include "Vec.h"
#include <vector>
using std::vector;

namespace CMSat {

class OccSimplifier;
class GateFinder;
class Solver;

class SubsumeStrengthen
{
public:
    SubsumeStrengthen(OccSimplifier* simplifier, Solver* solver);
    size_t mem_used() const;

    void backw_sub_long_with_long();
    bool backw_str_long_with_long();
    bool backw_sub_str_long_with_bins();
    bool backw_sub_str_long_with_bins_watch(const Lit lit, bool both_bins = false);
    bool handle_added_long_cl(const bool main_run);
    void remove_binary_cl(const OccurClause& cl);


    Sub0Ret backw_sub_with_long(const ClOffset offset);

    void backw_sub_with_impl(
        const vector<Lit>& lits,
        Sub1Ret& ret_sub_str
    );
    bool backw_sub_str_with_impl(
        const vector<Lit>& lits,
        Sub1Ret& ret_sub_str);
    bool backw_sub_str_with_long(
        ClOffset offset,
        Sub1Ret& ret_sub_str);

    struct Stats
    {
        Stats& operator+=(const Stats& other);
        void print_short(const Solver* solver) const;
        void print() const;

        Sub0Ret sub0;
        Sub1Ret sub1;

        double subsumeTime = 0.0;
        double strengthenTime = 0.0;
    };

    void finishedRun();
    const Stats& get_stats() const;
    const Stats& getRunStats() const;

    template<class T>
    void find_subsumed(
        const ClOffset offset
        , const T& ps
        , const cl_abst_type abs
        , vector<OccurClause>& out_subsumed
        , const bool only_irred = false
    );

private:
    Stats globalstats;
    Stats runStats;

    OccSimplifier* simplifier;
    Solver* solver;

    //Called from simplifier at resolvent-adding of var-elim
    template<class T>
    Sub0Ret subsume_and_unlink(
        const ClOffset offset
        , const T& ps
        , const cl_abst_type abs
    );

    void randomise_clauses_order();

    template<class T>
    uint32_t find_smallest_watchlist_for_clause(const T& ps) const;

    template<class T>
    void find_subsumed_and_strengthened(
        const ClOffset offset
        , const T& ps
        , const cl_abst_type abs
        , vector<OccurClause>& out_subsumed
        , vector<Lit>& out_lits
    );

    template<class T>
    void fill_sub_str(
        const ClOffset offset
        , const T& ps
        , cl_abst_type abs
        , vector<OccurClause>& out_subsumed
        , vector<Lit>& out_lits
        , const Lit lit
        , const bool inverted
    );

    template<class T1, class T2>
    bool subset(const T1& A, const T2& B);

    template<class T1, class T2>
    Lit subset1(const T1& A, const T2& B);

    vector<OccurClause> subs;
    vec<Watched> tmp;
    vector<Lit> subsLits;
    vector<Lit> tmpLits;
    size_t tried_bin_tri = 0;
    uint64_t subsumedBin = 0;
    uint64_t strBin = 0;
};

inline const SubsumeStrengthen::Stats& SubsumeStrengthen::getRunStats() const
{
    return runStats;
}

inline const SubsumeStrengthen::Stats& SubsumeStrengthen::get_stats() const
{
    return globalstats;
}

} //end namespace

#endif //__SUBSUMESTRENGTHEN_H__
