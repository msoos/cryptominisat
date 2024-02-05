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

#pragma once

#include "solvertypes.h"

#include <cstdint>
#include <limits>
#include <vector>
#include <set>
#include <iostream>
#include <algorithm>

using std::vector;
using std::set;

namespace CMSat {

class Xor {
public:
    Xor() = default;
    explicit Xor(const vector<uint32_t>& cl, const bool _rhs): rhs(_rhs) {
        for (const auto& l: cl) vars.push_back(l);
    }
    explicit Xor(const vector<Lit>& cl, const bool _rhs): rhs(_rhs) {
        for (const auto& l: cl) {
            assert(l.sign() == false);
            vars.push_back(l.var());
        };
    }
    bool trivial() const { return size() == 0 && rhs == false; }
    ~Xor() = default;

    vector<uint32_t>::const_iterator begin() const { return vars.begin(); }
    vector<uint32_t>::const_iterator end() const { return vars.end(); }
    vector<uint32_t>::iterator begin() { return vars.begin(); }
    vector<uint32_t>::iterator end() { return vars.end(); }

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

    const uint32_t& operator[](const uint32_t at) const { return vars[at]; }
    uint32_t& operator[](const uint32_t at) { return vars[at]; }
    void resize(const uint32_t newsize) { vars.resize(newsize); }
    const vector<uint32_t>& get_vars() const { return vars; }
    size_t size() const { return vars.size(); }

    bool rhs = false;
    uint8_t prop_confl_watch = 0; // which watch is propagating?
                                  // if it's CONFL, then it's 2 + (0/1)
    vector<uint32_t> vars;
    vector<Lit> reason_cl;
    int32_t reason_cl_ID = 0;
    uint32_t watched[2] = {0,0};
    uint32_t in_matrix = 1000;
    int32_t XID = 0;
};

inline std::ostream& operator<<(std::ostream& os, const Xor& x)
{
    for (uint32_t i = 0; i < x.size(); i++) {
        os << Lit(x[i], false);
        if (i+1 < x.size()) os << " + ";
    }
    os << " =  " << std::boolalpha << x.rhs << std::noboolalpha;

    if (x.watched[0] < x.size() && x.watched[1] < x.size()) {
        os << " -- watch vars: "; for(const auto& at: {0, 1}) os << x[x.watched[at]]+1 << ", ";
    }

    return os;
}

}
