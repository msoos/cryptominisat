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

#ifndef SOLVERTYPESMINI__H
#define SOLVERTYPESMINI__H

#include <cstdint>
#include <iostream>
#include <cassert>
#include <memory>
#include <vector>
#include <array>
#include <gmpxx.h>

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
        if (i != lits.size()-1) co << " ";
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

    friend lbool toLbool(uint8_t   v);
    constexpr friend uint32_t   toInt  (lbool l);
};

constexpr lbool l_True = lbool((uint8_t)0);
constexpr lbool l_False = lbool((uint8_t)1);
constexpr lbool l_Undef = lbool((uint8_t)2);

inline lbool toLbool(uint8_t v)
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
        template<typename T>
        OrGate(const Lit& _rhs, const T& _lits, int32_t _ID) :
            lits(_lits), rhs(_rhs), id(_ID)

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
        int32_t id;
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
    std::vector<Lit>* _assumptions = nullptr;
    std::vector<uint32_t>* indic_to_var  = nullptr;
    uint32_t orig_num_vars = 0;
    std::vector<uint32_t>* non_indep_vars = nullptr;
    std::vector<uint32_t>* indep_vars = nullptr;
    bool fast_backw_on = false;
    uint32_t* test_var = nullptr;
    uint32_t* test_indic = nullptr;
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

struct VarMap {
    explicit VarMap() = default;
    explicit VarMap(const Lit l) : lit(l) {}
    explicit VarMap(const lbool v) : val(v) {}
    bool operator==(const VarMap& other) const { return lit == other.lit && val == other.val; }
    bool operator!=(const VarMap& other) const { return !(*this == other); }
    bool invariant() const {
        // Must be at least one of them
        if (lit == lit_Undef && val == l_Undef) return false;
        // Can't be both
        if (lit != lit_Undef && val != l_Undef) return false;
        return true;
    }
    Lit lit = lit_Undef;
    lbool val = l_Undef;
};

inline std::ostream& operator<<(std::ostream& os, const VarMap& v)
{
    assert(v.invariant());
    if (v.lit != lit_Undef) {
        os << "VarMap lit:" << v.lit;
        return os;
    } else {
        os << "VarMap val: " << v.val;
        return os;
    }
    return os;
}

class Field {
public:
    virtual ~Field() = default;

    // Virtual methods for field operations
    virtual Field& operator=(const Field& other) = 0;
    virtual Field& operator+=(const Field& other) = 0;
    virtual std::unique_ptr<Field> add(const Field& other) = 0;
    virtual Field& operator-=(const Field& other) = 0;
    virtual Field& operator*=(const Field& other) = 0;
    virtual Field& operator/=(const Field& other) = 0;
    virtual bool operator==(const Field&) const = 0;
    virtual bool is_zero() const = 0;
    virtual bool is_one() const = 0;
    virtual void set_zero() = 0;
    virtual void set_one() = 0;
    virtual uint64_t bytes_used() const = 0;
    virtual std::unique_ptr<Field> dup() const = 0;
    virtual bool parse(const std::string& str, const uint32_t line_no) = 0;

    // A method to display the value (for demonstration purposes)
    virtual std::ostream& display(std::ostream& os) const = 0;

    static void skip_whitespace(const std::string& str, uint32_t& at) {
        char c = str[at];
        while (c == '\t' || c == '\r' || c == ' ') {
            at++;
            c = str[at];
        }
    }

    static mpz_class parse_sign(const std::string& str, uint32_t& at) {
        mpz_class sign = 1;
        if (str[at] == '-') {sign = -1; at++;}
        else if (str[at] == '+') {sign = 1; at++;}
        return sign;
    }

    static bool parse_int(mpz_class& ret, const std::string& str, uint32_t& at,
            const size_t line_num, int* len = nullptr) {
        mpz_class val = 0;
        skip_whitespace(str, at);

        char c = str[at];
        if (c < '0' || c > '9') {
            std::cerr
            << "PARSE ERROR! Unexpected char (dec: '" << c << ")"
            << " At line " << line_num
            << " we expected a number"
            << std::endl;
            return false;
        }

        while (c >= '0' && c <= '9') {
            if (len) (*len)++;
            mpz_class val2 = val*10 + (c - '0');
            if (val2 < val) {
                std::cerr << "PARSE ERROR! At line " << line_num
                << " the variable number is to high"
                << std::endl;
                return false;
            }
            val = val2;
            c = str[++at];
        }
        ret = val;
        return true;
    }

