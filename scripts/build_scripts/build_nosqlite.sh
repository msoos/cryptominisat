#!/bin/bash

set -e

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include tests cusp* scalmc*
cmake -DENABLE_PYTHON_INTERFACE=ON -DENABLE_TESTING=ON -DNOSQLITE=ON ..
make -j$(nproc)
make test
