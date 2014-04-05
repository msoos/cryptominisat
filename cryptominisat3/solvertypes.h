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
#include <limits>
#include "assert.h"
#include "solverconf.h"
#include "solvertypesmini.h"

namespace CMSat {

using std::vector;
using std::cout;
using std::endl;
using std::string;

inline std::string restart_type_to_string(const Restart type)
{
    switch(type) {
        case restart_type_glue:
            return "glue-based";

        case restart_type_glue_agility:
            return "glue&agility based";

        case restart_type_geom:
            return "geometric";

        case restart_type_agility:
            return "agility-based";

        case restart_type_never:
            return "never restart";

        case restart_type_automatic:
            return "automatic";
    }

    assert(false && "oops, one of the restart types has no string name");

    return "Ooops, undefined!";
}

//Removed by which algorithm. NONE = not eliminated
enum class Removed : unsigned char {
    none
    , elimed
    , replaced
    , queued_replacer //Only queued for removal. NOT actually removed
    , decomposed
};

inline std::string removed_type_to_string(const Removed removed) {
    switch(removed) {
        case Removed::none:
            return "not removed";

        case Removed::elimed:
            return "variable elimination";

        case Removed::replaced:
            return "variable replacement";

        case Removed::queued_replacer:
            return "queued for replacement (but not yet replaced)";

        case Removed::decomposed:
            return "decomposed into another component";
    }

    assert(false && "oops, one of the elim types has no string name");
    return "Oops, undefined!";
}

class BinaryClause {
    public:
        BinaryClause(const Lit _lit1, const Lit _lit2, const bool _red) :
            lit1(_lit1)
            , lit2(_lit2)
            , red(_red)
        {
            if (lit1 > lit2) std::swap(lit1, lit2);
        }

        bool operator<(const BinaryClause& other) const
        {
            if (lit1 < other.lit1) return true;
            if (lit1 > other.lit1) return false;

            if (lit2 < other.lit2) return true;
            if (lit2 > other.lit2) return false;
            return (red && !other.red);
        }

        bool operator==(const BinaryClause& other) const
        {
            return (lit1 == other.lit1
                    && lit2 == other.lit2
                    && red == other.red);
        }

        const Lit getLit1() const
        {
            return lit1;
        }

        const Lit getLit2() const
        {
            return lit2;
        }

        bool isRed() const
        {
            return red;
        }

    private:
        Lit lit1;
        Lit lit2;
        bool red;
};


inline std::ostream& operator<<(std::ostream& os, const BinaryClause val)
{
    os << val.getLit1() << " , " << val.getLit2()
    << " red: " << std::boolalpha << val.isRed() << std::noboolalpha;
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

inline double stats_line_percent(double num, double total)
{
    if (total == 0) {
        return 0;
    } else {
        return num/total*100.0;
    }
}

inline void print_value_kilo_mega(const uint64_t value)
{
    if (value > 20*1000ULL*1000ULL) {
        cout << " " << std::setw(4) << value/(1000ULL*1000ULL) << "M";
    } else if (value > 20ULL*1000ULL) {
        cout << " " << std::setw(4) << value/1000 << "K";
    } else {
        cout << " " << std::setw(5) << value;
    }
}

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
    {}

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
            , stats_line_percent(propsUnit, propagations)
            , "% of propagations"
        );

        printStatsLine("c propsBinIrred", propsBinIrred
            , stats_line_percent(propsBinIrred, propagations)
            , "% of propagations"
        );

        printStatsLine("c propsBinRed", propsBinRed
            , stats_line_percent(propsBinRed, propagations)
            , "% of propagations"
        );

        printStatsLine("c propsTriIred", propsTriIrred
            , stats_line_percent(propsTriIrred, propagations)
            , "% of propagations"
        );

        printStatsLine("c propsTriRed", propsTriRed
            , stats_line_percent(propsTriRed, propagations)
            , "% of propagations"
        );

        printStatsLine("c propsLongIrred", propsLongIrred
            , stats_line_percent(propsLongIrred, propagations)
            , "% of propagations"
        );

        printStatsLine("c propsLongRed", propsLongRed
            , stats_line_percent(propsLongRed, propagations)
            , "% of propagations"
        );

        printStatsLine("c LHBR", (triLHBR + longLHBR)
            , stats_line_percent(triLHBR + longLHBR,
                propsLongIrred + propsLongRed + propsTriIrred + propsTriRed)
            , "% of long propagations"
        );

        printStatsLine("c LHBR only by 3-long", triLHBR
            , stats_line_percent(triLHBR, triLHBR + longLHBR)
            , "% of LHBR"
        );
        #endif

        printStatsLine("c varSetPos", varSetPos
            , stats_line_percent(varSetPos, propagations)
            , "% of propagations"
        );

        printStatsLine("c varSetNeg", varSetNeg
            , stats_line_percent(varSetNeg, propagations)
            , "% of propagations"
        );

        printStatsLine("c flipped", varFlipped
            , stats_line_percent(varFlipped, propagations)
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

enum class ConflCausedBy {
    longirred
    , longred
    , binred
    , binirred
    , triirred
    , trired
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
            case ConflCausedBy::binirred :
                conflsBinIrred++;
                break;
            case ConflCausedBy::binred :
                conflsBinRed++;
                break;
            case ConflCausedBy::triirred :
                conflsTriIrred++;
                break;
            case ConflCausedBy::trired :
                conflsTriRed++;
                break;
            case ConflCausedBy::longirred :
                conflsLongIrred++;
                break;
            case ConflCausedBy::longred :
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
            , stats_line_percent(conflsBinIrred, numConflicts)
            , "%"
        );

        printStatsLine("c conflsBinRed", conflsBinRed
            , stats_line_percent(conflsBinRed, numConflicts)
            , "%"
        );

        printStatsLine("c conflsTriIrred", conflsTriIrred
            , stats_line_percent(conflsTriIrred, numConflicts)
            , "%"
        );

        printStatsLine("c conflsTriIrred", conflsTriRed
            , stats_line_percent(conflsTriRed, numConflicts)
            , "%"
        );

        printStatsLine("c conflsLongIrred" , conflsLongIrred
            , stats_line_percent(conflsLongIrred, numConflicts)
            , "%"
        );

        printStatsLine("c conflsLongRed", conflsLongRed
            , stats_line_percent(conflsLongRed, numConflicts)
            , "%"
        );

        long diff = (long)numConflicts
            - (long)(conflsBinIrred + (long)conflsBinRed
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

inline void orderLits(
    Lit& lit1
    , Lit& lit2
    , Lit& lit3
 ) {
    if (lit1 > lit3)
        std::swap(lit1, lit3);

    if (lit1 > lit2)
        std::swap(lit1, lit2);

    if (lit2 > lit3)
        std::swap(lit2, lit3);

    //They are now ordered
    assert(lit1 < lit2);
    assert(lit2 < lit3);
}

inline vector<Lit> sortLits(const vector<Lit>& lits)
{
    vector<Lit> tmp(lits);

    std::sort(tmp.begin(), tmp.end());
    return tmp;
}


} //end namespace

#endif //SOLVERTYPES_H
