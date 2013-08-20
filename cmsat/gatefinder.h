/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
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
    bool canElim(const Var var) const;
    void printGateStats() const;
    void printDot(); ///<Print Graphviz DOT file describing the gates

    //Stats
    struct Stats
    {
        Stats() :
            //Time
            findGateTime(0)
            , orBasedTime(0)
            , varReplaceTime(0)
            , andBasedTime(0)
            , erTime(0)

            //OR-gate
            , orGateUseful(0)
            , numLongCls(0)
            , numLongClsLits(0)
            , litsRem(0)

            //Var-replace
            , varReplaced(0)

            //And-gate
            , andGateUseful(0)
            , clauseSizeRem(0)

            //ER
            , numERVars(0)

            //Gate
            , learntGatesSize(0)
            , numRed(0)
            , irredGatesSize(0)
            , numNonRed(0)
        {}

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

        Stats& operator+=(const Stats& other)
        {
            findGateTime += other.findGateTime;
            orBasedTime += other.orBasedTime;
            varReplaceTime += other.varReplaceTime;
            andBasedTime += other.andBasedTime;
            erTime += other.erTime;

            //OR-gate
            orGateUseful += other.orGateUseful;
            numLongCls += other.numLongCls;
            numLongClsLits += other.numLongClsLits;
            litsRem += other.litsRem;
            varReplaced += other.varReplaced;

            //And-gate
            andGateUseful += other.andGateUseful;
            clauseSizeRem += other.clauseSizeRem;

            //ER
            numERVars += other.numERVars;

            //Gates
            learntGatesSize += other.learntGatesSize;
            numRed += other.numRed;
            irredGatesSize += other.irredGatesSize;
            numNonRed += other.numNonRed;

            return *this;
        }

        void print(const size_t nVars) const
        {
            cout << "c -------- GATE FINDING ----------" << endl;
            printStatsLine("c time"
                , totalTime()
            );

            printStatsLine("c find gate time"
                , findGateTime
                , findGateTime/totalTime()*100.0
                , "% time"
            );

            printStatsLine("c gate-based cl-sh time"
                , orBasedTime
                , orBasedTime/totalTime()*100.0
                , "% time"
            );

            printStatsLine("c gate-based cl-rem time"
                , andBasedTime
                , andBasedTime/totalTime()*100.0
                , "% time"
            );

            printStatsLine("c gate-based varrep time"
                , varReplaceTime
                , varReplaceTime/totalTime()*100.0
                , "% time"
            );

            printStatsLine("c gatefinder cl-short"
                , orGateUseful
                , (double)orGateUseful/(double)numLongCls
                , "% long cls"
            );

            printStatsLine("c gatefinder lits-rem"
                , litsRem
                , (double)litsRem/(double)numLongClsLits
                , "% long cls lits"
            );

            printStatsLine("c gatefinder cl-rem"
                , andGateUseful
                , (double)andGateUseful/(double)numLongCls
                , "% long cls"
            );

            printStatsLine("c gatefinder cl-rem's lits"
                , clauseSizeRem
                , (double)clauseSizeRem/(double)numLongClsLits
                , "% long cls lits"
            );

            printStatsLine("c gatefinder var-rep"
                , varReplaced
                , (double)varReplaced/(double)nVars
                , "% vars"
            );

            cout << "c -------- GATE FINDING END ----------" << endl;
        }

        void printShort()
        {
            //Gate find
            cout << "c [gate] found"
            << " irred:" << numNonRed
            << " avg-s: " << std::fixed << std::setprecision(1)
            << ((double)irredGatesSize/(double)numNonRed)
            << " red: " << numRed
            /*<< " avg-s: " << std::fixed << std::setprecision(1)
            << ((double)learntGatesSize/(double)numRed)*/
            << " T: " << std::fixed << std::setprecision(2)
            << findGateTime
            << endl;

            //gate-based shorten
            cout << "c [gate] shorten"
            << " cl: " << std::setw(5) << orGateUseful
            << " l-rem: " << std::setw(6) << litsRem
            << " T: " << std::fixed << std::setw(7) << std::setprecision(2)
            << orBasedTime
            << endl;

            //gate-based cl-rem
            cout << "c [gate] rem"
            << " cl: " << andGateUseful
            << " avg s: " << ((double)clauseSizeRem/(double)andGateUseful)
            << " T: " << std::fixed << std::setprecision(2)
            << andBasedTime
            << endl;

            //var-replace
            cout << "c [gate] eqlit"
            << " v-rep: " << std::setw(3) << varReplaced
            << " T: " << std::fixed << std::setprecision(2)
            << varReplaceTime
            << endl;
        }

        //Time
        double findGateTime;
        double orBasedTime;
        double varReplaceTime;
        double andBasedTime;
        double erTime;

        //OR-gate
        uint64_t orGateUseful;
        uint64_t numLongCls;
        uint64_t numLongClsLits;
        int64_t  litsRem;

        //Var-replace
        uint64_t varReplaced;

        //And-gate
        uint64_t andGateUseful;
        uint64_t clauseSizeRem;

        //ER
        uint64_t numERVars;

        //Gates
        uint64_t learntGatesSize;
        uint64_t numRed;
        uint64_t irredGatesSize;
        uint64_t numNonRed;
    };

    const Stats& getStats() const;

private:
    //Setup
    void clearIndexes();

    //Each algo
    void     findOrGates();
    uint32_t createNewVars();

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
    bool tryAndGate(
        const OrGate& gate
        , const bool reallyRemove
        , uint32_t& foundPotential
    );

    CL_ABST_TYPE  calculateSortedOcc(
        const OrGate& gate
        , uint16_t& maxSize
        , uint16_t& minSize
    );

    void treatAndGateClause(
        const ClOffset offset
        , const OrGate& gate
        , const Clause& cl
    );

    bool findAndGateOtherCl(
        const vector<ClOffset>& sizeSortedOcc
        , const Lit lit
        , const CL_ABST_TYPE abst2
        , ClOffset& other
    );

    ///temporary for and-gate treatment. Cleared at every treatAndGate() call
    vector<vector<ClOffset> > sizeSortedOcc;

    //Indexes, gate data
    vector<OrGate> orGates; //List of OR gates
    vector<vector<uint32_t> > gateOcc; //LHS of every NON-LEARNT gate is in this occur list (a = b V c, so 'a' is LHS)
    vector<vector<uint32_t> > gateOccEq; //RHS of every gate is in this occur list (a = b V c, so 'b' and 'c' are LHS)

    //Extended resolution
    vector<char>   dontElim; ///<These vars should not be eliminated, because they have been added through ER

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
    vector<unsigned char>& seen;
    vector<unsigned char>& seen2;
};

inline bool GateFinder::canElim(const Var var) const
{
    return !dontElim[var];
}

inline const GateFinder::Stats& GateFinder::getStats() const
{
    return globalStats;
}

} //end namespace

#endif //_GATEFINDER_H_
