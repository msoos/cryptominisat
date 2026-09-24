#!/usr/bin/env bash

# STATS=ON build for crystalball data gathering (scripts/crystal/).
# Needs sqlite3. Point cmake at a non-system copy with:
#   SQLITE3_INCLUDE_DIR=... SQLITE3_LIBRARY=... ./build_stats.sh

set -euo pipefail

SAT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
echo "solvers dir: $SAT_DIR"

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include cusp* scalmc* utils Make* deps _deps
rm -f ${SAT_DIR}/cryptominisat/tests/cnf-files/*sqlite

EXTRA=""
[[ -n "${SQLITE3_INCLUDE_DIR:-}" ]] && EXTRA="$EXTRA -DSQLITE3_INCLUDE_DIR=$SQLITE3_INCLUDE_DIR"
[[ -n "${SQLITE3_LIBRARY:-}" ]] && EXTRA="$EXTRA -DSQLITE3_LIBRARY=$SQLITE3_LIBRARY"

cmake -DSTATS=ON -DENABLE_TESTING=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -Dcadical_DIR="${SAT_DIR}/cadical/build" -Dcadiback_DIR="${SAT_DIR}/cadiback/build" $EXTRA ..
make -j4
