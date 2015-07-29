#!/bin/bash

set -e

NUM=14
NUM_MIN_ONE=`expr $NUM - 1`

rm -f outs/*
./reconf.py -i 5,0,8,11,9,10 -n $NUM -f outs/out  ~/media/sat/out/satcomp091113/reconf*/*stdout* > output
for i in $(seq 0 $NUM_MIN_ONE); do
    echo "reconf with $i"
    cp reconf.names outs/out${i}.names
    c5.0 -u 20 -f outs/out${i} -r > outs/out${i}.c50.out
done
./tocpp.py -i 5,0,8,11,9,10 -n $NUM > ../../cryptominisat4/features_to_reconf.cpp
read -r -p "Upload to AWS? [y/N] " response
case $response in
    [yY][eE][sS]|[yY])
        aws s3 cp ../../cryptominisat4/features_to_reconf.cpp s3://msoos-solve-data/solvers/
        echo "Uploded to AWS"
        ;;
    *)
        echo "Not uploaded to AWS"
        ;;
esac
