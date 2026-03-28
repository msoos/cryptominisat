#!/bin/bash

set -e

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include tests
CXX=clang++ cmake -DENABLE_PYTHON_INTERFACE=ON -DENABLE_TESTING=ON ..
make -j$(nproc)
make test
