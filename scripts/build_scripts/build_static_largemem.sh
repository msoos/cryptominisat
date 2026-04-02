#!/usr/bin/env bash

set -euo pipefail

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include tests
cmake -DLARGEMEM=ON -DENABLE_TESTING=OFF -DSTATICCOMPILE=ON -DNOBREAKID=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -Dcadical_DIR=../../cadical/build -Dcadiback_DIR=../../cadiback ..
make -j$(nproc)
strip cryptominisat5
