#!/usr/bin/env bash

set -euo pipefail

rm -rf .cmake
rm -rf lib*
rm -f *.a *.so*
rm -rf Test*
rm -rf tests*
rm -rf include
rm -rf tests
rm -rf approxmc*
rm -rf apx-src
rm -rf CM*
rm -rf cmake*
rm -rf deps
rm -rf _deps
cmake -DENABLE_TESTING=ON -DNOCADICAL=ON ..
make -j$(nproc)
make test
