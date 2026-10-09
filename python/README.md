# pycryptosat SAT solver

Python bindings to CryptoMiniSat. The solver runs inside the Python process.

## Installing

```
pip install pycryptosat
```

To build from source, you need a C++ compiler, GMP and pkg-config (e.g.
`sudo apt-get install build-essential libgmp-dev pkg-config` or
`brew install gmp pkgconf`), and network access, since the build downloads
CaDiCaL and CaDiBack. Then, from the root of the repository:

```
pip install .
```

## Usage

```
>>> from pycryptosat import Solver
>>> s = Solver()
>>> s.add_clause([1, 2])
>>> s.add_clause([-1])
>>> s.solve()
(True, (None, False, True))
>>> s.solve([-2])
(False, None)
>>> s.get_conflict()
[2]
>>> s.solve()
(True, (None, False, True))
```

A literal is a non-zero integer: `1` means variable 1 is True, `-1` that it is
False. `add_clause([1, 2])` adds the clause "1 or 2".

`solve()` returns `(True, solution)` if satisfiable, `(False, None)` if
unsatisfiable, and `(None, None)` if it ran out of budget. `solution` is a
tuple starting with `None`, so that `solution[i]` is the value of variable `i`.

`solve()` optionally takes a list of assumptions: literals that are assumed to
hold for that call only. Above, `solve([-2])` is unsatisfiable, but the next
`solve()` is satisfiable again. After an unsatisfiable `solve()`,
`get_conflict()` returns the negation of the assumptions responsible for it.

`Solver` methods:
  * `add_clause(literals)`
  * `add_clauses(clauses)`: a list of clauses, or a flat `array.array` of
    zero-terminated clauses (typecode `i`, `l` or `q`)
  * `add_xor_clause(variables, rhs)`: the XOR of the (positive) `variables`
    equals the bool `rhs`
  * `solve(assumptions=None, verbose=None, time_limit=None, confl_limit=None)`
  * `is_satisfiable()`: `solve()`, returning only the first element
  * `get_conflict()`
  * `nb_vars()`: the number of variables
  * `set_option(name, value)`: see [Solver options](#solver-options)

`Solver` keyword arguments:
  * `verbose`: verbosity level (int, default 0)
  * `time_limit`: CPU time limit in seconds, per `solve()` call (float, default
    no limit). Not reproducible from run to run; prefer `confl_limit`.
  * `confl_limit`: conflict limit, per `solve()` call (int, default no limit)
  * `threads`: number of threads (int, default 1)
  * `options`: dict of solver options, see [Solver options](#solver-options)

`verbose`, `time_limit` and `confl_limit` can also be overridden for a single
`solve()` call.

The module also has `get_option_names()` and `VERSION`.

## Solver options

Solver options can be passed to the constructor as a dict, or set one by one
with `set_option(name, value)`:

```
>>> from pycryptosat import Solver
>>> s = Solver(options={"maxmatrixrows": "5000", "polar": "rnd"})
>>> s.set_option("seed", "42")
```

Names are `cryptominisat5` command-line options without the leading `--`,
and both names and values are strings. Only the options below are available,
`pycryptosat.get_option_names()` returns them. Options must be set before the
first `add_clause()`, `add_clauses()`, `add_xor_clause()`, `solve()` or
`is_satisfiable()` call. Errors:
  * `ValueError`: unknown option or invalid value
  * `TypeError`: name or value is not a string, or `options` is not a dict
  * `RuntimeError`: `set_option()` called too late

Boolean options take `"0"` or `"1"`. Run `cryptominisat5 --help` to see the
default values and the full descriptions.

General:

| Option | Meaning |
|---|---|
| `seed` | Random seed |
| `mult` | Multiplier for all simplification cutoffs |
| `polar` | Polarity mode: `true`, `false`, `rnd`, `weight` or `auto` |
| `branchstr` | Branching strategies to switch between while solving |

Search:

| Option | Meaning |
|---|---|
| `restart` | Enable restarts |
| `stabilize` | Alternate stable and focused phases |
| `reduce` | Enable learnt clause database reduction |
| `lucky` | Search for lucky phases before the CDCL loop |
| `sls` | Run local search during rephasing |
| `rephase` | Enable resetting the saved phases |
| `target` | Target phases: 0 = never, 1 = in stable phases, 2 = always |
| `nonstop` | Never stop the search |

Simplification:

| Option | Meaning |
|---|---|
| `schedsimp` | Perform simplification rounds. 0 turns off all inprocessing |
| `presimp` | Simplify at the very start |
| `schedule` | Simplification schedule during the run |
| `preschedule` | Simplification schedule at startup |
| `confbtwsimp` | Conflicts before the first simplification |
| `occsimp` | Occurrence-based simplification (BVE, subsumption, ...) |
| `varelim` | Bounded variable elimination |
| `bva` | Bounded variable addition |
| `distill` | Clause distillation |
| `sweep` | SAT sweeping |
| `scc` | Find and replace equivalent literals |
| `intree` | Intree probing |
| `transred` | Transitive reduction of binary clauses |

XOR and Gaussian elimination:

| Option | Meaning |
|---|---|
| `xor` | Recover XORs from the clauses |
| `maxxorsize` | Largest XOR to recover (at most 12) |
| `xorfindtout` | Effort limit for XOR recovery |
| `maxxormat` | Largest matrix to echelonize during XOR recovery |
| `xorgatemaxsize` | Largest clause XOR-gate finding considers |
| `maxmatrixrows` | Gauss matrices with more rows are discarded |
| `maxmatrixcols` | Gauss matrices with more columns are discarded |
| `minmatrixrows` | Gauss matrices with fewer rows are discarded |
| `maxnummatrices` | Maximum number of Gauss matrices |
| `autodisablegauss` | Turn off Gauss matrices that perform badly |
| `gaussusefulcutoff` | Usefulness ratio below which a matrix is turned off |
| `gaussmincalls` | Gauss calls before a matrix may be turned off |
| `gausscheckevery` | Check whether to turn off a matrix every N conflicts |
