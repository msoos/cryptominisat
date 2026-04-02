#!/usr/bin/env bash

set -euo pipefail

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include tests cusp* scalmc* utils Make*
CXX=clang++ cmake -DENABLE_PYTHON_INTERFACE=ON -DENABLE_TESTING=ON -DSANITIZE=ON -Dcadical_DIR=../../cadical/build -Dcadiback_DIR=../../cadiback ..
make -j$(nproc)
make test
