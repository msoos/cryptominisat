#!/usr/bin/env bash

set -euo pipefail

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include tests cusp* scalmc*
CXX=clang++ cmake -DENABLE_TESTING=ON -DSTATICCOMPILE=ON -DCOVERAGE=ON -Dcadical_DIR=../../cadical/build -Dcadiback_DIR=../../cadiback ..
make -j$(nproc) VERBOSE=1
make test
