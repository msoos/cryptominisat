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
rm -f ${SAT_DIR}/cryptominisat/tests/cnf-files/*sqlite
cmake -DFINAL_PREDICTOR_BRANCH=ON -DFINAL_PREDICTOR=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -Dcadical_DIR="${SAT_DIR}/cadical/build" -Dcadiback_DIR="${SAT_DIR}/cadiback" ..
make -j$(nproc)
