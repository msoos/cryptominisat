#/bin/bash
set -e
for i in $(seq 0 8); do
    echo "reconf with $i"
    ./reconf.py -r $i -f outs/out${i}.data  ~/media/sat/out/reconf*/*stdout* -p > /dev/null
    cp reconf.names outs/out${i}.names
    c5.0 -f outs/out${i} > outs/reconf${i}
done
