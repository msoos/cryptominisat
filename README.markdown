[![release](https://img.shields.io/badge/Release-v5.0.0-blue.svg)](https://github.com/msoos/cryptominisat/releases/tag/cryptominisat-5.0.0)
![licence](https://img.shields.io/badge/License-LGPLv2-brightgreen.svg)
[![linux build](https://travis-ci.org/msoos/cryptominisat.svg?branch=master)](https://travis-ci.org/msoos/cryptominisat)
[![windows build](https://ci.appveyor.com/api/projects/status/github/gruntjs/grunt?branch=master&svg=true)](https://ci.appveyor.com/project/msoos/cryptominisat)
<a href="https://scan.coverity.com/projects/507">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/507/badge.svg"/>
</a>
[![code coverage](https://coveralls.io/repos/msoos/cryptominisat/badge.svg?branch=master)](https://coveralls.io/r/msoos/cryptominisat?branch=master)

CryptoMiniSat SAT solver
===========================================

This system provides CryptoMiniSat, an advanced SAT solver. The system has 3
interfaces: command-line, C++ library and python. The command-line interface
takes a [cnf](http://en.wikipedia.org/wiki/Conjunctive_normal_form) as an
input in the [DIMACS](http://www.satcompetition.org/2009/format-benchmarks2009.html)
format with the extension of XOR clauses. The C++ interface mimics this except
that it allows for a more efficient system, with assumptions and multiple
`solve()` calls. The python system is an interface to the C++ system that
provides the best of both worlds: ease of use and a powerful interface.

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

Compiling and installing under Linux
-----

You have to use cmake to compile and install. I suggest::
```
$ tar xzvf cryptominisat-version.tar.gz
$ cd cryptominisat-version
$ mkdir build
$ cd build
$ cmake ..
$ make -j4
$ sudo make install
$ sudo ldconfig
```

Once cryptominisat is installed, the binary is available under
`/usr/local/bin/cryptominisat5`, the library shared library is available
under `/usr/local/lib/libcryptominisat5.so` and the 3 header files are
available under `/usr/local/include/cryptominisat5/`.You can uninstall
both by executing `sudo make uninstall`.

Compiling under Windows
-----

```
$ unzip cryptominisat-version.zip
$ cd cryptominisat-version
$ cmake -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 14 2015 Win64" -DSTATICCOMPILE=ON
$ msbuild INSTALL.vcxproj
```

The `cryptominisat5_simple` binary should now be built. In case you have boost libraries installed, it may also detect it, and you may get the full `cryptominisat5` binary built too. The two binaries only differ in the number of options supported.

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
$ cryptominisat5 --verb 0 file.cnf
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
#include <cryptominisat5/cryptominisat.h>
#include <assert.h>
#include <vector>
using std::vector;
using namespace CMSat;

int main()
{
    SATSolver solver;
    vector<Lit> clause;

    //Let's use 4 threads
    solver.set_num_threads(4);

    //We need 3 variables
    solver.new_vars(3);

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

Preprocessor usage
-----

Run cryptominisat5 as:

```
./cryptominisat5 -p1 input.cnf simplified.cnf
some_sat_solver simplified.cnf > output
./cryptominisat5 -p2 output
```

where `some_sat_solver` is a SAT solver of your choice that outputs a solution in the format of:

```
s SATISFIABLE
v [solution] 0
```

or 

```
s UNSATISFIABLE
```

You can tune the schedule of simplifications by issuing `--sched "X,Y,Z..."`. The default schedule for preprocessing is:

```
handle-comps,scc-vrepl, cache-clean, cache-tryboth,sub-impl, intree-probe, probe,
sub-str-cls-with-bin, distill-cls, scc-vrepl, sub-impl,occ-backw-sub-str,
occ-xor, occ-clean-implicit, occ-bve, occ-bva, occ-gates,str-impl, cache-clean,
sub-str-cls-with-bin, distill-cls, scc-vrepl, sub-impl,str-impl, sub-impl,
sub-str-cls-with-bin, occ-backw-sub-str, occ-bve,check-cache-size, renumber
```

It is a good idea to put `renumber` as late as possible, as it renumbers the variables for memory usage reduction.

Gaussian elimination
-----
For building with Gaussian Elimination, you need to perform:

```
git clone https://github.com/msoos/cryptominisat.git
cd cryptominisat
mkdir build && cd build
cmake -DUSE_GAUSS=ON ..
make
```

To use Gaussian elimination, provide a CNF with xors in it (either in CNF or XOR+CNF form) and tune the gaussian parameters. Use `--hhelp` to find all the gaussian elimination options:

```
Gauss options:
  --iterreduce arg (=1)       Reduce iteratively the matrix that is updated.We
                              effectively are moving the start to the last
                              column updated
  --maxmatrixrows arg (=3000) Set maximum no. of rows for gaussian matrix. Too
                              large matrixesshould bee discarded for reasons of
                              efficiency
  --autodisablegauss arg (=1) Automatically disable gauss when performing badly
  --minmatrixrows arg (=5)    Set minimum no. of rows for gaussian matrix.
                              Normally, too smallmatrixes are discarded for
                              reasons of efficiency
  --savematrix arg (=2)       Save matrix every Nth decision level
  --maxnummatrixes arg (=3)   Maximum number of matrixes to treat.
```

Testing
-----
For testing you will need the GIT checkout and get the submodules:

```
git clone https://github.com/msoos/cryptominisat.git
cd cryptominisat
git submodule init
git submodule update
```

Then you need to build with `-DENABLE_TESTING=ON`, build and run the tests:

```
mkdir build
cd build
cmake -DENABLE_TESTING=ON ..
make -j4
make test
```

Web-based run explorer
-----
Please see under web/README.markdown for details. This is an experimental feature.
