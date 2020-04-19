/******************************************
Copyright (c) 2016, Mate Soos

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

#ifndef __SOLVERTYPESMINI_H__
#define __SOLVERTYPESMINI_H__

#include <cstdint>
#include <iostream>
#include <cassert>
#include <vector>

namespace CMSat {

constexpr uint32_t var_Undef(0xffffffffU >> 4);

class TooManyVarsError {};
class TooLongClauseError {};

class Lit
{
    uint32_t x;
    constexpr explicit Lit(uint32_t i) : x(i) { }
public:
    constexpr Lit() : x(var_Undef<<1) {}   // (lit_Undef)
    constexpr explicit Lit(uint32_t var, bool is_inverted) :
        x(var + var + is_inverted)
    {}

    constexpr const uint32_t& toInt() const { // Guarantees small, positive integers suitable for array indexing.
        return x;
    }
    constexpr Lit  operator~() const {
        return Lit(x ^ 1);
    }
    constexpr Lit  operator^(const bool b) const {
        return Lit(x ^ (uint32_t)b);
    }
    Lit& operator^=(const bool b) {
        x ^= (uint32_t)b;
        return *this;
    }
    constexpr bool sign() const {
        return x & 1;
    }
    constexpr uint32_t  var() const {
        return x >> 1;
    }
    constexpr Lit  unsign() const {
        return Lit(x & ~1U);
    }
    constexpr bool operator==(const Lit& p) const {
        return x == p.x;
    }
    constexpr bool operator!= (const Lit& p) const {
        return x != p.x;
    }
    /**
    @brief ONLY to be used for ordering such as: a, b, ~b, etc.
    */
    constexpr bool operator <  (const Lit& p) const {
        return x < p.x;     // '<' guarantees that p, ~p are adjacent in the ordering.
    }
    constexpr bool operator >  (const Lit& p) const {
        return x > p.x;
    }
    constexpr bool operator >=  (const Lit& p) const {
        return x >= p.x;
    }
    constexpr static Lit toLit(uint32_t data)
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

class lbool {
    uint8_t value;

public:
    constexpr explicit lbool(uint8_t v) : value(v) { }
    constexpr lbool()       : value(0) { }
    constexpr explicit lbool(bool x) : value(!x) { }

    constexpr bool  operator == (lbool b) const {
        return ((b.value & 2) & (value & 2)) | (!(b.value & 2) & (value == b.value));
    }
    constexpr bool  operator != (lbool b) const {
        return !(*this == b);
    }
    constexpr lbool operator ^  (bool  b) const {
        return lbool((uint8_t)(value ^ (uint8_t)b));
    }

    lbool operator && (lbool b) const {
        uint8_t sel = (value << 1) | (b.value << 3);
        uint8_t v   = (0xF7F755F4 >> sel) & 3;
        return lbool(v);
    }

    lbool operator || (lbool b) const {
        uint8_t sel = (value << 1) | (b.value << 3);
        uint8_t v   = (0xFCFCF400 >> sel) & 3;
        return lbool(v);
    }

    constexpr uint8_t getValue() const { return value; }

    friend lbool toLbool(uint32_t   v);
    constexpr friend uint32_t   toInt  (lbool l);
};

constexpr lbool l_True = lbool((uint8_t)0);
constexpr lbool l_False = lbool((uint8_t)1);
constexpr lbool l_Undef = lbool((uint8_t)2);

inline lbool toLbool(uint32_t v)
{
    lbool l;
    l.value = v;
    return l;
}


constexpr inline uint32_t toInt  (lbool l)
{
    return l.value;
}

inline lbool boolToLBool(const bool b)
{
    if (b) {
        return l_True;
    } else {
        return l_False;
    }
}

inline std::ostream& operator<<(std::ostream& cout, const lbool val)
{
    if (val == l_True) cout << "l_True";
    if (val == l_False) cout << "l_False";
    if (val == l_Undef) cout << "l_Undef";
    return cout;
}

enum class rst_dat_type {norm, var, cl};

}

#endif //__SOLVERTYPESMINI_H__
