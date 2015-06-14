#!/bin/bash

set -e

mkdir -p build
cd build
rm -rf cm* CM* cmsat4-src Make*
cmake ..
make
cp cryptominisat ../solver
