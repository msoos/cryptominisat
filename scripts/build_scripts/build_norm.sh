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
SAT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
echo "solvers dir: $SAT_DIR"
cmake -DENABLE_TESTING=ON \
    -Dcadical_DIR="${SAT_DIR}/cadical/build" \
    -Dcadiback_DIR="${SAT_DIR}/cadiback/build" \
    ..
make -j$(nproc)
make test
