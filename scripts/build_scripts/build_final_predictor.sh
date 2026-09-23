#!/usr/bin/env bash

# FINAL_PREDICTOR=ON build: the solver uses the crystalball xgboost models.
# Needs xgboost/c_api.h and libxgboost.so (the pip package's lib works),
# Python3 + NumPy dev for --predtype py. Point cmake at them with:
#   XGBOOST_INCLUDE_DIR=... XGBOOST_LIBRARY=... ./build_final_predictor.sh
# src/predict/predictor_{short,long,forever}.json are embedded as defaults.

set -euo pipefail

SAT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
echo "solvers dir: $SAT_DIR"

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include cusp* scalmc* utils Make* deps _deps

EXTRA=""
[[ -n "${XGBOOST_INCLUDE_DIR:-}" ]] && EXTRA="$EXTRA -DXGBOOST_INCLUDE_DIR=$XGBOOST_INCLUDE_DIR"
[[ -n "${XGBOOST_LIBRARY:-}" ]] && EXTRA="$EXTRA -DXGBOOST_LIBRARY=$XGBOOST_LIBRARY"

cmake -DFINAL_PREDICTOR=ON -DENABLE_TESTING=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -Dcadical_DIR="${SAT_DIR}/cadical/build" -Dcadiback_DIR="${SAT_DIR}/cadiback/build" $EXTRA ..
make -j4