    static bool check_end_of_weight(const std::string& str, uint32_t& at, const uint32_t line_no) {
        skip_whitespace(str, at);
        if (str[at] != '0') {
            std::cerr << "PARSE ERROR! expected 0 at the end of the weight."
                << "At line " << line_no
                << " -- did you forget to set the mode?" << std::endl;
            return false;
        }
        at++;
        skip_whitespace(str, at);
        if (at < str.size() && str[at] != '\n') {

            std::cerr << "PARSE ERROR! expected end of line at the end of the weight."
                << "At line " << line_no
                << " -- did you forget to set the mode?" << std::endl;
            return false;
        }
        return true;
    }
};

inline std::ostream& operator<<(std::ostream& os, const Field& f) {
    return f.display(os);
}

class FieldGen {
public:
    virtual ~FieldGen() = default;
    virtual std::unique_ptr<Field> zero() const = 0;
    virtual std::unique_ptr<Field> one() const = 0;
    virtual std::unique_ptr<FieldGen> dup() const = 0;
    virtual bool larger_than(const Field&, const Field&) const = 0;
    virtual bool weighted() const = 0;
};

class FDouble : public Field {
public:
    double val;
    FDouble(const double _val) : val(_val) {}
    FDouble(const FDouble& other) : val(other.val) {}

    Field& operator=(const Field& other) final {
        const auto& od = static_cast<const FDouble&>(other);
        val = od.val;
        return *this;
    }

    Field& operator+=(const Field& other) final {
        const auto& od = static_cast<const FDouble&>(other);
        val += od.val;
        return *this;
    }

    std::unique_ptr<Field> add(const Field& other) final {
        const auto& od = static_cast<const FDouble&>(other);
        return std::make_unique<FDouble>(val + od.val);
    }

    Field& operator-=(const Field& other) final {
        const auto& od = static_cast<const FDouble&>(other);
        val -= od.val;
        return *this;
    }

    Field& operator*=(const Field& other) final {
        const auto& od = static_cast<const FDouble&>(other);
        val *= od.val;
        return *this;
    }

    Field& operator/=(const Field& other) final {
        const auto& od = static_cast<const FDouble&>(other);
        if (od.val == 0) throw std::runtime_error("Division by zero");
        val /= od.val;
        return *this;
    }

    bool operator==(const Field& other) const final {
        const auto& od = static_cast<const FDouble&>(other);
        return od.val == val;
    }

    std::ostream& display(std::ostream& os) const final {
        os << val;
        return os;
    }

    std::unique_ptr<Field> dup() const final {
        return std::make_unique<FDouble>(val);
    }

    bool is_zero() const final { return val == 0; }
    bool is_one() const final { return val == 1; }
    void set_zero() final { val = 0; }
    void set_one() final { val = 1; }
    uint64_t bytes_used() const final { return sizeof(FDouble); }

    bool parse(const std::string& str, const uint32_t line_no) final {
        mpz_class head;
        mpz_class mult;
        uint32_t at = 0;
        auto sign = parse_sign(str, at);
        parse_int(head, str, at, line_no);
        mpq_class vald;
        if (str[at] == '.') {
            at++;
            mpz_class tail;
            int len = 0;
            if (!parse_int(tail, str, at, line_no, &len)) return false;
            mpz_class ten(10);
            mpz_ui_pow_ui(ten.get_mpz_t(), 10, len);
            mpq_class tenq(ten);
            mpq_class tailq(tail);
            vald = head + tailq/tenq;
        } else {
            vald = head;
        }
        vald *= sign;
        val = vald.get_d();
        return check_end_of_weight(str, at, line_no);
    }
};

class FGenDouble : public FieldGen {
public:
    ~FGenDouble() override = default;
    std::unique_ptr<Field> zero() const final {
        return std::make_unique<FDouble>(0);
    }

    std::unique_ptr<Field> one() const final {
        return std::make_unique<FDouble>(1.0);
    }

    std::unique_ptr<FieldGen> dup() const final {
        return std::make_unique<FGenDouble>();
    }

    bool larger_than(const Field& a, const Field& b) const final {
        const auto& ad = static_cast<const FDouble&>(a);
        const auto& bd = static_cast<const FDouble&>(b);
        return ad.val > bd.val;
    }

    bool weighted() const final { return true; }
};

}

#endif
