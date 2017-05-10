#!/bin/bash
set -e

cd /home/ubuntu/
rm -rf m4ri-20140914*
aws s3 cp s3://msoos-solve-data/solvers/m4ri-20140914.tar.gz . --region us-west-2
tar xzvf m4ri-20140914.tar.gz
cd m4ri-20140914/
./configure
make "-j$2"
sudo make install
echo "built and installed M4RI"

cd /home/ubuntu/cryptominisat
rm -rf build
mkdir -p build
cd build
rm -rf C* c*
cmake ${3} ..
make "-j${2}" VERBOSE=1
echo "built CMS"

# solver is now in cryptominisat/build/cryptominisat5

exit 0
