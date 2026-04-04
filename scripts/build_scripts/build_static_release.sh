#!/usr/bin/env bash

set -euo pipefail

SAT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
echo "solvers dir: $SAT_DIR"

rm -rf cm*
rm -rf CM*
rm -rf lib*
rm -rf cryptomini*
rm -rf Testing*
rm -rf tests*
rm -rf pycryptosat
rm -rf include
rm -rf tests
rm -rf deps
rm -rf _deps
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTING=OFF -DBUILD_SHARED_LIBS=OFF -DNOBREAKID=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -Dcadical_DIR="${SAT_DIR}/cadical/build" -Dcadiback_DIR="${SAT_DIR}/cadiback/build" -DGMP_LIBRARY=/usr/local/lib/libgmp.a -DGMPXX_LIBRARY=/usr/local/lib/libgmpxx.a ..
make -j$(nproc)
strip cryptominisat5
