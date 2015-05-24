#!/bin/bash
set -e

cd /home/ubuntu/cryptominisat
echo "cd-d into cmsat"

git checkout master
echo "pulled"

git pull
echo "pulled"

git checkout $1
echo "got revision $1"

cd build
echo "cd-d into build"

rm -rf C* c*
echo "deleted build"

cmake ..
echo "cmake-d"

make -j$2
echo "built"

exit 0
