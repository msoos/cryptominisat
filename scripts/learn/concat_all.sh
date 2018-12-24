#!/usr/bin/bash
set -e
set -x

rm -f data-sing/*.dat

ls data-sing/*.sqlitedb > files
numthreads=4
numlines=`wc -l files | awk '{print $1}'`
numper=$((numlines/numthreads))
echo "$numper"

numthreadsminone=$((numthreads-1))

untilnow=0
for (( i = 0; i < $numthreads; i++))
do
    if [[ $i -lt $numthreadsminone ]]; then
        toprint=$numper
    else
        echo "Else activated"
        toprint=$((numlines-untilnow))
    fi
    skipplus=$((untilnow+toprint))
    echo "at $i: skipplus $skipplus  -- toprint $toprint"
    head -n $skipplus files | tail -n $toprint > "files${i}"
    untilnow=$((untilnow+toprint))
    echo "And the final file length is: "
    wc -l files${i}
done

# run all threads
for (( i = 0; i < $numthreads; i++))
do
    ./gen_pandas.py `cat files${i}` --confs 7 > "gen_pandas_${i}" &
done

# wait all threads
for job in `jobs -p`
do
echo $job
    wait $job || let "FAIL+=1"
done

# check thread output
for (( i = 0; i < $numthreads; i++))
do
    egrep xzgrep --color -i -e "assert.*fail" -e "signal" -e "error" -e "kill" -e "terminate" "gen_pandas_${i}"
done


if [ "$FAIL" == "0" ];
then
    echo "All went well!"
else
    echo "FAIL of one of the threads! ($FAIL)"
    exit -1
fi

rm final/short-conf-${CONF}.dat
rm final/long-conf-${CONF}.dat

for CONF in {0..6}
do
    ./combine_dats.py -o final/short-conf-${CONF}.dat data-sing/*-short-conf-${CONF}.dat --fixed 2000 > "combine_out_short_${CONF}"
    ./combine_dats.py -o final/long-conf-${CONF}.dat  data-sing/*-long-conf-${CONF}.dat  --fixed 2000 > "combine_out_long_${CONF}"
done

mkdir ../src/predict
rm ../src/predict/*.h
for CONF in {0..6}
do
    ./predict.py "final/short-conf-${CONF}.dat" --name short --basedir "../src/predict/" --final --forest --split 0.02 --clusters 9 --conf "${CONF}" --clustmin 0.03
    ./predict.py "final/short-conf-${CONF}.dat" --name long  --basedir "../src/predict/" --final --forest --split 0.02 --clusters 9 --conf "${CONF}" --clustmin 0.03
done
