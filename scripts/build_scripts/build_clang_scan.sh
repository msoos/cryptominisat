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
rm -rf cusp*
rm -rf scalmc*
rm -rf deps
rm -rf _deps
scan-build cmake -DENABLE_TESTING=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -Dcadical_DIR=../../cadical/build -Dcadiback_DIR=../../cadiback ..
scan-build make -j$(nproc) cryptominisat5
