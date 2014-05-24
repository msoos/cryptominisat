/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.0 of the License.
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
#include <set>
#include "cset.h"
#include "xorfinderabst.h"
#include "watcharray.h"

namespace CMSat {

using std::vector;
using std::set;

//#define VERBOSE_DEBUG_XOR_FINDER

class Solver;
class Simplifier;

class Xor
{
    public:
        Xor(const vector<Lit>& cl, const bool _rhs) :
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
        os << Lit(thisXor.vars[i], false);

        if (i+1 < thisXor.vars.size())
            os << " + ";
    }
    os << " =  " << std::boolalpha << thisXor.rhs << std::noboolalpha;

    return os;
}

class FoundXors
{
    public:
        FoundXors(
            const vector<Lit>& cl
            , cl_abst_type _abst
            , vector<uint16_t>& seen
        ) :
            abst(_abst)
            , size(cl.size())
        {
            #ifdef VERBOSE_DEBUG_XOR_FINDER
            cout << "Trying to create XOR from clause: " << cl << endl;
            #endif

            assert(cl.size() < sizeof(origCl)/sizeof(Lit));
            for(size_t i = 0; i < size; i++) {
                origCl[i] = cl[i];
                if (i > 0)
                    assert(cl[i-1] < cl[i]);
            }

            calcClauseData(seen);
        }

        //GET-type functions
        cl_abst_type      getAbst() const;
        uint32_t          getSize() const;
        bool              getRHS() const;
        bool              foundAll() const;

        //Add
        template<class T> void add(const T& cl, vector<uint32_t>& varsMissing);

    private:
        void calcClauseData(vector<uint16_t>& seen)
        {
            //Calculate parameters of base clause.
            //Also set 'seen' for easy check in 'findXorMatch()'
            rhs = true;
            uint32_t whichOne = 0;
            for (uint32_t i = 0; i < size; i++) {
                rhs ^= origCl[i].sign();
                whichOne += ((uint32_t)origCl[i].sign()) << i;
                seen[origCl[i].var()] = 1;
            }

            foundComb.resize(1UL<<size, false);
            foundComb[whichOne] = true;
        }
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
        Lit origCl[5];
        const cl_abst_type abst;
        uint32_t size;
        bool rhs;
};

class XorFinder: public XorFinderAbst
{
public:
    XorFinder(Simplifier* subsumer, Solver* solver);
    virtual ~XorFinder() {}
    virtual bool findXors();

    struct Stats
    {
        void clear()
        {
            Stats tmp;
            *this = tmp;
        }

        double totalTime() const
        {
            return findTime + extractTime + blockCutTime;
        }

        Stats& operator+=(const Stats& other);
        void printShort() const;
        void print(const size_t numCalls) const;

        //Time
        double findTime = 0.0;
        double extractTime = 0.0;
        double blockCutTime = 0.0;

        //XOR stats
        uint64_t foundXors = 0;
        uint64_t sumSizeXors = 0;
        uint64_t numVarsInBlocks = 0;
        uint64_t numBlocks = 0;

        //Usefulness stats
        uint64_t time_outs = 0;
        uint64_t newUnits = 0;
        uint64_t newBins = 0;

        size_t zeroDepthAssigns = 0;
    };

    const Stats& getStats() const;
    size_t getNumCalls() const;
    virtual size_t memUsed() const;

private:

    int64_t maxTimeFindXors;

    //Find XORs
    void findXor(vector<Lit>& lits, cl_abst_type abst);

    ///Normal finding of matching clause for XOR
    void findXorMatch(
        watch_subarray_const occ
        , FoundXors& foundCls);

    ///Finding of matching clause for XOR with the twist that cache can be used to replace lits
    void findXorMatch(
        watch_subarray_const ws
        , const Lit lit
        , FoundXors& foundCls
    );
    void findXorMatchExt(
        watch_subarray_const occ
        , const Lit lit
        , FoundXors& foundCls
    );
    //TODO stamping finXorMatch with stamp
    /*void findXorMatch(
        const vector<LitExtra>& lits
        , const Lit lit
        , FoundXors& foundCls
    ) const;*/

