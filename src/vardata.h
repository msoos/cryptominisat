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

#ifndef __VARDATA_H__
#define __VARDATA_H__

#include "constants.h"
#include "propby.h"
#include "avgcalc.h"

namespace CMSat
{

struct VarData
{
    VarData()
    {
        is_bva = 0;
        occ_simp_tried = 0;
        saved_polarity = false;
        stable_polarity = false;
        best_polarity = false;
        inv_polarity = false;
    }

    ///contains the decision level at which the assignment was made.
    uint32_t level = numeric_limits<uint32_t>::max();
    uint32_t sublevel = numeric_limits<uint32_t>::max();

    #ifdef WEIGHTED_SAMPLING
    double weight = 0.5;
    #endif

    //Reason this got propagated. NULL means decision/toplevel
    PropBy reason = PropBy();

    lbool assumption = l_Undef;

    ///Whether var has been eliminated (var-elim, different component, etc.)
    Removed removed = Removed::none;

    ///The preferred polarity of each variable.
    uint8_t stable_polarity:1;
    uint8_t saved_polarity:1;
    uint8_t best_polarity:1;
    uint8_t inv_polarity:1;
    bool propagated = false;
    uint8_t is_bva:1;
    uint8_t occ_simp_tried:1;


    #if defined(STATS_NEEDED)
    uint32_t community_num = numeric_limits<uint32_t>::max();
    #endif

    #if defined(STATS_NEEDED_BRANCH) || defined(FINAL_PREDICTOR_BRANCH)
    uint32_t set = 0;
    uint64_t num_propagated = 0;
    uint64_t num_propagated_pos = 0;
    uint64_t num_decided = 0;
    uint64_t num_decided_pos = 0;
    bool     last_time_set_was_dec;
    uint32_t last_seen_in_1uip = 0;
    uint32_t last_decided_on = 0;
    uint32_t last_propagated = 0;
    uint32_t last_canceled = 0;

    //these are per-solver data
    uint64_t sumDecisions_at_picktime = 0;
    uint64_t sumConflicts_at_picktime = 0;
    uint64_t sumPropagations_at_picktime = 0;
    uint64_t sumAntecedents_at_picktime = 0;
    uint64_t sumAntecedentsLits_at_picktime = 0;
    uint64_t sumConflictClauseLits_at_picktime = 0;
    uint64_t sumDecisionBasedCl_at_picktime = 0;
    uint64_t sumClLBD_at_picktime = 0;
    uint64_t sumClSize_at_picktime = 0;

    uint64_t sumDecisions_below_during = 0;
    uint64_t sumConflicts_below_during = 0;
    uint64_t sumPropagations_below_during = 0;
    uint64_t sumAntecedents_below_during = 0;
    uint64_t sumAntecedentsLits_below_during = 0;
    uint64_t sumConflictClauseLits_below_during = 0;
    uint64_t sumDecisionBasedCl_below_during = 0;
    uint64_t sumClLBD_below_during = 0;
    uint64_t sumClSize_below_during = 0;

    //these are per-variable data
    uint64_t inside_conflict_clause = 0;
    uint64_t inside_conflict_clause_glue = 0;
    uint64_t inside_conflict_clause_antecedents = 0;

    uint64_t inside_conflict_clause_at_picktime = 0;
    uint64_t inside_conflict_clause_glue_at_picktime = 0;
    uint64_t inside_conflict_clause_antecedents_at_picktime = 0;

    uint64_t inside_conflict_clause_during = 0;
    uint64_t inside_conflict_clause_glue_during = 0;
    uint64_t inside_conflict_clause_antecedents_during = 0;

    uint64_t last_flipped = 0;
    bool dump;
    #endif
};

}

#endif //__VARDATA_H__
