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

#ifndef _GATEFINDER_H_
#define _GATEFINDER_H_

#include "solvertypes.h"
#include "cset.h"
#include <set>
#include "watcharray.h"
#include <array>

namespace CMSat {

class Solver;
class OccSimplifier;
using std::set;

inline std::ostream& operator<<(std::ostream& os, const OrGate& gate)
{
    os << " gate " << " lits: ";
    for(auto const& l: gate.lits) os << l << ",";
    os << " rhs: " << gate.rhs << " ID: " << gate.ID;
    return os;
}

class GateFinder
{
public:
    GateFinder(OccSimplifier *simplifier, Solver *control);
    void find_all();
    void cleanup();
    const vector<OrGate>& get_gates() const;

    //Stats
    struct Stats
    {
        void clear()
        {
            Stats tmp;
            *this = tmp;
        }

        double total_time() const
        {
            return findGateTime + orBasedTime + varReplaceTime
                + andBasedTime + erTime;
        }
        Stats& operator+=(const Stats& other);
        void print(const size_t nVars) const;

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
        uint64_t gatesSize = 0;
        uint64_t num = 0;
    };

    const Stats& get_stats() const;

private:
    OrGate* find_gate_to_elim_on(Lit lit, uint32_t cutoff);
    void print_graphviz_dot();

    //Setup
    void link_in_gate(const OrGate& gate);
    void add_gate_if_not_already_inside(const Lit rhs, const vector<Lit>& lhs, const int32_t ID);
    void find_or_gates_in_sweep_mode(Lit lit);

    //Finding
    vector<Lit> tmp_lhs;
    void find_or_gates_and_update_stats();
    void find_or_gates();
    void findOrGate(
        const Lit rhs
        , const Lit lit1
        , const Lit lit2
    );

    vector<ClOffset> subs; //to reduce overhead of allocation
    size_t findEqOrGates();

    //Indexes, gate data
    vector<OrGate> orGates; //List of OR gates

    //For temporaries
    vector<uint32_t> seen2Set; //Bits that have been set in seen2, and later need to be cleared
    set<ClOffset> clToUnlink;

    //Stats
    Stats runStats;
    Stats globalStats;

    //Limits
    int64_t  numMaxGateFinder;
    int64_t  numMaxShortenWithGates;
    int64_t  numMaxClRemWithGates;

    //long-term stats
    uint64_t numDotPrinted;

    //Main data
    OccSimplifier *simplifier;
    Solver *solver;
    vector<uint32_t>& seen;
    vector<uint8_t>& seen2;
    vector<Lit>& toClear;
};

inline const GateFinder::Stats& GateFinder::get_stats() const
{
    return globalStats;
}

inline const vector<OrGate>& GateFinder::get_gates() const
{
    return orGates;
}

} //end namespace

#endif //_GATEFINDER_H_
