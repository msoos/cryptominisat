#!/bin/bash
cd /home/ubuntu/cryptominisat
rc=$?
if [[ $rc != 0 ]] ; then
    exit $rc
fi
echo "cd-d into cmsat"

git pull
rc=$?
if [[ $rc != 0 ]] ; then
    exit $rc
fi
echo "pulled"

cd build
rc=$?
if [[ $rc != 0 ]] ; then
    exit $rc
fi
echo "cd-d into build"

rm -rf C* c*
rc=$?
if [[ $rc != 0 ]] ; then
    exit $rc
fi
echo "deleted build"

cmake ..
rc=$?
if [[ $rc != 0 ]] ; then
    exit $rc
fi
echo "cmake-d"

make -j8
rc=$?
if [[ $rc != 0 ]] ; then
    exit $rc
fi
echo "built"

exit 0
