/******************************************
Copyright (C) 2022 Authors of CryptoMiniSat, see AUTHORS file

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

#ifndef _BNN_H_
#define _BNN_H_

#include "solvertypes.h"

#include <vector>
#include <iostream>
#include <algorithm>

using std::vector;
using std::make_pair;

namespace CMSat {

struct LitW {
    LitW() {}

    explicit LitW(Lit _lit, int32_t _w):
        lit(_lit),
        w(_w)
    {}

    Lit lit;
    int32_t w;
};

struct BNN_Lit_Sorter {
    bool operator()(const LitW& a, const LitW& b) {
        return a.lit < b.lit;
    }
};

struct BNN_Weight_Sorter {
    bool operator()(const LitW& a, const LitW& b) {
        return a.w > b.w;
    }
};

class BNN
{
public:
    BNN()
    {}

    explicit BNN(
        const vector<Lit>& _in,
        const vector<uint32_t>& _ws,
        const uint32_t _cutoff,
        const Lit _out):
        out (_out)
    {
        assert(_in.size() > 0);

        cutoff = (int32_t)_cutoff;
        assert(_ws.size() == _in.size());
        for(uint32_t i = 0; i < _ws.size(); i ++) {
            assert(_ws[i] > 0);
            LitW lw(_in[i], (int32_t)_ws[i]);
            in.push_back(lw);
        }

//         std::sort(in.begin(), in.end(), BNN_Weight_Sorter);
//         uint32_t total_weight = 0;
//         for(const auto& x: in)
//             total_weight+=x.w;
//         for(int32_t i = in.size()-1; i >= 0; i--) {
//             in[i].max_after = total_weight - in[i].w;
//             total_weight -= in[i].w;
//         }
//         assert(total_weight == 0);
    }

    const LitW& operator[](const uint32_t at) const
    {
        return in[at];
    }

    LitW& operator[](const uint32_t at)
    {
        return in[at];
    }

    auto& get_in()
    {
        return in;
    }

    const Lit& get_out() const
    {
        return out;
    }

    size_t size() const
    {
        return in.size();
    }

    bool empty() const
    {
        return in.empty();
    }

    vector<Lit> reason;
    vector<LitW> in;
    int32_t cutoff;
    Lit out;
};

inline std::ostream& operator<<(std::ostream& os, const BNN& bnn)
{
    for (uint32_t i = 0; i < bnn.size(); i++) {
        os << bnn[i].lit << " * " << bnn[i].w;

        if (i+1 < bnn.size())
            os << " + ";
    }
    os << " >  " << bnn.cutoff
    << " -- out: " << bnn.out;

    return os;
}

}

#endif //_BNN_H_
