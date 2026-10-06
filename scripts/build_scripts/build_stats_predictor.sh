#!/usr/bin/env bash
# STATS=ON build for crystalball data gathering (scripts/crystal/).
# Needs sqlite3. Point cmake at a non-system copy with:
#   SQLITE3_INCLUDE_DIR=... SQLITE3_LIBRARY=... ./build_stats.sh

# STATS and FINAL_PREDICTOR together: gathers clause data while a model
# drives the reduce (the second gathering round). From an empty build dir.

set -euo pipefail

SAT_DIR="$(cd "$(dirname "$(readlink -f "$0")")/../../.." && pwd)"
echo "solvers dir: $SAT_DIR"

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include cusp* scalmc* utils Make* deps _deps

XGBOOST_DIR="$(cd "$SAT_DIR/.." && pwd)/xgboost"
XGBOOST_INCLUDE_DIR="${XGBOOST_INCLUDE_DIR:-$XGBOOST_DIR/include}"
XGBOOST_LIBRARY="${XGBOOST_LIBRARY:-$XGBOOST_DIR/lib/libxgboost.so}"
EXTRA=""
[[ -n "${SQLITE3_INCLUDE_DIR:-}" ]] && EXTRA="$EXTRA -DSQLITE3_INCLUDE_DIR=$SQLITE3_INCLUDE_DIR"
[[ -n "${SQLITE3_LIBRARY:-}" ]] && EXTRA="$EXTRA -DSQLITE3_LIBRARY=$SQLITE3_LIBRARY"

cmake -DSTATS=ON -DFINAL_PREDICTOR=ON -DENABLE_TESTING=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -Dcadical_DIR="${SAT_DIR}/cadical/build" -Dcadiback_DIR="${SAT_DIR}/cadiback/build" \
    -DXGBOOST_INCLUDE_DIR="$XGBOOST_INCLUDE_DIR" -DXGBOOST_LIBRARY="$XGBOOST_LIBRARY" $EXTRA ..
make -j4
