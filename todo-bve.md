# BVE: CryptoMiniSat vs CaDiCaL

Originally a read of CaDiCaL 2.1.3. Re-checked against **CaDiCaL 3.0.1**
(`../cadical`, `elim.cpp`/`elimfast.cpp`/`gates.cpp`/`definition.cpp`/
`instantiate.cpp`) against CMS `occsimplifier.cpp`.

The skeleton is the same — occurrence lists, score-ordered heap, gate detection
for substitution, `pos+neg+bound` resolvent count limit, backward subsumption of
resolvents, extension stack. The differences are in the details.

**Items 2-6 are now implemented in CMS** (branch `develop-more-cadical`).
Item 1 was tried and discarded. `definition_unit` (under "New in CaDiCaL 3.0.1")
is implemented and proof-checked but measures flat. See "Status" under each.

## 1. On-the-fly self-subsuming resolution — TRIED, DEAD END

`elim.cpp:208-455` in 3.0.1 (unchanged from 2.1.3). CaDiCaL counts how many
literals of each antecedent survived into the resolvent (`s`, `t`). If
`s > size`, the resolvent is `c` minus the pivot, so it strengthens `c` in place
and skips the resolution; if both exceed, the resolvent subsumes both
antecedents. Han & Somenzi SAT'09.

CMS's `resolve_clauses` only returns a tautology flag — it builds `dummy` and
never compares sizes back to the antecedents.

**Implemented, measured, discarded.** Detection during resolvent generation,
applied on the path where `maybe_eliminate` gives up (the only place CMS
currently discards the work). Correct — 640 fuzz rounds, 109 with `cake_xlrup`
proof checking, zero failures. It fires: 14620 literals recovered across 29
instances.

But it does not help. `irred_long_lits` after simplification, on/off:

- 25 of 29 instances byte-identical
- smaller: `blasted_squaring42` -1978, `isolateRightmost` -62
- larger: `hash-6` +4813, `hash-10-7` +1301
- net +4074 (0.08% worse); vars eliminated 51547 vs 51582

Not a budget problem — all three differing instances finish `T-out: N` with ~98%
of the limit unused. It is heuristic perturbation: `remove_literal` updates
`n_occurs`, which reshuffles the elimination heap, so a different set of
variables gets tried.

Two reasons it does not transfer:

- **No bound advantage, contrary to first reading.** CaDiCaL strengthens `c`
  eagerly, so `pos` drops by one *and* that resolvent goes uncounted:
  `#others <= (pos-1)+neg+bound`. CMS counts the resolvent against
  `pos+neg+grow`. Same inequality.
- CaDiCaL's only real gain is that strengthenings survive a failed elimination,
  and CMS already recovers those literals downstream via `occ-backw-sub-str` and
  distillation. CaDiCaL leans on BVE for them. Hence 25/29 identical.

If revisited, keep three restrictions: long clauses only (binaries give units,
needing enqueue+propagate on the abort path); do **not** hook
`generate_resolvents_weakened` (weakened resolvents can be shorter than the true
one, so the size test false-positives and strengthens unsoundly); guard stale
clause IDs, since an earlier strengthening in the same batch changes a clause's
ID and invalidates the FRAT chain.

## 2. The elimination bound is reset and ramped blindly — DONE

CaDiCaL's `lim.elimbound` is persistent across the whole solve and only doubles
(0→1→2→4→8→16, `elimboundmax`=16) when a phase ran to completion — schedule
emptied and subsume/block/cover produced no new candidates
(`increase_elimination_bound`, `elim.cpp:965`).

CMS used to set `grow = 0` at the top of every `eliminate_vars()` and ramp
0→3→×1.5 up to `min_bva_gain`=16 within the call regardless of whether the cheap
bound was exhausted. So CMS paid for expensive high-growth resolutions before
finishing the cheap ones, and threw away progress between calls. The
GlueMiniSat/COMinisatPS/Maple idea CaDiCaL credits.

**Status: implemented.** `OccSimplifier::grow` is no longer reset per call;
`increase_elim_bound()` doubles 0→1→2→4→8→16 and re-marks every variable. The
gate (`round_complete`) requires the schedule to be empty, the touched list
empty, and none of the time/count/memory budgets exhausted. Unlike CaDiCaL the
ramp can still happen several times inside one `eliminate_vars()` call — one
bump per completed round rather than one per phase — so a single `--preproc`
pass still reaches bound 16, but it never bumps on an incomplete round and the
bound carries over to the next call.

