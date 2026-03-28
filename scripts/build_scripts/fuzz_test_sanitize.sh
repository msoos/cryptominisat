#!/bin/bash

set -e

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include
CC="clang" CXX="clang++" cmake -DENABLE_TESTING=ON -DSLOW_DEBUG=ON -DSANITIZE=ON ..
make -j$(nproc) VERBOSE=1
cd ../scripts/fuzz
./fuzz_test.py --small
