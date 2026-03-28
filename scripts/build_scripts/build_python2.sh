#!/bin/bash

set -e

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include tests cusp* scalmc*
cmake -DFORCE_PYTHON2=ON -DENABLE_PYTHON_INTERFACE=ON -DENABLE_TESTING=ON ..
make -j$(nproc)
make test
