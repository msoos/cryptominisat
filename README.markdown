[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Linux build](https://travis-ci.org/msoos/cryptominisat.svg?branch=master)](https://travis-ci.org/msoos/cryptominisat)
[![Windows build](https://ci.appveyor.com/api/projects/status/8d000iy63xu7eau5?svg=true)](https://ci.appveyor.com/project/msoos/cryptominisat)
[![Coverity](https://scan.coverity.com/projects/507/badge.svg)](https://scan.coverity.com/projects/507)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/f043efa22ea64e9ba44fde0f3a4fb09f)](https://www.codacy.com/app/soos.mate/cryptominisat?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=msoos/cryptominisat&amp;utm_campaign=Badge_Grade)
[![Docker Hub](https://img.shields.io/badge/docker-latest-blue.svg)](https://hub.docker.com/r/msoos/cryptominisat/)


CryptoMiniSat SAT solver
===========================================

This system provides CryptoMiniSat, an advanced incremental SAT solver. The system has 3
interfaces: command-line, C++ library and python. The command-line interface
takes a [cnf](http://en.wikipedia.org/wiki/Conjunctive_normal_form) as an
input in the [DIMACS](http://www.satcompetition.org/2009/format-benchmarks2009.html)
format with the extension of XOR clauses. The C++ and python interface mimics this and also
allows for incremental use: assumptions and multiple `solve` calls.
A C compatible wrapper is also provided.

When citing, always reference our [SAT 2009 conference paper](https://link.springer.com/chapter/10.1007%2F978-3-642-02777-2_24), bibtex record is [here](http://dblp.uni-trier.de/rec/bibtex/conf/sat/SoosNC09).

License
-----

Please read LICENSE.txt for a discussion. Everything that is needed to build is MIT licensed. The M4RI library (not included) is GPL, so in case you have M4RI installed, you must build with `-DNOM4RI=ON` or `-DMIT=ON` in case you need a pure MIT build.

CrystalBall
-----

Build and use instructions below. Please see the [associated blog post](https://www.msoos.org/2019/06/crystalball-sat-solving-data-gathering-and-machine-learning/) for more information.

```
# prerequisites on a modern Debian/Ubuntu installation
sudo apt-get install build-essential cmake git
sudo apt-get install zlib1g-dev libsqlite3-dev
sudo apt-get install libboost-program-options-dev
sudo apt-get install python3-pip
sudo pip3 install sklearn pandas numpy lit matplotlib

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

# Gather the data, denormalize, label, output CSV,
# create the classifier, generate C++,
# and build the final SAT solver
./ballofcrystal.sh --csv goldb-heqc-i10mul.cnf
[...compilations and the full data pipeline...]

# let's use our newly built tool
# we are using configuration number short:3 long:3
./cryptominisat5 --predshort 3 --predlong 3 goldb-heqc-i10mul.cnf
[ ... ]
s UNSATISFIABLE

# Let's look at the data
cd goldb-heqc-i10mul.cnf-dir
sqlite3 mydata.db
sqlite> select count() from sum_cl_use;
94507
```

The CNFs go through the following set of transformations to become the generated code:

1. `./cryptominisat` dumps the data. Options: `--cldatadumpratio 0.08`, `--gluecut0 100`
2. `./drat-trim`
3. `./add_lemma_ind.py`
4. `./clean_update_data.py`
5. `./rem_data.py` Options: `--fair`, etc.
6. `./vardata_gen_pandas.py`. Options: `--limit`
7. `./gen_pandas.py` Options: `--limit`, `--confs`
8. `./concat_pandas.py`
9. `./predict.py` Options: `--forest/--tree/etc`, `--depth/--split/etc`


Docker usage
-----

To run on file `myfile.cnf`:

```
cat myfile.cnf | docker run --rm -i msoos/cryptominisat
```

To run on a hand-written CNF:

```
docker pull msoos/cryptominisat
echo "1 2 0" | docker run --rm -i msoos/cryptominisat
```

To run on the file `/home/myfolder/myfile.cnf.gz` by mounting it (may be faster):

```
docker run --rm -v /home/myfolder/myfile.cnf.gz:/f msoos/cryptominisat f
```

To build and run locally:

```
git clone https://github.com/msoos/cryptominisat.git
cd cryptominisat
git submodule update --init
docker build -t cms .
cat myfile.cnf | docker run --rm -i cms
```

To build and run the web interface:

```
git clone https://github.com/msoos/cryptominisat.git
cd cryptominisat
git submodule update --init
docker build -t cmsweb -f Dockerfile.web .
docker run --rm -i -p 80:80 cmsweb
```


Compiling in Linux
-----

To build and install, issue:

```
sudo apt-get install build-essential cmake
# not required but very useful
sudo apt-get install zlib1g-dev libboost-program-options-dev libm4ri-dev libsqlite3-dev help2man
tar xzvf cryptominisat-version.tar.gz
cd cryptominisat-version
mkdir build && cd build
cmake ..
make
sudo make install
sudo ldconfig
```

Compiling in Mac OSX
-----

First, you must get Homebew from https://brew.sh/ then:

```
brew install cmake boost zlib
tar xzvf cryptominisat-version.tar.gz
cd cryptominisat-version
mkdir build && cd build
cmake ..
make
sudo make install
```

Compiling in Windows
-----

You will need python installed, then for Visual Studio 2015:

```
C:\> [ download cryptominisat-version.zip ]
C:\> unzip cryptominisat-version.zip
C:\> rename cryptominisat-version cms
C:\> cd cms
C:\cms> mkdir build
C:\cms> cd build

C:\cms\build> [ download http://sourceforge.net/projects/boost/files/boost/1.59.0/boost_1_59_0.zip ]
C:\cms\build> unzip boost_1_59_0.zip
C:\cms\build> mkdir boost_1_59_0_install
C:\cms\build> cd boost_1_59_0
C:\cms\build\boost_1_59_0> bootstrap.bat --with-libraries=program_options
C:\cms\build\boost_1_59_0> b2 --with-program_options address-model=64 toolset=msvc-14.0 variant=release link=static threading=multi runtime-link=static install --prefix="C:\cms\build\boost_1_59_0_install" > boost_install.out
C:\cms\build\boost_1_59_0> cd ..

C:\cms\build> git clone https://github.com/madler/zlib
C:\cms\build> cd zlib
C:\cms\build\zlib> git checkout v1.2.8
C:\cms\build\zlib> mkdir build
C:\cms\build\zlib> mkdir myinstall
C:\cms\build\zlib> cd build
C:\cms\build\zlib\build> cmake -G "Visual Studio 14 2015 Win64" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=C:\cms\build\zlib\myinstall ..
C:\cms\build\zlib\build> msbuild /t:Build /p:Configuration=Release /p:Platform="x64" zlib.sln
C:\cms\build\zlib\build> msbuild INSTALL.vcxproj
C:\cms\build> cd ..\..

C:\cms\build> cmake -G "Visual Studio 14 2015 Win64" -DCMAKE_BUILD_TYPE=Release -DSTATICCOMPILE=ON -DZLIB_ROOT=C:\cms\build\zlib\myinstall -DBOOST_ROOT=C:\cms\build\boost_1_59_0_install ..
C:\cms\build> cmake --build --config Release .
```

You now have the static binary under `C:\cms\build\Release\cryptominisat5.exe`

Command-line usage
-----

Let's take the file:
```
p cnf 3 3
1 0
-2 0
-1 2 3 0
```

The file has 3 variables and 3 clauses, this is reflected in the header
`p cnf 3 3` which gives the number of variables as the first number and the number of clauses as the second.
Every clause is ended by '0'. The clauses say: 1 must be True, 2
must be False, and either 1 has to be False, 2 has to be True or 3 has to be
True. The only solution to this problem is:
```
cryptominisat5 --verb 0 file.cnf
s SATISFIABLE
v 1 -2 3 0
```

Which means, that setting variable 1 True, variable 2 False and variable 3 True satisfies the set of constraints (clauses) in the CNF. If the file had contained:
```
p cnf 3 4
1 0
-2 0
-3 0
-1 2 3 0
```

Then there is no solution and the solver returns `s UNSATISFIABLE`.

Incremental Python Usage
-----
The python module works with both Python 2 and Python 3. It must be compiled as per (notice "python-dev"):

```
sudo apt-get install build-essential cmake
sudo apt-get install zlib1g-dev libboost-program-options-dev libm4ri-dev libsqlite3-dev help2man
sudo apt-get install python3-setuptools python3-dev
tar xzvf cryptominisat-version.tar.gz
cd cryptominisat-version
mkdir build && cd build
cmake ..
make
sudo make install
sudo ldconfig

```

You can then use it in incremental mode as:

```
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

For more detailed usage instructions, please see the README.rst under the `python`
directory.

Incremental Library Usage
-----
The library uses a variable numbering scheme that starts from 0. Since 0 cannot
be negated, the class `Lit` is used as: `Lit(variable_number, is_negated)`. As
such, the 1st CNF above would become:

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

Rust bindings
-----

To build the Rust bindings:

```
git clone https://github.com/msoos/cryptominisat-rs/
cd cryptominisat-rs
cargo build --release
cargo test
```

You can use it as per the (README)[https://github.com/msoos/cryptominisat-rs/blob/master/README.markdown] in that repository. To include CryptoMiniSat in your Rust project, add the dependency to your `Cargo.toml` file:

```
cryptominisat = { git = "https://github.com/msoos/cryptominisat-rs", branch= "master" }
```

You can see an example project using CryptoMiniSat in Rust (here)[https://github.com/msoos/caqe/].


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
For building with Gaussian Elimination, you need to build as per:

```
sudo apt-get install build-essential cmake
sudo apt-get install zlib1g-dev libboost-program-options-dev libm4ri-dev libsqlite3-dev help2man
tar xzvf cryptominisat-version.tar.gz
cd cryptominisat-version
mkdir build && cd build
cmake -DUSE_GAUSS=ON ..
make
sudo make install
```

To use Gaussian elimination, provide a CNF with xors in it (either in CNF or XOR+CNF form) and tune the gaussian parameters. Use `--hhelp` to find all the gaussian elimination options:

```
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

If any of these options seem to be non-existent, then either you forgot to compile the SAT solver with the above options, or you forgot to re-install it with `sudo make install`.

Testing
-----
For testing you will need the GIT checkout and build as per:

```
sudo apt-get install build-essential cmake git
sudo apt-get install zlib1g-dev libboost-program-options-dev libm4ri-dev libsqlite3-dev help2man
sudo apt-get install git python3-pip python3-setuptools python3-dev
sudo pip3 install --upgrade pip
sudo pip3 install lit
git clone https://github.com/msoos/cryptominisat.git
cd cryptominisat
git submodule update --init
mkdir build && cd build
cmake -DENABLE_TESTING=ON ..
make -j4
make test
sudo make install
sudo ldconfig
```

Fuzzing
-----
Build for test as per above, then:

```
cd ../cryptominisat/scripts/fuzz/
./fuzz_test.py
```

Configuring a build for a minimal binary&library
-----
The following configures the system to build a bare minimal binary&library. It needs a compiler, but nothing much else:

```
cmake -DONLY_SIMPLE=ON -DNOZLIB=ON -DNOM4RI=ON -DSTATS=OFF -DNOVALGRIND=ON -DENABLE_TESTING=OFF .
```

CMake Arguments
-----
The following arguments to cmake configure the generated build artifacts. To use, specify options prior to running make in a clean subdirectory: `cmake <options> ..`

- `-DSTATICCOMPILE=<ON/OFF>` -- statically linked library and binary
- `-DUSE_GAUSS=<ON/OFF>` -- Gauss-Jordan Elimination support
- `-DSTATS=<ON/OFF>` -- advanced statistics (slower)
- `-DENABLE_TESTING=<ON/OFF>` -- test suite support
- `-DMIT=<ON/OFF>` -- MIT licensed components only
- `-DNOM4RI=<ON/OFF>` -- without toplevel Gauss-Jordan Elimination support
- `-DREQUIRE_M4RI=<ON/OFF>` -- abort if M4RI is not present
- `-DNOZLIB=<ON/OFF>` -- no gzip DIMACS input support
- `-DONLY_SIMPLE=<ON/OFF>` -- only the simple binary is built
- `-DNOVALGRIND=<ON/OFF>` -- no extended valgrind memory checking support
- `-DLARGEMEM=<ON/OFF>` -- more memory available for clauses (but slower on most problems)


Trying different configurations
-----
Try solving using different reconfiguration values 3,4,6,7,12,13,14,15,16 as per:

```
./cryptominisat5 --reconfat 0 --reconf 3 my_hard_problem.cnf
./cryptominisat5 --reconfat 0 --reconf 4 my_hard_problem.cnf
...
./cryptominisat5 --reconfat 0 --reconf 16 my_hard_problem.cnf
```

These configurations are designed to be relatively orthogonal. Check if any of them solve a lot faster. If it does, try using that for similar problems going forward. Please do come back to the author with what you have found to work best for you.

Getting learnt clauses
-----
As an experimental feature, you can get the learnt clauses from the system with the following code, where `lits` is filled with learnt clauses every time `get_next_small_clause` is called. The example below will eventually return all clauses of size 4 or less. You can call `end_getting_small_clauses` at any time.

```
SATSolver s;
//fill the solver, run solve, etc.

//Get all clauses of size 4 or less

s->start_getting_small_clauses(4);

vector<Lit> lits;
bool ret = true;
while (ret) {
    bool ret = s->get_next_small_clause(lits);
    if (ret) {
        //deal with clause in "lits"
        add_to_my_db(lits);
    }
}
s->end_getting_small_clauses();
```

C usage
-----
See src/cryptominisat_c.h.in for details. This is an experimental feature.
