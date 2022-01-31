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

class BNN
{
public:
    BNN()
    {}

    explicit BNN(
        const vector<Lit>& _in,
        const int32_t _cutoff,
        const Lit _out):
        in(_in),
        cutoff(_cutoff),
        out (_out)
    {
        assert(_in.size() > 0);
    }

    const Lit& operator[](const uint32_t at) const
    {
        return in[at];
    }

    Lit& operator[](const uint32_t at)
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

    vector<Lit> in;
    int32_t cutoff;
    Lit out;
    bool isRemoved = false;
};

inline std::ostream& operator<<(std::ostream& os, const BNN& bnn)
{
    for (uint32_t i = 0; i < bnn.size(); i++) {
        os << "lit[" << bnn[i] << "]";

        if (i+1 < bnn.size())
            os << " + ";
    }
    os << " >=  " << bnn.cutoff
    << " -- outlit: " << bnn.out;

    return os;
}

}

#endif //_BNN_H_
