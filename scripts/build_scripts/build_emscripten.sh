#!/usr/bin/env bash
# Builds cryptominisat for wasm in its own build-wasm/ (the native build/ stays) and
# installs it into $EMINSTALL. Run from anywhere, after source ~/emscripten.sh.
set -euo pipefail
REPO="$(cd "$(dirname "$(readlink -f "$0")")/../.." && pwd)"
SAT_DIR="$(cd "$REPO/.." && pwd)"
mkdir -p "$REPO/build-wasm"
cd "$REPO/build-wasm"
emcmake cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_INSTALL_PREFIX="${EMINSTALL}" -DENABLE_TESTING=OFF -DPOLYS=OFF \
    -Dcadical_DIR="${SAT_DIR}/cadical/build-wasm" \
    -Dcadiback_DIR="${SAT_DIR}/cadiback/build-wasm" \
    "$REPO"
emmake make -j"$(nproc)"
emmake make install
cp cryptominisat5.wasm "$REPO/html"
cp "${EMINSTALL}/bin/cryptominisat5.js" "$REPO/html"
