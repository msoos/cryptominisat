[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
![build](https://github.com/msoos/cryptominisat/workflows/build/badge.svg)


# CryptoMiniSat SAT solver
This system provides CryptoMiniSat, an advanced incremental SAT solver. The
system has 3 interfaces: command-line, C++ library and python. The command-line
interface takes a [cnf](http://en.wikipedia.org/wiki/Conjunctive_normal_form)
as an input in the
[DIMACS](http://www.satcompetition.org/2009/format-benchmarks2009.html) format
with the extension of XOR clauses. The C++ and python interface mimics this and
also allows for incremental use: assumptions and multiple `solve` calls. A C
and a Rust compatible wrapper is also provided.

When citing, always reference our [SAT 2009 conference
paper](https://link.springer.com/chapter/10.1007%2F978-3-642-02777-2_24),
bibtex record is [here](http://dblp.uni-trier.de/rec/bibtex/conf/sat/SoosNC09).


## Compiling
Use of the [release binaries](https://github.com/msoos/cryptominisat/releases) is
_strongly_ encouraged, as Ganak requires a specific set of libraries to be
installed. The second best thing to use is Nix. Simply [install
nix](https://nixos.org/download/) and then:
```shell
nix shell github:msoos/cryptominisat
```

Then you will have `cryptominisat` binary available and ready to use.

If this is somehow not what you want, you can also build it. See the [GitHub
Action](https://github.com/msoos/cryptominisat/actions/workflows/build.yml) for the
specific set of steps.

You can also run CryptoMiniSat from your web browser, without installing
anything, [here](https://www.msoos.org/cryptominisat/).

## Command-line usage
Let's take the file:
```plain
p cnf 3 3
1 0
-2 0
-1 2 3 0
```

The file has 3 variables and 3 clauses, this is reflected in the header `p cnf
3 3` which gives the number of variables as the first number and the number of
clauses as the second. Every clause is ended by '0'. The clauses say: 1 must be
True, 2 must be False, and either 1 has to be False, 2 has to be True or 3 has
to be True. The only solution to this problem is:
```plain
cryptominisat5 --verb 0 file.cnf
s SATISFIABLE
v 1 -2 3 0
```

Which means, that setting variable 1 True, variable 2 False and variable 3 True
satisfies the set of constraints (clauses) in the CNF. If the file had
contained:
```plain
p cnf 3 4
1 0
-2 0
-3 0
-1 2 3 0
```

Then there is no solution and the solver returns `s UNSATISFIABLE`.

## Incremental Python Usage
The python module works with both Python 3. Just execute:
```shell
pip3 install pycryptosat
```

You can then use it in incremental mode as:
```python
>>> from pycryptosat import Solver
>>> s = Solver()
>>> s.add_clause([1])
>>> s.add_clause([-2])
>>> s.add_clause([-1, 2, 3])
>>> sat, solution = s.solve()
>>> print sat
True
>>> print solution
(None, True, False, True)
>>> sat, solution = s.solve([-3])
>> print sat
False
>>> sat, solution = s.solve()
>>> print sat
True
>>> s.add_clause([-3])
>>> sat, solution = s.solve()
>>> print sat
False
```

We can also try to assume any variable values for a single solver run:
```python
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
If you want to build the python module, you can do this:
```shell
sudo apt-get install build-essential
sudo apt-get install python3-setuptools python3-dev
git clone https://github.com/msoos/cryptominisat
python -m build
pip install dist/pycryptosat-*.whl
```

### Incremental Library Usage
The library uses a variable numbering scheme that starts from 0. Since 0 cannot
be negated, the class `Lit` is used as: `Lit(variable_number, is_negated)`. As
such, the 1st CNF above would become:
```c++
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

    //We need 3 variables. They will be: 0,1,2
    //Variable numbers are always trivially increasing
    solver.new_vars(3);

    //add "1 0"
    clause.push_back(Lit(0, false));
    solver.add_clause(clause);

    //add "-2 0"
    clause.clear();
    clause.push_back(Lit(1, true));
    solver.add_clause(clause);

    //add "-1 2 3 0"
    clause.clear();
    clause.push_back(Lit(0, true));
    clause.push_back(Lit(1, false));
    clause.push_back(Lit(2, false));
    solver.add_clause(clause);

    lbool ret = solver.solve();
    assert(ret == l_True);
    std::cout
    << "Solution is: "
    << solver.get_model()[0]
    << ", " << solver.get_model()[1]
    << ", " << solver.get_model()[2]
    << std::endl;

    //assumes 3 = FALSE, no solutions left
    vector<Lit> assumptions;
    assumptions.push_back(Lit(2, true));
    ret = solver.solve(&assumptions);
    assert(ret == l_False);

    //without assumptions we still have a solution
    ret = solver.solve();
    assert(ret == l_True);

    //add "-3 0"
    //No solutions left, UNSATISFIABLE returned
    clause.clear();
    clause.push_back(Lit(2, true));
    solver.add_clause(clause);
    ret = solver.solve();
    assert(ret == l_False);

    return 0;
}
```

The library usage also allows for assumptions. We can add these lines just
before the `return 0;` above:
```
vector<Lit> assumptions;
assumptions.push_back(Lit(2, true));
lbool ret = solver.solve(&assumptions);
assert(ret == l_False);

lbool ret = solver.solve();
assert(ret == l_True);
```

Since we assume that variable 2 must be false, there is no solution. However,
if we solve again, without the assumption, we get back the original solution.
Assumptions allow us to assume certain literal values for a _specific run_ but
not all runs -- for all runs, we can simply add these assumptions as 1-long
clauses.

## Multiple solutions
To find multiple solutions to your problem, just run the solver in a loop
and ban the previous solution found:
```c
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

## Rust bindings
To build the Rust bindings:
```plain
git clone https://github.com/msoos/cryptominisat-rs/
cd cryptominisat-rs
cargo build --release
cargo test
```

You can use it as per the [README](https://github.com/msoos/cryptominisat-rs/blob/master/README.markdown) in that repository. To include CryptoMiniSat in your Rust project, add the dependency to your `Cargo.toml` file:
```
cryptominisat = { git = "https://github.com/msoos/cryptominisat-rs", branch= "master" }
```

You can see an example project using CryptoMiniSat in Rust [here](https://github.com/msoos/caqe/).

## Preprocessing
If you wish to use CryptoMiniSat as a preprocessor, we encourage you
to try out our model counting preprocessor, [Arjun](https://www.github.com/meelgroup/arjun).

## Gauss-Jordan elimination
Since CryptoMiniSat 5.8, Gauss-Jordan elimination is compiled into the solver
by default. However, it will turn off automatically in case the solver observes
GJ not to perform too well. To use Gaussian elimination, provide a CNF with
xors in it (either in CNF or XOR+CNF form) and either run with default setup,
or, tune it to your heart's desire:
```plain
Gauss options:
  --iterreduce arg (=1)       Reduce iteratively the matrix that is updated.We
                              effectively are moving the start to the last
                              column updated
  --maxmatrixrows arg (=3000) Set maximum no. of rows for gaussian matrix. Too
                              large matrixes should be discarded for reasons of
                              efficiency
  --autodisablegauss arg (=1) Automatically disable gauss when performing badly
  --minmatrixrows arg (=5)    Set minimum no. of rows for gaussian matrix.
                              Normally, too small matrixes are discarded for
                              reasons of efficiency
  --savematrix arg (=2)       Save matrix every Nth decision level
  --maxnummatrixes arg (=3)   Maximum number of matrixes to treat.
```

In particular, you may want to set `--autodisablegauss 0` in case you are sure it'll help.

## CrystalBall
Build and use instructions below. Please see the [associated blog
post](https://www.msoos.org/2019/06/crystalball-sat-solving-data-gathering-and-machine-learning/)
for more information.

```shell
# prerequisites on a modern Debian/Ubuntu installation
sudo apt-get install build-essential cmake git
sudo apt-get install zlib1g-dev libsqlite3-dev
sudo apt-get install libboost-program-options-dev libboost-serialization-dev
sudo apt-get install python3-pip
sudo pip3 install sklearn pandas numpy lit matplotlib

# build and install Louvain Communities
git clone https://github.com/meelgroup/louvain-community
cd louvain-community
mkdir build && cd build
cmake ..
make -j10
sudo make install
cd ../..

# build and install LightGBM
git clone https://github.com/microsoft/LightGBM
cd LightGBM
mkdir build && cd build
cmake ..
make -j10
sudo make install
cd ../..

# getting the code
git clone https://github.com/msoos/cryptominisat
cd cryptominisat
git checkout crystalball
git submodule update --init
mkdir build && cd build
ln -s ../scripts/crystal/* .
ln -s ../scripts/build_scripts/* .

# Let's get an unsatisfiable CNF
wget https://www.msoos.org/largefiles/goldb-heqc-i10mul.cnf.gz
gunzip goldb-heqc-i10mul.cnf.gz

# Gather the data, denormalize, label,
# create the classifier, generate C++,
# and build the final SAT solver
./ballofcrystal.sh goldb-heqc-i10mul.cnf
[...compilations and the full data pipeline...]

# let's use our newly built tool
./cryptominisat5 goldb-heqc-i10mul.cnf
[ ... ]
s UNSATISFIABLE

# Let's look at the data
cd goldb-heqc-i10mul.cnf-dir
sqlite3 mydata.db
sqlite> select count() from sum_cl_use;
94507
```

## CMake Arguments
The following arguments to cmake configure the generated build artifacts. To
use, specify options prior to running make in a clean subdirectory: `cmake
<options> ..`

- `-DSTATICCOMPILE=<ON/OFF>` -- statically linked library and binary.
- `-DSTATS=<ON/OFF>` -- advanced statistics (slower). Needs [louvain
  communities](https://github.com/meelgroup/louvain-community) installed.
- `-DENABLE_TESTING=<ON/OFF>` -- test suite support
- `-DLARGEMEM=<ON/OFF>` -- more memory available for clauses (but slower on
  most problems)
- `-DIPASIR=<ON/OFF>` -- Build `libipasircryptominisat.so` for
  [IPASIR](https://www.cs.utexas.edu/users/moore/acl2/manuals/current/manual/index-seo.php/IPASIR____IPASIR)
  interface support

## C usage
See src/cryptominisat_c.h.in for details. This is an experimental feature.

## License
Everything that is needed to build by default is MIT licensed. If you
specifically instruct the system it can build with Bliss, which are both GPL.
However, by default CryptoMiniSat will not build with these.
