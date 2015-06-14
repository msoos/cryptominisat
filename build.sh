#!/bin/bash

set -e

cd m4ri-20140914
./configure
make
cd ..

BASE=`pwd`
M4RI="${M4RI}m4ri-20140914"


mkdir -p build
cd build
rm -rf cm* CM* cmsat4-src Make*
cmake -DM4RI_ROOT_DIR=$M4RI ..
make VERBOSE=1
cp cryptominisat ../solver
ldd solver
./solver --version
