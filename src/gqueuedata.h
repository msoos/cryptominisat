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

#include <cstdint>
#include "propby.h"

namespace CMSat {

struct GaussQData {
    bool do_eliminate; // we do elimination when basic variable is invoked
    uint32_t new_resp_var;                     // do elimination variable
    uint32_t new_resp_row ;         // do elimination row
    PropBy confl;              // returning conflict
    gauss_res ret; //final return value to Searcher
    uint32_t currLevel; //level at which the variable was decided on


    uint32_t num_props = 0;  // total gauss propogation time for DPLL
    uint32_t num_conflicts = 0;   // total gauss conflict    time for DPLL
    uint32_t engaus_disable_checks = 0;
    bool disabled = false;  // Can be disabled. In this case, all xor constraints
                            // are reattached as plain XOR constraints

    void reset()
    {
        do_eliminate = false;
        ret = gauss_res::none;
    }
};

}