CMS's `cl_inc_rate > var_dec_rate` stopping rule (which CaDiCaL has no analogue
for) is kept and now also blocks the bound increase.

## 3. No touched-variable scheduling — DONE

CaDiCaL schedules only variables whose `elim` flag was set — those occurring in
an irredundant clause removed or shrunk since the last round — and
`eliminating()` skips the whole phase unless new units or new marks appeared.

CMS's `order_vars_for_elim()` inserted every eligible variable each round and
relied on time limits to cut off. On a formula where BVE already ran, CMS
re-tried everything.

**Status: implemented.** `VarData::elim_cand` (CaDiCaL's `Flags::elim`),
initialised to 1, cleared when the variable is popped off the elimination heap,
set again by `CNF::mark_elim_cand()` from every place an irredundant clause is
removed or shrunk:

- `OccSimplifier`: `unlink_clause`, `clean_clause`, `remove_literal`,
  `rem_cls_from_watch_due_to_varelim`, `add_clause_to_blck`,
  `lit_rem_with_or_gates`, `clear_vars_from_cls_that_have_been_set`
- `SubsumeStrengthen`: binary removal paths
- `Solver::detach_modified_clause` / `Solver::detach_bin_clause` (covers all
  three distillers and `detach_clauses_in_xors`)
- `ClauseCleaner` (long + implicit), `VarReplacer` (long + `updateBin`),
  `SubsumeImplicit`, `Searcher::remove_useless_bins`

Every bound increase re-marks all variables, matching CaDiCaL. A missed mark
costs eliminations but is never unsound.

CMS does not have CaDiCaL's `ineliminating()` phase-level guard — the `occ-bve`
schedule token still runs, it just finds an empty schedule quickly.

## 4. Cutoff shapes differ substantially — DONE

|                  | CaDiCaL                                  | CMS (was)                                              |
|------------------|------------------------------------------|--------------------------------------------------------|
| occurrence limit | `elimocclim` 100, on the larger polarity | `varelim_cutoff_too_many_clauses` 2000, on the product |
| resolvent size   | `elimclslim` 100                         | `velim_resolvent_too_large` 20                         |

CMS refused 50×50 (CaDiCaL allows) but permitted 2×900 (CaDiCaL refuses). And
CMS's resolvent-size cap was 5× tighter.

**Status: implemented.** `varelim_cutoff_too_many_clauses` replaced by
`varelim_occ_cutoff` = 100 applied to `max(pos, neg)` (`--varelimocclim`);
`velim_resolvent_too_large` raised 20 → 100 (`--varelimclslim`).

## 5. Score function — DONE

CaDiCaL: `elimprod*pos*neg + elimsum*(pos+neg)`, and pure literals get a
negative score so they are tried first (`compute_elim_score`, `elim.cpp:21`).
CMS used `pos*neg` alone. The sum term breaks ties — 1×10 and 2×5 have equal
product but sums 11 vs 7.

**Status: implemented.** `heuristicCalcVarElimScore` returns `int64_t`,
`varElimComplexity` is now signed, weights are `--varelimprod` /
`--varelimsum` (both 1).

## 6. elim_propagate — DONE (partially)

On deriving a unit while counting resolvents, CaDiCaL propagates it immediately
inside the elimination structures, marking satisfied clauses garbage and
updating the schedule (`elim.cpp:142`). Note the comment at `elim.cpp:250` —
they deliberately do *not* do this while adding resolvents, after a rare
model-reconstruction bug. Crucially, a unit resolvent does **not** count against
`pos+neg+bound`, and it survives a failed elimination.

CMS used to treat a unit resolvent like any other: it counted against the limit,
and if the elimination was then rejected the unit was thrown away.

**Status: implemented, with a safer shape than CaDiCaL's.** Unit resolvents are
collected in `elim_unit_resolvents` during `generate_resolvents{,_weakened}`,
are not counted against the bound, and are added (with their two-parent FRAT
hints) once generation finishes — whether or not the elimination succeeded.
Because adding them invalidates the occurrence lists the counting loop just
walked, `test_elim_and_fill_resolvents` then returns false and the variable is
retried in a later round rather than eliminating mid-flight. That avoids
CaDiCaL's reentrancy hazard entirely.

Not done: eager garbage-marking of clauses satisfied by the new unit *during*
counting. CMS's `clear_vars_from_cls_that_have_been_set()` after each
`maybe_eliminate` does it a bit later.

## 7. Minor — not done

In-round garbage collection at `2*irrlits/3 + (1<<20)`; automatic
`mark_redundant_clauses_with_eliminated_variables_as_garbage()` (CMS has this
only as the `occ-del-elimed` schedule token).

## New in CaDiCaL 3.0.1

### `definition.cpp` + `kitten.c` — CaDiCaL caught up with `find_irreg_gate`

The 2.1.3 note that "CMS is ahead because CaDiCaL only has syntactic AND/ITE/XOR
gates" is **no longer true**. `find_definition()` exports both occurrence lists
of the pivot to kitten (an embedded sub-solver, `kitten.c`, ported from kissat),
with the pivot literal itself excluded from each clause. UNSAT means a
definition exists, and the clausal core is the gate. This is exactly CMS's
`find_irreg_gate()`, which does the same with picosat and `picosat_coreclause`.

Differences worth stealing:

- **One-sided core ⇒ failed literal — DONE.** CaDiCaL tracks which side the core
  clauses came from (`definition_unit` bitmask). If the core only uses
  `occs(lit)`, then `{C \ {lit} : lit ∈ C}` is UNSAT, so `lit` is implied and
  becomes a unit.

  **Status: implemented**, proof and all (`--varelimirregunit`, default on).
  `find_irreg_gate` already knows which side each core clause came from — it
  fills `out_a`/`out_b` separately — so the detection is one `empty()` test.

  The unit is not RUP, so the blocker was the proof. Solved by replaying
  picosat's own resolution trace: `picosat_write_extended_trace_data()` (an
  mpicosat addition, previously unused) hands back the clausal core in extended
  tracecheck form, `idx lits… 0 antecedents… 0`, originals first then learned in
  derivation order. Every core clause is `C \ {lit}` of a CMS clause, so adding
  `lit` back to each learned clause turns the trace into a CMS derivation whose
  final step — picosat's empty clause — is the unit itself. Original clauses map
  straight to their CMS IDs via the `a_map`/`b_map` picosat-index → `Watched`
  tables; each learned one is emitted as a FRAT lemma with a fresh ID and
  deleted again once the unit is in.

  Two things had to be got right:

  - **Hint order.** `add_zhain` sorts a chain's antecedents by clause index, so
    picosat's order is *not* a propagation order and strict LRAT checking
    rejects it. `order_pico_chain()` re-derives one: assume the negation of the
    lemma, propagate its antecedents, record the order they fire in, drop the
    ones that come out satisfied. The chain is therefore verified by
    construction — if it does not conflict we emit nothing and fall back to
    using the core as a gate.
  - **The trace literals must equal what we added.** They do: `trivial_clause`
    only drops duplicates and tautologies, `simplify` collects satisfied clauses
    but never shrinks one, and with `TRACE` on `collect_clause` never frees an
    original.

  When FRAT is off the trace is skipped entirely and the unit is enqueued
  directly.

  **Measured, and it is a wash.** 30-instance harness below,
  `--varelimirregunit` 1 vs 0, one instance times out in both arms:

  | instance | lits | free | units | elim | cls |
  |----------------|-----:|----:|----:|----:|----:|
  | 03B-1          |   +0 |  +0 |  +1 |  -1 |  +0 |
  | 03B-4          |   +0 |  +0 |  +4 |  -4 |  +0 |
  | 13A-3          |   +0 |  +0 |  +6 |  -6 |  +0 |
  | 90-34-2-q      |   +0 |  +0 |  +9 |  -9 |  +0 |
  | blasted_case12 |   +0 |  +0 |  +8 |  -8 |  +0 |
  | herman21       |  -70 |  -1 | +32 | -31 | -14 |
  | s38417_7_4     |  -53 |  +1 |  +2 |  -3 |  +3 |
  | tire-2         |   +0 |  +0 |  +2 |  -2 |  +0 |
  | **total**      | **-123** | **0** | **+64** | **-64** | **-11** |

  `units +64` against `elimed_vars -64`, `free_vars` dead level: every unit this
  derives is a variable BVE was going to eliminate anyway. Six of the eight
  differing instances are otherwise byte-identical — same clause count, same
  literals, same bins. Only herman21 and s38417 move any literals, 123 out of
  4.65M. 21 of 29 instances do not differ at all.

  Two reasons the win does not materialise:

  - The unit substitutes for an elimination rather than adding to it. A unit is
    the better form in principle — it propagates, costs one clause, and needs no
    extension-stack entry — but the resolvents it displaces were cheap.
  - On deriving the unit we return `false` and do not eliminate the variable
    that round, so the `|core| × |negs|` resolvents "saved" would only ever have
    been *added* if the elimination had passed the bound test, which mostly it
    would not have.

  Correctness is not in doubt: `no-chain` was zero on every run — 9 of the 30
  instances plus 400 generated UNSAT instances carrying planted implied literals
  (569 units), all `cake_xlrup`-verified, over 720 fuzz rounds.

  Same shape as item 1: correct, fires, verifiable, does not help. Kept behind
  `--varelimirregunit` so it costs nothing when off. The reusable part is the
  trace replay itself — `picosat_write_extended_trace_data` +
  `order_pico_chain` turn any picosat refutation into an FRAT chain, which is
  what any future "the sub-solver proved it, now prove it to the checker" needs.

  Note what CMS used to do instead was *sound*, just wasteful: with `out_b`
  empty the gate resolution generates `|core| × |negs|` resolvents which
  together imply `D \ {~lit}` for every `D ∈ negs` — i.e. exactly the result of
  substituting `lit = true` — and the skipped `antec_poss × negs` resolvents are
  subsumed by them. The unit gets the same effect for one clause.
