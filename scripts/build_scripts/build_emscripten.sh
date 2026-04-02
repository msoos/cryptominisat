#!/usr/bin/env bash

set -euo pipefail
rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include tests utils Make*
emcmake cmake -DSTATICCOMPILE=ON -DCMAKE_INSTALL_PREFIX="${EMINSTALL}" -DENABLE_TESTING=OFF -DPOLYS=OFF -Dcadical_DIR=../../cadical/build -Dcadiback_DIR=../../cadiback ..
emmake make -j$(nproc)
emmake make install
cp cryptominisat5.wasm ../html
cp "${EMINSTALL}/bin/cryptominisat5.js" ../html
