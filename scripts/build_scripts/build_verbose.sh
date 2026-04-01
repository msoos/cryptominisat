#!/bin/bash

set -e

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include tests cusp* scalmc* utils Make*
cmake -DNOBREAKID=ON -DENABLE_TESTING=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
make -j1 VERBOSE=1
ctest -j1 --verbose
