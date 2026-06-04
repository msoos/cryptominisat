---
name: cryptominisat
description: >-
  Solve Boolean satisfiability (SAT) problems with the pycryptosat Python
  library. Use when a problem can be expressed as Boolean variables and logical
  constraints — e.g. "is this set of rules consistent?", "find an assignment
  satisfying these conditions", "are these requirements contradictory?". CMS is
  a pure SAT solver: it answers SAT/UNSAT over CNF (AND-of-ORs) clauses, with
  native support for XOR (parity) constraints. Do NOT use it directly for
  integer/real arithmetic or optimization — see "When NOT to use this" below.
---

# Solving SAT problems with pycryptosat

CryptoMiniSat is a Boolean SAT solver. Your job as the agent is to:

1. **Model** the problem as Boolean variables and clauses (CNF).
2. **Solve** it with `pycryptosat.Solver`.
3. **Decode** the solver's answer back into the original problem's terms.

The solver is the source of truth: any model it returns is provably correct, and
an UNSAT answer is a proof that no solution exists. Your only responsibility is a
faithful translation in step 1 and step 3.

## Setup

```sh
pip install pycryptosat
```

```python
from pycryptosat import Solver
```

## The model: variables and clauses

- A **variable** is a positive integer starting at `1`. You assign meaning to
  each one. **Always keep a Python dict mapping your concepts to variable
  numbers** so you can decode the answer later. Do not hand-pick numbers; use a
  counter.
- A **literal** is a variable (`True`) or its negation (`-var`, `False`).
- A **clause** is a list of literals meaning "at least one of these is true"
  (a logical OR). The whole problem is the AND of all clauses.

Helper to allocate variables by name:

```python
class VarPool:
    def __init__(self):
        self._m = {}
    def v(self, name):
        if name not in self._m:
            self._m[name] = len(self._m) + 1   # 1-based
        return self._m[name]
    def name_of(self, var):
        return {i: n for n, i in self._m.items()}[var]
```

### Translating common logic into clauses

| You want to say | Clause(s) to add |
|---|---|
| `A` is true | `[a]` |
| `A` is false | `[-a]` |
| `A OR B` | `[a, b]` |
| `A implies B` (`A → B`) | `[-a, b]` |
| `A iff B` (`A ↔ B`) | `[-a, b]`, `[a, -b]` |
| at least one of `A,B,C` | `[a, b, c]` |
| at most one of `A,B,C` | `[-a,-b]`, `[-a,-c]`, `[-b,-c]` (pairwise) |
| exactly one of `A,B,C` | "at least one" + "at most one" |
| `A XOR B XOR ...` (odd # true) | use `add_xor_clause` (see below) |

For "at most k" / large cardinality constraints, the pairwise encoding above
blows up; consider a dedicated encoder (e.g. the `pysat` package's
`pysat.card`) and feed the resulting clauses into CMS.

## The core API

```python
s = Solver()
s.add_clause([1, 2])          # (x1 OR x2)
s.add_clause([-2])            # x2 is False
s.add_clauses([[1,3],[-3]])   # add several at once
sat, model = s.solve()
```

- `solve()` returns `(sat, model)`:
  - `sat == True`  → `model` is a tuple indexed by variable: `model[v]` is the
    `True`/`False` value of variable `v`. **`model[0]` is always `None`** (index
    0 is unused), so index with your real variable numbers directly.
  - `sat == False` → no solution exists (UNSAT); `model` is `None`.
  - `sat == None`  → a resource limit was hit before deciding; `model` is `None`.
- `s.nb_vars()` — highest variable number seen so far.
- `s.is_satisfiable()` — convenience boolean.

### Decoding the answer

```python
sat, model = s.solve()
if sat:
    for name, var in pool._m.items():
        print(name, "=", model[var])
```

### XOR clauses (a CryptoMiniSat strength)

`add_xor_clause(lits, rhs)` asserts that the XOR of the variables equals `rhs`
(a bool). This is native and compact — far better than encoding parity in plain
CNF, and a reason to pick CMS over CNF-only or SMT tools for parity-heavy
problems. Literals must be over plain variables (negative literals are rejected;
fold the sign into `rhs` instead).

```python
s.add_xor_clause([1, 2, 3], True)   # x1 XOR x2 XOR x3 == True (odd # true)
s.add_xor_clause([1, 2], False)     # x1 == x2
```

### Assumptions and "why is it UNSAT?"

`solve(assumptions=[...])` solves *temporarily* under the given literals without
permanently adding them. The solver state is unchanged afterward, so you can ask
many what-if questions on the same instance.

When a call with assumptions returns UNSAT, `get_conflict()` returns a subset of
those assumption literals that are jointly responsible — a minimal explanation of
the contradiction. Use this to report *which* requirements conflict.

```python
sat, model = s.solve(assumptions=[2, 4])
if not sat:
    print("conflicting assumptions:", s.get_conflict())
```

### Resource limits

`Solver(...)` keyword args:

- `time_limit` (float, seconds) — wall-clock budget. Non-deterministic across
  runs (depends on machine load).
- `confl_limit` (int) — budget in conflicts. **Prefer this for reproducible
  runs.**
- `threads` (int, default 1) — parallel solving.
- `verbose` (int, default 0) — solver logging.

If a limit is hit, `solve()` returns `(None, None)`.

## End-to-end example

Problem: there are three switches A, B, C. At least one is on. A and B can't
both be on. If A is on, C must be on. Find a valid configuration; then check
whether a configuration with A on but C off is possible.

```python
from pycryptosat import Solver

pool = VarPool()
A, B, C = pool.v("A"), pool.v("B"), pool.v("C")

s = Solver()
s.add_clause([A, B, C])     # at least one on
s.add_clause([-A, -B])      # not both A and B
s.add_clause([-A, C])       # A implies C

sat, model = s.solve()
print("satisfiable:", sat)
if sat:
    print({n: model[v] for n, v in pool._m.items()})

# What-if: A on and C off — should be impossible (A implies C)
sat2, _ = s.solve(assumptions=[A, -C])
print("A on & C off possible:", sat2)            # False
if not sat2:
    print("conflict:", s.get_conflict())          # subset of [A, -C]
```

## Workflow checklist for the agent

1. Identify the Boolean decisions in the problem; allocate one variable per
   decision via the `VarPool`, recording the meaning of each.
2. Translate every condition into clauses using the table above. Re-read the
   problem and confirm every stated constraint produced at least one clause.
3. `solve()`. Branch on `True` / `False` / `None`.
4. On `True`, decode `model[var]` back to concept names and present the answer in
   the problem's own language — never as raw variable numbers.
5. On `False`, report that the constraints are contradictory; if the user asked a
   conditional/what-if question, use `assumptions` + `get_conflict()` to explain
   which conditions clash.
6. On `None`, raise `confl_limit`/`time_limit` and retry, and tell the user the
   instance was too hard within the budget.

## When NOT to use this

CryptoMiniSat is *Boolean only*. It has no native theories. Reach for an SMT
solver (e.g. Z3) instead when the problem centers on:

- integer or real arithmetic, inequalities over numbers, linear/nonlinear
  constraints (unless ranges are tiny and you bit-blast them yourself);
- optimization (`minimize`/`maximize`) — CMS only decides SAT/UNSAT. Optimization
  requires an external MaxSAT-style search loop you'd build on top;
- strings, arrays, uninterpreted functions, quantifiers.

CMS shines on purely combinatorial / Boolean-constraint problems and especially
on instances with many XOR/parity constraints.
