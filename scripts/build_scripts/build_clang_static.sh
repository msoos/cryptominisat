#!/usr/bin/env bash

set -euo pipefail

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include tests
CXX=clang++ cmake -DENABLE_TESTING=ON -DSTATICCOMPILE=ON -Dcadical_DIR=../../cadical/build -Dcadiback_DIR=../../cadiback -DGMP_LIBRARY=/usr/local/lib/libgmp.a -DGMPXX_LIBRARY=/usr/local/lib/libgmpxx.a ..
make -j$(nproc)
make test
