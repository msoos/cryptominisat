#!/bin/bash

set -e

rm -f outs/*
./reconf.py -n 13 -f outs/out  ~/media/sat/out/satcomp091113/reconf*/*stdout* > output
for i in $(seq 0 12); do
    echo "reconf with $i"
    cp reconf.names outs/out${i}.names
    c5.0 -b -f outs/out${i} -r > outs/out${i}.c50.out
done
./tocpp.py -n 13 > ../../cryptominisat4/features_to_reconf.cpp
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
