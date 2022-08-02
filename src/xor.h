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

#ifndef _XOR_H_
#define _XOR_H_

#include "solvertypes.h"

#include <vector>
#include <set>
#include <iostream>
#include <algorithm>
#ifdef USE_TBUDDY
#include <pseudoboolean.h>
namespace tbdd = trustbdd;
#endif

using std::vector;
using std::set;


namespace CMSat {

class Xor
{
public:
    Xor()
    {}

    explicit Xor(const vector<uint32_t>& cl, const bool _rhs, const vector<uint32_t>& _clash_vars):
        rhs(_rhs)
        , clash_vars(_clash_vars)
    {
        for (uint32_t i = 0; i < cl.size(); i++) vars.push_back(cl[i]);
    }

#ifdef USE_TBUDDY
    tbdd::xor_constraint* create_bdd_xor()
    {
        if (bdd == NULL) {
            ilist l = ilist_new(vars.size());
            ilist_resize(l, vars.size());
            for (uint32_t i = 0; i < vars.size(); i++) l[i] = vars[i]+1;
            bdd = new tbdd::xor_constraint(l, rhs);
        }
        return bdd;
    }
#endif

    template<typename T>
    explicit Xor(const T& cl, const bool _rhs, const vector<uint32_t>& _clash_vars):
        rhs(_rhs)
        , clash_vars(_clash_vars)
    {
        for (uint32_t i = 0; i < cl.size(); i++) {
            vars.push_back(cl[i].var());
        }
    }

    explicit Xor(const vector<uint32_t>& cl, const bool _rhs, const uint32_t clash_var):
        rhs(_rhs)
    {
        clash_vars.push_back(clash_var);
        for (uint32_t i = 0; i < cl.size(); i++) {
            vars.push_back(cl[i]);
        }
    }

    ~Xor()
    {
    }

    vector<uint32_t>::const_iterator begin() const
    {
        return vars.begin();
    }

    vector<uint32_t>::const_iterator end() const
    {
        return vars.end();
    }

    vector<uint32_t>::iterator begin()
    {
        return vars.begin();
    }

    vector<uint32_t>::iterator end()
    {
        return vars.end();
    }

    bool operator<(const Xor& other) const
    {
        uint64_t i = 0;
        while(i < other.size() && i < size()) {
            if (other[i] != vars[i]) {
                return (vars[i] < other[i]);
            }
            i++;
        }

        if (other.size() != size()) {
            return size() < other.size();
        }
        return false;
    }

    const uint32_t& operator[](const uint32_t at) const
    {
        return vars[at];
    }

    uint32_t& operator[](const uint32_t at)
    {
        return vars[at];
    }

    void resize(const uint32_t newsize)
    {
        vars.resize(newsize);
    }

    vector<uint32_t>& get_vars()
    {
        return vars;
    }

    const vector<uint32_t>& get_vars() const
    {
        return vars;
    }

    size_t size() const
    {
        return vars.size();
    }

    bool empty() const
    {
        if (!vars.empty())
            return false;

        if (!clash_vars.empty())
            return false;

        if (rhs != false) {
            return false;
        }

        return true;
    }

    void merge_clash(const Xor& other, vector<uint32_t>& seen) {
        for(const auto& v: clash_vars) {
            seen[v] = 1;
        }

        for(const auto& v: other.clash_vars) {
            if (!seen[v]) {
                seen[v] = 1;
                clash_vars.push_back(v);
            }
        }

        for(const auto& v: clash_vars) {
            seen[v] = 0;
        }
    }


    bool rhs = false;
    vector<uint32_t> clash_vars;
    bool detached = false;
    vector<uint32_t> vars;
    #ifdef USE_TBUDDY
    tbdd::xor_constraint* bdd = NULL;
    #endif
};

inline std::ostream& operator<<(std::ostream& os, const Xor& thisXor)
{
    for (uint32_t i = 0; i < thisXor.size(); i++) {
        os << Lit(thisXor[i], false);

        if (i+1 < thisXor.size())
            os << " + ";
    }
    os << " =  " << std::boolalpha << thisXor.rhs << std::noboolalpha;

    os << " -- clash: ";
    for(const auto& c: thisXor.clash_vars) {
        os << c+1 << ", ";
    }

    return os;
}

}

#endif //_XOR_H_
