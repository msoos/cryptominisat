#!/usr/bin/env bash

set -euo pipefail

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include tests
cmake -DENABLE_PYTHON_INTERFACE=OFF -DSTATICCOMPILE=ON -Dcadical_DIR=../../cadical/build -Dcadiback_DIR=../../cadiback ..
make -j$(nproc) VERBOSE=1
strip cryptominisat5
