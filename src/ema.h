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

#include <cassert>
#include <cstdint>

namespace CMSat {

//Exponential moving average with ADAM-style bias-corrected
//initialization, as in CaDiCaL [KingmaBa-ICLR'15]
struct EMA {
    double value = 0.0;  ///<unbiased (corrected) moving average
    double biased = 0.0;
    double alpha = 0.0;
    double beta = 0.0;
    double exp = 0.0;    ///<pow(beta, num updates)

    EMA() = default;
    explicit EMA(const double window) :
        alpha(1.0 / window),
        beta(1.0 - alpha),
        exp(beta > 0 ? 1.0 : 0.0)
    {
        assert(window >= 1);
    }

    operator double() const { return value; }

    void update(const double y) {
        biased += alpha * (y - biased);
        if (exp) {
            exp *= beta;
            assert(exp < 1);
            value = biased / (1.0 - exp);
        } else {
            value = biased;
        }
    }
};

//Donald Knuth's 'reluctant doubling' formulation of the Luby sequence,
//as in CaDiCaL. tick() is called once per conflict; once countdown hits
//zero the trigger is set and consumed via operator bool().
class Reluctant {
    uint64_t u = 1;
    uint64_t v = 1;
    uint64_t limit = 0;
    uint64_t period = 0;
    uint64_t countdown = 0;
    bool trigger = false;

public:
    void enable(const uint64_t p, const uint64_t l) {
        assert(p > 0);
        u = v = 1;
        period = countdown = p;
        trigger = false;
        limit = l;
    }

    void disable() { period = 0; trigger = false; }

    void tick() {
        if (!period) return;
        if (trigger) return;
        if (--countdown) return;

        if ((u & -u) == v) { u++; v = 1; }
        else v *= 2;

        if (limit && v >= limit) u = v = 1;
        countdown = v * period;
        trigger = true;
    }

    operator bool() {
        if (!trigger) return false;
        trigger = false;
        return true;
    }
};

} //namespace CMSat
