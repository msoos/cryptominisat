# Oracle Solver: Watch-Free Persistent Assumptions

## Background

Standard CDCL solvers handle assumptions by backtracking to level 0 and then
replaying each assumption as a separate decision at its own level, followed by
unit propagation. For N assumptions this means N sequential propagation waves.
When N is large (e.g. 1000 indicator literals), this overhead dominates the
solving cost.

The Oracle solver (derived from SharpSAT-TD, Tuukka Korhonen 2021) avoids
this entirely through a combination of a three-tier level scheme and a function
called `SetAssumpLit` that surgically injects assignments without entering the
normal watch-list or undo-trail machinery.

---

## Three-Tier Level Scheme

The Oracle partitions assignments into three tiers by decision level:

| Level | Meaning | Undone by `UnDecide`? |
|-------|---------|----------------------|
| 1 | Permanently frozen units | Never |
| 2 | Working assumption shell (pre-set + propagated) | Only entries that are in `decided[]` |
| 3+ | Regular CDCL search decisions during `HardSolve` | Yes |

`UnDecide(k)` walks the `decided[]` trail from the back and unassigns
everything at level ≥ k. The key exploit is that an assignment can be recorded
in `lit_val[]` and `vs[v].level` **without** being pushed onto `decided[]`,
making it invisible to `UnDecide`.

---

## `SetAssumpLit` — the Core Mechanism

```cpp
void Oracle::SetAssumpLit(Lit lit, bool freeze);
```

The function does three things that together make an assumption "free":

### 1. Pre-move all watches away from the variable

```cpp
for (Lit tl : {PosLit(v), NegLit(v)}) {
    for (const Watch w : watches[tl]) {
        // Find any other unassigned literal f in the clause
        for (size_t i = w.cls+2; clauses[i]; i++) {
            if (LitVal(clauses[i]) == 0) { f = i; }
        }
        // Physically swap f into the watch position,
        // then attach the watch to f instead of tl.
        swap(clauses[f], clauses[pos]);
        watches[clauses[pos]].push_back({w.cls, clauses[opos], w.size});
    }
    watches[tl].clear();   // tl now has zero watches
}
```

After this loop both `watches[lit]` and `watches[Neg(lit)]` are empty. Every
clause that previously watched this variable now watches some other unassigned
literal. This is exactly the work that the propagator would have done when
forced to move the watch because the literal became false — it is done here,
eagerly, once, at assumption-setup time.

### 2. Assign the literal at the correct level

```cpp
if (freeze) Assign(lit, 0, 1);   // permanent unit at level 1
else        Assign(lit, 0, 2);   // soft assumption at level 2
```

`Assign` sets `lit_val`, `vs[v].level`, pushes `v` onto `decided[]`, and
pushes `Neg(lit)` onto `prop_q[]`.

### 3. Surgically undo the trail side-effects

```cpp
decided.pop_back();   // remove from undo trail
prop_q.pop_back();    // remove from propagation queue
```

The result: the literal is fully assigned in the value/level arrays, but it
has **no watches** and is **not on any trail or queue**. It cannot be undone by
`UnDecide` and generates zero propagation work.

---

## How `Solve()` Benefits

```cpp
TriState Oracle::Solve(const vector<Lit>& assumps, ...) {
    for (const auto& lit : assumps) {
        if (LitVal(lit) == -1) { UnDecide(2); return false; }
        else if (LitVal(lit) == 0) { Decide(lit, 2); }
        // LitVal(lit) == 1: pre-set by SetAssumpLit, skip entirely
    }
    Propagate(2);        // one single propagation wave for all assumptions
    HardSolve(...);
    UnDecide(2);         // rolls back CDCL search, not the pre-set assumptions
}
```

Assumptions pre-set via `SetAssumpLit` are already assigned (`LitVal == 1`),
so the loop skips them at zero cost. Any remaining (dynamic) assumptions are
queued at level 2, and then a **single `Propagate(2)` call** handles all their
consequences simultaneously — not N sequential waves.

`UnDecide(2)` at the end rolls back the CDCL search decisions (levels ≥ 2 in
`decided[]`) but does not touch the watch-free, trail-free pre-set assumptions,
so their state persists into the next `Solve()` call.

---

## Use in Practice: Indicator Literals (`oracle_use.cpp`)

The immediate use case is clause-subset enumeration. Each original clause `C`
is extended with an indicator literal `ind_i`:

```
(ind_i ∨ c1 ∨ c2 ∨ ...)
```

- `ind_i = true` → clause is satisfied (disabled)
- `ind_i = false` → clause is a real constraint (active)

To test whether clause `i` is redundant:

```
SetAssumpLit(ind_i = true,  freeze=false)  // tentatively disable clause i
Solve(assumps)
if SAT:  SetAssumpLit(ind_i = true,  freeze=true)  // confirmed redundant
if UNSAT: SetAssumpLit(ind_i = false, freeze=true)  // clause is needed
```

Each `SetAssumpLit` call touches only the watches of **one** indicator variable
— O(clauses containing that variable). The solver never backtracks to level 0.
All N indicator states accumulate as a persistent, watch-free shadow that
survives every `UnDecide(2)` between `Solve()` calls.

---

## Summary of Costs vs. Standard Approach

| Operation | Standard CDCL | Oracle |
|-----------|--------------|--------|
| Apply N assumptions | N backtrack+propagate cycles | 1 propagation wave |
| Change assumption k | Full backtrack to level 0, re-apply all N | `SetAssumpLit` on variable k only |
| Freeze an assumption permanently | Add unit clause, restart | `SetAssumpLit(..., freeze=true)` at level 1 |
| Watch maintenance | Done lazily during propagation | Done eagerly, once, in `SetAssumpLit` |

The scheme is particularly effective when the number of assumptions is large
and only a small fraction changes between consecutive `Solve()` calls — exactly
the pattern that arises in clause-subset minimization and similar
enumeration tasks.
