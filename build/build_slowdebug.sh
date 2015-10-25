#!/bin/bash

set -e

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include
cmake -DENABLE_TESTING=ON -DSLOW_DEBUG=ON ..
make -j4 VERBOSE=1
make test
