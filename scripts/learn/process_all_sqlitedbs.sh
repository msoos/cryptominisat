#!/usr/bin/bash
set -e
set -x

numthreads=4
numconfs=7
location=data-sing

rm -f ${location}/*.dat
ls ${location}/*.sqlitedb > files
numlines=$(wc -l files | awk '{print $1}')
numper=$((numlines/numthreads))
echo "numper: $numper"
shuf files > files_shuf

numthreadsminone=$((numthreads-1))

untilnow=0
for (( i = 0; i < numthreads; i++))
do
    if [[ $i -lt $numthreadsminone ]]; then
        toprint=$numper
        echo "toprint: ${toprint}"
    else
        toprint=$((numlines-untilnow))
        echo "Else activated, toprint: ${toprint}"
    fi
    skipplus=$((untilnow+toprint))
    echo "at $i: skipplus $skipplus  -- toprint $toprint"
    head -n $skipplus files_shuf | tail -n $toprint > "files${i}"
    untilnow=$((untilnow+toprint))
    echo "And the final file length is: "
    wc -l files${i}
done

# run all threads
for (( i = 0; i < numthreads; i++))
do
    ./gen_pandas.py $(cat "files${i}") --fixed 2000 --confs $numconfs 2>&1 > "gen_pandas_${i}" &
done

# wait all threads
FAIL=0
for job in $(jobs -p)
do
    echo "waiting for job '$job' ..."
    wait "${job}" || FAIL=$((FAIL+=1))
done

# check thread output
rm error
for (( i = 0; i < numthreads; i++))
do
    egrep xzgrep --color -i -e "assert.*fail" -e "signal" -e "error" -e "kill" -e "terminate" "gen_pandas_${i}" | tee -a error
done

# check for FAILs
if [ "$FAIL" == "0" ];
then
    echo "All went well!"
else
    echo "FAIL of one of the threads! ($FAIL)"
    exit -1
fi

# exit in case of errors
if [[ -s $(diff error) ]]; then
    echo "OK, no errors"
else
    echo "ERROR: Issues occurred!"
    exit -1
fi

for (( CONF = 0; CONF < numconfs; CONF++))
do
    rm combine_out_short_${CONF}.dat
    rm combine_out_long_${CONF}.dat

    ./combine_dats.py -o "comb-short-conf-${CONF}.dat" ${location}/*-short-conf-${CONF}.dat > "combine_out_short_${CONF}"
    ./combine_dats.py -o "comb-long-conf-${CONF}.dat"  ${location}/*-long-conf-${CONF}.dat  > "combine_out_long_${CONF}"
done

for (( i = 0; i < numthreads; i++))
do
    egrep xzgrep --color -i -e "assert.*fail" -e "signal" -e "error" -e "kill" -e "terminate" "gen_pandas_${i}" | tee -a error
done

mkdir ../src/predict
rm ../src/predict/*.h
for (( CONF = 0; CONF < numconfs; CONF++))
do
    ./predict.py "comb-short-conf-${CONF}.dat" --name short --basedir "../src/predict/" --final --forest --split 0.02 --clusters 9 --conf "${CONF}" --clustmin 0.03
    ./predict.py "comb-long-conf-${CONF}.dat" --name long  --basedir "../src/predict/" --final --forest --split 0.02 --clusters 9 --conf "${CONF}" --clustmin 0.03
done
