#!/usr/bin/bash
set -e
set -x

for CONF in {0..6}
do
    ./combine_dats.py -o final/short-conf-${CONF}.dat data-sing/*-short-conf-${CONF}.dat
    ./combine_dats.py -o final/long-conf-${CONF}.dat  data-sing/*-long-conf-${CONF}.dat
done

mkdir ../src/predict
rm ../src/predict/*.h
for CONF in {0..6}
do
    ./predict.py "final/short-conf-${CONF}.dat" --name short --basedir "../src/predict/" --final --tree --split 0.1 --clusters 9 --conf "${CONF}" --clustmin 0.03
    ./predict.py "final/short-conf-${CONF}.dat" --name long  --basedir "../src/predict/" --final --tree --split 0.1 --clusters 9 --conf "${CONF}" --clustmin 0.03
done
