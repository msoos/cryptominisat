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
rm -rf utils
rm -rf Make*
rm -rf deps
rm -rf _deps
emcmake cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX="${EMINSTALL}" -DENABLE_TESTING=OFF -DPOLYS=OFF -Dcadical_DIR="${SAT_DIR}/cadical/build" -Dcadiback_DIR="${SAT_DIR}/cadiback" ..
emmake make -j$(nproc)
emmake make install
cp cryptominisat5.wasm "${SAT_DIR}/cryptominisat/html"
cp "${EMINSTALL}/bin/cryptominisat5.js" "${SAT_DIR}/cryptominisat/html"
