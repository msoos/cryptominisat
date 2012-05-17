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

#ifndef _XORFINDER_H_
#define _XORFINDER_H_

#include <vector>
#include <set>
#include <iostream>
#include <algorithm>
#include "ImplCache.h"
#include "CSet.h"
using std::vector;
using std::set;

class Solver;
class Subsumer;

class Xor
{
    public:
        Xor(const Clause& cl, const bool _rhs) :
            rhs(_rhs)
        {
            for (uint32_t i = 0; i < cl.size(); i++) {
                vars.push_back(cl[i].var());
            }
            std::sort(vars.begin(), vars.end());
        }

        bool operator==(const Xor& other) const
        {
            return (rhs == other.rhs && vars == other.vars);
        }

        vector<Var> vars;
        bool rhs;
};

inline std::ostream& operator<<(std::ostream& os, const Xor& thisXor)
{
    for (uint32_t i = 0; i < thisXor.vars.size(); i++) {
        os << Lit(thisXor.vars[i], false) << " ";
    }
    os << " =  " << std::boolalpha << thisXor.rhs << std::noboolalpha;
    return os;
}

class FoundXors
{
    struct MyClAndBin {
        MyClAndBin(Lit _lit1, Lit _lit2) :
            isBin(true)
            , lit1(_lit1)
            , lit2(_lit2)
        {
            //keep them in order
            if (lit2 < lit1)
                std::swap(lit1, lit2);
        }

        MyClAndBin(ClauseIndex index) :
            isBin(false)
            , clsimp(index)
        {
        }

        bool operator<(const MyClAndBin& other) const
        {
            if (isBin && !other.isBin) return true;
            if (!isBin && other.isBin) return false;

            if (isBin) {
                if (lit1 < other.lit1) return true;
                if (lit1 > other.lit1) return false;
                if (lit2 < other.lit2) return true;
                return false;
            } else {
                if (clsimp.index < other.clsimp.index) return true;
                return false;
            }
        }

        bool operator==(const MyClAndBin& other) const
        {
            if (isBin != other.isBin)
                return false;

            if (isBin) {
                return (lit1 == other.lit1 && lit2 == other.lit2);
            } else {
                return (clsimp.index == other.clsimp.index);
            }
        }

        bool isBin;
        Lit lit1;
        Lit lit2;
        ClauseIndex clsimp;
    };

    public:
        FoundXors(const Clause& cl, const AbstData& clData, const bool _rhs, const uint32_t whichOne) :
            origCl(cl)
            , abst(clData.abst)
            , size(clData.size)
            , rhs(_rhs)
        {
            assert(size == cl.size() && "Abst data is out of sync of the clause?");

            foundComb.resize(1UL<<cl.size(), false);
            foundComb[whichOne] = true;
        }

        //GET-type functions
        CL_ABST_TYPE      getAbst() const;
        uint32_t          getSize() const;
        bool              getRHS() const;
        const Clause&           getOrigCl() const;
        bool              foundAll() const;

        //Add
        template<class T> void add(const T& cl);
        void  add(Lit lit1, Lit lit2);

    private:
        uint32_t NumberOfSetBits(uint32_t i) const;
        bool     bit(const uint32_t a, const uint32_t b) const;

        //bitfield to indicate which of the following is already set
        //-1 -2 -3
        //-1  2  3
        // 1 -2  3
        // 1  2 -3
        //order the above according to sign: if sign:
        //LSB ... MSB
        // 1 1 1
        // 1 0 0
        // 0 1 0
        // 0 0 1
        vector<bool> foundComb;
        const Clause& origCl;
        const CL_ABST_TYPE abst;
        const uint16_t size;
        const bool rhs;
};

class XorFinder
{
public:
    XorFinder(Subsumer* subsumer, Solver* solver);
    bool findXors();

    struct Stats
    {
        Stats() :
            //Time
            findTime(0)
            , extractTime(0)
            , blockCutTime(0)

            //XOR
            , foundXors(0)
            , sumSizeXors(0)
            , numVarsInBlocks(0)
            , numBlocks(0)

