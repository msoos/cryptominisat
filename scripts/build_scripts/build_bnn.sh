#!/bin/bash

set -e

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include tests cusp* scalmc* utils Make*
cmake -DENABLE_BNN=ON -DNOBREAKID=ON -DENABLE_TESTING=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
make -j$(nproc)
ctest --rerun-failed --output-on-failure
