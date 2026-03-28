#!/bin/bash

set -e

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include tests
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTING=OFF -DSTATICCOMPILE=ON -DNOBREAKID=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
make -j$(nproc)
strip cryptominisat5
