/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2011, Mate Soos and collaborators. All rights reserved.
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

#include "SolverTypes.h"
#include "CSet.h"

class ThreadControl;
class Subsumer;

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
        OrGate(const std::vector<Lit>& _lits, const Lit& _eqLit, const bool _learnt) :
            lits(_lits)
            , eqLit(_eqLit)
            , learnt(_learnt)
            , removed(false)
        {
            std::sort(lits.begin(), lits.end());
        }

        bool operator==(const OrGate& other) const
        {
            return (eqLit == other.eqLit && lits == other.lits);
        }
        std::vector<Lit> lits; //LHS
        Lit eqLit; //RHS
        bool learnt;
        bool removed;
};

inline std::ostream& operator<<(std::ostream& os, const OrGate& gate)
{
    os << " gate ";
    //os << " no. " << std::setw(4) << gate.num;
    os << " lits: ";
    for (uint32_t i = 0; i < gate.lits.size(); i++) {
        os << gate.lits[i] << " ";
    }
    os << " eqLit: " << gate.eqLit;
    os << " learnt " << gate.learnt;
    os << " removed: " << gate.removed;
    return os;
}

struct OrGateSorter2 {
    bool operator() (const OrGate& gate1, const OrGate& gate2) {
        if (gate1.lits.size() > gate2.lits.size()) return true;
        if (gate1.lits.size() < gate2.lits.size()) return false;

        assert(gate1.lits.size() == gate2.lits.size());
        for (uint32_t i = 0; i < gate1.lits.size(); i++) {
            if (gate1.lits[i] < gate2.lits[i]) return true;
            if (gate1.lits[i] > gate2.lits[i]) return false;
        }

        return false;
    }
};

class GateFinder
{
public:
    GateFinder(Subsumer *subsumer, ThreadControl *control);

    //Setup
    void newVar();

    bool     treatOrGates();
    void           findOrGates();
    uint32_t createNewVars();

    //Getter functions
    bool canElim(const Var var) const;
    void printGateStats() const;
    void printDot(); ///<Print Graphviz DOT file describing the gates

private:
    void clearIndexes();

    //Helpers to find
    void findOrGates(const bool learntGatesToo);
    void findOrGate(const Lit eqLit, const ClauseIndex& c, const bool learntGatesToo, bool wasLearnt);

    bool doAllOptimisationWithGates();
    bool shortenWithOrGate(const OrGate& gate);
    bool findEqOrGates();

    //And gate treatment
    bool    treatAndGate(const OrGate& gate, const bool reallyRemove, uint32_t& foundPotential, uint64_t& numOp);
    CL_ABST_TYPE  calculateSortedOcc(const OrGate& gate, uint16_t& maxSize, vector<size_t>& seen2Set, uint64_t& numOp);
    bool    treatAndGateClause(const ClauseIndex& other, const OrGate& gate, const Clause& cl);
    bool    findAndGateOtherCl(const vector<ClauseIndex>& sizeSortedOcc, const Lit lit, const CL_ABST_TYPE abst2, ClauseIndex& other);
    vector<vector<ClauseIndex> > sizeSortedOcc; ///<temporary for and-gate treatment. Cleared at every treatAndGate() call

    //Indexes, gate data
    vector<OrGate> orGates; //List of OR gates
    vector<vector<uint32_t> > gateOcc; //LHS of every NON-LEARNT gate is in this occur list
    vector<vector<uint32_t> > gateOccEq; //RHS of every gate is in this occur list

    //Extended resolution
    bool     extendedResolution();
    vector<char>   dontElim; ///<These vars should not be eliminated, because they have been added through ER

    //Stats
    int64_t  gateLitsRemoved;
    uint32_t numERVars;
    bool     finishedAddingVars;
    uint32_t numOrGateReplaced;
    uint32_t andGateNumFound;
    uint32_t andGateTotalSize;
    uint32_t numDotPrinted;
    uint16_t maxGateSize;

    //Limits
    int64_t  numMaxGateFinder;
    int64_t  numMaxCreateNewVars;
    int64_t  numMaxShortenWithGates;
    int64_t  numMaxClRemWithGates;

    //Main data
    Subsumer *subsumer;
    ThreadControl *control;
    vector<char>& seen;
    vector<char>& seen2;
};

inline bool GateFinder::canElim(const Var var) const
{
    return !dontElim[var];
}

#endif //_GATEFINDER_H_
