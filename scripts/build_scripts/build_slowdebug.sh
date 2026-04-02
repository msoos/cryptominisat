#!/usr/bin/env bash

set -euo pipefail

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTING=ON -DSLOW_DEBUG=ON -Dcadical_DIR=../../cadical/build -Dcadiback_DIR=../../cadiback ..
make -j$(nproc) VERBOSE=1
make test
