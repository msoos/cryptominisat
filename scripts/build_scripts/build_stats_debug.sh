#!/bin/bash

set -e

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include tests
rm -f ../tests/cnf-files/*sqlite
cmake -DSTATS=ON -DENABLE_TESTING=ON -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
make test
