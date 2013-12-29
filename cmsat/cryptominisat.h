#ifndef __CRYPTOMINISAT_H__
#define __CRYPTOMINISAT_H__

#include <vector>
#include <iostream>
#include "solverconf.h"

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
        bool operator<(const Lit other) const {
            return x < other.x;
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

    inline std::ostream& operator<<(std::ostream& os, const Lit lit)
    {
        os << (lit.sign() ? "-" : "") << (lit.var() + 1);
        return os;
    }

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

    inline std::ostream& operator<<(std::ostream& cout, const lbool val)
    {
        if (val == l_True) cout << "l_True";
        if (val == l_False) cout << "l_False";
        if (val == l_Undef) cout << "l_Undef";
        return cout;
    }

    class Solver
    {
    public:
        Solver(SolverConf conf = SolverConf());
        uint32_t nVars() const;
        bool add_clause(const std::vector<Lit>& lits);
        void new_var();
        lbool solve(std::vector<Lit>* assumptions = 0);
        const std::vector<lbool>& get_model() const;
        const std::vector<Lit>& get_conflict() const;
        const std::vector<Lit>& get_unitary_clauses() const;
        void add_file(const std::string& filename);
        SolverConf get_conf() const;
        std::string get_version() const;
        void print_stats() const;
        void set_drup(std::ostream* os);
    private:
        void* solver;
    };
};

#endif //__CRYPTOMINISAT_H__
