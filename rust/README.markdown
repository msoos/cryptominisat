Rust bindings for CryptoMiniSat
===========================================

This provides Rust bindings for CryptoMiniSat, an open source SAT solver, written in C++. You can find more information about it [here](https://github.com/msoos/cryptominisat).

Basic Usage
-----

Here is an example of basic usage.

```
extern crate cryptominisat;
use cryptominisat::*;

fn main() {
    let mut s = Solver::new();
    s.set_num_threads(4);

    let x = s.new_var();
    let y = s.new_var();
    let z = s.new_var();

    s.add_clause(&[x]);
    s.add_clause(&[!y]);
    s.add_clause(&[!x, y, z]);

    assert!(s.solve() == Lbool::True);
    assert!(s.is_true(x));
    assert!(s.is_true(!y));
    assert!(s.is_true(z));
}
```

Variables are indexed by unsigned integers 0...n-1. A literal represents either a variable or a negated variable. A clause is a set of literals, at least one of which must be true in the final solution. For convience, new_var() both adds a variable and returns a literal representing the new variable. However, you can manually create literals and clauses if you want to. The above code is syntactic sugar for the following.

```
    let mut solver = Solver::new();
    let mut clause = Vec::new();

    solver.set_num_threads(4);
    solver.new_vars(3);

    clause.push(Lit::new(0, false).unwrap());
    solver.add_clause(&clause);

    clause.clear();
    clause.push(Lit::new(1, true).unwrap());
    solver.add_clause(&clause);

    clause.clear();
    clause.push(Lit::new(0, true).unwrap());
    clause.push(Lit::new(1, false).unwrap());
    clause.push(Lit::new(2, false).unwrap());
    solver.add_clause(&clause);

    let ret = solver.solve();
    assert!(ret == Lbool::True);
    assert!(solver.get_model()[0] == Lbool::True);
    assert!(solver.get_model()[1] == Lbool::False);
    assert!(solver.get_model()[2] == Lbool::True);
```

Note that Lit::new() returns None for n >= 2^31, but Cryptominisat itself imposes a much lower limit of MAX_NUM_VARS, currently 2^28 - 1.

Assumptions
-----

The library usage also allows for assumptions. An assumption tells the solver that a given literal should be assumed to be true, *for this run only*. This differs from adding it via add_clause() because it can easily be undone and benefits from previous work done by the solver, whereas removing an added clause requires recreating the solver entirely, and throwing away all the internal saved progress. In the following example, assuming different values for a results in differing values for b and d as well.

```
    let mut s = Solver::new();
    let a = s.new_var();
    let b = s.new_var();
    let c = s.new_var();
    let d = s.new_var();

    s.add_clause(&[a, b, !c]);
    s.add_clause(&[a, !b, c]);
    s.add_clause(&[!a, b, c]);
    s.add_clause(&[!a, !b, !c]);

    s.add_clause(&[!d, b, !c]);
    s.add_clause(&[!d, !b, c]);
    s.add_clause(&[d, b, c]);
    s.add_clause(&[d, !b, !c]);

    s.add_clause(&[a, d, !c]);
    s.add_clause(&[a, !d, c]);
    s.add_clause(&[!a, d, c]);
    s.add_clause(&[!a, !d, !c]);

    assert!(s.solve_with_assumptions(&[a]) == Lbool::True);
    assert!(s.is_true(a));
    assert!(s.is_true(!b));
    assert!(s.is_true(c));
    assert!(s.is_true(!d));

    assert!(s.solve_with_assumptions(&[!a]) == Lbool::True);
    assert!(s.is_true(!a));
    assert!(s.is_true(b));
    assert!(s.is_true(c));
    assert!(s.is_true(d));
```

Xor clauses
-----

In addition to traditional OR clauses, Cryptominisat also supports XOR clauses. These take an additional bool parameter, rhs, which gives the expected value of xoring together all the specified literals. For example, the following line imposes the constraint that a is true or d is false, but not both. If the second parameter was false instead of true, it would require that either a is true and d is false or a is false and d is true.

```
    s.add_xor_literal_clause(&[a, !d], true);
```

Error handling
-----
Cryptominisat handles errors by writing a message to stderr and then aborting. If you want panics instead, you should write a wrapper that detects improper usage beforehand and panics. A nonexhaustive list of conditions that can cause errors includes

* Passing 0 to set_num_threads()
* Calling set_num_threads() after any other methods have been called on the solver
* Creating too many variables
* Passing a variable that hasn't been added to the solver

License
-----
MIT