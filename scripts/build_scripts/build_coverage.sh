#!/bin/bash

set -e

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include tests cusp* scalmc*
cmake -DENABLE_TESTING:BOOL=ON \
                   -DCOVERAGE:BOOL=ON \
                   -DUSE_GAUSS=ON \
                   -DSTATS:BOOL=ON \
                   -DSLOW_DEBUG:BOOL=ON \
                   -DSTATICCOMPILE:BOOL=ON \
                   ..
make -j$(nproc)
make test
