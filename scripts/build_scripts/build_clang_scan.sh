#!/bin/bash

set -e

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include tests cusp* scalmc*
scan-build cmake -DENABLE_PYTHON_INTERFACE=ON -DENABLE_TESTING=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
scan-build make -j$(nproc) cryptominisat5
