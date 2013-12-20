#include <vector>

namespace CMSat {
    class Solver;
};

namespace CryptoMiniSat {
    class Lit
    {
        unsigned x;
        explicit Lit(const unsigned _x) :
            x(_x)
        {}
    public:
        explicit Lit(unsigned var, bool sign) :
            x((2*var) | (unsigned)sign)
        {}

        const unsigned& toInt() const { // Guarantees small, positive integers suitable for array indexing.
            return x;
        }
        Lit operator~() const {
            return Lit(x ^ 1);
        }
        Lit operator^(const bool b) const {
            return Lit(x ^ (unsigned)b);
        }
        Lit& operator^=(const bool b) {
            x ^= (unsigned)b;
            return *this;
        }
        bool sign() const {
            return x & 1;
        }
        unsigned var() const {
            return x >> 1;
        }
        bool operator==(const Lit p) const {
            return x == p.x;
        }
        bool operator!= (const Lit p) const {
            return x != p.x;
        }
    };

    class lbool
    {
        char     value;
        explicit lbool(const char v) : value(v) { }

    public:
        lbool() :
            value(0)
        {
        };

        bool operator==(lbool b) const
        {
            return value == b.value;
        }

        bool operator!=(lbool b) const
        {
            return value != b.value;
        }

        friend lbool toLbool(const char v);
    };
    inline lbool toLbool(const char v)
    {
        return lbool(v);
    }

    const lbool l_True  = toLbool( 1);
    const lbool l_False = toLbool(-1);
    const lbool l_Undef = toLbool( 0);

    class Solver
    {
    public:
        Solver();
        bool add_clause(const std::vector<Lit>& lits);
        void new_var();
        lbool solve(std::vector<Lit>* assumptions = 0);
        const std::vector<lbool>& get_model() const;
        const std::vector<Lit>& get_unitary_clauses() const;
    };

    CMSat::Solver* solver;
};
