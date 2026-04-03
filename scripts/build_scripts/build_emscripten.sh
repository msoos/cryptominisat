#!/usr/bin/env bash

set -euo pipefail
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
emcmake cmake -DSTATICCOMPILE=ON -DCMAKE_INSTALL_PREFIX="${EMINSTALL}" -DENABLE_TESTING=OFF -DPOLYS=OFF -Dcadical_DIR=../../cadical/build -Dcadiback_DIR=../../cadiback ..
emmake make -j$(nproc)
emmake make install
cp cryptominisat5.wasm ../html
cp "${EMINSTALL}/bin/cryptominisat5.js" ../html
