#!/bin/bash

set -e

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include
rm -rf tests
cmake -DSTATS=ON -DENABLE_TESTING=ON ..
make -j4
make test
cd ../tests/simp-checks/
./checks.py -l ../../build/cryptominisat4 testfiles/*
cd ../../build/
