#!/bin/bash

set -e

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include tests utils
rm -f ../tests/cnf-files/*sqlite
cmake -DSTATS=ON -DENABLE_TESTING=ON -DSTATICCOMPILE=ON ..
make -j$(nproc)
strip cryptominisat5
make test
