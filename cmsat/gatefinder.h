/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.0 of the License, or (at your option) any later version.
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

#ifndef _GATEFINDER_H_
#define _GATEFINDER_H_

#include "solvertypes.h"
#include "cset.h"
#include <boost/array.hpp>
#include <set>
#include "watcharray.h"

namespace CMSat {

class Solver;
class Simplifier;
using std::set;

class NewGateData
{
    public:
        NewGateData(const Lit _lit1, const Lit _lit2, const uint32_t _numLitRem, const uint32_t _numClRem) :
            lit1(_lit1)
            , lit2(_lit2)
            , numLitRem(_numLitRem)
            , numClRem(_numClRem)
        {}
        bool operator<(const NewGateData& n2) const
        {
            uint32_t value1 = numClRem*ANDGATEUSEFUL + numLitRem;
            uint32_t value2 = n2.numClRem*ANDGATEUSEFUL + n2.numLitRem;
            if (value1 != value2) return(value1 > value2);
            if (lit1 != n2.lit1) return(lit1 > n2.lit1);
            if (lit2 != n2.lit2) return(lit2 > n2.lit2);
            return false;
        }
        bool operator==(const NewGateData& n2) const
        {
            return(lit1 == n2.lit1
                && lit2 == n2.lit2
                && numLitRem == n2.numLitRem
                && numClRem == n2.numClRem);
        }
        Lit lit1;
        Lit lit2;
        uint32_t numLitRem;
        uint32_t numClRem;
};

struct SecondSorter
{
    bool operator() (const std::pair<Var, uint32_t> p1, const std::pair<Var, uint32_t> p2)
    {
        return p1.second > p2.second;
    }
};

class OrGate {
    public:
        OrGate(const Lit& _eqLit, Lit _lit1, Lit _lit2, const bool _red) :
            lit1(_lit1)
            , lit2(_lit2)
            , eqLit(_eqLit)
            , red(_red)
        {
            if (lit1 > lit2)
                std::swap(lit1, lit2);
        }

        bool operator<(const OrGate& other) const
        {
            if (eqLit != other.eqLit) {
                return (eqLit < other.eqLit);
            }

            if (lit1 != other.lit1) {
                return (lit1 < other.lit1);
            }

            return (lit2 < other.lit2);
        }

        bool operator==(const OrGate& other) const
        {
            return
                eqLit == other.eqLit
                && lit1 == other.lit1
                && lit2 == other.lit2
                ;
        }
        boost::array<Lit, 2> getLits() const
        {
            return boost::array<Lit, 2>{{lit1, lit2}};
        }

        //LHS
        Lit lit1;
        Lit lit2;

        //RHS
        Lit eqLit;

        //Data about gate
        bool red;
};

inline std::ostream& operator<<(std::ostream& os, const OrGate& gate)
{
    os
    << " gate "
    << " lits: " << gate.lit1 << ", " << gate.lit2
    << " eqLit: " << gate.eqLit
    << " learnt " << gate.red
    ;
    return os;
}

class GateFinder
{
public:
    GateFinder(Simplifier *subsumer, Solver *control);

    void newVar();
    bool doAll();

    //Getter functions
    void printGateStats() const;
    void printDot(); ///<Print Graphviz DOT file describing the gates

    //Stats
    struct Stats
    {
        void clear()
        {
            Stats tmp;
            *this = tmp;
        }

        double totalTime() const
        {
            return findGateTime + orBasedTime + varReplaceTime
                + andBasedTime + erTime;
        }
        Stats& operator+=(const Stats& other);
        void print(const size_t nVars) const;
        void printShort() const;

        //Time
        double findGateTime = 0.0;
        uint32_t find_gate_timeout = 0;
        double orBasedTime = 0.0;
        uint32_t or_based_timeout = 0;
        double varReplaceTime = 0.0;
        double andBasedTime = 0.0;
        uint32_t and_based_timeout = 0;
        double erTime = 0.0;

        //OR-gate
        uint64_t orGateUseful = 0;
        uint64_t numLongCls = 0;
        uint64_t numLongClsLits = 0;
        int64_t  litsRem = 0;

        //Var-replace
        uint64_t varReplaced = 0;

        //And-gate
        uint64_t andGateUseful = 0;
        uint64_t clauseSizeRem = 0;