    //Information extraction
    bool extractInfo();
    void cutIntoBlocks(const vector<size_t>& xorsToUse);
    bool extractInfoFromBlock(const vector<Var>& block, const size_t blockNum);
    vector<size_t> getXorsForBlock(const size_t blockNum);

    //Major calculated data and indexes to this data
    vector<Xor> xors; ///<Recovered XORs
    vector<vector<uint32_t> > xorOcc;
    std::set<ClOffset> triedAlready; ///<These clauses have been tried to be made into an XOR. No point in tryin again
    vector<vector<Var> > blocks; ///<Blocks of vars that are in groups of XORs
    vector<size_t> varToBlock; ///<variable-> block index map

    Simplifier* subsumer;
    Solver *solver;

    //Stats
    Stats runStats;
    Stats globalStats;
    size_t numCalls;

    //Temporary
    vector<Lit> tmpClause;
    vector<uint32_t> varsMissing;

    //Temporaries for putting xors into matrix, and extracting info from matrix
    vector<size_t> outerToInterVarMap;
    vector<size_t> interToOUterVarMap;

    //Other temporaries
    vector<uint16_t>& seen;
    vector<uint16_t>& seen2;
};


inline cl_abst_type FoundXors::getAbst() const
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

template<class T> void FoundXors::add(
    const T& cl
    , vector<uint32_t>& varsMissing
) {
    #ifdef VERBOSE_DEBUG_XOR_FINDER
    cout << "Adding to XOR: " << cl << endl;

    cout << "FoundComb before:" << endl;
    for(size_t i = 0; i < foundComb.size(); i++) {
        cout << "foundComb[" << i << "]: " << foundComb[i] << endl;
    }
    cout << "----" << endl;
    #endif

    assert(cl.size() <= size);

    //If clause covers more than one combination, this is used to calculate which ones
    varsMissing.clear();

    //Position of literal in the ORIGINAL clause.
    //This may be larger than the position in the current clause (as some literals could be missing)
    uint32_t origI = 0;

    //Position in current clause
    uint32_t i = 0;

    //Used to calculate this clause covers which combination(s)
    uint32_t whichOne = 0;

    bool thisRhs = true;

    for (typename T::const_iterator
        l = cl.begin(), end = cl.end()
        ; l != end
        ; l++, i++, origI++
    ) {
        thisRhs ^= l->sign();

        //some variables might be missing in the middle
        while(cl[i].var() != origCl[origI].var()) {
            varsMissing.push_back(origI);
            origI++;
            assert(origI < size && "cl must be sorted");
        }
        whichOne += ((uint32_t)l->sign()) << origI;
    }

    //if vars are missing from the end
    while(origI < size) {
        varsMissing.push_back(origI);
        origI++;
    }

    assert(cl.size() < size || rhs == thisRhs);

    //set to true every combination for the missing variables
    for (uint32_t i = 0; i < 1UL<<(varsMissing.size()); i++) {
        uint32_t thisWhichOne = whichOne;
        for (uint32_t i2 = 0; i2 < varsMissing.size(); i2++) {
            if (bit(i, i2)) thisWhichOne+= 1<<(varsMissing[i2]);
        }
        foundComb[thisWhichOne] = true;
    }

    #ifdef VERBOSE_DEBUG_XOR_FINDER
    cout << "whichOne was:" << whichOne << endl;
    cout << "FoundComb after:" << endl;
    for(size_t i = 0; i < foundComb.size(); i++) {
        cout << "foundComb[" << i << "]: " << foundComb[i] << endl;
    }
    cout << "----" << endl;
    #endif
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

    #ifdef VERBOSE_DEBUG_XOR_FINDER
    if (OK) {
        cout << "Found all for this clause" << endl;
    }
    #endif

    return OK;
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

} //end namespace

#endif //_XORFINDER_H_
