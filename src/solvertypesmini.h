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

#ifndef SOLVERTYPESMINI_H
#define SOLVERTYPESMINI_H

#include <cstdint>
#include <iostream>
#include <cassert>
#include <vector>
#include <array>
#include <algorithm>

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
    template <class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/) {
        ar & x;
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

    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/) {
        ar & value;
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

class OrGate {
    public:
        OrGate(const Lit& _rhs, const std::vector<Lit>& _lits, int32_t _ID) :
            lits(_lits), rhs(_rhs), ID(_ID)

        {
            std::sort(lits.begin(), lits.end());
        }

        bool operator==(const OrGate& other) const
        {
            return rhs == other.rhs && lits == other.lits;
        }
        const std::vector<Lit>& get_lhs() const
        {
            return lits;
        }

        //LHS
        std::vector<Lit> lits;

        //RHS
        Lit rhs;

        //ID of long clause
        int32_t ID;
};

class ITEGate {
    public:
        ITEGate() {}
        ITEGate(const Lit& _rhs, Lit _lit1, Lit _lit2, Lit _lit3) :
            rhs(_rhs)
        {
            lhs[0] = _lit1;
            lhs[1] = _lit2;
            lhs[2] = _lit3;
            std::sort(lhs.begin(), lhs.end());
        }

        std::array<Lit, 4> get_all() const
        {
            return std::array<Lit, 4>{{lhs[0], lhs[1], lhs[2], rhs}};
        }

        //LHS
        std::array<Lit, 3> lhs;

        //RHS
        Lit rhs;
};

enum class PolarityMode {
    polarmode_pos
    , polarmode_neg
    , polarmode_rnd
    , polarmode_automatic
    , polarmode_stable
    , polarmode_best_inv
    , polarmode_best
    , polarmode_saved
    , polarmode_weighted
};

enum class rst_dat_type {norm, var, cl};

struct FastBackwData {
    std::vector<Lit>* _assumptions = NULL;
    std::vector<uint32_t>* indic_to_var  = NULL;
    uint32_t orig_num_vars = 0;
    std::vector<uint32_t>* non_indep_vars = NULL;
    std::vector<uint32_t>* indep_vars = NULL;
    bool fast_backw_on = false;
    uint32_t* test_var = NULL;
    uint32_t* test_indic = NULL;
    uint32_t max_confl = 500;
    uint32_t cur_max_confl = 0;
    uint32_t indep_because_ran_out_of_confl = 0;
    uint64_t start_sumConflicts;
};

class BNN
{
public:
    typedef Lit* iterator;
    typedef const Lit* const_iterator;

    BNN()
    {}

    explicit BNN(
        const std::vector<Lit>& _in,
        const int32_t _cutoff,
        const Lit _out):
        cutoff(_cutoff),
        out (_out)
    {
        if (out == lit_Undef) {
            set = true;
        }
        assert(_in.size() > 0);
        undefs = _in.size();
        ts = 0;
        sz = _in.size();
        for(uint32_t i = 0; i < _in.size(); i++) {
            getData()[i] = _in[i];
        }
    }

    Lit* getData()
    {
        return (Lit*)((char*)this + sizeof(BNN));
    }

    const Lit* getData() const
    {
        return (Lit*)((char*)this + sizeof(BNN));
    }

    const Lit& operator[](const uint32_t at) const
    {
        return getData()[at];
    }

    Lit& operator[](const uint32_t at)
    {
        return getData()[at];
    }

    const Lit& get_out() const
    {
        return out;
    }

    const Lit* begin() const
    {
        return getData();
    }

    Lit* begin()
    {
        return getData();
    }

    const Lit* end() const
    {
        return getData()+size();
    }

    Lit* end()
    {
        return getData()+size();
    }

    bool empty() const
    {
        return sz == 0;
    }

    void resize(uint32_t _sz) {
        sz = _sz;
    }

    uint32_t size() const {
        return sz;
    }

    int32_t cutoff;
    Lit out = lit_Undef;
    bool set = false;
    bool isRemoved = false;
    int32_t ts = 0;
    int32_t undefs = 0;
    uint32_t sz;
};

inline std::ostream& operator<<(std::ostream& os, const BNN& bnn)
{
    for (uint32_t i = 0; i < bnn.size(); i++) {
        os << "lit[" << bnn[i] << "]";

        if (i+1 < bnn.size())
            os << " + ";
    }
    os << " >=  " << bnn.cutoff;
    if (!bnn.set)
        os << " <-> " << bnn.out;
    os << " [size: " << bnn.size() << "]";

    return os;
}

}

#endif //SOLVERTYPESMINI_H
