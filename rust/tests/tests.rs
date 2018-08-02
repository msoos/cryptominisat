// Copyright (c) 2016 Robert Grosse

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

extern crate cryptominisat;
use cryptominisat::*;

fn new_lit(var: u32, neg: bool) -> Lit {
    Lit::new(var, neg).unwrap()
}

#[test]
fn readme_code() {
    let mut solver = Solver::new();
    let mut clause = Vec::new();

    solver.set_num_threads(4);
    solver.new_vars(3);

    clause.push(new_lit(0, false));
    solver.add_clause(&clause);

    clause.clear();
    clause.push(new_lit(1, true));
    solver.add_clause(&clause);

    clause.clear();
    clause.push(new_lit(0, true));
    clause.push(new_lit(1, false));
    clause.push(new_lit(2, false));
    solver.add_clause(&clause);

    let ret = solver.solve();

    assert!(ret == Lbool::True);
    assert!(solver.get_model()[0] == Lbool::True);
    assert!(solver.get_model()[1] == Lbool::False);
    assert!(solver.get_model()[2] == Lbool::True);
}

#[test]
fn xor_4_long() {
    let mut s = Solver::new();
    s.new_vars(4);
    s.add_xor_clause(&vec![0, 1, 2, 3], false);
    s.add_xor_clause(&vec![0], false);
    s.add_xor_clause(&vec![1], false);
    s.add_xor_clause(&vec![2], false);
    let ret = s.solve();
    assert!(ret == Lbool::True);
    assert!(s.get_model()[0] == Lbool::False);
    assert!(s.get_model()[1] == Lbool::False);
    assert!(s.get_model()[2] == Lbool::False);
    assert!(s.get_model()[3] == Lbool::False);
    assert!(s.nvars() == 4);
}

#[test]
fn replace_false() {
    let mut s = Solver::new();
    s.new_vars(2);
    s.add_clause(&vec![new_lit(0, false), new_lit(1, true)]); //a V -b
    s.add_clause(&vec![new_lit(0, true), new_lit(1, false)]); //-a V b
    // a == b

    let mut assumps = Vec::new();
    assumps.push(new_lit(0, false));
    assumps.push(new_lit(1, true));
    // a = 1, b = 0

    let ret = s.solve_with_assumptions(&assumps);
    assert!(ret == Lbool::False);

    assert!(s.get_conflict().len() == 2);

    let mut tmp = s.get_conflict().to_vec();
    tmp.sort();
    assert!(tmp[0] == new_lit(0, true));
    assert!(tmp[1] == new_lit(1, false));
}

#[test]
fn sugared_readme_code() {
    let mut s = Solver::new();
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

#[test]
fn sugared_xor_code() {
    let mut s = Solver::new();
    let a = s.new_var();
    let b = s.new_var();
    let c = s.new_var();
    let d = s.new_var();

    s.add_xor_literal_clause(&[a, !b, c, d], false);
    s.add_xor_literal_clause(&[!a, !b], false);
    s.add_xor_literal_clause(&[c, !b, d], false);
    s.add_xor_literal_clause(&[a, !!d], true);
    assert!(s.solve() == Lbool::True);
    assert!(s.is_true(!a));
    assert!(s.is_true(!b));
    assert!(s.is_true(!c));
    assert!(s.is_true(d));

    assert!(s.solve_with_assumptions(&[c]) == Lbool::False);
    assert!(s.solve_with_assumptions(&[!a, d]) == Lbool::True);
}

#[test]
fn assumptions_test() {
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

    assert!(s.solve() == Lbool::True);
    assert!(s.is_true(c));

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
}
