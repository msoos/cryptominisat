#!/bin/bash

set -e

rm -rf cm* CM* lib* cryptomini* Testing* tests* py-lib include
cmake -DENABLE_TESTING=ON ..
make -j4 VERBOSE=1
make test
cd ../tests/simp-checks/
./checks.py -l ../../build/cryptominisat4 testfiles/*
cd ../../build/
