# pycryptosat: bindings to the CryptoMiniSat SAT solver

This directory provides Python bindings to CryptoMiniSat on the C++ level,
i.e. when importing pycryptosat, the CryptoMiniSat solver becomes part of the
Python process itself.

## Compiling
The pycryptosat python package compiles separately from the CryptoMiniSat library,
so if you have special flags included with CryptoMiniSat, it will NOT be reflected
in the python package.

In order to compile, install the python developer tools:

```
apt-get install python-dev
```

Then run

```
python -m build
```

## Usage

The `pycryptosat` module has one object, `Solver` that has two functions
`solve` and `add_clause`.

The funcion `add_clause()` takes an iterable list of literals such as
`[1, 2]` which represents the truth `1 or 2 = True`. For example,
`add_clause([1])` sets variable `1` to `True`.

The function `solve()` solves the system of equations that have been added
with `add_clause()`:

```
>>> from pycryptosat import Solver
>>> s = Solver()
>>> s.add_clause([1, 2])
>>> sat, solution = s.solve()
>>> print sat
True
>>> print solution
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
  * `time_limit`: the time limit (integer)
  * `confl_limit`: the propagation limit (integer)
  * `verbose`: the verbosity level (integer)

Both `time_limit` and `confl_limit` set a budget to the solver. The former is based on time elapsed while the former is based on number of conflicts met during search. If the solver runs out of budget, it returns with `(None, None)`. If both limits are used, the solver will terminate whenever one of the limits are hit (whichever first). Warning: Results from `time_limit` may differ from run to run, depending on compute load, etc. Use `confl_limit` for more reproducible runs.

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