- **Core shrinking — not done.** `elimdefcores` is 1 in CaDiCaL 3.0.1, i.e. one
  core, no re-solve. Dead code by default there too.
- **Sub-solver reuse — not done, and not portable.** kitten has `kitten_clear`;
  picosat has no equivalent, `picosat_reset` is a destructor. The only reuse
  mechanism is `picosat_push`/`picosat_pop`, which prefixes every clause added
  inside a context with a context literal (`simplify_and_add_original_clause`) —
  that literal would show up in the traced clause literals the unit derivation
  above now depends on. Not worth it: the sub-problem is capped at
  `--varelimirregocclim` = 100 clauses, so `picosat_init` is small next to the
  solve.
- **Budgets.** CMS budgets by literals added (`picosat_gate_limitK`) plus a
  per-query conflict cap, now `--varelimirregconfl` (was a hardcoded 300 that
  ignored the `picosat_confl_limit` its two sibling call sites use).

Note `elimdef` defaults to **0** in CaDiCaL — this is off by default there,
while CMS's `find_irreg_gate` is on. kitten's other user, `sweep.cpp` (SAT
sweeping, default on), is a separate feature CMS has no equivalent of; CMS's
closest thing is the Korhonen–Järvisalo oracle.

### `elimfast.cpp` — cheap preprocessing-only BVE

