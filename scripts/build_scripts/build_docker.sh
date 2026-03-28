#!/bin/bash

set -e

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include tests
rm -f ../tests/cnf-files/*sqlite
cmake -DNOMYSQL=ON -DSTATS=ON -DENABLE_TESTING=ON ..
make -j$(nproc)
make test
