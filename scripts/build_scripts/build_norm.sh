#!/usr/bin/env bash

set -euo pipefail

rm -rf .cmake
rm -rf lib*
rm -rf Test*
rm -rf tests*
rm -rf include
rm -rf tests
rm -rf approxmc*
rm -rf apx-src
rm -rf CM*
rm -rf cmake*
rm -rf deps
rm -rf _deps
SOLVERS_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
echo "solvers dir: $SOLVERS_DIR"
cmake -DENABLE_TESTING=ON \
    -Dcadical_DIR="${SOLVERS_DIR}/cadical/build" \
    -Dcadiback_DIR="${SOLVERS_DIR}/cadiback/build" \
    -Dcryptominisat5_DIR="${SOLVERS_DIR}/cryptominisat/build" \
    -Dsbva_DIR="${SOLVERS_DIR}/sbva/build" \
    -Dtreedecomp_DIR="${SOLVERS_DIR}/treedecomp/build" \
    -Darjun_DIR="${SOLVERS_DIR}/arjun/build" \
    ..
make -j$(nproc)
make test
