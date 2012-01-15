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

#ifndef SOLVERTYPES_H
#define SOLVERTYPES_H

#include "constants.h"
#include "Vec.h"

#include <iostream>
#include <vector>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <limits>
#include <vector>
using std::vector;

//Typedefs
typedef uint32_t Var;
static const Var var_Undef(0xffffffffU >>1);
enum RestType {glue_restart, geom_restart, agility_restart, branch_depth_delta_restart};
enum { polarity_true = 0, polarity_false = 1, polarity_rnd = 3, polarity_auto = 4};

/**
@brief A Literal, i.e. a variable with a sign
*/
class Lit
{
    uint32_t     x;
    explicit Lit(uint32_t i) : x(i) { };
public:
    Lit() : x(2*var_Undef) {}   // (lit_Undef)
    explicit Lit(Var var, bool sign) :
        x((2*var) | (uint32_t)sign)
    {}

    const uint32_t& toInt() const { // Guarantees small, positive integers suitable for array indexing.
        return x;
    }
    Lit  operator~() const {
        return Lit(x ^ 1);
    }
    Lit  operator^(const bool b) const {
        return Lit(x ^ (uint32_t)b);
    }
    Lit& operator^=(const bool b) {
        x ^= (uint32_t)b;
        return *this;
    }
    bool sign() const {
        return x & 1;
    }
    Var  var() const {
        return x >> 1;
    }
    Lit  unsign() const {
        return Lit(x & ~1);
    }
    bool operator==(const Lit& p) const {
        return x == p.x;
    }
    bool operator!= (const Lit& p) const {
        return x != p.x;
    }
    /**
    @brief ONLY to be used for ordering such as: a, b, ~b, etc.
    */
    bool operator <  (const Lit& p) const {
        return x < p.x;     // '<' guarantees that p, ~p are adjacent in the ordering.
    }
    bool operator >  (const Lit& p) const {
        return x > p.x;
    }
    static Lit toLit(uint32_t data)
    {
        return Lit(data);
    }
};

static const Lit lit_Undef(var_Undef, false);  // Useful special constants.
static const Lit lit_Error(var_Undef, true );  //

inline std::ostream& operator<<(std::ostream& os, const Lit lit)
{
    if (lit == lit_Undef) {
        os << "lit_Undef";
    } else {
        os << (lit.sign() ? "-" : "") << (lit.var() + 1);
    }
    return os;
}

inline std::ostream& operator<<(std::ostream& cout, const std::vector<Lit>& lits)
{
    for (uint32_t i = 0; i < lits.size(); i++) {
        cout << lits[i] << " ";
    }
    return cout;
}


///Class that can hold: True, False, Undef
class lbool
{
    char     value;
    explicit lbool(char v) : value(v) { }

public:
    lbool() :
        value(0)
    {
    };

    char getchar() const
    {
        return value;
    }

    bool isUndef() const
    {
        return !value;
    }

    bool isDef() const
    {
        return value;
    }

    bool getBool() const
    {
        return value == 1;
    }

    bool operator==(lbool b) const
    {
        return value == b.value;
    }

    bool operator!=(lbool b) const
    {
        return value != b.value;
    }

    lbool operator^(const char b) const
    {
        return b ? lbool(-value) : lbool(value);
    }
    //lbool operator ^ (const bool b) const { return b ? lbool(-value) : lbool(value); }

    friend lbool toLbool(const char v);
    friend lbool boolToLBool(const bool b);
    friend class llbool;
};
inline lbool toLbool(const char   v)
{
    return lbool(v);
}
inline lbool boolToLBool(const bool b)
{
    return lbool(2*b-1);
}

const lbool l_True  = toLbool( 1);
const lbool l_False = toLbool(-1);
const lbool l_Undef = toLbool( 0);

inline std::ostream& operator<<(std::ostream& cout, const lbool val)
{
    if (val == l_True) cout << "l_True";
    if (val == l_False) cout << "l_False";
    if (val == l_Undef) cout << "l_Undef";
    return cout;
}

struct PropData {
    bool learntStep; //Step that lead here from ancestor is learnt
    Lit ancestor;
    bool hyperBin; //It's a hyper-binary clause
    bool hyperBinNotAdded; //It's a hyper-binary clause, but was never added because all the rest was zero-level
};

struct BlockedClause {
    BlockedClause()
    {}

    BlockedClause(const Lit _blockedOn, const vector<Lit>& _lits) :
        blockedOn(_blockedOn)
        , lits(_lits)
    {}

    Lit blockedOn;
    vector<Lit> lits;
};

inline std::ostream& operator<<(std::ostream& os, const BlockedClause& bl)
{
    os << bl.lits << " blocked on: " << bl.blockedOn;

    return os;
}

class BinaryClause {
    public:
        BinaryClause(const Lit _lit1, const Lit _lit2, const bool _learnt) :
            lit1(_lit1)
            , lit2(_lit2)
            , learnt(_learnt)
        {
            if (lit1 > lit2) std::swap(lit1, lit2);
        }

        bool operator<(const BinaryClause& other) const
        {
            if (lit1 < other.lit1) return true;
            if (lit1 > other.lit1) return false;

            if (lit2 < other.lit2) return true;
            if (lit2 > other.lit2) return false;
            return (learnt && !other.learnt);
        }

        bool operator==(const BinaryClause& other) const
        {
            return (lit1 == other.lit1
                    && lit2 == other.lit2
                    && learnt == other.learnt);
        }

        const Lit getLit1() const
        {
            return lit1;
        }

        const Lit getLit2() const
        {
            return lit2;
        }

        bool getLearnt() const
        {
            return learnt;
        }

    private:
        Lit lit1;
        Lit lit2;
        bool learnt;
};


inline std::ostream& operator<<(std::ostream& os, const BinaryClause val)
{
    os << val.getLit1() << " , " << val.getLit2()
    << " learnt: " << std::boolalpha << val.getLearnt() << std::noboolalpha;
    return os;
}

class AgilityData
{
    public:
        AgilityData(
            const double _agilityG
            , const double agilityLimit
        ) :
            agilityG(_agilityG)
            , agility(agilityLimit)
        {
            assert(agilityG < 1 && agilityG > 0);
        }

        void update(const bool flipped)
        {
            agility *= agilityG;
            if (flipped)
                agility += 1.0 - agilityG;
        }

        double getAgility() const
        {
            return agility;
        }

        void reset(const double agilityLimit)
        {
            agility = agilityLimit;
        }

    private:
        const double agilityG;
        double agility;
};

struct SearchFuncParams
{
    SearchFuncParams(uint64_t _conflictsToDo, uint64_t _maxNumConfl = std::numeric_limits< uint64_t >::max(), const bool _update = true) :
        needToStopSearch(false)
        , conflictsDoneThisRestart(0)
        , conflictsToDo(_conflictsToDo)
        , maxNumConfl(_maxNumConfl)
        , update(_update)
    {}

    bool needToStopSearch;
    uint64_t conflictsDoneThisRestart;

    const uint64_t conflictsToDo;
    const uint64_t maxNumConfl;
    const bool update;
};

#endif //SOLVERTYPES_H
