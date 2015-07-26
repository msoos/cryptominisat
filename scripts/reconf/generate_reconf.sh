#/bin/bash
set -e
rm -f outs/*
./reconf.py -n 13 -f outs/out  ~/media/sat/out/satcomp091113/reconf*/*stdout* > /dev/null
for i in $(seq 0 12); do
    echo "reconf with $i"
    cp reconf.names outs/out${i}.names
    c5.0 -f outs/out${i} -r > outs/reconf${i}
done
./tocpp.py -n 12 > ../../cryptominisat4/features_to_reconf.cpp

