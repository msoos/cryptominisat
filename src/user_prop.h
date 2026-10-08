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

#include <vector>
#include <cstddef>

#include "solvertypesmini.h"

namespace CMSat {

/**
IPASIR-UP: inspect and steer the CDCL search from outside the solver. See
"Satisfiability Modulo User Propagators", Fazekas, Niemetz, Preiner, Kirchweger,
Szeider & Biere, JAIR 81 (2024) 989-1017.

Literals crossing this interface are numbered as in add_clause() and
get_model(), and must be over variables observed via
SATSolver::add_observed_var().

At most one propagator can be connected. Connecting one restricts the solver to
a single thread and disables Gauss-Jordan elimination and chronological
backtracking.

SATSolver::is_decision() counts any literal assigned above the root with no
reason, including the current assumptions. It is only meaningful during
solve(): afterwards the solver is at level 0 and nothing is a decision.
*/
class ExternalPropagator
{
public:
    /// A lazy propagator only inspects complete assignments: no notify_*
    /// calls, no cb_propagate() or cb_decide(). It still gets
    /// cb_check_found_model(), and cb_has_external_clause() after rejecting a
    /// model -- its only way to add anything. Read on every callback, so do
    /// not change it while connected.
    bool is_lazy = false;

    /// If true, reason clauses from cb_add_reason_clause_lit() may be deleted
    /// during clause database reduction.
    bool are_reasons_forgettable = false;

    /// Not part of IPASIR-UP: if true, every decision the solver's own
    /// heuristic makes on an observed variable is first offered to
    /// cb_decide_polarity(), which may flip its sign. Off by default, so a
    /// propagator written for plain IPASIR-UP is never asked.
    bool advises_polarity = false;

    virtual ~ExternalPropagator() = default;

    //////////////////////////////
    // Inspecting the trail
    //
    // The trail is a stack: notify_assignment() pushes, notify_backtrack()
    // pops. Notification is lazy, but the view is up to date whenever a cb_*
    // function is called.
    //////////////////////////////

    /// Observed literals that just became true, all on the current level.
    virtual void notify_assignment(const std::vector<Lit>& lits) = 0;

    /// A new decision level was opened. Its decision, if over an observed
    /// variable, arrives through notify_assignment().
    virtual void notify_new_decision_level() = 0;

    /// Every assignment above 'new_level' is undone. 'new_level' is always
    /// below the number of decision levels notified so far. Observed variables
    /// cannot be added or removed from inside this callback: the solver counts
    /// on staying at 'new_level'.
    virtual void notify_backtrack(size_t new_level) = 0;

    //////////////////////////////
    // Influencing the search
    //////////////////////////////

    /// A complete assignment: one literal per observed variable, in the order
    /// they were observed. Return false to reject it, then hand over at least
    /// one clause via cb_has_external_clause() or call force_backtrack();
    /// rejecting without doing either counts as acceptance.
    virtual bool cb_check_found_model(const std::vector<Lit>& model) = 0;

    /// The next decision, or lit_Undef to let the solver decide. An assigned
    /// literal is ignored. SATSolver::force_backtrack() may be called from
    /// here: the solver backtracks first, then makes the decision if it is
    /// still unassigned and every assumption is still on the trail; otherwise
    /// the decision is dropped and cb_decide() is asked again.
    virtual Lit cb_decide() { return lit_Undef; }

    /// Not part of IPASIR-UP, and only asked under 'advises_polarity': the
    /// solver's heuristic is about to decide 'lit'. Return 'lit' to keep it or
    /// ~lit to decide the other way; the variable is not the propagator's to
    /// choose. Not asked for assumptions, cb_decide() decisions or a lazy
    /// propagator. The advice overrides a phase set with SATSolver::phase().
    /// The callback must not change the solver: observed variables cannot be
    /// added or removed, and force_backtrack() is ignored.
    virtual Lit cb_decide_polarity(Lit lit) { return lit; }

    /// A literal implied by external knowledge under the current trail, or
    /// lit_Undef if there is nothing to propagate.
    virtual Lit cb_propagate() { return lit_Undef; }

    /// The reason for an earlier cb_propagate() of 'propagated_lit', one
    /// literal per call, closed with lit_Undef. It must contain
    /// 'propagated_lit' and be implied by the propagator's constraints. Every
    /// other literal must have been false *when the propagation was made*. By
    /// default this is asked much later, in conflict analysis (see
    /// SATSolver::set_lazy_external_reasons()), so record the reason when
    /// propagating rather than derive it from the trail on demand. Observed
    /// variables cannot be added or removed from inside this callback.
    virtual Lit cb_add_reason_clause_lit(Lit propagated_lit) {
        (void)propagated_lit;
        return lit_Undef;
    }

    /// Whether there is a clause to hand over. Set 'is_forgettable' to let the
    /// solver delete it during clause database reduction.
    virtual bool cb_has_external_clause(bool& is_forgettable) = 0;

    /// The literals of that clause, one at a time, closed with lit_Undef.
    virtual Lit cb_add_external_clause_lit() = 0;
};

}
