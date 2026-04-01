#!/bin/bash

set -e

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include tests cusp* scalmc* utils Make*
rm -f ../tests/cnf-files/*sqlite
cmake -DFINAL_PREDICTOR=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_TESTING=ON ..
make -j$(nproc)