            //Usefulness
            , newUnits(0)
            , newBins(0)
            , zeroDepthAssigns(0)
        {}

        void clear()
        {
            Stats tmp;
            *this = tmp;
        }

        double totalTime() const
        {
            return findTime + extractTime + blockCutTime;
        }

        Stats& operator+=(const Stats& other)
        {
            //Time
            findTime += other.findTime;
            extractTime += other.extractTime;
            blockCutTime += other.blockCutTime;

            //XOR
            foundXors += other.foundXors;
            sumSizeXors += other.sumSizeXors;
            numVarsInBlocks += other.numVarsInBlocks;
            numBlocks += other.numBlocks;

            //Usefulness
            newUnits += other.newUnits;
            newBins += other.newBins;
            zeroDepthAssigns += other.zeroDepthAssigns;

            return *this;
        }

        void printShort() const
        {
            cout
            << "c XOR finding "
            << " Num XORs: " << std::setw(6) << foundXors
            << " avg size: " << std::setw(4) << std::fixed << std::setprecision(1)
            << ((double)sumSizeXors/(double)foundXors)

            << " T: "
            << std::fixed << std::setprecision(2) << extractTime
            << endl;

            cout
            << "c Cut XORs into " << numBlocks << " block(s)"
            << " sum vars: " << numVarsInBlocks
            << " T: " << std::fixed << std::setprecision(2) << blockCutTime
            << endl;

            cout
            << "c Extracted XOR info."
            << " Units: " << newUnits
            << " Bins: " << newBins
            << " 0-depth-assigns: " << zeroDepthAssigns
            << " T: " << std::fixed << std::setprecision(2) << extractTime
            << endl;
        }

        void print(const size_t numCalls) const
        {
            cout << "c --------- XOR STATS ----------" << endl;
            printStatsLine("c num XOR found on avg"
                , (double)foundXors/(double)numCalls
                , "avg size"
            );

            printStatsLine("c XOR avg size"
                , (double)sumSizeXors/(double)foundXors
            );

            printStatsLine("c XOR 0-depth assings"
                , zeroDepthAssigns
            );

            printStatsLine("c XOR unit found"
                , newUnits
            );

            printStatsLine("c XOR bin found"
                , newBins
            );
            cout << "c --------- XOR STATS END ----------" << endl;
        }

        //Time
        double findTime;
        double extractTime;
        double blockCutTime;

        //XOR stats
        uint64_t foundXors;
        uint64_t sumSizeXors;
        uint64_t numVarsInBlocks;
        uint64_t numBlocks;

        //Usefulness stats
        uint64_t newUnits;
        uint64_t newBins;

        size_t zeroDepthAssigns;
    };

    const Stats& getStats() const;
    size_t getNumCalls() const;

private:
    //Find XORs
    void findXor(ClauseIndex c);
    void findXorMatch(const Occur& occ, FoundXors& foundCls); ///<Normal finding of matching clause for XOR
    void findXorMatchExt(const Occur& occ, FoundXors& foundCls); ///<Finding of matching clause for XOR with the twist that cache can be used to replace lits
    void findXorMatch(const vec<Watched>& ws, const Lit lit, FoundXors& foundCls) const;
    void findXorMatch(const vector<LitExtra>& lits, const Lit lit, FoundXors& foundCls) const;

    //const uint32_t tryToXor(const Xor& thisXor, const uint32_t thisIndex);
    bool mixXorAndGates();

    //Information extraction
    bool extractInfo();
    void cutIntoBlocks(const vector<size_t>& xorsToUse);
    bool extractInfoFromBlock(const vector<Var>& block, const size_t blockNum);
    vector<size_t> getXorsForBlock(const size_t blockNum);

    //Major calculated data and indexes to this data
    vector<Xor> xors; ///<Recovered XORs
    vector<vector<uint32_t> > xorOcc;
    vector<char> triedAlready; ///<These clauses have been tried to be made into an XOR. No point in tryin again
    vector<vector<Var> > blocks; ///<Blocks of vars that are in groups of XORs
    vector<size_t> varToBlock; ///<variable-> block index map

    Subsumer* subsumer;
    Solver *solver;

