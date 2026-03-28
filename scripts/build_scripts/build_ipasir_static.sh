#!/bin/bash

set -e

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include tests
cmake -DIPASIR=ON -DENABLE_TESTING=ON -DSTATICCOMPILE=ON ..
make -j$(nproc) VERBOSE=1
strip cryptominisat5
