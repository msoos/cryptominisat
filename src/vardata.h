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

#include <limits>
#include "propby.h"
#include "avgcalc.h"

using std::numeric_limits;

namespace CMSat
{

struct VarData
{
    VarData([[maybe_unused]] uint32_t num) {
        is_bva = 0;
        occ_simp_tried = 0;
        elim_cand = 1;
        saved_polarity = false;
        target_polarity = false;
        target_polarity_set = false;
        best_polarity = false;
        best_polarity_set = false;
    }

    ///contains the decision level at which the assignment was made.
    uint32_t level = numeric_limits<uint32_t>::max();
    uint32_t sublevel = numeric_limits<uint32_t>::max();

    //Reason this got propagated. nullptr means decision/toplevel
    PropBy reason = PropBy();

    lbool assumption = l_Undef;

    ///Whether var has been eliminated (var-elim, different component, etc.)
    Removed removed = Removed::none;

    //CaDiCaL's phases: 'saved' is always set, 'target' (largest conflict-free
    //trail since the last rephase) and 'best' (largest ever) may be unset
    uint8_t saved_polarity:1;
    uint8_t target_polarity:1;
    uint8_t target_polarity_set:1;
    uint8_t best_polarity:1;
    uint8_t best_polarity_set:1;
    uint8_t is_bva:1;
    uint8_t occ_simp_tried:1;
    ///CaDiCaL's Flags::elim -- occurred in an irredundant clause that was
    ///removed or shrunk since BVE last tried this variable
    uint8_t elim_cand:1;
    uint8_t propagated:1 = false;

    float weight = 0.5;

    #if defined(STATS_NEEDED)
    uint32_t community_num = numeric_limits<uint32_t>::max();
    #endif

};

}
