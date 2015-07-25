#/bin/bash
set -e
rm -f outs/*
for i in $(seq 0 11); do
    echo "reconf with $i"
    ./reconf.py -n 13 -r $i -f outs/out${i}.data  ~/media/sat/out/satcomp091113/reconf*/*stdout* -p > /dev/null
    cp reconf.names outs/out${i}.names
    c5.0 -f outs/out${i} -r > outs/reconf${i}
done
./tocpp.py -n 12 > ../../cryptominisat4/features_to_reconf.cpp

