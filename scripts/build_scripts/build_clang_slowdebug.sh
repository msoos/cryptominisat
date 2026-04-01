#!/bin/bash

set -e

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include
CXX=clang++ cmake -DENABLE_TESTING=ON -DSLOW_DEBUG=ON ..
make -j$(nproc) VERBOSE=1
make test
