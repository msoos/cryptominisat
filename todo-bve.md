# BVE: CryptoMiniSat vs CaDiCaL

Read of CaDiCaL 2.1.3 `elim.cpp`/`gates.cpp`/`instantiate.cpp` (from
`build_dbg/_deps/cadical-src`) against CMS `occsimplifier.cpp`.

The skeleton is the same — occurrence lists, score-ordered heap, gate detection
for substitution, `pos+neg+bound` resolvent count limit, backward subsumption of
resolvents, extension stack. The differences are in the details.

Only item 1 has been implemented and benchmarked. Everything else is a source
read, unmeasured.

## 1. On-the-fly self-subsuming resolution — TRIED, DEAD END

`elim.cpp:420-455`. CaDiCaL counts how many literals of each antecedent survived
into the resolvent (`s`, `t`). If `s > size`, the resolvent is `c` minus the
pivot, so it strengthens `c` in place and skips the resolution; if both exceed,
the resolvent subsumes both antecedents. Han & Somenzi SAT'09.

CMS's `resolve_clauses` (`occsimplifier.cpp:4768`) only returns a tautology flag
— it builds `dummy` and never compares sizes back to the antecedents.

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

## 2. The elimination bound is reset and ramped blindly

CaDiCaL's `lim.elimbound` is persistent across the whole solve and only doubles
(0→1→2→4→8→16, `elimboundmax`=16) when a phase ran to completion — schedule
emptied and subsume/block/cover produced no new candidates
(`increase_elimination_bound`, `elim.cpp:944`).

CMS sets `grow = 0` at the top of every `eliminate_vars()`
(`occsimplifier.cpp:1153`) and ramps 0→3→×1.5 up to `min_bva_gain`=16 within the
call regardless of whether the cheap bound was exhausted
(`occsimplifier.cpp:1294-1297`). So CMS pays for expensive high-growth
resolutions before finishing the cheap ones, and throws away progress between
calls. The GlueMiniSat/COMinisatPS/Maple idea CaDiCaL credits.

## 3. No touched-variable scheduling

CaDiCaL schedules only variables whose `elim` flag was set — those occurring in
an irredundant clause removed or shrunk since the last round — and
`eliminating()` skips the whole phase unless new units or new marks appeared.

CMS's `order_vars_for_elim()` (`occsimplifier.cpp:5005`) inserts every eligible
variable each round and relies on time limits to cut off. On a formula where BVE
already ran, CMS re-tries everything.

## 4. Cutoff shapes differ substantially

|                  | CaDiCaL                                | CMS                                                  |
|------------------|----------------------------------------|------------------------------------------------------|
| occurrence limit | `elimocclim` 100, on the larger polarity | `varelim_cutoff_too_many_clauses` 2000, on the product |
| resolvent size   | `elimclslim` 100                       | `velim_resolvent_too_large` 20                       |

CMS refuses 50×50 (CaDiCaL allows) but permits 2×900 (CaDiCaL refuses). And
CMS's resolvent-size cap is 5× tighter. Both are cheap experiments.

## 5. Score function

CaDiCaL: `elimprod*pos*neg + elimsum*(pos+neg)`, and pure literals get a
negative score so they are tried first (`compute_elim_score`, `elim.cpp:21`).
CMS: `pos*neg` alone (`heuristicCalcVarElimScore`, `occsimplifier.cpp:4981`).
The sum term breaks ties — 1×10 and 2×5 have equal product but sums 11 vs 7.

## 6. elim_propagate

On deriving a unit while counting resolvents, CaDiCaL propagates it immediately
inside the elimination structures, marking satisfied clauses garbage and
updating the schedule (`elim.cpp:142`). Note the comment at `elim.cpp:250` —
they deliberately do *not* do this while adding resolvents, after a rare
model-reconstruction bug. CMS's `clear_vars_from_cls_that_have_been_set()` after
each `maybe_eliminate` is coarser.

## 7. Minor

In-round garbage collection at `2*irrlits/3 + (1<<20)`; automatic
`mark_redundant_clauses_with_eliminated_variables_as_garbage()` (CMS has this
only as the `occ-del-elimed` schedule token).

## Where CMS is ahead

`find_irreg_gate` (picosat definability check — CaDiCaL only has syntactic
AND/ITE/XOR/equivalence); `weaken()`/`check_taut_weaken_dummy`, covered-clause
style resolvent weakening that CaDiCaL only has as `cover()`, default off;
`varelim_check_resolvent_subs` forward-subsumption simulation; the
clause-growth-vs-variable-reduction stopping rule (`cl_inc_rate > var_dec_rate`,
`occsimplifier.cpp:1263`), which CaDiCaL has no analogue for; XOR-awareness.

## Look like gaps but are not

`block` (BCE), `cover` (CCE) and `instantiate` are all default 0 in CaDiCaL
2.1.3 — the interleaving in `elim()` is dead code by default. And CaDiCaL's
`instantiate.cpp` is a different technique from `--distillinst`, which is
CaDiCaL's vivify instantiation.

## Ranking

Revised after item 1 measured as a wash. Prefer changes that alter *which*
variables get tried and how often BVE runs, over ones that duplicate work CMS
already does elsewhere:

1. completion-gated persistent elimination bound (item 2)
2. touched-variable scheduling (item 3)
3. the two cutoffs (item 4)
4. score sum term (item 5)
5. ~~OTF self-subsumption (item 1)~~ — measured, no gain

## Measuring

A/B harness used for item 1, ~10 min for a run: loop the 29
`../approxmc/build/*.no_w.cnf.gz` instances under
`cryptominisat5 --verb 2 --maxconfl 1 --presimp 1`, compare `irred_long_lits`
and `free_vars` from the last `[simp-stats] AFTER` line. Same harness answers
items 2-5.
