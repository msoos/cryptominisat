/***************************************************************************************[Solver.cc]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2009, Niklas Sorensson
Copyright (c) 2009-2012, Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef __SOLVERTYPES_H__
#define __SOLVERTYPES_H__

#include <cassert>
#include <iostream>
#include "cmsat/Vec.h"
#include "cmsat/constants.h"

#include <stdio.h>
#include <vector>

//**********************************
// Variables, literals, lifted booleans, clauses:
//**********************************

// NOTE! Variables are just integers. No abstraction here. They should be chosen from 0..N,
// so that they can be used as array indices.

namespace CMSat {

typedef uint32_t Var;
static const uint32_t var_Undef = 0xffffffffU >>1;
enum RestartType {dynamic_restart, static_restart, auto_restart};

/**
@brief A Literal, i.e. a variable with a sign
*/
class Lit
{
    uint32_t     x;
    explicit Lit(uint32_t i) : x(i) { };
public:
    Lit() : x(2*var_Undef) {}   // (lit_Undef)
    explicit Lit(Var var, bool sign) : x((var+var) + (int)sign) { }

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
    inline void print(FILE* outfile = stdout) const
    {
        fprintf(outfile,"%s%d ", sign() ? "-" : "", var()+1);
    }
    inline void printFull(FILE* outfile = stdout) const
    {
        fprintf(outfile,"%s%d 0\n", sign() ? "-" : "", var()+1);
    }
    static Lit toLit(uint32_t data)
    {
        return Lit(data);
    }
};

static const Lit lit_Undef(var_Undef, false);  // Useful special constants.
static const Lit lit_Error(var_Undef, true );  //

inline std::ostream& operator<<(std::ostream& cout, const Lit& lit)
{
    cout << (lit.sign() ? "-" : "") << (lit.var() + 1);
    return cout;
}

inline std::ostream& operator<<(std::ostream& cout, const vec<Lit>& lits)
{
    for (uint32_t i = 0; i < lits.size(); i++) {
        cout << lits[i] << " ";
    }
    return cout;
}

inline void printClause(FILE* outFile, const std::vector<Lit>& clause)
{
    for (size_t i = 0; i < clause.size(); i++) {
        fprintf(outFile,"%s%d ", clause[i].sign() ? "-" : "", clause[i].var()+1);
    }
    fprintf(outFile, "0\n");
}

inline void printClause(FILE* outFile, const vec<Lit>& clause)
{
    for (uint32_t i = 0; i < clause.size(); i++) {
        fprintf(outFile,"%s%d ", clause[i].sign() ? "-" : "", clause[i].var()+1);
    }
    fprintf(outFile, "0\n");
}

//**********************************
// Lifted booleans
//**********************************

class llbool;

class lbool
{
    char     value;
    explicit lbool(char v) : value(v) { }

public:
    lbool()       : value(0) { };
    inline char getchar() const {
        return value;
    }
    inline lbool(llbool b);

    inline bool isUndef() const {
        return !value;
    }
    inline bool isDef() const {
        return value;
    }
    inline bool getBool() const {
        return value == 1;
    }
    inline bool operator==(lbool b) const {
        return value == b.value;
    }
    inline bool operator!=(lbool b) const {
        return value != b.value;
    }
    lbool operator^(const bool b) const {
        return b ? lbool(-value) : lbool(value);
    }

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


/**
@brief A very hackish lbool that also supports l_Nothing and l_Continue
*/
class llbool
{
    char value;

public:
    llbool(): value(0) {};
    llbool(lbool v) :
            value(v.value) {};
    llbool(char a) :
            value(a) {}

    inline bool operator!=(const llbool& v) const {
        return (v.value != value);
    }

    inline bool operator==(const llbool& v) const {
        return (v.value == value);
    }

    friend class lbool;
};
const llbool l_Nothing  = toLbool(2);
const llbool l_Continue = toLbool(3);

lbool::lbool(llbool b) : value(b.value) {}

inline std::ostream& operator<<(std::ostream& os, const llbool val)
{
    if (val == l_True) os << "l_True";
    if (val == l_False) os << "l_False";
    if (val == l_Undef) os << "l_Undef";
    if (val == l_Nothing) os << "l_Nothing";
    if (val == l_Continue) os << "l_Continue";
    return os;
}

enum { polarity_true = 0, polarity_false = 1, polarity_rnd = 3, polarity_auto = 4};

struct BinPropData {
    uint32_t lev;
    Lit lev1Ancestor;
    bool learntLeadHere;
    bool hasChildren;
};

}

#endif //SOLVERTYPES_H
