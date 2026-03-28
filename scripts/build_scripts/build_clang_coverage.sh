#!/bin/bash

set -e

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include tests cusp* scalmc*
CXX=clang++ cmake -DENABLE_TESTING=ON -DSTATICCOMPILE=ON -DCOVERAGE=ON ..
make -j$(nproc) VERBOSE=1
make test