        //ER
        uint64_t numERVars = 0;

        //Gates
        uint64_t learntGatesSize = 0;
        uint64_t numRed = 0;
        uint64_t irredGatesSize = 0;
        uint64_t numIrred = 0;
    };

    const Stats& getStats() const;

private:
    //Setup
    void clearIndexes();

    //Each algo
    void findOrGates();
    void createNewVars();
    size_t num_long_irred_cls(const Lit lit) const;

    //Helpers to find
    void findOrGates(const bool redGatesToo);
    void findOrGate(
        const Lit eqLit
        , const Lit lit1
        , const Lit lit2
        , const bool redGatesToo
        , bool wasRed
    );

    bool doAllOptimisationWithGates();
    bool shortenWithOrGate(const OrGate& gate);
    size_t findEqOrGates();

    //And gate treatment
    bool remove_clauses_using_and_gate(
        const OrGate& gate
        , const bool reallyRemove
        , const bool only_irred
        , uint32_t& reduction
    );

    CL_ABST_TYPE  calc_sorted_occ_and_set_seen2(
        const OrGate& gate
        , uint16_t& maxSize
        , uint16_t& minSize
        , const bool only_irred
    );
    void set_seen2_and_abstraction(
        const Clause& cl
        , CL_ABST_TYPE& abstraction
    );
    bool check_seen_and_gate_against_cl(
        const Clause& this_cl
        , const OrGate& gate
    );

    void treatAndGateClause(
        const ClOffset other_cl_offset
        , const OrGate& gate
        , const ClOffset this_cl_offset
    );
    CL_ABST_TYPE calc_abst_and_set_seen(
       const Clause& cl
        , const OrGate& gate
    );
    ClOffset find_pair_for_and_gate_reduction(
        const Watched& ws
        , const size_t minSize
        , const size_t maxSize
        , const CL_ABST_TYPE abstraction
        , const OrGate& gate
        , const bool only_irred
    );

    ClOffset findAndGateOtherCl(
        const vector<ClOffset>& sizeSortedOcc
        , const Lit lit
        , const CL_ABST_TYPE abst2
        , const bool gate_is_red
        , const bool only_irred
    );
    bool findAndGateOtherCl_tri(
       watch_subarray_const ws_list
       , const bool gate_is_red
       , const bool only_irred
       , Watched& ret
    );
    bool find_pair_for_and_gate_reduction_tri(
        const Watched& ws
        , const OrGate& gate
        , const bool only_irred
        , Watched& found_pair
    );
    bool remove_clauses_using_and_gate_tri(
       const OrGate& gate
        , const bool really_remove
        , const bool only_irred
        , uint32_t& reduction
    );
    void set_seen2_tri(
       const OrGate& gate
        , const bool only_irred
    );
    bool check_seen_and_gate_against_lit(
        const Lit lit
        , const OrGate& gate
    );

    ///temporary for and-gate treatment. Cleared at every treatAndGate() call
    vector<vector<ClOffset> > sizeSortedOcc;

    //Indexes, gate data
    vector<OrGate> orGates; //List of OR gates
    vector<vector<uint32_t> > gateOcc; //LHS of every NON-LEARNT gate is in this occur list (a = b V c, so 'a' is LHS)
    vector<vector<uint32_t> > gateOccEq; //RHS of every gate is in this occur list (a = b V c, so 'b' and 'c' are LHS)

    //For temporaries
    vector<size_t> seen2Set; //Bits that have been set in seen2, and later need to be cleared
    set<ClOffset> clToUnlink;

    //Graph
    void printDot2(); ///<Print Graphviz DOT file describing the gates

    //Stats
    Stats runStats;
    Stats globalStats;

    //Limits
    int64_t  numMaxGateFinder;
    int64_t  numMaxCreateNewVars;
    int64_t  numMaxShortenWithGates;
    int64_t  numMaxClRemWithGates;

    //long-term stats
    uint64_t numDotPrinted;

    //Main data
    Simplifier *simplifier;
    Solver *solver;
    vector<uint16_t>& seen;
    vector<uint16_t>& seen2;
};

inline const GateFinder::Stats& GateFinder::getStats() const
{
    return globalStats;
}

} //end namespace

#endif //_GATEFINDER_H_
