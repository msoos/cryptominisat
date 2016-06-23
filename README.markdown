CryptoMiniSat SAT solver
===========================================

This system provides CryptoMiniSat, an advanced SAT solver. The system has 3
interfaces: command-line, C++ library and python. The command-line interface
takes a [cnf](http://en.wikipedia.org/wiki/Conjunctive_normal_form) as an
input in the [DIMACS](http://www.satcompetition.org/2009/format-benchmarks2009.html)
format with the extension of XOR clauses. The C++ interface mimics this except
that it allows for a more efficient system, with assumptions and multiple
`solve()` calls. The python system is an interface to the C++ system that
provides the best of both words: ease of use and a powerful interface.

TravisCi: [![Build Status](https://travis-ci.org/msoos/cryptominisat.svg?branch=master)](https://travis-ci.org/msoos/cryptominisat)

AppVeyor: [![Build Status Appveyor](https://ci.appveyor.com/api/projects/status/github/gruntjs/grunt?branch=master&svg=true)](https://ci.appveyor.com/project/msoos/cryptominisat)

<a href="https://scan.coverity.com/projects/507">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/507/badge.svg"/>
</a>

[![Coverage Status](https://coveralls.io/repos/msoos/cryptominisat/badge.svg?branch=master)](https://coveralls.io/r/msoos/cryptominisat?branch=master)

Prerequisites
-----

You need to have the following installed in case you use Debian or Ubuntu -- for
other distros, the packages should be similarly named::
```
$ sudo apt-get install build-essential cmake
```

The following are not required but are useful::
```
$ sudo apt-get install valgrind libm4ri-dev libmysqlclient-dev libsqlite3-dev
```

Compiling and installing
-----

You have to use cmake to compile and install. I suggest::
```
$ tar xzvf my-cryptominisat-tarball.tar.gz
$ cd cryptominisat-version
$ mkdir build
$ cd build
$ cmake ..
$ make -j4
$ sudo make install
```

Once cryptominisat is installed, the binary is available under
`/usr/local/bin/cryptominisat4`, the library shared library is available
under `/usr/local/lib/libcryptominisat4.so` and the 3 header files are
available under `/usr/local/include/cryptominisat4/`. To use the python
bindings, you must have python installed while compiling and after the
compilation has finished, issue:

```
$ sudo ldconfig
```

You can uninstall both by simply doing `sudo make uninstall` in their respective
directories.

Command-line usage
-----

Let's take the file::
```
p cnf 2 3
1 0
-2 0
-1 2 3 0
```

The files has 3 clauses and 2 variables, this is reflected in the header
`p cnf 2 3`. Every clause is ended by '0'. The clauses say: 1 must be True, 2
must be False, and either 1 has to be False, 2 has to be True or 3 has to be
True. The only solution to this problem is::
```
$ cryptominisat4 --verb 0 file.cnf
s SATISFIABLE
v 1 -2 3 0
```

If the file had contained::
```
p cnf 2 4
1 0
-2 0
-3 0
-1 2 3 0
```

Then there is no solution and the solver returns `s UNSATISFIABLE`.

Python usage
-----

The python module is under the directory `python`. You have to first compile
and install this module, as explained above. You can then use it as::

```
>>> from pycryptosat import Solver
>>> s = Solver()
>>> s.add_clause([1])
>>> s.add_clause([-2])
>>> s.add_clause([3])
>>> s.add_clause([-1, 2, 3])
>>> sat, solution = s.solve()
>>> print sat
True
>>> print solution
(None, True, False, True)
```

We can also try to assume any variable values for a single solver run::
```
>>> sat, solution = s.solve([-3])
>>> print sat
False
>>> print solution
None
>>> sat, solution = s.solve()
>>> print sat
True
>>> print solution
(None, True, False, True)
```

For more detailed instruction, please see the README.rst under the `python`
directory.

Library usage
-----
The library uses a variable numbering scheme that starts from 0. Since 0 cannot
be negated, the class `Lit` is used as: `Lit(variable_number, is_negated)`. As
such, the 1st CNF above would become::

```
#include <cryptominisat4/cryptominisat.h>
#include <assert.h>
#include <vector>
using std::vector;
using namespace CMSat;

int main()
{
    SATSolver solver;
    vector<Lit> clause;

    //We need 3 variables
    solver.new_vars(3);

    //Let's use 4 threads
    solver.set_num_threads(4);

    //adds "1 0"
    clause.push_back(Lit(0, false));
    solver.add_clause(clause);

    //adds "-2 0"
    clause.clear();
    clause.push_back(Lit(1, true));
    solver.add_clause(clause);

    //adds "-1 2 3 0"
    clause.clear();
    clause.push_back(Lit(0, true));
    clause.push_back(Lit(1, false));
    clause.push_back(Lit(2, false));
    solver.add_clause(clause);

    lbool ret = solver.solve();
    assert(ret == l_True);
    assert(solver.get_model()[0] == l_True);
    assert(solver.get_model()[1] == l_False);
    assert(solver.get_model()[2] == l_True);
    std::cout
    << "Solution is: "
    << solver.get_model()[0]
    << ", " << solver.get_model()[1]
    << ", " << solver.get_model()[2]
    << std::endl;

    return 0;
}
```

The library usage also allows for assumptions. We can add these lines just
before the `return 0;` above::
```
vector<Lit> assumptions;
assumptions.push_back(Lit(2, true));
lbool ret = solver.solve(assumptions);
assert(ret == l_False);

lbool ret = solver.solve();
assert(ret == l_True);
```

Since we assume that variabe 2 must be false, there is no solution. However,
if we solve again, without the assumption, we get back the original solution.
Assumptions allow us to assume certain literal values for a _specific run_ but
not all runs -- for all runs, we can simply add these assumptions as 1-long
clauses.

Multiple solutions
-----

To find multiple solutions to your problem, just run the solver in a loop
and ban the previous solution found:

```
while(true) {
    lbool ret = solver->solve();
    if (ret != l_True) {
        assert(ret == l_False);
        //All solutions found.
        exit(0);
    }

    //Use solution here. print it, for example.

    //Banning found solution
    vector<Lit> ban_solution;
    for (uint32_t var = 0; var < solver->nVars(); var++) {
        if (solver->get_model()[var] != l_Undef) {
            ban_solution.push_back(
                Lit(var, (solver->get_model()[var] == l_True)? true : false));
        }
    }
    solver->add_clause(ban_solution);
}
```

The above loop will run as long as there are solutions. It is __highly__
suggested to __only__ add into the new clause(`bad_solutions` above) the
variables that are "important" or "main" to your problem. Variables that were
only used to translate the original problem into CNF should not be added.
This way, you will not get spurious solutions that don't differ in the main,
important variables.


Testing
-----
For testing you will need the GIT checkout and get the submodules:

```
git clone https://github.com/msoos/cryptominisat.git
cd cryptominisat
git submodules init
git submodules update
```

Then you need to build with `-DENABLE_TESTING=ON`, build and run the tests:

```
mkdir build
cmake -DENABLE_TESTING=ON ..
make -j4
make test
```