    //Stats
    Stats runStats;
    Stats globalStats;
    size_t numCalls;

    //Temporaries for putting xors into matrix, and extracting info from matrix
    vector<size_t> outerToInterVarMap;
    vector<size_t> interToOUterVarMap;

    //Other temporaries
    vector<char>& seen;
    vector<char>& seen2;
};


inline CL_ABST_TYPE FoundXors::getAbst() const
{
    return abst;
}

inline uint32_t FoundXors::getSize() const
{
    return size;
}

inline bool FoundXors::getRHS() const
{
    return rhs;
}

inline const Clause& FoundXors::getOrigCl() const
{
    return origCl;
}

template<class T> void FoundXors::add(const T& cl)
{
    assert(cl.size() <= size);

    vector<uint32_t> varsMissing; //If clause covers more than one combination, this is used to calculate which ones
    uint32_t origI = 0; //Position of literal in the ORIGINAL clause. This may be larger than the position in the current clause (as some literals could be missing)
    uint32_t i = 0; //Position in current clause
    uint32_t whichOne = 0; //Used to calculate this clause covers which combination(s)

    for (typename T::const_iterator l = cl.begin(), end = cl.end(); l != end; l++, i++, origI++) {
        //some variables might be missing
        while(cl[i].var() != origCl[origI].var()) {
            varsMissing.push_back(origI);
            origI++;
            assert(origI < origCl.size() && "cl must be sorted");
        }
        whichOne += ((uint32_t)l->sign()) << origI;
    }

    //set to true every combination for the missing variables
    for (uint32_t i = 0; i < 1UL<<(varsMissing.size()); i++) {
        uint32_t thisWhichOne = whichOne;
        for (uint32_t i2 = 0; i2 < varsMissing.size(); i2++) {
            if (bit(i, i2)) thisWhichOne+= 1<<(varsMissing[i2]);
        }
        foundComb[thisWhichOne] = true;
    }
}

inline bool FoundXors::foundAll() const
{
    bool OK = true;
    for (uint32_t i = 0; i < foundComb.size(); i++) {
        //Only count combinations with the correct RHS
        if ((NumberOfSetBits(i)%2) == (uint32_t)rhs) {
            continue;
        }

        //If this combination hasn't been found yet, then the XOR is not complete
        if (!foundComb[i]) {
            OK = false;
            break;
        }
    }
    return OK;
}

inline void FoundXors::add(Lit lit1, Lit lit2)
{
    //Make sure that order is correct
    if (lit1 > lit2)
        std::swap(lit1, lit2);

    uint32_t whichOne = 0;
    vector<Var> varsMissing;

    //whichOne is a PARTIAL bitfield representation of what this XOR covers
    for (uint32_t i = 0; i < origCl.size(); i++) {
        if (lit1.var() == origCl[i].var()) whichOne += ((uint32_t)lit1.sign()) << i;
        else if (lit2.var() == origCl[i].var()) whichOne += ((uint32_t)lit2.sign()) << i;
        else varsMissing.push_back(i);
    }
    //varsMissing now contains the INDEX of the variables that are missing

    //set to true every combination for the missing variables
    for (uint32_t i = 0; i < 1UL<<(varsMissing.size()); i++) {
        uint32_t thisWhichOne = whichOne;
        for (uint32_t i2 = 0; i2 < varsMissing.size(); i2++) {
            if (bit(i, i2)) thisWhichOne+= 1<<(varsMissing[i2]);
        }
        foundComb[thisWhichOne] = true;
    }
}

inline uint32_t FoundXors::NumberOfSetBits(uint32_t i) const
{
    //Magic is coming! (copied from some book.... never trust code like this!)
    i = i - ((i >> 1) & 0x55555555);
    i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
    return (((i + (i >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;
}

inline bool FoundXors::bit(const uint32_t a, const uint32_t b) const
{
    return (((a)>>(b))&1);
}

inline const XorFinder::Stats& XorFinder::getStats() const
{
    return globalStats;
}

inline size_t XorFinder::getNumCalls() const
{
    return numCalls;
}


#endif //_XORFINDER_H_
