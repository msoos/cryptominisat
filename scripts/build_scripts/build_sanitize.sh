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
rm -rf cusp*
rm -rf scalmc*
rm -rf utils
rm -rf Make*
rm -rf deps
rm -rf _deps
CXX=clang++ cmake -DENABLE_TESTING=ON -DSANITIZE=ON -Dcadical_DIR="${SAT_DIR}/cadical/build" -Dcadiback_DIR="${SAT_DIR}/cadiback" ..
make -j$(nproc)
make test
