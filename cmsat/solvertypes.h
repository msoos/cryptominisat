/*
 * forl
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

#include <sstream>
#include <algorithm>
#include <limits>
#include <vector>
#include <iostream>
#include <iomanip>
#include <string>
#include "assert.h"

namespace forl {

using std::vector;
using std::cout;
using std::endl;
using std::string;

//Typedefs
typedef uint32_t Var;
static const Var var_Undef(~0U);
enum RestartType {
    glue_restart
    , glue_agility_restart
    , geom_restart
    , agility_restart
    , no_restart
    , auto_restart
};

inline std::string restart_type_to_string(const RestartType type)
{
    switch(type) {
        case glue_restart:
            return "glue-based";

        case glue_agility_restart:
            return "glue&agility based";

        case geom_restart:
            return "geometric";

        case  agility_restart:
            return "agility-based";

        case no_restart:
            return "never restart";

        case auto_restart:
            return "automatic";
    }

    assert(false && "oops, one of the restart types has no string name");

    return "Ooops, none defined!";
}

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
    bool operator >=  (const Lit& p) const {
        return x >= p.x;
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

inline std::ostream& operator<<(std::ostream& co, const std::vector<Lit>& lits)
{
    for (uint32_t i = 0; i < lits.size(); i++) {
        co << lits[i];

        if (i != lits.size()-1)
            co << " ";
    }

    return co;
}


///Class that can hold: True, False, Undef
class lbool
{
    char     value;
    explicit lbool(const char v) : value(v) { }

public:
    lbool() :
        value(0)
    {
    };

    char getchar() const
    {
        return value;
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

struct BlockedClause {
    BlockedClause()
    {}

    BlockedClause(const Lit _blockedOn, const vector<Lit>& _lits) :
        blockedOn(_blockedOn)
        , toRemove(false)
        , lits(_lits)
    {}

    Lit blockedOn;
    bool toRemove;
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

template<class T, class T2> void printStatsLine(
    string left
    , T value
    , T2 value2
    , string extra
) {
    cout
    << std::fixed << std::left << std::setw(27) << left
    << ": " << std::setw(11) << std::setprecision(2) << value
    << " (" << std::left << std::setw(9) << std::setprecision(2) << value2
    << " " << extra << ")"
    << std::right
    << endl;
}

inline void printStatsLine(
    string left
    , uint64_t value
    , uint64_t value2
    , uint64_t value3
) {
    cout
    << std::fixed << std::left << std::setw(27) << left
    << ": " << std::setw(11) << std::setprecision(2) << value
    << "/" << value2
    << "/" << value3
    << std::right
    << endl;
}

template<class T, class T2> void printStatsLine(
    string left
    , T value
    , string extra1
    , T2 value2
    , string extra2
) {
    cout
    << std::fixed << std::left << std::setw(27) << left
    << ": " << std::setw(11) << std::setprecision(2) << value
    << " " << extra1
    << " (" << std::left << std::setw(9) << std::setprecision(2) << value2
    << " " << extra2 << ")"
    << std::right
    << endl;
}

template<class T> void printStatsLine(
    string left
    , T value
    , string extra = ""
) {
    cout
    << std::fixed << std::left << std::setw(27) << left
    << ": " << std::setw(11) << std::setprecision(2)
    << value
    << " " << extra
    << std::right
    << endl;
}

struct AssignStats
{
    AssignStats() :
        sumAssignPos(0)
        , sumAssignNeg(0)
        , sumFlippedPolar(0)
        , sumFlippedPolarByDecider(0)
    {};

    uint64_t sumAssignPos;
    uint64_t sumAssignNeg;
    uint64_t sumFlippedPolar;
    uint64_t sumFlippedPolarByDecider;

};

struct PropStats
{
    PropStats() :
        propagations(0)
        , bogoProps(0)
        , otfHyperTime(0)
        , otfHyperPropCalled(0)
        #ifdef STATS_NEEDED
        , propsUnit(0)
        , propsBinIrred(0)
        , propsBinRed(0)
        , propsTriIrred(0)
        , propsTriRed(0)
        , propsLongIrred(0)
        , propsLongRed(0)

        //LHBR
        , triLHBR(0)
        , longLHBR(0)
        #endif

        //Var setsing
        , varSetPos(0)
        , varSetNeg(0)
        , varFlipped(0)
    {
    }

    void clear()
    {
        PropStats tmp;
        *this = tmp;
    }

    PropStats& operator+=(const PropStats& other)
    {
        propagations += other.propagations;
        bogoProps += other.bogoProps;
        otfHyperTime += other.otfHyperTime;
        otfHyperPropCalled += other.otfHyperPropCalled;
        #ifdef STATS_NEEDED
        propsUnit += other.propsUnit;
        propsBinIrred += other.propsBinIrred;
        propsBinRed += other.propsBinRed;
        propsTriIrred += other.propsTriIrred;
        propsTriRed += other.propsTriRed;
        propsLongIrred += other.propsLongIrred;
        propsLongRed += other.propsLongRed;

        //LHBR
        triLHBR += other.triLHBR;
        longLHBR += other.longLHBR;
        #endif

        //Var settings
        varSetPos += other.varSetPos;
        varSetNeg += other.varSetNeg;
        varFlipped += other.varFlipped;

        return *this;
    }

    PropStats& operator-=(const PropStats& other)
    {
        propagations -= other.propagations;
        bogoProps -= other.bogoProps;
        otfHyperTime -= other.otfHyperTime;
        otfHyperPropCalled -= other.otfHyperPropCalled;
        #ifdef STATS_NEEDED
        propsUnit -= other.propsUnit;
        propsBinIrred -= other.propsBinIrred;
        propsBinRed -= other.propsBinRed;
        propsTriIrred -= other.propsTriIrred;
        propsTriRed -= other.propsTriRed;
        propsLongIrred -= other.propsLongIrred;
        propsLongRed -= other.propsLongRed;

        //LHBR
        triLHBR -= other.triLHBR;
        longLHBR -= other.longLHBR;
        #endif

        //Var settings
        varSetPos -= other.varSetPos;
        varSetNeg -= other.varSetNeg;
        varFlipped -= other.varFlipped;

        return *this;
    }

    PropStats operator-(const PropStats& other) const
    {
        PropStats result = *this;
        result -= other;
        return result;
    }

    PropStats operator+(const PropStats& other) const
    {
        PropStats result = *this;
        result += other;
        return result;
    }

    void print(const double cpu_time) const
    {
        cout << "c PROP stats" << endl;
        printStatsLine("c Mbogo-props", (double)bogoProps/(1000.0*1000.0)
            , (double)bogoProps/(cpu_time*1000.0*1000.0)
            , "/ sec"
        );

        printStatsLine("c MHyper-props", (double)otfHyperTime/(1000.0*1000.0)
            , (double)otfHyperTime/(cpu_time*1000.0*1000.0)
            , "/ sec"
        );

        printStatsLine("c Mprops", (double)propagations/(1000.0*1000.0)
            , (double)propagations/(cpu_time*1000.0*1000.0)
            , "/ sec"
        );

        #ifdef STATS_NEEDED
        printStatsLine("c propsUnit", propsUnit
            , 100.0*(double)propsUnit/(double)propagations
            , "% of propagations"
        );

        printStatsLine("c propsBinIrred", propsBinIrred
            , 100.0*(double)propsBinIrred/(double)propagations
            , "% of propagations"
        );

        printStatsLine("c propsBinRed", propsBinRed
            , 100.0*(double)propsBinRed/(double)propagations
            , "% of propagations"
        );

        printStatsLine("c propsTriIred", propsTriIrred
            , 100.0*(double)propsTriIrred/(double)propagations
            , "% of propagations"
        );

        printStatsLine("c propsTriRed", propsTriRed
            , 100.0*(double)propsTriRed/(double)propagations
            , "% of propagations"
        );

        printStatsLine("c propsLongIrred", propsLongIrred
            , 100.0*(double)propsLongIrred/(double)propagations
            , "% of propagations"
        );

        printStatsLine("c propsLongRed", propsLongRed
            , 100.0*(double)propsLongRed/(double)propagations
            , "% of propagations"
        );

        printStatsLine("c LHBR", (triLHBR + longLHBR)
            , 100.0*(double)(triLHBR + longLHBR)/(double)(
                propsLongIrred + propsLongRed + propsTriIrred + propsTriRed)
            , "% of long propagations"
        );

        printStatsLine("c LHBR only by 3-long", triLHBR
            , 100.0*(double)triLHBR/(double)(triLHBR + longLHBR)
            , "% of LHBR"
        );
        #endif

        printStatsLine("c varSetPos", varSetPos
            , 100.0*(double)varSetPos/(double)propagations
            , "% of propagations"
        );

        printStatsLine("c varSetNeg", varSetNeg
            , 100.0*(double)varSetNeg/(double)propagations
            , "% of propagations"
        );

        printStatsLine("c flipped", varFlipped
            , 100.0*(double)varFlipped/(double)propagations
            , "% of propagations"
        );

    }

    uint64_t propagations; ///<Number of propagations made
    uint64_t bogoProps;    ///<An approximation of time
    uint64_t otfHyperTime;
    uint32_t otfHyperPropCalled;

    #ifdef STATS_NEEDED
    //Stats for propagations
    uint64_t propsUnit;
    uint64_t propsBinIrred;
    uint64_t propsBinRed;
    uint64_t propsTriIrred;
    uint64_t propsTriRed;
    uint64_t propsLongIrred;
    uint64_t propsLongRed;

    //Lazy hyper-binary clause added
    uint64_t triLHBR; //LHBR by 3-long clauses
    uint64_t longLHBR; //LHBR by 3+-long clauses
    #endif

    //Var settings
    uint64_t varSetPos;
    uint64_t varSetNeg;
    uint64_t varFlipped;
};

enum ConflCausedBy {
    CONFL_BY_LONG_IRRED_CLAUSE
    , CONFL_BY_LONG_RED_CLAUSE
    , CONFL_BY_BIN_RED_CLAUSE
    , CONFL_BY_BIN_IRRED_CLAUSE
    , CONFL_BY_TRI_IRRED_CLAUSE
    , CONFL_BY_TRI_RED_CLAUSE
};

struct ConflStats
{
    ConflStats() :
        conflsBinIrred(0)
        , conflsBinRed(0)
        , conflsTriIrred(0)
        , conflsTriRed(0)
        , conflsLongIrred(0)
        , conflsLongRed(0)
        , numConflicts(0)
    {}

    void clear()
    {
        ConflStats tmp;
        *this = tmp;
    }

    ConflStats& operator+=(const ConflStats& other)
    {
        conflsBinIrred += other.conflsBinIrred;
        conflsBinRed += other.conflsBinRed;
        conflsTriIrred += other.conflsTriIrred;
        conflsTriRed += other.conflsTriRed;
        conflsLongIrred += other.conflsLongIrred;
        conflsLongRed += other.conflsLongRed;

        numConflicts += other.numConflicts;

        return *this;
    }

    ConflStats& operator-=(const ConflStats& other)
    {
        conflsBinIrred -= other.conflsBinIrred;
        conflsBinRed -= other.conflsBinRed;
        conflsTriIrred -= other.conflsTriIrred;
        conflsTriRed -= other.conflsTriRed;
        conflsLongIrred -= other.conflsLongIrred;
        conflsLongRed -= other.conflsLongRed;

        numConflicts -= other.numConflicts;

        return *this;
    }

    void update(const ConflCausedBy lastConflictCausedBy)
    {
        switch(lastConflictCausedBy) {
            case CONFL_BY_BIN_IRRED_CLAUSE :
                conflsBinIrred++;
                break;
            case CONFL_BY_BIN_RED_CLAUSE :
                conflsBinRed++;
                break;
            case CONFL_BY_TRI_IRRED_CLAUSE :
                conflsTriIrred++;
                break;
            case CONFL_BY_TRI_RED_CLAUSE :
                conflsTriRed++;
                break;
            case CONFL_BY_LONG_IRRED_CLAUSE :
                conflsLongIrred++;
                break;
            case CONFL_BY_LONG_RED_CLAUSE :
                conflsLongRed++;
                break;
            default:
                assert(false);
        }
    }

    void printShort(double cpu_time) const
    {
        //Search stats
        printStatsLine("c conflicts", numConflicts
            , (double)numConflicts/cpu_time
            , "/ sec"
        );
    }

    void print(double cpu_time) const
    {
        //Search stats
        cout << "c CONFLS stats" << endl;
        printShort(cpu_time);

        printStatsLine("c conflsBinIrred", conflsBinIrred
            , 100.0*(double)conflsBinIrred/(double)numConflicts
            , "%"
        );

        printStatsLine("c conflsBinRed", conflsBinRed
            , 100.0*(double)conflsBinRed/(double)numConflicts
            , "%"
        );

        printStatsLine("c conflsTriIrred", conflsTriIrred
            , 100.0*(double)conflsTriIrred/(double)numConflicts
            , "%"
        );

        printStatsLine("c conflsTriIrred", conflsTriRed
            , 100.0*(double)conflsTriRed/(double)numConflicts
            , "%"
        );

        printStatsLine("c conflsLongIrred" , conflsLongIrred
            , 100.0*(double)conflsLongIrred/(double)numConflicts
            , "%"
        );

        printStatsLine("c conflsLongRed", conflsLongRed
            , 100.0*(double)conflsLongRed/(double)numConflicts
            , "%"
        );

        long diff = (long)numConflicts
            - (long)(conflsBinIrred + conflsBinRed
                + (long)conflsTriIrred + (long)conflsTriRed
                + (long)conflsLongIrred + (long)conflsLongRed
            );

        if (diff != 0) {
            cout
            << "c DEBUG"
            << "((int)numConflicts - (int)(conflsBinIrred + conflsBinRed"
            << endl
            << "c  + conflsTriIrred + conflsTriRed + conflsLongIrred + conflsLongRed)"
            << " = "
            << (((int)numConflicts - (int)(conflsBinIrred + conflsBinRed
                + conflsTriIrred + conflsTriRed + conflsLongIrred + conflsLongRed)))
            << endl;

            //assert(diff == 0);
        }
    }

    uint64_t conflsBinIrred;
    uint64_t conflsBinRed;
    uint64_t conflsTriIrred;
    uint64_t conflsTriRed;
    uint64_t conflsLongIrred;
    uint64_t conflsLongRed;

    ///Number of conflicts
    uint64_t  numConflicts;
};

} //end namespace

#endif //SOLVERTYPES_H
