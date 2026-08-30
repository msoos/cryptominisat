#!/bin/bash
# Build a sanitized solver and fuzz it. Run from an empty build dir.
# Usage: ./fuzz_test_sanitize.sh [num_rounds]

set -e

ROUNDS="${1:-30}"

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include
CC="clang" CXX="clang++" cmake -DENABLE_TESTING=OFF -DSLOW_DEBUG=ON -DSANITIZE=ON ..
make -j4 cryptominisat5-bin
SOLVER="$(pwd)/cryptominisat5"
cd ../scripts/fuzz
./fuzz.py --exec "$SOLVER" --fuzzlim "$ROUNDS"
