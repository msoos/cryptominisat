#!/usr/bin/env bash

set -euo pipefail

rm -rf cm*
rm -rf CM*
rm -rf lib*
rm -rf cryptomini*
rm -rf Testing*
rm -rf tests*
rm -rf pycryptosat
rm -rf include
rm -rf tests
rm -rf deps
rm -rf _deps
CC=afl-gcc CXX=afl-g++ cmake -DLIMITMEM=ON -DENABLE_TESTING=OFF -DNOZLIB=ON -Dcadical_DIR=../../cadical/build -Dcadiback_DIR=../../cadiback ..
make -j$(nproc)
mkdir -p afl/testcase_dir/
mkdir -p afl/findings_dir/
cat <<EOF > afl/testcase_dir/input.cnf
p cnf 13 24
4 5 0
-2 1 0
x 2 3 -4 0
c stuff
c ind 2 3 4 0
c Solver::new_var()
c Solver::new_vars( 20 )
c Solver::solve( 3 4 )
c Solver::simplify( 1 -3 )
c Solver::solve( 1 -3 )
2 3 0
EOF

export AFL_SKIP_CPUFREQ="1"
# afl-fuzz -M master -i afl/testcase_dir/ -o afl/findings_dir/ ./cryptominisat5 --presimp 1 --debuglib 1