New file. `elimfast()` runs from `Internal::preprocess()` only, with no gates,
no substitution, no weakening: a fixed bound `fastelimbound` = 8 (capped at
`pos+neg`), `fastelimocclim` = 100, `fastelimclslim` = 100, up to
`fastelimrounds` = 4 rounds, and it skips the resolvent count entirely when
`pos*neg <= bound`. At the end it marks *all* active variables as elim
candidates so the first real `elim()` phase starts from a full schedule.

Not ported: CMS's BVE budget is already tick-based, and CMS has no separate
preprocessing-only phase structure to hang this off.

### Other new files, not BVE

`congruence.cpp` (congruence closure over AND/XOR/ITE gates, default on),
`sweep.cpp` (kitten SAT sweeping, default on), `factor.cpp` (BVA, default off),
`tier.cpp`, `warmup.cpp`, `walk_full_occs.cpp`.

## Where CMS is still ahead

`weaken()`/`check_taut_weaken_dummy`, covered-clause style resolvent weakening
that CaDiCaL only has as `cover()`, default off; `varelim_check_resolvent_subs`
forward-subsumption simulation; the clause-growth-vs-variable-reduction stopping
rule (`cl_inc_rate > var_dec_rate`), which CaDiCaL has no analogue for;
XOR-awareness (`eliminate_xor_vars`, `xorclauses_vars`).

## Look like gaps but are not

`block` (BCE), `cover` (CCE), `condition` (globally blocked) and `instantiate`
are all default 0 in CaDiCaL 3.0.1 — the interleaving in `elim()` is dead code
by default. And CaDiCaL's `instantiate.cpp` is a different technique from
`--distillinst`, which is CaDiCaL's vivify instantiation.

## Measuring

A/B harness, ~10 min for a run: loop the 30
`../approxmc/build/*.no_w.cnf.gz` instances under
`cryptominisat5 --verb 2 --maxconfl 1 --presimp 1`, compare `irred_long_lits`
and `free_vars` from the last `[simp-stats] AFTER` line.

Take `units` and `elimed_vars` off that line too. `definition_unit` looked like
a small win on `irred_long_lits` alone; the two of them moving +64/-64 against a
flat `free_vars` is what showed it was pure relabelling. A change that only
moves a variable between the two categories has done nothing.
