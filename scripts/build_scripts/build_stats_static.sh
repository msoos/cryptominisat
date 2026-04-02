#!/usr/bin/env bash

set -euo pipefail

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include tests utils
rm -f ../tests/cnf-files/*sqlite
cmake -DSTATS=ON -DENABLE_TESTING=ON -DSTATICCOMPILE=ON -Dcadical_DIR=../../cadical/build -Dcadiback_DIR=../../cadiback ..
make -j$(nproc)
strip cryptominisat5
make test
