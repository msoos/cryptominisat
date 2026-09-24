# pycryptosat SAT solver

This directory provides Python bindings to CryptoMiniSat on the C++ level,
i.e. when importing pycryptosat, the CryptoMiniSat solver becomes part of the
Python process itself.

## Installing

```
pip install pycryptosat
```

## Building from source

The build uses
[scikit-build-core](https://github.com/scikit-build/scikit-build-core), which
drives CMake under the hood. CMake automatically fetches and builds the required
`cadical` and `cadiback` dependencies, so **no manual dependency installation is
needed beyond GMP**.

### Quick start (Linux)

```sh
sudo apt-get install libgmp-dev   # or: yum install gmp-devel
python -m venv venv
source venv/bin/activate
pip install scikit-build-core cmake ninja build
pip install . --no-build-isolation
```

### Quick start (macOS)

```sh
brew install gmp
python -m venv venv
source venv/bin/activate
pip install scikit-build-core cmake ninja build
pip install . --no-build-isolation
```

### Build a wheel without installing

```sh
python -m venv venv
source venv/bin/activate
pip install scikit-build-core cmake ninja build
python -m build --wheel --no-isolation   # wheel lands in dist/
```

The resulting wheel is fully self-contained on macOS (GMP is bundled by
`delocate`). On Linux the wheel depends on `libgmp.so.10`, which is part of
the `manylinux` ABI and is present on any standard distribution.

## Usage

The `pycryptosat` module has one object, `Solver`, with the following methods:
`solve`, `add_clause`, `add_clauses`, `add_xor_clause`, `set_option`,
`nb_vars`, `is_satisfiable`, and `get_conflict`.

The function `add_clause()` takes an iterable list of literals such as
`[1, 2]` which represents the truth `1 or 2 = True`. For example,
`add_clause([1])` sets variable `1` to `True`.

The function `solve()` solves the system of equations that have been added
with `add_clause()`:

```
>>> from pycryptosat import Solver
>>> s = Solver()
>>> s.add_clause([1, 2])
>>> sat, solution = s.solve()
>>> print(sat)
True
>>> print(solution)
(None, True, True)
```

The return value is a tuple. First part of the tuple indicates whether the
problem is satisfiable. In this case, it's `True`, i.e. satisfiable. The second
part is a tuple contains the solution, preceded by None, so you can index into
it with the variable number. E.g. `solution[1]` returns the value for
variable `1`.

The `solve()` method optionally takes an argument `assumptions` that
allows the user to set values to specific variables in the solver in a temporary
fashion. This means that in case the problem is satisfiable but e.g it's
unsatisfiable if variable 2 is FALSE, then `solve([-2])` will return
UNSAT. However, a subsequent call to `solve()` will still return a solution.
If instead of an assumption `add_clause()` would have been used, subsequent
`solve()` calls would have returned unsatisfiable.

`Solver` takes the following keyword arguments:
  * `verbose`: the verbosity level (integer, default 0)
  * `time_limit`: the time limit in seconds (float)
  * `confl_limit`: the conflict limit (integer)
  * `threads`: the number of threads to use (integer, default 1)
  * `options`: solver options, see [Solver options](#solver-options) (dict)

Both `time_limit` and `confl_limit` set a budget to the solver. The former is
based on time elapsed while the latter is based on number of conflicts met
during search. If the solver runs out of budget, it returns with `(None, None)`.
If both limits are used, the solver will terminate whenever one of the limits
are hit (whichever first). Warning: Results from `time_limit` may differ from
run to run, depending on compute load, etc. Use `confl_limit` for more
reproducible runs.

## Solver options

Solver options can be passed to the constructor as a dict, or set one by one
with `set_option(name, value)`:

```
>>> from pycryptosat import Solver
>>> s = Solver(options={"maxmatrixrows": "5000", "polar": "rnd"})
>>> s.set_option("seed", "42")
```

Names are the `cryptominisat5` command-line options without the leading `--`,
and both names and values must be strings. Options must be set before the
first `add_clause()`, `add_clauses()`, `add_xor_clause()` or `solve()` call.
Errors:
  * `ValueError`: unknown option or invalid value
  * `TypeError`: name or value is not a string, or `options` is not a dict
  * `RuntimeError`: `set_option()` called too late

Integer and boolean options take `"0"`/`"1"`. Run `cryptominisat5 --help` to
see the default values and the full descriptions.
`pycryptosat.get_option_names()` returns the names of all available options.

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
| `breakid` | Break symmetries with BreakID, if compiled in |

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

## Example

Let us consider the following clauses, represented using
the DIMACS `cnf <http://en.wikipedia.org/wiki/Conjunctive_normal_form>`_
format::

```
p cnf 5 3
1 -5 4 0
-1 5 3 4 0
-3 -4 0
```

Here, we have 5 variables and 3 clauses, the first clause being
(x\ :sub:`1`  or not x\ :sub:`5` or x\ :sub:`4`).
Note that the variable x\ :sub:`2` is not used in any of the clauses,
which means that for each solution with x\ :sub:`2` = True, we must
also have a solution with x\ :sub:`2` = False.  In Python, each clause is
most conveniently represented as a list of integers.  Naturally, it makes
sense to represent each solution also as a list of integers, where the sign
corresponds to the Boolean value (+ for True and - for False) and the
absolute value corresponds to i\ :sup:`th` variable::

```
>>> import pycryptosat
>>> solver = pycryptosat.Solver()
>>> solver.add_clause([1, -5, 4])
>>> solver.add_clause([-1, 5, 3, 4])
>>> solver.add_clause([-3, -4])
>>> solver.solve()
(True, (None, True, False, False, True, True))
```

This solution translates to: x\ :sub:`1` = x\ :sub:`4` = x\ :sub:`5` = True,
x\ :sub:`2` = x\ :sub:`3` = False

# Special CMake options

Extra compile definitions (e.g. `LARGE_OFFSETS`) can be passed via
`SKBUILD_CMAKE_ARGS` or added to `pyproject.toml`'s `[tool.scikit-build]
cmake.args` list.
