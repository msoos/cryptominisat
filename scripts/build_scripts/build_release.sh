#!/bin/bash

set -e

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include tests cusp* scalmc*
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_PYTHON_INTERFACE=ON -DENABLE_TESTING=ON ..
make -j$(nproc)
make test
