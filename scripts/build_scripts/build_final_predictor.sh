#!/usr/bin/env bash

# FINAL_PREDICTOR=ON build: the solver uses the crystalball xgboost models.
# Needs xgboost/c_api.h and libxgboost.so (the pip package's lib works).
# Defaults to an xgboost checkout next to sat_solvers/; override with XGBOOST_INCLUDE_DIR=... XGBOOST_LIBRARY=...
# src/predict/predictor_disc.json is embedded as the default.

set -euo pipefail

SAT_DIR="$(cd "$(dirname "$(readlink -f "$0")")/../../.." && pwd)"
echo "solvers dir: $SAT_DIR"

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include cusp* scalmc* utils Make* deps _deps

XGBOOST_DIR="$(cd "$SAT_DIR/.." && pwd)/xgboost"
XGBOOST_INCLUDE_DIR="${XGBOOST_INCLUDE_DIR:-$XGBOOST_DIR/include}"
XGBOOST_LIBRARY="${XGBOOST_LIBRARY:-$XGBOOST_DIR/lib/libxgboost.so}"

cmake -DFINAL_PREDICTOR=ON -DENABLE_TESTING=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -Dcadical_DIR="${SAT_DIR}/cadical/build" -Dcadiback_DIR="${SAT_DIR}/cadiback/build" \
    -DXGBOOST_INCLUDE_DIR="$XGBOOST_INCLUDE_DIR" -DXGBOOST_LIBRARY="$XGBOOST_LIBRARY" ..
make -j$(nproc)
