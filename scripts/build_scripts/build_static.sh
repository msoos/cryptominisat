#!/usr/bin/env bash

set -euo pipefail

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include tests
cmake -DENABLE_TESTING=OFF -DSTATICCOMPILE=ON -DNOBREAKID=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -Dcadical_DIR=../../cadical/build -Dcadiback_DIR=../../cadiback -DGMP_LIBRARY=/usr/local/lib/libgmp.a -DGMPXX_LIBRARY=/usr/local/lib/libgmpxx.a ..
make -j$(nproc)
strip cryptominisat5
